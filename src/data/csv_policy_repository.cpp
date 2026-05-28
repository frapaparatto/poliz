#include "./csv_policy_repository.hpp"

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>

#include "../domain/policy_status.hpp"
#include "../domain/utils.hpp"
#include "file_handle.hpp"

namespace insura::data {
CsvPolicyRepository::CsvPolicyRepository(std::string filepath)
    : filepath_(std::move(filepath)) {}

void CsvPolicyRepository::insertPolicy(domain::Policy policy) {
  std::lock_guard<std::mutex> lock(mtx_);
  policies_.push_back(std::move(policy));
  dirty_ = true;
}

void CsvPolicyRepository::removePolicy(std::string_view uuid) {
  std::lock_guard<std::mutex> lock(mtx_);
  auto it = std::remove_if(
      policies_.begin(), policies_.end(),
      [uuid](const domain::Policy& p) { return p.getUuid() == uuid; });

  assert(it != policies_.end() && "removePolicy: UUID not found");
  policies_.erase(it, policies_.end());
  dirty_ = true;
}

std::optional<domain::Policy> CsvPolicyRepository::findByUuid(
    std::string_view uuid) const {
  std::lock_guard<std::mutex> lock(mtx_);
  auto it = std::find_if(
      policies_.begin(), policies_.end(),
      [uuid](const domain::Policy& p) { return p.getUuid() == uuid; });

  if (it == policies_.end()) return std::nullopt;
  return *it;
}

std::vector<domain::Policy> CsvPolicyRepository::findByClientUuid(
    std::string_view client_uuid) const {
  std::lock_guard<std::mutex> lock(mtx_);
  std::vector<domain::Policy> found;

  std::copy_if(policies_.begin(), policies_.end(), std::back_inserter(found),
               [client_uuid](const domain::Policy& p) {
                 return p.getClientUuid() == client_uuid;
               });

  return found;
}

/* Returns by value, not by reference. Returning a reference would
 * allow the caller to hold a pointer into the internal vector after
 * the mutex is released. Any write from another thread that triggers
 * reallocation would invalidate that pointer (undefined behavior).
 * The copy is made while the lock is held, so the caller gets a
 * consistent snapshot. */
std::vector<domain::Policy> CsvPolicyRepository::findAll() const {
  std::lock_guard<std::mutex> lock(mtx_);
  return policies_;
}

std::vector<domain::Policy> CsvPolicyRepository::findWhere(
    const domain::PolicyFilter& filter) const {
  std::lock_guard<std::mutex> lock(mtx_);
  std::vector<domain::Policy> found;

  std::copy_if(
      policies_.begin(), policies_.end(), std::back_inserter(found),

      [&filter](const domain::Policy& p) {
        if (filter.client_uuid && *filter.client_uuid != p.getClientUuid())
          return false;
        if (filter.status && *filter.status != p.getPolicyStatus())
          return false;
        if (filter.type && *filter.type != p.getPolicyType()) return false;

        return true;
      });

  return found;
}

void CsvPolicyRepository::updatePolicy(domain::Policy updated) {
  std::lock_guard<std::mutex> lock(mtx_);
  auto it = std::find_if(policies_.begin(), policies_.end(),
                         [&updated](const domain::Policy& p) {
                           return p.getUuid() == updated.getUuid();
                         });

  assert(it != policies_.end() && "updatePolicy: UUID not found");
  *it = std::move(updated);
  dirty_ = true;
}

std::string CsvPolicyRepository::serialize(const domain::Policy& p) const {
  std::ostringstream ss;
  ss << p.getUuid() << ",";
  ss << p.getClientUuid() << ",";
  ss << domain::policyTypeToString(p.getPolicyType()) << ",";
  ss << p.getPolicyStartDate() << ",";
  ss << p.getPolicyEndDate().value_or("") << ",";
  ss << std::fixed << std::setprecision(2) << p.getPolicyAmount() << ",";
  ss << domain::policyStatusToString(p.getPolicyStatus()) << ",";
  ss << p.getPolicyNotes().value_or("") << ",";
  ss << p.getCreatedAt() << ",";
  ss << p.getUpdatedAt();

  return ss.str();
}

domain::Policy CsvPolicyRepository::deserialize(const std::string& line) const {
  std::stringstream ss(line);
  std::string uuid, client_uuid, policy_type, start_date, end_date, amount,
      policy_status, notes, created_at, updated_at;

  std::getline(ss, uuid, ',');
  std::getline(ss, client_uuid, ',');
  std::getline(ss, policy_type, ',');
  std::getline(ss, start_date, ',');
  std::getline(ss, end_date, ',');
  std::getline(ss, amount, ',');
  std::getline(ss, policy_status, ',');
  std::getline(ss, notes, ',');
  std::getline(ss, created_at, ',');
  std::getline(ss, updated_at, ',');

  double parsed_amount;
  try {
    parsed_amount = std::stod(amount);
  } catch (const std::exception&) {
    throw std::runtime_error("invalid amount in CSV: " + amount);
  }

  return domain::Policy(uuid, client_uuid,
                        domain::policyTypeFromString(policy_type), start_date,
                        utils::stringToOptional(end_date), parsed_amount,
                        domain::policyStatusFromString(policy_status),
                        utils::stringToOptional(notes), created_at, updated_at);
}


void CsvPolicyRepository::load() {
  std::lock_guard<std::mutex> lock(mtx_);
  std::vector<domain::Policy> tmp_policies;

  if (std::filesystem::exists(filepath_)) {
    FileHandler in(filepath_, std::ios::in);
    std::string line;

    while (std::getline(in.getStream(), line)) {
      tmp_policies.push_back(deserialize(line));
    }
  } else {
    throw std::runtime_error("file not found: " + filepath_);
  }

  policies_ = std::move(tmp_policies);
}

void CsvPolicyRepository::save() const {
  std::lock_guard<std::mutex> lock(mtx_);
  std::string tmp = filepath_ + ".tmp";
  {
    /* Scoped because destructor needs to run */
    FileHandler out(tmp, std::ios::out);
    for (const auto& policy : policies_) {
      out.getStream() << serialize(policy) << '\n';
    }
  }
  std::rename(tmp.c_str(), filepath_.c_str());
  dirty_ = false;
}
}  // namespace insura::data
