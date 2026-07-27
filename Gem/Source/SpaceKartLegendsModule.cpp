#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include "SpaceKartLegendsSystemComponent.h"

namespace SpaceKartLegends
{
    class SpaceKartLegendsModule final
        : public AZ::Module
    {
    public:
        AZ_RTTI(SpaceKartLegendsModule, "{8F79EE37-0AE8-4B03-95DF-EAC3CC021D8E}", AZ::Module);
        AZ_CLASS_ALLOCATOR(SpaceKartLegendsModule, AZ::SystemAllocator);

        SpaceKartLegendsModule()
        {
            m_descriptors.insert(
                m_descriptors.end(),
                {
                    SpaceKartLegendsSystemComponent::CreateDescriptor()
                });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<SpaceKartLegendsSystemComponent>()
            };
        }
    };
}

AZ_DECLARE_MODULE_CLASS(Gem_SpaceKartLegends, SpaceKartLegends::SpaceKartLegendsModule)
