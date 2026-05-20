#pragma once
#include <optional>
#include <string>

/*
 * On choices
 * - Why Policy constructor accepts the client_uuid?
 *   Because the responsibility to resolve the client_email -> client_uuid
 *   mapping lies with the policyService, the policy itself needs only to
 *   create its own uuid.
 */

namespace insura::domain {
/* TODO: evaluate policy deduplication strategy. Currently no mechanism
 * prevents adding duplicate policies (same type, client, start date).
 * Options to consider at end-of-project polish:
 * 1. Compare type + client_uuid + start_date on add
 * 2. Add a human-readable policy number (auto-generated, persisted,
 *    counter derived from highest existing number on load)
 * 3. Accept duplicates as valid (user corrects manually)
 * Decision deferred: current scope assumes no accidental duplicates. */
class Policy {
 public:
  enum class PolicyStatus {
    ACTIVE,
    EXPIRED,
    CANCELLED,
    PENDING,
  };

  enum class PolicyType {
    AUTO,
    LIFE,
    HOME,
    HEALTH,
  };

  Policy(std::string client_uuid, PolicyType policy_type,
         std::string start_date, std::optional<std::string> end_date,
         double amount, PolicyStatus policy_status,
         std::optional<std::string> notes);

  /* Constructor used for loading from csv */
  Policy(std::string uuid, std::string client_uuid, PolicyType policy_type,
         std::string start_date, std::optional<std::string> end_date,
         double amount, PolicyStatus policy_status,
         std::optional<std::string> notes, std::string created_at,
         std::string updated_at);

  const std::string& getUuid() const;
  const std::string& getClientUuid() const;
  PolicyType getPolicyType() const;
  PolicyStatus getPolicyStatus() const;
  const std::string& getPolicyStartDate() const;
  const std::optional<std::string>& getPolicyEndDate() const;
  double getPolicyAmount() const;
  const std::optional<std::string>& getPolicyNotes() const;
  const std::string& getCreatedAt() const;
  const std::string& getUpdatedAt() const;

  void setPolicyType(PolicyType policy_type);
  void setPolicyStatus(PolicyStatus policy_status);
  void setPolicyStartDate(const std::string& start_date);
  void setPolicyEndDate(const std::string& end_date);
  void setPolicyAmount(double amount);
  void setPolicyNotes(const std::string& notes);

 private:
  std::string uuid_;
  std::string client_uuid_;
  PolicyType policy_type_;
  std::string start_date_;
  std::optional<std::string> end_date_;
  double amount_;
  PolicyStatus policy_status_;
  std::optional<std::string> notes_;
  std::string created_at_;
  std::string updated_at_;
};

}  // namespace insura::domain
