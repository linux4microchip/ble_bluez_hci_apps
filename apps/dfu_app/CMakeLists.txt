cmake_minimum_required(VERSION 3.22)

PROJECT (dfu-bluez)

add_executable (dfu-bluez)

SET (APP_DIR ${ble-apps_SOURCE_DIR}/apps/dfu_app/src)

SET (APP_SRCS ${APP_DIR}/main.c
              ${APP_DIR}/application.c
              ${APP_DIR}/app_cmd.c
              ${APP_DIR}/app_timer.c
              ${APP_DIR}/app_hci_dfu.c
              ${APP_DIR}/app_hci_vnd.c)

add_compile_options(-DVERSION="0.9.0.3")

find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED dbus-1)
pkg_check_modules(GLIB REQUIRED glib-2.0)

include_directories(${DBUS_INCLUDE_DIRS})
include_directories(${GLIB_INCLUDE_DIRS})

SET (CMAKE_VERBOSE_MAKEFILE TRUE)

target_sources (dfu-bluez PRIVATE ${APP_SRCS})
target_include_directories(dfu-bluez PUBLIC ${APP_DIR})
target_link_libraries(dfu-bluez PUBLIC glib-2.0 dbus-1 shared-glib bluetooth readline)
