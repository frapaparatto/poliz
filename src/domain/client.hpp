#pragma once
#include <optional>
#include <string>

/*
 * Notes:
 * - Google C++ Style Guide:
 *   - Use PascalCase for methods and functions, I've decided to use camelCase
 * - No destructor: I've not allocated memory, default destructor is enough
 */

namespace insura::domain {

class Client {
 public:
  enum class ClientStatus {
    NEW,
    CONTACTED,
    IN_PROGRESS,
    OPEN_DEAL,
    ATTEMPTED_TO_CONTACT,
    CLOSED_WON,
    CLOSED_LOST,
  };

  Client(std::string first_name, std::string last_name, std::string email,
         std::optional<std::string> phone, std::optional<std::string> job_title,
         std::optional<std::string> company, std::optional<std::string> address,
         std::optional<std::string> city,
         std::optional<std::string> postal_code, ClientStatus status,
         std::optional<std::string> notes);

  Client(std::string uuid, std::string first_name, std::string last_name,
         std::string email, std::optional<std::string> phone,
         std::optional<std::string> job_title,
         std::optional<std::string> company, std::optional<std::string> address,
         std::optional<std::string> city,
         std::optional<std::string> postal_code, ClientStatus status,
         std::optional<std::string> notes, std::string created_at,
         std::string updated_at);

  const std::string& getUuid() const;
  const std::string& getEmail() const;
  const std::string& getFirstName() const;
  const std::string& getLastName() const;
  const std::string& getUpdatedAt() const;
  const std::string& getCreatedAt() const;

  void setFirstName(const std::string& first_name);
  void setLastName(const std::string& last_name);
  void setEmail(const std::string& email);
  void setPhone(const std::string& phone);
  void setJobTitle(const std::string& job_title);
  void setCompany(const std::string& company);
  void setAddress(const std::string& address);
  void setCity(const std::string& city);
  void setPostalCode(const std::string& postal_code);

  void setStatus(ClientStatus status);
  void setNotes(const std::string& notes);

  const std::optional<std::string>& getPhone() const;
  const std::optional<std::string>& getJobTitle() const;
  const std::optional<std::string>& getCompany() const;
  const std::optional<std::string>& getAddress() const;
  const std::optional<std::string>& getCity() const;
  const std::optional<std::string>& getPostalCode() const;
  const std::optional<std::string>& getNotes() const;

  ClientStatus getStatus() const;

 private:
  std::string uuid_;
  std::string first_name_;
  std::string last_name_;
  std::string email_;
  std::optional<std::string> phone_;
  std::optional<std::string> job_title_;
  std::optional<std::string> company_;
  std::optional<std::string> address_;
  std::optional<std::string> city_;
  std::optional<std::string> postal_code_;
  ClientStatus lead_status_ = ClientStatus::NEW;
  std::optional<std::string> notes_;
  std::string created_at_;
  std::string updated_at_;
};

}  // namespace insura::domain
