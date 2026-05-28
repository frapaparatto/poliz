#include "client.hpp"

#include <cassert>
#include <stdexcept>

#include "utils.hpp"

namespace insura::domain {

Client::Client(std::string first_name, std::string last_name, std::string email,
               std::optional<std::string> phone,
               std::optional<std::string> job_title,
               std::optional<std::string> company,
               std::optional<std::string> address,
               std::optional<std::string> city,
               std::optional<std::string> postal_code, ClientStatus status,
               std::optional<std::string> notes) {
  if (first_name.empty()) {
    throw std::invalid_argument("first name cannot be empty");
  }

  if (last_name.empty()) {
    throw std::invalid_argument("last name cannot be empty");
  }

  if (email.empty()) {
    throw std::invalid_argument("email cannot be empty");
  }

  if (!utils::isValidEmail(email)) {
    throw std::invalid_argument("invalid email");
  }

  uuid_ = utils::generateUuid();
  assert(!uuid_.empty() && "generateUuid returned empty string");

  first_name_ = std::move(first_name);
  last_name_ = std::move(last_name);
  email_ = std::move(email);
  phone_ = std::move(phone);
  job_title_ = std::move(job_title);
  company_ = std::move(company);
  address_ = std::move(address);
  city_ = std::move(city);
  postal_code_ = std::move(postal_code);
  lead_status_ = status;
  notes_ = std::move(notes);
  created_at_ = utils::currentTimestamp();
}

Client::Client(std::string uuid, std::string first_name, std::string last_name,
               std::string email, std::optional<std::string> phone,
               std::optional<std::string> job_title,
               std::optional<std::string> company,
               std::optional<std::string> address,
               std::optional<std::string> city,
               std::optional<std::string> postal_code, ClientStatus status,
               std::optional<std::string> notes, std::string created_at,
               std::string updated_at)
    : uuid_(std::move(uuid)),
      first_name_(std::move(first_name)),
      last_name_(std::move(last_name)),
      email_(std::move(email)),
      phone_(std::move(phone)),
      job_title_(std::move(job_title)),
      company_(std::move(company)),
      address_(std::move(address)),
      city_(std::move(city)),
      postal_code_(std::move(postal_code)),
      lead_status_(status),
      notes_(std::move(notes)),
      created_at_(std::move(created_at)),
      updated_at_(std::move(updated_at)) {}

const std::string& Client::getUuid() const { return uuid_; }
const std::string& Client::getFirstName() const { return first_name_; }
const std::string& Client::getLastName() const { return last_name_; }
const std::string& Client::getEmail() const { return email_; }
const std::optional<std::string>& Client::getPhone() const { return phone_; }
const std::optional<std::string>& Client::getAddress() const {
  return address_;
}
const std::optional<std::string>& Client::getCity() const { return city_; }
const std::optional<std::string>& Client::getPostalCode() const {
  return postal_code_;
}
const std::optional<std::string>& Client::getJobTitle() const {
  return job_title_;
}
const std::optional<std::string>& Client::getCompany() const {
  return company_;
}
Client::ClientStatus Client::getStatus() const { return lead_status_; }
const std::string& Client::getUpdatedAt() const { return updated_at_; }
const std::string& Client::getCreatedAt() const { return created_at_; }

const std::optional<std::string>& Client::getNotes() const { return notes_; }

void Client::setFirstName(const std::string& first_name) {
  if (first_name.empty())
    throw std::invalid_argument("first name cannot be empty");
  first_name_ = first_name;
  updated_at_ = utils::currentTimestamp();
}

void Client::setLastName(const std::string& last_name) {
  if (last_name.empty())
    throw std::invalid_argument("last name cannot be empty");
  last_name_ = last_name;
  updated_at_ = utils::currentTimestamp();
}

void Client::setEmail(const std::string& email) {
  if (email.empty()) throw std::invalid_argument("email cannot be empty");
  if (!utils::isValidEmail(email)) throw std::invalid_argument("invalid email");
  email_ = email;
  updated_at_ = utils::currentTimestamp();
}

void Client::setNotes(const std::string& notes) {
  notes_ = notes;
  updated_at_ = utils::currentTimestamp();
}

void Client::setPhone(const std::string& phone) {
  if (!utils::isDigitsOnly(phone))
    throw std::invalid_argument("invalid phone number");

  phone_ = phone;
  updated_at_ = utils::currentTimestamp();
}
void Client::setAddress(const std::string& address) {
  address_ = address;
  updated_at_ = utils::currentTimestamp();
}
void Client::setCity(const std::string& city) {
  city_ = city;
  updated_at_ = utils::currentTimestamp();
}
void Client::setPostalCode(const std::string& postal_code) {
  if (!utils::isDigitsOnly(postal_code))
    throw std::invalid_argument("invalid postal code");

  postal_code_ = postal_code;
  updated_at_ = utils::currentTimestamp();
}

void Client::setJobTitle(const std::string& job_title) {
  job_title_ = job_title;
  updated_at_ = utils::currentTimestamp();
}
void Client::setCompany(const std::string& company) {
  company_ = company;
  updated_at_ = utils::currentTimestamp();
}

void Client::setStatus(ClientStatus status) {
  lead_status_ = status;
  updated_at_ = utils::currentTimestamp();
}

}  // namespace insura::domain
