# DaemonApp.Chat — native chat tabs (A4). The shared ChatConversationController
# view-model (bound by BOTH the GUI ChatPage.qml and the TUI RootWidget chat tabs)
# plus the GUI page, which reuses the existing BlockEditor (MarkdownDocumentView)
# transcript renderer and the existing Composer — no bespoke editor.
qt_add_qml_module(da_chat_view
    STATIC
    URI DaemonApp.Chat
    VERSION 1.0
    SOURCES
        chat_conversation_controller.h
        chat_conversation_controller.cpp
        conv_send_controller.h
        conv_send_controller.cpp
    QML_FILES
        ChatPage.qml
)
add_library(daemon-app::chat_view ALIAS da_chat_view)

target_link_libraries(da_chat_view
    PRIVATE
        Qt6::Quick
        daemon-app::transports
        daemon-app::theme
        daemon-app::controls
        daemon-app::settings
        daemon-app::blockeditor
        daemon-app::composer
        daemon-app::local
        daemon-app::warnings
)
