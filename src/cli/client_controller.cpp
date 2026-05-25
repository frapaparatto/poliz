#include "client_controller.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>

#include "../domain/client_status.hpp"
#include "../domain/policy_status.hpp"
#include "../domain/strops.hpp"
#include "../domain/utils.hpp"
#include "./client_view.hpp"
#include "cli_helper.hpp"

namespace {

constexpr int kPTypeWidth = 14;
constexpr int kPStatusWidth = 14;
constexpr int kPAmountWidth = 14;
constexpr int kDateLen = 10;    // fixed "YYYY-MM-DD"
constexpr int kArrowWidth = 3;  // " → " visual width
const std::string kPolicySep(kPTypeWidth + kPStatusWidth + kPAmountWidth +
                                 kDateLen * 2 + kArrowWidth,
                             '-');

std::string fmtAmount(double v) {
  std::ostringstream ss;
  ss << "€" << std::fixed << std::setprecision(2) << v;
  return ss.str();
}

void displayClientPolicies(
    const std::vector<insura::domain::Policy>& policies) {
  std::cout << std::left << std::setw(kPTypeWidth) << "Type"
            << std::setw(kPStatusWidth) << "Status" << std::setw(kPAmountWidth)
            << "Amount"
            << "Start → End\n"
            << kPolicySep << '\n';

  for (const auto& p : policies) {
    const std::string amt = fmtAmount(p.getPolicyAmount());
    const std::string end_date = p.getPolicyEndDate().value_or("N/A");
    const std::string range = p.getPolicyStartDate() + " → " + end_date;

    std::cout << std::left << std::setw(kPTypeWidth)
              << insura::domain::policyTypeToString(p.getPolicyType())
              << std::setw(kPStatusWidth)
              << insura::domain::policyStatusToString(p.getPolicyStatus())
              << std::setw(kPAmountWidth + 2) << amt << range << '\n';
  }
}

/* CLI-layer format validators: kept here for immediate re-prompt UX.
 * Domain-layer checks in client.cpp remain the authoritative last line of
 * defence and must not be removed. */

/* ordered to match enum declaration; index i corresponds to menu choice i+1 */
constexpr insura::domain::Client::ClientStatus kStatusOptions[] = {
    insura::domain::Client::ClientStatus::NEW,
    insura::domain::Client::ClientStatus::CONTACTED,
    insura::domain::Client::ClientStatus::IN_PROGRESS,
    insura::domain::Client::ClientStatus::OPEN_DEAL,
    insura::domain::Client::ClientStatus::ATTEMPTED_TO_CONTACT,
    insura::domain::Client::ClientStatus::CLOSED_WON,
    insura::domain::Client::ClientStatus::CLOSED_LOST,
};
constexpr int kStatusCount = std::size(kStatusOptions);

std::optional<insura::domain::Client::ClientStatus> promptStatus() {
  for (int i = 0; i < kStatusCount; ++i) {
    std::cout << "  " << (i + 1) << ". "
              << insura::domain::statusToString(kStatusOptions[i]);
    if (kStatusOptions[i] == insura::domain::Client::ClientStatus::NEW)
      std::cout << " (default)";
    std::cout << "\n";
  }
  while (true) {
    std::cout << "Select status (1-7, Press Enter for default): ";
    std::string input;
    std::getline(std::cin, input);
    input = insura::domain::strops::trim(input);
    if (input.empty()) return std::nullopt;
    if (input.size() == 1 && input[0] >= '1' && input[0] <= '7') {
      return kStatusOptions[static_cast<std::size_t>(input[0] - '1')];
    }
    std::cout << "  Please enter a number between 1 and 7.\n";
  }
}

}  // namespace

namespace insura::cli {

ClientController::ClientController(service::ClientService& client_service,
                                   domain::IClientRepository& repo,
                                   service::PolicyService& policy_service)
    : client_service_(client_service),
      repo_(repo),
      policy_service_(policy_service) {
  commands_["add"] = [this]() { cmdAdd(); };
  commands_["search"] = [this]() { cmdSearch(); };
  commands_["list"] = [this]() { cmdList(); };
  commands_["view"] = [this]() { cmdView(); };
  commands_["edit"] = [this]() { cmdEdit(); };
  commands_["delete"] = [this]() { cmdDelete(); };
}

void ClientController::save() { repo_.save(); }

bool ClientController::isDirty() const { return repo_.isDirty(); }

void ClientController::execute(const std::string& cmd) {
  auto it = commands_.find(cmd);

  if (it != commands_.end())
    it->second();
  else
    std::cout << "\nUnknown command.\n";

  pause();
}

void ClientController::cmdView() {
  auto client = resolveClient(client_service_);
  if (!client) return;

  std::cout << '\n';
  ClientView::displayOne(*client);

  auto policies = policy_service_.searchByClient(client->getUuid());
  if (!policies.empty()) {
    std::cout << "\nPolicies:\n";
    displayClientPolicies(policies);
  }
}

void ClientController::cmdAdd() {
  domain::ClientData data;

  /* Required fields */
  std::string first = promptRequired("First name: ");
  data.first_name = insura::domain::strops::capitalize(first);
  std::string last = promptRequired("Last name: ");
  data.last_name = insura::domain::strops::capitalize(last);

  while (true) {
    std::cout << "Email: ";
    std::string email;
    std::getline(std::cin, email);
    email = insura::domain::strops::trim(email);
    if (email.empty()) {
      std::cout << "  This field is required.\n";
      continue;
    }
    if (!insura::utils::isValidEmail(email)) {
      std::cout << "  Invalid email format.\n";
      continue;
    }
    data.email = insura::domain::strops::lower(email);
    break;
  }

  /* Optional fields */
  while (true) {
    auto phone = promptOptional("Phone (optional, digits only): ");
    if (!phone.has_value()) break;
    if (!insura::utils::isValidPhone(*phone)) {
      std::cout << "  Phone must contain digits only.\n";
      continue;
    }
    data.phone = std::move(phone);
    break;
  }

  std::optional<std::string> job = promptOptional("Job title (optional): ");
  data.job_title = job ? insura::domain::strops::capitalize(*job) : job;

  std::optional<std::string> company = promptOptional("Company (optional): ");
  data.company =
      company ? insura::domain::strops::capitalize(*company) : company;

  auto address = promptOptional("Address (optional): ");
  data.address =
      address ? insura::domain::strops::capitalize(*address) : address;
  std::optional<std::string> city = promptOptional("City (optional): ");
  data.city = city ? insura::domain::strops::capitalize(*city) : city;

  while (true) {
    auto postal_code = promptOptional("Postal code (optional, digits only): ");
    if (!postal_code.has_value()) break;
    if (!insura::utils::isDigitsOnly(*postal_code)) {
      std::cout << "  Postal code must contain digits only.\n";
      continue;
    }
    data.postal_code = std::move(postal_code);
    break;
  }

  std::cout << "Status:\n";
  data.lead_status = promptStatus();

  auto notes = promptOptional("Notes (optional): ");
  data.notes = notes ? insura::domain::strops::capitalize(*notes) : notes;

  try {
    client_service_.addClient(data);
    std::cout << "\nClient added successfully.\n";
  } catch (const std::invalid_argument& e) {
    std::cout << "\nError: " << e.what() << "\n";
  }
}

void ClientController::cmdSearch() {
  std::string term;
  std::cout << "Search: ";
  std::getline(std::cin, term);
  term = insura::domain::strops::trim(term);
  if (term.empty()) return;

  std::vector<domain::Client> found = client_service_.searchClients(term);
  if (found.empty()) {
    std::cout << "No contacts found.\n";
    return;
  }

  std::cout << '\n';
  ClientView::displayAll(found);

  /* Optional drill-down into one entry for full detail. */
  if (found.size() == 1) {
    std::cout << "\nView details? (Enter to view, n to skip): ";
    std::string input;
    std::getline(std::cin, input);
    input = insura::domain::strops::trim(input);
    if (input != "n") {
      std::cout << '\n';
      ClientView::displayOne(found[0]);
    }
  } else {
    std::cout << "\nView details for a specific entry? (1-" << found.size()
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
    if (idx < 1 || idx > static_cast<int>(found.size())) {
      std::cout << "Out of range.\n";
      return;
    }
    std::cout << '\n';
    ClientView::displayOne(found[static_cast<std::size_t>(idx - 1)]);
  }
}

/* This function both prompts the user for updated field values and populates
 * the ClientData struct, intentionally kept together, same pattern as
 * cmdAdd. If a second non-CLI caller appears, the two responsibilities can
 * be separated at that point. */
domain::ClientData ClientController::promptEditData(
    const domain::Client& current) {
  domain::ClientData updated_data;

  {
    /* Required fields, shown with current value; leave blank to keep */
    std::optional<std::string> first =
        promptOptional("First name [" + current.getFirstName() + "]: ");
    if (first)
      updated_data.first_name = insura::domain::strops::capitalize(*first);
  }

  {
    std::optional<std::string> last =
        promptOptional("Last name [" + current.getLastName() + "]: ");
    if (last)
      updated_data.last_name = insura::domain::strops::capitalize(*last);
  }

  while (true) {
    std::optional<std::string> email =
        promptOptional("Email [" + current.getEmail() + "]: ");
    if (!email)
      break;
    else if (!insura::utils::isValidEmail(*email)) {
      std::cout << "  Invalid email format.\n";
      continue;
    } else {
      updated_data.email = insura::domain::strops::lower(*email);
      break;
    }
  }

  while (true) {
    std::string prompt = current.getPhone()
                             ? "Phone [" + current.getPhone().value() + "]: "
                             : "Phone (optional): ";

    auto phone = promptOptional(prompt);
    if (!phone.has_value()) break;

    if (!insura::utils::isValidPhone(*phone)) {
      std::cout << "  Phone must contain digits only.\n";
      continue;
    }
    updated_data.phone = std::move(phone);
    break;
  }

  {
    std::string prompt =
        current.getJobTitle()
            ? "Job title [" + current.getJobTitle().value() + "]: "
            : "Job title (optional): ";
    auto job_title = promptOptional(prompt);
    updated_data.job_title =
        job_title ? insura::domain::strops::capitalize(*job_title) : job_title;
  }

  {
    std::string prompt =
        current.getCompany()
            ? "Company [" + current.getCompany().value() + "]: "
            : "Company (optional): ";
    auto company = promptOptional(prompt);
    updated_data.company =
        company ? insura::domain::strops::capitalize(*company) : company;
  }

  {
    std::string prompt =
        current.getAddress()
            ? "Address [" + current.getAddress().value() + "]: "
            : "Address (optional): ";
    auto address = promptOptional(prompt);
    updated_data.address =
        address ? insura::domain::strops::capitalize(*address) : address;
  }

  {
    std::string prompt = current.getCity()
                             ? "City [" + current.getCity().value() + "]: "
                             : "City (optional): ";
    auto city = promptOptional(prompt);
    updated_data.city = city ? insura::domain::strops::capitalize(*city) : city;
  }

  while (true) {
    std::string prompt =
        current.getPostalCode()
            ? "Postal code [" + current.getPostalCode().value() + "]: "
            : "Postal code (optional, digits only): ";
    auto postal_code = promptOptional(prompt);
    if (!postal_code.has_value()) break;
    if (!insura::utils::isDigitsOnly(*postal_code)) {
      std::cout << "  Postal code must contain digits only.\n";
      continue;
    }
    updated_data.postal_code = std::move(postal_code);
    break;
  }

  {
    std::cout << "Status ["
              << insura::domain::statusToString(current.getStatus())
              << "] (1-7 to change, Enter to keep):\n";
    for (int i = 0; i < kStatusCount; ++i) {
      std::cout << "  " << (i + 1) << ". "
                << insura::domain::statusToString(kStatusOptions[i]) << "\n";
    }
    while (true) {
      std::cout << "> ";
      std::string input;
      std::getline(std::cin, input);
      input = insura::domain::strops::trim(input);
      if (input.empty()) break;
      if (input.size() == 1 && input[0] >= '1' && input[0] <= '7') {
        updated_data.lead_status =
            kStatusOptions[static_cast<std::size_t>(input[0] - '1')];
        break;
      }
      std::cout << "  Please enter a number between 1 and 7, or press Enter "
                   "to keep.\n";
    }
  }

  {
    std::string prompt = current.getNotes()
                             ? "Notes [" + current.getNotes().value() + "]: "
                             : "Notes (optional): ";
    auto notes = promptOptional(prompt);
    updated_data.notes =
        notes ? insura::domain::strops::capitalize(*notes) : notes;
  }

  return updated_data;
}

void ClientController::cmdList() {
  std::cout << '\n';
  ClientView::displayAll(repo_.findAll());
}

void ClientController::cmdDelete() {
  auto client = resolveClient(client_service_);
  if (!client) return;

  std::cout << '\n';
  ClientView::displayOne(*client);
  std::string choice;
  std::cout << "\nAre you sure? (Y/n): ";
  std::getline(std::cin, choice);

  if (choice == "n") return;

  try {
    client_service_.deleteClient(client->getUuid());
    std::cout << "\nClient eliminated successfully.\n";
  } catch (const std::invalid_argument& e) {
    std::cout << "\nError: " << e.what() << "\n";
  }
}

void ClientController::cmdEdit() {
  auto client = resolveClient(client_service_);
  if (!client) return;

  std::cout << '\n';
  ClientView::displayOne(*client);
  std::string choice;
  std::cout << "\nEdit this record? (Y/n): ";
  std::getline(std::cin, choice);
  if (choice == "n") return;

  domain::ClientData updated = promptEditData(*client);
  try {
    /* TODO: evaluate whether "No updates made." should stay or just show nothing. */
    if (client_service_.editClient(client->getUuid(), updated))
      std::cout << "\nClient updated successfully.\n";
    else
      std::cout << "\nNo updates made.\n";
  } catch (const std::invalid_argument& e) {
    std::cout << "\nError: " << e.what() << "\n";
  }
}

}  // namespace insura::cli
