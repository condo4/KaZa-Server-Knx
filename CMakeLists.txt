cmake_minimum_required(VERSION 3.16)

project(KnxPlugin VERSION 0.1 LANGUAGES CXX)

include(GNUInstallDirs)

if(NOT CMAKE_CROSSCOMPILING AND NOT DISABLE_PKG)
    if(EXISTS /etc/debian_version)
        message(STATUS "Build for debian package")
        set(BUILD_DEBIAN_PACKAGE ON)
    endif()
endif()


set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use minizip for KNXPROJ
include(FindPkgConfig)
if (PKG_CONFIG_FOUND)
        PKG_CHECK_MODULES(UNZIP minizip)
endif (PKG_CONFIG_FOUND)

find_package(Qt6 6.2 COMPONENTS Quick Xml REQUIRED)

find_package(KaZa REQUIRED)
include_directories(${KAZA_INCLUDE_DIR})

add_library(KnxPlugin SHARED src/knxbus.cpp src/knxbus.h src/knxobject.cpp src/knxobject.h src/plugin.cpp src/plugin.h)

if(NOT DEFINED QML_MODULE_INSTALL_PATH)
    if(EXISTS /usr/lib64/qt6/qml/builtins.qmltypes)
        set(QML_MODULE_INSTALL_PATH "/usr/lib64/qt6/qml/")
    elseif(EXISTS /usr/lib/aarch64-linux-gnu/qt6/qml/builtins.qmltypes)
        set(QML_MODULE_INSTALL_PATH "/usr/lib/aarch64-linux-gnu/qt6/qml/")
    else()
        set(QML_MODULE_INSTALL_PATH ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/qml )
    endif()
endif()

target_compile_definitions(KnxPlugin PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(KnxPlugin PRIVATE Qt6::Quick Qt6::Xml  eibclient ${UNZIP_LIBRARIES} minizip)
target_include_directories(KnxPlugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS KnxPlugin DESTINATION ${QML_MODULE_INSTALL_PATH}/org/kazoe/knx)
install(FILES qmldir DESTINATION ${QML_MODULE_INSTALL_PATH}/org/kazoe/knx)


if(BUILD_DEBIAN_PACKAGE)
    set(DEB_DEPEND "")
    file(READ /etc/debian_version DEBIAN_VERSION)
    string(REGEX MATCH "^[0-9]+" DEBIAN_MAJOR "${DEBIAN_VERSION}")
    set(CPACK_GENERATOR "DEB")
    set(CPACK_PACKAGE_NAME "kaza-knx")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    set(CPACK_PACKAGE_VERSION_PATCH "${CMAKE_PROJECT_VERSION_PATCH}-debian${DEBIAN_MAJOR}")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6serialbus6 qml6-module-qtqml")
    set(CPACK_PACKAGE_DESCRIPTION "KNX integration for KaZa Server")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Fabien Proriol <fabien.proriol@kazoe.org>")
    include(CPack)
endif()
