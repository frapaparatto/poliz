#pragma once
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "../domain/i_interaction_repository.hpp"

namespace insura::data {

class CsvInteractionRepository : public domain::IInteractionRepository {
 public:
  explicit CsvInteractionRepository(std::string filepath);
  void load();
  /* moved here to make it easy to test */
  void save() const override;
  void insertInteraction(
      std::unique_ptr<domain::Interaction> interaction) override;
  void removeInteraction(std::string_view uuid) override;
  void updateInteraction(
      std::unique_ptr<domain::Interaction> updated) override;
  /* missing */
  std::vector<std::unique_ptr<domain::Interaction>> filterByType(
      domain::Interaction::InteractionType type) const override;
  /* missing */
  std::vector<std::unique_ptr<domain::Interaction>> filterByDate(
      std::string_view start_date, std::string_view end_date) const override;
  std::vector<std::unique_ptr<domain::Interaction>> findByClientUuid(
      std::string_view client_uuid) const override;
  std::unique_ptr<domain::Interaction> findByUuid(std::string_view uuid) const override;
  std::vector<std::unique_ptr<domain::Interaction>> findAll() const override;

 private:
  bool isDirty() const override { return dirty_; }
  std::string serialize(
      const std::unique_ptr<domain::Interaction>& i) const;
  std::string serializeAppointment(const domain::Appointment& a) const;
  std::string serializeContract(const domain::Contract& c) const;
  std::unique_ptr<domain::Interaction> deserialize(
      const std::string& line) const;

  std::vector<std::unique_ptr<domain::Interaction>> interactions_;
  std::string filepath_;
  mutable bool dirty_ = false;
  mutable std::mutex mtx_;
};

}  // namespace insura::data
