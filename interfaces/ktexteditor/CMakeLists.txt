include_directories( ${KDE4_KPARTS_INCLUDES} )

########### next target ###############

set(ktexteditor_LIB_SRCS
   ktexteditor.cpp
   range.cpp
   cursor.cpp
   templateinterface.cpp
   editorchooser.cpp
   attribute.cpp
   cursorfeedback.cpp
   rangefeedback.cpp
   smartcursor.cpp
   smartrange.cpp
   codecompletioninterface.cpp
   codecompletionmodel.cpp
   codecompletionmodelcontrollerinterface.cpp
   configinterface.cpp
   smartinterface.cpp
   )


kde4_add_ui_files(ktexteditor_LIB_SRCS editorchooser_ui.ui )

kde4_add_library(ktexteditor SHARED ${ktexteditor_LIB_SRCS})

target_link_libraries(ktexteditor  ${KDE4_KDECORE_LIBS} ${QT_QTDBUS_LIBRARY} ${KDE4_KPARTS_LIBS} )
target_link_libraries(ktexteditor  LINK_INTERFACE_LIBRARIES ${KDE4_KPARTS_LIBS} )

set_target_properties(ktexteditor
   PROPERTIES VERSION ${GENERIC_LIB_VERSION}
   SOVERSION ${GENERIC_LIB_SOVERSION}
)
install(TARGETS ktexteditor EXPORT kdelibsLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############
install( FILES
    ktexteditor_export.h
    factory.h
    editor.h
    document.h
    view.h
    plugin.h
    cursor.h
    range.h
    configpage.h
    searchinterface.h
    markinterface.h
    sessionconfiginterface.h
    editorchooser.h
    texthintinterface.h
    variableinterface.h
    templateinterface.h
    modificationinterface.h
    commandinterface.h
    smartinterface.h
    attribute.h
    cursorfeedback.h
    rangefeedback.h
    smartcursor.h
    smartcursornotifier.h
    smartcursorwatcher.h
    smartrange.h
    smartrangenotifier.h
    smartrangewatcher.h
    codecompletioninterface.h
    codecompletionmodel.h
    codecompletionmodelcontrollerinterface.h
    configinterface.h
    containerinterface.h
    annotationinterface.h
    loadsavefiltercheckplugin.h
    DESTINATION  ${INCLUDE_INSTALL_DIR}/ktexteditor COMPONENT Devel)

install( FILES ktexteditor.desktop ktexteditorplugin.desktop  DESTINATION  ${SERVICETYPES_INSTALL_DIR} )
install( FILES kcm_ktexteditor.desktop  DESTINATION  ${DATA_INSTALL_DIR}/kcm_componentchooser )

install( FILES ktexteditor_loadsavefiltercheckplugin.desktop  DESTINATION  ${SERVICETYPES_INSTALL_DIR} )
