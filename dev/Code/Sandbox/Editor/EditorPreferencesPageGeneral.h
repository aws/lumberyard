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

#include "Include/IPreferencesPage.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Math/Vector3.h>

class CEditorPreferencesPage_General
    : public IPreferencesPage
{
public:
    AZ_RTTI(CEditorPreferencesPage_General, "{9CFBBE85-560D-4720-A830-50EF25D06ED5}", IPreferencesPage)

    static void Reflect(AZ::SerializeContext& serialize);

    CEditorPreferencesPage_General();
    virtual ~CEditorPreferencesPage_General() = default;

    virtual const char* GetCategory() override { return "General Settings"; }
    virtual const char* GetTitle() override { return "General"; }
    virtual void OnApply() override;
    virtual void OnCancel() override {}
    virtual bool OnQueryCancel() override { return true; }

    enum class ToolBarIconSize
    {
        ToolBarIconSize_16 = 16,
        ToolBarIconSize_24 = 24,
        ToolBarIconSize_32 = 32
    };

private:
    void InitializeSettings();

    struct GeneralSettings
    {
        AZ_TYPE_INFO(GeneralSettings, "{C2AE8F6D-7AA6-499E-A3E8-ECCD0AC6F3D2}")

        bool m_previewPanel;
        bool m_treeBrowserPanel;
        bool m_applyConfigSpec;
        bool m_enableSourceControl;
        bool m_saveOnlyModified;
        bool m_freezeReadOnly;
        bool m_frozenSelectable;
        SEditorSettings::ConsoleColorTheme m_consoleBackgroundColorTheme;
        bool m_showDashboard;
        bool m_autoLoadLastLevel;
        bool m_bShowTimeInConsole;
        int m_toolbarIconSize;
        bool m_stylusMode;
        bool m_restoreViewportCamera;
        bool m_bLayerDoubleClicking;
        bool m_bShowNews;
        bool m_showFlowGraphNotification;
        bool m_enableSceneInspector;
        bool m_enableUI2;
        bool m_enableLegacyUI;

        // Only used to tell if the user has changed this value since it requires a restart
        bool m_enableLegacyUIInitialValue;
    };

    struct Messaging
    {
        AZ_TYPE_INFO(Messaging, "{A6AD87CB-E905-409B-A2BF-C43CDCE63B0C}")

        bool m_showCircularDependencyError;
    };

    struct Undo
    {
        AZ_TYPE_INFO(Undo, "{A3AC0728-F132-4BF2-B122-8A631B636E81}")

        int m_undoLevels;
        bool m_undoSliceOverrideSaveValue;
    };

    struct DeepSelection
    {
        AZ_TYPE_INFO(DeepSelection, "{225616BF-66DE-41EC-9FDD-F5A104112547}")

        float m_deepSelectionRange;
    };

    struct VertexSnapping
    {
        AZ_TYPE_INFO(VertexSnapping, "{20F16350-990C-4096-86E3-40D56DDDD702}")

        float m_vertexCubeSize;
        bool m_bRenderPenetratedBoundBox;
    };

    struct MetricsSettings
    {
        AZ_TYPE_INFO(MetricsSettings, "{18134A8F-8940-47C4-B129-D874AA829EA8}")

        bool m_enableMetricsTracking;
    };

    GeneralSettings m_generalSettings;
    Messaging m_messaging;
    Undo m_undo;
    DeepSelection m_deepSelection;
    VertexSnapping m_vertexSnapping;
    MetricsSettings m_metricsSettings;
};

static const char* EditorPreferencesGeneralRestoreViewportCameraSettingName = "Restore Viewport Camera on Game Mode Exit";

