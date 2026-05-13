# Deploy Qt frameworks and re-sign the app bundle during CPack
set(APP_BUNDLE "${CPACK_TEMPORARY_INSTALL_DIRECTORY}/dsdesk.app")

# Find macdeployqt - search common Qt installation paths
find_program(MACDEPLOYQT_EXECUTABLE macdeployqt
    HINTS /opt/homebrew/bin /usr/local/bin /opt/homebrew/opt/qt6/bin
)
if(MACDEPLOYQT_EXECUTABLE AND EXISTS "${APP_BUNDLE}")
    message(STATUS "Running macdeployqt on ${APP_BUNDLE}")
    # Try to find qmldir relative to the source
    set(QML_DIR "")
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../src/qml")
        set(QML_DIR "-qmldir=${CMAKE_CURRENT_LIST_DIR}/../src/qml")
    endif()
    execute_process(
        COMMAND "${MACDEPLOYQT_EXECUTABLE}" "${APP_BUNDLE}" ${QML_DIR}
        RESULT_VARIABLE RESULT
    )
    if(NOT RESULT EQUAL 0)
        message(WARNING "macdeployqt failed with result: ${RESULT}")
    endif()
endif()

# Re-sign the app bundle
find_program(CODESIGN_EXECUTABLE codesign)
if(CODESIGN_EXECUTABLE AND EXISTS "${APP_BUNDLE}")
    message(STATUS "Re-signing ${APP_BUNDLE}")
    execute_process(
        COMMAND "${CODESIGN_EXECUTABLE}" --force --deep --sign - "${APP_BUNDLE}"
        RESULT_VARIABLE RESULT
    )
    if(NOT RESULT EQUAL 0)
        message(WARNING "Code signing failed with result: ${RESULT}")
    endif()
endif()
