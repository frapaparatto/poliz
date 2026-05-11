#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <iostream>

#include "./cli/application.hpp"
#include "./data/csv_client_repository.hpp"
#include "./data/csv_policy_repository.hpp"
#include "./domain/strops.hpp"
#include "./service/client_service.hpp"
#include "./service/policy_service.hpp"

// TODO END OF THE PROJECT: order all notes in docs/personal becasue lots of
// them needs to be used together with daily notes e.g. all pattern could became
// feynman-like files to explain pattern better. Explaining most important
// patterns could be a good idea

// TODO: address autocomplete via geocoding API (Google Places or Nominatim) -
// implement after TUI search is complete
// TODO: at the end of the project, adjust all docs journal notes and remove the
// - that AI added in each note
// TODO: add in the readme the section that explains how I used AI

/* Default filepath used when the user starts a fresh session without specifying
 * one. Hardcoded for now.
 * TODO: replace with a value read from a config file (e.g. insura.conf or
 * ~/.insura/config). The config should store at minimum: default_filepath and
 * optionally last_opened_filepath. main.cpp reads it here and passes the
 * resolved path down — the repo stays unaware of any config. */

namespace {
static constexpr const char* kDefaultDir = "insurapro_data";

constexpr int kInitWidth = 16;

void displayMainMenu() {
  std::cout << "\nInsuraPro CRM: Main menu\n\n"
            << "  " << std::left << std::setw(kInitWidth) << "new"
            << "create a new empty crm\n"
            << "  " << std::setw(kInitWidth) << "load"
            << "load an existing crm\n"
            << "  " << std::setw(kInitWidth) << "exit"
            << "close the program\n\n";
}

struct CrmRepositories {
  std::unique_ptr<insura::data::CsvClientRepository> client_repo;
  std::unique_ptr<insura::data::CsvPolicyRepository> policy_repo;
};

void cmdExit() { std::exit(0); }

CrmRepositories cmdNew() {
  std::string dirpath;
  std::cout << "Enter the directory path (default: " << kDefaultDir << "): ";
  std::getline(std::cin, dirpath);

  if (dirpath.empty()) dirpath = kDefaultDir;

  if (!std::filesystem::exists(dirpath)) {
    std::filesystem::create_directory(dirpath);
  }

  std::string client_path = dirpath + "/clients.csv";
  std::string policy_path = dirpath + "/policies.csv";

  CrmRepositories repositories;
  repositories.client_repo =
      std::make_unique<insura::data::CsvClientRepository>(client_path);
  repositories.policy_repo =
      std::make_unique<insura::data::CsvPolicyRepository>(policy_path);

  return repositories;
}

CrmRepositories cmdLoad() {
  while (true) {
    std::string dirpath;
    std::cout << "Enter the directory path to load: ";
    std::getline(std::cin, dirpath);
    dirpath = insura::domain::strops::trim(dirpath);

    if (dirpath.empty()) {
      std::cout << "  Directory path cannot be empty.\n";
      continue;
    }

    if (!std::filesystem::exists(dirpath)) {
      std::cout << "  Directory doesn't exist.\n\n"
                << "  " << std::left << std::setw(kInitWidth) << "retry"
                << "enter a different path\n"
                << "  " << std::setw(kInitWidth) << "new"
                << "create a new CRM in that directory\n"
                << "  " << std::setw(kInitWidth) << "back"
                << "back to main menu\n"
                << "  " << std::setw(kInitWidth) << "exit"
                << "close the program\n\n";
      std::string choice;
      std::cout << "> ";
      std::getline(std::cin, choice);
      choice = insura::domain::strops::trim(choice);

      if (choice == "retry") continue;
      if (choice == "new") {
        std::filesystem::create_directory(dirpath);
        std::string client_path = dirpath + "/clients.csv";
        std::string policy_path = dirpath + "/policies.csv";

        CrmRepositories repositories;
        repositories.client_repo =
            std::make_unique<insura::data::CsvClientRepository>(client_path);
        repositories.policy_repo =
            std::make_unique<insura::data::CsvPolicyRepository>(policy_path);
        return repositories;
      }
      if (choice == "back") return {};
      if (choice == "exit") cmdExit();
      std::cout << "  Invalid option.\n";
      continue;
    }

    std::string client_path = dirpath + "/clients.csv";
    std::string policy_path = dirpath + "/policies.csv";

    CrmRepositories repositories;
    repositories.client_repo =
        std::make_unique<insura::data::CsvClientRepository>(client_path);
    repositories.policy_repo =
        std::make_unique<insura::data::CsvPolicyRepository>(policy_path);

    try {
      repositories.client_repo->load();
    } catch (const std::exception& e) {
      std::cerr << "  Warning: could not load clients: " << e.what() << '\n';
      std::cout << "  Starting with empty client list.\n";
    }

    try {
      repositories.policy_repo->load();
    } catch (const std::exception& e) {
      std::cerr << "  Warning: could not load policies: " << e.what() << '\n';
      std::cout << "  Starting with empty policy list.\n";
    }

    return repositories;
  }
}
}  // namespace

int main() {
  CrmRepositories repos;

  while (true) {
    displayMainMenu();
    std::cout << "> ";
    std::string option;
    std::getline(std::cin, option);
    option = insura::domain::strops::trim(option);

    if (option == "new") {
      repos = cmdNew();
      break;
    } else if (option == "load") {
      repos = cmdLoad();
      if (repos.client_repo && repos.policy_repo) break;
    } else if (option == "exit") {
      cmdExit();
    } else {
      std::cout << "  Invalid option.\n";
    }
  }

  insura::service::PolicyService policy_service(*repos.policy_repo);
  insura::service::ClientService client_service(*repos.client_repo,
                                                *repos.policy_repo);
  insura::cli::Application app(client_service, *repos.client_repo,
                               policy_service, *repos.policy_repo);

  app.run();
  return 0;
}
