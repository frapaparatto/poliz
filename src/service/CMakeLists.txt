add_library(insura_service STATIC)
target_compile_options(insura_service PRIVATE -Wall -Wextra -Wpedantic)

target_sources(insura_service
  PRIVATE
    client_service.cpp
    policy_service.cpp
    interaction_service.cpp
    auto_save_service.cpp
)

target_link_libraries(insura_service
  PUBLIC
    insura_domain
)

target_include_directories(insura_service
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
