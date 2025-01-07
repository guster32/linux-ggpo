include(CMakeSources.cmake)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED gtk+-3.0)



add_executable(VectorWar
${GGPO_EXAMPLES_VECTORWAR_SRC}
)

add_dependencies(VectorWar GGPO)
target_include_directories(VectorWar PRIVATE ${GTK_INCLUDE_DIRS})
target_link_libraries(VectorWar LINK_PUBLIC GGPO ${GTK_LIBRARIES})
target_compile_options(VectorWar PRIVATE ${GTK_CFLAGS_OTHER})

# set_target_properties(GGPO PROPERTIES VERSION ${PROJECT_VERSION})
