include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0  # Stable version
)

FetchContent_MakeAvailable(Catch2)
add_executable(crm_tests test_strops.cpp test_utils.cpp test_status_converter.cpp test_interaction_status_converter.cpp test_domain.cpp test_calculate_amount.cpp integration_test.cpp)
target_compile_options(crm_tests PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(crm_tests 
  PRIVATE 
  Catch2::Catch2WithMain
  insura_data
  insura_cli
  insura_service
)

# This line tells CMake where to find the "Catch.cmake" script
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

target_include_directories(crm_tests PRIVATE ../src)
include(CTest)
include(Catch)
catch_discover_tests(crm_tests)
