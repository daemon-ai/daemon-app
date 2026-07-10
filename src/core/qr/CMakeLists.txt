# daemon-app::qr - the shared QR module-matrix seam (QrMatrix + the unicode half-block projection).
# Front-end agnostic: the GUI Kit.QrCode painted item and the TUI auth panel both render the SAME
# matrix. The vendored Nayuki qrcodegen (pinned in flake.nix, compiled via Dependencies.cmake's
# `qrcodegen` target) is an implementation detail of qr_matrix.cpp and never leaks into the header.
add_library(da_qr STATIC
    qr_matrix.h
    qr_matrix.cpp
)
add_library(daemon-app::qr ALIAS da_qr)

target_include_directories(da_qr PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_qr
    PUBLIC
        Qt6::Core
    PRIVATE
        qrcodegen::qrcodegen
        daemon-app::warnings
)
