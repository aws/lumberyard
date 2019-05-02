/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Color.h>
#include <AzFramework/Physics/Collision.h>
#include <AzFramework/Physics/World.h>

namespace PhysX
{
    /// PhysX specific settings.
    class Settings
    {
    public:
        AZ_CLASS_ALLOCATOR(Settings, AZ::SystemAllocator, 0);
        AZ_RTTI(Settings, "{BA958D76-E323-4F94-86E2-004E63957CB6}");
        virtual ~Settings() = default;

        enum class PvdTransportType
        {
            Network,
            File
        };

        enum class PvdAutoConnectMode
        {
            Disabled, ///< Auto connection is disabled (default).
            Editor, ///< Auto connection takes place on editor launch and remains open until closed.
            Game, ///< Auto connection for game mode.
            Server ///<  Auto connection from the server.
        };

        AZStd::string m_pvdHost = "127.0.0.1"; ///< PhysX Visual Debugger hostname.
        int m_pvdPort = 5425; ///< PhysX Visual Debugger port (default: 5425).
        unsigned int m_pvdTimeoutInMilliseconds = 10; ///< Timeout used when connecting to PhysX Visual Debugger.
        PvdTransportType m_pvdTransportType = PvdTransportType::Network; ///< PhysX Visual Debugger transport preference.
        AZStd::string m_pvdFileName = "physxDebugInfo.pxd2"; ///< PhysX Visual Debugger output filename.
        PvdAutoConnectMode m_pvdAutoConnectMode = PvdAutoConnectMode::Disabled; ///< PVD auto connect preference.
        bool m_pvdReconnect = true; ///< Reconnect when switching between game and edit mode automatically (Editor mode only).

        struct ColliderProximityVisualization
        {
            AZ_TYPE_INFO(ColliderProximityVisualization, "{2A9BA0AE-C6A7-4F87-B7F0-D62444035478}");
            bool m_enabled = false; ///< If camera proximity based collider visulization is currently active.
            AZ::Vector3 m_cameraPosition = AZ::Vector3::CreateZero(); ///< Camera position to perform proximity based collider visulization around.
            float m_radius = 1.0f; ///< The radius to visualize colliders around the camera position.
        };

        /// Collider proximity based visualization preferences.
        ColliderProximityVisualization m_colliderProximityVisualization;

        /// Determine if the current debug type preference is the network (for the editor context).
        inline bool IsNetworkDebug() const { return m_pvdTransportType == PvdTransportType::Network; }

        /// Determine if the current debug type preference is file output (for the editor context).
        inline bool IsFileDebug() const { return m_pvdTransportType == PvdTransportType::File; }

        /// Determine if auto connection is Disabled.
        inline bool IsAutoConnectionDisabled() const { return m_pvdAutoConnectMode == PvdAutoConnectMode::Disabled; }

        /// Determine if auto connection is enabled for Editor mode.
        inline bool IsAutoConnectionEditorMode() const { return m_pvdAutoConnectMode == PvdAutoConnectMode::Editor; }

        /// Determine if auto connection is enabled for Game mode.
        inline bool IsAutoConnectionGameMode() const { return m_pvdAutoConnectMode == PvdAutoConnectMode::Game; }

        /// Determine if auto connection is enabled for Server mode.
        inline bool IsAutoConnectionServerMode() const{ return m_pvdAutoConnectMode == PvdAutoConnectMode::Server; }
    };

    /// PhysX editor settings.
    class EditorConfiguration
    {
    public:

        AZ_CLASS_ALLOCATOR(EditorConfiguration, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(EditorConfiguration, "{016890FC-F1EB-478B-822E-1A40977ECBBB}");

        /// Center of Mass Debug Draw Circle Size
        float m_centerOfMassDebugSize = 0.1f;

        /// Center of Mass Debug Draw Circle Color
        AZ::Color m_centerOfMassDebugColor = AZ::Color((AZ::u8)255, (AZ::u8)0, (AZ::u8)0, (AZ::u8)255);
    };

    /// Configuration structure for PhysX.
    /// Used to initialise the PhysX Gem.
    class Configuration
    {
    public:
        AZ_CLASS_ALLOCATOR(Configuration, AZ::SystemAllocator, 0);
        AZ_RTTI(Configuration, "{9C342C95-3E27-437C-9C15-FEE651C824DD}");
        virtual ~Configuration() = default;

        Settings m_settings; ///< PhysX specific settings.
        Physics::WorldConfiguration m_worldConfiguration; ///< Default world configuration.
        Physics::CollisionLayers m_collisionLayers; ///< Collision layers defined in the project.
        Physics::CollisionGroups m_collisionGroups; ///< Collision groups defined in the project.
        EditorConfiguration m_editorConfiguration; ///< Editor configuration for PhysX.
    };

    /// Configuration requests.
    class ConfigurationRequests
        : public AZ::EBusTraits
    {
    public:

        virtual ~ConfigurationRequests() {}

        /// Gets the PhysX configuration.
        virtual const Configuration& GetConfiguration() = 0;

        /// Sets the PhysX configuration.
        virtual void SetConfiguration(const Configuration& configuration) = 0;
    };

    using ConfigurationRequestBus = AZ::EBus<ConfigurationRequests>;

    /// Configuration notifications.
    class ConfigurationNotifications
        : public AZ::EBusTraits
    {
    public:
        /// Raised when the PhysX configuration has been loaded.
        virtual void OnConfigurationLoaded() {};

        /// Raised when the PhysX configuration has been refreshed.
        virtual void OnConfigurationRefreshed(const Configuration&) {};

    protected:
        ~ConfigurationNotifications() = default;
    };

    using ConfigurationNotificationBus = AZ::EBus<ConfigurationNotifications>;
}
