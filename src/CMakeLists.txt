include(CMakeSources.cmake)

add_library(GGPO
	${GGPO_LIB_SRC}
)

target_include_directories(GGPO PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/ggpo>
)

target_compile_options(GGPO PRIVATE -fvisibility=default)


set_target_properties(GGPO PROPERTIES VERSION ${PROJECT_VERSION})

