# Shared agent/profile SETUP view-model (Phase C): the one "engine + backend + inference" pipeline
# both front ends bind (GUI AgentSetupForm, TUI AgentSetupView) and FirstRunModel delegates to.
# Interface-only deps (profiles/accounts/models seams) so it stays free of the daemon layer - the
# foreign-agent catalog + EngineIdentity summariser are injected by the service graph. Quick-free:
# Qt6::Core only.
add_library(da_setup STATIC
    agent_setup_model.h
    agent_setup_model.cpp
)
add_library(daemon-app::setup ALIAS da_setup)

set_target_properties(da_setup PROPERTIES AUTOMOC ON)

target_include_directories(da_setup PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_setup
    PUBLIC
        Qt6::Core
    PRIVATE
        daemon-app::profiles
        daemon-app::accounts
        daemon-app::models
        daemon-app::warnings
)
