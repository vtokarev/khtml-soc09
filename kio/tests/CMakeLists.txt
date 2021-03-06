set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
# for <kio/global.h>
include_directories( ${KDE4_KIO_INCLUDES} )

MACRO(KIO_UNIT_TESTS)
       FOREACH(_testname ${ARGN})
               kde4_add_unit_test(${_testname} ${_testname}.cpp)
               target_link_libraries(${_testname} ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})
       ENDFOREACH(_testname)
ENDMACRO(KIO_UNIT_TESTS)
MACRO(KIO_EXECUTABLE_TESTS)
       FOREACH(_testname ${ARGN})
               kde4_add_executable(${_testname} TEST ${_testname}.cpp)
               target_link_libraries(${_testname} ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY})
       ENDFOREACH(_testname)
ENDMACRO(KIO_EXECUTABLE_TESTS)

# jobtest seems to trigger a ctest problem; jobtest finishes and ctest waits for ever

KIO_UNIT_TESTS(
  karchivetest
  krununittest
  kprotocolinfotest
  jobtest
  jobremotetest
  kurlcompletiontest
  kfileitemtest
  kbookmarktest
  kacltest
  kdirmodeltest
  kdirlistertest
  kmountpointtest
  fileundomanagertest
)

KIO_EXECUTABLE_TESTS(
  ksycocatest
  getalltest
  kruntest
  ktartest
  kziptest
  kioslavetest
  kdirwatchtest
  speed
  kiopassdlgtest
  kscantest
  kdirlistertest_gui
  previewtest
  kionetrctest
  kpropsdlgtest
  kmfitest
  dataprotocoltest
  ksycocaupdatetest
  netaccesstest
  kdirwatchtest_gui
  kdirmodeltest_gui
  kmimetypechoosertest_gui
  ksslkeygentest
  kdbusservicestartertest
)

kde4_add_unit_test(klimitediodevicetest klimitediodevicetest.cpp ../kio/klimitediodevice.cpp)
target_link_libraries(klimitediodevicetest ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY})
