if(NOT PROJECT_NAME)
    cmake_minimum_required(VERSION 3.24)

    if(DEFINED ENV{O3DE_ENGINE_PATH})
        list(APPEND CMAKE_MODULE_PATH "$ENV{O3DE_ENGINE_PATH}/cmake")
    endif()

    find_package(o3de REQUIRED)
    project(SpaceKart LANGUAGES C CXX VERSION 0.1.0.0)
    o3de_initialize()
endif()
