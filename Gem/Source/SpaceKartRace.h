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
        int m_lap = 1;
        int m_place = 1;
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
        void SetDrifting(bool drifting);
        void UseBoost();
        void RecoverPlayer();
        void SelectNextCircuit();

        AZ::Vector3 GetTrackPosition(float progress, float lateralOffset = 0.0f) const;
        AZ::Vector3 GetTrackTangent(float progress) const;
        AZ::Vector3 GetPlayerPosition() const;
        AZ::Vector3 GetPlayerTangent() const;

        const DriverState& GetPlayer() const;
        const CircuitDefinition& GetCircuit() const;
        int GetCircuitIndex() const;

        void DrawWorld(AzFramework::DebugDisplayRequests& debugDisplay) const;
        void DrawHud(AzFramework::DebugDisplayRequests& debugDisplay) const;

    private:
        void UpdatePlayer(float deltaTime);
        void UpdateAi(float deltaTime);
        void UpdateRanking();
        void DrawTrack(AzFramework::DebugDisplayRequests& debugDisplay) const;
        void DrawKart(AzFramework::DebugDisplayRequests& debugDisplay, const DriverState& driver, int driverIndex) const;
        void DrawSpaceEnvironment(AzFramework::DebugDisplayRequests& debugDisplay) const;

        static float WrapProgress(float progress);
        static float RaceScore(const DriverState& driver);

        AZStd::array<CircuitDefinition, CircuitCount> m_circuits;
        AZStd::array<DriverState, DriverCount> m_drivers;
        int m_circuitIndex = 0;
        float m_steering = 0.0f;
        bool m_drifting = false;
        float m_elapsedTime = 0.0f;
    };
}
