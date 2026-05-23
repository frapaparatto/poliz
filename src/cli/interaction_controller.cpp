

#include "interaction_controller.hpp"

#include <cassert>
#include <iostream>
#include <string>

#include "../domain/interaction_status.hpp"
#include "../domain/strops.hpp"
#include "../domain/utils.hpp"
#include "cli_helper.hpp"
#include "interaction_view.hpp"

namespace {

constexpr insura::domain::Interaction::InteractionType
    kInteractionTypeOptions[] = {
        insura::domain::Interaction::InteractionType::APPOINTMENT,
        insura::domain::Interaction::InteractionType::CONTRACT,
};
constexpr int kInteractionTypeCount = 2;

constexpr int kDurationOptions[] = {30, 60, 90, 120};
constexpr int kDurationCount = std::size(kDurationOptions);

constexpr int kContractYearsOptions[] = {1, 2, 3};
constexpr int kContractYearsCount = std::size(kContractYearsOptions);

constexpr insura::domain::Contract::ContractStatus kContractStatusOptions[] = {
    insura::domain::Contract::ContractStatus::DRAFT,
    insura::domain::Contract::ContractStatus::SIGNED,
    insura::domain::Contract::ContractStatus::ACTIVE,
    insura::domain::Contract::ContractStatus::TERMINATED,
};
constexpr int kContractStatusCount = std::size(kContractStatusOptions);

std::string fmtDuration(int minutes) {
  int h = minutes / 60;
  int m = minutes % 60;
  if (h == 0) return std::to_string(m) + "min";
  if (m == 0) return std::to_string(h) + "h";
  return std::to_string(h) + "h " + std::to_string(m) + "min";
}

insura::domain::Interaction::InteractionType promptInteractionType() {
  for (int i = 0; i < kInteractionTypeCount; ++i)
    std::cout << "  " << (i + 1) << ". "
              << insura::domain::strops::capitalize(
                     insura::domain::interactionTypeToString(
                         kInteractionTypeOptions[i]))
              << '\n';
  while (true) {
    std::cout << "Select type (1-2): ";
    std::string input;
    std::getline(std::cin, input);
    input = insura::domain::strops::trim(input);
    if (input.size() == 1 && (input[0] == '1' || input[0] == '2'))
      return kInteractionTypeOptions[static_cast<std::size_t>(input[0] - '1')];
    std::cout << "  Please enter 1 or 2.\n";
  }
}

int promptDuration() {
  for (int i = 0; i < kDurationCount; ++i)
    std::cout << "  " << (i + 1) << ". " << fmtDuration(kDurationOptions[i])
              << '\n';
  while (true) {
    std::cout << "Select duration (1-4): ";
    std::string input;
    std::getline(std::cin, input);
    input = insura::domain::strops::trim(input);
    if (input.size() == 1 && input[0] >= '1' && input[0] <= '4')
      return kDurationOptions[static_cast<std::size_t>(input[0] - '1')];
    std::cout << "  Please enter a number between 1 and 4.\n";
  }
}

int promptContractYears() {
  for (int i = 0; i < kContractYearsCount; ++i)
    std::cout << "  " << (i + 1) << ". " << kContractYearsOptions[i]
              << " year(s)\n";
  while (true) {
    std::cout << "Select duration (1-3): ";
    std::string input;
    std::getline(std::cin, input);
    input = insura::domain::strops::trim(input);
    if (input.size() == 1 && input[0] >= '1' && input[0] <= '3')
      return kContractYearsOptions[static_cast<std::size_t>(input[0] - '1')];
    std::cout << "  Please enter a number between 1 and 3.\n";
  }
}

/* Returns nullopt on Enter; the service defaults to DRAFT. */
std::optional<insura::domain::Contract::ContractStatus> promptContractStatus() {
  for (int i = 0; i < kContractStatusCount; ++i) {
    std::cout << "  " << (i + 1) << ". "
              << insura::domain::contractStatusToString(
                     kContractStatusOptions[i]);
    if (kContractStatusOptions[i] ==
        insura::domain::Contract::ContractStatus::DRAFT)
      std::cout << " (default)";
    std::cout << '\n';
  }
  while (true) {
    std::cout << "Select status (1-4, Enter for default): ";
    std::string input;
    std::getline(std::cin, input);
    input = insura::domain::strops::trim(input);
    if (input.empty()) return std::nullopt;
    if (input.size() == 1 && input[0] >= '1' && input[0] <= '4')
      return kContractStatusOptions[static_cast<std::size_t>(input[0] - '1')];
    std::cout << "  Please enter a number between 1 and 4, or press Enter for "
                 "default.\n";
  }
}

}  // namespace

namespace insura::cli {
InteractionController::InteractionController(
    service::InteractionService& interaction_service,
    domain::IInteractionRepository& interaction_repo,
    service::ClientService& client_service,
    domain::IClientRepository& client_repo)
    : interaction_service_(interaction_service),
      interaction_repo_(interaction_repo),
      client_service_(client_service),
      client_repo_(client_repo) {
  commands_["add"] = [this]() { cmdAdd(); };
  commands_["search"] = [this]() { cmdSearch(); };
  commands_["list"] = [this]() { cmdList(); };
  commands_["view"] = [this]() { cmdView(); };
  commands_["edit"] = [this]() { cmdEdit(); };
  commands_["delete"] = [this]() { cmdDelete(); };
}

void InteractionController::execute(const std::string& cmd) {
  auto it = commands_.find(cmd);
  if (it != commands_.end())
    it->second();
  else
    std::cout << "\nUnknown commands\n";

  pause();
}

void InteractionController::save() { interaction_repo_.save(); }
bool InteractionController::isDirty() const {
  return interaction_repo_.isDirty();
};

void InteractionController::cmdAdd() {
  std::cout << "\nInteraction type:\n";
  auto type = promptInteractionType();

  std::cout << "\nClient:\n";
  auto client = resolveClient(client_service_);
  if (!client) return;

  std::cout << '\n';
  if (!confirmClient(*client, "interaction")) return;

  if (type == domain::Interaction::InteractionType::APPOINTMENT) {
    domain::AppointmentData data;
    data.client_uuid = client->getUuid();

    while (true) {
      std::cout << "\nDate (YYYY-MM-DD, Enter for today): ";
      std::string date;
      std::getline(std::cin, date);
      date = insura::domain::strops::trim(date);
      if (date.empty()) {
        data.date = insura::utils::date::today();
        break;
      }
      if (!insura::utils::date::isValidDate(date)) {
        std::cout << "Invalid date. Expected format: YYYY-MM-DD.\n";
        continue;
      }
      data.date = date;
      break;
    }

    /* TODO: add a isValidTime function, I should handle 24h and 12h format or
     * just tell the user the format in advance */
    data.time = promptRequired("Time (HH:MM): ");

    std::cout << "Duration:\n";
    data.duration = promptDuration();

    data.report = promptOptional("Report (optional): ");
    data.notes = promptOptional("Notes (optional): ");

    try {
      interaction_service_.addAppointment(data);
      std::cout << "\nAppointment added successfully.\n";
    } catch (const std::invalid_argument& e) {
      std::cout << "\nError: " << e.what() << '\n';
    }

  } else {
    domain::ContractData data;
    data.client_uuid = client->getUuid();

    while (true) {
      std::cout << "\nDate (YYYY-MM-DD, Enter for today): ";
      std::string date;
      std::getline(std::cin, date);
      date = insura::domain::strops::trim(date);
      if (date.empty()) {
        data.date = insura::utils::date::today();
        break;
      }
      if (!insura::utils::date::isValidDate(date)) {
        std::cout << "  Invalid date. Expected format: YYYY-MM-DD.\n";
        continue;
      }
      data.date = date;
      break;
    }

    while (true) {
      std::cout << "Value (EUR): ";
      std::string input;
      std::getline(std::cin, input);
      input = insura::domain::strops::trim(input);
      try {
        double v = std::stod(input);
        if (v <= 0.0) {
          std::cout << "  Value must be greater than zero.\n";
          continue;
        }
        data.value = v;
        break;
      } catch (const std::exception&) {
        std::cout << "  Please enter a valid number.\n";
      }
    }

    data.product_name = promptRequired("Product name: ");

    while (true) {
      std::cout << "Signed date (YYYY-MM-DD, Enter for today): ";
      std::string date;
      std::getline(std::cin, date);
      date = insura::domain::strops::trim(date);
      if (date.empty()) {
        data.signed_date = insura::utils::date::today();
        break;
      }
      if (!insura::utils::date::isValidDate(date)) {
        std::cout << "  Invalid date. Expected format: YYYY-MM-DD.\n";
        continue;
      }
      data.signed_date = date;
      break;
    }

    std::cout << "Contract duration:\n";
    data.contract_years = promptContractYears();

    std::cout << "Status:\n";
    data.status = promptContractStatus();

    data.notes = promptOptional("Notes (optional): ");

    try {
      interaction_service_.addContract(data);
      std::cout << "\nContract added successfully.\n";
    } catch (const std::invalid_argument& e) {
      std::cout << "\nError: " << e.what() << '\n';
    }
  }
}

domain::AppointmentData InteractionController::promptAppointmentEditData(
    const domain::Appointment& current) {
  domain::AppointmentData data;

  {
    std::string input;
    std::cout << "Date [" << current.getDate() << "] (Enter to keep): ";
    std::getline(std::cin, input);
    input = insura::domain::strops::trim(input);
    if (!input.empty()) data.date = input;
  }

  {
    std::string input;
    std::cout << "Time [" << current.getAppointmentTime()
              << "] (Enter to keep): ";
    std::getline(std::cin, input);
    input = insura::domain::strops::trim(input);
    if (!input.empty()) data.time = input;
  }

  {
    std::cout << "Duration [" << fmtDuration(current.getDuration())
              << "] (1-4 to change, Enter to keep):\n";
    for (int i = 0; i < kDurationCount; ++i)
      std::cout << "  " << (i + 1) << ". " << fmtDuration(kDurationOptions[i])
                << '\n';
    while (true) {
      std::cout << "> ";
      std::string input;
      std::getline(std::cin, input);
      input = insura::domain::strops::trim(input);
      if (input.empty()) break;
      if (input.size() == 1 && input[0] >= '1' && input[0] <= '4') {
        data.duration =
            kDurationOptions[static_cast<std::size_t>(input[0] - '1')];
        break;
      }
      std::cout << "  Please enter a number between 1 and 4, or press Enter "
                   "to keep.\n";
    }
  }

  {
    const auto& report = current.getAppointmentReport();
    std::string prompt = report.has_value()
                             ? "Report [" + *report + "] (Enter to keep): "
                             : "Report (optional): ";
    data.report = promptOptional(prompt);
  }

  {
    const auto& notes = current.getNotes();
    std::string prompt = notes.has_value()
                             ? "Notes [" + *notes + "] (Enter to keep): "
                             : "Notes (optional): ";
    data.notes = promptOptional(prompt);
  }

  return data;
}

/* Contract edit is restricted to status and notes per the same business
 * rule as Policy (ADR-019): contract-defining fields are immutable after
 * creation. */
domain::ContractData InteractionController::promptContractEditData(
    const domain::Contract& current) {
  domain::ContractData data;

  {
    std::cout << "Status ["
              << insura::domain::contractStatusToString(current.getStatus())
              << "] (1-4 to change, Enter to keep):\n";
    for (int i = 0; i < kContractStatusCount; ++i)
      std::cout << "  " << (i + 1) << ". "
                << insura::domain::contractStatusToString(
                       kContractStatusOptions[i])
                << '\n';
    while (true) {
      std::cout << "> ";
      std::string input;
      std::getline(std::cin, input);
      input = insura::domain::strops::trim(input);
      if (input.empty()) break;
      if (input.size() == 1 && input[0] >= '1' && input[0] <= '4') {
        data.status =
            kContractStatusOptions[static_cast<std::size_t>(input[0] - '1')];
        break;
      }
      std::cout << "  Please enter a number between 1 and 4, or press Enter "
                   "to keep.\n";
    }
  }

  {
    const auto& n = current.getNotes();
    std::string prompt = n.has_value() ? "Notes [" + *n + "] (Enter to keep): "
                                       : "Notes (optional): ";
    data.notes = promptOptional(prompt);
  }

  return data;
}

void InteractionController::cmdList() {
  /* TODO: add filter options (by type, by date range, by contract status)
   * mirroring PolicyController::cmdSearch; requires InteractionFilter in the
   * service layer first. */
  std::unordered_map<std::string, std::string> name_map;
  auto interactions = interaction_repo_.findAll();

  if (interactions.empty()) {
    std::cout << "No interactions found.\n";
    return;
  }

  for (const auto& i : interactions) {
    if (name_map.count(i->getClientUuid()) == 0) {
      auto client = client_repo_.findByUuid(i->getClientUuid());
      assert(client.has_value() &&
             "interaction references non-existent client");
      name_map[client->getUuid()] =
          client->getFirstName() + " " + client->getLastName();
    }
  }

  std::cout << '\n';
  InteractionView::displayAll(interaction_repo_.findAll(), name_map);
}

void InteractionController::cmdSearch() {
  /* TODO: expand search axes (by type, by date range, by contract status)
   * once InteractionFilter exists in the service layer; see cmdList TODO. */
  std::cout << "\nClient:\n";
  auto client = resolveClient(client_service_);
  if (!client) return;

  auto results = interaction_service_.searchByClient(client->getUuid());

  if (results.empty()) {
    std::cout << "No interactions found.\n";
    return;
  }

  std::unordered_map<std::string, std::string> name_map;
  name_map[client->getUuid()] =
      client->getFirstName() + " " + client->getLastName();

  std::cout << '\n';
  InteractionView::displayAll(results, name_map);
}

void InteractionController::cmdView() {
  auto result = resolveInteraction(interaction_service_, client_service_);
  if (!result) return;
  auto& [interaction, client] = *result;

  std::cout << '\n';
  InteractionView::displayOne(
      interaction, client.getFirstName() + " " + client.getLastName());
}

void InteractionController::cmdDelete() {
  auto result = resolveInteraction(interaction_service_, client_service_);
  if (!result) return;

  auto& [interaction, client] = *result;

  std::cout << '\n';
  InteractionView::displayOne(
      interaction, client.getFirstName() + " " + client.getLastName());

  std::string choice;
  std::cout << "\nAre you sure? (Y/n): ";
  std::getline(std::cin, choice);

  if (choice == "n") return;

  try {
    interaction_service_.deleteInteraction(interaction->getUuid());
    std::cout << "\nInteraction deleted successfully.\n";

  } catch (const std::invalid_argument& e) {
    std::cout << "\nError: " << e.what() << '\n';
  }
}

void InteractionController::cmdEdit() {

  auto result = resolveInteraction(interaction_service_, client_service_);
  if (!result) return;

  auto& [interaction, client] = *result;

  std::cout << '\n';
  InteractionView::displayOne(
      interaction, client.getFirstName() + " " + client.getLastName());

  std::string choice;
  std::cout << "\nEdit this record? (Y/n): ";
  std::getline(std::cin, choice);

  if (choice == "n") return;

  auto type = interaction->getType();
  if (type == insura::domain::Interaction::InteractionType::APPOINTMENT) {
    const auto* appt =
        dynamic_cast<const insura::domain::Appointment*>(interaction.get());
    assert(appt && "cmdEdit: APPOINTMENT cast failed");
    domain::AppointmentData updated = promptAppointmentEditData(*appt);
    try {
      if (interaction_service_.editAppointment(appt->getUuid(), updated))
        std::cout << "\nAppointment updated successfully.\n";
      else
        std::cout << "\nNo updates made.\n";
    } catch (const std::invalid_argument& e) {
      std::cout << "\nError: " << e.what() << '\n';
    }
  } else {
    const auto* contract =
        dynamic_cast<const insura::domain::Contract*>(interaction.get());
    assert(contract && "cmdEdit: CONTRACT cast failed");
    domain::ContractData updated = promptContractEditData(*contract);
    try {
      if (interaction_service_.editContract(contract->getUuid(), updated))
        std::cout << "\nContract updated successfully.\n";
      else
        std::cout << "\nNo updates made.\n";
    } catch (const std::invalid_argument& e) {
      std::cout << "\nError: " << e.what() << '\n';
    }
  }
}

}  // namespace insura::cli
