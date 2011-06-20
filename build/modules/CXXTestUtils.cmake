MACRO(ADD_CXXTEST_CPP_TARGET)
  PARSE_ARGUMENTS(CXXTEST "DEPENDS;OUTPUTDIR;LIBRARYDIR" "" ${ARGN})
  CAR(CXXTEST_NAME ${CXXTEST_DEFAULT_ARGS})
  CDR(CXXTEST_FILES ${CXXTEST_DEFAULT_ARGS})

  SET(CXXTEST_EXEC_TARGET ${CXXTEST_NAME})
  SET(CXXTEST_TEST_FILES)
  SET(CXXTEST_H_FILES)
  SET(CXXTEST_OPTIONS --include=sirikata/core/util/Standard.hh)
  #IF(CXXTEST_OUTPUTDIR)
  #  SET(CXXTEST_OPTIONS ${CXXTEST_OPTIONS} -o ${CXXTEST_OUTPUTDIR})
  #ENDIF(CXXTEST_OUTPUTDIR)
  SET(CXXTEST_CPP_FILE ${CMAKE_CURRENT_BINARY_DIR}/test.cc)

  IF(PYTHON_EXECUTABLE)
    SET(CXXTEST_COMPILER ${PYTHON_EXECUTABLE})
    SET(CXXTEST_GEN ${CXXTEST_LIBRARYDIR}/cxxtestgen.py)
  ELSE()
    FIND_PACKAGE(Perl)
    IF(PERL_EXECUTABLE)
      SET(CXXTEST_CPP_FILE test.cc)      #perl cannot output to a full path.
      SET(CXXTEST_COMPILER ${PERL_EXECUTABLE})
      SET(CXXTEST_GEN ${CXXTEST_LIBRARYDIR}/cxxtestgen.pl)
    ELSE()
      MESSAGE(STATUS "!!! Cannot locate python or perl -- tests will not be compiled.")
    ENDIF()
  ENDIF()
  FOREACH(FILE ${CXXTEST_FILES})
    SET(CXXTEST_H_FILE ${FILE})
    SET(CXXTEST_TEST_FILES ${CXXTEST_TEST_FILES} ${CXXTEST_H_FILE})
  ENDFOREACH()
  IF (CXXTEST_COMPILER)
    SET(FINAL_CXXTEST_COMMAND ${CXXTEST_COMPILER} ${CXXTEST_GEN} ${CXXTEST_OPTIONS} -o ${CXXTEST_CPP_FILE} ${CXXTEST_TEST_FILES})
    ADD_CUSTOM_COMMAND(OUTPUT ${CXXTEST_CPP_FILE}
                       COMMAND ${CXXTEST_COMPILER} ${CXXTEST_GEN} ${CXXTEST_OPTIONS} -o ${CXXTEST_CPP_FILE} ${CXXTEST_TEST_FILES}
                       DEPENDS ${CXXTEST_TEST_FILES} ${CXXTEST_DEPENDS}
                       COMMENT "Building ${CXXTEST_TEST_FILES} -> ${CXXTEST_CPP_FILE}")
  ELSE()
    ADD_CUSTOM_COMMAND(OUTPUT ${CXXTEST_CPP_FILE}
                       COMMAND exit 1
                       COMMENT "Unable to build ${CXXTEST_CPP_FILE} because python and perl were not found.")
  ENDIF()

  SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CXXTEST_CPP_FILE})

ENDMACRO(ADD_CXXTEST_CPP_TARGET)
