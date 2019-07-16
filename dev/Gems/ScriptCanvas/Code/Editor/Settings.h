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

#include <AzCore/Math/Uuid.h>
#include <AzCore/UserSettings/UserSettings.h>

#include <QByteArray>
#include <QMainWindow>

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvasEditor
{
    namespace EditorSettings
    {
        class WindowSavedState
            : public AZ::UserSettings
        {
        public:
            AZ_RTTI(WindowSavedState, "{67DACC4D-B92C-4B5A-8884-6AF7C7B74246}", AZ::UserSettings);
            AZ_CLASS_ALLOCATOR(WindowSavedState, AZ::SystemAllocator, 0);

            WindowSavedState() = default;

            void Init(const QByteArray& windowState, const QByteArray& windowGeometry);
            void Restore(QMainWindow* window);

            static void Reflect(AZ::ReflectContext* context);

        private:

            const AZStd::vector<AZ::u8>& GetWindowState();

            AZStd::vector<AZ::u8> m_storedWindowState;
            AZStd::vector<AZ::u8> m_windowGeometry;
            AZStd::vector<AZ::u8> m_windowState;

        };

        // Structure used for Toggleable Configurations
        // i.e. something that has a configuration time and the ability to turn it on/off
        class ToggleableConfiguration
        {
        public:
            AZ_RTTI(ToggleableConfiguration, "{24E8CAE7-0B5E-4B5E-94CC-08B9148B4AB5}");
            AZ_CLASS_ALLOCATOR(ToggleableConfiguration, AZ::SystemAllocator, 0);

            ToggleableConfiguration()
                : ToggleableConfiguration(false, 1000)
            {

            }

            ToggleableConfiguration(bool enabled, int timeMS)
                : m_enabled(enabled)
                , m_timeMS(timeMS)
            {

            }

            virtual ~ToggleableConfiguration() = default;

            bool m_enabled;
            int m_timeMS;
        };

        class ShakeToDespliceSettings
        {
            friend class ScriptCanvasEditorSettings;
        public:
            AZ_RTTI(ShakeToDespliceSettings, "{6401FA20-7A17-407E-81E3-D1389C9C70B7}");
            AZ_CLASS_ALLOCATOR(ShakeToDespliceSettings, AZ::SystemAllocator, 0);

            ShakeToDespliceSettings()
                : m_enabled(true)
                , m_shakeCount(3)
                , m_maximumShakeTimeMS(1000)
                , m_minimumShakeLengthPercent(3)
                , m_deadZonePercent(1)
                , m_straightnessPercent(65)
            {
            }

            virtual ~ShakeToDespliceSettings() = default;

            float GetStraightnessPercent() const
            {
                return m_straightnessPercent * 0.01f;
            }

            float GetMinimumShakeLengthPercent() const
            {
                return m_minimumShakeLengthPercent * 0.01f;
            }

            float GetDeadZonePercent() const
            {
                return m_deadZonePercent * 0.01f;
            }

            bool m_enabled;

            int m_shakeCount;
            int m_maximumShakeTimeMS;

        private:

            float m_minimumShakeLengthPercent;
            float m_deadZonePercent;
            
            float m_straightnessPercent;
        };

        class EdgePanningSettings
        {
            friend class ScriptCanvasEditorSettings;
        public:
            AZ_RTTI(EdgePanningSettings, "{38399A9B-8D4B-4198-AAA2-D1E8761F5563}");
            AZ_CLASS_ALLOCATOR(EdgePanningSettings, AZ::SystemAllocator, 0);

            EdgePanningSettings()
                : m_edgeScrollPercent(5.0f)
                , m_edgeScrollSpeed(75.0f)
            {
            }

            virtual ~EdgePanningSettings() = default;

            float GetEdgeScrollPercent() const
            {
                return m_edgeScrollPercent * 0.01f;
            }

            float GetEdgeScrollSpeed() const
            {
                return m_edgeScrollSpeed;
            }

        private:

            float m_edgeScrollPercent;
            float m_edgeScrollSpeed;
        };

        class ScriptCanvasEditorSettings
            : public AZ::UserSettings
        {
        public:
            AZ_RTTI(ScriptCanvasEditorSettings, "{D8D5453C-BFB8-4C71-BBAF-0F10FDD69B3F}", AZ::UserSettings);
            AZ_CLASS_ALLOCATOR(ScriptCanvasEditorSettings, AZ::SystemAllocator, 0);

            static void Reflect(AZ::ReflectContext* context);
            static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

            ScriptCanvasEditorSettings();

            double m_snapDistance;

            bool m_showPreviewMessage;
            bool m_showExcludedNodes; //! During preview we're excluding some behavior context nodes that may not work perfectly in Script Canvas.

            bool m_allowBookmarkViewpointControl;
            bool m_allowNodeNudgingOnSplice;

            ToggleableConfiguration m_dragNodeCouplingConfig;
            ToggleableConfiguration m_dragNodeSplicingConfig;

            ToggleableConfiguration m_dropNodeSplicingConfig;  

            ToggleableConfiguration m_autoSaveConfig;

            ShakeToDespliceSettings m_shakeDespliceConfig;

            EdgePanningSettings     m_edgePanningSettings;

            AZStd::unordered_set<AZ::Uuid> m_pinnedDataTypes;

            int m_variablePanelSorting;

            bool m_showValidationWarnings;
            bool m_showValidationErrors;

            AZ::u32 m_alignmentTimeMS;
        };
    }
}