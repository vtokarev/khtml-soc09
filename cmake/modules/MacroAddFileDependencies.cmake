# MACRO_ADD_FILE_DEPENDENCIES(<_file> depend_files...)

# Copyright (c) 2006, Alexander Neundorf, <neundorf@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


MACRO (MACRO_ADD_FILE_DEPENDENCIES _file)

   GET_SOURCE_FILE_PROPERTY(_deps ${_file} OBJECT_DEPENDS)
   if (_deps)
      set(_deps ${_deps} ${ARGN})
   else (_deps)
      set(_deps ${ARGN})
   endif (_deps)

   SET_SOURCE_FILES_PROPERTIES(${_file} PROPERTIES OBJECT_DEPENDS "${_deps}")

ENDMACRO (MACRO_ADD_FILE_DEPENDENCIES)
