#pragma once
#include <optional>
#include <string>

#include "interaction.hpp"

namespace insura::domain {

struct AppointmentData {
  std::string client_uuid;
  std::string date;
  std::string time;
  int duration;  // minutes: 30, 60, 90, 120
  std::optional<std::string> report;
  std::optional<std::string> notes;
};

struct ContractData {
  std::string client_uuid;
  std::string date;
  double value;
  std::string product_name;
  std::string signed_date;
  int contract_years;  // 1, 2, 3
  std::optional<Contract::ContractStatus> status;
  std::optional<std::string> notes;
};

}  // namespace insura::domain
