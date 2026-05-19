#pragma once
#include <mutex>

#include "../domain/client.hpp"
#include "../domain/i_client_repository.hpp"

namespace insura::data {

class CsvClientRepository : public domain::IClientRepository {
 public:
  explicit CsvClientRepository(std::string filepath);
  void load();
  void insertClient(domain::Client client) override;
  void removeClient(std::string_view uuid) override;
  void updateClient(domain::Client updated) override;
  std::optional<domain::Client> findByUuid(
      std::string_view uuid) const override;
  std::optional<domain::Client> findByEmail(
      std::string_view email) const override;
  std::vector<domain::Client> findAll() const override;

 private:
  mutable std::mutex mtx_;
  void save() const override;
  bool isDirty() const override { return dirty_; }
  std::string serialize(const domain::Client& c) const;
  domain::Client deserialize(const std::string& line) const;
  std::vector<domain::Client> clients_;
  std::string filepath_;
  mutable bool dirty_ = false;
  ;
};
}  // namespace insura::data
