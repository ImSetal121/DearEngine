find_program(GLSLC_PROG glslc)
find_program(SPIRV_CROSS_PROG spirv-cross)

macro(compile_shader_to_spv shader_name output_name)
    if (GLSLC_PROG)
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${output_name}
            COMMAND ${GLSLC_PROG}  ${CMAKE_CURRENT_SOURCE_DIR}/${shader_name} -o ${CMAKE_CURRENT_SOURCE_DIR}/${output_name}
            COMMENT "compiling shader ${CMAKE_CURRENT_SOURCE_DIR}/${shader_name} -> ${output_name}"
            MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/${shader_name}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            VERBATIM
        )
    else()
        message(WARN "don't find glslc, can't compiling shader ${CMAKE_CURRENT_SOURCE_DIR}/${shader_name}")
    endif()
endmacro()

macro(compile_shader_to_msl spv_name msl_name)
    if (SPIRV_CROSS_PROG)
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${msl_name}
            COMMAND ${SPIRV_CROSS_PROG} ${CMAKE_CURRENT_SOURCE_DIR}/${spv_name} --msl --output ${CMAKE_CURRENT_SOURCE_DIR}/${msl_name}
            COMMENT "spirv-cross ${spv_name} -> ${msl_name} (MSL)"
            MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/${spv_name}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            VERBATIM
        )
    else()
        message(WARN "spirv-cross not found, skipping MSL generation for ${spv_name}")
    endif()
endmacro()