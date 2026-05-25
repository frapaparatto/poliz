#pragma once
#include <mutex>

#include "../domain/i_policy_repository.hpp"
#include "../domain/policy.hpp"

namespace insura::data {

class CsvPolicyRepository : public domain::IPolicyRepository {
 public:
  explicit CsvPolicyRepository(std::string filepath);
  void load();
  /* moved here to make it easy to test */
  void save() const override;
  void insertPolicy(domain::Policy policy) override;
  void removePolicy(std::string_view uuid) override;
  void updatePolicy(domain::Policy updated) override;

  std::optional<domain::Policy> findByUuid(
      std::string_view uuid) const override;
  std::vector<domain::Policy> findByClientUuid(
      std::string_view client_uuid) const override;
  std::vector<domain::Policy> findWhere(
      const domain::PolicyFilter& filter) const override;

  std::vector<domain::Policy> findAll() const override;

 private:
  bool isDirty() const override { return dirty_; }
  std::string serialize(const domain::Policy& p) const;
  domain::Policy deserialize(const std::string& line) const;

  std::vector<domain::Policy> policies_;
  std::string filepath_;
  mutable bool dirty_ = false;
  mutable std::mutex mtx_;
};

}  // namespace insura::data
