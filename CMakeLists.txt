# 3.21+ works (including the CMake bundled with the Qt installer), newer policies up to 4.2
cmake_minimum_required(VERSION 3.21...4.2)
set(PROJECT_NAME "AdvancedVolumeMixer")
project(${PROJECT_NAME})

# Release unless the IDE/command line says otherwise
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ---- Where is Qt ----
# AVM_QT_DIR is the Qt kit folder, e.g. C:/Qt/6.8.3/mingw_64. First match wins:
#   1. -DAVM_QT_DIR=... (or an explicit -DCMAKE_PREFIX_PATH=...)
#   2. QT_DIR=... line in build.local next to this file (git-ignored,
#      build-release.bat writes it on the first run)
#   3. QTDIR environment variable
if(NOT AVM_QT_DIR AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/build.local")
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/build.local" avm_local_qt REGEX "^QT_DIR=" LIMIT_COUNT 1)
    if(avm_local_qt)
        string(REGEX REPLACE "^QT_DIR=" "" avm_local_qt "${avm_local_qt}")
        string(STRIP "${avm_local_qt}" avm_local_qt)
        file(TO_CMAKE_PATH "${avm_local_qt}" AVM_QT_DIR)
    endif()
endif()
if(NOT AVM_QT_DIR AND DEFINED ENV{QTDIR})
    file(TO_CMAKE_PATH "$ENV{QTDIR}" AVM_QT_DIR)
endif()
set(AVM_QT_DIR "${AVM_QT_DIR}" CACHE PATH "Qt kit folder, e.g. C:/Qt/6.8.3/mingw_64")
if(AVM_QT_DIR)
    list(PREPEND CMAKE_PREFIX_PATH "${AVM_QT_DIR}")
endif()

find_package(Qt6 6.8 QUIET COMPONENTS Core)
if(NOT Qt6_FOUND)
    message(FATAL_ERROR
        "Qt 6.8+ not found.\n"
        "Point the build to your Qt kit folder (the one with bin/ and lib/cmake/Qt6), one of:\n"
        "  - run build-release.bat, it asks once and remembers\n"
        "  - cmake -DAVM_QT_DIR=C:/Qt/6.8.3/mingw_64 ...\n"
        "  - put QT_DIR=C:/Qt/6.8.3/mingw_64 into build.local next to CMakeLists.txt\n"
        "  - set the QTDIR environment variable")
endif()

find_package(Qt6 COMPONENTS
        Core
        Gui
        Qml
        Quick
        QuickControls2
        Widgets
        REQUIRED)

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)
qt_policy(SET QTP0005 NEW)

qt_standard_project_setup()

# ---- Release optimizations ----
# Tuned for MinGW (what the Qt installer ships). Other compilers just get
# their default Release flags.
include(CheckIPOSupported)
check_ipo_supported(RESULT AVM_IPO_SUPPORTED OUTPUT AVM_IPO_ERROR LANGUAGES CXX)
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(AVM_IPO_SUPPORTED)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    endif()
elseif(AVM_IPO_SUPPORTED)
    # Manual -flto instead of CMAKE_INTERPROCEDURAL_OPTIMIZATION: CMake adds
    # -fno-fat-lto-objects, and then Qt's generated qml_register_types_*
    # symbols vanish from the static QML module libs at link time (MinGW ld
    # reads some objects without the LTO plugin). Fat objects keep real code too.
    add_compile_options($<$<CONFIG:Release>:-flto=auto> $<$<CONFIG:Release>:-ffat-lto-objects>)
    add_link_options($<$<CONFIG:Release>:-flto=auto>)
    # Archive with the LTO plugin so the symbol index covers LTO objects
    if(CMAKE_CXX_COMPILER_AR AND CMAKE_CXX_COMPILER_RANLIB)
        set(CMAKE_AR "${CMAKE_CXX_COMPILER_AR}")
        set(CMAKE_RANLIB "${CMAKE_CXX_COMPILER_RANLIB}")
    endif()
else()
    message(STATUS "LTO is not supported: ${AVM_IPO_ERROR}")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options($<$<CONFIG:Release>:-O3>)
    # Strip symbols and drop unused sections
    add_compile_options($<$<CONFIG:Release>:-ffunction-sections> $<$<CONFIG:Release>:-fdata-sections>)
    add_link_options($<$<CONFIG:Release>:-s> $<$<CONFIG:Release>:-Wl,--gc-sections>)
endif()
# qDebug() compiles to nothing in Release, warnings stay
add_compile_definitions($<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>)

qt_add_library(AVMTheme STATIC)
target_link_libraries(AVMTheme PRIVATE Qt6::Quick)
set_source_files_properties(ui/theme/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(AVMTheme
        URI AdvancedVolumeMixer.ThemeModule
        VERSION 1.0
        OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/AdvancedVolumeMixer/ThemeModule
        QML_FILES
        ui/theme/Theme.qml
)

qt_add_library(AVMComponents STATIC)
qt_add_qml_module(AVMComponents
        URI AdvancedVolumeMixer.ComponentsModule
        VERSION 1.0
        OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/AdvancedVolumeMixer/ComponentsModule
        IMPORTS
            TARGET AVMTheme
        QML_FILES
            ui/components/TitleBar.qml
            ui/components/IconButton.qml
            ui/components/AppButton.qml
            ui/components/AppTextField.qml
            ui/components/SectionLabel.qml
            ui/components/WindowResizeHandles.qml
            ui/components/ProfileToolbar.qml
            ui/components/ProcessPicker.qml
            ui/components/OutputListPanel.qml
            ui/components/OutputListItem.qml
            ui/components/ProfileScroller.qml
            ui/components/ProfileCard.qml
            ui/components/ProfileSettingsPanel.qml
            ui/components/HotkeyRecorder.qml
            ui/components/ProcessRuleList.qml
            ui/components/ProcessRuleItem.qml
            ui/components/AddProcessDialog.qml
            ui/components/TemplateSelector.qml
            ui/components/TemplateEditor.qml
        SOURCES
            source/public/controllers/AudioMixerController.cpp
            source/public/controllers/AudioMixerController.h
            source/public/models/AudioOutputsModel.cpp
            source/public/models/AudioOutputsModel.h
            source/public/models/MixerProfileModel.cpp
            source/public/models/MixerProfileModel.h
            source/public/models/ProcessRulesModel.cpp
            source/public/models/ProcessRulesModel.h
            source/public/structs/AudioOutputData.h
            source/public/structs/VolumeProfileData.h
            source/public/winWrappers/AudioDeviceWatcher.cpp
            source/public/winWrappers/AudioDeviceWatcher.h
            source/public/winWrappers/AudioSessionManager.cpp
            source/public/winWrappers/AudioSessionManager.h
            source/public/winWrappers/GlobalHotkeyManager.cpp
            source/public/winWrappers/GlobalHotkeyManager.h
)
target_link_libraries(AVMComponents PRIVATE AVMTheme)

qt_add_executable(${PROJECT_NAME}
        main.cpp
        source/public/tray/TrayManager.cpp
        source/public/tray/TrayManager.h
)

# GUI subsystem: no console window
set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
target_include_directories(${PROJECT_NAME} PRIVATE source/public/tray)

qt_add_qml_module(${PROJECT_NAME}
        URI AdvancedVolumeMixerModule
        VERSION 1.0
        OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/AdvancedVolumeMixer
        IMPORTS
            TARGET AVMTheme
            TARGET AVMComponents
        QML_FILES
        ui/Main.qml
)

target_include_directories(AVMComponents PUBLIC
        source/public
        source/public/controllers
        source/public/models
        source/public/winWrappers
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        AVMComponents
        AVMTheme
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Widgets
)

qt_import_qml_plugins(${PROJECT_NAME})

find_program(WINDEPLOYQT windeployqt HINTS "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_BINS}" REQUIRED)

set(AVM_WINDEPLOYQT_ARGS
        --qmldir "${CMAKE_SOURCE_DIR}/ui"
        --no-translations
        --no-system-d3d-compiler
        --no-system-dxc-compiler
        --no-opengl-sw
        --no-ffmpeg
        --skip-plugin-types qmltooling,tls,networkinformation,generic
)

# Dev runs straight from the build dir
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${WINDEPLOYQT}" --dir "$<TARGET_FILE_DIR:${PROJECT_NAME}>" ${AVM_WINDEPLOYQT_ARGS} "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMENT "Deploying Qt runtime to the build dir"
)

# ---- Clean deploy: cmake --install <build> --prefix <dir> (see build-release.bat) ----
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION .)

install(CODE "
    set(WINDEPLOYQT \"${WINDEPLOYQT}\")
    set(AVM_WINDEPLOYQT_ARGS \"${AVM_WINDEPLOYQT_ARGS}\")
")
install(CODE [[
    set(dist "${CMAKE_INSTALL_PREFIX}")
    execute_process(
        COMMAND "${WINDEPLOYQT}" --release ${AVM_WINDEPLOYQT_ARGS} "${dist}/AdvancedVolumeMixer.exe"
        RESULT_VARIABLE result
        OUTPUT_QUIET
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "windeployqt failed: ${result}")
    endif()

    # windeployqt copies qml/QtQuick with every submodule and all Controls
    # styles. Keep only what ui/*.qml imports plus its dependencies
    # (Material -> Basic, impl, Templates, Shapes, Window).
    function(avm_keep_only dir)
        file(GLOB entries LIST_DIRECTORIES true "${dir}/*")
        foreach(entry IN LISTS entries)
            get_filename_component(name "${entry}" NAME)
            list(FIND ARGN "${name}" keep) # not IN_LIST: install scripts run without policies
            if(IS_DIRECTORY "${entry}" AND keep EQUAL -1)
                file(REMOVE_RECURSE "${entry}")
            endif()
        endforeach()
    endfunction()

    avm_keep_only("${dist}/qml/QtQuick" Controls Layouts Shapes Templates Window)
    avm_keep_only("${dist}/qml/QtQuick/Controls" Basic Material impl)
    avm_keep_only("${dist}/qml/QtQml" Models WorkerScript)

    # Plugins nothing here uses: SVG/icon/image formats (icons are drawn in
    # code), virtual keyboard, widget styles (tray menu uses built-in Fusion)
    foreach(dir iconengines imageformats platforminputcontexts styles)
        file(REMOVE_RECURSE "${dist}/${dir}")
    endforeach()

    file(GLOB unused_dlls
        "${dist}/Qt6QuickControls2Imagine*.dll"
        "${dist}/Qt6QuickControls2Universal*.dll"
        "${dist}/Qt6QuickControls2Fusion*.dll"
        "${dist}/Qt6QuickControls2FluentWinUI3*.dll"
        "${dist}/Qt6QuickControls2Windows*.dll"
        "${dist}/Qt6VirtualKeyboard*.dll"
        "${dist}/Qt6QuickEffects.dll"
        "${dist}/Qt6Svg.dll"
    )
    if(unused_dlls)
        file(REMOVE ${unused_dlls})
    endif()

    # Separate debug info shipped with the MinGW Qt build
    file(GLOB_RECURSE debug_files "${dist}/*.debug")
    if(debug_files)
        file(REMOVE ${debug_files})
    endif()
]])
