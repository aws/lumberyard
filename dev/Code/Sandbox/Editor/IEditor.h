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

#ifndef CRYINCLUDE_EDITOR_IEDITOR_H
#define CRYINCLUDE_EDITOR_IEDITOR_H
#pragma once

#ifdef PLUGIN_EXPORTS
    #define PLUGIN_API DLL_EXPORT
#else
    #define PLUGIN_API DLL_IMPORT
#endif

// disable virtual function override warnings, as MFC headers do not pass them.
#pragma warning(disable: 4264)
#pragma warning(disable: 4266)

#include <ISystem.h>
#include <functor.h>
#include "Include/SandboxAPI.h"
#include "Util/UndoUtil.h"
#include <CryVersion.h>

#include <WinWidget/WinWidgetId.h>

#include <AzCore/Component/EntityId.h>

class QMenu;

struct CRuntimeClass;
class CBaseLibraryDialog;
struct QtViewPane;
class QMainWindow;
struct QMetaObject;

class CBaseObject;
class CCryEditDoc;
class CSelectionGroup;
class CEditTool;
class CAnimationContext;
class CTrackViewSequenceManager;
class CGameEngine;
struct IIconManager;
class CToolBoxManager;
class CClassFactory;
class CEntityPrototypeManager;
class CMaterialManager;
class CMusicManager;
class CMaterail;
class CEntityPrototype;
struct IEditorParticleManager;
class CPrefabManager;
class CLensFlareManager;
class CEAXPresetManager;
class CErrorReport;
class CBaseLibraryItem;
class CBaseLibraryDialog;
class ICommandManager;
class CEditorCommandManager;
class CHyperGraphManager;
class CConsoleSynchronization;
class CUIEnumsDatabase;
struct ISourceControl;
struct IEditorClassFactory;
struct IDataBaseItem;
struct ITransformManipulator;
struct IDataBaseManager;
class IFacialEditor;
class CDialog;
#if defined(AZ_PLATFORM_WINDOWS)
class C3DConnexionDriver;
#endif
class CRuler;
class CSettingsManager;
struct IExportManager;
class CDisplaySettings;
struct SGizmoParameters;
class CLevelIndependentFileMan;
class CMissingAssetResolver;
class CSelectionTreeManager;
struct IResourceSelectorHost;
struct SEditorSettings;
class CGameExporter;
class IAWSResourceManager;

namespace WinWidget
{
    class WinWidgetManager;
}

struct ISystem;
struct IGame;
struct I3DEngine;
struct IRenderer;
struct AABB;
struct IEventLoopHook;
struct IErrorReport; // Vladimir@conffx
struct IMissingAssetResolver;  // Vladimir@conffx
struct IFileUtil;  // Vladimir@conffx
struct IEditorLog;  // Vladimir@conffx
struct IAssetBrowser;  // Vladimir@conffx
struct IEditorMaterialManager;  // Vladimir@conffx
struct IBaseLibraryManager;  // Vladimir@conffx
struct IImageUtil;  // Vladimir@conffx
struct IEditorParticleUtils;  // Leroy@conffx
struct ILogFile; // Vladimir@conffx

// Qt/QML

#ifdef DEPRECATED_QML_SUPPORT
class QQmlEngine;
#endif // #ifdef DEPRECATED_QML_SUPPORT

class QWidget;
class QMimeData;
class QString;
class QStringList;
class QColor;
class QPixmap;

#if !AZ_TRAIT_OS_PLATFORM_APPLE && !defined(AZ_PLATFORM_LINUX)
typedef void* HANDLE;
struct HWND__;
typedef HWND__* HWND;
#endif

namespace Editor
{
    class EditorQtApplication;
}


// Global editor notify events.
enum EEditorNotifyEvent
{
    // Global events.
    eNotify_OnInit = 10,               // Sent after editor fully initialized.
    eNotify_OnQuit,                    // Sent before editor quits.
    eNotify_OnIdleUpdate,              // Sent every frame while editor is idle.

    // Document events.
    eNotify_OnBeginNewScene,           // Sent when the document is begin to be cleared.
    eNotify_OnEndNewScene,             // Sent after the document have been cleared.
    eNotify_OnBeginSceneOpen,          // Sent when document is about to be opened.
    eNotify_OnEndSceneOpen,            // Sent after document have been opened.
    eNotify_OnBeginSceneSave,          // Sent when document is about to be saved.
    eNotify_OnEndSceneSave,            // Sent after document have been saved.
    eNotify_OnBeginLayerExport,        // Sent when a layer is about to be exported.
    eNotify_OnEndLayerExport,          // Sent after a layer have been exported.
    eNotify_OnCloseScene,              // Send when the document is about to close.
    eNotify_OnSceneClosed,             // Send when the document is closed.
    eNotify_OnMissionChange,           // Send when the current mission changes.
    eNotify_OnBeginLoad,               // Sent when the document is start to load.
    eNotify_OnEndLoad,                 // Sent when the document loading is finished
    eNotify_OnBeginExportToGame,       // Sent when the level starts to be exported to game
    eNotify_OnExportToGame,            // Sent when the level is exported to game

    // Editing events.
    eNotify_OnEditModeChange,          // Sent when editing mode change (move,rotate,scale,....)
    eNotify_OnEditToolChange,          // Sent when edit tool is changed (ObjectMode,TerrainModify,....)

    // Deferred terrain create event.
    eNotify_OnBeginTerrainCreate,      // Sent when terrain is created later (and not during level creation)
    eNotify_OnEndTerrainCreate,        // Sent when terrain is created later (and not during level creation)

    // Game related events.
    eNotify_OnBeginGameMode,           // Sent when editor goes to game mode.
    eNotify_OnEndGameMode,             // Sent when editor goes out of game mode.
    eNotify_Deprecated0,               // formerly eNotify_OnEnableFlowSystemUpdate
    eNotify_Deprecated1,               // formerly eNotify_OnDisableFlowSystemUpdate

    // AI/Physics simulation related events.
    eNotify_OnBeginSimulationMode,     // Sent when simulation mode is started.
    eNotify_OnEndSimulationMode,       // Sent when editor goes out of simulation mode.

    // UI events.
    eNotify_OnUpdateViewports,         // Sent when editor needs to update data in the viewports.
    eNotify_OnReloadTrackView,         // Sent when editor needs to update the track view.
    eNotify_OnSplashScreenCreated,     // Sent when the editor splash screen was created.
    eNotify_OnSplashScreenDestroyed,   // Sent when the editor splash screen was destroyed.

    eNotify_OnUpdateSequencer,         // Sent when editor needs to update the CryMannequin sequencer view.
    eNotify_OnUpdateSequencerKeys,     // Sent when editor needs to update keys in the CryMannequin track view.
    eNotify_OnUpdateSequencerKeySelection,    // Sent when CryMannequin sequencer view changes selection of keys.

    eNotify_OnInvalidateControls,      // Sent when editor needs to update some of the data that can be cached by controls like combo boxes.
    eNotify_OnStyleChanged,            // Sent when UI color theme was changed

    // Object events.
    eNotify_OnSelectionChange,         // Sent when object selection change.
    eNotify_OnPlaySequence,            // Sent when editor start playing animation sequence.
    eNotify_OnStopSequence,            // Sent when editor stop playing animation sequence.
    eNotify_OnOpenGroup,               // Sent when a group/prefab was opened
    eNotify_OnCloseGroup,              // Sent when a group/prefab was closed

    // Task specific events.
    eNotify_OnTerrainRebuild,          // Sent when terrain was rebuilt (resized,...)
    eNotify_OnBeginTerrainRebuild,     // Sent when terrain begin rebuilt (resized,...)
    eNotify_OnEndTerrainRebuild,       // Sent when terrain end rebuilt (resized,...)
    eNotify_OnVegetationObjectSelection, // When vegetation objects selection change.
    eNotify_OnVegetationPanelUpdate,   // When vegetation objects selection change.

    eNotify_OnDisplayRenderUpdate,     // Sent when editor finish terrain texture generation.

    eNotify_OnTimeOfDayChange,         // Time of day parameters where modified.

    eNotify_OnDataBaseUpdate,          // DataBase Library was modified.

    eNotify_OnLayerImportBegin,         //layer import was started
    eNotify_OnLayerImportEnd,           //layer import completed

    eNotify_OnBeginSWNewScene,          // Sent when SW document is begin to be cleared.
    eNotify_OnEndSWNewScene,                // Sent after SW document have been cleared.
    eNotify_OnBeginSWMoveTo,                // moveto operation was started
    eNotify_OnEndSWMoveTo,          // moveto operation completed
    eNotify_OnSWLockUnlock,             // Sent when commit, rollback or getting lock from segmented world
    eNotify_OnSWVegetationStatusChange, // When changed segmented world status of vegetation map

    eNotify_OnBeginUndoRedo,
    eNotify_OnEndUndoRedo,
    eNotify_CameraChanged, // When the active viewport camera was changed

    // Designer Object Specific events.
    eNotify_OnConvertToDesignerObjects,
    eNotify_OnExportBrushes, // For Designer objects, or objects using the Designer Tool.

    eNotify_OnTextureLayerChange,      // Sent when texture layer was added, removed or moved

    eNotify_OnSplatmapImport, // Sent when splatmaps get imported

#ifdef DEPRECATED_QML_SUPPORT
    eNotify_BeforeQMLDestroyed, // called before QML is destroyed so you can kill your resources (if any)
    eNotify_QMLReady, // when QML has been re-initialized - this can happen during runtime if plugins are unloaded and loaded and is your opportunity to register your types.
#else
    // QML is deprecated! These are left here so that the enum order doesn't change. Don't use
    eNotify_BeforeQMLDestroyed_Deprecated,
    eNotify_QMLReady_Deprecated,
#endif

    eNotify_OnParticleUpdate,          // A particle effect was modified.
    eNotify_OnAddAWSProfile,           // An AWS profile was added
    eNotify_OnSwitchAWSProfile,        // The AWS profile was switched
    eNotify_OnSwitchAWSDeployment,     // The AWS deployment was switched
    eNotify_OnFirstAWSUse,             // This should only be emitted once

    eNotify_OnRefCoordSysChange,

    // Entity selection events.
    eNotify_OnEntitiesSelected,
    eNotify_OnEntitiesDeselected,

    // More document events - added here in case enum values matter to any event consumers, metrics reporters, etc.
    eNotify_OnBeginCreate,               // Sent when the document is starting to be created.
    eNotify_OnEndCreate,                 // Sent when the document creation is finished.

};

// UI event handler
struct IUIEvent
{
    virtual void OnClick(DWORD dwId) = 0;
    virtual bool IsEnabled(DWORD dwId) = 0;
    virtual bool IsChecked(DWORD dwId) = 0;
    virtual const char* GetUIElementName(DWORD dwId) = 0;
};

//! Add object that implements this interface to Load listeners of IEditor
//! To receive notifications when new document is loaded.
struct IDocListener
{
    virtual ~IDocListener() = default;

    //! Called after new level is created.
    virtual void OnNewDocument() = 0;
    //! Called after level have been loaded.
    virtual void OnLoadDocument() = 0;
    //! Called when document is being closed.
    virtual void OnCloseDocument() = 0;
    //! Called when mission changes.
    virtual void OnMissionChange() = 0;
};

//! Derive from this class if you want to register for getting global editor notifications.
struct IEditorNotifyListener
{
    bool m_bIsRegistered;

    IEditorNotifyListener()
        : m_bIsRegistered(false) {}
    virtual ~IEditorNotifyListener()
    {
        if (m_bIsRegistered)
        {
            CryFatalError("Destroying registered IEditorNotifyListener");
        }
    }

    //! called by the editor to notify the listener about the specified event.
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event) = 0;
};

enum ERollupControlIds
{
    ROLLUP_OBJECTS = 0,
    ROLLUP_TERRAIN,
    ROLLUP_MODELLING,
    ROLLUP_DISPLAY,
    ROLLUP_LAYERS,
};

//! Axis constrains value.
enum AxisConstrains
{
    AXIS_NONE = 0,
    AXIS_X,
    AXIS_Y,
    AXIS_Z,
    AXIS_XY,
    AXIS_YZ,
    AXIS_XZ,
    AXIS_XYZ,
    //! Follow terrain constrain
    AXIS_TERRAIN,
};

//! Reference coordinate system values
enum RefCoordSys
{ // Don't change this order. Should be in the same order as MainWindow::CreateRefCoordComboBox()
    COORDS_VIEW = 0,
    COORDS_LOCAL,
    COORDS_PARENT,
    COORDS_WORLD,
    COORDS_USERDEFINED,
    LAST_COORD_SYSTEM, // Must always be the last member
};

// Insert locations for menu items
enum EMenuInsertLocation
{
    // Custom menu of the plugin
    eMenuPlugin,
    // Predefined editor menus
    eMenuEdit,
    eMenuFile,
    eMenuInsert,
    eMenuGenerators,
    eMenuScript,
    eMenuView,
    eMenuHelp
};

//! Global editor operation mode
enum EOperationMode
{
    eOperationModeNone = 0, // None
    eCompositingMode, // Normal operation mode where objects are composited in the scene
    eModellingMode // Geometry modeling mode
};

enum EEditMode
{
    eEditModeSelect,
    eEditModeSelectArea,
    eEditModeMove,
    eEditModeRotate,
    eEditModeScale,
    eEditModeTool,
    eEditModeRotateCircle,
};

//! Mouse events that viewport can send
enum EMouseEvent
{
    eMouseMove,
    eMouseLDown,
    eMouseLUp,
    eMouseLDblClick,
    eMouseRDown,
    eMouseRUp,
    eMouseRDblClick,
    eMouseMDown,
    eMouseMUp,
    eMouseMDblClick,
    eMouseWheel,
    eMouseLeave,
};

//! Viewports update flags
enum UpdateConentFlags
{
    eUpdateHeightmap    = 0x01,
    eUpdateStatObj      = 0x02,
    eUpdateObjects      = 0x04, //! Update objects in viewport.
    eRedrawViewports    = 0x08 //! Just redraw viewports..
};

enum MouseCallbackFlags
{
    MK_CALLBACK_FLAGS = 0x100
};

//! Types of database items
enum EDataBaseItemType
{
    EDB_TYPE_MATERIAL,
    EDB_TYPE_PARTICLE,
    EDB_TYPE_MUSIC,
    EDB_TYPE_ENTITY_ARCHETYPE,
    EDB_TYPE_PREFAB,
    EDB_TYPE_EAXPRESET,
    EDB_TYPE_SOUNDMOOD,
    EDB_TYPE_DEPRECATED, // formerly EDB_TYPE_GAMETOKEN
    EDB_TYPE_FLARE
};

enum EEditorPathName
{
    EDITOR_PATH_OBJECTS,
    EDITOR_PATH_TEXTURES,
    EDITOR_PATH_SOUNDS,
    EDITOR_PATH_MATERIALS,
    EDITOR_PATH_UI_ICONS,
    EDITOR_PATH_LAST
};

enum EModifiedModule
{
    eModifiedNothing = 0x0,
    eModifiedTerrain = BIT(0),
    eModifiedBrushes = BIT(1),
    eModifiedEntities = BIT(2),
    eModifiedAll = -1
};

//! Callback class passed to PickObject.
struct IPickObjectCallback
{
    virtual ~IPickObjectCallback() = default;

    //! Called when object picked.
    virtual void OnPick(CBaseObject* picked) = 0;
    //! Called when pick mode cancelled.
    virtual void OnCancelPick() = 0;
    //! Return true if specified object is pickable.
    virtual bool OnPickFilter(CBaseObject* filterObject) { return true; };
    //! If need a specific behavior when holding space, return true or if not, return false.
    virtual bool IsNeedSpecificBehaviorForSpaceAcce()   {   return false;   }
};

//! Class provided by editor for various registration functions.
struct CRegistrationContext
{
    CEditorCommandManager* pCommandManager;
    CClassFactory* pClassFactory;
};

//! Interface provided by editor to reach status bar functionality.
struct IMainStatusBar
{
    virtual void SetStatusText(const QString& text) = 0;
    virtual QWidget* SetItem(QString indicatorName, QString text, QString tip, int iconId) = 0;
    virtual QWidget* SetItem(QString indicatorName, QString text, QString tip, const QPixmap& icon) = 0;

    virtual QWidget* GetItem(QString indicatorName) = 0;
};

// forward declaration
struct IAnimSequence;
class CTrackViewSequence;

//! Interface to expose TrackViewSequenceManager functionality to SequenceComponent
struct ITrackViewSequenceManager
{
    virtual IAnimSequence* OnCreateSequenceObject(QString name, bool isLegacySequence = true, AZ::EntityId entityId = AZ::EntityId()) = 0;

    //! Notifies of the delete of a sequence entity OR legacy sequence object
    //! @param entityId The Sequence Component Entity Id OR the legacy sequence object Id packed in the lower 32-bits, as returned from IAnimSequence::GetSequenceEntityId()
    virtual void OnDeleteSequenceEntity(const AZ::EntityId& entityId) = 0;

    //! Get the first sequence with the given name. They may be more than one sequence with this name.
    //! Only intended for use with scripting or other cases where a user provides a name.
    virtual CTrackViewSequence* GetSequenceByName(QString name) const = 0;

    //! Get the sequence with the given EntityId. For legacy support, legacy sequences can be found by giving
    //! the sequence ID in the lower 32 bits of the EntityId.
    virtual CTrackViewSequence* GetSequenceByEntityId(const AZ::EntityId& entityId) const = 0;

    virtual void OnCreateSequenceComponent(AZStd::intrusive_ptr<IAnimSequence>& sequence) = 0;

    virtual void OnSequenceActivated(const AZ::EntityId& entityId) = 0;
};

//! Interface to expose TrackViewSequence functionality to SequenceComponent
struct ITrackViewSequence
{
    virtual void Load() = 0;
};

//! Interface to permit usage of editor functionality inside the plugin
struct IEditor
{
    virtual void DeleteThis() = 0;
    //! Access to Editor ISystem interface.
    virtual ISystem* GetSystem() = 0;
    virtual IGame* GetGame() = 0;
    virtual I3DEngine* Get3DEngine() = 0;
    virtual IRenderer* GetRenderer() = 0;
    //! Access to class factory.
    virtual IEditorClassFactory* GetClassFactory() = 0;
    //! Access to commands manager.
    virtual CEditorCommandManager* GetCommandManager() = 0;
    virtual ICommandManager* GetICommandManager() = 0;
    // Executes an Editor command.
    virtual void ExecuteCommand(const char* sCommand, ...) = 0;
    virtual void ExecuteCommand(const QString& sCommand) = 0;
    virtual void SetDocument(CCryEditDoc* pDoc) = 0;
    //! Get active document
    virtual CCryEditDoc* GetDocument() const = 0;
    //! Check if there is a level loaded
    virtual bool IsLevelLoaded() const = 0;
    //! Set document modified flag.
    virtual void SetModifiedFlag(bool modified = true) = 0;
    virtual void SetModifiedModule(EModifiedModule eModifiedModule, bool boSet = true) = 0;
    virtual bool IsLevelExported() const = 0;
    virtual bool SetLevelExported(bool boExported = true) = 0;
    //! Check if active document is modified.
    virtual bool IsModified() = 0;
    //! Save current document.
    virtual bool SaveDocument() = 0;
    //! Legacy version of WriteToConsole; don't use.
    virtual void WriteToConsole(const char* string) = 0;
    //! Write the passed string to the editors console
    virtual void WriteToConsole(const QString& string) = 0;
    //! Set value of console variable.
    virtual void SetConsoleVar(const char* var, float value) = 0;
    //! Get value of console variable.
    virtual float GetConsoleVar(const char* var) = 0;
    //! Shows or Hides console window.
    //! @return Previous visibility flag of console.
    virtual bool ShowConsole(bool show) = 0;
    // Get Main window status bar
    virtual IMainStatusBar* GetMainStatusBar() = 0;
    //! Change the message in the status bar
    virtual void SetStatusText(const QString& pszString) = 0;
    //! Query main window of the editor
    virtual QMainWindow* GetEditorMainWindow() const = 0;
    //! Returns the path of the editors Master CD folder
    virtual QString GetMasterCDFolder() = 0;
    //! Get current level name (name only)
    virtual QString GetLevelName() = 0;
    //! Get path to folder of current level (Absolute, contains slash)
    virtual QString GetLevelFolder() = 0;
    //! Get path to folder of current level (absolute)
    virtual QString GetLevelDataFolder() = 0;
    //! Get path to folder of current level.
    virtual QString GetSearchPath(EEditorPathName path) = 0;
    //! This folder is supposed to store Sandbox user settings and state
    virtual QString GetResolvedUserFolder() = 0;
    //! Returns the name of the sys_game_folder
    virtual QString GetProjectName() = 0;
    //! Execute application and get console output.
    virtual bool ExecuteConsoleApp(
        const QString& program,
        const QStringList& arguments,
        QString& outputText,
        bool noTimeOut = false,
        bool showWindow = false) = 0;
    //! Sets the document modified flag in the editor
    virtual void SetDataModified() = 0;
    //! Tells if editor startup is finished
    virtual bool IsInitialized() const = 0;
    //! Check if editor running in gaming mode.
    virtual bool IsInGameMode() = 0;
    //! Check if editor running in AI/Physics mode.
    virtual bool IsInSimulationMode() = 0;
    //! Set game mode of editor.
    virtual void SetInGameMode(bool inGame) = 0;
    //! Return true if Editor runs in the testing mode.
    virtual bool IsInTestMode() = 0;
    //! Return true if Editor runs in the preview mode.
    virtual bool IsInPreviewMode() = 0;
    //! Return true if Editor runs in the console only mode.
    virtual bool IsInConsolewMode() = 0;
    //! return true if editor is running the level load tests mode.
    virtual bool IsInLevelLoadTestMode() = 0;
    //! Return true if Editor runs in the material editing mode.
    virtual bool IsInMatEditMode() = 0;
    //! Enable/Disable updates of editor.
    virtual void EnableUpdate(bool enable) = 0;
    //! Enable/Disable accelerator table, (Enabled by default).
    virtual void EnableAcceleratos(bool bEnable) = 0;
    virtual SFileVersion GetFileVersion() = 0;
    virtual SFileVersion GetProductVersion() = 0;
    //! Retrieve pointer to game engine instance
    virtual CGameEngine* GetGameEngine() = 0;
    virtual CDisplaySettings*   GetDisplaySettings() = 0;
    virtual const SGizmoParameters& GetGlobalGizmoParameters() = 0;
    //! Create new object
    virtual CBaseObject* NewObject(const char* typeName, const char* fileName = "", const char* name = "", float x = 0.0f, float y = 0.0f, float z = 0.0f, bool modifyDoc = true) = 0;
    //! Delete object
    virtual void DeleteObject(CBaseObject* obj) = 0;
    //! Clone object
    virtual CBaseObject* CloneObject(CBaseObject* obj) = 0;
    //! Starts creation of new object
    virtual void StartObjectCreation(const QString& type) = 0;
    virtual void StartObjectCreation(const QString& type, const QString& file) = 0;
    //! Get current selection group
    virtual CSelectionGroup* GetSelection() = 0;
    virtual CBaseObject* GetSelectedObject() = 0;
    virtual int ClearSelection() = 0;
    //! Select object
    virtual void SelectObject(CBaseObject* obj) = 0;
    //! Lock current objects selection
    //! While selection locked, other objects cannot be selected or unselected
    virtual void LockSelection(bool bLock) = 0;
    //! Check if selection is currently locked
    virtual bool IsSelectionLocked() = 0;
    //! Get access to object manager.
    virtual struct IObjectManager* GetObjectManager() = 0;
    virtual CSettingsManager* GetSettingsManager() = 0;
    //! Set pick object mode.
    //! When object picked callback will be called, with OnPick
    //! If pick operation is canceled Cancel will be called
    //! @param targetClass specifies objects of which class are supposed to be picked
    //! @param bMultipick if true pick tool will pick multiple object
    virtual void PickObject(
        IPickObjectCallback* callback,
        const QMetaObject* targetClass = 0,
        const char* statusText = 0,
        bool bMultipick = false) = 0;
    //! Cancel current pick operation
    virtual void CancelPick() = 0;
    //! Return true if editor now in object picking mode
    virtual bool IsPicking() = 0;
    //! Get DB manager that own items of specified type.
    virtual IDataBaseManager* GetDBItemManager(EDataBaseItemType itemType) = 0;
    //! Get Entity prototype manager.
    virtual CEntityPrototypeManager* GetEntityProtManager() = 0;
    //! Get Manager of Materials.
    virtual CMaterialManager* GetMaterialManager() = 0;
    virtual IBaseLibraryManager* GetMaterialManagerLibrary() = 0; // Vladimir@conffx
    virtual IEditorMaterialManager* GetIEditorMaterialManager() = 0; // Vladimir@Conffx
    //! Returns IconManager.
    virtual IIconManager* GetIconManager() = 0;
    //! Returns Particle manager.
    virtual IEditorParticleManager* GetParticleManager() = 0;
    //! Returns Particle editor utilities.
    virtual IEditorParticleUtils* GetParticleUtils() = 0;
    //! Get Music Manager.
    virtual CMusicManager* GetMusicManager() = 0;
    //! Get Prefabs Manager.
    virtual CPrefabManager* GetPrefabManager() = 0;
    //! Get Lens Flare Manager.
    virtual CLensFlareManager* GetLensFlareManager() = 0;
    virtual float GetTerrainElevation(float x, float y) = 0;
    virtual class CHeightmap* GetHeightmap() = 0;
    virtual class IHeightmap* GetIHeightmap() = 0;
    virtual class CVegetationMap* GetVegetationMap() = 0;
    virtual class CAIManager*   GetAI() = 0;
    virtual Editor::EditorQtApplication* GetEditorQtApplication() = 0;
    virtual const QColor& GetColorByName(const QString& name) = 0;

    virtual struct IMovieSystem* GetMovieSystem() = 0;
    virtual class CEquipPackLib* GetEquipPackLib() = 0;
    virtual class CPluginManager* GetPluginManager() = 0;
    //! Get access to terrain manager.
    virtual class CTerrainManager* GetTerrainManager() = 0;
    virtual class CViewManager* GetViewManager() = 0;
    virtual class CViewport* GetActiveView() = 0;
    virtual void SetActiveView(CViewport* viewport) = 0;
    virtual struct IBackgroundTaskManager* GetBackgroundTaskManager() = 0;
    virtual struct IEditorFileMonitor* GetFileMonitor() = 0;

    // These are needed for Qt integration:
    virtual void RegisterEventLoopHook(IEventLoopHook* pHook) = 0;
    virtual void UnregisterEventLoopHook(IEventLoopHook* pHook) = 0;
    // ^^^

    //! QMimeData is used by the Qt clipboard.
    //! IMPORTANT: Any QMimeData allocated for the clipboard will be deleted
    //! when the editor exists. If a QMimeData is allocated by a different
    //! memory allocator (for example, in a different DLL) than the one used
    //! by the main editor, a crash will occur on exit, if data is left in
    //! the clipboard. The solution is to enfore all allocations of QMimeData
    //! using CreateQMimeData().
    virtual QMimeData* CreateQMimeData() const = 0;
    virtual void DestroyQMimeData(QMimeData* data) const = 0;

    virtual IMissingAssetResolver* GetMissingAssetResolver() = 0;
    //////////////////////////////////////////////////////////////////////////
    // Access for CLevelIndependentFileMan
    // Manager can be used to register as an module that is asked before editor quits / loads level / creates level
    // This gives the module the change to save changes or cancel the process
    //////////////////////////////////////////////////////////////////////////
    virtual class CLevelIndependentFileMan* GetLevelIndependentFileMan() = 0;
    //! Notify all views that data is changed.
    virtual void UpdateViews(int flags = 0xFFFFFFFF, const AABB* updateRegion = NULL) = 0;
    virtual void ResetViews() = 0;
    //! Update information in track view dialog.
    virtual void ReloadTrackView() = 0;
    //! Update information in CryMannequin sequencer dialog.
    virtual void UpdateSequencer(bool bOnlyKeys = false) = 0;
    //! Current position marker
    virtual Vec3 GetMarkerPosition() = 0;
    //! Set current position marker.
    virtual void    SetMarkerPosition(const Vec3& pos) = 0;
    //! Set current selected region.
    virtual void    SetSelectedRegion(const AABB& box) = 0;
    //! Get currently selected region.
    virtual void    GetSelectedRegion(AABB& box) = 0;
    //! Get current ruler
    virtual CRuler* GetRuler() = 0;
    //! Select Current rollup bar
    virtual int SelectRollUpBar(int rollupBarId) = 0;
    //! Insert a new Qt QWidget based page into the roll up bar
    virtual int AddRollUpPage(
        int rollbarId,
        const QString& pszCaption,
        QWidget* pwndTemplate,
        int iIndex = -1,
        bool bAutoExpand = true) = 0;

    //! Remove a dialog page from the roll up bar
    virtual void RemoveRollUpPage(int rollbarId, int iIndex) = 0;
    //! Rename a dialog page in the roll up bar
    virtual void RenameRollUpPage(int rollbarId, int iIndex, const char* nNewName) = 0;
    //! Expand one of the rollup pages
    virtual void ExpandRollUpPage(int rollbarId, int iIndex, bool bExpand = true) = 0;
    //! Enable or disable one of the rollup pages
    virtual void EnableRollUpPage(int rollbarId, int iIndex, bool bEnable = true) = 0;
    virtual int GetRollUpPageCount(int rollbarId) = 0;
    virtual void SetOperationMode(EOperationMode mode) = 0;
    virtual EOperationMode GetOperationMode() = 0;
    //! editMode - EEditMode
    virtual void SetEditMode(int editMode) = 0;
    virtual int GetEditMode() = 0;
    //! Assign current edit tool, destroy previously used edit too.
    virtual void SetEditTool(CEditTool* tool, bool bStopCurrentTool = true) = 0;
    //! Assign current edit tool by class name.
    virtual void SetEditTool(const QString& sEditToolName, bool bStopCurrentTool = true) = 0;
    //! Reinitializes the current edit tool if one is selected.
    virtual void ReinitializeEditTool() = 0;
    //! Returns current edit tool.
    virtual CEditTool* GetEditTool() = 0;
    //! Shows/Hides transformation manipulator.
    //! if bShow is true also returns a valid ITransformManipulator pointer.
    virtual ITransformManipulator* ShowTransformManipulator(bool bShow) = 0;
    //! Return a pointer to a ITransformManipulator pointer if shown.
    //! NULL is manipulator is not shown.
    virtual ITransformManipulator* GetTransformManipulator() = 0;
    //! Set constrain on specified axis for objects construction and modifications.
    //! @param axis one of AxisConstrains enumerations.
    virtual void SetAxisConstraints(AxisConstrains axis) = 0;
    //! Get axis constrain for objects construction and modifications.
    virtual AxisConstrains GetAxisConstrains() = 0;
    //! Set whether axes are forced to the same value when they are changed (x = y = z).
    virtual void SetAxisVectorLock(bool bAxisVectorLock) = 0;
    //! Get whether axes are forced to the same value when they are changed (x = y = z).
    virtual bool IsAxisVectorLocked() = 0;
    //! If set, when axis terrain constrain is selected, snapping only to terrain.
    virtual void SetTerrainAxisIgnoreObjects(bool bIgnore) = 0;
    virtual bool IsTerrainAxisIgnoreObjects() = 0;
    //! Set current reference coordinate system used when constructing/modifying objects.
    virtual void SetReferenceCoordSys(RefCoordSys refCoords) = 0;
    //! Get current reference coordinate system used when constructing/modifying objects.
    virtual RefCoordSys GetReferenceCoordSys() = 0;
    virtual XmlNodeRef FindTemplate(const QString& templateName) = 0;
    virtual void AddTemplate(const QString& templateName, XmlNodeRef& tmpl) = 0;
    //! Open database library and select specified item.
    //! If parameter is NULL current selection in material library does not change.
    virtual CBaseLibraryDialog* OpenDataBaseLibrary(EDataBaseItemType type, IDataBaseItem* pItem = NULL) = 0;

    virtual const QtViewPane* OpenView(QString sViewClassName, bool reuseOpen = true) = 0;
    virtual QWidget* FindView(QString viewClassName) = 0;

    virtual bool CloseView(const char* sViewClassName) = 0;
    virtual bool SetViewFocus(const char* sViewClassName) = 0;
    virtual void CloseView(const GUID& classId) = 0; // close ALL panels related to classId, used when unloading plugins.

    // We want to open a view object but not wrap it in a view pane)
    virtual QWidget* OpenWinWidget(WinWidgetId openId) = 0;
    virtual WinWidget::WinWidgetManager* GetWinWidgetManager() const = 0;

    //! Opens standard color selection dialog.
    //! Initialized with the color specified in color parameter.
    //! Returns true if selection is made and false if selection is canceled.
    virtual bool SelectColor(QColor &color, QWidget *parent = 0) = 0;
    //! Get shader enumerator.
    virtual class CShaderEnum* GetShaderEnum() = 0;
    virtual class CUndoManager* GetUndoManager() = 0;
    //! Begin operation requiring undo
    //! Undo manager enters holding state.
    virtual void BeginUndo() = 0;
    //! Restore all undo objects registered since last BeginUndo call.
    //! @param bUndo if true all Undo object registered since BeginUpdate call up to this point will be undone.
    virtual void RestoreUndo(bool undo = true) = 0;
    //! Accept changes and registers an undo object with the undo manager.
    //! This will allow the user to undo the operation.
    virtual void AcceptUndo(const QString& name) = 0;
    //! Cancel changes and restore undo objects.
    virtual void CancelUndo() = 0;
    //! Normally this is NOT needed but in special cases this can be useful.
    //! This allows to group a set of Begin()/Accept() sequences to be undone in one operation.
    virtual void SuperBeginUndo() = 0;
    //! When a SuperBegin() used, this method is used to Accept.
    //! This leaves the undo database in its modified state and registers the IUndoObjects with the undo system.
    //! This will allow the user to undo the operation.
    virtual void SuperAcceptUndo(const QString& name) = 0;
    //! Cancel changes and restore undo objects.
    virtual void SuperCancelUndo() = 0;
    //! Suspend undo recording.
    virtual void SuspendUndo() = 0;
    //! Resume undo recording.
    virtual void ResumeUndo() = 0;
    // Undo last operation.
    virtual void Undo() = 0;
    //! Redo last undo.
    virtual void Redo() = 0;
    //! Check if undo information is recording now.
    virtual bool IsUndoRecording() = 0;
    //! Check if undo information is suspzended now.
    virtual bool IsUndoSuspended() = 0;
    //! Put new undo object, must be called between Begin and Accept/Cancel methods.
    virtual void RecordUndo(struct IUndoObject* obj) = 0;
    //! Completely flush all Undo and redo buffers.
    //! Must be done on level reloads or global Fetch operation.
    virtual bool FlushUndo(bool isShowMessage = false) = 0;
    //! Clear the last N number of steps in the undo stack
    virtual bool ClearLastUndoSteps(int steps) = 0;
    //! Clear all current Redo steps in the undo stack
    virtual bool ClearRedoStack() = 0;
    //! Retrieve current animation context.
    virtual CAnimationContext* GetAnimation() = 0;
    //! Retrieve sequence manager
    virtual CTrackViewSequenceManager* GetSequenceManager() = 0;
    virtual ITrackViewSequenceManager* GetSequenceManagerInterface() = 0;

    //! Returns external tools manager.
    virtual CToolBoxManager* GetToolBoxManager() = 0;
    //! Get global Error Report instance.
    virtual IErrorReport* GetErrorReport() = 0;
    virtual IErrorReport* GetLastLoadedLevelErrorReport() = 0;
    virtual void StartLevelErrorReportRecording() = 0;
    virtual void CommitLevelErrorReport() = 0;
    // Retrieve interface to FileUtil
    virtual IFileUtil* GetFileUtil() = 0;
    // Notify all listeners about the specified event.
    virtual void Notify(EEditorNotifyEvent event) = 0;
    // Notify all listeners about the specified event, except for one.
    virtual void NotifyExcept(EEditorNotifyEvent event, IEditorNotifyListener* listener) = 0;
    //! Register Editor notifications listener.
    virtual void RegisterNotifyListener(IEditorNotifyListener* listener) = 0;
    //! Unregister Editor notifications listener.
    virtual void UnregisterNotifyListener(IEditorNotifyListener* listener) = 0;
    //! Register document notifications listener.
    virtual void RegisterDocListener(IDocListener* listener) = 0;
    //! Unregister document notifications listener.
    virtual void UnregisterDocListener(IDocListener* listener) = 0;
    //! Retrieve interface to the source control.
    virtual ISourceControl* GetSourceControl() = 0;
    //! Retrieve true if source control is provided and enabled in settings
    virtual bool IsSourceControlAvailable() = 0;
    //! Only returns true if source control is both available AND currently connected and functioning
    virtual bool IsSourceControlConnected() = 0;

    virtual CUIEnumsDatabase* GetUIEnumsDatabase() = 0;
    virtual void AddUIEnums() = 0;
    virtual void GetMemoryUsage(ICrySizer* pSizer) = 0;
    virtual void ReduceMemory() = 0;

    //! Export manager for exporting objects and a terrain from the game to DCC tools
    virtual IExportManager* GetExportManager() = 0;
    //! Set current configuration spec of the editor.
    virtual void SetEditorConfigSpec(ESystemConfigSpec spec, ESystemConfigPlatform platform) = 0;
    virtual ESystemConfigSpec GetEditorConfigSpec() const = 0;
    virtual ESystemConfigPlatform GetEditorConfigPlatform() const = 0;
    virtual void ReloadTemplates() = 0;
    virtual IResourceSelectorHost* GetResourceSelectorHost() = 0;
    virtual struct IBackgroundScheduleManager* GetBackgroundScheduleManager() = 0;
    virtual void ShowStatusText(bool bEnable) = 0;

    // Resource Manager
    virtual void SetAWSResourceManager(IAWSResourceManager* awsResourceManager) = 0;
    virtual IAWSResourceManager* GetAWSResourceManager() = 0;
    // Console federation helper
    virtual void LaunchAWSConsole(QString destUrl) = 0;

    // Prompt to open the Project Configurator with a specific message.
    virtual bool ToProjectConfigurator(const QString& msg, const QString& caption, const QString& location) = 0;

    // Provides a way to extend the context menu of an object. The function gets called every time the menu is opened.
    typedef Functor2<QMenu*, const CBaseObject*> TContextMenuExtensionFunc;
    virtual void RegisterObjectContextMenuExtension(TContextMenuExtensionFunc func) = 0;

    virtual void SetCurrentMissionTime(float time) = 0;

    virtual SSystemGlobalEnvironment* GetEnv() = 0;
    virtual IAssetBrowser* GetAssetBrowser() = 0; // Vladimir@Conffx
    virtual IImageUtil* GetImageUtil() = 0;  // Vladimir@conffx
    virtual SEditorSettings* GetEditorSettings() = 0;

#ifdef DEPRECATED_QML_SUPPORT
    virtual QQmlEngine* GetQMLEngine() const = 0;
#endif // #ifdef DEPRECATED_QML_SUPPORT

    virtual ILogFile* GetLogFile() = 0;  // Vladimir@conffx

    // unload all plugins.  Destroys the QML engine because it has to.
    virtual void UnloadPlugins(bool shuttingDown = false) = 0;

    // reloads the plugins.  Loads up the QML engine again.
    virtual void LoadPlugins() = 0;

    // no other way to get this from a plugin (that I can find)
    virtual CBaseObject* BaseObjectFromEntityId(unsigned int id) = 0;

    // For flagging if the legacy UI items should be enabled
    virtual bool IsLegacyUIEnabled() = 0;
    virtual void SetLegacyUIEnabled(bool enabled) = 0;

    virtual bool IsNewViewportInteractionModelEnabled() const = 0;
};

//! Callback used by editor when initializing for info in UI dialogs
struct IInitializeUIInfo
{
    virtual void SetInfoText(const char* text) = 0;
};

#endif // CRYINCLUDE_EDITOR_IEDITOR_H
