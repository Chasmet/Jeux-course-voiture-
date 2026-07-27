#include "SpaceKartRace.h"

#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/std/algorithm.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include <cmath>

namespace SpaceKartLegends
{
    namespace
    {
        constexpr float BasePlayerSpeed = 30.0f;
        constexpr float MaxLateralOffset = 4.2f;
        constexpr int TrackSegments = 144;

        float Clamp(float value, float minValue, float maxValue)
        {
            return AZStd::max(minValue, AZStd::min(maxValue, value));
        }
    }

    SpaceKartRace::SpaceKartRace()
        : m_circuits{
            CircuitDefinition{"Orbite Zero", 42.0f, 30.0f, 2.5f, 10.0f, 1.00f, AZ::Color(0.10f, 0.24f, 0.42f, 1.0f), AZ::Color(0.15f, 0.85f, 1.0f, 1.0f)},
            CircuitDefinition{"Anneaux de Saturne", 50.0f, 22.0f, 5.5f, 9.0f, 1.05f, AZ::Color(0.55f, 0.30f, 0.10f, 1.0f), AZ::Color(1.0f, 0.65f, 0.15f, 1.0f)},
            CircuitDefinition{"Nebuleuse Turbo", 36.0f, 36.0f, 8.0f, 11.0f, 1.10f, AZ::Color(0.38f, 0.10f, 0.52f, 1.0f), AZ::Color(0.95f, 0.20f, 1.0f, 1.0f)},
            CircuitDefinition{"Station Titan", 58.0f, 28.0f, 3.0f, 8.5f, 1.12f, AZ::Color(0.18f, 0.34f, 0.30f, 1.0f), AZ::Color(0.25f, 1.0f, 0.60f, 1.0f)},
            CircuitDefinition{"Trou Noir Final", 44.0f, 34.0f, 12.0f, 9.0f, 1.18f, AZ::Color(0.08f, 0.08f, 0.12f, 1.0f), AZ::Color(1.0f, 0.15f, 0.40f, 1.0f)}
        }
    {
        m_drivers[0].m_name = "Cheikh";
        m_drivers[0].m_color = AZ::Color(0.12f, 0.45f, 1.0f, 1.0f);
        m_drivers[1].m_name = "Yvane";
        m_drivers[1].m_color = AZ::Color(1.0f, 0.25f, 0.10f, 1.0f);
        m_drivers[2].m_name = "Nelvyn";
        m_drivers[2].m_color = AZ::Color(0.20f, 0.90f, 0.35f, 1.0f);
        m_drivers[3].m_name = "Nova";
        m_drivers[3].m_color = AZ::Color(0.95f, 0.25f, 0.95f, 1.0f);
        Reset();
    }

    void SpaceKartRace::Reset(int circuitIndex)
    {
        m_circuitIndex = AZStd::max(0, AZStd::min(CircuitCount - 1, circuitIndex));
        m_elapsedTime = 0.0f;
        m_steering = 0.0f;
        m_drifting = false;

        for (int i = 0; i < DriverCount; ++i)
        {
            DriverState& driver = m_drivers[i];
            driver.m_progress = -0.018f * static_cast<float>(i);
            driver.m_lateralOffset = (static_cast<float>(i) - 1.5f) * 1.7f;
            driver.m_speed = BasePlayerSpeed * (0.96f + 0.015f * static_cast<float>(i));
            driver.m_boostTime = 0.0f;
            driver.m_driftCharge = 0.0f;
            driver.m_lap = 1;
            driver.m_place = i + 1;
            driver.m_finished = false;
        }
    }

    void SpaceKartRace::Update(float deltaTime)
    {
        if (deltaTime <= 0.0f)
        {
            return;
        }

        m_elapsedTime += deltaTime;
        UpdatePlayer(deltaTime);
        UpdateAi(deltaTime);
        UpdateRanking();
    }

    void SpaceKartRace::SetSteering(float steering)
    {
        m_steering = Clamp(steering, -1.0f, 1.0f);
    }

    void SpaceKartRace::SetDrifting(bool drifting)
    {
        if (m_drifting && !drifting && m_drivers[0].m_driftCharge > 0.45f)
        {
            m_drivers[0].m_boostTime = Clamp(0.45f + m_drivers[0].m_driftCharge * 0.45f, 0.45f, 1.8f);
            m_drivers[0].m_driftCharge = 0.0f;
        }
        m_drifting = drifting;
    }

    void SpaceKartRace::UseBoost()
    {
        DriverState& player = m_drivers[0];
        if (!player.m_finished)
        {
            player.m_boostTime = AZStd::max(player.m_boostTime, 1.25f);
        }
    }

    void SpaceKartRace::RecoverPlayer()
    {
        m_drivers[0].m_lateralOffset = 0.0f;
        m_drivers[0].m_speed = BasePlayerSpeed * 0.8f;
        m_drivers[0].m_driftCharge = 0.0f;
    }

    void SpaceKartRace::SelectNextCircuit()
    {
        Reset((m_circuitIndex + 1) % CircuitCount);
    }

    void SpaceKartRace::UpdatePlayer(float deltaTime)
    {
        DriverState& player = m_drivers[0];
        if (player.m_finished)
        {
            return;
        }

        const CircuitDefinition& circuit = GetCircuit();
        const float targetSpeed = BasePlayerSpeed * circuit.m_speedFactor + (player.m_boostTime > 0.0f ? 16.0f : 0.0f);
        const float acceleration = player.m_boostTime > 0.0f ? 18.0f : 8.0f;
        player.m_speed += (targetSpeed - player.m_speed) * Clamp(acceleration * deltaTime, 0.0f, 1.0f);

        const float steeringGrip = m_drifting ? 7.2f : 5.2f;
        player.m_lateralOffset += m_steering * steeringGrip * deltaTime;
        player.m_lateralOffset = Clamp(player.m_lateralOffset, -MaxLateralOffset, MaxLateralOffset);

        if (m_drifting && std::fabs(m_steering) > 0.25f)
        {
            player.m_driftCharge = Clamp(player.m_driftCharge + deltaTime, 0.0f, 2.8f);
        }
        else if (!m_drifting)
        {
            player.m_driftCharge = AZStd::max(0.0f, player.m_driftCharge - deltaTime * 0.35f);
        }

        player.m_boostTime = AZStd::max(0.0f, player.m_boostTime - deltaTime);
        const float approximateLength = AZ::Constants::TwoPi * (circuit.m_radiusX + circuit.m_radiusY) * 0.5f;
        player.m_progress += (player.m_speed / approximateLength) * deltaTime;

        while (player.m_progress >= 1.0f)
        {
            player.m_progress -= 1.0f;
            ++player.m_lap;
            if (player.m_lap > LapCount)
            {
                player.m_lap = LapCount;
                player.m_finished = true;
                player.m_progress = 0.9999f;
            }
        }
    }

    void SpaceKartRace::UpdateAi(float deltaTime)
    {
        const CircuitDefinition& circuit = GetCircuit();
        const float approximateLength = AZ::Constants::TwoPi * (circuit.m_radiusX + circuit.m_radiusY) * 0.5f;

        for (int i = 1; i < DriverCount; ++i)
        {
            DriverState& driver = m_drivers[i];
            if (driver.m_finished)
            {
                continue;
            }

            const float personality = 0.94f + static_cast<float>(i) * 0.025f;
            const float wave = std::sin(m_elapsedTime * (0.9f + i * 0.17f) + i * 1.4f);
            const float catchUp = driver.m_place > 2 ? 1.04f : 1.0f;
            const float target = BasePlayerSpeed * circuit.m_speedFactor * personality * catchUp + wave * 1.4f;
            driver.m_speed += (target - driver.m_speed) * Clamp(3.5f * deltaTime, 0.0f, 1.0f);
            driver.m_lateralOffset = std::sin(driver.m_progress * AZ::Constants::TwoPi * 2.0f + i) * (1.0f + i * 0.28f);
            driver.m_progress += (driver.m_speed / approximateLength) * deltaTime;

            while (driver.m_progress >= 1.0f)
            {
                driver.m_progress -= 1.0f;
                ++driver.m_lap;
                if (driver.m_lap > LapCount)
                {
                    driver.m_lap = LapCount;
                    driver.m_finished = true;
                    driver.m_progress = 0.9999f;
                }
            }
        }
    }

    void SpaceKartRace::UpdateRanking()
    {
        for (int i = 0; i < DriverCount; ++i)
        {
            int place = 1;
            for (int j = 0; j < DriverCount; ++j)
            {
                if (RaceScore(m_drivers[j]) > RaceScore(m_drivers[i]))
                {
                    ++place;
                }
            }
            m_drivers[i].m_place = place;
        }
    }

    float SpaceKartRace::WrapProgress(float progress)
    {
        progress -= std::floor(progress);
        return progress;
    }

    float SpaceKartRace::RaceScore(const DriverState& driver)
    {
        return static_cast<float>(driver.m_lap - 1) + driver.m_progress;
    }

    AZ::Vector3 SpaceKartRace::GetTrackPosition(float progress, float lateralOffset) const
    {
        const CircuitDefinition& circuit = GetCircuit();
        const float p = WrapProgress(progress);
        const float angle = p * AZ::Constants::TwoPi;
        const float phase = static_cast<float>(m_circuitIndex) * 0.7f;
        AZ::Vector3 center(
            std::cos(angle) * circuit.m_radiusX,
            std::sin(angle) * circuit.m_radiusY,
            std::sin(angle * (2.0f + 0.35f * m_circuitIndex) + phase) * circuit.m_heightWave);

        const AZ::Vector3 tangent = GetTrackTangent(p);
        const AZ::Vector3 right = tangent.Cross(AZ::Vector3::CreateAxisZ()).GetNormalizedSafe(AZ::Vector3::CreateAxisX());
        return center + right * lateralOffset;
    }

    AZ::Vector3 SpaceKartRace::GetTrackTangent(float progress) const
    {
        const float epsilon = 0.001f;
        const AZ::Vector3 a = GetTrackPosition(WrapProgress(progress - epsilon), 0.0f);
        const AZ::Vector3 b = GetTrackPosition(WrapProgress(progress + epsilon), 0.0f);
        return (b - a).GetNormalizedSafe(AZ::Vector3::CreateAxisY());
    }

    AZ::Vector3 SpaceKartRace::GetPlayerPosition() const
    {
        return GetTrackPosition(m_drivers[0].m_progress, m_drivers[0].m_lateralOffset) + AZ::Vector3(0.0f, 0.0f, 0.75f);
    }

    AZ::Vector3 SpaceKartRace::GetPlayerTangent() const
    {
        return GetTrackTangent(m_drivers[0].m_progress);
    }

    const DriverState& SpaceKartRace::GetPlayer() const
    {
        return m_drivers[0];
    }

    const CircuitDefinition& SpaceKartRace::GetCircuit() const
    {
        return m_circuits[m_circuitIndex];
    }

    int SpaceKartRace::GetCircuitIndex() const
    {
        return m_circuitIndex;
    }

    void SpaceKartRace::DrawWorld(AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        DrawSpaceEnvironment(debugDisplay);
        DrawTrack(debugDisplay);
        for (int i = 0; i < DriverCount; ++i)
        {
            DrawKart(debugDisplay, m_drivers[i], i);
        }
    }

    void SpaceKartRace::DrawTrack(AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        const CircuitDefinition& circuit = GetCircuit();
        const AZ::Vector3 up = AZ::Vector3::CreateAxisZ();

        for (int i = 0; i < TrackSegments; ++i)
        {
            const float p0 = static_cast<float>(i) / TrackSegments;
            const float p1 = static_cast<float>(i + 1) / TrackSegments;
            const AZ::Vector3 a = GetTrackPosition(p0);
            const AZ::Vector3 b = GetTrackPosition(p1);
            const AZ::Vector3 center = (a + b) * 0.5f;
            const AZ::Vector3 forward = (b - a).GetNormalizedSafe(AZ::Vector3::CreateAxisY());
            const AZ::Vector3 right = forward.Cross(up).GetNormalizedSafe(AZ::Vector3::CreateAxisX());
            const float length = (b - a).GetLength();

            debugDisplay.SetColor(circuit.m_trackColor);
            debugDisplay.DrawSolidOBB(center, right, forward, up, AZ::Vector3(circuit.m_trackWidth * 0.5f, length * 0.55f, 0.32f));

            if ((i % 3) == 0)
            {
                debugDisplay.SetColor(circuit.m_glowColor);
                debugDisplay.DrawSolidCylinder(center + right * (circuit.m_trackWidth * 0.52f) + up * 0.45f, forward, 0.13f, length);
                debugDisplay.DrawSolidCylinder(center - right * (circuit.m_trackWidth * 0.52f) + up * 0.45f, forward, 0.13f, length);
            }

            if ((i % 18) == 0)
            {
                debugDisplay.SetColor(AZ::Color(0.15f, 0.85f, 1.0f, 0.85f));
                debugDisplay.DrawSolidOBB(center + up * 0.45f, right, forward, up, AZ::Vector3(circuit.m_trackWidth * 0.36f, length * 0.32f, 0.08f));
            }
        }
    }

    void SpaceKartRace::DrawKart(AzFramework::DebugDisplayRequests& debugDisplay, const DriverState& driver, int driverIndex) const
    {
        const AZ::Vector3 position = GetTrackPosition(driver.m_progress, driver.m_lateralOffset) + AZ::Vector3(0.0f, 0.0f, 0.8f);
        const AZ::Vector3 tangent = GetTrackTangent(driver.m_progress);
        const AZ::Transform transform = AZ::Transform::CreateLookAt(position, position + tangent, AZ::Transform::Axis::YPositive);

        debugDisplay.PushMatrix(transform);
        debugDisplay.SetColor(driver.m_color);
        debugDisplay.DrawSolidBox(AZ::Vector3(-0.82f, -1.15f, -0.28f), AZ::Vector3(0.82f, 1.15f, 0.28f));
        debugDisplay.SetColor(AZ::Color(0.08f, 0.10f, 0.14f, 1.0f));
        debugDisplay.DrawSolidBox(AZ::Vector3(-0.58f, -0.40f, 0.18f), AZ::Vector3(0.58f, 0.55f, 0.72f));

        debugDisplay.SetColor(AZ::Color(0.03f, 0.03f, 0.04f, 1.0f));
        const AZ::Vector3 wheelAxis = AZ::Vector3::CreateAxisX();
        debugDisplay.DrawSolidCylinder(AZ::Vector3(-0.94f, -0.70f, -0.12f), wheelAxis, 0.31f, 0.28f);
        debugDisplay.DrawSolidCylinder(AZ::Vector3(0.94f, -0.70f, -0.12f), wheelAxis, 0.31f, 0.28f);
        debugDisplay.DrawSolidCylinder(AZ::Vector3(-0.94f, 0.72f, -0.12f), wheelAxis, 0.31f, 0.28f);
        debugDisplay.DrawSolidCylinder(AZ::Vector3(0.94f, 0.72f, -0.12f), wheelAxis, 0.31f, 0.28f);

        const AZ::Color suitColor = driverIndex == 0 ? AZ::Color(0.08f, 0.12f, 0.22f, 1.0f) : driver.m_color;
        debugDisplay.SetColor(suitColor);
        debugDisplay.DrawSolidCylinder(AZ::Vector3(0.0f, 0.02f, 1.00f), AZ::Vector3::CreateAxisZ(), 0.34f, 0.75f);
        debugDisplay.SetColor(AZ::Color(0.58f, 0.36f, 0.22f, 1.0f));
        debugDisplay.DrawBall(AZ::Vector3(0.0f, 0.04f, 1.62f), 0.39f);
        debugDisplay.SetColor(driver.m_color);
        debugDisplay.DrawSolidCylinder(AZ::Vector3(0.0f, 0.01f, 1.82f), AZ::Vector3::CreateAxisZ(), 0.42f, 0.22f);

        debugDisplay.SetColor(AZ::Color(0.75f, 0.90f, 1.0f, 1.0f));
        debugDisplay.DrawSolidCylinder(AZ::Vector3(0.0f, 0.72f, 0.88f), AZ::Vector3::CreateAxisX(), 0.08f, 0.75f);

        if (driver.m_boostTime > 0.0f)
        {
            debugDisplay.SetColor(AZ::Color(0.20f, 0.85f, 1.0f, 0.9f));
            debugDisplay.DrawSolidCone(AZ::Vector3(-0.35f, -1.35f, 0.0f), -AZ::Vector3::CreateAxisY(), 0.28f, 1.6f);
            debugDisplay.DrawSolidCone(AZ::Vector3(0.35f, -1.35f, 0.0f), -AZ::Vector3::CreateAxisY(), 0.28f, 1.6f);
        }

        debugDisplay.PopMatrix();
    }

    void SpaceKartRace::DrawSpaceEnvironment(AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        debugDisplay.SetColor(AZ::Color(0.75f, 0.88f, 1.0f, 1.0f));
        for (int i = 0; i < 80; ++i)
        {
            const float a = static_cast<float>(i) * 2.399963f;
            const float radius = 72.0f + static_cast<float>((i * 19) % 85);
            const float z = -25.0f + static_cast<float>((i * 37) % 78);
            debugDisplay.DrawBall(AZ::Vector3(std::cos(a) * radius, std::sin(a) * radius, z), 0.12f + 0.03f * (i % 3));
        }

        debugDisplay.SetColor(AZ::Color(0.85f, 0.44f, 0.12f, 1.0f));
        debugDisplay.DrawBall(AZ::Vector3(-82.0f, 40.0f, 18.0f), 9.0f);
        debugDisplay.SetColor(AZ::Color(0.28f, 0.52f, 0.95f, 1.0f));
        debugDisplay.DrawBall(AZ::Vector3(76.0f, -48.0f, -8.0f), 6.5f);

        if (m_circuitIndex == 4)
        {
            debugDisplay.SetColor(AZ::Color(0.02f, 0.02f, 0.025f, 1.0f));
            debugDisplay.DrawBall(AZ::Vector3::CreateZero(), 10.0f);
            debugDisplay.SetColor(GetCircuit().m_glowColor);
            debugDisplay.DrawWireSphere(AZ::Vector3::CreateZero(), 12.5f);
            debugDisplay.DrawWireSphere(AZ::Vector3::CreateZero(), 15.0f);
        }
    }

    void SpaceKartRace::DrawHud(AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        const DriverState& player = GetPlayer();
        const AZStd::string line1 = AZStd::string::format("%s  |  Position %d/%d", GetCircuit().m_name, player.m_place, DriverCount);
        const AZStd::string line2 = AZStd::string::format("Tour %d/%d  |  Vitesse %d km/h", player.m_lap, LapCount, static_cast<int>(player.m_speed * 6.0f));
        const AZStd::string line3 = AZStd::string::format("Mini-turbo %.0f%%", Clamp(player.m_driftCharge / 2.8f, 0.0f, 1.0f) * 100.0f);

        debugDisplay.SetColor(AZ::Color::CreateOne());
        debugDisplay.Draw2dTextLabel(26.0f, 30.0f, 1.45f, line1.c_str(), false);
        debugDisplay.Draw2dTextLabel(26.0f, 56.0f, 1.25f, line2.c_str(), false);
        debugDisplay.Draw2dTextLabel(26.0f, 80.0f, 1.05f, line3.c_str(), false);

        if (player.m_finished)
        {
            debugDisplay.SetColor(GetCircuit().m_glowColor);
            debugDisplay.Draw2dTextLabel(0.50f, 0.42f, 2.4f, "COURSE TERMINEE", true);
            debugDisplay.Draw2dTextLabel(0.50f, 0.50f, 1.5f, "Appuie sur N pour le circuit suivant", true);
        }
    }
}
