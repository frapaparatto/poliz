#pragma once
#include "../domain/client_data.hpp"
#include "../domain/i_client_repository.hpp"
#include "../domain/i_interaction_repository.hpp"
#include "../domain/i_policy_repository.hpp"

namespace insura::service {

/*
 * Holds references to all three repositories, not just the client
 * repository. The cascade in deleteClient (remove client's policies
 * and interactions before the client itself) is the only operation
 * that needs cross-entity write access, and it lives here because
 * the client service is the one place that owns the "delete client
 * means delete their data" rule.
 */
class ClientService {
 public:
  ClientService(domain::IClientRepository& repo,
                domain::IPolicyRepository& policy_repo,
                domain::IInteractionRepository& interactions_repo)
      : repo_(repo), policies_(policy_repo), interactions_(interactions_repo) {}

  void addClient(const domain::ClientData& client_data);
  void deleteClient(std::string_view uuid);
  bool editClient(std::string_view uuid,
                  const domain::ClientData& new_client_data);
  std::vector<domain::Client> searchClients(std::string_view search_term) const;

 private:
  bool isEmailUnique(std::string_view email) const;
  domain::IClientRepository& repo_;
  domain::IPolicyRepository& policies_;
  domain::IInteractionRepository& interactions_;
};

}  // namespace insura::service
