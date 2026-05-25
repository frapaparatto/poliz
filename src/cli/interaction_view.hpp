#pragma once
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "../domain/interaction.hpp"

namespace insura::cli {

class InteractionView {
 public:
  static void displayAll(
      const std::vector<std::unique_ptr<domain::Interaction>>& interactions,
      const std::unordered_map<std::string, std::string>& client_names);
  static void displayAllAppointments(
      const std::vector<std::unique_ptr<domain::Interaction>>& interactions,
      const std::unordered_map<std::string, std::string>& client_names);
  static void displayAllContracts(
      const std::vector<std::unique_ptr<domain::Interaction>>& interactions,
      const std::unordered_map<std::string, std::string>& client_names);
  static void displayOne(
      const std::unique_ptr<domain::Interaction>& interaction,
      std::string_view client_name);
};

}  // namespace insura::cli
