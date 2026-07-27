#include "SpaceKartLegendsSystemComponent.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>
#include <AzFramework/Components/CameraBus.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>

namespace SpaceKartLegends
{
    SpaceKartLegendsSystemComponent::SpaceKartLegendsSystemComponent()
        : AzFramework::InputChannelEventListener(AzFramework::InputChannelEventListener::GetPriorityDefault())
    {
    }

    void SpaceKartLegendsSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SpaceKartLegendsSystemComponent, AZ::Component>()
                ->Version(2);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SpaceKartLegendsSystemComponent>(
                    "Space Kart Legends",
                    "Systeme de course arcade 3D, IA, circuits et prototype Android.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    void SpaceKartLegendsSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("SpaceKartLegendsService"));
    }

    void SpaceKartLegendsSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("SpaceKartLegendsService"));
    }

    void SpaceKartLegendsSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void SpaceKartLegendsSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void SpaceKartLegendsSystemComponent::Init()
    {
    }

    void SpaceKartLegendsSystemComponent::Activate()
    {
        m_leftPressed = false;
        m_rightPressed = false;
        m_acceleratePressed = true;
        m_brakePressed = false;
        m_driftPressed = false;
        m_gamepadSteering = 0.0f;
        m_race.Reset();

        AzFramework::InputChannelEventListener::Connect();
        AZ::TickBus::Handler::BusConnect();

        AzFramework::EntityContextId gameContextId = AzFramework::EntityContextId::CreateNull();
        AzFramework::GameEntityContextRequestBus::BroadcastResult(
            gameContextId,
            &AzFramework::GameEntityContextRequestBus::Events::GetGameEntityContextId);
        AzFramework::ViewportDebugDisplayEventBus::Handler::BusConnect(gameContextId);
    }

    void SpaceKartLegendsSystemComponent::Deactivate()
    {
        AzFramework::ViewportDebugDisplayEventBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        AzFramework::InputChannelEventListener::Disconnect();
        m_activeCamera = AZ::EntityId();
    }

    void SpaceKartLegendsSystemComponent::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        ApplyDigitalSteering();
        m_race.SetAccelerating(m_acceleratePressed);
        m_race.SetBraking(m_brakePressed);
        m_race.Update(deltaTime);
        UpdateCamera();
    }

    void SpaceKartLegendsSystemComponent::ApplyDigitalSteering()
    {
        float steering = m_gamepadSteering;
        if (m_leftPressed)
        {
            steering -= 1.0f;
        }
        if (m_rightPressed)
        {
            steering += 1.0f;
        }
        m_race.SetSteering(AZStd::max(-1.0f, AZStd::min(1.0f, steering)));
    }

    bool SpaceKartLegendsSystemComponent::OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel)
    {
        const AzFramework::InputChannelId& id = inputChannel.GetInputChannelId();
        const bool active = inputChannel.GetState() != AzFramework::InputChannel::State::Ended;

        if (id == AzFramework::InputDeviceKeyboard::Key::AlphanumericA ||
            id == AzFramework::InputDeviceKeyboard::Key::NavigationArrowLeft)
        {
            m_leftPressed = active;
            return true;
        }

        if (id == AzFramework::InputDeviceKeyboard::Key::AlphanumericD ||
            id == AzFramework::InputDeviceKeyboard::Key::NavigationArrowRight)
        {
            m_rightPressed = active;
            return true;
        }

        if (id == AzFramework::InputDeviceKeyboard::Key::AlphanumericW ||
            id == AzFramework::InputDeviceKeyboard::Key::NavigationArrowUp)
        {
            m_acceleratePressed = active;
            return true;
        }

        if (id == AzFramework::InputDeviceKeyboard::Key::AlphanumericS ||
            id == AzFramework::InputDeviceKeyboard::Key::NavigationArrowDown)
        {
            m_brakePressed = active;
            return true;
        }

        if (id == AzFramework::InputDeviceKeyboard::Key::EditSpace ||
            id == AzFramework::InputDeviceGamepad::Button::A)
        {
            m_driftPressed = active;
            m_race.SetDrifting(active);
            return true;
        }

        if ((id == AzFramework::InputDeviceKeyboard::Key::AlphanumericB ||
             id == AzFramework::InputDeviceGamepad::Button::B) &&
            inputChannel.GetState() == AzFramework::InputChannel::State::Began)
        {
            m_race.UseBoost();
            return true;
        }

        if (id == AzFramework::InputDeviceKeyboard::Key::AlphanumericR &&
            inputChannel.GetState() == AzFramework::InputChannel::State::Began)
        {
            m_race.RecoverPlayer();
            return true;
        }

        if (id == AzFramework::InputDeviceKeyboard::Key::AlphanumericN &&
            inputChannel.GetState() == AzFramework::InputChannel::State::Began)
        {
            m_race.SelectNextCircuit();
            return true;
        }

        if (id == AzFramework::InputDeviceGamepad::ThumbStickAxis1D::LX)
        {
            m_gamepadSteering = inputChannel.GetValue();
            return true;
        }

        return false;
    }

    void SpaceKartLegendsSystemComponent::UpdateCamera()
    {
        if (!m_activeCamera.IsValid())
        {
            Camera::CameraSystemRequestBus::BroadcastResult(
                m_activeCamera,
                &Camera::CameraSystemRequestBus::Events::GetActiveCamera);
        }

        if (!m_activeCamera.IsValid())
        {
            return;
        }

        const AZ::Vector3 playerPosition = m_race.GetPlayerPosition();
        const AZ::Vector3 tangent = m_race.GetPlayerTangent();
        const AZ::Vector3 up = AZ::Vector3::CreateAxisZ();
        const AZ::Vector3 cameraPosition = playerPosition - tangent * 12.5f + up * 6.8f;
        const AZ::Vector3 cameraTarget = playerPosition + tangent * 5.5f + up * 1.1f;
        const AZ::Transform cameraTransform = AZ::Transform::CreateLookAt(
            cameraPosition,
            cameraTarget,
            AZ::Transform::Axis::YPositive);

        AZ::TransformBus::Event(
            m_activeCamera,
            &AZ::TransformBus::Events::SetWorldTM,
            cameraTransform);
    }

    void SpaceKartLegendsSystemComponent::DisplayViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        m_race.DrawWorld(debugDisplay);
    }

    void SpaceKartLegendsSystemComponent::DisplayViewport2d(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        m_race.DrawHud(debugDisplay);
    }
}
