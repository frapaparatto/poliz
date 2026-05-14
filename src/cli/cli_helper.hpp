#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../domain/client.hpp"
#include "../domain/policy.hpp"
#include "../service/client_service.hpp"
#include "../service/policy_service.hpp"

namespace insura::cli {

/*
 * Shared I/O primitives used by every controller that needs text input.
 * Implementations are identical across controllers so they live here
 * rather than in per-controller anonymous namespaces.
 */
std::string promptRequired(std::string_view prompt);
std::optional<std::string> promptOptional(std::string_view prompt);

/*
 * Client resolution helpers.
 *
 * selectClient: given an already-fetched list, displays a numbered menu
 * and returns the entry the user picks. Precondition: list is non-empty.
 *
 * resolveClient: the full "view/edit/delete" pipeline: prompts for a
 * search term, fetches matches, delegates to selectClient when multiple
 * results come back. Returns nullopt only when the term is non-empty but
 * no client is found (not a loop, caller decides whether to retry).
 * Not intended for cmdSearch, which shows the full results list instead
 * of forcing resolution to one entry.
 */
domain::Client selectClient(const std::vector<domain::Client>& clients);
std::optional<domain::Client> resolveClient(service::ClientService& service);

/*
 * Policy resolution helpers: same responsibility split as above.
 *
 * selectPolicy: numbered menu over an already-fetched list.
 *
 * resolvePolicy: full pipeline for view/edit/delete, calls resolveClient
 * first, then fetches the client's policies, then calls selectPolicy when
 * there are multiple. Not for cmdSearch.
 */
domain::Policy selectPolicy(const std::vector<domain::Policy>& policies);
std::optional<std::pair<domain::Policy, domain::Client>> resolvePolicy(
    service::PolicyService& policy_service,
    service::ClientService& client_service);

}  // namespace insura::cli
