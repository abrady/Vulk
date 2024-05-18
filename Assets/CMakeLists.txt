##############################################################################
## Assets
##############################################################################
cmake_minimum_required(VERSION 3.10)

project(Assets)

# define a function to copy assets from the source directory to the build directory
function(copy_assets DIR_NAME GLOBAL_ASSETS_VAR)
    set(ASSETS_TO_BUILD ${${GLOBAL_ASSETS_VAR}}) # get the list of assets from parent scope
    set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/Assets/${DIR_NAME}")
    set(DEST_DIR "${BUILT_RESOURCES_TARGET_DIR}/Assets/${DIR_NAME}") 

    file(MAKE_DIRECTORY ${DEST_DIR})
    file(GLOB_RECURSE ASSETS CONFIGURE_DEPENDS "${SOURCE_DIR}/*")
    foreach(ASSET_FILE ${ASSETS})
        file(RELATIVE_PATH REL_PATH "${SOURCE_DIR}" "${ASSET_FILE}")
        set(DEST "${DEST_DIR}/${REL_PATH}")
        get_filename_component(DEST_FILE_DIR "${DEST}" DIRECTORY)
        file(MAKE_DIRECTORY "${DEST_FILE_DIR}")
        #message("adding a copy of ${ASSET_FILE} to ${DEST}")
        add_custom_command(
            OUTPUT "${DEST}"
            COMMAND ${CMAKE_COMMAND} -E copy "${ASSET_FILE}" "${DEST}"
            DEPENDS "${ASSET_FILE}"
            COMMENT "Copying ${DIR_NAME} ${ASSET_FILE} to ${DEST}"
        )
        list(APPEND ASSETS_TO_BUILD "${DEST}")
    endforeach()
    set(${GLOBAL_ASSETS_VAR} "${ASSETS_TO_BUILD}" PARENT_SCOPE) # set the list of assets in parent scope (equivalent to return value)
endfunction()

# accumulate the assets to build in this
set(ASSETS_TO_COPY "")

# these assets don't need any processing, but we need everything in the build
# directory to be usable by the executable
set(STRAIGHT_COPY_DIRECTORIES Scenes Models Textures Materials)
foreach(DIR ${STRAIGHT_COPY_DIRECTORIES})
    copy_assets(${DIR} ASSETS_TO_COPY)
endforeach()


# Append each file name to the file
foreach(SOURCE_FILE ${ASSETS_TO_COPY})
    file(APPEND ${ASSETS_OUTPUT_FILE} "${SOURCE_FILE}\n")
endforeach()


##
# build buildable assets
##

function(build_assets DIR_NAME GLOBAL_ASSETS_VAR)
    set(ASSETS_TO_BUILD ${${GLOBAL_ASSETS_VAR}}) # get the list of assets from parent scope
    set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/Assets/${DIR_NAME}")
    set(DEST_DIR "${BUILT_RESOURCES_TARGET_DIR}/Assets/${DIR_NAME}") 

    file(MAKE_DIRECTORY ${DEST_DIR})
    file(GLOB_RECURSE ASSETS CONFIGURE_DEPENDS "${SOURCE_DIR}/*")
    foreach(ASSET_FILE ${ASSETS})
        file(RELATIVE_PATH REL_PATH "${SOURCE_DIR}" "${ASSET_FILE}")
        set(DEST "${DEST_DIR}/${REL_PATH}")
        get_filename_component(DEST_FILE_DIR "${DEST}" DIRECTORY)
        file(MAKE_DIRECTORY "${DEST_FILE_DIR}")
        add_custom_command(
            OUTPUT "${DEST}"
            COMMAND $<TARGET_FILE:BuildTool> ${DIR_NAME} "${ASSET_FILE}" "${DEST}"
            DEPENDS BuildTool "${ASSET_FILE}" 
            COMMENT "Building ${DIR_NAME} ${ASSET_FILE} to ${DEST}"
        )
        list(APPEND ASSETS_TO_BUILD "${DEST}")
    endforeach()
    set(${GLOBAL_ASSETS_VAR} "${ASSETS_TO_BUILD}" PARENT_SCOPE) # set the list of assets in parent scope (equivalent to return value)
endfunction()

set(ASSETS_TO_BUILD "")
set(BUILDTOOL_RESOURCE_DIRS )
foreach(DIR ${BUILDTOOL_RESOURCE_DIRS})
    build_assets(${DIR} ASSETS_TO_BUILD)
endforeach()

# Append each file name to the file
foreach(SOURCE_FILE ${ASSETS_TO_BUILD})
file(APPEND ${ASSETS_OUTPUT_FILE} "${SOURCE_FILE}\n")
endforeach()

add_custom_target(
    BuildAssetsTarget
    ALL    
    DEPENDS ${ASSETS_TO_COPY} ${ASSETS_TO_BUILD} CompileShadersTarget
)

# add_custom_command(
#     OUTPUT ${ASSETS_OUTPUT_FILE}                                        # just marks that this finished
#     DEPENDS BuildAssetsTarget
#     COMMENT "Building assets"
# )

# TODO add this to VulkSamples
add_dependencies(VulkSamples BuildAssetsTarget)

##
# build the pipelines
##
# Set the source and destination directories
set(PIPELINES_SOURCE_DIR "${CMAKE_SOURCE_DIR}/Assets/Pipelines")
set(PIPELINES_DEST_DIR "${BUILT_RESOURCES_TARGET_DIR}/Assets/Pipelines") 
# Create the destination directory if it doesn't exist
file(MAKE_DIRECTORY ${PIPELINES_DEST_DIR})
file(GLOB_RECURSE PIPELINE_SRC_FILES CONFIGURE_DEPENDS "${PIPELINES_SOURCE_DIR}/*.pipeline")
# Create a custom command for each pipeline file

# removed BuildTool dependency
foreach(FILE ${PIPELINE_SRC_FILES})
    get_filename_component(FILE_NAME ${FILE} NAME)
    set(ASSETS_OUTPUT_FILE "${PIPELINES_DEST_DIR}/${FILE_NAME}.bin")
    add_custom_command(
        OUTPUT ${ASSETS_OUTPUT_FILE}
        COMMAND $<TARGET_FILE:BuildTool> pipeline ${SHADER_DEST_DIR} ${ASSETS_OUTPUT_FILE} ${FILE}
        DEPENDS ${COMPILED_SHADERS} ${SHADER_GENERATED_HEADER_FILE} CompileShadersTarget
        MAIN_DEPENDENCY ${FILE}
        COMMENT "building pipeline: $<TARGET_FILE:BuildTool> pipeline ${SHADER_DEST_DIR} ${ASSETS_OUTPUT_FILE} ${FILE}"
    )
    list(APPEND PIPELINEBUILDER_ASSETS_OUTPUT_FILES ${ASSETS_OUTPUT_FILE})
endforeach()
# Create a custom target that depends on the output of the previous commands
add_custom_target(
    BuildPipelines ALL
    DEPENDS ${PIPELINEBUILDER_ASSETS_OUTPUT_FILES}
)

# Append each file name to the file
foreach(SOURCE_FILE ${PIPELINEBUILDER_ASSETS_OUTPUT_FILES})
    file(APPEND ${ASSETS_OUTPUT_FILE} "${SOURCE_FILE}\n")
endforeach()