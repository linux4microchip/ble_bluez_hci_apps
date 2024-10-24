cmake_minimum_required(VERSION 3.22)

PROJECT(ble-uart-bluez)

add_executable(ble-uart-bluez)

SET (GATTSRV_DIR ${ble-apps_SOURCE_DIR}/services)
SET (PROFILE_DIR ${ble-apps_SOURCE_DIR}/profiles)
SET (APP_DIR ${ble-apps_SOURCE_DIR}/apps/ble_uart_app/src)

SET (GATT_SERVICE_SRCS ${GATTSRV_DIR}/ble_trs/ble_trs.c)
SET (PROFILE_SRCS ${PROFILE_DIR}/ble_trsp/ble_trsps.c ${PROFILE_DIR}/ble_trsp/ble_trspc.c)

SET (APP_SRCS ${APP_DIR}/main.c
              ${APP_DIR}/app_dbp.c
              ${APP_DIR}/application.c
              ${APP_DIR}/app_cmd.c
              ${APP_DIR}/app_mgmt.c
              ${APP_DIR}/app_timer.c
              ${APP_DIR}/app_utility.c
              ${APP_DIR}/app_scan.c
              ${APP_DIR}/app_adv.c
              ${APP_DIR}/app_sm.c
              ${APP_DIR}/app_ble_handler.c
              ${APP_DIR}/app_trps.c
              ${APP_DIR}/app_trpc.c
              ${APP_DIR}/app_trp_common.c
              ${APP_DIR}/app_agent.c)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED dbus-1)
pkg_check_modules(GLIB REQUIRED glib-2.0)

include_directories(${DBUS_INCLUDE_DIRS})
include_directories(${GLIB_INCLUDE_DIRS})

target_sources (ble-uart-bluez PRIVATE ${GATT_SERVICE_SRCS} ${PROFILE_SRCS} ${APP_SRCS})
target_include_directories(ble-uart-bluez PUBLIC ${GATTSRV_DIR} ${PROFILE_DIR})
target_link_libraries(ble-uart-bluez PUBLIC dbus-1 glib-2.0 bluetooth gdbus-internal shared-glib bluetooth-internal readline)
