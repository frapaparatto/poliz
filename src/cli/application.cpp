#include "application.hpp"

#include <iostream>

#include "../domain/strops.hpp"
#include "../service/auto_save_service.hpp"
#include "client_controller.hpp"
#include "interaction_controller.hpp"
#include "menu.hpp"
#include "policy_controller.hpp"

namespace insura::cli {

Application::Application(bool autosave_enabled, int autosave_interval,
                         service::ClientService& client_service,
                         domain::IClientRepository& client_repo,
                         service::PolicyService& policy_service,
                         domain::IPolicyRepository& policy_repo,
                         service::InteractionService& interaction_service,
                         domain::IInteractionRepository& interaction_repo) {
  controllers_["clients"] = std::make_unique<ClientController>(
      client_service, client_repo, policy_service);
  controllers_["policies"] = std::make_unique<PolicyController>(
      policy_service, policy_repo, client_service, client_repo);
  controllers_["interactions"] = std::make_unique<InteractionController>(
      interaction_service, interaction_repo, client_service, client_repo);

  if (autosave_enabled) {
    autosave_.emplace([this] { cmdSave(true); }, autosave_interval);
  }
}

/* preferred over std::system("clear"): no subprocess, no shell dependency */
void Application::cmdClear() { std::cout << "\033[2J\033[H"; }

void Application::cmdSave(bool silent) {
  try {
    for (auto& [name, controller] : controllers_) {
      controller->save();
    }
    if (!silent) std::cout << "Data saved correctly.\n";
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
  }
}

void Application::cmdExit() {
  if (autosave_) autosave_->stop();
  bool any_dirty = false;

  for (auto& [name, controller] : controllers_) {
    if (controller->isDirty()) {
      any_dirty = true;
      break;
    }
  }

  if (any_dirty) {
    std::string choice;
    std::cout << "Save before exiting? (Y/n): ";
    std::getline(std::cin, choice);
    choice = domain::strops::trim(choice);
    if (choice != "n") cmdSave(false);
  }

  std::cout << "Closing session.\n";
  running_ = false;
}

bool Application::handleAppCmds(const std::string& cmd) {
  if (cmd == "save") {
    cmdSave(false);
    return true;
  } else if (cmd == "exit") {
    cmdExit();
    return true;
  } else if (cmd == "clear") {
    cmdClear();
    return true;
  }
  return false;
}

void Application::run() {
  running_ = true;
  active_controller_ = nullptr;

  while (running_) {
    std::string ctrl;
    std::string option;
    Menu::displayInitMenu();
    std::cout << "> ";
    std::getline(std::cin, ctrl);

    ctrl = domain::strops::trim(ctrl);

    if (handleAppCmds(ctrl)) {
      continue;
    }

    auto it = controllers_.find(ctrl);
    if (it != controllers_.end()) {
      active_controller_ = it->second.get();

      while (running_) {
        Menu::displayEntityMenu(ctrl);
        std::cout << "> ";
        std::getline(std::cin, option);

        option = domain::strops::trim(option);

        if (option == "back") break;
        if (!handleAppCmds(option)) {
          active_controller_->execute(option);
        }
      }

    } else {
      std::cout << "\nUnknown command. Type a command from the menu above.\n";
    }
  }
}

}  // namespace insura::cli
