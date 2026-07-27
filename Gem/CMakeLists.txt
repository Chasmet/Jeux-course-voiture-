o3de_gem_setup("SpaceKartLegends")

ly_add_target(
    NAME ${gem_name}.Private.Object STATIC
    NAMESPACE Gem
    FILES_CMAKE
        spacekartlegends_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            AZ::AzGameFramework
            Gem::Atom_AtomBridge.Static
            Gem::Camera.Static
)

ly_add_target(
    NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        spacekartlegends_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::${gem_name}.Private.Object
            AZ::AzCore
)

ly_add_source_properties(
    SOURCES Source/SpaceKartLegendsModule.cpp
    PROPERTY COMPILE_DEFINITIONS
    VALUES O3DE_GEM_NAME=${gem_name} O3DE_GEM_VERSION=${gem_version}
)

ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Tools NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Clients NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Servers NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Unified NAMESPACE Gem TARGETS Gem::${gem_name})

o3de_add_variant_dependencies_for_gem_dependencies(
    GEM_NAME ${gem_name}
    VARIANTS Builders Tools Clients Servers Unified
)
