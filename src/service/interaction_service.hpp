#pragma once
#include <memory>
#include <vector>

#include "../domain/i_interaction_repository.hpp"
#include "../domain/interaction_data.hpp"

namespace insura::service {

class InteractionService {
 public:
  explicit InteractionService(domain::IInteractionRepository& repo)
      : interaction_repo_(repo) {}

  void addAppointment(const domain::AppointmentData& data);
  void addContract(const domain::ContractData& data);
  void deleteInteraction(std::string_view uuid);
  bool editAppointment(std::string_view uuid,
                       const domain::AppointmentData& data);
  bool editContract(std::string_view uuid, const domain::ContractData& data);
  std::vector<std::unique_ptr<domain::Interaction>> searchByClient(
      std::string_view client_uuid) const;
  std::vector<std::unique_ptr<domain::Interaction>> searchByType(
      domain::Interaction::InteractionType type) const;
  std::vector<std::unique_ptr<domain::Interaction>> searchByDateRange(
      std::string_view start, std::string_view end) const;

 private:
  domain::IInteractionRepository& interaction_repo_;
};

}  // namespace insura::service
