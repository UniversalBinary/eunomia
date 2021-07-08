cmake_minimum_required(VERSION 3.19)

if(WIN32 AND DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(Eunomia)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if (WIN32)
    set(CMAKE_PREFIX_PATH "C:/Qt/5.12.11/msvc2017_64/lib/cmake")
endif()

if (UNIX)
    set(CMAKE_PREFIX_PATH "/opt/Qt/6.1.0/gcc_64/lib/cmake")
endif()


set(QT_VERSION 6)
set(REQUIRED_LIBS Core Gui Widgets Xml)
set(REQUIRED_LIBS_QUALIFIED Qt6::Core Qt6::Gui Qt6::Widgets Qt::Xml)

add_executable(${PROJECT_NAME} main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        utils.hpp
        bookingOnPoint.hpp
        abstractGateway.hpp
        imapEmailGateway.hpp
        resources.qrc
        bookingOnPointList.hpp server_status_terminal.hpp)

if (NOT CMAKE_PREFIX_PATH)
    message(FATAL_ERROR "CMAKE_PREFIX_PATH is not defined, you may need to set it to the location of the Qt installation prefix.")
endif ()

find_package(Qt${QT_VERSION} COMPONENTS ${REQUIRED_LIBS} REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC ${REQUIRED_LIBS_QUALIFIED})

if (WIN32)
    set(DEBUG_SUFFIX)
    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif ()
    set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
    if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
        set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
            set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        endif ()
    endif ()
    if (EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
    endif ()
    foreach (QT_LIB ${REQUIRED_LIBS})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/bin/Qt${QT_VERSION}${QT_LIB}${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endforeach (QT_LIB)
endif ()

set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.74 COMPONENTS system regex filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC ${Boost_LIBRARIES})

find_library(VMIME_LIBRARIES NAMES vmime REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC ${VMIME_LIBRARIES})

find_library(PODOFO_LIBRARIES NAMES podofo REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC ${PODOFO_LIBRARIES})

find_library(POPPLER_LIBRARIES NAMES poppler REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC ${POPPLER_LIBRARIES})