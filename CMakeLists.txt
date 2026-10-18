cmake_minimum_required(VERSION 3.16)

project(urcontrol-qt VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)
find_package(ALSA REQUIRED)
set(URCONTROL_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/third-party/urcontrol-c"
)

find_program(MAKE_EXECUTABLE NAMES gmake make REQUIRED)

set(URCONTROL_LIBRARY
    "${URCONTROL_DIR}/liburcontrol.a"
)

add_custom_command(
    OUTPUT "${URCONTROL_LIBRARY}"

    COMMAND "${MAKE_EXECUTABLE}"

    WORKING_DIRECTORY "${URCONTROL_DIR}"

    DEPENDS
        "${URCONTROL_DIR}/Makefile"
        "${URCONTROL_DIR}/urcontrol.c"
        "${URCONTROL_DIR}/urcontrol_params.c"

    COMMENT "Building urcontrol-c using its Makefile"
    VERBATIM
)

add_custom_target(urcontrol_build
    DEPENDS "${URCONTROL_LIBRARY}"
)

add_library(urcontrol STATIC IMPORTED GLOBAL)

set_target_properties(urcontrol PROPERTIES
    IMPORTED_LOCATION "${URCONTROL_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${URCONTROL_DIR}"
)

add_dependencies(urcontrol urcontrol_build)

set(TS_FILES urcontrol-qt_ru_RU.ts
    urcontrol_qt_en_US.ts)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(urcontrol-qt
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        audiofader.h audiofader.cpp
        audiolevelmeter.h audiolevelmeter.cpp
        midiport.h
        midiport.cpp

        mixerchannel.h mixerchannel.cpp mixerchannel.ui
        mixerchannel.cpp mixerchannel.h
        device.h device.cpp
        audioknob.h audioknob.cpp
        parameterreader.h
        parameterreader.cpp
        parameterapplier.h parameterapplier.cpp


    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET urcontrol-qt APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

else()
    if(ANDROID)
        add_library(urcontrol-qt SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(urcontrol-qt
            ${PROJECT_SOURCES}
        )
    endif()

endif()

if(QT_VERSION_MAJOR EQUAL 6)
    qt_add_translations(urcontrol-qt
        TS_FILES ${TS_FILES}
        RESOURCE_PREFIX "/i18n"
    )
else()
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(urcontrol-qt PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    urcontrol
    ALSA::ALSA
)

target_include_directories(urcontrol-qt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.urcontrol-qt)
endif()
set_target_properties(urcontrol-qt PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS urcontrol-qt
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(urcontrol-qt)
endif()

configure_file(devices.ini devices.ini COPYONLY)
