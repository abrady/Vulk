

###############################################
## Shader Related
###############################################

######
## Generate Shader Headers
######

# we do this first so the most up to date includes are available to the shader compilation
set(SHADER_GENERATED_HEADER_DIR "${GENERATED_RESOURCES_TARGET_DIR}/Shaders")
set(SHADER_GENERATED_HEADER_FILE "${SHADER_GENERATED_HEADER_DIR}/VulkShaderEnums_generated.glsl")
file(MAKE_DIRECTORY ${SHADER_GENERATED_HEADER_DIR})
add_custom_command(
    OUTPUT ${SHADER_GENERATED_HEADER_FILE}
    COMMAND $<TARGET_FILE:BuildTool> GenVulkShaderEnums ${SHADER_GENERATED_HEADER_FILE}
    DEPENDS BuildTool "${CMAKE_SOURCE_DIR}/Source/Schemas/thrift/VulkShaderEnums.thrift"
    COMMENT "Generating shader includes header from ${SHADER_GENERATED_HEADER_FILE}"
)

######
## Shaders
######

set(GLSLC_FLAGS "-g" "--target-env=vulkan1.3" "-I${SHADER_GENERATED_HEADER_DIR}" "-I${CMAKE_SOURCE_DIR}/Source/Shaders/Common") # build debug symbols and turn off optimizations
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND GLSLC_FLAGS "-O0")
endif()

# TODO: glslc has a -M option that will print a makefile dependency, so we could use that instead of globbing
file(GLOB SHADER_COMMON_FILES "${CMAKE_SOURCE_DIR}/Source/Shaders/Common/*")

# Compile vertex shaders
file(MAKE_DIRECTORY ${SHADER_DEST_DIR}/Vert)
file(GLOB_RECURSE VERTEX_SHADERS CONFIGURE_DEPENDS "${SHADER_SOURCE_DIR}/Vert/*")
foreach(SHADER ${VERTEX_SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    set(SHADER_OUTPUT "${SHADER_DEST_DIR}/Vert/${SHADER_NAME}.vertspv")
    add_custom_command(
        OUTPUT "${SHADER_OUTPUT}"
        COMMAND glslc ${GLSLC_FLAGS} "${SHADER}" -o "${SHADER_OUTPUT}"
        DEPENDS ${SHADER} ${SHADER_COMMON_FILES} ${SHADER_GENERATED_HEADER_FILE}
        COMMENT "building vertex shader ${SHADER_OUTPUT} from ${SHADER} flags ${GLSLC_FLAGS}"
    )
    list(APPEND COMPILED_SHADERS "${SHADER_OUTPUT}")
endforeach()
# Compile fragment shaders
file(MAKE_DIRECTORY ${SHADER_DEST_DIR}/Frag)
file(GLOB_RECURSE FRAGMENT_SHADERS CONFIGURE_DEPENDS "${SHADER_SOURCE_DIR}/Frag/*")
foreach(SHADER ${FRAGMENT_SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    set(SHADER_OUTPUT "${SHADER_DEST_DIR}/Frag/${SHADER_NAME}.fragspv")
    add_custom_command(
        OUTPUT "${SHADER_OUTPUT}"
        COMMAND glslc ${GLSLC_FLAGS} "${SHADER}" -o "${SHADER_OUTPUT}"
        DEPENDS ${SHADER} ${SHADER_COMMON_FILES} ${SHADER_GENERATED_HEADER_FILE}
        COMMENT "building fragment shader glslc ${GLSLC_FLAGS} ${SHADER} -o ${SHADER_OUTPUT}"
    )
    list(APPEND COMPILED_SHADERS "${SHADER_OUTPUT}")
endforeach()

# Compile Geometry shaders
file(MAKE_DIRECTORY ${SHADER_DEST_DIR}/Geom)
file(GLOB_RECURSE GEOMETRY_SHADERS CONFIGURE_DEPENDS "${SHADER_SOURCE_DIR}/Geom/*")
foreach(SHADER ${GEOMETRY_SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    set(SHADER_OUTPUT "${SHADER_DEST_DIR}/Geom/${SHADER_NAME}.geomspv")
    add_custom_command(
        OUTPUT "${SHADER_OUTPUT}"
        COMMAND glslc ${GLSLC_FLAGS} "${SHADER}" -o "${SHADER_OUTPUT}"
        DEPENDS ${SHADER} ${SHADER_COMMON_FILES} ${SHADER_GENERATED_HEADER_FILE}
        COMMENT "building geometry shader ${SHADER} from ${SHADER} flags ${GLSLC_FLAGS}"
    )
    list(APPEND COMPILED_SHADERS "${SHADER_OUTPUT}")
endforeach()

# Add a custom target to trigger the shader compilation
add_custom_target(
    CompileShadersTarget 
    DEPENDS ${COMPILED_SHADERS}
)
#message("shaders ${COMPILED_SHADERS}")

# Append each file name to the file
foreach(SOURCE_FILE ${COMPILED_SHADERS})
    file(APPEND ${ASSETS_OUTPUT_FILE} "${SOURCE_FILE}\n")
endforeach()
