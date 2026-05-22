cmake_minimum_required(VERSION 3.20)
project(TuringSimulatorCpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC compiler flags for Qt compatibility
if(MSVC)
    add_compile_options(/Zc:__cplusplus /permissive-)
endif()

# Qt6 configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
include(CTest)

# Help CMake find Qt6 on the original Windows/MSVC setup without breaking
# other platforms that provide Qt through CMAKE_PREFIX_PATH or the system.
if(WIN32 AND NOT DEFINED Qt6_DIR)
    set(_QT6_MSVC_DIR "C:/Qt/6.11.1/msvc2022_64/lib/cmake/Qt6")
    if(EXISTS "${_QT6_MSVC_DIR}/Qt6Config.cmake")
        set(Qt6_DIR "${_QT6_MSVC_DIR}" CACHE PATH "Path to Qt6Config.cmake")
    endif()
endif()

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Sql
)

# Core Turing Machine library (non-GUI)
add_library(turing_core
    include/Tape.h
    include/TuringMachine.h
    src/Tape.cpp
    src/TuringMachine.cpp
)

target_include_directories(turing_core PUBLIC include)

if(BUILD_TESTING)
    add_executable(turing_core_tests
        tests/TuringCoreTests.cpp
    )
    target_link_libraries(turing_core_tests PRIVATE turing_core)
    add_test(NAME turing_core_tests COMMAND turing_core_tests)
endif()

# GUI executable
add_executable(TuringSimulatorCpp
    main.cpp
    src/ui/MachineEditorTabDisplay.cpp
    src/ui/MachineEditorTabLogic.cpp
    src/ui/MachineEditorTabPersistence.cpp
    src/ui/MachineEditorTabView.cpp
    src/ui/TransitionDelegate.cpp
    src/ui/TuringSimulatorUI.cpp
    src/ThemeManager.cpp
    include/TransitionDelegate.h
    include/MachineEditorTab.h
    include/TuringSimulatorUI.h
    include/ThemeManager.h
    include/ui.h
    include/ui/TransitionTableRoles.h
    include/storage.h
    src/storage/FileStorage.cpp
    src/storage/DatabaseStorage.cpp
)

target_include_directories(TuringSimulatorCpp PRIVATE include)

target_link_libraries(TuringSimulatorCpp PRIVATE
    turing_core
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
)

find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating Doxygen documentation"
        VERBATIM
    )
endif()

# Copy Qt runtime DLLs to build directory for Windows
if(WIN32)
    set(QT_BIN_DIR "C:/Qt/6.11.1/msvc2022_64/bin")
    set(QT_PLUGINS_DIR "C:/Qt/6.11.1/msvc2022_64/plugins")

    # Copy core DLLs
    add_custom_command(TARGET TuringSimulatorCpp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_BIN_DIR}/Qt6Cored.dll"
            "${QT_BIN_DIR}/Qt6Guid.dll"
            "${QT_BIN_DIR}/Qt6Widgetsd.dll"
            "$<TARGET_FILE_DIR:TuringSimulatorCpp>/"
        COMMENT "Copying Qt6 runtime DLLs"
    )

    # Copy platform plugin
    add_custom_command(TARGET TuringSimulatorCpp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:TuringSimulatorCpp>/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT_PLUGINS_DIR}/platforms/qwindowsd.dll"
            "$<TARGET_FILE_DIR:TuringSimulatorCpp>/platforms/"
        COMMENT "Copying Qt6 platform plugins"
    )
endif()
