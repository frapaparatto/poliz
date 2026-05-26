#include "menu.hpp"

#include <iomanip>
#include <iostream>
#include <string_view>

namespace insura::cli {

namespace {
constexpr int kInitWidth   = 16;
constexpr int kEntityWidth = 10;
}  // namespace

void Menu::displayInitMenu() {
  std::cout << "\nInsuraPro CRM\n\n"
            << "  " << std::left << std::setw(kInitWidth) << "clients"
            << "manage clients\n"
            << "  " << std::setw(kInitWidth) << "policies"
            << "manage policies\n"
            << "  " << std::setw(kInitWidth) << "interactions"
            << "manage interactions\n"
            << "  " << std::setw(kInitWidth) << "save"
            << "save data to CSV\n"
            << "  " << std::setw(kInitWidth) << "clear"
            << "clear the screen\n"
            << "  " << std::setw(kInitWidth) << "exit"
            << "exit\n\n";
}

void Menu::displayEntityMenu(std::string_view entity) {
  std::cout << "\nInsuraPro CRM \xE2\x80\xBA " << entity << "\n\n"
            << "  " << std::left << std::setw(kEntityWidth) << "add"
            << "add a new record\n"
            << "  " << std::setw(kEntityWidth) << "search"
            << "search records\n"
            << "  " << std::setw(kEntityWidth) << "list"
            << "list all records\n"
            << "  " << std::setw(kEntityWidth) << "view"
            << "view full details\n"
            << "  " << std::setw(kEntityWidth) << "edit"
            << "edit a record\n"
            << "  " << std::setw(kEntityWidth) << "delete"
            << "delete a record\n"
            << "  " << std::setw(kEntityWidth) << "back"
            << "go back\n"
            << "  " << std::setw(kEntityWidth) << "save"
            << "save\n"
            << "  " << std::setw(kEntityWidth) << "clear"
            << "clear the screen\n"
            << "  " << std::setw(kEntityWidth) << "exit"
            << "exit\n\n";
}

}  // namespace insura::cli
