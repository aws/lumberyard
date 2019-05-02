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
#include "StdAfx.h"
#include "EditorPreferencesPageGeneral.h"
#include "MainWindow.h"
#include <LyMetricsProducer/LyMetricsAPI.h>

#include <QMessageBox>

#include <Core/QtEditorApplication.h>

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

#define EDITORPREFS_EVENTNAME "EPGEvent"
#define EDITORPREFS_EVENTVALTOGGLE "operation"
#define UNDOSLICESAVE_VALON "UndoSliceSaveValueOn"
#define UNDOSLICESAVE_VALOFF "UndoSliceSaveValueOff"

void CEditorPreferencesPage_General::Reflect(AZ::SerializeContext& serialize)
{
    serialize.Class<GeneralSettings>()
        ->Version(2)
        ->Field("PreviewPanel", &GeneralSettings::m_previewPanel)
        ->Field("TreeBrowserPanel", &GeneralSettings::m_treeBrowserPanel)
        ->Field("ApplyConfigSpec", &GeneralSettings::m_applyConfigSpec)
        ->Field("EnableSourceControl", &GeneralSettings::m_enableSourceControl)
        ->Field("SaveOnlyModified", &GeneralSettings::m_saveOnlyModified)
        ->Field("FreezeReadOnly", &GeneralSettings::m_freezeReadOnly)
        ->Field("FrozenSelectable", &GeneralSettings::m_frozenSelectable)
        ->Field("ConsoleBackgroundColorTheme", &GeneralSettings::m_consoleBackgroundColorTheme)
        ->Field("AutoloadLastLevel", &GeneralSettings::m_autoLoadLastLevel)
        ->Field("ShowTimeInConsole", &GeneralSettings::m_bShowTimeInConsole)
        ->Field("ToolbarIconSize", &GeneralSettings::m_toolbarIconSize)
        ->Field("StylusMode", &GeneralSettings::m_stylusMode)
        ->Field("LayerDoubleClicking", &GeneralSettings::m_bLayerDoubleClicking)
        ->Field("ShowNews", &GeneralSettings::m_bShowNews)
        ->Field("ShowFlowgraphNotification", &GeneralSettings::m_showFlowGraphNotification)
        ->Field("EnableUI20", &GeneralSettings::m_enableUI2)
        ->Field("EnableSceneInspector", &GeneralSettings::m_enableSceneInspector)
		->Field("RestoreViewportCamera", &GeneralSettings::m_restoreViewportCamera)
        ->Field("EnableLegacyUI", &GeneralSettings::m_enableLegacyUI);

    serialize.Class<Messaging>()
        ->Version(2)
        ->Field("ShowDashboard", &Messaging::m_showDashboard)
        ->Field("ShowCircularDependencyError", &Messaging::m_showCircularDependencyError);

    serialize.Class<Undo>()
        ->Version(2)
        ->Field("UndoLevels", &Undo::m_undoLevels)
        ->Field("UndoSliceOverrideSaves", &Undo::m_undoSliceOverrideSaveValue);;

    serialize.Class<DeepSelection>()
        ->Version(2)
        ->Field("DeepSelectionRange", &DeepSelection::m_deepSelectionRange)
        ->Field("StickDuplicate", &DeepSelection::m_stickDuplicate);

    serialize.Class<VertexSnapping>()
        ->Version(1)
        ->Field("VertexCubeSize", &VertexSnapping::m_vertexCubeSize)
        ->Field("RenderPenetratedBoundBox", &VertexSnapping::m_bRenderPenetratedBoundBox);

    serialize.Class<MetricsSettings>()
        ->Version(1)
        ->Field("EnableMetricsTracking", &MetricsSettings::m_enableMetricsTracking);

    serialize.Class<SliceSettings>()
        ->Version(1)
        ->Field("DynamicByDefault", &SliceSettings::m_slicesDynamicByDefault);

    serialize.Class<CEditorPreferencesPage_General>()
        ->Version(1)
        ->Field("General Settings", &CEditorPreferencesPage_General::m_generalSettings)
        ->Field("Messaging", &CEditorPreferencesPage_General::m_messaging)
        ->Field("Undo", &CEditorPreferencesPage_General::m_undo)
        ->Field("Deep Selection", &CEditorPreferencesPage_General::m_deepSelection)
        ->Field("Vertex Snapping", &CEditorPreferencesPage_General::m_vertexSnapping)
        ->Field("Metrics Settings", &CEditorPreferencesPage_General::m_metricsSettings)
        ->Field("Slice Settings", &CEditorPreferencesPage_General::m_sliceSettings);



    AZ::EditContext* editContext = serialize.GetEditContext();
    if (editContext)
    {
        // Check if we should show legacy properties
        AZ::Crc32 shouldShowLegacyItems = AZ::Edit::PropertyVisibility::Hide;
        if (GetIEditor()->IsLegacyUIEnabled())
        {
            shouldShowLegacyItems = AZ::Edit::PropertyVisibility::Show;
        }

        // We don't want to expose the legacy UI toggle if the CryEntityRemoval gem
        // is present
        bool isCryEntityRemovalGemPresent = AzToolsFramework::IsComponentWithServiceRegistered(AZ_CRC("CryEntityRemovalService", 0x229a458c));

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
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_autoLoadLastLevel, "Auto-load last level at startup", "Auto-load last level at startup")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_bShowTimeInConsole, "Show Time In Console", "Show Time In Console")
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GeneralSettings::m_toolbarIconSize, "Toolbar Icon Size", "Toolbar Icon Size")
                ->EnumAttribute(ToolBarIconSize::ToolBarIconSize_16, "16")
                ->EnumAttribute(ToolBarIconSize::ToolBarIconSize_24, "24")
                ->EnumAttribute(ToolBarIconSize::ToolBarIconSize_32, "32")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_stylusMode, "Stylus Mode", "Stylus Mode for tablets and other pointing devices")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_restoreViewportCamera, EditorPreferencesGeneralRestoreViewportCameraSettingName, "Keep the original editor viewport transform when exiting game mode.")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_bLayerDoubleClicking, "Enable Double Clicking in Layer Editor", "Enable Double Clicking in Layer Editor")
                ->Attribute(AZ::Edit::Attributes::Visibility, shouldShowLegacyItems)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_showFlowGraphNotification, "Show FlowGraph Notification", "Display the FlowGraph notification regarding scripting")
                ->Attribute(AZ::Edit::Attributes::Visibility, shouldShowLegacyItems)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_enableUI2, "Enable UI 2.0 (EXPERIMENTAL)", "Enable this to switch the UI to the UI 2.0 styling")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_enableSceneInspector, "Enable Scene Inspector (EXPERIMENTAL)", "Enable the option to inspect the internal data loaded from scene files like .fbx. This is an experimental feature. Restart the Scene Settings if the option is not visible under the Help menu.")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &GeneralSettings::m_enableLegacyUI, "Enable Legacy UI (DEPRECATED)", "Enable the deprecated legacy UI")
                ->Attribute(AZ::Edit::Attributes::Visibility, !isCryEntityRemovalGemPresent);

        editContext->Class<Messaging>("Messaging", "")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Messaging::m_showDashboard, "Show Welcome to Lumberyard at startup", "Show Welcome to Lumberyard at startup")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Messaging::m_showCircularDependencyError, "Show Error: Circular dependency", "Show an error message when adding a slice instance to the target slice would create a cyclic asset dependency. All other valid overrides will be saved even if this is turned off.");

        editContext->Class<Undo>("Undo", "")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &Undo::m_undoLevels, "Undo Levels", "This field specifies the number of undo levels")
            ->Attribute(AZ::Edit::Attributes::Min, 0)
            ->Attribute(AZ::Edit::Attributes::Max, 10000)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Undo::m_undoSliceOverrideSaveValue, "Undo Slice Override Saves", "Allow slice saves to be undone");

        editContext->Class<DeepSelection>("Selection", "")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &DeepSelection::m_stickDuplicate, "Stick duplicate to cursor", "Stick duplicate to cursor")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &DeepSelection::m_deepSelectionRange, "Deep selection range", "Deep Selection Range")
            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 1000.0f);

        editContext->Class<VertexSnapping>("Vertex Snapping", "")
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &VertexSnapping::m_vertexCubeSize, "Vertex Cube Size", "Vertex Cube Size")
            ->Attribute(AZ::Edit::Attributes::Min, 0.0001f)
            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &VertexSnapping::m_bRenderPenetratedBoundBox, "Render Penetrated BoundBoxes", "Render Penetrated BoundBoxes");

        editContext->Class<MetricsSettings>("Metrics", "")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &MetricsSettings::m_enableMetricsTracking, "Enable Metrics Tracking", "Enable Metrics Tracking");

        editContext->Class<SliceSettings>("Slices", "")
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &SliceSettings::m_slicesDynamicByDefault, "New Slices Dynamic By Default", "When creating slices, they will be set to dynamic by default");

        editContext->Class<CEditorPreferencesPage_General>("General Editor Preferences", "Class for handling General Editor Preferences")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_General::m_generalSettings, "General Settings", "General Editor Preferences")
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_General::m_messaging, "Messaging", "Messaging")
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_General::m_undo, "Undo", "Undo Preferences")
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_General::m_deepSelection, "Selection", "Selection")
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_General::m_vertexSnapping, "Vertex Snapping", "Vertex Snapping")
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_General::m_metricsSettings, "Metrics", "Metrics Settings")
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_General::m_sliceSettings, "Slices", "Slice Settings");
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
    gSettings.bShowDashboardAtStartup = m_messaging.m_showDashboard;
    gSettings.m_showCircularDependencyError = m_messaging.m_showCircularDependencyError;
    gSettings.bAutoloadLastLevelAtStartup = m_generalSettings.m_autoLoadLastLevel;
    gSettings.stylusMode = m_generalSettings.m_stylusMode;
    gSettings.restoreViewportCamera = m_generalSettings.m_restoreViewportCamera;
    gSettings.showFlowgraphNotification = m_generalSettings.m_showFlowGraphNotification;
    gSettings.enableSceneInspector = m_generalSettings.m_enableSceneInspector;
    gSettings.enableLegacyUI = m_generalSettings.m_enableLegacyUI;

    gSettings.bEnableUI2 = m_generalSettings.m_enableUI2;
    Editor::EditorQtApplication::instance()->EnableUI2(gSettings.bEnableUI2);

    if (m_generalSettings.m_toolbarIconSize != gSettings.gui.nToolbarIconSize)
    {
        gSettings.gui.nToolbarIconSize = m_generalSettings.m_toolbarIconSize;
        MainWindow::instance()->AdjustToolBarIconSize();
    }

    //undo
    gSettings.undoLevels = m_undo.m_undoLevels;

    if (gSettings.m_undoSliceOverrideSaveValue != m_undo.m_undoSliceOverrideSaveValue)
    {
        if (m_undo.m_undoSliceOverrideSaveValue)
        {
            LyMetrics_SendEvent(EDITORPREFS_EVENTNAME, { { EDITORPREFS_EVENTVALTOGGLE, UNDOSLICESAVE_VALON } });
        }
        else
        {
            LyMetrics_SendEvent(EDITORPREFS_EVENTNAME, { { EDITORPREFS_EVENTVALTOGGLE, UNDOSLICESAVE_VALOFF } });
        }
    }
    gSettings.m_undoSliceOverrideSaveValue = m_undo.m_undoSliceOverrideSaveValue;

    //deep selection
    gSettings.deepSelectionSettings.fRange = m_deepSelection.m_deepSelectionRange;
    gSettings.deepSelectionSettings.bStickDuplicate = m_deepSelection.m_stickDuplicate;

    //vertex snapping
    gSettings.vertexSnappingSettings.vertexCubeSize = m_vertexSnapping.m_vertexCubeSize;
    gSettings.vertexSnappingSettings.bRenderPenetratedBoundBox = m_vertexSnapping.m_bRenderPenetratedBoundBox;

    //metrics
    if (gSettings.sMetricsSettings.bEnableMetricsTracking != m_metricsSettings.m_enableMetricsTracking)
    {
        gSettings.sMetricsSettings.bEnableMetricsTracking = m_metricsSettings.m_enableMetricsTracking;
        LyMetrics_OnOptOutStatusChange(gSettings.sMetricsSettings.bEnableMetricsTracking);
    }

    //slices
    gSettings.sliceSettings.dynamicByDefault = m_sliceSettings.m_slicesDynamicByDefault;

    // If the legacy UI toggle was changed, inform the user they need to restart
    // the Editor in order for the change to take effect
    if (gSettings.enableLegacyUI != m_generalSettings.m_enableLegacyUIInitialValue)
    {
        QMessageBox::warning(AzToolsFramework::GetActiveWindow(), QObject::tr("Restart required"), QObject::tr("You must restart the Editor in order for your Legacy UI change to take effect."));
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
    m_generalSettings.m_autoLoadLastLevel = gSettings.bAutoloadLastLevelAtStartup;
    m_generalSettings.m_stylusMode = gSettings.stylusMode;
    m_generalSettings.m_restoreViewportCamera = gSettings.restoreViewportCamera;
    m_generalSettings.m_showFlowGraphNotification = gSettings.showFlowgraphNotification;
    m_generalSettings.m_enableUI2 = gSettings.bEnableUI2;
    m_generalSettings.m_enableSceneInspector = gSettings.enableSceneInspector;
    m_generalSettings.m_enableLegacyUI = gSettings.enableLegacyUI;
    m_generalSettings.m_enableLegacyUIInitialValue = gSettings.enableLegacyUI;

    m_generalSettings.m_toolbarIconSize = gSettings.gui.nToolbarIconSize;

    //Messaging
    m_messaging.m_showDashboard = gSettings.bShowDashboardAtStartup;
    m_messaging.m_showCircularDependencyError = gSettings.m_showCircularDependencyError;

    //undo
    m_undo.m_undoLevels = gSettings.undoLevels;
    m_undo.m_undoSliceOverrideSaveValue = gSettings.m_undoSliceOverrideSaveValue;

    //deep selection
    m_deepSelection.m_deepSelectionRange = gSettings.deepSelectionSettings.fRange;
    m_deepSelection.m_stickDuplicate = gSettings.deepSelectionSettings.bStickDuplicate;

    //vertex snapping
    m_vertexSnapping.m_vertexCubeSize = gSettings.vertexSnappingSettings.vertexCubeSize;
    m_vertexSnapping.m_bRenderPenetratedBoundBox = gSettings.vertexSnappingSettings.bRenderPenetratedBoundBox;

    //metrics
    m_metricsSettings.m_enableMetricsTracking = gSettings.sMetricsSettings.bEnableMetricsTracking;

    //slices
    m_sliceSettings.m_slicesDynamicByDefault = gSettings.sliceSettings.dynamicByDefault;
}