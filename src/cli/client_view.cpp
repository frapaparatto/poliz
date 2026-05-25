#include "client_view.hpp"

#include <iomanip>
#include <iostream>
#include <string>

#include "cli_helper.hpp"
#include "client_status.hpp"

namespace insura::cli {

namespace {
constexpr int kColWidth = 18;
constexpr int kEmailColWidth = 35;
const std::string kSeparator(kColWidth * 4 + kEmailColWidth, '-');
}  // namespace

/* TODO: understand which informations are the most useful to display, for now
 * it's ok to leave those */

void ClientView::displayAll(const std::vector<domain::Client>& clients) {
  std::cout << std::left << std::setw(kColWidth) << "First Name"
            << std::setw(kColWidth) << "Last Name"
            << std::setw(kEmailColWidth) << "Email"
            << std::setw(kColWidth) << "Lead Status"
            << std::setw(kColWidth) << "Created At" << '\n'
            << kSeparator << '\n';

  for (const auto& c : clients) {
    std::cout << std::left << std::setw(kColWidth) << c.getFirstName()
              << std::setw(kColWidth) << c.getLastName()
              << std::setw(kEmailColWidth) << truncate(c.getEmail(), kEmailColWidth)
              << std::setw(kColWidth) << domain::statusToString(c.getStatus())
              << std::setw(kColWidth) << c.getCreatedAt() << '\n';
  }
}

void ClientView::displayOne(const domain::Client& c) {
  auto opt = [](const std::optional<std::string>& v) -> std::string {
    return v.has_value() ? v.value() : "N/A";
  };

  std::cout << "First name:  " << c.getFirstName() << '\n'
            << "Last name:   " << c.getLastName() << '\n'
            << "Email:       " << c.getEmail() << '\n'
            << "Phone:       " << opt(c.getPhone()) << '\n'
            << "Job title:   " << opt(c.getJobTitle()) << '\n'
            << "Company:     " << opt(c.getCompany()) << '\n'
            << "Address:     " << opt(c.getAddress()) << '\n'
            << "City:        " << opt(c.getCity()) << '\n'
            << "Postal code: " << opt(c.getPostalCode()) << '\n'
            << "Lead status: " << domain::statusToString(c.getStatus()) << '\n'
            << "Created at:  " << c.getCreatedAt() << '\n'
            << "Updated at:  " << c.getUpdatedAt() << '\n';
}

}  // namespace insura::cli
