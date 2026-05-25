add_library(insura_data STATIC)
target_compile_options(insura_data PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(insura_data
PUBLIC
  insura_domain
)

target_sources(insura_data
PRIVATE
  csv_client_repository.cpp
  csv_interaction_repository.cpp
  csv_policy_repository.cpp
  file_handle.cpp
)

target_include_directories(insura_data
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
