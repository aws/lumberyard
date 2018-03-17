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
#include "stdafx.h"
#include "EditorPreferencesPageGeneral.h"
#include "MainWindow.h"
#include <LyMetricsProducer/LyMetricsAPI.h>

#include <Core/QtEditorApplication.h>

void CEditorPreferencesPage_General::Reflect(AZ::SerializeContext& serialize)
{
    serialize.Class<GeneralSettings>()
        ->Version(1)
        ->Field("PreviewPanel", &GeneralSettings::m_previewPanel)
        ->Field("TreeBrowserPanel", &GeneralSettings::m_treeBrowserPanel)
        ->Field("ApplyConfigSpec", &GeneralSettings::m_applyConfigSpec)
        ->Field("EnableSourceControl", &GeneralSettings::m_enableSourceControl)
        ->Field("SaveOnlyModified", &GeneralSettings::m_saveOnlyModified)
        ->Field("FreezeReadOnly", &GeneralSettings::m_freezeReadOnly)
        ->Field("FrozenSelectable", &GeneralSettings::m_frozenSelectable)
        ->Field("ConsoleBackgroundColorTheme", &GeneralSettings::m_consoleBackgroundColorTheme)
        ->Field("ShowDashboard", &GeneralSettings::m_showDashboard)
        ->Field("AutoloadLastLevel", &GeneralSettings::m_autoLoadLastLevel)
        ->Field("ShowTimeInConsole", &GeneralSettings::m_bShowTimeInConsole)
        ->Field("ToolbarIconSize", &GeneralSettings::m_toolbarIconSize)
        ->Field("StylusMode", &GeneralSettings::m_stylusMode)
        ->Field("LayerDoubleClicking", &GeneralSettings::m_bLayerDoubleClicking)
        ->Field("ShowNews", &GeneralSettings::m_bShowNews)
        ->Field("ShowFlowgraphNotification", &GeneralSettings::m_showFlowGraphNotification)
        ->Field("EnableUI20", &GeneralSettings::m_enableUI2)
        ->Field("EnableSceneInspector", &GeneralSettings::m_enableSceneInspector);

    serialize.Class<Undo>()
        ->Version(1)
        ->Field("UndoLevels", &Undo::m_undoLevels);

    serialize.Class<DeepSelection>()
        ->Version(1)
        ->Field("DeepSelectionRange", &DeepSelection::m_deepSelectionRange);

    serialize.Class<VertexSnapping>()
        ->Version(1)
        ->Field("VertexCubeSize", &VertexSnapping::m_vertexCubeSize)
        ->Field("RenderPenetratedBoundBox", &VertexSnapping::m_bRenderPenetratedBoundBox);

    serialize.Class<MetricsSettings>()
        ->Version(1)
        ->Field("EnableMetricsTracking", &MetricsSettings::m_enableMetricsTracking);

    serialize.Class<CEditorPreferencesPage_General>()
        ->Version(1)
        ->Field("General Settings", &CEditorPreferencesPage_General::m_generalSettings)
        ->Field("Undo", &CEditorPreferencesPage_General::m_undo)
        ->Field("Deep Selection", &CEditorPreferencesPage_General::m_deepSelection)
        ->Field("Vertex Snapping", &CEditorPreferencesPage_General::m_vertexSnapping)
        ->Field("Metrics Settings", &CEditorPreferencesPage_General::m_metricsSettings);



    AZ::EditContext* editContext = serialize.GetEditContext();
    if (editContext)
    {
        // Check if we should show legacy properties
        AZ::Crc32 shouldShowLegacyItems = AZ::Edit::PropertyVisibility::Hide;
        if (GetIEditor()->IsLegacyUIEnabled())
        {
            shouldShowLegacyItems = AZ::Edit::PropertyVisibility::Show;
        }

        editContext->Class<GeneralSettings>("General Settings", "General Editor Preferences")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_previewPanel, "Show Geometry Preview Panel", "Show Geometry Preview Panel")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_treeBrowserPanel, "Show Geometry Tree Browser Panel", "Show Geometry Tree Browser Panel")
                ->Attribute(AZ::Edit::Attributes::Visibility, shouldShowLegacyItems)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_applyConfigSpec, "Hide objects by config spec", "Hide objects by config spec")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_enableSourceControl, "Enable Source Control", "Enable Source Control")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_saveOnlyModified, "External Layers: Save only Modified", "External Layers: Save only Modified")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_freezeReadOnly, "Freeze Read-only external layer on Load", "Freeze Read-only external layer on Load")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_frozenSelectable, "Frozen layers are selectable", "Frozen layers are selectable")
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GeneralSettings::m_consoleBackgroundColorTheme, "Console Background", "Console Background")
                ->EnumAttribute(SEditorSettings::ConsoleColorTheme::Light, "Light")
                ->EnumAttribute(SEditorSettings::ConsoleColorTheme::Dark, "Dark")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_showDashboard, "Show Welcome to Lumberyard at startup", "Show Welcome to Lumberyard at startup")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_autoLoadLastLevel, "Auto-load last level at startup", "Auto-load last level at startup")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_bShowTimeInConsole, "Show Time In Console", "Show Time In Console")
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GeneralSettings::m_toolbarIconSize, "Toolbar Icon Size", "Toolbar Icon Size")
                ->EnumAttribute(ToolBarIconSize::ToolBarIconSize_16, "16")
                ->EnumAttribute(ToolBarIconSize::ToolBarIconSize_24, "24")
                ->EnumAttribute(ToolBarIconSize::ToolBarIconSize_32, "32")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_stylusMode, "Stylus Mode", "Stylus Mode for tablets and other pointing devices")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_bLayerDoubleClicking, "Enable Double Clicking in Layer Editor", "Enable Double Clicking in Layer Editor")
                ->Attribute(AZ::Edit::Attributes::Visibility, shouldShowLegacyItems)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_showFlowGraphNotification, "Show FlowGraph Notification", "Display the FlowGraph notification regarding scripting")
                ->Attribute(AZ::Edit::Attributes::Visibility, shouldShowLegacyItems)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_enableUI2, "Enable UI 2.0 (EXPERIMENTAL)", "Enable this to switch the UI to the UI 2.0 styling")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_enableSceneInspector, "Enable Scene Inspector (EXPERIMENTAL)", "Enable the option to inspect the internal data loaded from scene files like .fbx. This is an experimental feature. Restart the Scene Settings if the option is not visible under the Help menu.");

        editContext->Class<Undo>("Undo", "")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &Undo::m_undoLevels, "Undo Levels", "This field specifies the number of undo levels")
            ->Attribute(AZ::Edit::Attributes::Min, 0)
            ->Attribute(AZ::Edit::Attributes::Max, 10000);

        editContext->Class<DeepSelection>("Deep Selection", "")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &DeepSelection::m_deepSelectionRange, "Range", "Deep Selection Range")
            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 1000.0f);

        editContext->Class<VertexSnapping>("Vertex Snapping", "")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &VertexSnapping::m_vertexCubeSize, "Vertex Cube Size", "Vertex Cube Size")
            ->Attribute(AZ::Edit::Attributes::Min, 0.0001f)
            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &VertexSnapping::m_bRenderPenetratedBoundBox, "Render Penetrated BoundBoxes", "Render Penetrated BoundBoxes");

        editContext->Class<MetricsSettings>("Metrics", "")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &MetricsSettings::m_enableMetricsTracking, "Enable Metrics Tracking", "Enable Metrics Tracking");

        editContext->Class<CEditorPreferencesPage_General>("General Editor Preferences", "Class for handling General Editor Preferences")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_General::m_generalSettings, "General Settings", "General Editor Preferences")
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_General::m_undo, "Undo", "Undo Preferences")
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_General::m_deepSelection, "Deep Selection", "Deep Selection")
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_General::m_vertexSnapping, "Vertex Snapping", "Vertex Snapping")
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_General::m_metricsSettings, "Metrics", "Metrics Settings");
    }
}

CEditorPreferencesPage_General::CEditorPreferencesPage_General()
{
    InitializeSettings();
}

void CEditorPreferencesPage_General::OnApply()
{
    //general settings
    gSettings.bPreviewGeometryWindow = m_generalSettings.m_previewPanel;
    gSettings.bGeometryBrowserPanel = m_generalSettings.m_treeBrowserPanel;
    gSettings.bApplyConfigSpecInEditor = m_generalSettings.m_applyConfigSpec;
    gSettings.enableSourceControl = m_generalSettings.m_enableSourceControl;
    gSettings.saveOnlyModified = m_generalSettings.m_saveOnlyModified;
    gSettings.freezeReadOnly = m_generalSettings.m_freezeReadOnly;
    gSettings.frozenSelectable = m_generalSettings.m_frozenSelectable;
    gSettings.consoleBackgroundColorTheme = m_generalSettings.m_consoleBackgroundColorTheme;
    gSettings.bShowTimeInConsole = m_generalSettings.m_bShowTimeInConsole;
    gSettings.bLayerDoubleClicking = m_generalSettings.m_bLayerDoubleClicking;
    gSettings.bShowDashboardAtStartup = m_generalSettings.m_showDashboard;
    gSettings.bAutoloadLastLevelAtStartup = m_generalSettings.m_autoLoadLastLevel;
    gSettings.stylusMode = m_generalSettings.m_stylusMode;
    gSettings.showFlowgraphNotification = m_generalSettings.m_showFlowGraphNotification;
    gSettings.enableSceneInspector = m_generalSettings.m_enableSceneInspector;

    gSettings.bEnableUI2 = m_generalSettings.m_enableUI2;
    Editor::EditorQtApplication::instance()->EnableUI2(gSettings.bEnableUI2);

    if (m_generalSettings.m_toolbarIconSize != gSettings.gui.nToolbarIconSize)
    {
        gSettings.gui.nToolbarIconSize = m_generalSettings.m_toolbarIconSize;
        MainWindow::instance()->AdjustToolBarIconSize();
    }

    //undo
    gSettings.undoLevels = m_undo.m_undoLevels;

    //deep selection
    gSettings.deepSelectionSettings.fRange = m_deepSelection.m_deepSelectionRange;

    //vertex snapping
    gSettings.vertexSnappingSettings.vertexCubeSize = m_vertexSnapping.m_vertexCubeSize;
    gSettings.vertexSnappingSettings.bRenderPenetratedBoundBox = m_vertexSnapping.m_bRenderPenetratedBoundBox;

    //metrics
    if (gSettings.sMetricsSettings.bEnableMetricsTracking != m_metricsSettings.m_enableMetricsTracking)
    {
        gSettings.sMetricsSettings.bEnableMetricsTracking = m_metricsSettings.m_enableMetricsTracking;
        LyMetrics_OnOptOutStatusChange(gSettings.sMetricsSettings.bEnableMetricsTracking);
    }
}

void CEditorPreferencesPage_General::InitializeSettings()
{
    //general settings
    m_generalSettings.m_previewPanel = gSettings.bPreviewGeometryWindow;
    m_generalSettings.m_treeBrowserPanel = gSettings.bGeometryBrowserPanel;
    m_generalSettings.m_applyConfigSpec = gSettings.bApplyConfigSpecInEditor;
    m_generalSettings.m_enableSourceControl = gSettings.enableSourceControl;
    m_generalSettings.m_saveOnlyModified = gSettings.saveOnlyModified;
    m_generalSettings.m_freezeReadOnly = gSettings.freezeReadOnly;
    m_generalSettings.m_frozenSelectable = gSettings.frozenSelectable;
    m_generalSettings.m_consoleBackgroundColorTheme = gSettings.consoleBackgroundColorTheme;
    m_generalSettings.m_bShowTimeInConsole = gSettings.bShowTimeInConsole;
    m_generalSettings.m_bLayerDoubleClicking = gSettings.bLayerDoubleClicking;
    m_generalSettings.m_showDashboard = gSettings.bShowDashboardAtStartup;
    m_generalSettings.m_autoLoadLastLevel = gSettings.bAutoloadLastLevelAtStartup;
    m_generalSettings.m_stylusMode = gSettings.stylusMode;
    m_generalSettings.m_showFlowGraphNotification = gSettings.showFlowgraphNotification;
    m_generalSettings.m_enableUI2 = gSettings.bEnableUI2;
    m_generalSettings.m_enableSceneInspector = gSettings.enableSceneInspector;

    m_generalSettings.m_toolbarIconSize = gSettings.gui.nToolbarIconSize;

    //undo
    m_undo.m_undoLevels = gSettings.undoLevels;

    //deep selection
    m_deepSelection.m_deepSelectionRange = gSettings.deepSelectionSettings.fRange;

    //vertex snapping
    m_vertexSnapping.m_vertexCubeSize = gSettings.vertexSnappingSettings.vertexCubeSize;
    m_vertexSnapping.m_bRenderPenetratedBoundBox = gSettings.vertexSnappingSettings.bRenderPenetratedBoundBox;

    //metrics
    m_metricsSettings.m_enableMetricsTracking = gSettings.sMetricsSettings.bEnableMetricsTracking;
}