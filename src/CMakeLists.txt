add_subdirectory(domain)
add_subdirectory(service)
add_subdirectory(data)
add_subdirectory(cli)

add_executable(crm-app main.cpp)
target_compile_options(crm-app PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(crm-app
  PRIVATE
    insura_cli
    insura_service
    insura_data
    insura_domain
)
