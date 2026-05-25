add_library(insura_cli STATIC)
target_compile_options(insura_cli PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(insura_cli
PRIVATE
  insura_domain
  insura_service
)

target_include_directories(insura_cli
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(insura_cli
PRIVATE
  menu.cpp
  cli_helper.cpp
  client_view.cpp
  policy_view.cpp
  interaction_view.cpp
  application.cpp
  client_controller.cpp
  policy_controller.cpp
  interaction_controller.cpp
)



