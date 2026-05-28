#pragma once
#include <functional>
#include <unordered_map>

#include "../domain/i_client_repository.hpp"
#include "../domain/i_interaction_repository.hpp"
#include "../service/client_service.hpp"
#include "../service/interaction_service.hpp"
#include "i_entity_controller.hpp"

namespace insura::cli {
class InteractionController : public IEntityController {
 public:
  InteractionController(service::InteractionService& interaction_service,
                        domain::IInteractionRepository& interaction_repo,
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
  std::unordered_map<std::string, std::function<void()>> commands_;
  service::InteractionService& interaction_service_;
  domain::IInteractionRepository& interaction_repo_;
  service::ClientService& client_service_;
  domain::IClientRepository& client_repo_;

  /* Splitted functions to avoid one monolithic function that 
   * uses branching for detecting the type and for populating
   * the correct DTO */
  domain::AppointmentData promptAppointmentEditData(
      const domain::Appointment& current);
  domain::ContractData promptContractEditData(const domain::Contract& current);
};

}  // namespace insura::cli
