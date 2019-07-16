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

#include <QMainWindow>
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
#include <QTranslator>
#include <QMimeData>
#include <QToolButton>
#include <QWidget>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManagerBus.h>

#include <AzFramework/Asset/AssetCatalogBus.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>

#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Widgets/Bookmarks/BookmarkDockWidget.h>
#include <GraphCanvas/Styling/StyleManager.h>

#include <Core/Core.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Bus/DocumentContextBus.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Debugger/ClientTransceiver.h>

#include <Editor/Assets/ScriptCanvasAssetHolder.h>
#include <Editor/Undo/ScriptCanvasGraphCommand.h>
#include <Editor/Utilities/RecentFiles.h>
#include <Editor/View/Dialogs/Settings.h>
#include <Editor/View/Widgets/NodePalette/NodePaletteModel.h>

#include <Editor/View/Widgets/AssetGraphSceneDataBus.h>
#include <Editor/View/Widgets/AssetGraphSceneData.h>

#if SCRIPTCANVAS_EDITOR
#include <Include/EditorCoreAPI.h>
#endif

namespace GraphCanvas
{
    class AssetEditorToolbar;
    class GraphCanvasMimeEvent;
    class GraphCanvasEditorEmptyDockWidget;
    class MiniMapDockWidget;

    namespace Styling
    {
        class StyleSheet;
    }
}

namespace Ui
{
    class MainWindow;
}

class QDir;
class QFile;
class QProgressDialog;

namespace ScriptCanvasEditor
{
    enum class UnsavedChangesOptions;
    namespace Widget
    {
        class BusSenderTree;
        class CommandLine;
        class GraphTabBar;
        class NodePaletteDockWidget;
        class NodeProperties;
        class NodeOutliner;
        class PropertyGrid;
        class LogPanelWidget;
    }

    class EBusHandlerActionMenu;
    class UndoManager;
    class VariableDockWidget;
    class UnitTestDockWidget;
    class ScriptCanvasBatchConverter;
    class LoggingWindow;
    class GraphValidationDockWidget;
    class MainWindowStatusWidget;
    class StatisticsDialog;

    // Custom Context Menus
    class SceneContextMenu;
    class ConnectionContextMenu;

    class MainWindow
        : public QMainWindow
        , private UIRequestBus::Handler
        , private GeneralRequestBus::Handler
        , private UndoNotificationBus::Handler
        , private GraphCanvas::SceneNotificationBus::MultiHandler
        , private GraphCanvas::SceneUIRequestBus::MultiHandler
        , private GraphCanvas::AssetEditorSettingsRequestBus::Handler
        , private DocumentContextNotificationBus::MultiHandler
        , private GraphCanvas::AssetEditorRequestBus::Handler
        , private VariablePaletteRequestBus::Handler
        , private ScriptCanvas::BatchOperationNotificationBus::Handler
        , private AssetGraphSceneBus::Handler
#if SCRIPTCANVAS_EDITOR
        //, public IEditorNotifyListener
#endif
        , private AutomationRequestBus::Handler
        , private GraphCanvas::ViewNotificationBus::Handler
        , public AZ::SystemTickBus::Handler
    {
        Q_OBJECT
    private:
        friend class ScriptCanvasBatchConverter;

    public:

        MainWindow();
        ~MainWindow() override;

    private:
        // UIRequestBus
        QWidget* GetMainWindow() override { return qobject_cast<QWidget*>(this); }

        // Undo Handlers
        void PostUndoPoint(AZ::EntityId sceneId) override;
        void SignalSceneDirty(const AZ::EntityId& sceneId) override;

        void PushPreventUndoStateUpdate() override;
        void PopPreventUndoStateUpdate() override;
        void ClearPreventUndoStateUpdate() override;
        void CreateUndoManager();
        void TriggerUndo() override;
        void TriggerRedo() override;

        // VariablePaletteRequestBus
        void RegisterVariableType(const ScriptCanvas::Data::Type& variableType) override;
        ////

        // GraphCanvas::AssetEditorRequestBus
        void OnSelectionManipulationBegin() override;
        void OnSelectionManipulationEnd() override;

        AZ::EntityId CreateNewGraph() override;

        void CustomizeConnectionEntity(AZ::Entity* connectionEntity);

        GraphCanvas::ContextMenuAction::SceneReaction ShowSceneContextMenu(const QPoint& screenPoint, const QPointF& scenePoint) override;

        GraphCanvas::ContextMenuAction::SceneReaction ShowNodeContextMenu(const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint) override;

        GraphCanvas::ContextMenuAction::SceneReaction ShowNodeGroupContextMenu(const AZ::EntityId& groupId, const QPoint& screenPoint, const QPointF& scenePoint) override;
        GraphCanvas::ContextMenuAction::SceneReaction ShowCollapsedNodeGroupContextMenu(const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint) override;

        GraphCanvas::ContextMenuAction::SceneReaction ShowBookmarkContextMenu(const AZ::EntityId& bookmarkId, const QPoint& screenPoint, const QPointF& scenePoint) override;

        GraphCanvas::ContextMenuAction::SceneReaction ShowConnectionContextMenu(const AZ::EntityId& connectionId, const QPoint& screenPoint, const QPointF& scenePoint) override;

        GraphCanvas::ContextMenuAction::SceneReaction ShowSlotContextMenu(const AZ::EntityId& slotId, const QPoint& screenPoint, const QPointF& scenePoint) override;
        ////

        // SystemTickBus
        void OnSystemTick() override;
        ////

        //! ScriptCanvas::BatchOperationsNotificationBus
        void OnCommandStarted(AZ::Crc32 commandTag);
        void OnCommandFinished(AZ::Crc32 commandTag);

        // UI Handlers        
        GraphCanvas::Endpoint CreateNodeForProposal(const AZ::EntityId& connectionId, const GraphCanvas::Endpoint& endpoint, const QPointF& scenePoint, const QPoint& screenPoint) override;
        void OnWrapperNodeActionWidgetClicked(const AZ::EntityId& wrapperNode, const QRect& actionWidgetBoundingRect, const QPointF& scenePoint, const QPoint& screenPoint) override;

        // File menu
        void OnFileNew();
        bool OnFileSave(const DocumentContextRequests::SaveCB& saveCB);
        bool OnFileSaveAs(const DocumentContextRequests::SaveCB& saveCB);
        bool OnFileSaveCaller(){return OnFileSave(nullptr);};
        bool OnFileSaveAsCaller(){return OnFileSaveAs(nullptr);};
        void OnFileOpen();

        // Edit menu
        void SetupEditMenu();
        void OnEditMenuShow();
        void RefreshPasteAction();
        void RefreshGraphPreferencesAction();
        void OnEditCut();
        void OnEditCopy();
        void OnEditPaste();
        void OnEditDuplicate();
        void OnEditDelete();
        void OnRemoveUnusedVariables();
        void OnRemoveUnusedNodes();
        void OnRemoveUnusedElements();

        // View menu
        void OnViewNodePalette();
        void OnViewOutline();
        void OnViewProperties();
        void OnViewDebugger();
        void OnViewCommandLine();
        void OnViewLog();
        void OnBookmarks();
        void OnVariableManager();
        void OnUnitTestManager();
        void OnViewMiniMap();
        void OnViewLogWindow();
        void OnViewGraphValidation();
        void OnViewDebuggingWindow();
        void OnViewStatisticsPanel();
        void OnRestoreDefaultLayout();

        void UpdateViewMenu();
        /////////////////////////////////////////////////////////////////////////////////////////////

        //SceneNotificationBus
        void OnSelectionChanged() override;

        void OnNodePositionChanged(const AZ::EntityId& nodeId, const AZ::Vector2& position) override;
        /////////////////////////////////////////////////////////////////////////////////////////////
        
        void OnVariableSelectionChanged(const AZStd::vector<AZ::EntityId>& variablePropertyIds);
        void QueuePropertyGridUpdate();
        void DequeuePropertyGridUpdate();

        void SetDefaultLayout();

        void RefreshSelection();
        void Clear();

        void OnTabInserted(int index);
        void OnTabRemoved(int index);
        void OnTabCloseRequest(int index);
        void OnTabCloseButtonPressed(int index);

        bool IsTabOpen(const AZ::Data::AssetId& assetId, int& outTabIndex) const;
        QVariant GetTabData(const AZ::Data::AssetId& assetId);

        //! GeneralRequestBus
        AZ::Outcome<int, AZStd::string> OpenScriptCanvasAssetId(const AZ::Data::AssetId& assetId);
        AZ::Outcome<int, AZStd::string> OpenScriptCanvasAsset(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset, int tabIndex = -1) override;
        int CloseScriptCanvasAsset(const AZ::Data::AssetId& assetId) override;

        bool IsScriptCanvasAssetOpen(const AZ::Data::AssetId& assetId) const override;

        const CategoryInformation* FindNodePaletteCategoryInformation(AZStd::string_view categoryPath) const override;
        const NodePaletteModelInformation* FindNodePaletteModelInformation(const ScriptCanvas::NodeTypeIdentifier& nodeType) const override;
        ////
        AZ::Outcome<int, AZStd::string> CreateScriptCanvasAsset(AZStd::string_view assetPath, int tabIndex = -1);
        AZ::Outcome<int, AZStd::string> UpdateScriptCanvasAsset(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset);
        
        void RefreshScriptCanvasAsset(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset);

        //! Removes the assetId -> ScriptCanvasAsset mapping and disconnects from the asset
        //! The DocumentContextNotificationBus and the GraphCanvas::SceneNotificationBus
        void RemoveScriptCanvasAsset(const AZ::Data::AssetId& assetId);
        void MoveScriptCanvasAsset(const AZ::Data::Asset<ScriptCanvasAsset>& newAsset, const ScriptCanvasAssetFileInfo& assetFileInfo);

        void OnChangeActiveGraphTab(const Widget::GraphTabMetadata&) override;
        GraphCanvas::GraphId GetActiveGraphCanvasGraphId() const override;
        AZ::EntityId GetActiveScriptCanvasGraphId() const override;

        bool IsInUndoRedo(const AZ::EntityId& graphCanvasGraphId) const override;
        bool IsActiveInUndoRedo() const override;

        GraphCanvas::GraphId GetGraphCanvasGraphId(const AZ::EntityId& scriptCanvasGraphId) const override;
        AZ::EntityId GetScriptCanvasGraphId(const GraphCanvas::GraphId& graphCanvasCanvasGraphId) const override;

        AZ::EntityId FindGraphCanvasGraphIdByAssetId(const AZ::Data::AssetId& assetId) const override;
        AZ::EntityId FindScriptCanvasGraphIdByAssetId(const AZ::Data::AssetId& assetId) const override;
        ////////////////////////////

        // GraphCanvasSettingsRequestBus
        double GetSnapDistance() const override;

        bool IsBookmarkViewportControlEnabled() const override;

        bool IsDragNodeCouplingEnabled() const override;
        AZStd::chrono::milliseconds GetDragCouplingTime() const override;

        bool IsDragConnectionSpliceEnabled() const override;
        AZStd::chrono::milliseconds GetDragConnectionSpliceTime() const override;

        bool IsDropConnectionSpliceEnabled() const override;
        AZStd::chrono::milliseconds GetDropConnectionSpliceTime() const override;

        bool IsSplicedNodeNudgingEnabled() const override;

        bool IsShakeToDespliceEnabled() const override;
        int GetShakesToDesplice() const override;
        float GetMinimumShakePercent() const override;
        float GetShakeDeadZonePercent() const override;
        float GetShakeStraightnessPercent() const override;
        AZStd::chrono::milliseconds GetMaximumShakeDuration() const override;

        AZStd::chrono::milliseconds GetAlignmentTime() const override;

        float GetEdgePanningPercentage() const override;
        float GetEdgePanningScrollSpeed() const override;
        ////

        // AutomationRequestBus
        NodeIdPair ProcessCreateNodeMimeEvent(GraphCanvas::GraphCanvasMimeEvent* mimeEvent, const AZ::EntityId& graphCanvasGraphId, AZ::Vector2 nodeCreationPos) override;
        const GraphCanvas::GraphCanvasTreeItem* GetNodePaletteRoot() const override;

        void SignalAutomationBegin() override;
        void SignalAutomationEnd() override;
        ////

        AZ::EntityId FindEditorNodeIdByAssetNodeId(const AZ::Data::AssetId& assetId, AZ::EntityId assetNodeId) const override;
        AZ::EntityId FindAssetNodeIdByEditorNodeId(const AZ::Data::AssetId& assetId, AZ::EntityId editorNodeId) const override;
        
    private:
        void DeleteNodes(const AZ::EntityId& sceneId, const AZStd::vector<AZ::EntityId>& nodes) override;
        void DeleteConnections(const AZ::EntityId& sceneId, const AZStd::vector<AZ::EntityId>& connections) override;
        void DisconnectEndpoints(const AZ::EntityId& sceneId, const AZStd::vector<GraphCanvas::Endpoint>& endpoints) override;
        /////////////////////////////////////////////////////////////////////////////////////////////        

        //! DocumentContextNotificationBus
        void OnScriptCanvasAssetReady(const AZ::Data::Asset<ScriptCanvasAsset>& scriptCanvasAsset) override;
        ////

        GraphCanvas::Endpoint HandleProposedConnection(const GraphCanvas::GraphId& graphId, const GraphCanvas::ConnectionId& connectionId, const GraphCanvas::Endpoint& endpoint, const GraphCanvas::NodeId& proposedNode, const QPoint& screenPoint);

        void SourceFileChanged(const AZ::Data::AssetId& saveAssetId, AZStd::string relPath, AZStd::string scanFolder, const AZ::Data::AssetId& assetId);

        void CloneAssetEntity(AZ::Data::Asset<ScriptCanvasAsset> scriptCanvasAsset);
        void ActivateAssetEntity(AZ::Data::Asset<ScriptCanvasAsset> scriptCanvasAsset);

        //! UndoNotificationBus
        void OnCanUndoChanged(bool canUndo) override;
        void OnCanRedoChanged(bool canRedo) override;
        ////

        GraphCanvas::ContextMenuAction::SceneReaction HandleContextMenu(GraphCanvas::EditorContextMenu& editorContextMenu, const AZ::EntityId& memberId, const QPoint& screenPoint, const QPointF& scenePoint) const;

        void OnAutoSave();

        //! Helper function which serializes a file to disk
        //! \param filename name of file to serialize the Entity
        //! \param asset asset to save
        void SaveScriptCanvasAsset(AZStd::string_view filename, AZ::Data::Asset<ScriptCanvasAsset> asset, const DocumentContextRequests::SaveCB& saveCB);
        AZStd::string GetSuggestedFullFilenameToSaveAs(const AZ::Data::AssetId& assetId);

        AZ::Data::Asset<ScriptCanvasAsset> CopyAssetForSave(const AZ::Data::AssetId& newAssetId, AZ::Data::Asset<ScriptCanvasAsset> oldAsset);

        void MarkAssetModified(const AZ::Data::AssetId& assetId);
        void MarkAssetUnmodified(const AZ::Data::AssetId& assetId);

        // QMainWindow
        void closeEvent(QCloseEvent *event) override;
        UnsavedChangesOptions ShowSaveDialog(const QString& filename);
        bool SaveAsset(const AZ::Data::AssetId& unsavedAssetId, const DocumentContextRequests::SaveCB& saveCB);

        void OpenFile(const char* fullPath);
        void CreateMenus();

        void SignalActiveSceneChanged(const AZ::EntityId& scriptCanvasGraphId);

        void SaveWindowState();
        void RestoreWindowState();

        void RunBatchConversion();

        void OnShowValidationErrors();
        void OnShowValidationWarnings();
        void OnValidateCurrentGraph();

        // ViewNotificationBus
        void OnViewParamsChanged(const GraphCanvas::ViewParams& viewParams) override;
        void OnZoomChanged(qreal zoomLevel) override;
        ////

    public slots:
        void UpdateRecentMenu();
        void OnViewVisibilityChanged(bool visibility);

    private:

        int CreateAssetTab(const AZ::Data::AssetId& assetId, int tabIndex = -1);
        void OpenAssetHelper(AZ::Data::Asset<ScriptCanvasAsset>& asset, int tabIndex);

        //! \param asset The AssetId of the ScriptCanvas Asset.
        void SetActiveAsset(const AZ::Data::AssetId& assetId);
        void RefreshActiveAsset();

        void SignalConversionComplete();

        void PrepareActiveAssetForSave();

        void RestartAutoTimerSave(bool forceTimer = false);

        QWidget* m_host = nullptr;

        AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* m_scriptEventsAssetModel;
        AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* m_scriptCanvasAssetModel;

        Widget::GraphTabBar*                m_tabBar = nullptr;
        GraphCanvas::AssetEditorToolbar*    m_editorToolbar = nullptr;
        QToolButton*                        m_validateGraphToolButton = nullptr;
        Widget::NodeOutliner*               m_nodeOutliner = nullptr;
        VariableDockWidget*                 m_variableDockWidget = nullptr;
        GraphValidationDockWidget*          m_validationDockWidget = nullptr;
        UnitTestDockWidget*                 m_unitTestDockWidget = nullptr;
        StatisticsDialog*                   m_statisticsDialog = nullptr;
        Widget::NodePaletteDockWidget*      m_nodePalette = nullptr;
        Widget::LogPanelWidget*             m_logPanel = nullptr;
        Widget::PropertyGrid*               m_propertyGrid = nullptr;
        Widget::CommandLine*                m_commandLine = nullptr;
        GraphCanvas::BookmarkDockWidget*    m_bookmarkDockWidget = nullptr;
        GraphCanvas::MiniMapDockWidget*     m_minimap = nullptr;
        LoggingWindow*                      m_loggingWindow = nullptr;

        MainWindowStatusWidget*             m_statusWidget = nullptr;

        NodePaletteModel                    m_nodePaletteModel;

        void CreateUnitTestWidget();

        // Reusable context menu for the context menu's that have a node palette.
        SceneContextMenu* m_sceneContextMenu;
        ConnectionContextMenu* m_connectionContextMenu;

        AZ::EntityId m_entityMimeDelegateId;

        // Reusable context menu for adding/removing ebus events from a wrapper node
        EBusHandlerActionMenu* m_ebusHandlerActionMenu;

        GraphCanvas::GraphCanvasEditorEmptyDockWidget* m_emptyCanvas; // Displayed when there is no open graph
        QVBoxLayout* m_layout;

        AZ::Data::AssetId m_activeAssetId;

        AssetGraphSceneMapper m_assetGraphSceneMapper;

        bool                  m_loadingNewlySavedFile;
        AZStd::string         m_newlySavedFile;

        bool m_enterState;
        bool m_ignoreSelection;        
        AZ::s32 m_preventUndoStateUpdateCount;

        Ui::MainWindow* ui;
        AZStd::array<AZStd::pair<QAction*, QMetaObject::Connection>, c_scriptCanvasEditorSettingsRecentFilesCountMax> m_recentActions;
        QHBoxLayout *m_horizontalTabBarLayout;
        QSpacerItem* m_plusButtonSpacer;

        AZStd::unique_ptr<UndoManager> m_undoManager;
        AZStd::intrusive_ptr<EditorSettings::ScriptCanvasEditorSettings> m_userSettings;

        bool m_isInAutomation;

        bool m_allowAutoSave;
        QTimer m_autoSaveTimer;

        QByteArray m_defaultLayout;
        QTranslator m_translator;

        AZStd::vector<AZ::EntityId> m_selectedVariableIds;

        AZStd::unordered_set< AZ::Data::AssetId > m_loadingAssets;
        AZStd::unordered_set< AZ::Uuid > m_variablePaletteTypes;

        ScriptCanvasBatchConverter* m_converter;
        ScriptCanvas::Debugger::ClientTransceiver m_clientTRX;
        GraphCanvas::StyleManager m_styleManager;
    };
}
