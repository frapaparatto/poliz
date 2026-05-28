#include "cli_helper.hpp"

#include <iomanip>
#include <iostream>

#include "../domain/interaction_status.hpp"
#include "../domain/policy_status.hpp"
#include "../domain/strops.hpp"
#include "../domain/utils.hpp"

namespace insura::cli {

void pause() {
  std::string option;
  std::cout << "\nPress Enter to continue...";
  std::getline(std::cin, option);
}

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

bool confirmClient(const domain::Client& client, std::string_view entity) {
  std::cout << "First name:  " << client.getFirstName() << '\n'
            << "Last name:   " << client.getLastName() << '\n'
            << "Email:       " << client.getEmail() << '\n'
            << "\nAdd " << entity << " for this client? (Y/n): ";
  std::string choice;
  std::getline(std::cin, choice);
  return choice != "n";
}

std::optional<domain::Client> resolveClient(service::ClientService& service) {
  std::cout << "\nSearch: ";
  std::string term;
  std::getline(std::cin, term);
  term = insura::domain::strops::trim(term);

  if (term.empty()) return std::nullopt;

  std::vector<domain::Client> found = service.searchClients(term);
  if (found.empty()) {
    std::cout << "No contacts found.\n";
    return std::nullopt;
  }
  if (found.size() == 1) return found.at(0);
  return selectClient(found);
}

domain::Policy selectPolicy(const std::vector<domain::Policy>& policies) {
  for (std::size_t i = 0; i < policies.size(); ++i) {
    std::cout << "  [" << (i + 1) << "] " << std::left << std::setw(10)
              << insura::domain::policyTypeToString(policies[i].getPolicyType())
              << std::setw(20) << ("since " + policies[i].getPolicyStartDate())
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

std::string truncate(const std::string& s, std::size_t max) {
  if (s.empty()) return "";
  if (s.size() <= max) return s;
  return s.substr(0, max - 5) + "...";
}

std::string truncate(const std::optional<std::string>& opt, std::size_t max) {
  if (!opt.has_value()) return "";
  return truncate(*opt, max);
}

std::unique_ptr<domain::Interaction> selectInteraction(
    const std::vector<std::unique_ptr<domain::Interaction>>& interactions) {
  for (std::size_t i = 0; i < interactions.size(); ++i) {
    std::cout << "  [" << (i + 1) << "] " << std::left << std::setw(14)
              << insura::domain::interactionTypeToString(
                     interactions[i]->getType())
              << interactions[i]->getDate() << '\n';
  }
  while (true) {
    std::cout << "\nSelect an interaction (1-" << interactions.size() << "): ";
    std::string input;
    std::getline(std::cin, input);
    if (!insura::utils::isDigitsOnly(input)) {
      std::cout << "  Please enter a valid number.\n";
      continue;
    }
    int index = std::stoi(input);
    if (index < 1 || index > static_cast<int>(interactions.size())) {
      std::cout << "  Number out of range.\n";
      continue;
    }
    return interactions[static_cast<std::size_t>(index - 1)]->clone();
  }
}

std::optional<std::pair<std::unique_ptr<domain::Interaction>, domain::Client>>
resolveInteraction(service::InteractionService& interaction_service,
                   service::ClientService& client_service) {
  auto client = resolveClient(client_service);
  if (!client) return std::nullopt;

  auto interactions = interaction_service.searchByClient(client->getUuid());
  if (interactions.empty()) {
    std::cout << "No interactions found for this client.\n";
    return std::nullopt;
  }

  if (interactions.size() == 1)
    return std::pair{interactions.at(0)->clone(), *client};

  std::cout << "\nClient: " << client->getFirstName() << " "
            << client->getLastName() << " (" << client->getEmail() << ")\n\n";

  return std::pair{selectInteraction(interactions), *client};
}

}  // namespace insura::cli
