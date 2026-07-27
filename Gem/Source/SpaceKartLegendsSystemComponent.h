#pragma once

#include "SpaceKartRace.h"

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/containers/array.h>
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
        enum class TouchRole : AZ::u8
        {
            None,
            Steering,
            Drift,
            Brake
        };

        enum class FrontendState : AZ::u8
        {
            PilotSelection,
            Racing,
            Results,
            ChampionshipComplete
        };

        void EnsureActiveCamera();
        void DestroyOwnedCamera();
        void UpdateCamera();
        void ApplyDigitalSteering();
        bool HandleTouchInput(const AzFramework::InputChannel& inputChannel);
        void ReleaseTouchRole(size_t touchIndex);

        void ResetInputState();
        void NavigateFrontend(int direction);
        void ConfirmFrontend();
        void FinishCurrentRace();
        void RestartCurrentRace();
        void DrawFrontendHud(AzFramework::DebugDisplayRequests& debugDisplay) const;
        static int PointsForPlace(int place);

        SpaceKartRace m_race;
        FrontendState m_frontendState = FrontendState::PilotSelection;
        int m_currentCircuit = 0;
        int m_championshipPoints = 0;
        int m_lastRacePoints = 0;
        bool m_resultHandled = false;

        bool m_leftPressed = false;
        bool m_rightPressed = false;
        bool m_acceleratePressed = true;
        bool m_brakePressed = false;
        bool m_driftPressed = false;
        float m_gamepadSteering = 0.0f;
        float m_touchSteering = 0.0f;
        int m_activeDriftTouches = 0;
        int m_activeBrakeTouches = 0;
        AZStd::array<TouchRole, 10> m_touchRoles{};
        AZ::EntityId m_activeCamera;
        bool m_ownsActiveCamera = false;
    };
}
