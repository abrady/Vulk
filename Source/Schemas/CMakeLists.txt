
project(GenSchemaFiles)

find_package(flatbuffers CONFIG REQUIRED)
find_package(Thrift CONFIG REQUIRED)

######
## Flatbuffer generation
######
set(GENERATED_HEADERS_DIR ${CMAKE_BINARY_DIR}/generated/flatbuf)
set(FLATBUFFERS_FLATC_EXECUTABLE $<TARGET_FILE:flatbuffers::flatc>)
make_directory(${GENERATED_HEADERS_DIR})
file(MAKE_DIRECTORY ${GENERATED_HEADERS_DIR})

# List of schema files
file(GLOB FLATBUF_SCHEMA_FILES "${CMAKE_SOURCE_DIR}/Source/Schemas/Flatbuf/*.fbs")
#message("schema files: ${FLATBUF_SCHEMA_FILES}")

# Generate commands for each schema file
foreach(SCHEMA_FILE IN LISTS FLATBUF_SCHEMA_FILES)
    #message("schema file: ${SCHEMA_FILE}")
    get_filename_component(SCHEMA_NAME ${SCHEMA_FILE} NAME_WE)
    set(GENERATED_HEADER ${GENERATED_HEADERS_DIR}/${SCHEMA_NAME}_generated.h)
    add_custom_command(
        OUTPUT ${GENERATED_HEADER}
        COMMAND ${FLATBUFFERS_FLATC_EXECUTABLE} --cpp -o ${GENERATED_HEADERS_DIR} ${SCHEMA_FILE}
        MAIN_DEPENDENCY ${SCHEMA_FILE}
        COMMENT "Compiling FlatBuffers schema: ${SCHEMA_FILE}"
    )
    list(APPEND GENERATED_HEADERS ${GENERATED_HEADER})
endforeach()

# add_custom_target(GenSchemaFiles ALL DEPENDS ${GENERATED_HEADERS} ${THRIFT_SOURCES})

######
## Thrift generation
######

# Find the Thrift compiler.
find_program(THRIFT_COMPILER thrift)

set(THRIFT_INCLUDE_DIR "${CMAKE_BINARY_DIR}/generated/thrift_gen")
make_directory(${THRIFT_INCLUDE_DIR})

set(THRIFT_CPP_FILES)

# Define a function to compile Thrift files.
function(compile_thrift_file THRIFT_FILE)
    get_filename_component(file_name_we ${THRIFT_FILE} NAME_WE)
    set(generated_sources 
        "${THRIFT_INCLUDE_DIR}/${file_name_we}_types.h"
        # "${THRIFT_INCLUDE_DIR}/${file_name_we}_types.cpp"
        # "${THRIFT_INCLUDE_DIR}/${file_name_we}_constants.h"
        # "${THRIFT_INCLUDE_DIR}/${file_name_we}_constants.cpp"
        # "${THRIFT_INCLUDE_DIR}/${file_name_we}_server.skeleton.cpp"
    )

    # After generating your headers...
    foreach(HEADER_FILE ${generated_sources})
        message("header file: ${HEADER_FILE}")
        get_filename_component(FILE_DIR ${HEADER_FILE} PATH)
        get_filename_component(FILE_NAME ${HEADER_FILE} NAME_WE)
        set(FILE_PATH_WITHOUT_EXT "${FILE_DIR}/${FILE_NAME}")
        
        # Append .cpp extension and add it to THRIFT_CPP_FILES list
        list(APPEND CPP_FILES "${FILE_PATH_WITHOUT_EXT}.cpp")
    endforeach()

    list(APPEND THRIFT_CPP_FILES ${CPP_FILES})
    set(THRIFT_CPP_FILES ${THRIFT_CPP_FILES} PARENT_SCOPE)

    message("cpp files: ${CPP_FILES}")
    add_custom_command(
        OUTPUT ${generated_sources} ${CPP_FILES}
        COMMAND ${THRIFT_COMPILER} --gen cpp -out ${THRIFT_INCLUDE_DIR} ${THRIFT_FILE}
        DEPENDS ${THRIFT_FILE}
        COMMENT "Generating Thrift sources for ${THRIFT_FILE}"
    )
    set_source_files_properties(${generated_sources} PROPERTIES GENERATED TRUE)

    # Include the generated files in the parent scope
    set(THRIFT_SOURCES ${THRIFT_SOURCES} ${generated_sources} PARENT_SCOPE)
endfunction()

# Compile Thrift files.
file(GLOB THRIFT_SCHEMA_FILES "${CMAKE_SOURCE_DIR}/Source/Schemas/Thrift/*.thrift")
message("thrift files: ${THRIFT_SCHEMA_FILES}")

# Generate commands for each schema file
foreach(SCHEMA_FILE IN LISTS THRIFT_SCHEMA_FILES)
  compile_thrift_file(${SCHEMA_FILE})
endforeach()
 
# Create an INTERFACE library target
message("thrift headers: ${THRIFT_SOURCES}")
message("cpp files: ${THRIFT_CPP_FILES}")

add_custom_target(GenerateCppFiles DEPENDS ${THRIFT_CPP_FILES})
add_library(GenSchemaFiles main.cpp ${THRIFT_CPP_FILES} ${GENERATED_HEADERS_DIR} ${GENERATED_HEADERS} ${THRIFT_INCLUDE_DIR} ${THRIFT_SOURCES})

target_link_libraries(GenSchemaFiles PRIVATE flatbuffers::flatbuffers)
target_link_libraries(GenSchemaFiles PRIVATE thrift::thrift)

# Now other targets can link against MyLibrary
# and include the headers from GENERATED_HEADERS_DIR and THRIFT_INCLUDE_DIR
target_include_directories(GenSchemaFiles PUBLIC ${GENERATED_HEADERS_DIR} ${THRIFT_INCLUDE_DIR})
