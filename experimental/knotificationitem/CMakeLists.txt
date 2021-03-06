
# Project needs a name of course
project(notification)

add_subdirectory(cmake)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories(
   ${CMAKE_SOURCE_DIR}
   ${CMAKE_BINARY_DIR}
   ${KDE4_INCLUDES}
   )

set(knotificationitem_SRCS knotificationitem.cpp knotificationitemdbus_p.cpp)
set(knotificationitemtest_SRC test/knotificationitemtest.cpp)

QT4_ADD_DBUS_ADAPTOR(knotificationitem_SRCS     org.kde.NotificationItem.xml
                     knotificationitemdbus_p.h Experimental::KNotificationItemDBus)

set(systemtraydaemon_xml org.kde.NotificationItemWatcher.xml)
QT4_ADD_DBUS_INTERFACE(knotificationitem_SRCS ${systemtraydaemon_xml} notificationwatcher_interface)

set(notifications_xml org.freedesktop.Notifications.xml)
QT4_ADD_DBUS_INTERFACE(knotificationitem_SRCS ${notifications_xml} notifications_interface)

kde4_add_library(knotificationitem-1 SHARED ${knotificationitem_SRCS})
set_target_properties(knotificationitem-1 PROPERTIES VERSION 1.0.0 SOVERSION 1)
target_link_libraries(knotificationitem-1 ${KDE4_KDEUI_LIBS})

kde4_add_executable(knotificationitemtest ${knotificationitemtest_SRC})
target_link_libraries(knotificationitemtest ${KDE4_KDEUI_LIBS} knotificationitem-1)


install(TARGETS knotificationitem-1 EXPORT kdelibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES
    knotificationitem.h
    knotificationitem_export.h
    DESTINATION ${INCLUDE_INSTALL_DIR}/knotificationitem-1 COMPONENT Devel)

install(FILES
    org.kde.NotificationItem.xml
    org.kde.NotificationItemWatcher.xml
    DESTINATION ${KDE4_DBUS_INTERFACES_DIR})
