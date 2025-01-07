cmake_minimum_required(VERSION 3.5)

# TODO: SDK Versioning.
project(GGPO VERSION 1.0.0)

# What do we want to build?
option(GGPO_BUILD_SDK "Enable the build of the GGPO SDK" ON)
option(GGPO_BUILD_VECTORWAR "Enable the build of the Vector War example app" ON)
option(BUILD_SHARED_LIBS "Enable the build of shared libraries (.so) instead of static ones (.a)" ON)

if(GGPO_BUILD_SDK)
	add_subdirectory(src)
endif()

if(GGPO_BUILD_VECTORWAR)
	add_subdirectory(src/apps/vectorwar)
endif()
