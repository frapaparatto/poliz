
#include "client_status.hpp"

#include <stdexcept>
#include <string>

namespace insura::domain {

std::string statusToString(insura::domain::Client::ClientStatus status) {
  switch (status) {
    case insura::domain::Client::ClientStatus::NEW:
      return "new";
    case insura::domain::Client::ClientStatus::CONTACTED:
      return "contacted";
    case insura::domain::Client::ClientStatus::IN_PROGRESS:
      return "in_progress";
    case insura::domain::Client::ClientStatus::OPEN_DEAL:
      return "open_deal";
    case insura::domain::Client::ClientStatus::ATTEMPTED_TO_CONTACT:
      return "attempted_to_contact";
    case insura::domain::Client::ClientStatus::CLOSED_LOST:
      return "closed_lost";
    case insura::domain::Client::ClientStatus::CLOSED_WON:
      return "closed_won";
    default:
      throw std::invalid_argument("unhandled client status value");
  }
}

insura::domain::Client::ClientStatus statusFromString(std::string_view str) {
  if (str == "new") return insura::domain::Client::ClientStatus::NEW;
  if (str == "contacted")
    return insura::domain::Client::ClientStatus::CONTACTED;
  if (str == "in_progress")
    return insura::domain::Client::ClientStatus::IN_PROGRESS;
  if (str == "open_deal")
    return insura::domain::Client::ClientStatus::OPEN_DEAL;
  if (str == "attempted_to_contact")
    return insura::domain::Client::ClientStatus::ATTEMPTED_TO_CONTACT;
  if (str == "closed_lost")
    return insura::domain::Client::ClientStatus::CLOSED_LOST;
  if (str == "closed_won")
    return insura::domain::Client::ClientStatus::CLOSED_WON;

  throw std::invalid_argument("unknown client status: " + std::string(str));
}

}  // namespace insura::domain
