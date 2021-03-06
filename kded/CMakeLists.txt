project(kded)

add_subdirectory( test )

include_directories( ${KDE4_KIO_INCLUDES} )

########### next target ###############

set(kded_KDEINIT_SRCS kded.cpp kdedadaptor.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kded.xml # just so that it gets generated
)


kde4_add_kdeinit_executable(kded4  ${kded_KDEINIT_SRCS})

target_link_libraries(kdeinit_kded4  ${KDE4_KIO_LIBS} ${X11_LIBRARIES})

install(TARGETS kdeinit_kded4 ${INSTALL_TARGETS_DEFAULT_ARGS})

target_link_libraries( kded4 kdeinit_kded4 )
install(TARGETS kded4 ${INSTALL_TARGETS_DEFAULT_ARGS} )


qt4_generate_dbus_interface( kdedadaptor.h org.kde.kded.xml )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kded.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR})



########### next target ###############

set(kbuildsycoca_KDEINIT_SRCS
   kbuildsycoca.cpp
   kbuildmimetypefactory.cpp
   kmimefileparser.cpp
   kmimeassociations.cpp
   kbuildservicetypefactory.cpp
   kbuildservicefactory.cpp
   kbuildservicegroupfactory.cpp
   #kbuildimageiofactory.cpp
   kbuildprotocolinfofactory.cpp
   kctimefactory.cpp
   vfolder_menu.cpp
   )


# We need to add a '4' so that kde3 apps running kbuildsycoca don't run the kde4 one.
kde4_add_kdeinit_executable( kbuildsycoca4 NOGUI ${kbuildsycoca_KDEINIT_SRCS})

# needs kio for KServiceGroup
target_link_libraries(kdeinit_kbuildsycoca4  ${KDE4_KIO_LIBS} )

install(TARGETS kdeinit_kbuildsycoca4 ${INSTALL_TARGETS_DEFAULT_ARGS})

target_link_libraries( kbuildsycoca4 kdeinit_kbuildsycoca4 )
install(TARGETS kbuildsycoca4 ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### next target ###############

set(kdontchangethehostname_SRCS khostname.cpp )


kde4_add_executable(kdontchangethehostname NOGUI ${kdontchangethehostname_SRCS})

target_link_libraries(kdontchangethehostname  ${KDE4_KDECORE_LIBS} )

install(TARGETS kdontchangethehostname DESTINATION ${LIBEXEC_INSTALL_DIR} )

########### install files ###############
if (WIN32)
install( FILES applications.menu  DESTINATION  ${SHARE_INSTALL_PREFIX}/xdg/menus )
else (WIN32)
install( FILES applications.menu  DESTINATION  ${SYSCONF_INSTALL_DIR}/xdg/menus )
endif (WIN32)
install( FILES kdedmodule.desktop DESTINATION  ${SERVICETYPES_INSTALL_DIR} )
install( FILES kded.upd           DESTINATION  ${DATA_INSTALL_DIR}/kconf_update )


