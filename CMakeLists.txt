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
include(GNUInstallDirs)

# Help CMake find Qt6 on the original Windows/MSVC setup without breaking
# other platforms that provide Qt through CMAKE_PREFIX_PATH or the system.
if(WIN32 AND NOT DEFINED Qt6_DIR)
    file(GLOB _QT6_MSVC_DIRS "C:/Qt/6*/msvc2022_64/lib/cmake/Qt6")
    list(SORT _QT6_MSVC_DIRS COMPARE NATURAL ORDER DESCENDING)
    if(_QT6_MSVC_DIRS)
        list(GET _QT6_MSVC_DIRS 0 _QT6_MSVC_DIR)
    endif()
    if(_QT6_MSVC_DIR AND EXISTS "${_QT6_MSVC_DIR}/Qt6Config.cmake")
        set(Qt6_DIR "${_QT6_MSVC_DIR}" CACHE PATH "Path to Qt6Config.cmake")
    endif()
endif()

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Network
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
    WIN32
    MACOSX_BUNDLE
    main.cpp
    src/GeminiMachineGenerator.cpp
    src/ui/GeminiApiKeysDialog.cpp
    src/ui/GeminiDialogStyle.cpp
    src/ui/GeminiGenerationRunner.cpp
    src/ui/GeminiSettingsDialog.cpp
    src/ui/MachineEditorTabAi.cpp
    src/ui/MachineEditorTabDisplay.cpp
    src/ui/MachineEditorTabLogic.cpp
    src/ui/MachineEditorTabPersistence.cpp
    src/ui/MachineEditorTabView.cpp
    src/ui/TransitionDelegate.cpp
    src/ui/TuringSimulatorUI.cpp
    src/ThemeManager.cpp
    include/TransitionDelegate.h
    include/GeminiMachineGenerator.h
    include/MachineEditorTab.h
    include/TuringSimulatorUI.h
    include/ThemeManager.h
    include/ui.h
    include/ui/GeminiApiKeysDialog.h
    include/ui/GeminiDialogStyle.h
    include/ui/GeminiGenerationRunner.h
    include/ui/GeminiSettingsDialog.h
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
    Qt6::Network
    Qt6::Widgets
    Qt6::Sql
)

if(COMMAND qt_finalize_executable)
    qt_finalize_executable(TuringSimulatorCpp)
endif()

find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating Doxygen documentation"
        VERBATIM
    )
endif()

install(TARGETS TuringSimulatorCpp
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(APPLE)
    install(CODE [[
        file(REMOVE "${CMAKE_INSTALL_PREFIX}/TuringSimulatorCpp.app/Contents/MacOS/machines.db")
    ]])
endif()

if(COMMAND qt_generate_deploy_app_script)
    qt_generate_deploy_app_script(
        TARGET TuringSimulatorCpp
        OUTPUT_SCRIPT qt_deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
        INCLUDE_PLUGIN_TYPES platforms sqldrivers tls
    )
    install(SCRIPT ${qt_deploy_script})
endif()
