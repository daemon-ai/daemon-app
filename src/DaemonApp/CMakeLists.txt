# Each feature is its own QML module under a directory mirroring its URI
# (DaemonApp.<Feature>). Only App is wired in the minimal foundation; the rest
# are added as they are implemented.

add_subdirectory(App)

# add_subdirectory(Theme)
# add_subdirectory(Controls)
# add_subdirectory(Sidebar)
# add_subdirectory(ConversationsList)
# add_subdirectory(Conversation)
# add_subdirectory(Transcript)
# add_subdirectory(Composer)
# add_subdirectory(Settings)
