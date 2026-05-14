#pragma once
#include <functional>
#include <unordered_map>

#include "../domain/client.hpp"
#include "../domain/client_data.hpp"
#include "../domain/i_client_repository.hpp"
#include "../service/client_service.hpp"
#include "../service/policy_service.hpp"
#include "i_entity_controller.hpp"

namespace insura::cli {

class ClientController : public IEntityController {
 public:
  ClientController(service::ClientService& client_service,
                   domain::IClientRepository& repo,
                   service::PolicyService& policy_service);

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
  service::ClientService& client_service_;
  domain::IClientRepository& repo_;
  service::PolicyService& policy_service_;
  std::unordered_map<std::string, std::function<void()>> commands_;

  /* Client resolution uses the shared cli_helper free functions
   * (resolveClient / selectClient) rather than per-class methods. */
  domain::ClientData promptEditData(const domain::Client& current);
};

}  // namespace insura::cli
