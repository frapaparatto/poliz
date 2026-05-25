#include "interaction_service.hpp"

#include <cassert>
#include <memory>
#include <stdexcept>
#include <string>

#include "../domain/utils.hpp"

namespace insura::service {

void InteractionService::addAppointment(
    const domain::AppointmentData& appointment_data) {
  if (!insura::utils::date::isValidDate(appointment_data.date)) {
    throw std::invalid_argument("invalid date: " + appointment_data.date);
  }

  auto appt = std::make_unique<domain::Appointment>(
      appointment_data.client_uuid, appointment_data.date,
      appointment_data.time, appointment_data.duration, appointment_data.report,
      appointment_data.notes);

  interaction_repo_.insertInteraction(std::move(appt));
}

void InteractionService::addContract(
    const domain::ContractData& contract_data) {
  if (!insura::utils::date::isValidDate(contract_data.date)) {
    throw std::invalid_argument("invalid date: " + contract_data.date);
  }
  if (!insura::utils::date::isValidDate(contract_data.signed_date)) {
    throw std::invalid_argument("invalid signed date: " +
                                contract_data.signed_date);
  }
  if (contract_data.value <= 0.0) {
    throw std::invalid_argument("contract value must be greater than zero");
  }

  auto expired_date = insura::utils::date::calculateEndDate(
      contract_data.signed_date, contract_data.contract_years * 12);
  auto status =
      contract_data.status.value_or(domain::Contract::ContractStatus::DRAFT);

  auto contract = std::make_unique<domain::Contract>(
      contract_data.client_uuid, contract_data.date, contract_data.value,
      contract_data.product_name, contract_data.signed_date, expired_date,
      status, contract_data.notes);

  interaction_repo_.insertInteraction(std::move(contract));
}

void InteractionService::deleteInteraction(std::string_view uuid) {
  auto found = interaction_repo_.findByUuid(uuid);
  assert(found != nullptr && "deleteInteraction: UUID not found");
  if (!found) throw std::invalid_argument("interaction not found");

  interaction_repo_.removeInteraction(uuid);
}

bool InteractionService::editAppointment(std::string_view uuid,
                                         const domain::AppointmentData& data) {
  auto found = interaction_repo_.findByUuid(uuid);
  assert(found != nullptr && "editAppointment: UUID not found");
  if (!found) throw std::invalid_argument("interaction not found");

  /* clone() returns unique_ptr<Interaction> (base type) even though
   * the heap object is a full Appointment with all its fields.
   *
   * dynamic_cast recovers the derived type so we can access
   * appointment-specific setters. Returns nullptr if the type
   * is wrong, which would be a programmer error at this point
   * since the caller already checked getType(). */
  auto cloned = found->clone();
  auto* appt = dynamic_cast<domain::Appointment*>(cloned.get());
  assert(appt && "editAppointment: cast to Appointment failed");
  if (!appt) throw std::invalid_argument("interaction is not an appointment");

  bool changed = false;

  if (!data.date.empty()) {
    appt->setDate(data.date);
    changed = true;
  }
  if (!data.time.empty()) {
    appt->setTime(data.time);
    changed = true;
  }
  if (data.duration != 0) {
    appt->setDuration(data.duration);
    changed = true;
  }
  if (data.report.has_value()) {
    appt->setReport(*data.report);
    changed = true;
  }
  if (data.notes.has_value()) {
    appt->setNotes(*data.notes);
    changed = true;
  }

  if (changed) interaction_repo_.updateInteraction(std::move(cloned));
  return changed;
}

/* Contract edit is restricted to status and notes. All
 * contract-defining fields (value, product_name, signed_date,
 * date) are immutable after creation. Changing them would
 * invalidate the formal agreement the contract represents.
 * Same business rule pattern as Policy (ADR-019). */
bool InteractionService::editContract(std::string_view uuid,
                                      const domain::ContractData& data) {
  auto found = interaction_repo_.findByUuid(uuid);
  assert(found != nullptr && "editContract: UUID not found");
  if (!found) throw std::invalid_argument("interaction not found");

  auto cloned = found->clone();
  auto* contract = dynamic_cast<domain::Contract*>(cloned.get());
  assert(contract && "editContract: cast to Contract failed");
  if (!contract) throw std::invalid_argument("interaction is not a contract");

  bool changed = false;

  if (data.status.has_value()) {
    contract->setStatus(*data.status);
    changed = true;
  }
  if (data.notes.has_value()) {
    contract->setNotes(*data.notes);
    changed = true;
  }

  if (changed) interaction_repo_.updateInteraction(std::move(cloned));
  return changed;
}

std::vector<std::unique_ptr<domain::Interaction>>
InteractionService::searchByClient(std::string_view client_uuid) const {
  return interaction_repo_.findByClientUuid(client_uuid);
}

std::vector<std::unique_ptr<domain::Interaction>>
InteractionService::searchByType(
    domain::Interaction::InteractionType type) const {
  return interaction_repo_.filterByType(type);
}

std::vector<std::unique_ptr<domain::Interaction>>
InteractionService::searchByDateRange(std::string_view start,
                                      std::string_view end) const {
  return interaction_repo_.filterByDate(start, end);
}

}  // namespace insura::service
