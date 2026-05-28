#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../domain/client.hpp"
#include "../domain/interaction.hpp"
#include "../domain/policy.hpp"
#include "../service/client_service.hpp"
#include "../service/interaction_service.hpp"
#include "../service/policy_service.hpp"

namespace insura::cli {

/*
 * Shared I/O primitives used by every controller that needs text input.
 * Implementations are identical across controllers so they live here
 * rather than in per-controller anonymous namespaces.
 */
std::string promptRequired(std::string_view prompt);
std::optional<std::string> promptOptional(std::string_view prompt);
void pause();
bool confirmClient(const domain::Client& client, std::string_view entity);

/*
 * Client resolution helpers.
 *
 * selectClient: given an already-fetched list, displays a numbered menu
 * and returns the entry the user picks. Precondition: list is non-empty.
 *
 * resolveClient: the full "view/edit/delete" pipeline: prompts for a
 * search term, fetches matches, delegates to selectClient when multiple
 * results come back. Returns nullopt on empty input (user goes back) or
 * when no client matches the term. Caller decides whether to retry.
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

std::unique_ptr<domain::Interaction> selectInteraction(
    const std::vector<std::unique_ptr<domain::Interaction>>& interactions);

std::optional<std::pair<std::unique_ptr<domain::Interaction>, domain::Client>>
resolveInteraction(service::InteractionService& interaction_service,
                   service::ClientService& client_service);

/*
 * Truncates a string to exactly max characters, replacing the last three
 * with "..." when the string is longer than max. Prevents long values from
 * overflowing into adjacent std::setw columns.
 * The optional overload returns an empty string when the optional is empty.
 * Assumes max >= 3.
 */
std::string truncate(const std::string& s, std::size_t max);
std::string truncate(const std::optional<std::string>& opt, std::size_t max);

}  // namespace insura::cli
