#include "policy_controller.hpp"

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

/* CLI-layer format validators: kept here for immediate re-prompt UX.
 * Domain-layer checks in PolicyService remain the authoritative last
 * line of defence and must not be removed. */

/* ordered to match enum declaration; index i corresponds to menu choice i+1 */
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

/* Returns nullopt when the user presses Enter to accept the default. For
 * cmdAdd the caller treats nullopt as PENDING; for cmdEdit it means keep
 * the current value. */
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

/*
 * Mirrors the pricing table from PolicyService::calculateAmount and ADR-019.
 * Used here for the confirmation preview only: the authoritative calculation
 * stays in the service layer. Row order must match PolicyType enum order.
 */
constexpr double kDisplayPricingTable[4][4] = {
    {350.00, 650.00, 1200.00, 1700.00}, /* AUTO   */
    {150.00, 280.00, 520.00, 740.00},   /* LIFE   */
    {120.00, 220.00, 400.00, 570.00},   /* HOME   */
    {200.00, 380.00, 700.00, 1000.00},  /* HEALTH */
};
static_assert(std::size(kDisplayPricingTable) == 4,
              "Display pricing table rows must match PolicyType count (4)");

double previewPolicyAmount(insura::domain::Policy::PolicyType type,
                           int duration_months) {
  int col;
  switch (duration_months) {
    case 6:
      col = 0;
      break;
    case 12:
      col = 1;
      break;
    case 24:
      col = 2;
      break;
    case 36:
      col = 3;
      break;
    default:
      return 0.0;
  }
  return kDisplayPricingTable[static_cast<int>(type)][col];
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
    std::cout << "\nUnknown commands\n";
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
  /* Step 1: resolve client; UUID stored on the policy, name never persisted */
  std::cout << "Client:\n";
  auto client = resolveClient(client_service_);
  if (!client) return;

  std::cout << '\n';
  if (!PolicyView::confirmClient(*client)) return;

  domain::PolicyData data;
  data.client_uuid = client->getUuid();

  /* Step 2: policy type */
  std::cout << "\nPolicy type:\n";
  data.policy_type_ = promptPolicyType();

  /* Step 3: duration */
  std::cout << "Duration:\n";
  data.duration_months = promptPolicyDuration();

  /* Step 4: start date: validated before the preview is shown */
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

  /* Show derived values before the user commits. */
  auto end_date = insura::utils::date::calculateEndDate(data.start_date,
                                                        data.duration_months);
  double preview_amount =
      previewPolicyAmount(data.policy_type_, data.duration_months);

  std::cout << "\n  End date:  " << end_date << '\n'
            << "  Amount:    " << std::fixed << std::setprecision(2)
            << preview_amount << " EUR\n\n";

  /* Step 5: optional status, nullopt lets the service apply the default
   * (PENDING) per ADR-019 */
  std::cout << "Status:\n";
  data.policy_status = promptPolicyStatus();

  /* Step 6: optional notes */
  data.notes = promptOptional("Notes (optional): ");

  try {
    policy_service_.addPolicy(data);
    std::cout << "\nPolicy added successfully.\n";
  } catch (const std::invalid_argument& e) {
    std::cout << "\n  Error: " << e.what() << '\n';
  }
}

void PolicyController::cmdList() {
  std::unordered_map<std::string, std::string> name_map;
  for (const auto& c : client_repo_.findAll()) {
    name_map[c.getUuid()] = c.getFirstName() + " " + c.getLastName();
  }

  std::cout << '\n';
  PolicyView::displayAll(policy_repo_.findAll(), name_map);
}

void PolicyController::cmdSearch() {
  /* TODO: search vs view distinction — search shows a results list;
   * view resolves to exactly one policy. */

  std::cout << "Search by:\n"
            << "  1. Client\n"
            << "  2. Type\n"
            << "  3. Status\n"
            << "> ";
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
    std::cout << "  Invalid choice.\n";
    return;
  }

  if (results.empty()) {
    std::cout << "No policies found.\n";
    return;
  }

  std::unordered_map<std::string, std::string> name_map;
  for (const auto& c : client_repo_.findAll()) {
    name_map[c.getUuid()] = c.getFirstName() + " " + c.getLastName();
  }

  std::cout << '\n';
  PolicyView::displayAll(results, name_map);

  /* Optional drill-down into one entry for full detail */
  if (results.size() == 1) {
    std::cout << "\nView details? (Enter to view, n to skip): ";
    std::string input;
    std::getline(std::cin, input);
    input = insura::domain::strops::trim(input);
    if (input != "n") {
      std::cout << '\n';
      PolicyView::displayOne(results[0]);
    }
  } else {
    std::cout << "\nView details for a specific entry? (1-" << results.size()
              << ", Enter to skip): ";
    std::string input;
    std::getline(std::cin, input);
    input = insura::domain::strops::trim(input);
    if (input.empty()) return;
    if (!insura::utils::isDigitsOnly(input)) {
      std::cout << "  Invalid input.\n";
      return;
    }
    int idx = std::stoi(input);
    if (idx < 1 || idx > static_cast<int>(results.size())) {
      std::cout << "  Out of range.\n";
      return;
    }
    std::cout << '\n';
    PolicyView::displayOne(results[static_cast<std::size_t>(idx - 1)]);
  }
}

void PolicyController::cmdView() {
  auto policy = resolvePolicy(policy_service_, client_service_);
  if (!policy) return;
  std::cout << '\n';
  PolicyView::displayOne(*policy);
}

void PolicyController::cmdEdit() {
  auto policy = resolvePolicy(policy_service_, client_service_);
  if (!policy) return;

  std::cout << '\n';
  PolicyView::displayOne(*policy);
  domain::PolicyData updated = promptPolicyEditData(*policy);

  try {
    policy_service_.editPolicy(policy->getUuid(), updated);
    std::cout << "\nPolicy updated successfully.\n";
  } catch (const std::invalid_argument& e) {
    std::cout << "\n  Error: " << e.what() << '\n';
  }
}

void PolicyController::cmdDelete() {
  auto policy = resolvePolicy(policy_service_, client_service_);
  if (!policy) return;

  std::cout << '\n';
  PolicyView::displayOne(*policy);
  std::string choice;
  std::cout << "\nAre you sure? (Y/n): ";
  std::getline(std::cin, choice);

  if (choice == "n") return;

  try {
    policy_service_.deletePolicy(policy->getUuid());
    std::cout << "\nPolicy deleted successfully.\n";
  } catch (const std::invalid_argument& e) {
    std::cout << "\n  Error: " << e.what() << '\n';
  }
}

}  // namespace insura::cli
