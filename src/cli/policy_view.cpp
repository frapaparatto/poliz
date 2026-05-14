#include "policy_view.hpp"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

#include "../domain/client.hpp"
#include "../domain/policy_status.hpp"

namespace insura::cli {

namespace {
constexpr int kTypeWidth   = 10;
constexpr int kClientWidth = 22;
constexpr int kStatusWidth = 15;
constexpr int kAmountWidth = 12;
constexpr int kDateWidth   = 12;
const std::string kSeparator(
    kTypeWidth + kClientWidth + kStatusWidth + kAmountWidth + kDateWidth * 2,
    '-');

std::string fmtAmount(double v) {
  std::ostringstream ss;
  ss << "€" << std::fixed << std::setprecision(2) << v;
  return ss.str();
}
}  // namespace

void PolicyView::displayAll(
    const std::vector<domain::Policy>& policies,
    const std::unordered_map<std::string, std::string>& client_names) {
  if (policies.empty()) {
    std::cout << "No policies found.\n";
    return;
  }

  std::cout << std::left << std::setw(kTypeWidth)   << "Type"
                         << std::setw(kClientWidth) << "Client"
                         << std::setw(kStatusWidth) << "Status"
                         << std::setw(kAmountWidth) << "Amount"
                         << std::setw(kDateWidth)   << "Start Date"
                         << std::setw(kDateWidth)   << "End Date" << '\n'
            << kSeparator << '\n';

  for (const auto& p : policies) {
    auto it = client_names.find(p.getClientUuid());
    assert(it != client_names.end() &&
           "policy must be associated with a valid client");
    const std::string& name = it->second;

    std::cout << std::left << std::setw(kTypeWidth)
              << domain::policyTypeToString(p.getPolicyType())
              << std::setw(kClientWidth) << name
              << std::setw(kStatusWidth)
              << domain::policyStatusToString(p.getPolicyStatus())
              << std::setw(kAmountWidth + 2) << fmtAmount(p.getPolicyAmount())
              << std::setw(kDateWidth)        << p.getPolicyStartDate()
              << std::setw(kDateWidth)        << p.getPolicyEndDate().value_or("N/A")
              << '\n';
  }
}

bool PolicyView::confirmClient(const domain::Client& client) {
  std::cout << "First name:  " << client.getFirstName() << '\n'
            << "Last name:   " << client.getLastName() << '\n'
            << "Email:       " << client.getEmail() << '\n'
            << "\nAdd policy for this client? (Y/n): ";
  std::string choice;
  std::getline(std::cin, choice);
  return choice != "n";
}

void PolicyView::displayOne(const domain::Policy& p,
                            std::string_view client_name) {
  auto opt = [](const std::optional<std::string>& v) -> const std::string& {
    static const std::string kNa = "N/A";
    return v.has_value() ? v.value() : kNa;
  };

  std::cout << "Client:      " << client_name << '\n'
            << "Type:        " << domain::policyTypeToString(p.getPolicyType())
            << '\n'
            << "Status:      "
            << domain::policyStatusToString(p.getPolicyStatus()) << '\n'
            << "Amount:      " << fmtAmount(p.getPolicyAmount()) << '\n'
            << "Start date:  " << p.getPolicyStartDate() << '\n'
            << "End date:    " << opt(p.getPolicyEndDate()) << '\n'
            << "Notes:       " << opt(p.getPolicyNotes()) << '\n'
            << "Created at:  " << p.getCreatedAt() << '\n'
            << "Updated at:  " << p.getUpdatedAt() << '\n';
}

}  // namespace insura::cli
