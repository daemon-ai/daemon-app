# Model-catalog seam + in-memory mock (Phase 3 Models hub backing). The mock
# simulates HF search + ticking download progress over the shared VariantListModel.
add_library(da_models STATIC
    imodel_catalog.h
    mock_model_catalog.h
    mock_model_catalog.cpp
)
add_library(daemon-app::models ALIAS da_models)

set_target_properties(da_models PROPERTIES AUTOMOC ON)

target_include_directories(da_models PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_models
    PUBLIC
        Qt6::Core
        daemon-app::uimodels
    PRIVATE
        daemon-app::warnings
)
