
project(GenSchemaFiles)

find_package(FBThrift CONFIG REQUIRED)
set(GenSchemaFiles ALL DEPENDS ${GENERATED_HEADERS} ${THRIFT_SOURCES})

######
## Thrift generation
######

# Find the Thrift compiler.
find_program(THRIFT_COMPILER thrift1)

set(THRIFT_INCLUDE_DIR "${CMAKE_BINARY_DIR}/generated/thrift_gen")
make_directory(${THRIFT_INCLUDE_DIR})

set(THRIFT_CPP_FILES)

# Define a function to compile Thrift files.
function(generate_thrift_file THRIFT_FILE)
    get_filename_component(file_name_we ${THRIFT_FILE} NAME_WE)
    set(generated_headers 
        "${THRIFT_INCLUDE_DIR}/gen-cpp2/${file_name_we}_clients.h"
        "${THRIFT_INCLUDE_DIR}/gen-cpp2/${file_name_we}_constants.h"
        "${THRIFT_INCLUDE_DIR}/gen-cpp2/${file_name_we}_data.h"
        "${THRIFT_INCLUDE_DIR}/gen-cpp2/${file_name_we}_metadata.h"
        "${THRIFT_INCLUDE_DIR}/gen-cpp2/${file_name_we}_types.h"
        "${THRIFT_INCLUDE_DIR}/gen-cpp2/${file_name_we}_types.tcc"
    )
    set (generated_cpps
        "${THRIFT_INCLUDE_DIR}/gen-cpp2/${file_name_we}_constants.cpp"
        "${THRIFT_INCLUDE_DIR}/gen-cpp2/${file_name_we}_data.cpp"
        "${THRIFT_INCLUDE_DIR}/gen-cpp2/${file_name_we}_metadata.cpp"
        "${THRIFT_INCLUDE_DIR}/gen-cpp2/${file_name_we}_types.cpp"
    )

    # After generating your headers...
    foreach(CPP_FILE ${generated_cpps})
        list(APPEND CPP_FILES ${CPP_FILE})
    endforeach()

    list(APPEND THRIFT_CPP_FILES ${CPP_FILES})
    set(THRIFT_CPP_FILES ${THRIFT_CPP_FILES} PARENT_SCOPE)

    message("cpp files: ${CPP_FILES}")
    add_custom_command(
        OUTPUT ${generated_headers} ${generated_cpps}
        COMMAND ${THRIFT_COMPILER} --gen mstch_cpp2:include_prefix=. -o ${THRIFT_INCLUDE_DIR} ${THRIFT_FILE}
        DEPENDS ${THRIFT_FILE}
        COMMENT "Generating Thrift sources for ${THRIFT_FILE}: ${THRIFT_COMPILER} --gen mstch_cpp2:include_prefix=. -o ${THRIFT_INCLUDE_DIR} ${THRIFT_FILE}"
    )
    set_source_files_properties(${generated_headers} PROPERTIES GENERATED TRUE)

    # Include the generated files in the parent scope
    set(THRIFT_SOURCES ${THRIFT_SOURCES} ${generated_headers} PARENT_SCOPE)
endfunction()

# Gen Thrift files.
file(GLOB THRIFT_SCHEMA_FILES "${CMAKE_SOURCE_DIR}/Source/Schemas/Thrift/*.thrift")
message("thrift files: ${THRIFT_SCHEMA_FILES}")

# Generate commands for each schema file
foreach(SCHEMA_FILE IN LISTS THRIFT_SCHEMA_FILES)
  generate_thrift_file(${SCHEMA_FILE})
endforeach()
 
# Create an INTERFACE library target
message("thrift headers: ${THRIFT_SOURCES}")
message("cpp files: ${THRIFT_CPP_FILES}")
message("fbthrift: ${FBTHRIFT_CMAKE_DIR}")
get_cmake_property(_variableNames VARIABLES)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()

add_custom_target(GenerateCppFiles DEPENDS ${THRIFT_CPP_FILES})
add_library(GenSchemaFiles main.cpp ${THRIFT_CPP_FILES} ${GENERATED_HEADERS_DIR} ${GENERATED_HEADERS} ${THRIFT_INCLUDE_DIR} ${THRIFT_SOURCES})
if (MSVC)
    target_compile_options(GenSchemaFiles PRIVATE /wd4267)
    set_target_properties(GenSchemaFiles PROPERTIES LINK_FLAGS "/NODEFAULTLIB:library")
endif()

# target_link_libraries(GenSchemaFiles PRIVATE thrift::thrift)
target_link_libraries(GenSchemaFiles PRIVATE FBThrift::async FBThrift::thrift FBThrift::compiler FBThrift::mustache)


# Now other targets can link against MyLibrary
# and include the headers from GENERATED_HEADERS_DIR and THRIFT_INCLUDE_DIR

target_include_directories(GenSchemaFiles PUBLIC ${GENERATED_HEADERS_DIR}/gen-cpp2 ${GENERATED_HEADERS_DIR} ${THRIFT_INCLUDE_DIR})
