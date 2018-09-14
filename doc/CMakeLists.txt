find_package(Doxygen REQUIRED)

add_custom_target(doc
                  ALL
                  COMMAND doxygen doxyconfig.in
                  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/doc)
