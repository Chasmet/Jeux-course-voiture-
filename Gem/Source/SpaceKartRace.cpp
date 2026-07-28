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
        constexpr int ItemGateCount = 4;
        constexpr float ItemGateProgresses[ItemGateCount] = {0.16f, 0.41f, 0.66f, 0.91f};

        float Clamp(float value, float minValue, float maxValue)
        {
            return AZStd::max(minValue, AZStd::min(maxValue, value));
        }

        AZ::Color KartBodyColor(int driverIndex)
        {
            switch (driverIndex)
            {
            case 0:
                return AZ::Color(0.76f, 0.70f, 0.56f, 1.0f);
            case 1:
                return AZ::Color(0.035f, 0.045f, 0.065f, 1.0f);
            case 2:
                return AZ::Color(0.045f, 0.055f, 0.070f, 1.0f);
            default:
                return AZ::Color(0.16f, 0.06f, 0.22f, 1.0f);
            }
        }

        AZ::Color SuitPrimaryColor(int driverIndex)
        {
            switch (driverIndex)
            {
            case 0:
                return AZ::Color(0.72f, 0.68f, 0.58f, 1.0f);
            case 1:
                return AZ::Color(0.025f, 0.030f, 0.045f, 1.0f);
            case 2:
                return AZ::Color(0.035f, 0.040f, 0.052f, 1.0f);
            default:
                return AZ::Color(0.10f, 0.05f, 0.15f, 1.0f);
            }
        }

        AZ::Color ItemColor(ItemType item)
        {
            switch (item)
            {
            case ItemType::CometBoost:
                return AZ::Color(0.15f, 0.85f, 1.0f, 1.0f);
            case ItemType::PlasmaShield:
                return AZ::Color(0.25f, 0.55f, 1.0f, 1.0f);
            case ItemType::GravityMine:
                return AZ::Color(1.0f, 0.25f, 0.45f, 1.0f);
            case ItemType::PhotonPulse:
                return AZ::Color(0.95f, 0.40f, 1.0f, 1.0f);
            case ItemType::None:
            default:
                return AZ::Color(0.75f, 0.90f, 1.0f, 1.0f);
            }
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
        m_drivers[0].m_color = AZ::Color(0.10f, 0.55f, 1.0f, 1.0f);
        m_drivers[1].m_name = "Yvane";
        m_drivers[1].m_color = AZ::Color(1.0f, 0.72f, 0.05f, 1.0f);
        m_drivers[2].m_name = "Nelvyn";
        m_drivers[2].m_color = AZ::Color(0.15f, 0.95f, 0.42f, 1.0f);
        m_drivers[3].m_name = "Nova";
        m_drivers[3].m_color = AZ::Color(0.95f, 0.25f, 0.95f, 1.0f);
        Reset();
    }

    void SpaceKartRace::Reset(int circuitIndex)
    {
        m_circuitIndex = AZStd::max(0, AZStd::min(CircuitCount - 1, circuitIndex));
        m_elapsedTime = 0.0f;
        m_countdownTime = 3.5f;
        m_racePhase = RacePhase::Countdown;
        m_steering = 0.0f;
        m_accelerating = true;
        m_braking = false;
        m_drifting = false;

        for (int i = 0; i < DriverCount; ++i)
        {
            DriverState& driver = m_drivers[i];
            driver.m_progress = 0.002f;
            driver.m_lateralOffset = (static_cast<float>(i) - 1.5f) * 1.75f;
            driver.m_speed = 0.0f;
            driver.m_boostTime = 0.0f;
            driver.m_driftCharge = 0.0f;
            driver.m_airTime = 0.0f;
            driver.m_shieldTime = 0.0f;
            driver.m_slowTime = 0.0f;
            driver.m_spinTime = 0.0f;
            driver.m_aiItemDecisionTime = 1.5f + static_cast<float>(i) * 0.35f;
            driver.m_lap = 1;
            driver.m_place = i + 1;
            driver.m_nextPickupIndex = 0;
            driver.m_item = ItemType::None;
            driver.m_finished = false;
        }
    }

    void SpaceKartRace::Update(float deltaTime)
    {
        if (deltaTime <= 0.0f)
        {
            return;
        }

        const float safeDeltaTime = AZStd::min(deltaTime, 0.05f);
        m_elapsedTime += safeDeltaTime;

        if (m_racePhase == RacePhase::Countdown)
        {
            m_countdownTime = AZStd::max(0.0f, m_countdownTime - safeDeltaTime);
            if (m_countdownTime <= 0.0f)
            {
                m_racePhase = RacePhase::Racing;
                for (int i = 0; i < DriverCount; ++i)
                {
                    m_drivers[i].m_speed = BasePlayerSpeed * (0.82f + 0.025f * static_cast<float>(i));
                }
            }
            return;
        }

        if (m_racePhase == RacePhase::Finished)
        {
            return;
        }

        UpdatePlayer(safeDeltaTime);
        UpdateAi(safeDeltaTime);
        UpdateRanking();

        if (m_drivers[0].m_finished)
        {
            m_racePhase = RacePhase::Finished;
        }
    }

    void SpaceKartRace::SetSteering(float steering)
    {
        m_steering = Clamp(steering, -1.0f, 1.0f);
    }

    void SpaceKartRace::SetAccelerating(bool accelerating)
    {
        m_accelerating = accelerating;
    }

    void SpaceKartRace::SetBraking(bool braking)
    {
        m_braking = braking;
    }

    void SpaceKartRace::SetDrifting(bool drifting)
    {
        if (m_drifting && !drifting && m_racePhase == RacePhase::Racing && m_drivers[0].m_driftCharge > 0.45f)
        {
            m_drivers[0].m_boostTime = Clamp(0.45f + m_drivers[0].m_driftCharge * 0.45f, 0.45f, 1.8f);
            m_drivers[0].m_driftCharge = 0.0f;
        }
        m_drifting = drifting;
    }

    void SpaceKartRace::UseBoost()
    {
        DriverState& player = m_drivers[0];
        if (m_racePhase == RacePhase::Racing && !player.m_finished)
        {
            player.m_boostTime = AZStd::max(player.m_boostTime, 1.25f);
            player.m_airTime = AZStd::max(player.m_airTime, 0.20f);
        }
    }

    void SpaceKartRace::UseItem()
    {
        if (m_racePhase == RacePhase::Racing)
        {
            UseDriverItem(0);
        }
    }

    void SpaceKartRace::RecoverPlayer()
    {
        DriverState& player = m_drivers[0];
        player.m_lateralOffset = 0.0f;
        player.m_speed = BasePlayerSpeed * 0.8f;
        player.m_driftCharge = 0.0f;
        player.m_boostTime = 0.0f;
        player.m_airTime = 0.0f;
        player.m_slowTime = 0.0f;
        player.m_spinTime = 0.0f;
    }

    void SpaceKartRace::SelectNextCircuit()
    {
        Reset((m_circuitIndex + 1) % CircuitCount);
    }

    void SpaceKartRace::UpdateDriverTimers(DriverState& driver, float deltaTime)
    {
        driver.m_boostTime = AZStd::max(0.0f, driver.m_boostTime - deltaTime);
        driver.m_airTime = AZStd::max(0.0f, driver.m_airTime - deltaTime);
        driver.m_shieldTime = AZStd::max(0.0f, driver.m_shieldTime - deltaTime);
        driver.m_slowTime = AZStd::max(0.0f, driver.m_slowTime - deltaTime);
        driver.m_spinTime = AZStd::max(0.0f, driver.m_spinTime - deltaTime);
        driver.m_aiItemDecisionTime = AZStd::max(0.0f, driver.m_aiItemDecisionTime - deltaTime);
    }

    void SpaceKartRace::UpdatePlayer(float deltaTime)
    {
        DriverState& player = m_drivers[0];
        UpdateDriverTimers(player, deltaTime);
        if (player.m_finished)
        {
            return;
        }

        const CircuitDefinition& circuit = GetCircuit();
        float throttleFactor = m_accelerating ? 1.0f : 0.72f;
        if (m_braking)
        {
            throttleFactor = 0.36f;
        }
        if (player.m_slowTime > 0.0f)
        {
            throttleFactor *= 0.62f;
        }
        if (player.m_spinTime > 0.0f)
        {
            throttleFactor *= 0.42f;
        }

        const float boostSpeed = player.m_boostTime > 0.0f ? 16.0f : 0.0f;
        const float targetSpeed = BasePlayerSpeed * circuit.m_speedFactor * throttleFactor + boostSpeed;
        const float acceleration = player.m_boostTime > 0.0f ? 18.0f : (m_braking ? 12.0f : 7.5f);
        player.m_speed += (targetSpeed - player.m_speed) * Clamp(acceleration * deltaTime, 0.0f, 1.0f);

        if (player.m_spinTime > 0.0f)
        {
            player.m_lateralOffset += std::sin(m_elapsedTime * 22.0f) * 4.8f * deltaTime;
        }
        else
        {
            const float steeringGrip = m_drifting ? 7.6f : 5.4f;
            const float speedSteeringScale = Clamp(player.m_speed / BasePlayerSpeed, 0.45f, 1.35f);
            player.m_lateralOffset += m_steering * steeringGrip * speedSteeringScale * deltaTime;
        }
        player.m_lateralOffset = Clamp(player.m_lateralOffset, -MaxLateralOffset, MaxLateralOffset);

        if (m_drifting && player.m_spinTime <= 0.0f && std::fabs(m_steering) > 0.25f && player.m_speed > BasePlayerSpeed * 0.55f)
        {
            player.m_driftCharge = Clamp(player.m_driftCharge + deltaTime, 0.0f, 2.8f);
            player.m_speed = AZStd::max(player.m_speed - deltaTime * 1.1f, BasePlayerSpeed * 0.62f);
        }
        else if (!m_drifting)
        {
            player.m_driftCharge = AZStd::max(0.0f, player.m_driftCharge - deltaTime * 0.35f);
        }

        const float approximateLength = AZ::Constants::TwoPi * (circuit.m_radiusX + circuit.m_radiusY) * 0.5f;
        const float previousProgress = player.m_progress;
        player.m_progress += (player.m_speed / approximateLength) * deltaTime;
        CheckItemPickup(player, 0, previousProgress);

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
            UpdateDriverTimers(driver, deltaTime);
            if (driver.m_finished)
            {
                continue;
            }

            if (driver.m_item != ItemType::None && driver.m_aiItemDecisionTime <= 0.0f)
            {
                UseDriverItem(i);
                driver.m_aiItemDecisionTime = 2.0f + 0.35f * static_cast<float>(i);
            }

            const float personality = 0.94f + static_cast<float>(i) * 0.025f;
            const float wave = std::sin(m_elapsedTime * (0.9f + i * 0.17f) + i * 1.4f);
            const float catchUp = driver.m_place > 2 ? 1.04f : 1.0f;
            const float statusFactor = driver.m_spinTime > 0.0f ? 0.42f : (driver.m_slowTime > 0.0f ? 0.64f : 1.0f);
            const float boostSpeed = driver.m_boostTime > 0.0f ? 10.0f : 0.0f;
            const float target = (BasePlayerSpeed * circuit.m_speedFactor * personality * catchUp + wave * 1.4f) * statusFactor + boostSpeed;
            driver.m_speed += (target - driver.m_speed) * Clamp(3.5f * deltaTime, 0.0f, 1.0f);

            const float normalLine = std::sin(driver.m_progress * AZ::Constants::TwoPi * 2.0f + i) * (1.0f + i * 0.28f);
            driver.m_lateralOffset = driver.m_spinTime > 0.0f
                ? Clamp(normalLine + std::sin(m_elapsedTime * 20.0f + i) * 2.2f, -MaxLateralOffset, MaxLateralOffset)
                : normalLine;

            const float previousProgress = driver.m_progress;
            driver.m_progress += (driver.m_speed / approximateLength) * deltaTime;
            CheckItemPickup(driver, i, previousProgress);

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

    void SpaceKartRace::CheckItemPickup(DriverState& driver, int driverIndex, float previousProgress)
    {
        if (driver.m_finished || driver.m_item != ItemType::None)
        {
            return;
        }

        const int pickupIndex = AZStd::max(0, AZStd::min(ItemGateCount - 1, driver.m_nextPickupIndex));
        const float gateProgress = ItemGateProgresses[pickupIndex];
        if (previousProgress < gateProgress && driver.m_progress >= gateProgress)
        {
            driver.m_item = SelectPickupItem(driver, driverIndex);
            driver.m_nextPickupIndex = (pickupIndex + 1) % ItemGateCount;
            driver.m_aiItemDecisionTime = 0.75f + 0.25f * static_cast<float>(driverIndex);
        }
    }

    ItemType SpaceKartRace::SelectPickupItem(const DriverState& driver, int driverIndex) const
    {
        if (driver.m_place >= 4)
        {
            return ((driver.m_lap + driverIndex) % 2) == 0 ? ItemType::CometBoost : ItemType::PhotonPulse;
        }
        if (driver.m_place == 1)
        {
            return ((driver.m_lap + driverIndex) % 2) == 0 ? ItemType::PlasmaShield : ItemType::GravityMine;
        }

        const int selection = (driverIndex + driver.m_lap + driver.m_nextPickupIndex) % 4;
        switch (selection)
        {
        case 0:
            return ItemType::CometBoost;
        case 1:
            return ItemType::PlasmaShield;
        case 2:
            return ItemType::GravityMine;
        default:
            return ItemType::PhotonPulse;
        }
    }

    void SpaceKartRace::UseDriverItem(int driverIndex)
    {
        if (driverIndex < 0 || driverIndex >= DriverCount)
        {
            return;
        }

        DriverState& driver = m_drivers[driverIndex];
        if (driver.m_finished || driver.m_item == ItemType::None)
        {
            return;
        }

        switch (driver.m_item)
        {
        case ItemType::CometBoost:
            driver.m_boostTime = AZStd::max(driver.m_boostTime, 1.85f);
            driver.m_airTime = AZStd::max(driver.m_airTime, 0.14f);
            break;
        case ItemType::PlasmaShield:
            driver.m_shieldTime = AZStd::max(driver.m_shieldTime, 5.0f);
            break;
        case ItemType::GravityMine:
        {
            const int target = FindTargetBehind(driverIndex);
            if (target >= 0)
            {
                const float impulse = (target % 2) == 0 ? -1.4f : 1.4f;
                ApplyHit(target, 0.45f, 1.15f, impulse);
            }
            break;
        }
        case ItemType::PhotonPulse:
        {
            const int target = FindTargetAhead(driverIndex);
            if (target >= 0)
            {
                const float impulse = (target % 2) == 0 ? 1.0f : -1.0f;
                ApplyHit(target, 1.8f, 0.35f, impulse);
            }
            break;
        }
        case ItemType::None:
            break;
        }

        driver.m_item = ItemType::None;
    }

    int SpaceKartRace::FindTargetAhead(int driverIndex) const
    {
        int bestTarget = -1;
        float bestDistance = 1000.0f;
        for (int i = 0; i < DriverCount; ++i)
        {
            if (i == driverIndex || m_drivers[i].m_finished)
            {
                continue;
            }
            const float distance = ForwardRaceDistance(m_drivers[driverIndex], m_drivers[i]);
            if (distance > 0.0f && distance < bestDistance)
            {
                bestDistance = distance;
                bestTarget = i;
            }
        }
        return bestTarget;
    }

    int SpaceKartRace::FindTargetBehind(int driverIndex) const
    {
        int bestTarget = -1;
        float bestDistance = 1000.0f;
        for (int i = 0; i < DriverCount; ++i)
        {
            if (i == driverIndex || m_drivers[i].m_finished)
            {
                continue;
            }
            const float distance = ForwardRaceDistance(m_drivers[i], m_drivers[driverIndex]);
            if (distance > 0.0f && distance < bestDistance)
            {
                bestDistance = distance;
                bestTarget = i;
            }
        }
        return bestTarget;
    }

    void SpaceKartRace::ApplyHit(int targetIndex, float slowTime, float spinTime, float lateralImpulse)
    {
        if (targetIndex < 0 || targetIndex >= DriverCount)
        {
            return;
        }

        DriverState& target = m_drivers[targetIndex];
        if (target.m_shieldTime > 0.0f)
        {
            target.m_shieldTime = 0.0f;
            return;
        }

        target.m_slowTime = AZStd::max(target.m_slowTime, slowTime);
        target.m_spinTime = AZStd::max(target.m_spinTime, spinTime);
        target.m_speed *= 0.55f;
        target.m_lateralOffset = Clamp(target.m_lateralOffset + lateralImpulse, -MaxLateralOffset, MaxLateralOffset);
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

    float SpaceKartRace::ForwardRaceDistance(const DriverState& from, const DriverState& to)
    {
        return RaceScore(to) - RaceScore(from);
    }

    AZ::Vector3 SpaceKartRace::GetCenterlinePosition(float progress) const
    {
        const CircuitDefinition& circuit = GetCircuit();
        const float p = WrapProgress(progress);
        const float angle = p * AZ::Constants::TwoPi;
        const float phase = static_cast<float>(m_circuitIndex) * 0.7f;
        return AZ::Vector3(
            std::cos(angle) * circuit.m_radiusX,
            std::sin(angle) * circuit.m_radiusY,
            std::sin(angle * (2.0f + 0.35f * m_circuitIndex) + phase) * circuit.m_heightWave);
    }

    AZ::Vector3 SpaceKartRace::GetTrackPosition(float progress, float lateralOffset) const
    {
        const AZ::Vector3 center = GetCenterlinePosition(progress);
        const AZ::Vector3 tangent = GetTrackTangent(progress);
        const AZ::Vector3 right = tangent.Cross(AZ::Vector3::CreateAxisZ()).GetNormalizedSafe(AZ::Vector3::CreateAxisX());
        return center + right * lateralOffset;
    }

    AZ::Vector3 SpaceKartRace::GetTrackTangent(float progress) const
    {
        constexpr float epsilon = 0.001f;
        const AZ::Vector3 a = GetCenterlinePosition(progress - epsilon);
        const AZ::Vector3 b = GetCenterlinePosition(progress + epsilon);
        return (b - a).GetNormalizedSafe(AZ::Vector3::CreateAxisY());
    }

    AZ::Vector3 SpaceKartRace::GetPlayerPosition() const
    {
        const DriverState& player = m_drivers[0];
        const float jumpHeight = player.m_airTime > 0.0f ? std::sin((player.m_airTime / 0.20f) * AZ::Constants::Pi) * 1.2f : 0.0f;
        return GetTrackPosition(player.m_progress, player.m_lateralOffset) + AZ::Vector3(0.0f, 0.0f, 0.75f + jumpHeight);
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

    RacePhase SpaceKartRace::GetRacePhase() const
    {
        return m_racePhase;
    }

    float SpaceKartRace::GetCountdownTime() const
    {
        return m_countdownTime;
    }

    const char* SpaceKartRace::GetItemName(ItemType item)
    {
        switch (item)
        {
        case ItemType::CometBoost:
            return "Turbo Comete";
        case ItemType::PlasmaShield:
            return "Bouclier Plasma";
        case ItemType::GravityMine:
            return "Mine Gravitationnelle";
        case ItemType::PhotonPulse:
            return "Impulsion Photon";
        case ItemType::None:
        default:
            return "Aucun";
        }
    }

    void SpaceKartRace::DrawWorld(AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        DrawSpaceEnvironment(debugDisplay);
        DrawTrack(debugDisplay);
        DrawItemGates(debugDisplay);
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

        const AZ::Vector3 start = GetTrackPosition(0.0f) + up * 0.48f;
        const AZ::Vector3 startForward = GetTrackTangent(0.0f);
        const AZ::Vector3 startRight = startForward.Cross(up).GetNormalizedSafe(AZ::Vector3::CreateAxisX());
        debugDisplay.SetColor(AZ::Color(1.0f, 1.0f, 1.0f, 1.0f));
        debugDisplay.DrawSolidOBB(start, startRight, startForward, up, AZ::Vector3(circuit.m_trackWidth * 0.45f, 0.35f, 0.08f));
    }

    void SpaceKartRace::DrawItemGates(AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        const AZ::Vector3 up = AZ::Vector3::CreateAxisZ();
        for (int gate = 0; gate < ItemGateCount; ++gate)
        {
            const float progress = ItemGateProgresses[gate];
            const AZ::Vector3 center = GetTrackPosition(progress) + up * 1.0f;
            const AZ::Vector3 tangent = GetTrackTangent(progress);
            const AZ::Vector3 right = tangent.Cross(up).GetNormalizedSafe(AZ::Vector3::CreateAxisX());
            for (int lane = -1; lane <= 1; ++lane)
            {
                const ItemType preview = static_cast<ItemType>(1 + ((gate + lane + 4) % 4));
                debugDisplay.SetColor(ItemColor(preview));
                debugDisplay.DrawBall(center + right * static_cast<float>(lane) * 2.7f, 0.62f);
                debugDisplay.DrawWireSphere(center + right * static_cast<float>(lane) * 2.7f, 0.85f);
            }
        }
    }

    void SpaceKartRace::DrawKart(AzFramework::DebugDisplayRequests& debugDisplay, const DriverState& driver, int driverIndex) const
    {
        const float jumpHeight = driver.m_airTime > 0.0f ? std::sin((driver.m_airTime / 0.20f) * AZ::Constants::Pi) * 1.2f : 0.0f;
        const AZ::Vector3 position = GetTrackPosition(driver.m_progress, driver.m_lateralOffset) + AZ::Vector3(0.0f, 0.0f, 0.8f + jumpHeight);
        const AZ::Vector3 tangent = GetTrackTangent(driver.m_progress);
        const AZ::Transform transform = AZ::Transform::CreateLookAt(position, position + tangent, AZ::Transform::Axis::YPositive);

        debugDisplay.PushMatrix(transform);
        debugDisplay.SetColor(KartBodyColor(driverIndex));
        debugDisplay.DrawSolidBox(AZ::Vector3(-0.86f, -1.18f, -0.30f), AZ::Vector3(0.86f, 1.18f, 0.30f));

        debugDisplay.SetColor(driver.m_color);
        debugDisplay.DrawSolidBox(AZ::Vector3(-0.58f, 0.36f, -0.18f), AZ::Vector3(0.58f, 1.26f, 0.22f));
        debugDisplay.DrawSolidBox(AZ::Vector3(-0.72f, -1.26f, -0.18f), AZ::Vector3(0.72f, -0.88f, 0.18f));

        debugDisplay.SetColor(AZ::Color(0.025f, 0.025f, 0.035f, 1.0f));
        const AZ::Vector3 wheelAxis = AZ::Vector3::CreateAxisX();
        debugDisplay.DrawSolidCylinder(AZ::Vector3(-0.96f, -0.72f, -0.12f), wheelAxis, 0.31f, 0.28f);
        debugDisplay.DrawSolidCylinder(AZ::Vector3(0.96f, -0.72f, -0.12f), wheelAxis, 0.31f, 0.28f);
        debugDisplay.DrawSolidCylinder(AZ::Vector3(-0.96f, 0.74f, -0.12f), wheelAxis, 0.31f, 0.28f);
        debugDisplay.DrawSolidCylinder(AZ::Vector3(0.96f, 0.74f, -0.12f), wheelAxis, 0.31f, 0.28f);

        debugDisplay.SetColor(SuitPrimaryColor(driverIndex));
        debugDisplay.DrawSolidCylinder(AZ::Vector3(0.0f, 0.02f, 1.00f), AZ::Vector3::CreateAxisZ(), 0.34f, 0.75f);
        debugDisplay.SetColor(driver.m_color);
        debugDisplay.DrawSolidCylinder(AZ::Vector3(0.0f, 0.02f, 1.02f), AZ::Vector3::CreateAxisZ(), 0.37f, 0.12f);

        const AZ::Color skinColor = driverIndex == 0
            ? AZ::Color(0.42f, 0.25f, 0.15f, 1.0f)
            : AZ::Color(0.31f, 0.17f, 0.10f, 1.0f);
        debugDisplay.SetColor(skinColor);
        debugDisplay.DrawBall(AZ::Vector3(0.0f, 0.04f, 1.62f), 0.39f);

        if (driverIndex == 1)
        {
            debugDisplay.SetColor(AZ::Color(0.025f, 0.020f, 0.018f, 1.0f));
            debugDisplay.DrawBall(AZ::Vector3(0.0f, 0.02f, 1.94f), 0.37f);
        }
        else if (driverIndex == 2)
        {
            debugDisplay.SetColor(AZ::Color(0.025f, 0.020f, 0.018f, 1.0f));
            debugDisplay.DrawSolidCylinder(AZ::Vector3(0.0f, 0.02f, 1.91f), AZ::Vector3::CreateAxisZ(), 0.34f, 0.12f);
        }

        debugDisplay.SetColor(AZ::Color(0.72f, 0.86f, 1.0f, 1.0f));
        debugDisplay.DrawSolidCylinder(AZ::Vector3(0.0f, 0.72f, 0.88f), AZ::Vector3::CreateAxisX(), 0.08f, 0.75f);

        if (driver.m_boostTime > 0.0f)
        {
            debugDisplay.SetColor(driver.m_color);
            debugDisplay.DrawSolidCone(AZ::Vector3(-0.35f, -1.35f, 0.0f), -AZ::Vector3::CreateAxisY(), 0.28f, 1.6f);
            debugDisplay.DrawSolidCone(AZ::Vector3(0.35f, -1.35f, 0.0f), -AZ::Vector3::CreateAxisY(), 0.28f, 1.6f);
        }

        if (driver.m_shieldTime > 0.0f)
        {
            debugDisplay.SetColor(AZ::Color(0.25f, 0.65f, 1.0f, 0.85f));
            debugDisplay.DrawWireSphere(AZ::Vector3(0.0f, 0.0f, 0.72f), 1.55f);
        }

        if (driver.m_item != ItemType::None)
        {
            debugDisplay.SetColor(ItemColor(driver.m_item));
            debugDisplay.DrawBall(AZ::Vector3(0.0f, -1.05f, 1.55f), 0.18f);
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
        const AZStd::string line1 = AZStd::string::format("%s | %s | Position %d/%d", GetCircuit().m_name, player.m_name.c_str(), player.m_place, DriverCount);
        const AZStd::string line2 = AZStd::string::format("Tour %d/%d | Vitesse %d km/h", player.m_lap, LapCount, static_cast<int>(player.m_speed * 6.0f));
        const AZStd::string line3 = AZStd::string::format("Mini-turbo %.0f%% | Objet: %s", Clamp(player.m_driftCharge / 2.8f, 0.0f, 1.0f) * 100.0f, GetItemName(player.m_item));

        debugDisplay.SetColor(AZ::Color(1.0f, 1.0f, 1.0f, 1.0f));
        debugDisplay.Draw2dTextLabel(26.0f, 30.0f, 1.35f, line1.c_str(), false);
        debugDisplay.Draw2dTextLabel(26.0f, 56.0f, 1.20f, line2.c_str(), false);
        debugDisplay.Draw2dTextLabel(26.0f, 80.0f, 1.00f, line3.c_str(), false);

        debugDisplay.SetColor(AZ::Color(0.65f, 0.90f, 1.0f, 1.0f));
        debugDisplay.Draw2dTextLabel(0.12f, 0.88f, 1.05f, "DIRECTION", true);
        debugDisplay.Draw2dTextLabel(0.87f, 0.18f, 1.05f, "OBJET", true);
        debugDisplay.Draw2dTextLabel(0.87f, 0.57f, 1.05f, "DERAPAGE", true);
        debugDisplay.Draw2dTextLabel(0.87f, 0.88f, 1.05f, "FREIN", true);

        if (m_racePhase == RacePhase::Countdown)
        {
            const int count = AZStd::max(1, static_cast<int>(std::ceil(m_countdownTime)));
            const AZStd::string countdown = m_countdownTime <= 0.15f ? "GO !" : AZStd::string::format("%d", count);
            debugDisplay.SetColor(GetCircuit().m_glowColor);
            debugDisplay.Draw2dTextLabel(0.50f, 0.42f, 3.2f, countdown.c_str(), true);
        }
        else if (m_racePhase == RacePhase::Finished)
        {
            debugDisplay.SetColor(GetCircuit().m_glowColor);
            debugDisplay.Draw2dTextLabel(0.50f, 0.42f, 2.4f, "COURSE TERMINEE", true);
            debugDisplay.Draw2dTextLabel(0.50f, 0.50f, 1.5f, "Circuit suivant: N", true);
        }
    }
}
