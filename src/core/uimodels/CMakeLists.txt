# Shared generic list model (QVariantMap rows under a single `entry` role) used
# by every mock-backed manager surface. Header-only, but compiled as an OBJECT-
# free INTERFACE-ish static lib so AUTOMOC processes the Q_OBJECT.
add_library(da_uimodels STATIC
    variant_list_model.h
    variant_list_model.cpp
)
add_library(daemon-app::uimodels ALIAS da_uimodels)

set_target_properties(da_uimodels PROPERTIES AUTOMOC ON)

target_include_directories(da_uimodels PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_uimodels
    PUBLIC
        Qt6::Core
    PRIVATE
        daemon-app::warnings
)
