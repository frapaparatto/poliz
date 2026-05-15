#pragma once
#include "../domain/i_policy_repository.hpp"
#include "../domain/policy_data.hpp"

namespace insura::service {
class PolicyService {
 public:
  explicit PolicyService(domain::IPolicyRepository& repo)
      : policy_repo_(repo) {}
  void addPolicy(const domain::PolicyData& policy_data);
  void deletePolicy(std::string_view policy_uuid);
  bool editPolicy(std::string_view policy_uuid,
                  const domain::PolicyData& update_policy_data);
  std::vector<domain::Policy> searchByClient(
      std::string_view client_uuid) const;
  std::vector<domain::Policy> searchPolicy(
      const domain::PolicyFilter& filter) const;

 private:
  double calculateAmount(domain::Policy::PolicyType type, int duration);
  domain::IPolicyRepository& policy_repo_;
};

}  // namespace insura::service
