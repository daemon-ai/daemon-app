# Interactive-auth seam (begin -> browser -> complete over AuthBegin/AuthComplete): the
# IAuthFlowService interface, the shared AuthFlowController view-model (GUI sheet + TUI panel
# bind one instance), the loopback RedirectSink, and the offscreen/standalone mock.
add_library(da_auth STATIC
    iauth_flow_service.h
    auth_flow_controller.h
    auth_flow_controller.cpp
    redirect_sink.h
    redirect_sink.cpp
    mock_auth_flow_service.h
    mock_auth_flow_service.cpp
)
add_library(daemon-app::auth ALIAS da_auth)

set_target_properties(da_auth PROPERTIES AUTOMOC ON)

target_include_directories(da_auth PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_auth
    PUBLIC
        Qt6::Core
        Qt6::Network
    PRIVATE
        # QDesktopServices::openUrl (the controller's browser hand-off).
        Qt6::Gui
        daemon-app::warnings
)
