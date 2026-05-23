#include "csv_interaction_repository.hpp"

#include <cassert>
#include <filesystem>
#include <mutex>
#include <sstream>
#include <stdexcept>

#include "../domain/interaction_status.hpp"
#include "../domain/utils.hpp"
#include "file_handle.hpp"

namespace insura::data {

CsvInteractionRepository::CsvInteractionRepository(std::string filepath)
    : filepath_(std::move(filepath)) {}

void CsvInteractionRepository::insertInteraction(
    std::unique_ptr<domain::Interaction> interaction) {
  std::lock_guard<std::mutex> lock(mtx_);
  interactions_.push_back(std::move(interaction));
  dirty_ = true;
}

void CsvInteractionRepository::removeInteraction(std::string_view uuid) {
  std::lock_guard<std::mutex> lock(mtx_);
  auto it =
      std::remove_if(interactions_.begin(), interactions_.end(),
                     [uuid](const std::unique_ptr<domain::Interaction>& i) {
                       return i->getUuid() == uuid;
                     });

  assert(it != interactions_.end() && "removeInteraction: UUID not found");
  interactions_.erase(it, interactions_.end());

  dirty_ = true;
}

void CsvInteractionRepository::updateInteraction(
    std::unique_ptr<domain::Interaction> updated) {
  std::lock_guard<std::mutex> lock(mtx_);
  auto it =
      std::find_if(interactions_.begin(), interactions_.end(),
                   [&updated](const std::unique_ptr<domain::Interaction>& i) {
                     return i->getUuid() == updated->getUuid();
                   });

  assert(it != interactions_.end() && "updateInteraction: UUID not found");
  *it = std::move(updated);
  dirty_ = true;
}

std::unique_ptr<domain::Interaction> CsvInteractionRepository::findByUuid(
    std::string_view uuid) const {
  std::lock_guard<std::mutex> lock(mtx_);
  auto it = find_if(interactions_.begin(), interactions_.end(),
                    [uuid](const std::unique_ptr<domain::Interaction>& i) {
                      return i->getUuid() == uuid;
                    });
  if (it == interactions_.end()) return nullptr;
  return it->get()->clone();
}

std::vector<std::unique_ptr<domain::Interaction>>
CsvInteractionRepository::findByClientUuid(std::string_view client_uuid) const {
  std::lock_guard<std::mutex> lock(mtx_);
  std::vector<std::unique_ptr<domain::Interaction>> found;
  for (const auto& i : interactions_) {
    if (i->getClientUuid() == client_uuid) found.push_back(i->clone());
  }

  return found;
}

std::vector<std::unique_ptr<domain::Interaction>>
CsvInteractionRepository::filterByType(
    domain::Interaction::InteractionType type) const {
  std::lock_guard<std::mutex> lock(mtx_);
  std::vector<std::unique_ptr<domain::Interaction>> result;
  for (const auto& i : interactions_) {
    if (i->getType() == type) result.push_back(i->clone());
  }
  return result;
}

std::vector<std::unique_ptr<domain::Interaction>>
CsvInteractionRepository::filterByDate(std::string_view start_date,
                                       std::string_view end_date) const {
  std::lock_guard<std::mutex> lock(mtx_);
  std::vector<std::unique_ptr<domain::Interaction>> result;
  for (const auto& i : interactions_) {
    if (i->getDate() >= start_date && i->getDate() <= end_date)
      result.push_back(i->clone());
  }
  return result;
}

std::vector<std::unique_ptr<domain::Interaction>>
CsvInteractionRepository::findAll() const {
  std::lock_guard<std::mutex> lock(mtx_);
  std::vector<std::unique_ptr<domain::Interaction>> result;
  /* since I know the size upfront, I can avoid reallocations */
  result.reserve(interactions_.size());
  for (const auto& i : interactions_) {
    result.push_back(i->clone());
  }

  return result;
}

std::string CsvInteractionRepository::serializeAppointment(
    const domain::Appointment& a) const {
  /* usage of dynamic cast because is safer since it uses the vtable
   * to verify the object is actually the correct type, if not
   * returns nullptr
   *
   * I could use static cast but the cost of dynamic cast is
   * negligible for my program and is defensive so I prefer that.
   *
   * same for serializeContract
   * */
  std::ostringstream ss;
  ss << a.getUuid() << ",";
  ss << a.getClientUuid() << ",";
  ss << domain::interactionTypeToString(a.getType()) << ",";
  ss << a.getDate() << ",";
  ss << a.getAppointmentTime() << ",";
  ss << std::to_string(a.getDuration()) << ",";
  ss << a.getAppointmentReport().value_or("") << ",";
  ss << a.getNotes().value_or("") << ",";
  ss << a.getCreatedAt() << ",";
  ss << a.getUpdatedAt();
  return ss.str();
}

std::string CsvInteractionRepository::serializeContract(
    const domain::Contract& c) const {
  std::ostringstream ss;
  ss << c.getUuid() << ",";
  ss << c.getClientUuid() << ",";
  ss << domain::interactionTypeToString(c.getType()) << ",";
  ss << c.getDate() << ",";
  ss << std::to_string(c.getValue()) << ",";
  ss << c.getProductName() << ",";
  ss << c.getSignedDate() << ",";
  ss << c.getExpiredDate().value_or("") << ",";
  ss << domain::contractStatusToString(c.getStatus()) << ",";
  ss << c.getNotes().value_or("") << ",";
  ss << c.getCreatedAt() << ",";
  ss << c.getUpdatedAt();
  return ss.str();
}

std::string CsvInteractionRepository::serialize(
    const std::unique_ptr<domain::Interaction>& i) const {
  if (i->getType() == domain::Interaction::InteractionType::APPOINTMENT) {
    const auto* a = dynamic_cast<const domain::Appointment*>(i.get());
    if (!a) throw std::runtime_error("serialize: APPOINTMENT cast failed");
    return serializeAppointment(*a);
  } else {
    const auto* c = dynamic_cast<const domain::Contract*>(i.get());
    if (!c) throw std::runtime_error("serialize: CONTRACT cast failed");
    return serializeContract(*c);
  }
}

std::unique_ptr<domain::Interaction> CsvInteractionRepository::deserialize(
    const std::string& line) const {
  std::stringstream ss(line);
  std::string uuid, client_uuid, type_string;

  std::getline(ss, uuid, ',');
  std::getline(ss, client_uuid, ',');
  std::getline(ss, type_string, ',');

  domain::Interaction::InteractionType type =
      domain::interactionTypeFromString(type_string);

  if (type == domain::Interaction::InteractionType::APPOINTMENT) {
    std::string date, time, duration_str, report, notes, created_at, updated_at;
    std::getline(ss, date, ',');
    std::getline(ss, time, ',');
    std::getline(ss, duration_str, ',');
    std::getline(ss, report, ',');
    std::getline(ss, notes, ',');
    std::getline(ss, created_at, ',');
    std::getline(ss, updated_at, ',');

    int duration;
    try {
      duration = std::stoi(duration_str);
    } catch (const std::exception&) {
      throw std::runtime_error("invalid duration in CSV: " + duration_str);
    }

    return std::make_unique<domain::Appointment>(
        uuid, client_uuid, date, time, duration,
        utils::stringToOptional(report), utils::stringToOptional(notes),
        created_at, updated_at);
  } else {
    std::string date, value_str, product_name, signed_date, expired_date,
        contract_status_string, notes, created_at, updated_at;
    std::getline(ss, date, ',');
    std::getline(ss, value_str, ',');
    std::getline(ss, product_name, ',');
    std::getline(ss, signed_date, ',');
    std::getline(ss, expired_date, ',');
    std::getline(ss, contract_status_string, ',');
    std::getline(ss, notes, ',');
    std::getline(ss, created_at, ',');
    std::getline(ss, updated_at, ',');

    double value;
    try {
      value = std::stod(value_str);
    } catch (const std::exception&) {
      throw std::runtime_error("invalid value in CSV: " + value_str);
    }

    return std::make_unique<domain::Contract>(
        uuid, client_uuid, date, value, product_name, signed_date,
        utils::stringToOptional(expired_date),
        domain::contractStatusFromString(contract_status_string),
        utils::stringToOptional(notes), created_at, updated_at);
  }
}

void CsvInteractionRepository::load() {
  std::lock_guard<std::mutex> lock(mtx_);
  std::vector<std::unique_ptr<domain::Interaction>> tmp_interactions;

  if (std::filesystem::exists(filepath_)) {
    FileHandler in(filepath_, std::ios::in);
    std::string line;
    while (std::getline(in.getStream(), line)) {
      tmp_interactions.push_back(deserialize(line));
    }
  } else {
    throw std::runtime_error("file not found: " + filepath_);
  }

  interactions_ = std::move(tmp_interactions);
  dirty_ = false;
}


void CsvInteractionRepository::save() const {
  std::lock_guard<std::mutex> lock(mtx_);
  std::string tmp = filepath_ + ".tmp";
  {
    FileHandler out(tmp, std::ios::out);
    for (const auto& i : interactions_) {
      out.getStream() << serialize(i) << '\n';
    }
  }
  std::rename(tmp.c_str(), filepath_.c_str());
  dirty_ = false;
}



}  // namespace insura::data
