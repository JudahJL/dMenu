cmake_minimum_required(VERSION 3.29)
message("Using toolchain file ${CMAKE_TOOLCHAIN_FILE}.")
########################################################################################################################
## Define project
########################################################################################################################
project(
	dmenu
	VERSION 1.0.0.0
	DESCRIPTION ""
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)
set(CMAKE_OPTIMIZE_DEPENDENCIES ON)

include(GNUInstallDirs)

if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
	message(FATAL_ERROR "in-source builds are not allowed")
endif()

configure_file(
		${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.rc.in
		${CMAKE_CURRENT_BINARY_DIR}/version.rc
		@ONLY)

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
	cmake_policy(SET CMP0135 NEW)
endif()

if (POLICY CMP0141)
	cmake_policy(SET CMP0141 NEW)
	set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# Add headers
file(
		GLOB_RECURSE
		headers
		${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
)
list(REMOVE_ITEM headers ${CMAKE_CURRENT_SOURCE_DIR}/include/PCH.h)

# Add sources
file(
		GLOB_RECURSE
		sources
		${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc
		${CMAKE_CURRENT_BINARY_DIR}/version.rc
)

set(tests )

set(misc
		${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
		${CMAKE_CURRENT_SOURCE_DIR}/CMakePresets.json
		${CMAKE_CURRENT_SOURCE_DIR}/vcpkg-configuration.json
		${CMAKE_CURRENT_SOURCE_DIR}/vcpkg.json
		${CMAKE_CURRENT_SOURCE_DIR}/.clang-format
)

source_group(
		TREE ${CMAKE_CURRENT_SOURCE_DIR}
		FILES
		${headers}
		${sources}
		${tests}
		${misc})

#########################################################################################################################
### Build options
#########################################################################################################################
message("Options:")
option(BUILD_TESTS "Build unit tests." OFF)
message("\tTests: ${BUILD_TESTS}")
set(SKSE_SUPPORT_XBYAK ON)

########################################################################################################################
## Configure target DLL
########################################################################################################################
set(PROJECT_AUTHOR "dTry")

if(DEFINED ENV{GITHUB_ENV})
	# If running in a GitHub Actions environment, always use the submodule directory.
	set(CommonLibSSE "${CMAKE_CURRENT_SOURCE_DIR}/extern/CommonLibSSE-NG/")
	message(STATUS "GitHub Actions detected. Using CommonLibSSE submodule directory.")
elseif(DEFINED ENV{CommonLibSSEPath})
	# If CommonLibSSEPath is set, use the provided environment variable.
	set(CommonLibSSE $ENV{CommonLibSSEPath})
	message(STATUS "CommonLibSSEPath environment variable found. Using: ${CommonLibSSE}")
else()
	# If not in GitHub Actions and CommonLibSSEPath is not set, fallback to the submodule directory.
	set(CommonLibSSE "${CMAKE_CURRENT_SOURCE_DIR}/extern/CommonLibSSE-NG/")
	message(WARNING "CommonLibSSEPath not defined. Using submodule directory: ${CommonLibSSE}")
	message(WARNING "Ensure that the CommonLibSSE-NG submodule is initialized.")
endif()

if(NOT DEFINED CommonLibSSE)
	message(FATAL_ERROR "Variable CommonLibSSE is not Defined.Make sure to define it")
endif()

message(STATUS "CommonLibSSE Path: ${CommonLibSSE}")

add_subdirectory(${CommonLibSSE} CommonLibSSE-NG)
include(${CommonLibSSE}/cmake/CommonLibSSE.cmake)

find_package(nlohmann_json CONFIG REQUIRED)

find_package(xbyak REQUIRED CONFIG)

find_package(imgui REQUIRED)

add_commonlibsse_plugin(${PROJECT_NAME} SOURCES ${headers} ${sources} ${misc} AUTHOR ${PROJECT_AUTHOR})
add_library("${PROJECT_NAME}::${PROJECT_NAME}" ALIAS "${PROJECT_NAME}")

target_include_directories(${PROJECT_NAME}
		PRIVATE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src>
		$<INSTALL_INTERFACE:src>
		nlohmann_json::nlohmann_json
		imgui::imgui
		xbyak::xbyak)

target_include_directories(${PROJECT_NAME}
		PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(
		${PROJECT_NAME}
		PRIVATE
		nlohmann_json::nlohmann_json
		imgui::imgui
		xbyak::xbyak)

target_precompile_headers(${PROJECT_NAME}
		PRIVATE
		include/PCH.h)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include"
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(TARGETS ${PROJECT_NAME}
		DESTINATION "${CMAKE_INSTALL_LIBDIR}")

########################################################################################################################
## Automatic plugin deployment
########################################################################################################################
# Automatic deployment to Mod Organizer 2 mod directory.
foreach(DEPLOY_TARGET $ENV{SkyrimPluginTargets})
	set(DEPLOY_TARGET_COMMON_DIR "${DEPLOY_TARGET}/${PROJECT_NAME} - ${CMAKE_BUILD_TYPE}/SKSE/Plugins/")
	message("Adding deployment target ${DEPLOY_TARGET}.")

	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory "${DEPLOY_TARGET_COMMON_DIR}"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${DEPLOY_TARGET_COMMON_DIR}")

	if(${CMAKE_BUILD_TYPE} STREQUAL Debug OR ${CMAKE_BUILD_TYPE} STREQUAL RelWithDebInfo)
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_PDB_FILE:${PROJECT_NAME}>" "${DEPLOY_TARGET_COMMON_DIR}")
	endif()
endforeach()
