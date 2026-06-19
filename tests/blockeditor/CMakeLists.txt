# BlockEditor engine tests, ported from upstream qml-block-editor. They link the
# plain-C++ engines (be_core / be_diagram) directly - no QML engine - so they run
# headless under ctest.

find_package(Qt6 6.11 REQUIRED COMPONENTS Test)

qt_add_executable(be_core_tests core_tests.cpp)
target_link_libraries(be_core_tests PRIVATE Qt6::Test be_core)
add_test(NAME be_core_tests COMMAND be_core_tests)
set_tests_properties(be_core_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

qt_add_executable(be_diagram_tests diagram_tests.cpp)
target_link_libraries(be_diagram_tests PRIVATE Qt6::Test Qt6::Gui be_diagram)
add_test(NAME be_diagram_tests COMMAND be_diagram_tests)
set_tests_properties(be_diagram_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
