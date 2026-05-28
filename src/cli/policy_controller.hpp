#pragma once
#include <functional>
#include <unordered_map>

#include "../domain/i_client_repository.hpp"
#include "../domain/i_policy_repository.hpp"
#include "../domain/policy.hpp"
#include "../domain/policy_data.hpp"
#include "../service/client_service.hpp"
#include "../service/policy_service.hpp"
#include "i_entity_controller.hpp"

namespace insura::cli {

class PolicyController : public IEntityController {
 public:
  PolicyController(service::PolicyService& policy_service,
                   domain::IPolicyRepository& policy_repo,
                   service::ClientService& client_service,
                   domain::IClientRepository& client_repo);

  void save() override;
  bool isDirty() const override;
  void cmdAdd() override;
  void cmdList() override;
  void cmdSearch() override;
  void cmdView() override;
  void cmdEdit() override;
  void cmdDelete() override;
  void execute(const std::string& cmd) override;

 private:
  service::PolicyService& policy_service_;
  domain::IPolicyRepository& policy_repo_;
  service::ClientService& client_service_;
  domain::IClientRepository& client_repo_;
  std::unordered_map<std::string, std::function<void()>> commands_;

  domain::PolicyData promptPolicyEditData(const domain::Policy& current);
};

}  // namespace insura::cli
