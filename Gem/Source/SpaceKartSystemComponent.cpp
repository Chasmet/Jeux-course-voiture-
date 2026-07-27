#include "SpaceKartSystemComponent.h"

#include <SpaceKart/SpaceKartTypeIds.h>

#include <AzCore/Math/MathUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/string/conversions.h>
#include <AzFramework/Input/Channels/InputChannel.h>
#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Touch/InputDeviceTouch.h>
#include <DebugDraw/DebugDrawBus.h>

#include <cmath>

namespace SpaceKart
{
    AZ_COMPONENT_IMPL(SpaceKartSystemComponent, "SpaceKartSystemComponent", SpaceKartSystemComponentTypeId);

    namespace
    {
        constexpr float TwoPi = 6.28318530718f;
        constexpr float TrackLength = 900.0f;
        constexpr float LaneWidth = 2.2f;
        constexpr int TrackSegments = 96;

        float Wrap01(float value)
        {
            value = std::fmod(value, 1.0f);
            return value < 0.0f ? value + 1.0f : value;
        }
    }

    void SpaceKartSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SpaceKartSystemComponent, AZ::Component>()->Version(1);
        }
    }

    void SpaceKartSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("SpaceKartService"));
    }

    void SpaceKartSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("SpaceKartService"));
    }

    void SpaceKartSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType&) {}
    void SpaceKartSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType&) {}

    void SpaceKartSystemComponent::Init()
    {
        m_racers = {{
            {"Cheikh", AZ::Color(0.15f, 0.65f, 1.0f, 1.0f), 0.0f, 0.0f, -0.7f, 21.0f, 1},
            {"Yvane", AZ::Color(1.0f, 0.72f, 0.05f, 1.0f), 0.0f, 0.0f, -0.2f, 20.2f, 2},
            {"Nelvin", AZ::Color(0.32f, 1.0f, 0.38f, 1.0f), 0.0f, 0.0f, 0.3f, 20.6f, 3},
            {"Nova-7", AZ::Color(0.9f, 0.22f, 1.0f, 1.0f), 0.0f, 0.0f, 0.8f, 20.9f, 4}
        }};
    }

    void SpaceKartSystemComponent::Activate()
    {
        ResetRace();
        InputChannelEventListener::Connect();
        AZ::TickBus::Handler::BusConnect();
    }

    void SpaceKartSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        InputChannelEventListener::Disconnect();
    }

    void SpaceKartSystemComponent::ResetRace()
    {
        for (size_t i = 0; i < m_racers.size(); ++i)
        {
            m_racers[i].m_totalProgress = -static_cast<float>(i) * 0.012f;
            m_racers[i].m_speed = 0.0f;
            m_racers[i].m_rank = static_cast<int>(i) + 1;
        }
        m_steering = 0.0f;
        m_boostPressed = false;
        m_raceFinished = false;
        m_finishTimer = 0.0f;
    }

    void SpaceKartSystemComponent::OnTick(float deltaTime, AZ::ScriptTimePoint)
    {
        UpdateRace(AZ::GetClamp(deltaTime, 0.0f, 0.05f));
        DrawPrototype();
    }

    void SpaceKartSystemComponent::UpdateRace(float deltaTime)
    {
        if (m_raceFinished)
        {
            m_finishTimer += deltaTime;
            if (m_finishTimer >= 4.0f)
            {
                m_circuitIndex = (m_circuitIndex + 1) % 5;
                ResetRace();
            }
            return;
        }

        Racer& player = m_racers[0];
        const float playerTargetSpeed = player.m_baseSpeed + (m_boostPressed ? 8.0f : 0.0f);
        player.m_speed += (playerTargetSpeed - player.m_speed) * AZ::GetClamp(deltaTime * 3.2f, 0.0f, 1.0f);
        player.m_lane = AZ::GetClamp(player.m_lane + m_steering * deltaTime * 1.45f, -1.0f, 1.0f);
        player.m_totalProgress += (player.m_speed / TrackLength) * deltaTime;

        for (size_t i = 1; i < m_racers.size(); ++i)
        {
            Racer& ai = m_racers[i];
            const float pulse = std::sin((ai.m_totalProgress * 19.0f) + static_cast<float>(i) * 1.7f);
            const float targetSpeed = ai.m_baseSpeed + pulse * 1.2f;
            ai.m_speed += (targetSpeed - ai.m_speed) * AZ::GetClamp(deltaTime * 2.0f, 0.0f, 1.0f);
            ai.m_lane = AZ::GetClamp(std::sin(ai.m_totalProgress * 13.0f + static_cast<float>(i)) * 0.75f, -1.0f, 1.0f);
            ai.m_totalProgress += (ai.m_speed / TrackLength) * deltaTime;
        }

        UpdateRanks();
        for (const Racer& racer : m_racers)
        {
            if (racer.m_totalProgress >= static_cast<float>(m_lapCount))
            {
                m_raceFinished = true;
                break;
            }
        }
    }

    void SpaceKartSystemComponent::UpdateRanks()
    {
        for (Racer& racer : m_racers)
        {
            racer.m_rank = 1;
            for (const Racer& other : m_racers)
            {
                if (other.m_totalProgress > racer.m_totalProgress)
                {
                    ++racer.m_rank;
                }
            }
        }
    }

    AZ::Vector3 SpaceKartSystemComponent::EvaluateTrack(float p, int circuitIndex) const
    {
        const float t = Wrap01(p) * TwoPi;
        switch (circuitIndex)
        {
        case 0:
            return AZ::Vector3(std::cos(t) * 8.0f, std::sin(t) * 5.2f, 0.55f + std::sin(t * 2.0f) * 0.45f);
        case 1:
            return AZ::Vector3(std::cos(t) * (7.2f + std::sin(t * 3.0f) * 1.2f), std::sin(t) * 7.2f, 0.8f + std::sin(t * 4.0f) * 0.8f);
        case 2:
            return AZ::Vector3(std::sin(t) * 8.0f, std::sin(t * 2.0f) * 4.0f, 0.7f + std::cos(t * 3.0f) * 0.55f);
        case 3:
            return AZ::Vector3(std::cos(t) * (6.0f + std::cos(t * 5.0f) * 1.4f), std::sin(t) * (6.0f + std::cos(t * 5.0f) * 1.4f), 1.2f + std::sin(t * 3.0f) * 1.1f);
        default:
            return AZ::Vector3(std::cos(t) * (8.5f - t * 0.22f), std::sin(t) * (8.5f - t * 0.22f), 0.8f + t * 0.16f);
        }
    }

    AZ::Vector3 SpaceKartSystemComponent::EvaluateTrackTangent(float p, int circuitIndex) const
    {
        const AZ::Vector3 current = EvaluateTrack(p, circuitIndex);
        const AZ::Vector3 next = EvaluateTrack(p + 0.002f, circuitIndex);
        return (next - current).GetNormalizedSafe(AZ::Vector3::CreateAxisY());
    }

    void SpaceKartSystemComponent::DrawPrototype() const
    {
        const AZ::Color trackColor(0.1f, 0.85f, 1.0f, 1.0f);
        for (int i = 0; i < TrackSegments; ++i)
        {
            const float p0 = static_cast<float>(i) / static_cast<float>(TrackSegments);
            const float p1 = static_cast<float>(i + 1) / static_cast<float>(TrackSegments);
            const AZ::Vector3 a = EvaluateTrack(p0, m_circuitIndex);
            const AZ::Vector3 b = EvaluateTrack(p1, m_circuitIndex);
            DebugDraw::DebugDrawRequestBus::Broadcast(&DebugDraw::DebugDrawRequests::DrawLineLocationToLocation, a, b, trackColor, 0.0f);
        }

        for (const Racer& racer : m_racers)
        {
            const float p = Wrap01(racer.m_totalProgress);
            const AZ::Vector3 center = EvaluateTrack(p, m_circuitIndex);
            const AZ::Vector3 tangent = EvaluateTrackTangent(p, m_circuitIndex);
            const AZ::Vector3 lateral(-tangent.GetY(), tangent.GetX(), 0.0f);
            const AZ::Vector3 position = center + lateral.GetNormalizedSafe(AZ::Vector3::CreateAxisX()) * (racer.m_lane * LaneWidth);
            DebugDraw::DebugDrawRequestBus::Broadcast(&DebugDraw::DebugDrawRequests::DrawSphereAtLocation, position, 0.38f, racer.m_color, 0.0f);
            DebugDraw::DebugDrawRequestBus::Broadcast(&DebugDraw::DebugDrawRequests::DrawLineLocationToLocation, position, position + tangent * 0.9f, racer.m_color, 0.0f);
        }

        const Racer& player = m_racers[0];
        const int lap = AZ::GetClamp(static_cast<int>(player.m_totalProgress) + 1, 1, m_lapCount);
        AZStd::string hud = AZStd::string::format("CHK SPACE KART | %s | Tour %d/%d | Position %d/4 | %.0f km/h", GetCircuitName().c_str(), lap, m_lapCount, player.m_rank, player.m_speed * 7.2f);
        DebugDraw::DebugDrawRequestBus::Broadcast(&DebugDraw::DebugDrawRequests::DrawTextOnScreen, hud, AZ::Color::CreateOne(), 0.0f);

        if (m_raceFinished)
        {
            const AZStd::string result = player.m_rank == 1 ? "VICTOIRE ! Circuit suivant..." : "Course terminée — circuit suivant...";
            DebugDraw::DebugDrawRequestBus::Broadcast(&DebugDraw::DebugDrawRequests::DrawScaledTextOnScreen, result, 1.5f, AZ::Color(1.0f, 0.75f, 0.1f, 1.0f), 0.0f);
        }
    }

    AZStd::string SpaceKartSystemComponent::GetCircuitName() const
    {
        static const AZStd::array<const char*, 5> names = {{
            "Orbite de Mars", "Anneaux de Saturne", "Éclipse lunaire", "Faille de la Nébuleuse", "Singularité finale"
        }};
        return names[static_cast<size_t>(m_circuitIndex)];
    }

    bool SpaceKartSystemComponent::OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel)
    {
        using namespace AzFramework;
        const InputChannelId& id = inputChannel.GetInputChannelId();
        const InputDeviceId& device = inputChannel.GetInputDevice().GetInputDeviceId();

        if (InputDeviceKeyboard::IsKeyboardDevice(device))
        {
            if (id == InputDeviceKeyboard::Key::NavigationArrowLeft)
            {
                m_steering = inputChannel.IsActive() ? -1.0f : 0.0f;
            }
            else if (id == InputDeviceKeyboard::Key::NavigationArrowRight)
            {
                m_steering = inputChannel.IsActive() ? 1.0f : 0.0f;
            }
            else if (id == InputDeviceKeyboard::Key::EditSpace)
            {
                m_boostPressed = inputChannel.IsActive();
            }
        }
        else if (InputDeviceGamepad::IsGamepadDevice(device))
        {
            if (id == InputDeviceGamepad::ThumbStickAxis1D::LX)
            {
                m_steering = inputChannel.GetValue();
            }
            else if (id == InputDeviceGamepad::Button::A)
            {
                m_boostPressed = inputChannel.IsActive();
            }
        }
        else if (InputDeviceTouch::IsTouchDevice(device))
        {
            if (const auto* position = inputChannel.GetCustomData<InputChannel::PositionData2D>())
            {
                const float x = position->m_normalizedPosition.GetX();
                if (x < 0.5f)
                {
                    m_steering = inputChannel.IsActive() ? AZ::GetClamp((x - 0.25f) * 4.0f, -1.0f, 1.0f) : 0.0f;
                }
                else
                {
                    m_boostPressed = inputChannel.IsActive();
                }
            }
        }
        return false;
    }
}
