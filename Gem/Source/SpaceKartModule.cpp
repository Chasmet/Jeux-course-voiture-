#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include "SpaceKartSystemComponent.h"
#include <SpaceKart/SpaceKartTypeIds.h>

namespace SpaceKart
{
    class SpaceKartModule : public AZ::Module
    {
    public:
        AZ_RTTI(SpaceKartModule, SpaceKartModuleTypeId, AZ::Module);
        AZ_CLASS_ALLOCATOR(SpaceKartModule, AZ::SystemAllocator);

        SpaceKartModule()
        {
            m_descriptors.insert(m_descriptors.end(), {
                SpaceKartSystemComponent::CreateDescriptor()
            });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return { azrtti_typeid<SpaceKartSystemComponent>() };
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), SpaceKart::SpaceKartModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_SpaceKart, SpaceKart::SpaceKartModule)
#endif
