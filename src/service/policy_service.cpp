#include "policy_service.hpp"

#include <cassert>
#include <stdexcept>
#include <string>

#include "../domain/utils.hpp"

namespace insura::service {

double PolicyService::calculateAmount(domain::Policy::PolicyType type,
                                      int duration) {
  /*
   * Pricing table: rows must match PolicyType enum declaration order.
   * This function relies on that ordering. A static_assert guards the
   * row count. The switch on duration makes column mapping explicit.
   *
   * | Type   |  6 months | 12 months | 24 months | 36 months |
   * |--------|-----------|-----------|-----------|-----------|
   * | AUTO   |   350.00  |   650.00  |  1200.00  |  1700.00  |
   * | LIFE   |   150.00  |   280.00  |   520.00  |   740.00  |
   * | HOME   |   120.00  |   220.00  |   400.00  |   570.00  |
   * | HEALTH |   200.00  |   380.00  |   700.00  |  1000.00  |
   */
  static constexpr double arr[4][4] = {{350.00, 650.00, 1200.00, 1700.00},
                                       {150.00, 280.00, 520.00, 740.00},
                                       {120.00, 220.00, 400.00, 570.00},
                                       {200.00, 380.00, 700.00, 1000.00}};
  static_assert(std::size(arr) == 4,
                "Pricing table rows must match PolicyType count");

  int duration_to_index;
  switch (duration) {
    case 6:
      duration_to_index = 0;
      break;
    case 12:
      duration_to_index = 1;
      break;
    case 24:
      duration_to_index = 2;
      break;
    case 36:
      duration_to_index = 3;
      break;
    default:
      throw std::invalid_argument("invalid duration months: " +
                                  std::to_string(duration));
  }

  auto type_index = static_cast<int>(type);
  return arr[type_index][duration_to_index];
}

void PolicyService::addPolicy(const domain::PolicyData& policy_data) {
  /* addPolicy only accepts type, client_uuid, start_date, and duration_months.
   * end_date and amount are derived here per ADR-019, not supplied by the
   * caller. */
  if (!insura::utils::date::isValidDate(policy_data.start_date)) {
    throw std::invalid_argument("invalid start date: " +
                                policy_data.start_date);
  }

  auto end_date = insura::utils::date::calculateEndDate(
      policy_data.start_date, policy_data.duration_months);
  auto amount =
      calculateAmount(policy_data.policy_type_, policy_data.duration_months);
  auto status =
      policy_data.policy_status.value_or(domain::Policy::PolicyStatus::PENDING);

  domain::Policy policy(policy_data.client_uuid, policy_data.policy_type_,
                        policy_data.start_date, end_date, amount, status,
                        policy_data.notes);

  policy_repo_.insertPolicy(policy);
}

void PolicyService::deletePolicy(std::string_view policy_uuid) {
  auto policy = policy_repo_.findByUuid(policy_uuid);
  assert(policy.has_value() && "deletePolicy: UUID not found");
  if (!policy) throw std::invalid_argument("policy not found");

  policy_repo_.removePolicy(policy_uuid);
}

void PolicyService::editPolicy(std::string_view policy_uuid,
                               const domain::PolicyData& update_policy_data) {
  auto policy = policy_repo_.findByUuid(policy_uuid);
  assert(policy.has_value() && "editPolicy: UUID not found");
  if (!policy) throw std::invalid_argument("policy not found");

  /* editPolicy only accepts status and notes because all other fields are
   * immutable contract-defining fields per ADR-019. */

  if (update_policy_data.policy_status.has_value()) {
    policy->setPolicyStatus(*update_policy_data.policy_status);
  }

  if (update_policy_data.notes.has_value()) {
    policy->setPolicyNotes(*update_policy_data.notes);
  }

  policy_repo_.updatePolicy(std::move(*policy));
}

std::vector<domain::Policy> PolicyService::searchByClient(
    std::string_view client_uuid) const {
  return policy_repo_.findByClientUuid(client_uuid);
}

std::vector<domain::Policy> PolicyService::searchPolicy(
    const domain::PolicyFilter& filter) const {
  return policy_repo_.findWhere(filter);
}

}  // namespace insura::service
