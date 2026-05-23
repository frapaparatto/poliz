#include "interaction_view.hpp"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

#include "../domain/interaction_status.hpp"
#include "../domain/strops.hpp"
#include "cli_helper.hpp"

namespace insura::cli {

namespace {
constexpr int kTypeWidth = 14;
constexpr int kClientWidth = 22;
constexpr int kDateWidth = 12;
constexpr int kNotesWidth = 30;
const std::string kSeparator(kTypeWidth + kClientWidth + kDateWidth +
                                 kNotesWidth,
                             '-');

std::string fmtAmount(double v) {
  std::ostringstream ss;
  /* TODO: understand if leave this as € or if write it diretcly
   * using that sequence is more portable */
  ss << "\xe2\x82\xac" << std::fixed << std::setprecision(2) << v;
  return ss.str();
}

std::string fmtDuration(int minutes) {
  int h = minutes / 60;
  int m = minutes % 60;
  if (h == 0) return std::to_string(m) + "min";
  if (m == 0) return std::to_string(h) + "h";
  return std::to_string(h) + "h " + std::to_string(m) + "min";
}

}  // namespace

void InteractionView::displayAll(
    const std::vector<std::unique_ptr<insura::domain::Interaction>>&
        interactions,
    const std::unordered_map<std::string, std::string>& client_names) {
  if (interactions.empty()) {
    std::cout << "No interactions found.\n";
    return;
  }

  std::cout << std::left << std::setw(kTypeWidth) << "Type"
            << std::setw(kClientWidth) << "Client" << std::setw(kDateWidth)
            << "Date" << std::setw(kNotesWidth) << "Notes" << '\n'
            << kSeparator << '\n';

  for (const auto& i : interactions) {
    auto it = client_names.find(i->getClientUuid());
    assert(it != client_names.end() &&
           "interaction must be associated with a valid client");
    const std::string& name = it->second;

    std::cout << std::left << std::setw(kTypeWidth)
              << insura::domain::strops::capitalize(
                     insura::domain::interactionTypeToString(i->getType()))
              << std::setw(kClientWidth) << name << std::setw(kDateWidth)
              << i->getDate() << std::setw(kNotesWidth)
              << insura::cli::truncate(i->getNotes(), kNotesWidth) << '\n';
  }
}

void InteractionView::displayOne(
    const std::unique_ptr<insura::domain::Interaction>& interaction,
    std::string_view client_name) {

  auto opt = [](const std::optional<std::string>& v) -> const std::string& {
    static const std::string kNa = "N/A";
    return v.has_value() ? v.value() : kNa;
  };

  std::cout << "Client:       " << client_name << '\n'
            << "Type:         "
            << insura::domain::strops::capitalize(
                   insura::domain::interactionTypeToString(interaction->getType()))
            << '\n'
            << "Date:         " << interaction->getDate() << '\n'
            << "Notes:        " << opt(interaction->getNotes()) << '\n'
            << "Created at:   " << interaction->getCreatedAt() << '\n'
            << "Updated at:   " << interaction->getUpdatedAt() << '\n';

  if (interaction->getType() ==
      insura::domain::Interaction::InteractionType::APPOINTMENT) {
    const auto* a =
        dynamic_cast<const insura::domain::Appointment*>(interaction.get());
    assert(a && "displayOne: APPOINTMENT cast failed");

    std::cout << "Time:         " << a->getAppointmentTime() << '\n'
              << "Duration:     " << fmtDuration(a->getDuration()) << '\n'
              << "Report:       " << opt(a->getAppointmentReport()) << '\n';
  } else {
    const auto* c =
        dynamic_cast<const insura::domain::Contract*>(interaction.get());
    assert(c && "displayOne: CONTRACT cast failed");

    std::cout << "Value:        " << fmtAmount(c->getValue()) << '\n'
              << "Product:      " << c->getProductName() << '\n'
              << "Signed date:  " << c->getSignedDate() << '\n'
              << "Expired date: " << opt(c->getExpiredDate()) << '\n'
              << "Status:       "
              << insura::domain::strops::capitalize(
                     insura::domain::contractStatusToString(c->getStatus()))
              << '\n';
  }
}

}  // namespace insura::cli
