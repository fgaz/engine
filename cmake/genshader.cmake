function(read_shader_includes shaderpath include_dirs includes_list_out)
	file(READ "${shaderpath}" contents)
	string(REGEX REPLACE ";" "\\\\;" contents "${contents}")
	string(REGEX REPLACE "\r" "" contents "${contents}")
	string(REGEX REPLACE "\n" ";" contents "${contents}")
	set(_local_list "${${includes_list_out}}")
	foreach (line ${contents})
		string(FIND "${line}" "#include" out)
		if("${out}" EQUAL 0)
			string(SUBSTRING ${line} 8 -1 include_file)
			string(REPLACE " " "" include_file ${include_file})
			string(REPLACE "\"" "" include_file ${include_file})
			foreach (dir ${include_dirs})
				if (EXISTS "${dir}/${include_file}")
					list(APPEND _local_list "${dir}/${include_file}")
					break()
				endif()
			endforeach()
		endif()
	endforeach()
	list(REMOVE_DUPLICATES _local_list)
	set(${includes_list_out} ${_local_list} PARENT_SCOPE)
endfunction()

macro(shader_include_dir TARGET DEPENDENCY)
	set(GEN_DIR ${GENERATE_DIR}/shaders/${DEPENDENCY}/)
	target_include_directories(${TARGET} PUBLIC ${GEN_DIR})
endmacro()

macro(generate_shaders TARGET)
	set(files ${ARGV})
	list(REMOVE_AT files 0)
	set(GEN_DIR ${GENERATE_DIR}/shaders/${TARGET}/)
	set(_template_header ${ROOT_DIR}/src/tools/shadertool/ShaderTemplate.h.in)
	set(_template_cpp ${ROOT_DIR}/src/tools/shadertool/ShaderTemplate.cpp.in)
	set(_template_constants_header ${ROOT_DIR}/src/tools/shadertool/ShaderConstantsTemplate.h.in)
	set(_template_ub ${ROOT_DIR}/src/tools/shadertool/UniformBufferTemplate.h.in)
	file(MAKE_DIRECTORY ${GEN_DIR})
	target_include_directories(${TARGET} PUBLIC ${GEN_DIR})
	set(_headers)
	set(_sources)
	set(_constantsheaders)
	add_custom_target(UpdateShaders${TARGET})
	file(WRITE ${CMAKE_BINARY_DIR}/UpdateShaderFile${TARGET}.cmake "configure_file(\${SRC} \${DST} @ONLY)")
	if (NOT DEFINED video_SOURCE_DIR)
		message(FATAL_ERROR "video project not found")
	endif()
	set(SHADERTOOL_INCLUDE_DIRS)
	set(_dir ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
	list(APPEND SHADERTOOL_INCLUDE_DIRS "${_dir}")
	set(DEPENDENCIES)
	engine_resolve_dependencies(${TARGET} DEPENDENCIES)
	foreach (D ${DEPENDENCIES})
		if (EXISTS ${${D}_SOURCE_DIR}/shaders)
			list(APPEND SHADERTOOL_INCLUDE_DIRS "${${D}_SOURCE_DIR}/shaders")
		endif()
	endforeach()
	list(REMOVE_DUPLICATES SHADERTOOL_INCLUDE_DIRS)
	set(SHADERTOOL_INCLUDE_DIRS_PARAM)
	foreach (IDIR ${SHADERTOOL_INCLUDE_DIRS})
		list(APPEND SHADERTOOL_INCLUDE_DIRS_PARAM "-I")
		list(APPEND SHADERTOOL_INCLUDE_DIRS_PARAM "${IDIR}")
	endforeach()
	foreach (_file ${files})
		set(_shaders)
		if (EXISTS ${_dir}/${_file}.frag AND EXISTS ${_dir}/${_file}.vert)
			list(APPEND _shaders ${_dir}/${_file}.frag ${_dir}/${_file}.vert)
			if (EXISTS ${_dir}/${_file}.geom)
				list(APPEND _shaders ${_dir}/${_file}.geom)
			endif()
		endif()
		if (EXISTS ${_dir}/${_file}.comp)
			list(APPEND _shaders ${_dir}/${_file}.comp)
		endif()
		if (_shaders)
			set(_shadersdeps)
			foreach (s ${_shaders})
				read_shader_includes(${s} "${SHADERTOOL_INCLUDE_DIRS}" _shadersdeps)
			endforeach()
			convert_to_camel_case(${_file} _f)
			set(_shaderheaderpath "${GEN_DIR}${_f}Shader.h")
			set(_shadersourcepath "${GEN_DIR}${_f}Shader.cpp")
			set(_shaderconstantheaderpath "${GEN_DIR}${_f}ShaderConstants.h")
			# TODO We have to add the shader/ dirs of all dependencies to the include path
			if (CMAKE_CROSS_COMPILING)
				message(STATUS "Looking for native tool in ${NATIVE_BUILD_DIR}")
				find_program(SHADERTOOL_EXECUTABLE NAMES vengi-shadertool PATHS ${NATIVE_BUILD_DIR}/shadertool)
				find_program(GLSLANGVALIDATOR_EXECUTABLE NAMES glslangValidator PATHS ${NATIVE_BUILD_DIR})
				add_custom_command(
					OUTPUT ${_shaderheaderpath}.in ${_shadersourcepath}.in ${_shaderconstantheaderpath}.in
					IMPLICIT_DEPENDS C ${_shaders}
					COMMENT "Validate ${_file}"
					COMMAND ${CMAKE_COMMAND} -E env "APP_HOMEPATH=${CMAKE_CURRENT_BINARY_DIR}/" ${SHADERTOOL_EXECUTABLE} --glslang ${GLSLANGVALIDATOR_EXECUTABLE} ${SHADERTOOL_INCLUDE_DIRS_PARAM} --postfix .in --shader ${_dir}/${_file} --constantstemplate  ${_template_constants_header} --headertemplate ${_template_header} --sourcetemplate ${_template_cpp} --buffertemplate ${_template_ub} --sourcedir ${GEN_DIR}
					DEPENDS ${_shaders} ${_shadersdeps} ${_template_header} ${_template_cpp} ${_template_ub} ${_template_constants_header}
				)
			else()
				add_custom_command(
					OUTPUT ${_shaderheaderpath}.in ${_shadersourcepath}.in ${_shaderconstantheaderpath}.in
					IMPLICIT_DEPENDS C ${_shaders}
					COMMENT "Validate ${_file}"
					COMMAND ${CMAKE_COMMAND} -E env "APP_HOMEPATH=${CMAKE_CURRENT_BINARY_DIR}/" $<TARGET_FILE:shadertool> --glslang $<TARGET_FILE:glslangValidator> ${SHADERTOOL_INCLUDE_DIRS_PARAM} --postfix .in --shader ${_dir}/${_file} --constantstemplate  ${_template_constants_header} --headertemplate ${_template_header} --sourcetemplate ${_template_cpp} --buffertemplate ${_template_ub} --sourcedir ${GEN_DIR}
					DEPENDS shadertool ${_shaders} ${_shadersdeps} ${_template_header} ${_template_cpp} ${_template_ub} ${_template_constants_header}
				)
			endif()
			list(APPEND _headers ${_shaderheaderpath})
			list(APPEND _sources ${_shadersourcepath})
			add_custom_command(
				OUTPUT ${_shaderheaderpath}
				COMMAND ${CMAKE_COMMAND} -D SRC=${_shaderheaderpath}.in -D DST=${_shaderheaderpath} -P ${CMAKE_BINARY_DIR}/UpdateShaderFile${TARGET}.cmake
				DEPENDS ${_shaderheaderpath}.in
			)
			add_custom_command(
				OUTPUT ${_shadersourcepath}
				COMMAND ${CMAKE_COMMAND} -D SRC=${_shadersourcepath}.in -D DST=${_shadersourcepath} -P ${CMAKE_BINARY_DIR}/UpdateShaderFile${TARGET}.cmake
				DEPENDS ${_shadersourcepath}.in
			)
			add_custom_command(
				OUTPUT ${_shaderconstantheaderpath}
				COMMAND ${CMAKE_COMMAND} -D SRC=${_shaderconstantheaderpath}.in -D DST=${_shaderconstantheaderpath} -P ${CMAKE_BINARY_DIR}/UpdateShaderFile${TARGET}.cmake
				DEPENDS ${_shaderconstantheaderpath}.in
			)
			list(APPEND _constantsheaders ${_shaderconstantheaderpath})
		else()
			message(FATAL_ERROR "Could not find any shader files for ${_file} and target '${TARGET}'")
		endif()
	endforeach()

	convert_to_camel_case(${TARGET} _filetarget)
	set(_shadersheader ${GEN_DIR}/${_filetarget}Shaders.h)
	file(WRITE ${_shadersheader}.in "#pragma once\n")
	foreach (header_path ${_headers})
		string(REPLACE "${GEN_DIR}" "" header "${header_path}")
		file(APPEND ${_shadersheader}.in "#include \"${header}\"\n")
	endforeach()
	add_custom_target(GenerateShaderBindings${TARGET}
		DEPENDS ${_headers} ${_constantsheaders}
		COMMENT "Generate shader bindings for ${TARGET} in ${GEN_DIR}"
	)
	engine_mark_as_generated(${_headers} ${_shaderconstantheaderpath} ${_sources} ${_shadersheader})
	generate_unity_sources(SOURCES TARGET ${TARGET} SRCS ${_sources} UNITY_SRC "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_shaders_unity.cpp")
	target_sources(${TARGET} PRIVATE ${_headers} ${_constantsheaders} ${_shadersheader})

	add_custom_target(GenerateShaderHeader${TARGET} ${CMAKE_COMMAND} -D SRC=${_shadersheader}.in -D DST=${_shadersheader} -P ${CMAKE_BINARY_DIR}/UpdateShaderFile${TARGET}.cmake)
	add_dependencies(${TARGET} GenerateShaderHeader${TARGET} UpdateShaders${TARGET})
	add_dependencies(GenerateShaderHeader${TARGET} GenerateShaderBindings${TARGET})
	add_dependencies(codegen GenerateShaderHeader${TARGET} UpdateShaders${TARGET})
endmacro()
