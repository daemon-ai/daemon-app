add_library(da_domain STATIC
    conversation.h
    agent_node.h
    tag.h
    sidebar_node.h
    domain.cpp
)
add_library(daemon-app::domain ALIAS da_domain)

# Includes are rooted at src/core so headers are addressed as <domain/...>.
target_include_directories(da_domain PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_domain
    PUBLIC Qt6::Core
    PRIVATE daemon-app::warnings
)
