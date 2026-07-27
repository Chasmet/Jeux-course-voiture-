o3de_gem_setup("SpaceKart")

ly_add_target(
    NAME SpaceKart.Private.Object STATIC
    NAMESPACE Gem
    FILES_CMAKE spacekart_files.cmake
    INCLUDE_DIRECTORIES PUBLIC Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            Gem::DebugDraw
)

ly_add_target(
    NAME SpaceKart ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE spacekart_shared_files.cmake
    INCLUDE_DIRECTORIES PUBLIC Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::SpaceKart.Private.Object
            AZ::AzCore
)

ly_add_source_properties(
    SOURCES Source/SpaceKartModule.cpp
    PROPERTY COMPILE_DEFINITIONS
    VALUES O3DE_GEM_NAME=SpaceKart O3DE_GEM_VERSION=0.1.0
)

ly_create_alias(NAME SpaceKart.Clients NAMESPACE Gem TARGETS Gem::SpaceKart)
ly_create_alias(NAME SpaceKart.Tools NAMESPACE Gem TARGETS Gem::SpaceKart)
ly_create_alias(NAME SpaceKart.Unified NAMESPACE Gem TARGETS Gem::SpaceKart)
