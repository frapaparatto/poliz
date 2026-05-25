#include "./interaction.hpp"

#include <cassert>
#include <stdexcept>

#include "./utils.hpp"

namespace insura::domain {

Interaction::Interaction(std::string client_uuid, InteractionType type,
                         std::string date, std::optional<std::string> notes)
    : client_uuid_(std::move(client_uuid)) {
  if (date.empty()) throw std::invalid_argument("date cannot be empty");

  if (!utils::date::isValidDate(date))
    throw std::invalid_argument("invalid date");

  uuid_ = utils::generateUuid();
  assert(!uuid_.empty() && "generateUuid returned empty string");
  interaction_type_ = type;
  date_ = std::move(date);
  notes_ = std::move(notes);
  created_at_ = utils::currentTimestamp();
}

Interaction::Interaction(std::string uuid, std::string client_uuid,
                         InteractionType type, std::string date,
                         std::optional<std::string> notes,
                         std::string created_at, std::string updated_at)
    /* updated_at_ is declared in the protected section, before all private
     * members. Initializer lists must follow declaration order or the compiler
     * warns (-Wreorder). */
    : updated_at_(std::move(updated_at)),
      uuid_(std::move(uuid)),
      client_uuid_(std::move(client_uuid)),
      interaction_type_(type),
      date_(std::move(date)),
      notes_(std::move(notes)),
      created_at_(std::move(created_at)) {}

const std::string& Interaction::getUuid() const { return uuid_; }
const std::string& Interaction::getClientUuid() const { return client_uuid_; }
Interaction::InteractionType Interaction::getType() const {
  return interaction_type_;
}
const std::string& Interaction::getDate() const { return date_; }
const std::optional<std::string>& Interaction::getNotes() const {
  return notes_;
}
const std::string& Interaction::getCreatedAt() const { return created_at_; }
const std::string& Interaction::getUpdatedAt() const { return updated_at_; }

void Interaction::setType(InteractionType type) {
  interaction_type_ = type;
  updated_at_ = utils::currentTimestamp();
}

void Interaction::setDate(const std::string& date) {
  if (date.empty()) throw std::invalid_argument("date cannot be empty");
  if (!utils::date::isValidDate(date))
    throw std::invalid_argument("invalid date");
  date_ = date;
  updated_at_ = utils::currentTimestamp();
}

void Interaction::setNotes(const std::string& notes) {
  notes_ = notes;
  updated_at_ = utils::currentTimestamp();
}

Contract::Contract(std::string client_uuid, std::string date, double value,
                   std::string product_name, std::string signed_date,
                   std::optional<std::string> expired_date,
                   ContractStatus status, std::optional<std::string> notes)
    : Interaction(std::move(client_uuid), InteractionType::CONTRACT, date,
                  notes) {
  if (signed_date.empty())
    throw std::invalid_argument("signed date cannot be empty");

  if (product_name.empty())
    throw std::invalid_argument("product name cannot be empty");

  if (!utils::date::isValidDate(signed_date))
    throw std::invalid_argument("invalid signed date");

  if (value <= 0)
    throw std::invalid_argument("contract value must be greater than zero");

  value_ = value;
  product_name_ = std::move(product_name);
  signed_date_ = std::move(signed_date);
  expired_date_ = std::move(expired_date);
  contract_status_ = status;
}

Contract::Contract(std::string uuid, std::string client_uuid, std::string date,
                   double value, std::string product_name,
                   std::string signed_date,
                   std::optional<std::string> expired_date,
                   ContractStatus status, std::optional<std::string> notes,
                   std::string created_at, std::string updated_at)
    : Interaction(std::move(uuid), std::move(client_uuid),
                  InteractionType::CONTRACT, std::move(date), std::move(notes),
                  std::move(created_at), std::move(updated_at)),
      value_(value),
      product_name_(std::move(product_name)),
      signed_date_(std::move(signed_date)),
      expired_date_(std::move(expired_date)),
      contract_status_(status) {}

double Contract::getValue() const { return value_; }
const std::string& Contract::getProductName() const { return product_name_; }
const std::string& Contract::getSignedDate() const { return signed_date_; }
const std::optional<std::string>& Contract::getExpiredDate() const {
  return expired_date_;
}
Contract::ContractStatus Contract::getStatus() const {
  return contract_status_;
}

std::unique_ptr<Interaction> Contract::clone() const {
  return std::make_unique<Contract>(*this);
}

void Contract::setValue(double value) {
  if (value <= 0)
    throw std::invalid_argument("contract value must be greater than zero");
  value_ = value;
  updated_at_ = utils::currentTimestamp();
}

void Contract::setProductName(const std::string& product_name) {
  if (product_name.empty())
    throw std::invalid_argument("product name cannot be empty");
  product_name_ = product_name;
  updated_at_ = utils::currentTimestamp();
}

void Contract::setSignedDate(const std::string& signed_date) {
  if (signed_date.empty())
    throw std::invalid_argument("signed date cannot be empty");
  if (!utils::date::isValidDate(signed_date))
    throw std::invalid_argument("invalid signed date");
  signed_date_ = signed_date;
  updated_at_ = utils::currentTimestamp();
}

void Contract::setExpiredDate(const std::string& expired_date) {
  if (!expired_date.empty() && !utils::date::isValidDate(expired_date))
    throw std::invalid_argument("invalid expired date");
  expired_date_ = expired_date.empty() ? std::nullopt
                                       : std::optional<std::string>{expired_date};
  updated_at_ = utils::currentTimestamp();
}

void Contract::setStatus(ContractStatus status) {
  contract_status_ = status;
  updated_at_ = utils::currentTimestamp();
}

Appointment::Appointment(std::string client_uuid, std::string date,
                         std::string time, int duration,
                         std::optional<std::string> report,
                         std::optional<std::string> notes)
    : Interaction(client_uuid, InteractionType::APPOINTMENT, date, notes) {
  if (time.empty())
    throw std::invalid_argument("appointment time cannot be empty");

  if (duration <= 0)
    throw std::invalid_argument(
        "appointment duration must be greater than zero");

  time_ = std::move(time);
  duration_ = duration;
  report_ = std::move(report);
}

Appointment::Appointment(std::string uuid, std::string client_uuid,
                         std::string date, std::string time, int duration,
                         std::optional<std::string> report,
                         std::optional<std::string> notes,
                         std::string created_at, std::string updated_at)
    : Interaction(std::move(uuid), std::move(client_uuid),
                  InteractionType::APPOINTMENT, std::move(date),
                  std::move(notes), std::move(created_at),
                  std::move(updated_at)),
      time_(std::move(time)),
      duration_(duration),
      report_(std::move(report)) {}

const std::string& Appointment::getAppointmentTime() const { return time_; }
int Appointment::getDuration() const { return duration_; }
const std::optional<std::string>& Appointment::getAppointmentReport() const {
  return report_;
}

void Appointment::setTime(const std::string& time) {
  if (time.empty())
    throw std::invalid_argument("appointment time cannot be empty");
  time_ = time;
  updated_at_ = utils::currentTimestamp();
}

void Appointment::setDuration(int duration) {
  if (duration <= 0)
    throw std::invalid_argument(
        "appointment duration must be greater than zero");
  duration_ = duration;
  updated_at_ = utils::currentTimestamp();
}

void Appointment::setReport(const std::string& report) {
  report_ = report;
  updated_at_ = utils::currentTimestamp();
}

std::unique_ptr<Interaction> Appointment::clone() const {
  return std::make_unique<Appointment>(*this);
}

}  // namespace insura::domain
