#include "cli_helper.hpp"

#include <iomanip>
#include <iostream>

#include "../domain/policy_status.hpp"
#include "../domain/strops.hpp"
#include "utils.hpp"

namespace insura::cli {

std::string promptRequired(std::string_view prompt) {
  std::string value;
  do {
    std::cout << prompt;
    std::getline(std::cin, value);
    value = insura::domain::strops::trim(value);
    if (value.empty()) std::cout << "  This field is required.\n";
  } while (value.empty());
  return value;
}

std::optional<std::string> promptOptional(std::string_view prompt) {
  std::string value;
  std::cout << prompt;
  std::getline(std::cin, value);
  value = insura::domain::strops::trim(value);
  if (value.empty()) return std::nullopt;
  return value;
}

domain::Client selectClient(const std::vector<domain::Client>& clients) {
  for (std::size_t i = 0; i < clients.size(); ++i) {
    std::cout << "  [" << (i + 1) << "] " << clients[i].getFirstName() << " "
              << clients[i].getLastName() << " (" << clients[i].getEmail()
              << ")\n";
  }
  while (true) {
    std::cout << "\nSelect a client (1-" << clients.size() << "): ";
    std::string input;
    std::getline(std::cin, input);
    if (!insura::utils::isDigitsOnly(input)) {
      std::cout << "  Please enter a valid number.\n";
      continue;
    }
    int index = std::stoi(input);
    if (index < 1 || index > static_cast<int>(clients.size())) {
      std::cout << "  Number out of range.\n";
      continue;
    }
    return clients[static_cast<std::size_t>(index - 1)];
  }
}

std::optional<domain::Client> resolveClient(service::ClientService& service) {
  std::string term;
  do {
    std::cout << "Search: ";
    std::getline(std::cin, term);

    /* TODO: decide whether an empty term should re-prompt or exit. Currently
     * an empty term loops silently; a non-empty term that finds nothing exits
     * with nullopt. The caller (view/edit/delete) decides how to handle that. */
    term = insura::domain::strops::trim(term);
    if (term.empty()) continue;

    std::vector<domain::Client> found = service.searchClients(term);

    if (found.empty()) {
      std::cout << "No contact found.\n";
    } else if (found.size() == 1) {
      return found.at(0);
    } else {
      return selectClient(found);
    }
  } while (term.empty());

  return std::nullopt;
}

domain::Policy selectPolicy(const std::vector<domain::Policy>& policies) {
  for (std::size_t i = 0; i < policies.size(); ++i) {
    std::cout << "  [" << (i + 1) << "] "
              << std::left << std::setw(10)
              << insura::domain::policyTypeToString(policies[i].getPolicyType())
              << std::setw(20)
              << ("since " + policies[i].getPolicyStartDate())
              << insura::domain::policyStatusToString(
                     policies[i].getPolicyStatus())
              << '\n';
  }
  while (true) {
    std::cout << "\nSelect a policy (1-" << policies.size() << "): ";
    std::string input;
    std::getline(std::cin, input);
    if (!insura::utils::isDigitsOnly(input)) {
      std::cout << "  Please enter a valid number.\n";
      continue;
    }
    int index = std::stoi(input);
    if (index < 1 || index > static_cast<int>(policies.size())) {
      std::cout << "  Number out of range.\n";
      continue;
    }
    return policies[static_cast<std::size_t>(index - 1)];
  }
}

std::optional<std::pair<domain::Policy, domain::Client>> resolvePolicy(
    service::PolicyService& policy_service,
    service::ClientService& client_service) {

  auto client = resolveClient(client_service);
  if (!client) return std::nullopt;

  std::vector<domain::Policy> policies =
      policy_service.searchByClient(client->getUuid());

  if (policies.empty()) {
    std::cout << "No policies found for this client.\n";
    return std::nullopt;
  }

  if (policies.size() == 1) return std::pair{policies.at(0), *client};

  std::cout << "\nClient: " << client->getFirstName() << " "
            << client->getLastName() << " (" << client->getEmail() << ")\n\n";

  return std::pair{selectPolicy(policies), *client};
}

}  // namespace insura::cli
