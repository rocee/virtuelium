
OPTION(${PROJECT_NAME}_BUILD_DOCS_DOXYGEN
        "Build doxygen documentation" OFF)
OPTION(${PROJECT_NAME}_BUILD_DOCS_SPHINX
        "Build sphinx documentation" OFF)

SET(${PROJECT_NAME}_BIN_SPHINX_DIR ${${PROJECT_NAME}_DOC_BIN_PATH}/sphinx)
SET(${PROJECT_NAME}_SRC_SPHINX_DIR ${${PROJECT_NAME}_DOC_SRC_PATH}/sphinx)

SET(${PROJECT_NAME}_BIN_DOXY_DIR ${${PROJECT_NAME}_DOC_BIN_PATH}/doxygen)
SET(${PROJECT_NAME}_SRC_DOXY_DIR ${${PROJECT_NAME}_DOC_SRC_PATH}/doxygen)



IF(${PROJECT_NAME}_BUILD_DOCS_DOXYGEN)
  ADD_SUBDIRECTORY(doxygen)
ENDIF(${PROJECT_NAME}_BUILD_DOCS_DOXYGEN)

IF(${PROJECT_NAME}_BUILD_DOCS_SPHINX)
  ADD_SUBDIRECTORY(sphinx)
ENDIF(${PROJECT_NAME}_BUILD_DOCS_SPHINX)

