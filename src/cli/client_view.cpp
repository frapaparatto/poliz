#include "client_view.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "../domain/policy_status.hpp"
#include "cli_helper.hpp"
#include "client_status.hpp"

namespace insura::cli {

namespace {
constexpr int kColWidth = 14;
constexpr int kEmailColWidth = 24;
const std::string kSeparator(kColWidth * 4 + kEmailColWidth, '-');

constexpr int kPTypeWidth = 14;
constexpr int kPStatusWidth = 14;
constexpr int kPAmountWidth = 14;
constexpr int kDateLen = 10;
constexpr int kArrowWidth = 3;
const std::string kPolicySep(kPTypeWidth + kPStatusWidth + kPAmountWidth +
                                 kDateLen * 2 + kArrowWidth,
                             '-');

std::string fmtAmount(double v) {
  std::ostringstream ss;
  ss << "€" << std::fixed << std::setprecision(2) << v;
  return ss.str();
}
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

void ClientView::displayPolicies(const std::vector<domain::Policy>& policies) {
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
              << domain::policyTypeToString(p.getPolicyType())
              << std::setw(kPStatusWidth)
              << domain::policyStatusToString(p.getPolicyStatus())
              << std::setw(kPAmountWidth + 2) << amt << range << '\n';
  }
}

}  // namespace insura::cli
