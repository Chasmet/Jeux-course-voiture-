if(NOT PROJECT_NAME)
    cmake_minimum_required(VERSION 3.24)
    include(cmake/EngineFinder.cmake OPTIONAL)
    find_package(o3de REQUIRED)
    project(SpaceKartLegends
        LANGUAGES C CXX
        VERSION 0.1.0.0
    )
    o3de_initialize()
endif()
