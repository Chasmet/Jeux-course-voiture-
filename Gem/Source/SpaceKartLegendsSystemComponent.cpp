#include "SpaceKartLegendsSystemComponent.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>
#include <AzFramework/Components/CameraBus.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Touch/InputDeviceTouch.h>

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
            serializeContext->Class<SpaceKartLegendsSystemComponent, AZ::Component>()->Version(5);
            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SpaceKartLegendsSystemComponent>(
                    "Space Kart Legends",
                    "Course arcade 3D, objets, IA, circuits et commandes Android.")
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
        m_touchSteering = 0.0f;
        m_activeDriftTouches = 0;
        m_activeBrakeTouches = 0;
        m_touchRoles.fill(TouchRole::None);
        m_activeCamera = AZ::EntityId();
        m_ownsActiveCamera = false;
        m_race.Reset();

        AzFramework::InputChannelEventListener::Connect();
        AZ::TickBus::Handler::BusConnect();

        AzFramework::EntityContextId gameContextId = AzFramework::EntityContextId::CreateNull();
        AzFramework::GameEntityContextRequestBus::BroadcastResult(
            gameContextId,
            &AzFramework::GameEntityContextRequestBus::Events::GetGameEntityContextId);
        AzFramework::ViewportDebugDisplayEventBus::Handler::BusConnect(gameContextId);

        EnsureActiveCamera();
        UpdateCamera();
    }

    void SpaceKartLegendsSystemComponent::Deactivate()
    {
        AzFramework::ViewportDebugDisplayEventBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        AzFramework::InputChannelEventListener::Disconnect();
        DestroyOwnedCamera();
        m_activeCamera = AZ::EntityId();
        m_touchRoles.fill(TouchRole::None);
        m_activeDriftTouches = 0;
        m_activeBrakeTouches = 0;
    }

    void SpaceKartLegendsSystemComponent::EnsureActiveCamera()
    {
        if (m_activeCamera.IsValid())
        {
            return;
        }

        Camera::CameraSystemRequestBus::BroadcastResult(
            m_activeCamera,
            &Camera::CameraSystemRequestBus::Events::GetActiveCamera);
        if (m_activeCamera.IsValid())
        {
            m_ownsActiveCamera = false;
            return;
        }

        AZ::Entity* cameraEntity = nullptr;
        AzFramework::GameEntityContextRequestBus::BroadcastResult(
            cameraEntity,
            &AzFramework::GameEntityContextRequestBus::Events::CreateGameEntity,
            "SpaceKartGameplayCamera");
        if (!cameraEntity)
        {
            return;
        }

        const AZ::EntityId cameraId = cameraEntity->GetId();
        AZ::Component* transformComponent = cameraEntity->CreateComponent<AzFramework::TransformComponent>();
        AZ::Component* cameraComponent = cameraEntity->CreateComponent(Camera::CameraComponentTypeId);
        if (!transformComponent || !cameraComponent)
        {
            AzFramework::GameEntityContextRequestBus::Broadcast(
                &AzFramework::GameEntityContextRequestBus::Events::DestroyGameEntity,
                cameraId);
            return;
        }

        cameraEntity->Init();
        AzFramework::GameEntityContextRequestBus::Broadcast(
            &AzFramework::GameEntityContextRequestBus::Events::ActivateGameEntity,
            cameraId);

        m_activeCamera = cameraId;
        m_ownsActiveCamera = true;
        Camera::CameraRequestBus::Event(m_activeCamera, &Camera::CameraRequestBus::Events::SetFovDegrees, 68.0f);
        Camera::CameraRequestBus::Event(m_activeCamera, &Camera::CameraRequestBus::Events::SetNearClipDistance, 0.08f);
        Camera::CameraRequestBus::Event(m_activeCamera, &Camera::CameraRequestBus::Events::SetFarClipDistance, 600.0f);
        Camera::CameraRequestBus::Event(m_activeCamera, &Camera::CameraRequestBus::Events::MakeActiveView);
    }

    void SpaceKartLegendsSystemComponent::DestroyOwnedCamera()
    {
        if (m_ownsActiveCamera && m_activeCamera.IsValid())
        {
            AzFramework::GameEntityContextRequestBus::Broadcast(
                &AzFramework::GameEntityContextRequestBus::Events::DestroyGameEntity,
                m_activeCamera);
        }
        m_ownsActiveCamera = false;
    }

    void SpaceKartLegendsSystemComponent::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        ApplyDigitalSteering();
        m_race.SetAccelerating(m_acceleratePressed);
        m_race.SetBraking(m_brakePressed || m_activeBrakeTouches > 0);
        m_race.SetDrifting(m_driftPressed || m_activeDriftTouches > 0);
        m_race.Update(deltaTime);
        UpdateCamera();
    }

    void SpaceKartLegendsSystemComponent::ApplyDigitalSteering()
    {
        float steering = m_gamepadSteering + m_touchSteering;
        steering += m_rightPressed ? 1.0f : 0.0f;
        steering -= m_leftPressed ? 1.0f : 0.0f;
        m_race.SetSteering(AZStd::max(-1.0f, AZStd::min(1.0f, steering)));
    }

    void SpaceKartLegendsSystemComponent::ReleaseTouchRole(size_t touchIndex)
    {
        if (touchIndex >= m_touchRoles.size())
        {
            return;
        }

        switch (m_touchRoles[touchIndex])
        {
        case TouchRole::Steering:
            m_touchSteering = 0.0f;
            break;
        case TouchRole::Drift:
            m_activeDriftTouches = AZStd::max(0, m_activeDriftTouches - 1);
            break;
        case TouchRole::Brake:
            m_activeBrakeTouches = AZStd::max(0, m_activeBrakeTouches - 1);
            break;
        case TouchRole::None:
            break;
        }
        m_touchRoles[touchIndex] = TouchRole::None;
    }

    bool SpaceKartLegendsSystemComponent::HandleTouchInput(const AzFramework::InputChannel& inputChannel)
    {
        const auto& touches = AzFramework::InputDeviceTouch::Touch::All;
        size_t touchIndex = 0;
        while (touchIndex < touches.size() && touches[touchIndex] != inputChannel.GetInputChannelId())
        {
            ++touchIndex;
        }
        if (touchIndex >= touches.size() || touchIndex >= m_touchRoles.size())
        {
            return false;
        }

        const auto* positionData = inputChannel.GetCustomData<AzFramework::InputChannel::PositionData2D>();
        if (!positionData)
        {
            return false;
        }
        if (inputChannel.IsStateEnded())
        {
            ReleaseTouchRole(touchIndex);
            return true;
        }

        const float x = positionData->m_normalizedPosition.GetX();
        const float y = positionData->m_normalizedPosition.GetY();
        if (inputChannel.IsStateBegan())
        {
            ReleaseTouchRole(touchIndex);
            if (x < 0.55f)
            {
                m_touchRoles[touchIndex] = TouchRole::Steering;
            }
            else if (y < 0.34f)
            {
                m_race.UseItem();
                return true;
            }
            else if (y < 0.78f)
            {
                m_touchRoles[touchIndex] = TouchRole::Drift;
                ++m_activeDriftTouches;
                return true;
            }
            else
            {
                m_touchRoles[touchIndex] = TouchRole::Brake;
                ++m_activeBrakeTouches;
                return true;
            }
        }

        if (m_touchRoles[touchIndex] == TouchRole::Steering)
        {
            const float centered = (x - 0.275f) / 0.24f;
            m_touchSteering = AZStd::max(-1.0f, AZStd::min(1.0f, centered));
        }
        return true;
    }

    bool SpaceKartLegendsSystemComponent::OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel)
    {
        if (AzFramework::InputDeviceTouch::IsTouchDevice(inputChannel.GetInputDevice().GetInputDeviceId()))
        {
            return HandleTouchInput(inputChannel);
        }

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
            return true;
        }
        if ((id == AzFramework::InputDeviceKeyboard::Key::AlphanumericB ||
             id == AzFramework::InputDeviceGamepad::Button::B) &&
            inputChannel.GetState() == AzFramework::InputChannel::State::Began)
        {
            m_race.UseItem();
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
        EnsureActiveCamera();
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
        AZ::TransformBus::Event(m_activeCamera, &AZ::TransformBus::Events::SetWorldTM, cameraTransform);
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
