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
#include <AzFramework/Physics/Material.h>
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

        /// Enable Global Collision Debug Draw
        enum class GlobalCollisionDebugState
        {
            AlwaysOn,         ///< Collision draw debug all entities.
            AlwaysOff,        ///< Collision debug draw disabled.
            Manual            ///< Set up in the entity.
        };
        GlobalCollisionDebugState m_globalCollisionDebugDraw = GlobalCollisionDebugState::Manual;

        /// Color scheme for debug collision
        enum class GlobalCollisionDebugColorMode
        {
            MaterialColor,   ///< Use debug color specified in the material library
            ErrorColor       ///< Show default color and flashing red for colliders with errors.
        };
        GlobalCollisionDebugColorMode m_globalCollisionDebugDrawColorMode = GlobalCollisionDebugColorMode::MaterialColor;

        /// Colors for joint lead
        enum class JointLeadColor
        {
            Aquamarine,
            AliceBlue,
            CadetBlue,
            Coral,
            Green,
            DarkGreen,
            ForestGreen,
            Honeydew
        };

        /// Colors for joint follower
        enum class JointFollowerColor
        {
            Yellow,
            Chocolate,
            HotPink,
            Lavender,
            Magenta,
            LightYellow,
            Maroon,
            Red
        };

        AZ::Color GetJointLeadColor()
        {
            switch (m_jointHierarchyLeadColor)
            {
            case JointLeadColor::Aquamarine:
                return AZ::Colors::Aquamarine;
            case JointLeadColor::AliceBlue:
                return AZ::Colors::AliceBlue;
            case JointLeadColor::CadetBlue:
                return AZ::Colors::CadetBlue;
            case JointLeadColor::Coral:
                return AZ::Colors::Coral;
            case JointLeadColor::Green:
                return AZ::Colors::Green;
            case JointLeadColor::DarkGreen:
                return AZ::Colors::DarkGreen;
            case JointLeadColor::ForestGreen:
                return AZ::Colors::ForestGreen;
            case JointLeadColor::Honeydew:
                return AZ::Colors::Honeydew;
            default:
                return AZ::Colors::Aquamarine;
            }
        }

        AZ::Color GetJointFollowerColor()
        {
            switch (m_jointHierarchyFollowerColor)
            {
            case JointFollowerColor::Chocolate:
                return AZ::Colors::Chocolate;
            case JointFollowerColor::HotPink:
                return AZ::Colors::HotPink;
            case JointFollowerColor::Lavender:
                return AZ::Colors::Lavender;
            case JointFollowerColor::Magenta:
                return AZ::Colors::Magenta;
            case JointFollowerColor::LightYellow:
                return AZ::Colors::LightYellow;
            case JointFollowerColor::Maroon:
                return AZ::Colors::Maroon;
            case JointFollowerColor::Red:
                return AZ::Colors::Red;
            case JointFollowerColor::Yellow:
                return AZ::Colors::Yellow;
            default:
                return AZ::Colors::Magenta;
            }
        }

        bool m_showJointHierarchy = true; ///< Flag to switch on/off the display of joints' lead-follower connections in the viewport.
        JointLeadColor m_jointHierarchyLeadColor = JointLeadColor::Aquamarine; ///< Color of the lead half of a lead-follower joint connection line.
        JointFollowerColor m_jointHierarchyFollowerColor = JointFollowerColor::Magenta; ///< Color of the follower half of a lead-follower joint connection line.
        float m_jointHierarchyDistanceThreshold = 1.0f; ///< Minimum distance required to draw from follower to joint. Distances shorter than this threshold will result in the line drawn from the joint to the lead.
    };

    /// PhysX wind settings.
    class WindConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(WindConfiguration, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(WindConfiguration, "{6EA3E646-ECDA-4044-912D-5722D5100066}");

        /// Tag value that will be used to identify entities that provide global wind value.
        /// Global wind has no bounds and affects objects across entire level.
        AZStd::string m_globalWindTag = "global_wind";
        /// Tag value that will be used to identify entities that provide local wind value.
        /// Local wind is only applied within bounds defined by PhysX collider.
        AZStd::string m_localWindTag = "wind";
    };

    /// Configuration structure for PhysX.
    /// Used to initialize the PhysX Gem.
    class PhysXConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(PhysXConfiguration, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(PhysXConfiguration, "{5E050D96-04E0-4863-8F4F-1E2E2B567022}");

        Settings m_settings; ///< PhysX specific settings.
        EditorConfiguration m_editorConfiguration; ///< Editor configuration for PhysX.
        WindConfiguration m_windConfiguration; ///< Wind configuration for PhysX.
    };

    /// Configuration requests bus traits. Singleton pattern.
    class ConfigurationRequestsTraits
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };

    /// Configuration requests.
    class ConfigurationRequests
    {
    public:
        AZ_TYPE_INFO(ConfigurationRequests, "{5FEC38FF-CA8D-4071-AA78-AC07549D4387}");

        ConfigurationRequests() = default;        

        // AZ::Interface requires these to be deleted.
        ConfigurationRequests(ConfigurationRequests&&) = delete;
        ConfigurationRequests& operator=(ConfigurationRequests&&) = delete;

        virtual const PhysXConfiguration& GetPhysXConfiguration() = 0;
        virtual void SetPhysXConfiguration(const PhysXConfiguration& configuration) = 0;

    protected:
        virtual ~ConfigurationRequests() = default;
    };

    using ConfigurationRequestBus = AZ::EBus<ConfigurationRequests, ConfigurationRequestsTraits>;

    /// Configuration notifications.
    class ConfigurationNotifications
        : public AZ::EBusTraits
    {
    public:
        /// Raised when the PhysX configuration has been loaded.
        virtual void OnPhysXConfigurationLoaded() {};
        /// Raised when the PhysX configuration has been refreshed.
        virtual void OnPhysXConfigurationRefreshed(const PhysXConfiguration&) {};
        /// Raised when the default material library has changed
        virtual void OnDefaultMaterialLibraryChanged(const AZ::Data::AssetId&) {}

    protected:
        ~ConfigurationNotifications() = default;
    };

    using ConfigurationNotificationBus = AZ::EBus<ConfigurationNotifications>;
}
