#pragma once
#include <memory>
#include <string>
#include <unordered_map>

#include "../domain/i_client_repository.hpp"
#include "../domain/i_interaction_repository.hpp"
#include "../domain/i_policy_repository.hpp"
#include "../service/auto_save_service.hpp"
#include "../service/client_service.hpp"
#include "../service/interaction_service.hpp"
#include "../service/policy_service.hpp"
#include "i_entity_controller.hpp"

namespace insura::cli {

class Application {
 public:
  Application(bool autosave_enabled, int autosave_interval,
              service::ClientService& client_service,
              domain::IClientRepository& client_repo,
              service::PolicyService& policy_service,
              domain::IPolicyRepository& policy_repo,
              service::InteractionService& interaction_service,
              domain::IInteractionRepository& interaction_repo);
  void run();

 private:
  bool running_ = false;
  std::optional<service::AutoSaveService> autosave_;
  std::unordered_map<std::string, std::unique_ptr<IEntityController>>
      controllers_;
  IEntityController* active_controller_ = nullptr;
  bool handleAppCmds(const std::string& option);

  /* Application-level commands */
  void cmdSave(bool silent);
  void cmdExit();
  void cmdClear();
};

}  // namespace insura::cli
