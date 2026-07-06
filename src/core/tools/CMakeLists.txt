# [wave2:app-approvals-safety] D2: tool-inventory seam + in-memory mock (Settings -> Tools).
add_library(da_tools STATIC
    itool_inventory.h
    mock_tool_inventory.h
    mock_tool_inventory.cpp
)
add_library(daemon-app::tools ALIAS da_tools)

set_target_properties(da_tools PROPERTIES AUTOMOC ON)

target_include_directories(da_tools PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_tools
    PUBLIC
        Qt6::Core
        daemon-app::uimodels
    PRIVATE
        daemon-app::warnings
)
