add_subdirectory(domain)
add_subdirectory(service)
add_subdirectory(data)
add_subdirectory(cli)

add_executable(poliz main.cpp)
target_compile_options(poliz PRIVATE -Wall -Wextra -Wpedantic)

target_link_libraries(poliz
  PRIVATE
    insura_cli
    insura_service
    insura_data
    insura_domain
)
