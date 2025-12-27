cmake_minimum_required(VERSION 3.14.0)

option(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES "Enable VST 3 Plug-in Examples" OFF)
option(SMTG_ENABLE_VST3_HOSTING_EXAMPLES "Enable VST 3 Hosting Examples" OFF)

set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13 CACHE STRING "")

set(vst3sdk_SOURCE_DIR "F:/projects/VST/vst-sdk_3.8.0_build-66_2025-10-20/VST_SDK/vst3sdk")
if(NOT vst3sdk_SOURCE_DIR)
    message(FATAL_ERROR "Path to VST3 SDK is empty!")
endif()

project(QBStrum
    # This is your plug-in version number. Change it here only.
    # Version number symbols usable in C++ can be found in
    # source/version.h and ${PROJECT_BINARY_DIR}/projectversion.h.
    VERSION 1.0.0.0 
    DESCRIPTION "QBStrum VST 3 Plug-in"
)

set(SMTG_VSTGUI_ROOT "${vst3sdk_SOURCE_DIR}")

add_subdirectory(${vst3sdk_SOURCE_DIR} ${PROJECT_BINARY_DIR}/vst3sdk)
smtg_enable_vst3_sdk()

smtg_add_vst3plugin(QBStrum
    source/version.h
    source/plugcids.h
    source/plugprocessor.h
    source/plugprocessor.cpp
    source/plugcontroller.h
    source/plugcontroller.cpp
    source/plugentry.cpp
)

#- VSTGUI Wanted ----
if(SMTG_ENABLE_VSTGUI_SUPPORT)
    target_sources(QBStrum
        PRIVATE
            resource/plugeditor.uidesc
    )
    target_link_libraries(QBStrum
        PRIVATE
            vstgui_support
    )
    smtg_target_add_plugin_resources(QBStrum
        RESOURCES
            "resource/plugeditor.uidesc"
    )
endif(SMTG_ENABLE_VSTGUI_SUPPORT)
# -------------------

smtg_target_add_plugin_snapshots (QBStrum
    RESOURCES
        resource/935FDF30800A5DD4A8D2329DD97A3B7C_snapshot.png
        resource/935FDF30800A5DD4A8D2329DD97A3B7C_snapshot_2.0x.png
)

target_link_libraries(QBStrum
    PRIVATE
        sdk
)

smtg_target_configure_version_file(QBStrum)

if(SMTG_MAC)
    smtg_target_set_bundle(QBStrum
        BUNDLE_IDENTIFIER sk
        COMPANY_NAME "MinMax"
    )
    smtg_target_set_debug_executable(QBStrum
        "/Applications/VST3PluginTestHost.app"
        "--pluginfolder;$(BUILT_PRODUCTS_DIR)"
    )
elseif(SMTG_WIN)
    target_sources(QBStrum PRIVATE 
        resource/win32resource.rc
    )
    if(MSVC)
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT QBStrum)

        smtg_target_set_debug_executable(QBStrum
            "$(ProgramW6432)/Steinberg/VST3PluginTestHost/VST3PluginTestHost.exe"
            "--pluginfolder \"$(OutDir)/\""
        )
    endif()
endif(SMTG_MAC)
