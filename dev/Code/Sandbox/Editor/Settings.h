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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once
////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   settings.h
//  Version:     v1.00
//  Created:     14/1/2003 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: General editor settings.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef CRYINCLUDE_EDITOR_SETTINGS_H
#define CRYINCLUDE_EDITOR_SETTINGS_H
#include "SettingsManager.h"

#include <QColor>
#include <QFont>
#include <QRect>
#include <QSettings>

class CGrid;

struct SGizmoSettings
{
    float axisGizmoSize;
    bool axisGizmoText;
    int axisGizmoMaxCount;

    float helpersScale;
    float tagpointScaleMulti;

    // Scale size and transparency for debug spheres when using Ruler tool
    float rulerSphereScale;
    float rulerSphereTrans;

    SGizmoSettings();
};

//////////////////////////////////////////////////////////////////////////
// Settings for snapping in the viewports.
//////////////////////////////////////////////////////////////////////////
struct SSnapSettings
{
    SSnapSettings()
    {
        constructPlaneDisplay = false;
        constructPlaneSize = 5;

        markerDisplay = false;
        markerColor = QColor(0, 200, 200);
        markerSize = 1.0f;

        bGridUserDefined = false;
        bGridGetFromSelected = false;
    }

    // Display settings for construction plane.
    bool  constructPlaneDisplay;
    float constructPlaneSize;

    // Display settings for snapping marker.
    bool  markerDisplay;
    QColor markerColor;
    float markerSize;

    bool bGridUserDefined;
    bool bGridGetFromSelected;
};


//////////////////////////////////////////////////////////////////////////
// Settings for View of Tools
//////////////////////////////////////////////////////////////////////////
struct SToolViewSettings
{
    SToolViewSettings()
    {
    }

    int nFrameRate;
    QString codec;
};

//////////////////////////////////////////////////////////////////////////
// Settings for deep selection.
//////////////////////////////////////////////////////////////////////////
struct SDeepSelectionSettings
{
    SDeepSelectionSettings()
        : fRange(1.f)
        , bStickDuplicate(false) {}

    //! If there are other objects hit within this value, one of them needs
    //! to be selected by user.
    //! If this value is 0.f, then deep selection mode won't work.
    float fRange;
    bool bStickDuplicate;
};

//////////////////////////////////////////////////////////////////////////
// Settings for vertex snapping.
//////////////////////////////////////////////////////////////////////////
struct SVertexSnappingSettings
{
    SVertexSnappingSettings()
        : vertexCubeSize(0.01f)
        , bRenderPenetratedBoundBox(false) {}
    float vertexCubeSize;
    bool bRenderPenetratedBoundBox;
};

//////////////////////////////////////////////////////////////////////////
struct SObjectColors
{
    SObjectColors()
    {
        prefabHighlight = QColor(214, 172, 5);
        groupHighlight = QColor(0, 255, 0);
        entityHighlight = QColor(112, 117, 102);
        fBBoxAlpha = 0.3f;
        geometryHighlightColor = QColor(192, 0, 192);
        solidBrushGeometryColor = QColor(192, 0, 192);
        fGeomAlpha = 0.2f;
        fChildGeomAlpha = 0.4f;
    }

    QColor prefabHighlight;
    QColor groupHighlight;
    QColor entityHighlight;
    QColor geometryHighlightColor;
    QColor solidBrushGeometryColor;

    float fBBoxAlpha;
    float fGeomAlpha;
    float fChildGeomAlpha;
};

//////////////////////////////////////////////////////////////////////////
struct SHyperGraphColors
{
    SHyperGraphColors()
    {
        opacity = 75.0f;
        colorArrow = QColor(25, 25, 25);
        colorInArrowHighlighted = QColor(255, 0, 0);
        colorOutArrowHighlighted = QColor(0, 161, 222);
        colorPortEdgeHighlighted = QColor(0, 161, 222);
        colorArrowDisabled = QColor(50, 50, 50);
        colorNodeOutline = QColor(60, 60, 60);
        colorNodeOutlineSelected = QColor(0, 161, 222);
        colorNodeBkg = QColor(60, 60, 60);
        colorNodeSelected = QColor(0, 161, 222);
        colorTitleText = QColor(0, 0, 0);
        colorTitleTextSelected = QColor(0, 0, 0);
        colorText = QColor(191, 191, 191);
        colorBackground = QColor(86, 86, 86);
        colorGrid = QColor(80, 80, 80);
        colorBreakPoint = QColor(255, 0, 0);
        colorBreakPointDisabled = QColor(255, 0, 0);
        colorBreakPointArrow = QColor(255, 255, 0);
        colorEntityPortNotConnected = QColor(255, 90, 90);
        colorPort = QColor(80, 80, 80);
        colorPortSelected = QColor(0, 161, 222);
        colorEntityTextInvalid = QColor(255, 255, 255);
        colorDownArrow = QColor(25, 25, 25);
        colorCustomNodeBkg = QColor(170, 170, 170);
        colorCustomSelectedNodeBkg = QColor(225, 255, 245);
        colorPortDebugging = QColor(228, 202, 66);
        colorPortDebuggingText = QColor(0, 0, 0);
        colorQuickSearchBackground = QColor(60, 60, 60);
        colorQuickSearchResultText = QColor(0, 161, 222);
        colorQuickSearchCountText = QColor(255, 255, 255);
        colorQuickSearchBorder = QColor(0, 0, 0);
        colorDebugNodeTitle = QColor(220, 180, 20);
        colorDebugNode = QColor(190, 60, 60);
    }

    float opacity;
    QColor colorArrow;
    QColor colorInArrowHighlighted;
    QColor colorOutArrowHighlighted;
    QColor colorPortEdgeHighlighted;
    QColor colorArrowDisabled;
    QColor colorNodeOutline;
    QColor colorNodeOutlineSelected;
    QColor colorNodeBkg;
    QColor colorNodeSelected;
    QColor colorTitleTextSelected;
    QColor colorTitleText;
    QColor colorText;
    QColor colorBackground;
    QColor colorGrid;
    QColor colorBreakPoint;
    QColor colorBreakPointDisabled;
    QColor colorBreakPointArrow;
    QColor colorEntityPortNotConnected;
    QColor colorPort;
    QColor colorPortSelected;
    QColor colorEntityTextInvalid;
    QColor colorDownArrow;
    QColor colorCustomNodeBkg;
    QColor colorCustomSelectedNodeBkg;
    QColor colorPortDebugging;
    QColor colorPortDebuggingText;
    QColor colorQuickSearchBackground;
    QColor colorQuickSearchResultText;
    QColor colorQuickSearchCountText;
    QColor colorQuickSearchBorder;
    QColor colorDebugNodeTitle;
    QColor colorDebugNode;
};

//////////////////////////////////////////////////////////////////////////
struct SViewportsSettings
{
    //! If enabled always show entity radiuse.
    bool bAlwaysShowRadiuses;
    //! If enabled always display boxes for prefab entity.
    bool bAlwaysDrawPrefabBox;
    //! If enabled always display objects inside prefab.
    bool bAlwaysDrawPrefabInternalObjects;
    //! True if 2D viewports will be synchronized with same view and origin.
    bool bSync2DViews;
    //! Camera FOV for perspective View.
    float fDefaultFov;
    //! Camera Aspect Ratio for perspective View.
    float fDefaultAspectRatio;
    //! Show safe frame.
    bool bShowSafeFrame;
    //! To highlight selected geometry.
    bool bHighlightSelectedGeometry;
    //! To highlight selected vegetation.
    bool bHighlightSelectedVegetation;

    //! To highlight selected geometry.
    int bHighlightMouseOverGeometry;

    //! Show triangle count on mouse over
    bool bShowMeshStatsOnMouseOver;

    //! If enabled will always display entity labels.
    bool bDrawEntityLabels;
    //! Show Trigger bounds.
    bool bShowTriggerBounds;
    //! Show Icons in viewport.
    bool bShowIcons;
    //! Scale icons with distance, so they aren't a fixed size no matter how far away you are
    bool bDistanceScaleIcons;

    //! Show Size-based Icons in viewport.
    bool bShowSizeBasedIcons;

    //! Show Helpers in viewport for frozen objects.
    int nShowFrozenHelpers;
    //! Fill Selected Shapes.
    bool bFillSelectedShapes;

    // Swap X/Y in map viewport.
    bool bTopMapSwapXY;
    // Texture resolution in the Top map viewport.
    int nTopMapTextureResolution;

    // Whether the grid guide (for move/rotate tool) will be shown or not.
    bool bShowGridGuide;

    // Whether the mouse cursor will be hidden when capturing the mouse.
    bool bHideMouseCursorWhenCaptured;

    // Size of square which the mouse must leave before a drag operation begins.
    int nDragSquareSize;

    // Enable context menu in the viewport
    bool bEnableContextMenu;

    //! Warning icons draw distance
    float fWarningIconsDrawDistance;
    //! Warnings for scale != 1
    bool bShowScaleWarnings;
    //! Warnings for rotation != 0
    bool bShowRotationWarnings;
};

struct SSelectObjectDialogSettings
{
    QString columns;
    int nLastColumnSortDirection;

    SSelectObjectDialogSettings()
        : nLastColumnSortDirection(0)
    {
    }
};

//////////////////////////////////////////////////////////////////////////
struct SGUI_Settings
{
    bool bWindowsVista;        // true when running on windows Vista
    QFont hSystemFont;         // Default system GUI font.
    QFont hSystemFontBold;     // Default system GUI bold font.
    QFont hSystemFontItalic;   // Default system GUI italic font.
    int nDefaultFontHieght;    // Default font height for 8 logical units.

    int nToolbarIconSize;      // Override size of the toolbar icons
};

//////////////////////////////////////////////////////////////////////////
struct STextureBrowserSettings
{
    int nCellSize; // Stores the default texture maximum cell size for
};

//////////////////////////////////////////////////////////////////////////
struct SExperimentalFeaturesSettings
{
    bool bTotalIlluminationEnabled;
};

//////////////////////////////////////////////////////////////////////////
struct SMetricsSettings
{
    bool bEnableMetricsTracking;
};

//////////////////////////////////////////////////////////////////////////
struct SSliceSettings
{
    bool dynamicByDefault;
};

//////////////////////////////////////////////////////////////////////////
struct SAssetBrowserSettings
{
    // stores the default thumb size
    int nThumbSize;
    // the current filename search filter in the search edit box
    QString sFilenameSearch,
    // the current filter preset name
            sPresetName;
    // current selected/checked/visible databases, db names separated by comma (,), Ex: "Textures,Models,Sounds"
    QString sVisibleDatabaseNames;
    // current visible columns in asset list
    QString sVisibleColumnNames;
    // current columns in asset list, in their actual order (including visible and hidden columns)
    QString sColumnNames;
    // check to show only the assets used in current level
    bool bShowUsedInLevel,
    // check to show only the assets loaded in the current level
         bShowLoadedInLevel,
    // check to filter only the favorite assets
         bShowFavorites,
    // hide LODs from thumbs view, for CGFs for example
         bHideLods,
    // true when the edited filter preset was changed, it will be saved automatically, without the user to push the "Save" button
         bAutoSaveFilterPreset,
    // true when we want sync between asset browser selection and viewport selection
         bAutoChangeViewportSelection,
    // true when we want sync between viewport selection and asset browser visible thumbs
         bAutoFilterFromViewportSelection;
};

//////////////////////////////////////////////////////////////////////////

const int kMannequinTrackSizeDefault = 32;
const int kMannequinTrackSizeMin         = 14;
const int kMannequinTrackSizeMax         = 32;

struct SMannequinSettings
{
    int                                 trackSize;
    bool                                bCtrlForScrubSnapping;
    float                               timelineWheelZoomSpeed;
};

struct SSmartOpenDialogSettings
{
    QRect rect;
    QString lastSearchTerm;
};

//////////////////////////////////////////////////////////////////////////
/** Various editor settings.
*/
struct SANDBOX_API SEditorSettings
{
    SEditorSettings();
    void    Save();
    void    Load();
    void    LoadCloudSettings();

    // needs to be called after crysystem has been loaded
    void    LoadDefaultGamePaths();

    // need to expose updating of the source control enable/disable flag
    // because its state is updateable through the main status bar
    void SaveEnableSourceControlFlag(bool triggerUpdate = false);

    void LoadEnableSourceControlFlag();

    void PostInitApply();

    bool BrowseTerrainTexture(bool bIsSave);

    //////////////////////////////////////////////////////////////////////////
    // Variables.
    //////////////////////////////////////////////////////////////////////////
    int undoLevels;
    bool m_undoSliceOverrideSaveValue;
    bool bShowDashboardAtStartup;
    bool m_showCircularDependencyError;
    bool bAutoloadLastLevelAtStartup;
    bool bMuteAudio;
    bool bEnableGameModeVR;

    //! Speed of camera movement.
    float cameraMoveSpeed;
    float cameraRotateSpeed;
    float cameraFastMoveSpeed;
    float wheelZoomSpeed;
    bool invertYRotation;
    bool invertPan;
    bool stylusMode; // if stylus mode is enabled, no setCursorPos will be performed (WACOM tablets, etc)
    bool restoreViewportCamera; // When true, restore the original editor viewport camera when exiting game mode

    //! Hide mask for objects.
    int objectHideMask;

    //! Selection mask for objects.
    int objectSelectMask;

    //////////////////////////////////////////////////////////////////////////
    // Viewport settings.
    //////////////////////////////////////////////////////////////////////////
    SViewportsSettings viewports;

    SToolViewSettings toolViewSettings;

    //////////////////////////////////////////////////////////////////////////
    // Files.
    //////////////////////////////////////////////////////////////////////////

    //! Before saving, make a backup into a subfolder
    bool bBackupOnSave;
    //! how many save backups to keep
    int backupOnSaveMaxCount;

    int useLowercasePaths;

    //////////////////////////////////////////////////////////////////////////
    // Autobackup.
    //////////////////////////////////////////////////////////////////////////
    //! Save auto backup file every autoSaveTime minutes.
    int autoBackupTime;
    //! Keep only several last autobackups.
    int autoBackupMaxCount;
    //! When this variable set to true automatic file backup is enabled.
    bool autoBackupEnabled;
    //! After this amount of minutes message box with reminder to save will pop on.
    int autoRemindTime;
    //////////////////////////////////////////////////////////////////////////


    //! If true preview windows is displayed when browsing geometries.
    bool bPreviewGeometryWindow;
    //! If true display geometry browser window for brushes and simple entities.
    bool bGeometryBrowserPanel;

    int showErrorDialogOnLoad;

    //! Keeps the editor active even if no focus is set
    int keepEditorActive;

    //! Pointer to currently used grid.
    CGrid* pGrid;

    SGizmoSettings gizmo;

    // Settings of the snapping.
    SSnapSettings snap;

    //! Source Control Enabling.
    bool enableSourceControl;

    //! External layers: Save only changed.
    bool saveOnlyModified;

    //! Freeze read-only layers on Load Level time.
    bool freezeReadOnly;

    //! Frozen layers are selectable.
    bool frozenSelectable;

    //! Text editor.
    QString textEditorForScript;
    QString textEditorForShaders;
    QString textEditorForBspaces;

    //! Asset editors
    QString textureEditor;
    QString animEditor;

    //////////////////////////////////////////////////////////////////////////
    //! Editor data search paths.
    QStringList searchPaths[10]; // EDITOR_PATH_LAST

    // This directory is related to the editor root.
    QString strStandardTempDirectory;
    QString strEditorEnv;

    SGUI_Settings gui;

    SHyperGraphColors hyperGraphColors;
    bool              bFlowGraphMigrationEnabled;
    bool              bFlowGraphShowNodeIDs;
    bool              bFlowGraphShowToolTip;
    bool              bFlowGraphEdgesOnTopOfNodes;
    bool                            bFlowGraphHighlightEdges;
    bool              bApplyConfigSpecInEditor;

    ESystemConfigSpec editorConfigSpec;

    //! Terrain Texture Export/Import filename.
    QString terrainTextureExport;

    // Read only parameter.
    // Refects the status of GetIEditor()->GetOperationMode
    // To change current operation mode use GetIEditor()->SetOperationMode
    // see EOperationMode
    int operationMode;

    // For the texture browser configurations.
    STextureBrowserSettings sTextureBrowserSettings;

    // Experimental features configurations.
    SExperimentalFeaturesSettings sExperimentalFeaturesSettings;

    // For the asset browser configurations.
    SAssetBrowserSettings sAssetBrowserSettings;

    SSelectObjectDialogSettings selectObjectDialog;

    // For Terrain Texture Generation Multiplier.
    float fBrMultiplier;

    // For Console window background
    enum class ConsoleColorTheme
    {
        Light,
        Dark
    };
    ConsoleColorTheme consoleBackgroundColorTheme;

    bool bShowTimeInConsole;

    // Enable Double Clicking in Layer Editor
    bool bLayerDoubleClicking;

    // FlowGraph scripting direction notification.
    bool showFlowgraphNotification;

    // Enable the option do get detailed information about the loaded scene data in the Scene Settings window.
    bool enableSceneInspector;

    // Enable the legacy UI (deprecated)
    bool enableLegacyUI;

    // Deep Selection Mode Settings
    SDeepSelectionSettings deepSelectionSettings;

    // Object Highlight Settings
    SObjectColors objectColorSettings;

    // Vertex Snapping Settings
    SVertexSnappingSettings vertexSnappingSettings;

    // Mannequin Settings
    SMannequinSettings mannequinSettings;
    SSmartOpenDialogSettings smartOpenSettings;

    bool bSettingsManagerMode;

    bool bForceSkyUpdate;

    bool bAutoSaveTagPoints;

    bool bNavigationContinuousUpdate;
    bool bNavigationShowAreas;
    bool bNavigationDebugDisplay;
    bool bVisualizeNavigationAccessibility;
    int  navigationDebugAgentType;

    bool bIsSearchFilterActive;
    int backgroundUpdatePeriod;
    const char* g_TemporaryLevelName;

    SMetricsSettings sMetricsSettings;

    SSliceSettings sliceSettings;

    bool bEnableUI2;

private:
    void SaveValue(const char* sSection, const char* sKey, int value);
    void SaveValue(const char* sSection, const char* sKey, const QColor& value);
    void SaveValue(const char* sSection, const char* sKey, float value);
    void SaveValue(const char* sSection, const char* sKey, const QString& value);

    void LoadValue(const char* sSection, const char* sKey, int& value);
    void LoadValue(const char* sSection, const char* sKey, QColor& value);
    void LoadValue(const char* sSection, const char* sKey, float& value);
    void LoadValue(const char* sSection, const char* sKey, bool& value);
    void LoadValue(const char* sSection, const char* sKey, QString& value);
    void LoadValue(const char* sSection, const char* sKey, ESystemConfigSpec& value);

    void SaveCloudSettings();
};

//! Single instance of editor settings for fast access.
SANDBOX_API extern SEditorSettings gSettings;

#endif // CRYINCLUDE_EDITOR_SETTINGS_H
