#include "policy_controller.hpp"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>

#include "../domain/policy_status.hpp"
#include "../domain/strops.hpp"
#include "../domain/utils.hpp"
#include "./policy_view.hpp"
#include "cli_helper.hpp"

namespace {

constexpr insura::domain::Policy::PolicyType kPolicyTypeOptions[] = {
    insura::domain::Policy::PolicyType::AUTO,
    insura::domain::Policy::PolicyType::LIFE,
    insura::domain::Policy::PolicyType::HOME,
    insura::domain::Policy::PolicyType::HEALTH,
};
constexpr int kPolicyTypeCount = std::size(kPolicyTypeOptions);

constexpr insura::domain::Policy::PolicyStatus kPolicyStatusOptions[] = {
    insura::domain::Policy::PolicyStatus::ACTIVE,
    insura::domain::Policy::PolicyStatus::EXPIRED,
    insura::domain::Policy::PolicyStatus::CANCELLED,
    insura::domain::Policy::PolicyStatus::PENDING,
};
constexpr int kPolicyStatusCount = std::size(kPolicyStatusOptions);

constexpr int kDurationOptions[] = {6, 12, 24, 36};
constexpr int kDurationCount = std::size(kDurationOptions);

insura::domain::Policy::PolicyType promptPolicyType() {
  for (int i = 0; i < kPolicyTypeCount; ++i) {
    std::cout << "  " << (i + 1) << ". "
              << insura::domain::policyTypeToString(kPolicyTypeOptions[i])
              << '\n';
  }
  while (true) {
    std::cout << "Select type (1-4): ";
    std::string input;
    std::getline(std::cin, input);
    input = insura::domain::strops::trim(input);
    if (input.size() == 1 && input[0] >= '1' && input[0] <= '4') {
      return kPolicyTypeOptions[static_cast<std::size_t>(input[0] - '1')];
    }
    std::cout << "  Please enter a number between 1 and 4.\n";
  }
}

int promptPolicyDuration() {
  for (int i = 0; i < kDurationCount; ++i) {
    std::cout << "  " << (i + 1) << ". " << kDurationOptions[i] << " months\n";
  }
  while (true) {
    std::cout << "Select duration (1-4): ";
    std::string input;
    std::getline(std::cin, input);
    input = insura::domain::strops::trim(input);
    if (input.size() == 1 && input[0] >= '1' && input[0] <= '4') {
      return kDurationOptions[static_cast<std::size_t>(input[0] - '1')];
    }
    std::cout << "  Please enter a number between 1 and 4.\n";
  }
}

/* Returns nullopt when the user presses Enter to accept the default */
std::optional<insura::domain::Policy::PolicyStatus> promptPolicyStatus() {
  for (int i = 0; i < kPolicyStatusCount; ++i) {
    std::cout << "  " << (i + 1) << ". "
              << insura::domain::policyStatusToString(kPolicyStatusOptions[i]);
    if (kPolicyStatusOptions[i] ==
        insura::domain::Policy::PolicyStatus::PENDING)
      std::cout << " (default)";
    std::cout << '\n';
  }
  while (true) {
    std::cout << "Select status (1-4, Enter for default): ";
    std::string input;
    std::getline(std::cin, input);
    input = insura::domain::strops::trim(input);
    if (input.empty()) return std::nullopt;
    if (input.size() == 1 && input[0] >= '1' && input[0] <= '4') {
      return kPolicyStatusOptions[static_cast<std::size_t>(input[0] - '1')];
    }
    std::cout << "  Please enter a number between 1 and 4.\n";
  }
}

}  // namespace

namespace insura::cli {

PolicyController::PolicyController(service::PolicyService& policy_service,
                                   domain::IPolicyRepository& policy_repo,
                                   service::ClientService& client_service,
                                   domain::IClientRepository& client_repo)
    : policy_service_(policy_service),
      policy_repo_(policy_repo),
      client_service_(client_service),
      client_repo_(client_repo) {
  commands_["add"] = [this]() { cmdAdd(); };
  commands_["search"] = [this]() { cmdSearch(); };
  commands_["list"] = [this]() { cmdList(); };
  commands_["view"] = [this]() { cmdView(); };
  commands_["edit"] = [this]() { cmdEdit(); };
  commands_["delete"] = [this]() { cmdDelete(); };
}

void PolicyController::execute(const std::string& cmd) {
  auto it = commands_.find(cmd);
  if (it != commands_.end())
    it->second();
  else
    std::cout << "\nUnknown command.\n";

  pause();
}

void PolicyController::save() { policy_repo_.save(); }

bool PolicyController::isDirty() const { return policy_repo_.isDirty(); }

/* Prompts only for the two fields mutable after creation per ADR-019.
 * All contract-defining fields (type, dates, amount, client) are immutable
 * and intentionally absent from this form. */
domain::PolicyData PolicyController::promptPolicyEditData(
    const domain::Policy& current) {
  domain::PolicyData updated;

  {
    std::cout << "Status ["
              << insura::domain::policyStatusToString(current.getPolicyStatus())
              << "] (1-4 to change, Enter to keep):\n";
    for (int i = 0; i < kPolicyStatusCount; ++i) {
      std::cout << "  " << (i + 1) << ". "
                << insura::domain::policyStatusToString(kPolicyStatusOptions[i])
                << '\n';
    }
    while (true) {
      std::cout << "> ";
      std::string input;
      std::getline(std::cin, input);
      input = insura::domain::strops::trim(input);
      if (input.empty()) break;
      if (input.size() == 1 && input[0] >= '1' && input[0] <= '4') {
        updated.policy_status =
            kPolicyStatusOptions[static_cast<std::size_t>(input[0] - '1')];
        break;
      }
      std::cout << "  Please enter a number between 1 and 4, or press Enter "
                   "to keep.\n";
    }
  }

  {
    std::string prompt =
        current.getPolicyNotes()
            ? "Notes [" + current.getPolicyNotes().value() + "]: "
            : "Notes (optional): ";
    updated.notes = promptOptional(prompt);
  }

  return updated;
}

void PolicyController::cmdAdd() {
  std::cout << "Client:\n";
  auto client = resolveClient(client_service_);
  if (!client) return;

  std::cout << '\n';
  if (!confirmClient(*client, "policy")) return;

  domain::PolicyData data;
  data.client_uuid = client->getUuid();

  std::cout << "\nPolicy type:\n";
  data.policy_type_ = promptPolicyType();

  std::cout << "Duration:\n";
  data.duration_months = promptPolicyDuration();

  while (true) {
    std::cout << "Start date (YYYY-MM-DD, Enter for today): ";
    std::string date;
    std::getline(std::cin, date);
    date = insura::domain::strops::trim(date);
    if (date.empty()) {
      data.start_date = insura::utils::date::today();
      break;
    }
    if (!insura::utils::date::isValidDate(date)) {
      std::cout << "  Invalid date. Expected format: YYYY-MM-DD.\n";
      continue;
    }
    data.start_date = date;
    break;
  }

  auto end_date = insura::utils::date::calculateEndDate(data.start_date,
                                                        data.duration_months);
  double preview_amount =
      policy_service_.calculateAmount(data.policy_type_, data.duration_months);

  std::cout << "\n  End date:  " << end_date << '\n'
            << "  Amount:    " << std::fixed << std::setprecision(2)
            << preview_amount << " EUR\n\n";

  std::cout << "Status:\n";
  data.policy_status = promptPolicyStatus();

  data.notes = promptOptional("Notes (optional): ");

  try {
    policy_service_.addPolicy(data);
    std::cout << "\nPolicy added successfully.\n";
  } catch (const std::invalid_argument& e) {
    std::cout << "\nError: " << e.what() << '\n';
  }
}

void PolicyController::cmdList() {
  std::unordered_map<std::string, std::string> name_map;
  auto policies = policy_repo_.findAll();

  /* check used in order to build the name map only for
   * client that has policies instead of creating a
   * name map for all clients
   */
  if (policies.empty()) {
    std::cout << "No policies found.\n";
    return;
  }

  for (const auto& p : policies) {
    if (name_map.count(p.getClientUuid()) == 0) {
      auto client = client_repo_.findByUuid(p.getClientUuid());
      assert(client.has_value() && "policy references non-existent client");
      name_map[client->getUuid()] =
          client->getFirstName() + " " + client->getLastName();
    }
  }

  std::cout << '\n';
  PolicyView::displayAll(policies, name_map);
}

void PolicyController::cmdSearch() {
  std::cout << "\nSearch by:\n"
            << "  1. Client\n"
            << "  2. Type\n"
            << "  3. Status\n"
            << "\n> ";

  std::string choice;
  std::getline(std::cin, choice);
  choice = insura::domain::strops::trim(choice);

  std::vector<domain::Policy> results;

  if (choice == "1") {
    std::cout << "Client:\n";
    auto client = resolveClient(client_service_);
    if (!client) return;
    results = policy_service_.searchByClient(client->getUuid());

  } else if (choice == "2") {
    std::cout << "Type:\n";
    domain::PolicyFilter filter;
    filter.type = promptPolicyType();
    results = policy_service_.searchPolicy(filter);

  } else if (choice == "3") {
    std::cout << "Status:\n";
    auto status = promptPolicyStatus();
    if (!status) {
      results = policy_repo_.findAll();
    } else {
      domain::PolicyFilter filter;
      filter.status = *status;
      results = policy_service_.searchPolicy(filter);
    }

  } else {
    std::cout << "Invalid choice.\n";
    return;
  }

  if (results.empty()) {
    std::cout << "No policies found.\n";
    return;
  }

  std::unordered_map<std::string, std::string> name_map;
  for (const auto& p : results) {
    if (name_map.count(p.getClientUuid()) == 0) {
      auto client = client_repo_.findByUuid(p.getClientUuid());
      assert(client.has_value() && "policy references non-existent client");
      name_map[client->getUuid()] =
          client->getFirstName() + " " + client->getLastName();
    }
  }

  std::cout << '\n';
  PolicyView::displayAll(results, name_map);

  /* optional: user can view detail for one of the results */
  auto clientName = [&](const domain::Policy& p) -> std::string {
    auto it = name_map.find(p.getClientUuid());
    assert(it != name_map.end() && "policy references non-existent client");
    return it->second;
  };

  if (results.size() == 1) {
    std::cout << "\nView details? (Enter to view, n to skip): ";
    std::string input;
    std::getline(std::cin, input);
    input = insura::domain::strops::trim(input);
    if (input != "n") {
      std::cout << '\n';
      PolicyView::displayOne(results[0], clientName(results[0]));
    }
  } else {
    std::cout << "\nView details for a specific entry? (1-" << results.size()
              << ", Enter to skip): ";
    std::string input;
    std::getline(std::cin, input);
    input = insura::domain::strops::trim(input);
    if (input.empty()) return;
    if (!insura::utils::isDigitsOnly(input)) {
      std::cout << "Invalid input.\n";
      return;
    }
    int idx = std::stoi(input);
    if (idx < 1 || idx > static_cast<int>(results.size())) {
      std::cout << "Out of range.\n";
      return;
    }
    const auto& p = results[static_cast<std::size_t>(idx - 1)];
    std::cout << '\n';
    PolicyView::displayOne(p, clientName(p));
  }
}

void PolicyController::cmdView() {
  auto result = resolvePolicy(policy_service_, client_service_);
  if (!result) return;
  auto& [policy, client] = *result;
  std::cout << '\n';
  PolicyView::displayOne(policy,
                         client.getFirstName() + " " + client.getLastName());
}

void PolicyController::cmdEdit() {
  auto result = resolvePolicy(policy_service_, client_service_);
  if (!result) return;
  auto& [policy, client] = *result;

  std::cout << '\n';
  PolicyView::displayOne(policy,
                         client.getFirstName() + " " + client.getLastName());
  std::string choice;
  std::cout << "\nEdit this record? (Y/n): ";
  std::getline(std::cin, choice);
  if (choice == "n") return;

  domain::PolicyData updated = promptPolicyEditData(policy);

  try {
    if (policy_service_.editPolicy(policy.getUuid(), updated))
      std::cout << "\nPolicy updated successfully.\n";
    else
      std::cout << "\nNo updates made.\n";
  } catch (const std::invalid_argument& e) {
    std::cout << "\nError: " << e.what() << '\n';
  }
}

void PolicyController::cmdDelete() {
  auto result = resolvePolicy(policy_service_, client_service_);
  if (!result) return;
  auto& [policy, client] = *result;

  std::cout << '\n';
  PolicyView::displayOne(policy,
                         client.getFirstName() + " " + client.getLastName());
  std::string choice;
  std::cout << "\nAre you sure? (Y/n): ";
  std::getline(std::cin, choice);

  if (choice == "n") return;

  try {
    policy_service_.deletePolicy(policy.getUuid());
    std::cout << "\nPolicy deleted successfully.\n";
  } catch (const std::invalid_argument& e) {
    std::cout << "\nError: " << e.what() << '\n';
  }
}

}  // namespace insura::cli
