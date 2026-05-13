cmake_minimum_required(VERSION 3.20)
project(DSDesk VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_MIN_VERSION "6.5.0")


set(APP_ID "com.dekomote.dsdesk")
set(APP_ORG "com.dekomote")
set(APP_DESCRIPTION "Synology Download Station client for Desktop")
set(APP_LICENSE "MIT")
set(APP_HOMEPAGE "https://github.com/dekomote/DSDesk")
set(APP_BUG_ADDRESS "https://github.com/dekomote/DSDesk/issues")
set(APP_AUTHOR_NAME "Dejan Noveski")
set(APP_AUTHOR_EMAIL "deko@duck.com")
set(APP_AUTHOR_URL "https://github.com/dekomote")
set(APP_COPYRIGHT "(c) 2026")
string(TIMESTAMP APP_RELEASE_DATE "%Y-%m-%d")
set(APP_LONG_DESCRIPTION "DSDesk is a desktop client for Synology Download Station.")


find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS
    Core
    Gui
    Network
    Qml
    Quick
    QuickControls2
)

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)
qt_standard_project_setup()

set(SOURCES
    src/main.cpp
    src/ipc.h
    src/synology/SynologyClient.cpp
    src/synology/DownloadModel.cpp
    src/synology/SynologySettings.cpp
    src/credentials/KeychainManager.cpp
)

set(HEADERS
    src/synology/SynologyClient.h
    src/synology/DownloadTask.h
    src/synology/DownloadModel.h
    src/synology/SynologySettings.h
    src/credentials/KeychainManager.h
)

set(QML_FILES
    src/qml/Main.qml
    src/qml/LoginView.qml
    src/qml/DownloadListView.qml
    src/qml/AddDownloadDialog.qml
    src/qml/DownloadItemDelegate.qml
    src/qml/TaskDetailsSheet.qml
)


qt_add_executable(dsdesk
    ${SOURCES}
    ${HEADERS}
    resources/resources.qrc
)

qt_add_qml_module(dsdesk
    URI DSDesk
    VERSION 1.0
    QML_FILES ${QML_FILES}
)


target_link_libraries(dsdesk PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

target_include_directories(dsdesk PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(dsdesk PRIVATE
    APP_VERSION_STRING="${PROJECT_VERSION}"
    APP_ORG_STRING="${APP_ORG}"
    APP_NAME_STRING="${PROJECT_NAME}"
    APP_DESCRIPTION="${APP_DESCRIPTION}"
    APP_LICENSE="${APP_LICENSE}"
    APP_HOMEPAGE="${APP_HOMEPAGE}"
    APP_BUG_ADDRESS="${APP_BUG_ADDRESS}"
    APP_AUTHOR_NAME="${APP_AUTHOR_NAME}"
    APP_AUTHOR_EMAIL="${APP_AUTHOR_EMAIL}"
    APP_AUTHOR_URL="${APP_AUTHOR_URL}"
    APP_COPYRIGHT="${APP_COPYRIGHT}"
    APP_ID="${APP_ID}"
)


if(APPLE)
    set_target_properties(dsdesk PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/resources/Info.plist.in"
        MACOSX_BUNDLE_ICON_FILE "icon.icns"
    )
    target_link_libraries(dsdesk PRIVATE "-framework Security")

    # Copy icon into bundle Resources
    add_custom_command(TARGET dsdesk POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.icns"
            "$<TARGET_BUNDLE_DIR:dsdesk>/Contents/Resources/icon.icns"
        COMMENT "Copying icon.icns to bundle..."
    )


elseif(WIN32)
    set_target_properties(dsdesk PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    target_sources(dsdesk PRIVATE resources/app.rc)

    # Deploy Qt DLLs for Windows
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt)
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET dsdesk POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                --qmldir "${CMAKE_SOURCE_DIR}/src/qml"
                --plugindir "${CMAKE_BINARY_DIR}/deploy/plugins"
                --libdir "${CMAKE_BINARY_DIR}/deploy"
                --no-translations
                "$<TARGET_FILE:dsdesk>"
            COMMENT "Running windeployqt..."
        )
        install(DIRECTORY "${CMAKE_BINARY_DIR}/deploy/" DESTINATION bin)
    endif()
else()
    install(TARGETS dsdesk RUNTIME DESTINATION bin)
    install(FILES resources/${APP_ID}.desktop DESTINATION share/applications)
    install(FILES resources/icons/${APP_ID}.svg DESTINATION share/icons/hicolor/scalable/apps)
    configure_file(resources/${APP_ID}.metainfo.xml.in ${CMAKE_CURRENT_BINARY_DIR}/${APP_ID}.metainfo.xml @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${APP_ID}.metainfo.xml DESTINATION share/metainfo)
endif()

install(TARGETS dsdesk
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

# CPack configuration
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_PACKAGE_CONTACT "${APP_AUTHOR_EMAIL}")
set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${APP_DESCRIPTION}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

# DEB
set(CPACK_DEBIAN_PACKAGE_DEPENDS "qml6-module-qtquick, qml6-module-qtquick-controls, libqt6core6, libqt6gui6, libqt6network6, libqt6qml6, libqt6quick6")
set(CPACK_DEBIAN_PACKAGE_SECTION "net")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/yourusername/DSDesk")

# RPM
if(EXISTS "/etc/os-release")
    file(READ "/etc/os-release" OS_RELEASE_CONTENT)
    if(OS_RELEASE_CONTENT MATCHES "ID(_LIKE)?=.*opensuse")
        set(IS_OPENSUSE TRUE)
    endif()
endif()

if(IS_OPENSUSE)
    set(CPACK_RPM_PACKAGE_REQUIRES "libQt6Core6, libQt6Gui6, libQt6Network6, libQt6Widgets6, libQt6Qml6, libQt6Quick6")
else()
    set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtbase, qt6-qtdeclarative")
endif()

set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Internet")
set(CPACK_RPM_PACKAGE_URL "${APP_HOMEPAGE}")

# NSIS (Windows Inno Setup-like)
set(CPACK_NSIS_DISPLAY_NAME "${PROJECT_NAME}")
set(CPACK_NSIS_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_NSIS_HELP_LINK "${APP_HOMEPAGE}")
set(CPACK_NSIS_URL_INFO_ABOUT "${APP_HOMEPAGE}")
set(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '$DESKTOP\\\\DSDesk.lnk' '$INSTDIR\\\\bin\\\\dsdesk.exe'")
set(CPACK_NSIS_DELETE_ICONS_EXTRA "Delete '$DESKTOP\\\\DSDesk.lnk'")

# DMG (macOS Drag and Drop)
set(CPACK_DMG_VOLUME_NAME "${PROJECT_NAME}")
set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/resources/dmg-background.png")

# macOS: deploy Qt frameworks and re-sign during packaging
set(CPACK_INSTALL_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/codesign_bundle.cmake")

include(CPack)
