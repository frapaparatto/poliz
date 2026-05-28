#include "policy.hpp"

#include <cassert>
#include <stdexcept>

#include "utils.hpp"

namespace insura::domain {

/*
 * Reasoning: client id can't be empty so I don't need to check
 * for it.
 *
 * cli -> select and resolve client -> service find uuid and pass
 * to the constructor
 */
Policy::Policy(std::string client_uuid, PolicyType policy_type,
               std::string start_date, std::optional<std::string> end_date,
               double amount, PolicyStatus policy_status,
               std::optional<std::string> notes)
    : client_uuid_(std::move(client_uuid))

{
  if (start_date.empty())
    throw std::invalid_argument("start date cannot be empty");

  if (!utils::date::isValidDate(start_date))
    throw std::invalid_argument("invalid start date");

  if (end_date && !utils::date::isValidDate(end_date.value()))
    throw std::invalid_argument("invalid end date");

  if (amount <= 0)
    throw std::invalid_argument("amount must be greater than zero");

  uuid_ = utils::generateUuid();
  assert(!uuid_.empty() && "generateUuid returned empty string");

  policy_type_ = policy_type;
  start_date_ = std::move(start_date);

  end_date_ = std::move(end_date);
  amount_ = amount;
  policy_status_ = policy_status;
  notes_ = std::move(notes);
  created_at_ = utils::currentTimestamp();
}

Policy::Policy(std::string uuid, std::string client_uuid,
               PolicyType policy_type, std::string start_date,
               std::optional<std::string> end_date, double amount,
               PolicyStatus policy_status, std::optional<std::string> notes,
               std::string created_at, std::string updated_at)
    : uuid_(std::move(uuid)),
      client_uuid_(std::move(client_uuid)),
      policy_type_(policy_type),
      start_date_(std::move(start_date)),
      end_date_(std::move(end_date)),
      amount_(amount),
      policy_status_(policy_status),
      notes_(std::move(notes)),
      created_at_(std::move(created_at)),
      updated_at_(std::move(updated_at)) {}

const std::string& Policy::getUuid() const { return uuid_; }
const std::string& Policy::getClientUuid() const { return client_uuid_; }
Policy::PolicyType Policy::getPolicyType() const { return policy_type_; }
Policy::PolicyStatus Policy::getPolicyStatus() const { return policy_status_; }
const std::string& Policy::getPolicyStartDate() const { return start_date_; }

const std::optional<std::string>& Policy::getPolicyEndDate() const {
  return end_date_;
}
double Policy::getPolicyAmount() const { return amount_; }
const std::optional<std::string>& Policy::getPolicyNotes() const {
  return notes_;
}
const std::string& Policy::getCreatedAt() const { return created_at_; }
const std::string& Policy::getUpdatedAt() const { return updated_at_; }

void Policy::setPolicyType(PolicyType policy_type) {
  policy_type_ = policy_type;
  updated_at_ = utils::currentTimestamp();
}

void Policy::setPolicyStatus(PolicyStatus policy_status) {
  policy_status_ = policy_status;
  updated_at_ = utils::currentTimestamp();
}

void Policy::setPolicyStartDate(const std::string& start_date) {
  if (start_date.empty())
    throw std::invalid_argument("start date cannot be empty");
  if (!utils::date::isValidDate(start_date))
    throw std::invalid_argument("invalid start date");
  start_date_ = start_date;
  updated_at_ = utils::currentTimestamp();
}

void Policy::setPolicyEndDate(const std::string& end_date) {
  if (!utils::date::isValidDate(end_date))
    throw std::invalid_argument("invalid end date");
  end_date_ = end_date;
  updated_at_ = utils::currentTimestamp();
}

void Policy::setPolicyAmount(double amount) {
  if (amount <= 0)
    throw std::invalid_argument("amount must be greater than zero");
  amount_ = amount;
  updated_at_ = utils::currentTimestamp();
}

void Policy::setPolicyNotes(const std::string& notes) {
  notes_ = notes;
  updated_at_ = utils::currentTimestamp();
}

}  // namespace insura::domain
