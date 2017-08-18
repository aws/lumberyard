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

#ifndef CRYINCLUDE_EDITOR_HYPERGRAPH_HYPERGRAPHDIALOG_H
#define CRYINCLUDE_EDITOR_HYPERGRAPH_HYPERGRAPHDIALOG_H
#pragma once

#include "HyperGraph.h"
#include "UserMessageDefines.h"
#include "Controls/BreakPointsCtrl.h"
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <IFlowGraphModuleManager.h>

class CEntityObject;
class CPrefabObject;
class CFlowGraph;
class CFlowGraphSearchResultsCtrl;
class CFlowGraphSearchCtrl;
class CHyperGraphDialog;
class CFlowGraphProperties;
class CFlowGraphTokensCtrl;

#include <QMainWindow>
#include <QDockWidget>
#include <QTreeWidget>
class QHyperGraphWidget;

//////////////////////////////////////////////////////////////////////////
class CHyperGraphsTreeCtrl
    : public QTreeWidget
    , public IHyperGraphManagerListener
{
public:
    typedef QList<QTreeWidgetItem*>     TDTreeItems;

    CHyperGraphsTreeCtrl(QWidget* parent = 0);
    ~CHyperGraphsTreeCtrl();
    virtual void OnHyperGraphManagerEvent(EHyperGraphEvent event, IHyperGraph* pGraph, IHyperNode* pNode);
    void SetCurrentGraph(CHyperGraph* pGraph);
    void Reload();
    void SetIgnoreReloads(bool bIgnore) { m_bIgnoreReloads = bIgnore; }
    QTreeWidgetItem* GetTreeItem(CHyperGraph* pGraph);
    BOOL DeleteEntityItem(CEntityObject* pEntity, QTreeWidgetItem*& hAdjacentItem);
    BOOL DeleteNonEntityItem(CHyperGraph* pGraph, QTreeWidgetItem*& hAdjacentItem);
    void GetSelectedItems(TDTreeItems& rchSelectedTreeItems);

    void SetItemImage(QTreeWidgetItem* item, int imageNdx) { item->setIcon(0, m_imageList[imageNdx]);  }
    CHyperGraph* GetItemData(QTreeWidgetItem* item);

protected:
    void OnObjectEvent(CBaseObject* pObject, int nEvent);
    void SortChildren(QTreeWidgetItem* parent);

    CHyperGraph* m_pCurrentGraph;
    bool m_bIgnoreReloads;
    QVector<QIcon> m_imageList;

    std::map<CEntityObject*, QTreeWidgetItem*> m_mapEntityToItem;
    std::map<CPrefabObject*, QTreeWidgetItem*> m_mapPrefabToItem;
};

//////////////////////////////////////////////////////////////////////////
namespace Ui {
    class QHyperGraphComponentsPanel;
}
class CFlowGraphManagerPrototypeFilteredModel;

class QHyperGraphComponentsPanel
    : public AzQtComponents::StyledDockWidget
{
public:
    QHyperGraphComponentsPanel(QWidget* parent);

    void Reload();
    void SetComponentFilterMask(uint32 mask);

private:
    Ui::QHyperGraphComponentsPanel* ui;
    CFlowGraphManagerPrototypeFilteredModel* m_model;
};


class QHyperGraphTaskPanel;
class QBreakpointsPanel;

//////////////////////////////////////////////////////////////////////////
//
// Main Dialog for HyperGraph.
//
//////////////////////////////////////////////////////////////////////////
class CHyperGraphDialog
    : public QMainWindow
    , public IEditorNotifyListener
    , public IHyperGraphManagerListener
{
    Q_OBJECT
public:
    typedef QList<QTreeWidgetItem*>     TDTreeItems;

    static void RegisterViewClass();
    static const GUID& GetClassID();
    static CHyperGraphDialog* instance();

    CHyperGraphDialog();
    ~CHyperGraphDialog();

    // Return pointer to the hyper graph view.
    void CenterViewAroundNode(CHyperNode* poNode, bool boFitZoom = false, float fZoom = -1.0f);
    void SetFocusOnView();
    void InvalidateView(bool bComplete = false);
    // scroll/zoom pToNode (of current graph) into view and optionally select it
    void ShowAndSelectNode(CHyperNode* pToNode, bool bSelect = true);

    CHyperGraphsTreeCtrl* GetGraphsTreeCtrl() { return m_graphsTreeCtrl; }
    CFlowGraphSearchCtrl* GetSearchControl() const { return m_pSearchOptionsView; }
    QHyperGraphWidget* GetGraphWidget() const { return m_widget; }

    void SetGraph(CHyperGraph* pGraph, bool viewOnly = false);
    CHyperGraph* GetGraph() const;


    void OnViewSelectionChange();
    void ShowResultsPane(bool bShow = true, bool bFocus = true);
    void UpdateGraphProperties(CHyperGraph* pGraph);
    // Delete item from treeview ctrl
    void DeleteItem(CFlowGraph* pGraph);

    void ReloadBreakpoints();
    void ClearBreakpoints();
    void ReloadGraphs(){ m_graphsTreeCtrl->Reload(); }
    void ReloadComponents(){ m_componentsTaskPanel->Reload(); }

    enum
    {
        IDD = IDD_TRACKVIEWDIALOG
    };

    enum EContextMenuID
    {
        ID_GRAPHS_CHANGE_FOLDER = 1,
        ID_GRAPHS_SELECT_ENTITY,
        ID_GRAPHS_REMOVE_GRAPH,
        ID_GRAPHS_ENABLE,
        ID_GRAPHS_ENABLE_ALL,
        ID_GRAPHS_DISABLE_ALL,
        ID_GRAPHS_RENAME_FOLDER,
        ID_GRAPHS_RENAME_GRAPH,
        ID_GRAPHS_REMOVE_NONENTITY_GRAPH,
        ID_BREAKPOINT_REMOVE_GRAPH,
        ID_BREAKPOINT_REMOVE_NODE,
        ID_BREAKPOINT_REMOVE_PORT,
        ID_BREAKPOINT_ENABLE,
        ID_TRACEPOINT_ENABLE,
        ID_GRAPHS_NEW_GLOBAL_MODULE,
        ID_GRAPHS_NEW_LEVEL_MODULE,
        ID_GRAPHS_ENABLE_DEBUGGING,
        ID_GRAPHS_DISABLE_DEBUGGING
    };

protected:
    virtual BOOL OnInitDialog();
    void closeEvent(QCloseEvent*) override;

protected slots:
    void OnFileNew();
    void OnFileOpen();
    void OnFileSave();
    void OnFileSaveAs();
    void OnFileNewAction();
    void OnFileNewCustomAction();
    void OnFileNewMatFXGraph();
    void OnFileNewGlobalFGModule();
    void OnFileNewLevelFGModule();
    void OnFileImport();
    void OnFileExportSelection();
    void OnEditUndo();
    void OnEditRedo();
    void OnFind();
    void OnEditGraphTokens();
    void OnEditModuleInputsOutputs();
    void OnMultiPlayerViewToggle();
    void OnToggleDebug();
    void OnEraseDebug();
    void OnDebugFlowgraphTypeToggle();
    void OnDebugFlowgraphTypeUpdate();
    void OnUpdateNav();
    void OnNavForward();
    void OnNavBack();
    void OnComponentsViewToggle();
    void OnComponentsViewUpdate();
    void OnPlay();
    void OnStop();
    void OnPause();
    void OnStep();
    void OnBreakpointsClick(QTreeWidgetItem* item, int column);
    void OnBreakpointsRightClick(const QPoint& pos);
    void OnGraphsRightClick(const QPoint& pos);
    void OnGraphsDblClick(QTreeWidgetItem* item, int column);
    void OnGraphsSelChanged();

protected:
    void OnUpdatePlay();
    void OnUpdateStep();

    void ClearGraph();
    CHyperGraph* NewGraph();
    void CreateMenuBar();
    void CreateRightPane();
    void CreateLeftPane();
    void UpdateActions();

    void GraphUpdateEnable(CFlowGraph* pFlowGraph);
    void EnableItems(QTreeWidgetItem* hItem, bool bEnable, bool bRecurse);
    void RenameItems(QTreeWidgetItem* hItem, QString& newGroupName, bool bRecurse);

    // Called by the editor to notify the listener about the specified event.
    void OnEditorNotifyEvent(EEditorNotifyEvent event);

    // Called by hypergraph manager (need this for nav back/forward)
    virtual void OnHyperGraphManagerEvent(EHyperGraphEvent event, IHyperGraph* pGraph, IHyperNode* pNode);

    void UpdateNavHistory();
    void UpdateTitle(CHyperGraph* pGraph);


    // Creates the right click context menu based on the currently selected items.
    // The current policy will make the menu have only the menu items that are common
    // to the entire selection.
    // OBS: renaming is only allowed for single selections.
    bool CreateContextMenu(QMenu& roMenu, TDTreeItems& rchSelectedItems);

    // This function will take the result of the menu and will process its
    // results applying the appropriate changes.
    void ProcessMenu(int nMenuOption, TDTreeItems& rchSelectedItems);

    bool CreateBreakpointContextMenu(QMenu& roMenu, QTreeWidgetItem* hItem);
    void ProcessBreakpointMenu(int nMenuOption, QTreeWidgetItem* hItem);

    //////////////////////////////////////////////////////////////////////////
    // This set of functions is used to help determining which menu items will
    // be available. Their usage is not 100% efficient, still, they improved
    // the readability of the code and thus made refactoring a lot easier.
    bool IsAnEntityFolderOrSubFolder(QTreeWidgetItem* hSelectedItem);
    bool IsAnEntitySubFolder(QTreeWidgetItem* hSelectedItem);
    // As both values, true or false would have interpretations,
    // we need to add another flag identifying if the result
    bool IsGraphEnabled(QTreeWidgetItem* hSelectedItem, bool& boIsEnabled);
    bool IsOwnedByAnEntity(QTreeWidgetItem* hSelectedItem);
    bool IsAssociatedToFlowGraph(QTreeWidgetItem* hSelectedItem);
    bool IsGraphType(QTreeWidgetItem* hSelectedItem, IFlowGraph::EFlowGraphType type);
    bool IsEnabledForDebugging(QTreeWidgetItem* hSelectedItem);
    bool IsComponentEntity(QTreeWidgetItem* hSelectedItem);
    bool IsGlobalModuleFolder(QTreeWidgetItem* hSelectedItem);
    bool IsLevelModuleFolder(QTreeWidgetItem* hSelectedItem);
    //////////////////////////////////////////////////////////////////////////
private:
    static CHyperGraphDialog* s_instance;
    void NewFGModule(IFlowGraphModule::EType type);
    bool RemoveModuleFile(const char* filePath);

    QHyperGraphWidget*  m_widget;
    QHyperGraphComponentsPanel* m_componentsTaskPanel;
    QHyperGraphTaskPanel* m_taskPanel;
    QBreakpointsPanel*  m_breakpointsPanel;
    QDockWidget* m_graphsTreeDock;
    CHyperGraphsTreeCtrl* m_graphsTreeCtrl;
    CFlowGraphSearchCtrl* m_pSearchOptionsView;
    CFlowGraphSearchResultsCtrl* m_pSearchResultsCtrl;

    uint32 m_componentsViewMask;

    std::list<CHyperGraph*> m_graphList;
    std::list<CHyperGraph*>::iterator m_graphIterCur;
    CHyperGraph* m_pNavGraph;
    IFlowGraphDebuggerPtr m_pFlowGraphDebugger;

    QAction*    m_viewMultiplayerAction;
    QAction*    m_debugToggleAction;
    QAction*    m_debugIgnoreAIAction;
    QAction*    m_debugIgnoreModuleAction;
    QAction*    m_debugIgnoreCustomAction;
    QAction*    m_debugIgnoreMaterialFXAction;
    QAction*    m_playAction;
    QAction*    m_undoAction;
    QAction*    m_redoAction;
    QAction*    m_debugEraseAction;
    QAction*    m_navBackAction;
    QAction*    m_navForwardAction;
    QAction*    m_componentsApprovedAction;
    QAction*    m_componentsAdvancedAction;
    QAction*    m_componentsDebugAction;
    QAction*    m_componentsObsoleteAction;

    bool        m_shownNotification = false;
    void        ShowNotificationDialog();
};

#endif // CRYINCLUDE_EDITOR_HYPERGRAPH_HYPERGRAPHDIALOG_H
