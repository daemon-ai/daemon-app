# Accounts/auth seam + in-memory mock (Phase 4 Accounts manager + wizard). The
# mock simulates OAuth handshakes and API-key storage over VariantListModel.
add_library(da_accounts STATIC
    iaccounts_service.h
    mock_accounts_service.h
    mock_accounts_service.cpp
)
add_library(daemon-app::accounts ALIAS da_accounts)

set_target_properties(da_accounts PROPERTIES AUTOMOC ON)

target_include_directories(da_accounts PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_accounts
    PUBLIC
        Qt6::Core
        daemon-app::uimodels
    PRIVATE
        daemon-app::warnings
)
