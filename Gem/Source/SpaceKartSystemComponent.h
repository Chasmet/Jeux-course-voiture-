#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/string/string.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>

namespace SpaceKart
{
    class SpaceKartSystemComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
        , public AzFramework::InputChannelEventListener
    {
    public:
        AZ_COMPONENT_DECL(SpaceKartSystemComponent);

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

    private:
        struct Racer
        {
            AZStd::string m_name;
            AZ::Color m_color;
            float m_totalProgress = 0.0f;
            float m_speed = 0.0f;
            float m_lane = 0.0f;
            float m_baseSpeed = 0.0f;
            int m_rank = 1;
        };

        void ResetRace();
        void UpdateRace(float deltaTime);
        void UpdateRanks();
        void DrawPrototype() const;
        AZ::Vector3 EvaluateTrack(float normalizedProgress, int circuitIndex) const;
        AZ::Vector3 EvaluateTrackTangent(float normalizedProgress, int circuitIndex) const;
        AZStd::string GetCircuitName() const;

        AZStd::array<Racer, 4> m_racers;
        int m_circuitIndex = 0;
        int m_lapCount = 3;
        float m_steering = 0.0f;
        bool m_boostPressed = false;
        bool m_raceFinished = false;
        float m_finishTimer = 0.0f;
    };
}
