project(nepomukcore)

# Some problems with conflicting qHash implementations
KDE4_NO_ENABLE_FINAL(nepomukcore)

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=300000)

include_directories(
  ${nepomuk_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/nepomuk/
  ${nepomukcore_SOURCE_DIR}
  ${kdecore_SOURCE_DIR}/utils
  ${CMAKE_SOURCE_DIR}
  ${KDE4_INCLUDES}
  ${KDE4_KIO_INCLUDES}
  ${QT_INCLUDES}
  ${SOPRANO_INCLUDE_DIR}
)

set(nepomuk_LIB_SRCS
  variant.cpp
  resourcedata.cpp
  resourcemanager.cpp
  resourcefiltermodel.cpp
  nepomukmainmodel.cpp
  tools.cpp
  ui/kratingwidget.cpp
  ui/ktagcloudwidget.cpp
  ui/kblocklayout.cpp
  ui/kmetadatatagcloud.cpp
  ui/kmetadatatagwidget.cpp
  ui/kautoscrollarea.cpp
  ui/ktagdisplaywidget.cpp
  ui/kratingpainter.cpp
  ui/nepomukmassupdatejob.cpp
  resource.cpp
  thing.cpp
  tag.cpp
  ontology/entity.cpp
  ontology/ontology.cpp
  ontology/class.cpp
  ontology/property.cpp
  ontology/literal.cpp
  ontology/ontologyloader.cpp
  ontology/ontologymanager.cpp
  ontology/nepomukontologyloader.cpp
  ontology/fileontologyloader.cpp
  ontology/desktopontologyloader.cpp
  ontology/global.cpp
  ontology/entitymanager.cpp
  nie.cpp
  nfo.cpp
  pimo.cpp
  nepomukservice.cpp
  graphwrapper.cpp
)


kde4_add_library(nepomuk SHARED ${nepomuk_LIB_SRCS})

add_dependencies(nepomuk nepomuk-rcgen)

target_link_libraries(nepomuk
  ${SOPRANO_LIBRARIES}
  ${SOPRANO_CLIENT_LIBRARIES}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTGUI_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${KDE4_KDECORE_LIBRARY}
  kdeui
)

target_link_libraries(nepomuk  LINK_INTERFACE_LIBRARIES  ${KDE4_KDEUI_LIBS} ${SOPRANO_LIBRARIES})


set_target_properties(nepomuk PROPERTIES
   VERSION ${GENERIC_LIB_VERSION}
   SOVERSION ${GENERIC_LIB_SOVERSION}
)

install(TARGETS nepomuk EXPORT kdelibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES
  variant.h
  resourcemanager.h
  tools.h
  nepomukservice.h
  resource.h
  thing.h
  tag.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/nepomuk COMPONENT Devel
)

add_subdirectory(ui)
add_subdirectory(ontology)
add_subdirectory(test)
