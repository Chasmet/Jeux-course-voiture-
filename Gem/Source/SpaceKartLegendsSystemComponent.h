#pragma once

#include "SpaceKartRace.h"

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>

namespace SpaceKartLegends
{
    class SpaceKartLegendsSystemComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
        , public AzFramework::InputChannelEventListener
        , public AzFramework::ViewportDebugDisplayEventBus::Handler
    {
    public:
        AZ_COMPONENT(SpaceKartLegendsSystemComponent, "{66B15AD0-339C-4C72-940B-6E111572D932}");

        SpaceKartLegendsSystemComponent();
        ~SpaceKartLegendsSystemComponent() override = default;

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        void Init() override;
        void Activate() override;
        void Deactivate() override;

        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        bool OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel) override;

        void DisplayViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        void DisplayViewport2d(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

    private:
        void UpdateCamera();
        void ApplyDigitalSteering();

        SpaceKartRace m_race;
        bool m_leftPressed = false;
        bool m_rightPressed = false;
        bool m_driftPressed = false;
        float m_gamepadSteering = 0.0f;
        AZ::EntityId m_activeCamera;
    };
}
