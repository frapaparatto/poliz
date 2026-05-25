#pragma once
#include <string>
#include <optional>
#include <memory>

namespace insura::domain {

class Interaction {
 public:
  enum class InteractionType {
    APPOINTMENT,
    CONTRACT,
  };

  Interaction(std::string client_uuid, InteractionType type, std::string date,
              std::optional<std::string> notes);

  /* load constructor */
  Interaction(std::string uuid, std::string client_uuid, InteractionType type,
              std::string date, std::optional<std::string> notes,
              std::string created_at, std::string updated_at);
  virtual ~Interaction() = default;

  virtual std::unique_ptr<Interaction> clone() const = 0;
  const std::string& getUuid() const;
  const std::string& getClientUuid() const;
  InteractionType getType() const;
  const std::string& getDate() const;
  const std::optional<std::string>& getNotes() const;
  const std::string& getCreatedAt() const;
  const std::string& getUpdatedAt() const;

  void setType(InteractionType type);
  void setDate(const std::string& date);
  void setNotes(const std::string& notes);

 protected:
  std::string updated_at_;

 private:
  std::string uuid_;
  std::string client_uuid_;
  InteractionType interaction_type_;
  std::string date_;
  std::optional<std::string> notes_;
  std::string created_at_;
};

class Appointment : public Interaction {
 public:
  Appointment(std::string client_uuid, std::string date, std::string time,
              int duration, std::optional<std::string> report,
              std::optional<std::string> notes);

  /* load constructor */
  Appointment(std::string uuid, std::string client_uuid, std::string date,
              std::string time, int duration, std::optional<std::string> report,
              std::optional<std::string> notes, std::string created_at,
              std::string updated_at);

  std::unique_ptr<Interaction> clone() const override;
  const std::string& getAppointmentTime() const;
  int getDuration() const;
  const std::optional<std::string>& getAppointmentReport() const;
  void setTime(const std::string& time);
  void setDuration(int duration);
  void setReport(const std::string& report);

 private:
  std::string time_;
  /* here it can be useful to make also fixed as for policy amount
   * instead of calculating it with a table, use 30min, 1h, 1.5h
   * and 2h
   * so another thing to understand is actually the format
   * int (minutes) -> string for display (?)
   * */
  int duration_;  // minutes (30, 60, 80, 120)
  std::optional<std::string> report_;
};

class Contract : public Interaction {
 public:
  enum class ContractStatus {
    DRAFT,
    SIGNED,
    ACTIVE,
    TERMINATED,
  };

  Contract(std::string client_uuid, std::string date, double value,
           std::string product_name, std::string signed_date,
           std::optional<std::string> expired_date, ContractStatus status,
           std::optional<std::string> notes);

  /* load constructor */
  Contract(std::string uuid, std::string client_uuid, std::string date,
           double value, std::string product_name, std::string signed_date,
           std::optional<std::string> expired_date, ContractStatus status,
           std::optional<std::string> notes, std::string created_at,
           std::string updated_at);

  double getValue() const;
  std::unique_ptr<Interaction> clone() const override;
  const std::string& getProductName() const;
  const std::string& getSignedDate() const;
  const std::optional<std::string>& getExpiredDate() const;
  ContractStatus getStatus() const;
  void setValue(double value);
  void setProductName(const std::string& product_name);
  void setSignedDate(const std::string& signed_date);
  void setExpiredDate(const std::string& expired_date);
  void setStatus(ContractStatus status);

 private:
  double value_;
  std::string product_name_;
  std::string signed_date_;
  /* TODO: candidate for auto-calculation: signed_date + contract duration;
   * revisit when refactoring contract lifecycle */
  std::optional<std::string> expired_date_;
  ContractStatus contract_status_;
};

}  // namespace insura::domain
