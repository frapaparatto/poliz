add_library(insura_domain STATIC)

target_sources(insura_domain
  PRIVATE
    client.cpp
    policy.cpp
    interaction.cpp
    utils.cpp
    client_status.cpp
    policy_status.cpp
    interaction_status.cpp
    strops.cpp
)

target_include_directories(insura_domain
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
