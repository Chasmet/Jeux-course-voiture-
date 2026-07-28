#pragma once

#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/string/string.h>

namespace AzFramework
{
    class DebugDisplayRequests;
}

namespace SpaceKartLegends
{
    enum class RacePhase : AZ::u8
    {
        Countdown,
        Racing,
        Finished
    };

    enum class ItemType : AZ::u8
    {
        None,
        CometBoost,
        PlasmaShield,
        GravityMine,
        PhotonPulse
    };

    struct CircuitDefinition
    {
        const char* m_name;
        float m_radiusX;
        float m_radiusY;
        float m_heightWave;
        float m_trackWidth;
        float m_speedFactor;
        AZ::Color m_trackColor;
        AZ::Color m_glowColor;
    };

    struct DriverState
    {
        AZStd::string m_name;
        AZ::Color m_color;
        float m_progress = 0.0f;
        float m_lateralOffset = 0.0f;
        float m_speed = 0.0f;
        float m_boostTime = 0.0f;
        float m_driftCharge = 0.0f;
        float m_airTime = 0.0f;
        float m_shieldTime = 0.0f;
        float m_slowTime = 0.0f;
        float m_spinTime = 0.0f;
        float m_aiItemDecisionTime = 0.0f;
        int m_lap = 1;
        int m_place = 1;
        int m_nextPickupIndex = 0;
        ItemType m_item = ItemType::None;
        bool m_finished = false;
    };

    class SpaceKartRace
    {
    public:
        static constexpr int DriverCount = 4;
        static constexpr int CircuitCount = 5;
        static constexpr int LapCount = 3;

        SpaceKartRace();

        void Reset(int circuitIndex = 0);
        void Update(float deltaTime);

        void SetSteering(float steering);
        void SetAccelerating(bool accelerating);
        void SetBraking(bool braking);
        void SetDrifting(bool drifting);
        void UseBoost();
        void UseItem();
        void RecoverPlayer();
        void SelectNextCircuit();

        void SelectPilot(int direction)
        {
            m_selectedPilotIndex = (m_selectedPilotIndex + direction) % DriverCount;
            if (m_selectedPilotIndex < 0)
            {
                m_selectedPilotIndex += DriverCount;
            }

            const AZStd::array<const char*, DriverCount> names = {"Cheikh", "Yvane", "Nelvyn", "Nova"};
            const AZStd::array<AZ::Color, DriverCount> colors = {
                AZ::Color(0.10f, 0.55f, 1.0f, 1.0f),
                AZ::Color(1.0f, 0.72f, 0.05f, 1.0f),
                AZ::Color(0.15f, 0.95f, 0.42f, 1.0f),
                AZ::Color(0.95f, 0.25f, 0.95f, 1.0f)
            };

            for (int slot = 0; slot < DriverCount; ++slot)
            {
                const int profile = (m_selectedPilotIndex + slot) % DriverCount;
                m_drivers[slot].m_name = names[profile];
                m_drivers[slot].m_color = colors[profile];
            }
            Reset(m_circuitIndex);
        }

        int GetSelectedPilotIndex() const
        {
            return m_selectedPilotIndex;
        }

        const char* GetSelectedPilotName() const
        {
            return m_drivers[0].m_name.c_str();
        }

        // Repositions the next pickup target after an item has been consumed.
        // This allows another item to be collected later in the same lap.
        void RefreshPickupTargets()
        {
            for (DriverState& driver : m_drivers)
            {
                if (driver.m_finished || driver.m_item != ItemType::None)
                {
                    continue;
                }

                if (driver.m_progress < 0.16f)
                {
                    driver.m_nextPickupIndex = 0;
                }
                else if (driver.m_progress < 0.41f)
                {
                    driver.m_nextPickupIndex = 1;
                }
                else if (driver.m_progress < 0.66f)
                {
                    driver.m_nextPickupIndex = 2;
                }
                else if (driver.m_progress < 0.91f)
                {
                    driver.m_nextPickupIndex = 3;
                }
                else
                {
                    driver.m_nextPickupIndex = 0;
                }
            }
        }

        AZ::Vector3 GetTrackPosition(float progress, float lateralOffset = 0.0f) const;
        AZ::Vector3 GetTrackTangent(float progress) const;
        AZ::Vector3 GetPlayerPosition() const;
        AZ::Vector3 GetPlayerTangent() const;

        const DriverState& GetPlayer() const;
        const CircuitDefinition& GetCircuit() const;
        int GetCircuitIndex() const;
        RacePhase GetRacePhase() const;
        float GetCountdownTime() const;
        static const char* GetItemName(ItemType item);

        void DrawWorld(AzFramework::DebugDisplayRequests& debugDisplay) const;
        void DrawHud(AzFramework::DebugDisplayRequests& debugDisplay) const;

    private:
        void UpdatePlayer(float deltaTime);
        void UpdateAi(float deltaTime);
        void UpdateDriverTimers(DriverState& driver, float deltaTime);
        void UpdateRanking();
        void CheckItemPickup(DriverState& driver, int driverIndex, float previousProgress);
        ItemType SelectPickupItem(const DriverState& driver, int driverIndex) const;
        void UseDriverItem(int driverIndex);
        int FindTargetAhead(int driverIndex) const;
        int FindTargetBehind(int driverIndex) const;
        void ApplyHit(int targetIndex, float slowTime, float spinTime, float lateralImpulse);

        void DrawTrack(AzFramework::DebugDisplayRequests& debugDisplay) const;
        void DrawItemGates(AzFramework::DebugDisplayRequests& debugDisplay) const;
        void DrawKart(AzFramework::DebugDisplayRequests& debugDisplay, const DriverState& driver, int driverIndex) const;
        void DrawSpaceEnvironment(AzFramework::DebugDisplayRequests& debugDisplay) const;

        AZ::Vector3 GetCenterlinePosition(float progress) const;
        static float WrapProgress(float progress);
        static float RaceScore(const DriverState& driver);
        static float ForwardRaceDistance(const DriverState& from, const DriverState& to);

        AZStd::array<CircuitDefinition, CircuitCount> m_circuits;
        AZStd::array<DriverState, DriverCount> m_drivers;
        int m_circuitIndex = 0;
        int m_selectedPilotIndex = 0;
        float m_steering = 0.0f;
        bool m_accelerating = true;
        bool m_braking = false;
        bool m_drifting = false;
        float m_elapsedTime = 0.0f;
        float m_countdownTime = 3.5f;
        RacePhase m_racePhase = RacePhase::Countdown;
    };
}
