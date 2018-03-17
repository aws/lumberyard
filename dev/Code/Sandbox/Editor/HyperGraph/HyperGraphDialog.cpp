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

#include "StdAfx.h"
#include <IAIAction.h>
#include <IAISystem.h>
#include "Ai/AIManager.h"
#include "HyperGraphDialog.h"
#include "IViewPane.h"
#include "HyperGraphManager.h"
#include "Material/MaterialFXGraphMan.h"
#include "HyperGraph/FlowGraphModuleManager.h"
#include "QtViewPaneManager.h"

#include "FlowGraphManager.h"
#include "FlowGraph.h"
#include "FlowGraphNode.h"
#include "FlowGraphVariables.h"
#include "Objects/EntityObject.h"
#include "Objects/PrefabObject.h"
#include "GameEngine.h"
#include "StringDlg.h"
#include "GenericSelectItemDialog.h"
#include "FlowGraphSearchCtrl.h"
#include "CommentNode.h"
#include "CommentBoxNode.h"
#include "BlackBoxNode.h"
#include "FlowGraphProperties.h"
#include "FlowGraphTokens.h"
#include "FlowGraphModuleDlgs.h"
#include <IAgent.h>
#include "CustomActions/CustomActionsEditorManager.h"
#include <ICustomActions.h>
#include <IGameFramework.h>
#include <ILevelSystem.h>
#include <IFlowGraphModuleManager.h>
#include "FlowGraphDebuggerEditor.h"
#include "QAbstractQVariantTreeDataModel.h"

#include "HyperGraphWidget.h"
#if QT_VERSION >= 0x50000
#include <QWindow>
#include <QGuiApplication>
#endif
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QDebug>
#include <QMimeData>
#include <QMessageBox>
#include <QSettings>
#include <QInputDialog>
#include <QTimer>
#include <HyperGraph/ui_QHyperGraphComponentsPanel.h>
#include <HyperGraph/ui_QHyperGraphTaskPanel.h>

#include <QtUtilWin.h>
#include <QtUtil.h>
#include "FlowGraphInformation.h"
#include "LyFlowGraphNotification.h"

#include <AzCore/Component/TickBus.h>

#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/UI/UICore/ProgressShield.hxx>

#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>
#include <QSettings>

CHyperGraphDialog* CHyperGraphDialog::s_instance = nullptr;

namespace
{
    const int FlowGraphLayoutVersion = 0x0004; // bump this up on every substantial pane layout change
}

#define GRAPH_FILE_FILTER "Graph XML Files (*.xml)"

#define HYPERGRAPH_DIALOGFRAME_CLASSNAME "HyperGraphDialog"

#define IDC_HYPERGRAPH_COMPONENTS 1
#define IDC_HYPERGRAPH_GRAPHS 2
#define IDC_COMPONENT_SEARCH 3
#define IDC_BREAKPOINTS 4

#define ID_PROPERTY_GROUP 1
#define ID_NODE_INFO_GROUP 2
#define ID_GRAPH_INFO_GROUP 3
#define ID_VARIABLES_GROUP 4

#define ID_COMPONENTS_GROUP 1

#define ID_NODE_INFO 100
#define ID_GRAPH_INFO 110


#define IMG_ENTITY      0
#define IMG_AI          1
#define IMG_FOLDER      2
#define IMG_DEFAULT     3
#define IMG_BIGX        4
#define IMG_SMALLX      5
#define IMG_UI          6
#define IMG_FX          7
#define IMG_FM          8
#define IMG_ERROR_OFFSET    9

namespace {
    QString g_GlobalFlowgraphsFolderName = "Global";
    QString g_LevelFlowgraphs = "Level";
    QString g_EntitiesFolderName = "Entities";
    QString g_AIActionsFolderName = "AI actions";
    QString g_CustomActionsFolderName = "Custom actions";
    QString g_GlobalFlowgraphModulesFolderName = "Modules";
    QString g_LevelFlowgraphModulesFolderName = "Modules";
    QString g_MaterialFXFolderName = "Material FX";
    QString g_ComponentEntityFolderName = "Components";
}

//////////////////////////////////////////////////////////////////////////
//
// CHyperGraphsTreeCtrl
//
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CHyperGraphsTreeCtrl::CHyperGraphsTreeCtrl(QWidget* parent)
    : QTreeWidget(parent)
    , m_pCurrentGraph(NULL)
{
    m_bIgnoreReloads = false;
    GetIEditor()->GetFlowGraphManager()->AddListener(this);
    int nSize = sizeof(&CHyperGraphsTreeCtrl::OnObjectEvent);
    GetIEditor()->GetObjectManager()->AddObjectEventListener(functor(*this, &CHyperGraphsTreeCtrl::OnObjectEvent));

    m_imageList << QIcon(":/HyperGraph/graphs/graph_entity.png");
    m_imageList << QIcon(":/HyperGraph/graphs/graph_ai.png");
    m_imageList << QIcon(":/HyperGraph/graphs/graph_folder.png");
    m_imageList << QIcon(":/HyperGraph/graphs/graph_default.png");
    m_imageList << QIcon(":/HyperGraph/graphs/graph_bigx.png");
    m_imageList << QIcon(":/HyperGraph/graphs/graph_smallx.png");
    m_imageList << QIcon(":/HyperGraph/graphs/graph_ui.png");
    m_imageList << QIcon(":/HyperGraph/graphs/graph_fx.png");
    m_imageList << QIcon(":/HyperGraph/graphs/graph_fm.png");
    m_imageList << QIcon(":/HyperGraph/graphs/graph_entity_error.png");
    m_imageList << QIcon(":/HyperGraph/graphs/graph_ai_error.png");
    m_imageList << QIcon(":/HyperGraph/graphs/graph_folder_error.png");
    m_imageList << QIcon(":/HyperGraph/graphs/graph_default_error.png");
    m_imageList << QIcon(":/HyperGraph/graphs/graph_bigx_error.png");
    m_imageList << QIcon(":/HyperGraph/graphs/graph_smallx_error.png");
    m_imageList << QIcon(":/HyperGraph/graphs/graph_ui_error.png");
    m_imageList << QIcon(":/HyperGraph/graphs/graph_fx_error.png");
    m_imageList << QIcon(":/HyperGraph/graphs/graph_fm_error.png");

    // to ensure that we get keyboard shortcuts
    setFocusPolicy(Qt::StrongFocus);
}

//////////////////////////////////////////////////////////////////////////
CHyperGraphsTreeCtrl::~CHyperGraphsTreeCtrl()
{
    //  GetIEditor()->GetAI()->SaveActionGraphs();
    GetIEditor()->GetFlowGraphManager()->RemoveListener(this);
    GetIEditor()->GetObjectManager()->RemoveObjectEventListener(functor(*this, &CHyperGraphsTreeCtrl::OnObjectEvent));
}


template <class T>
class VPtr
{
public:
    static T* asPtr(QVariant v)
    {
        return (T*)v.value<void*>();
    }

    static QVariant asQVariant(T* ptr)
    {
        return qVariantFromValue((void*)ptr);
    }
};

template <class T>
static QTreeWidgetItem* FindItemByData(QTreeWidgetItem* hItem, T* data)
{
    QVariant v = hItem->data(0, Qt::UserRole);
    if (v.isValid())
    {
        T* vd = VPtr<T>::asPtr(v);
        if (vd == data)
        {
            return hItem;
        }
    }

    for (int i = 0; i < hItem->childCount(); ++i)
    {
        QTreeWidgetItem* res = FindItemByData(hItem->child(i), data);
        if (res)
        {
            return res;
        }
    }

    return NULL;
}

template <class T>
static QTreeWidgetItem* FindItemByData(QTreeWidget* pTree, T* data)
{
    for (int i = 0; i < pTree->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem* res = FindItemByData(pTree->topLevelItem(i), data);
        if (res)
        {
            return res;
        }
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphsTreeCtrl::SetCurrentGraph(CHyperGraph* pCurrentGraph)
{
    if (pCurrentGraph == m_pCurrentGraph)
    {
        return;
    }

    m_pCurrentGraph = pCurrentGraph;
    QTreeWidgetItem* hItem = FindItemByData(this, pCurrentGraph);
    if (hItem)
    {
        // all parent nodes have to be expanded first
        QTreeWidgetItem* hParent = hItem;
        while (hParent = hParent->parent())
        {
            hParent->setExpanded(true);
        }
        selectionModel()->clear();
        hItem->setSelected(true);

        scrollToItem(hItem);
    }
}

CHyperGraph* CHyperGraphsTreeCtrl::GetItemData(QTreeWidgetItem* item)
{
    return VPtr<CHyperGraph>::asPtr(item->data(0, Qt::UserRole));
}

QTreeWidgetItem* CHyperGraphsTreeCtrl::GetTreeItem(CHyperGraph* pGraph)
{
    QTreeWidgetItem* hItem = FindItemByData(this, pGraph);
    return hItem;
}

BOOL CHyperGraphsTreeCtrl::DeleteNonEntityItem(CHyperGraph* pGraph, QTreeWidgetItem*& hAdjacentItem)
{
    if (pGraph == NULL)
    {
        return FALSE;
    }

    QTreeWidgetItem* hItem = GetTreeItem(pGraph);

    const bool isPartOfPrefab = pGraph->IsFlowGraph() && static_cast<CFlowGraph*>(pGraph)->GetEntity() && static_cast<CFlowGraph*>(pGraph)->GetEntity()->IsPartOfPrefab();

    if (hItem == NULL)
    {
        return FALSE;
    }
    else
    {
        if (!isPartOfPrefab)
        {
            hAdjacentItem = this->itemBelow(hItem); // GetNextSiblingItem(hItem);
            if (hAdjacentItem == NULL || hAdjacentItem->parent() != hItem->parent())
            {
                hAdjacentItem = this->itemAbove(hItem);  //GetPrevSiblingItem(hItem);
                if (hAdjacentItem == NULL || hAdjacentItem->parent() != hItem->parent())
                {
                    hAdjacentItem = hItem->parent();
                    if (hAdjacentItem == NULL)
                    {
                        hAdjacentItem = invisibleRootItem();
                    }
                }
            }
        }

        delete hItem;
        return TRUE;
    }

    return FALSE;
}


BOOL CHyperGraphsTreeCtrl::DeleteEntityItem(CEntityObject* pEntity, QTreeWidgetItem*& hAdjacentItem)
{
    if (pEntity == NULL)
    {
        return FALSE;
    }

    CHyperGraph* pGraph = pEntity->GetFlowGraph();

    if (pGraph == NULL)
    {
        return FALSE;
    }

    QTreeWidgetItem* hItem = GetTreeItem(pGraph);
    if (hItem == NULL)
    {
        return FALSE;
    }
    else
    {
        if (!pEntity->IsPartOfPrefab())
        {
            hAdjacentItem = itemBelow(hItem); // GetNextSiblingItem(hItem);
            if (hAdjacentItem == NULL || hAdjacentItem->parent() != hItem->parent())
            {
                hAdjacentItem = itemAbove(hItem); // GetPrevSiblingItem(hItem);
                if (hAdjacentItem == NULL || hAdjacentItem->parent() != hItem->parent())
                {
                    hAdjacentItem = hItem->parent(); // GetParentItem(hItem);
                    if (hAdjacentItem == NULL)
                    {
                        hAdjacentItem = invisibleRootItem();
                    }
                }
            }
        }

        delete hItem;
        if (m_mapEntityToItem.find(pEntity) != m_mapEntityToItem.end())
        {
            m_mapEntityToItem.erase(pEntity);
        }

        CPrefabObject* pPrefab = (CPrefabObject*)pEntity->GetGroup();
        if (pPrefab && m_mapPrefabToItem.find(pPrefab) != m_mapPrefabToItem.end())
        {
            m_mapPrefabToItem.erase(pPrefab);
        }

        return TRUE;
    }
    return FALSE;
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphsTreeCtrl::GetSelectedItems(TDTreeItems& rchSelectedTreeItems)
{
    rchSelectedTreeItems = selectedItems();
}
//////////////////////////////////////////////////////////////////////////

enum TreeItemOrder
{
    TVI_SORT,
    TVI_FIRST,
    TVI_LAST
};

static QTreeWidgetItem* InsertItem(QTreeWidget* pTree, const QString& title, const QIcon& icon)
{
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText(0, title);
    item->setIcon(0, icon);
    pTree->addTopLevelItem(item);
    return item;
}

static QTreeWidgetItem* InsertItem(const QString& title, const QIcon& icon, QTreeWidgetItem* pParent, TreeItemOrder order = TVI_SORT)
{
    Q_UNUSED(order);
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText(0, title);
    item->setIcon(0, icon);
    pParent->addChild(item);
    return item;
}

static inline QTreeWidgetItem* InsertItem(const char* title, const QIcon& icon, QTreeWidgetItem* pParent, TreeItemOrder order = TVI_SORT)
{
    return InsertItem(QString(title), icon, pParent, order);
}


struct SHGTCReloadItem
{
    SHGTCReloadItem(QString name, QString groupName, QTreeWidgetItem* parent, QIcon nImage, CFlowGraph* pFlowGraph, CEntityObject* pEntity = NULL)
    {
        // this->name.Format("0x%p %s", pFlowGraph, name.GetString());
        this->name = name;
        this->groupName = groupName;
        this->parent = parent;
        this->pFlowGraph = pFlowGraph;
        this->pEntity = pEntity;
        this->nImage = nImage;
    }
    QString name;
    QString groupName;
    QTreeWidgetItem* parent;
    CFlowGraph* pFlowGraph;
    CEntityObject* pEntity;
    QIcon nImage;

    bool operator<(const SHGTCReloadItem& rhs) const
    {
        int cmpResult = groupName.compare(rhs.groupName, Qt::CaseInsensitive);
        if (cmpResult != 0)
        {
            return cmpResult < 0;
        }
        return name.compare(rhs.name, Qt::CaseInsensitive) < 0;
    }

    QTreeWidgetItem* Add(CHyperGraphsTreeCtrl* pCtrl)
    {
        QTreeWidgetItem* hItem = InsertItem(name, nImage, parent);
        QVariant v = VPtr<CFlowGraph>::asQVariant(pFlowGraph);
        hItem->setData(0, Qt::UserRole, v);
        /*
        if (!pFlowGraph->IsEnabled())
        {
        LOGFONT logfont;
        pCtrl->GetItemFont( hItem,&logfont );
        logfont.lfStrikeOut = 1;
        pCtrl->SetItemFont( hItem,logfont );
        }
        */
        return hItem;
    }
};


//////////////////////////////////////////////////////////////////////////
void CHyperGraphsTreeCtrl::Reload()
{
    std::vector<SHGTCReloadItem> items;

    QStringList expandedFolders;
    {
        std::vector<QTreeWidgetItem*> workList;
        workList.push_back(invisibleRootItem());
        while (!workList.empty())
        {
            QTreeWidgetItem* item = workList.back();
            workList.pop_back();
            if (item->isExpanded())
            {
                expandedFolders << item->text(0);
            }
            for (int i = 0; i < item->childCount(); ++i)
            {
                if (item->child(i)->childCount())
                {
                    workList.push_back(item->child(i));
                }
            }
        }
    }

    clear();

    CFlowGraphManager* pMgr = GetIEditor()->GetFlowGraphManager();
    QTreeWidget* pTreeCtrl = this;

    m_mapEntityToItem.clear();
    m_mapPrefabToItem.clear();

    /*****************************Level Flowgraphs *****************************/
    /* Level Flowgraphs are the Flowgraphs that show up under the "Level Flowgraph"
    category inside the "Flowgraph Manager" pane. These are Flowgraphs that are scoped
    only to the specific "level" that they were created / placed in and have no meaning
    outside that "level"
    Prime examples of these are : Entity Flowgraphs and Level specific Flowgraph Modules*/

    // Level Flowgraphs
    // Folder that groups all Level Flowgraphs together
    QTreeWidgetItem* levelFlowgraphsGroup = InsertItem(pTreeCtrl, g_LevelFlowgraphs, m_imageList[IMG_FOLDER]);

    // Entity Flowgraphs
    QTreeWidgetItem* entityFlowgraphs = InsertItem(g_EntitiesFolderName, m_imageList[IMG_FOLDER], levelFlowgraphsGroup, TVI_FIRST);

    // Component Entity Flowgraphs
    QTreeWidgetItem* componentEntityFlowgraphs = InsertItem(g_ComponentEntityFolderName, m_imageList[IMG_FOLDER], levelFlowgraphsGroup, TVI_FIRST);

    // Level specific Flowgraph modules
    QTreeWidgetItem* levelFlowgraphModules = InsertItem(g_LevelFlowgraphModulesFolderName, m_imageList[IMG_FOLDER], levelFlowgraphsGroup, TVI_LAST);

    /*****************************Global Flowgraphs *****************************/
    /* Global Flowgraphs are the Flowgraphs that show up under the "Global Flowgraph"
    category inside the "Flowgraph Manager" pane. These are Flowgraphs that are scoped
    only to the specific "Project" that they were created / placed in and have no meaning
    outside that "project". All "levels" that belong to that project can share these
    Flowgraphs.
    Examples of these are : AI Action Flowgraphs and Global Flowgraph Modules*/

    // Global Flowgraphs
    // Folder that groups all Global Flowgraphs together
    QTreeWidgetItem* globalFlowgraphsGroup = InsertItem(pTreeCtrl, g_GlobalFlowgraphsFolderName, m_imageList[IMG_FOLDER]);

    // AI Actions
    QTreeWidgetItem* aiActionFlowgraphs = nullptr;

    if (GetISystem()->GetIFlowSystem() && GetISystem()->GetAISystem())
    {
        aiActionFlowgraphs = InsertItem(g_AIActionsFolderName, m_imageList[IMG_AI], globalFlowgraphsGroup, TVI_FIRST);
        if (expandedFolders.contains(aiActionFlowgraphs->text(0)))
        {
            aiActionFlowgraphs->setExpanded(true);
        }
    }

    // Material Effects
    QTreeWidgetItem* materialFXFlowgraphs = InsertItem(g_MaterialFXFolderName, m_imageList[IMG_FX], globalFlowgraphsGroup, TVI_LAST);

    // Custom Actions
    QTreeWidgetItem* customActionFlowgraphs = nullptr;

    if (GetISystem()->GetIGame() && GetISystem()->GetIGame()->GetIGameFramework()->GetICustomActionManager() != NULL)
    {
        customActionFlowgraphs = InsertItem(g_CustomActionsFolderName, m_imageList[IMG_FOLDER], globalFlowgraphsGroup, TVI_LAST);
        if (expandedFolders.contains(customActionFlowgraphs->text(0)))
        {
            customActionFlowgraphs->setExpanded(true);
        }
    }

    // Global Flowgraph Modules
    QTreeWidgetItem* globalFlowgraphModules = InsertItem(g_GlobalFlowgraphModulesFolderName, m_imageList[IMG_FOLDER], globalFlowgraphsGroup, TVI_LAST);

    /*  Prefab flowgraphs are a special case when it comes to scoping of Flowgraphs, they
    can be level specific or global depending on the context. It is apt to call these
    flowgraphs prefab scoped. i.e. Scoped to the flowgraph they are attached to.
    If a flowgraph is attached to a prefab that exists only in the "Level" prefab library
    then it is level specific and only available in the level the prefab is placed.
    However, if a prefab has been moved from the "Level" prefab library over to a new one
    which can, in theory, be shared by all flowgraphs in a "Project" then it is available
    globally
    One should be careful while using these and make sure that the scope you expect them
    to conform to, is actually the scope that they exist in */
    // All prefab flowgraphs
    QTreeWidgetItem* prefabFlowgraphs = InsertItem(pTreeCtrl, "Prefabs", m_imageList[IMG_FOLDER]);

    /*  External files are flowgraphs not attached to a level or a project, they are not
    saved or loaded along with a level but can be loaded from the flowgraph editor toolbar
    whenever a user wants to use them */
    // External file flowgraphs (not linked to a level or project)
    QTreeWidgetItem* externalFileFlowgraphs = InsertItem(pTreeCtrl, "External files", m_imageList[IMG_FOLDER]);

    entityFlowgraphs->setExpanded(true);
    if (expandedFolders.contains(levelFlowgraphsGroup->text(0)))
    {
        levelFlowgraphsGroup->setExpanded(true);
    }
    if (expandedFolders.contains(entityFlowgraphs->text(0)))
    {
        entityFlowgraphs->setExpanded(true);
    }
    if (expandedFolders.contains(levelFlowgraphModules->text(0)))
    {
        levelFlowgraphModules->setExpanded(true);
    }
    if (expandedFolders.contains(globalFlowgraphModules->text(0)))
    {
        globalFlowgraphModules->setExpanded(true);
    }
    if (expandedFolders.contains(materialFXFlowgraphs->text(0)))
    {
        materialFXFlowgraphs->setExpanded(true);
    }
    if (expandedFolders.contains(prefabFlowgraphs->text(0)))
    {
        prefabFlowgraphs->setExpanded(true);
    }
    if (expandedFolders.contains(externalFileFlowgraphs->text(0)))
    {
        externalFileFlowgraphs->setExpanded(true);
    }
    if (expandedFolders.contains(componentEntityFlowgraphs->text(0)))
    {
        componentEntityFlowgraphs->setExpanded(true);
    }

    struct NonCaseSensitiveString
        : public std::binary_function<QString, QString, bool>
    {
        bool operator()(const QString& left, const QString& right) const
        {
            return left.compare(right, Qt::CaseInsensitive) < 0;
        }
    };
    std::map<QString, QTreeWidgetItem*, NonCaseSensitiveString> groups[5];

    QTreeWidgetItem* hSelectedItem = 0;
    for (int i = 0; i < pMgr->GetFlowGraphCount(); i++)
    {
        CFlowGraph* pFlowGraph = pMgr->GetFlowGraph(i);
        CEntityObject* pEntity = pFlowGraph->GetEntity();
        IFlowGraph::EFlowGraphType type = pFlowGraph->GetIFlowGraph()->GetType();

        QString szGroupName = QStringLiteral("");
        const bool bHasError = pFlowGraph->HasError();
        const int img_offset = bHasError ? IMG_ERROR_OFFSET : 0;

        if (type == IFlowGraph::eFGT_Default && pEntity)
        {
            if (pEntity->GetGroup())
            {
                CGroup* pObjGroup = pEntity->GetGroup();
                assert(pObjGroup != 0);
                if (pObjGroup == 0)
                {
                    CryLogAlways("CHyperGraphsTreeCtrl::Reload(): Entity %p (%s) in Group but GroupPtr is 0",
                        pEntity, pEntity->GetName().toUtf8().data());
                    continue;
                }

                if (!pObjGroup->IsOpen())
                {
                    continue;
                }

                if (CPrefabObject* pPrefab = pEntity->GetPrefab())
                {
                    const QString pfGroupName = pPrefab->GetName();
#if 0
                    CryLogAlways("entity 0x%p name=%s group=%s",
                        pEntity,
                        pEntity->GetName().toUtf8().data(),
                        pfGroupName.toUtf8().data());
#endif

                    QTreeWidgetItem* hGroup = stl::find_in_map(groups[3], pfGroupName, NULL);
                    if (!hGroup && !pfGroupName.isEmpty())
                    {
                        hGroup = InsertItem(pfGroupName, m_imageList[IMG_FOLDER], prefabFlowgraphs, TVI_SORT);
                        groups[3][pfGroupName] = hGroup;
                        m_mapPrefabToItem[pPrefab] = hGroup;
                    }
                    if (bHasError)
                    {
                        if (hGroup)
                        {
                            hGroup->setIcon(0, m_imageList[IMG_FOLDER + IMG_ERROR_OFFSET]);
                        }
                        prefabFlowgraphs->setIcon(0, m_imageList[IMG_FOLDER + IMG_ERROR_OFFSET]);
                    }
                    if (!hGroup)
                    {
                        hGroup = prefabFlowgraphs;
                        szGroupName = QStringLiteral("Prefabs");
                    }
                    else
                    {
                        szGroupName = pfGroupName;
                    }
                    items.push_back(SHGTCReloadItem(pEntity->GetName(), szGroupName, hGroup, m_imageList[pFlowGraph->IsEnabled() ? IMG_ENTITY + img_offset : IMG_BIGX + img_offset], pFlowGraph, pEntity));
                }
                else
                {
                    QTreeWidgetItem* hGroup = stl::find_in_map(groups[0], pFlowGraph->GetGroupName(), NULL);
                    if (!hGroup && !pFlowGraph->GetGroupName().isEmpty())
                    {
                        hGroup = InsertItem(pFlowGraph->GetGroupName(), m_imageList[IMG_FOLDER], entityFlowgraphs, TVI_SORT);
                        groups[0][pFlowGraph->GetGroupName()] = hGroup;
                    }
                    if (bHasError)
                    {
                        if (hGroup)
                        {
                            hGroup->setIcon(0, m_imageList[IMG_FOLDER + IMG_ERROR_OFFSET]);
                        }

                        entityFlowgraphs->setIcon(0, m_imageList[IMG_FOLDER + IMG_ERROR_OFFSET]);
                        levelFlowgraphsGroup->setIcon(0, m_imageList[IMG_FOLDER + IMG_ERROR_OFFSET]);
                    }
                    if (!hGroup)
                    {
                        hGroup = entityFlowgraphs;
                        szGroupName = g_EntitiesFolderName;
                    }
                    else
                    {
                        szGroupName = pFlowGraph->GetGroupName();
                    }

                    QString reloadName = QString("[%1] %2").arg(pObjGroup->GetName(), pEntity->GetName());
                    items.push_back(SHGTCReloadItem(reloadName, szGroupName, hGroup, m_imageList[pFlowGraph->IsEnabled() ? IMG_ENTITY + img_offset : IMG_BIGX + img_offset], pFlowGraph, pEntity));
                }
            }
            else
            {
                QTreeWidgetItem* hGroup = stl::find_in_map(groups[0], pFlowGraph->GetGroupName(), NULL);
                if (!hGroup && !pFlowGraph->GetGroupName().isEmpty())
                {
                    hGroup = InsertItem(pFlowGraph->GetGroupName(), m_imageList[IMG_FOLDER], entityFlowgraphs, TVI_SORT);
                    groups[0][pFlowGraph->GetGroupName()] = hGroup;
                }

                if (bHasError)
                {
                    if (hGroup)
                    {
                        hGroup->setIcon(0, m_imageList[IMG_FOLDER + IMG_ERROR_OFFSET]);
                    }

                    entityFlowgraphs->setIcon(0, m_imageList[IMG_FOLDER + IMG_ERROR_OFFSET]);
                }
                if (!hGroup)
                {
                    hGroup = entityFlowgraphs;
                    szGroupName = g_EntitiesFolderName;
                }
                else
                {
                    szGroupName = pFlowGraph->GetGroupName();
                }

                items.push_back(SHGTCReloadItem(pEntity->GetName(), szGroupName,
                        hGroup, m_imageList[pFlowGraph->IsEnabled() ? IMG_ENTITY + img_offset : IMG_BIGX + img_offset], pFlowGraph, pEntity));
            }
        }
        else if (type == IFlowGraph::eFGT_AIAction)
        {
            IAIAction* pAIAction = pFlowGraph->GetAIAction();
            if (!pAIAction || pAIAction->GetUserEntity() || pAIAction->GetObjectEntity())
            {
                continue;
            }

            QTreeWidgetItem* hGroup = stl::find_in_map(groups[1], pFlowGraph->GetGroupName(), NULL);
            if (!hGroup && !pFlowGraph->GetGroupName().isEmpty())
            {
                hGroup = InsertItem(pFlowGraph->GetGroupName(), m_imageList[IMG_FOLDER], aiActionFlowgraphs, TVI_SORT);
                groups[1][pFlowGraph->GetGroupName()] = hGroup;
                szGroupName = pFlowGraph->GetGroupName();
            }

            if (bHasError)
            {
                if (hGroup)
                {
                    hGroup->setIcon(0, m_imageList[IMG_FOLDER + IMG_ERROR_OFFSET]);
                }

                aiActionFlowgraphs->setIcon(0, m_imageList[IMG_AI + IMG_ERROR_OFFSET]);
                globalFlowgraphsGroup->setIcon(0, m_imageList[IMG_FOLDER + IMG_ERROR_OFFSET]);
            }
            if (!hGroup)
            {
                hGroup = aiActionFlowgraphs;
                szGroupName = g_AIActionsFolderName;
            }
            else
            {
                szGroupName = pFlowGraph->GetGroupName();
            }

            items.push_back(SHGTCReloadItem(pAIAction->GetName(), szGroupName, hGroup, m_imageList[IMG_AI + img_offset], pFlowGraph));
        }
        else if (type == IFlowGraph::eFGT_CustomAction)
        {
            ICustomAction* pCustomAction = pFlowGraph->GetCustomAction();
            if (!pCustomAction || pCustomAction->GetObjectEntity())
            {
                continue;
            }

            QTreeWidgetItem* hGroup = stl::find_in_map(groups[4], pFlowGraph->GetGroupName(), NULL);
            if (!hGroup && !pFlowGraph->GetGroupName().isEmpty())
            {
                hGroup = InsertItem(pFlowGraph->GetGroupName(), m_imageList[2], customActionFlowgraphs, TVI_SORT);
                groups[4][pFlowGraph->GetGroupName()] = hGroup;
                szGroupName = pFlowGraph->GetGroupName();
            }
            if (bHasError)
            {
                if (hGroup)
                {
                    hGroup->setIcon(0, m_imageList[IMG_FOLDER + IMG_ERROR_OFFSET]);
                }

                customActionFlowgraphs->setIcon(0, m_imageList[IMG_UI + IMG_ERROR_OFFSET]);
                globalFlowgraphsGroup->setIcon(0, m_imageList[IMG_FOLDER + IMG_ERROR_OFFSET]);
            }

            if (!hGroup)
            {
                hGroup = customActionFlowgraphs;
                szGroupName = g_CustomActionsFolderName;
            }
            else
            {
                szGroupName = pFlowGraph->GetGroupName();
            }

            items.push_back(SHGTCReloadItem(pCustomAction->GetCustomActionGraphName(), szGroupName, hGroup, m_imageList[3], pFlowGraph));
        }
        else if (type == IFlowGraph::eFGT_Module)
        {
            if (IFlowGraphModule* pModule = gEnv->pFlowSystem->GetIModuleManager()->GetModule(pFlowGraph->GetName().toUtf8().data()))
            {
                IFlowGraphModule::EType moduleType = pModule->GetType();

                QTreeWidgetItem* hGroup = stl::find_in_map(groups[1], pFlowGraph->GetGroupName(), NULL);
                if (!hGroup && !pFlowGraph->GetGroupName().isEmpty())
                {
                    QTreeWidgetItem* parent = nullptr;
                    switch (moduleType)
                    {
                    case IFlowGraphModule::eT_Global:
                        parent = globalFlowgraphModules;
                        break;
                    case IFlowGraphModule::eT_Level:
                        parent = levelFlowgraphModules;
                        break;
                    default:
                        CRY_ASSERT_MESSAGE(false, "Unknown module type encountered!");
                        return;
                    }
                    hGroup = InsertItem(pFlowGraph->GetGroupName(), m_imageList[IMG_FOLDER], parent, TVI_SORT);
                    groups[1][pFlowGraph->GetGroupName()] = hGroup;
                    szGroupName = pFlowGraph->GetGroupName();
                }

                if (bHasError)
                {
                    if (hGroup)
                    {
                        hGroup->setIcon(0, m_imageList[IMG_FOLDER + IMG_ERROR_OFFSET]);
                    }

                    switch (moduleType)
                    {
                    case IFlowGraphModule::eT_Global:
                        globalFlowgraphModules->setIcon(0, m_imageList[IMG_FOLDER + IMG_ERROR_OFFSET]);
                        globalFlowgraphsGroup->setIcon(0, m_imageList[IMG_FOLDER + IMG_ERROR_OFFSET]);
                        break;
                    case IFlowGraphModule::eT_Level:
                        levelFlowgraphModules->setIcon(0, m_imageList[IMG_FOLDER + IMG_ERROR_OFFSET]);
                        levelFlowgraphsGroup->setIcon(0, m_imageList[IMG_FOLDER + IMG_ERROR_OFFSET]);
                        break;
                    default:
                        CRY_ASSERT_MESSAGE(false, "Unknown module type encountered!");
                        return;
                    }
                }
                if (!hGroup)
                {
                    switch (moduleType)
                    {
                    case IFlowGraphModule::eT_Global:
                        hGroup = globalFlowgraphModules;
                        szGroupName = g_GlobalFlowgraphModulesFolderName;
                        break;
                    case IFlowGraphModule::eT_Level:
                        hGroup = levelFlowgraphModules;
                        szGroupName = g_LevelFlowgraphModulesFolderName;
                        break;
                    default:
                        CRY_ASSERT_MESSAGE(false, "Unknown module type encountered!");
                        return;
                    }
                }
                else
                {
                    szGroupName = pFlowGraph->GetGroupName();
                }

                items.push_back(SHGTCReloadItem(pFlowGraph->GetName(), szGroupName, hGroup, m_imageList[IMG_FM + img_offset], pFlowGraph));
            }
        }
        else if (type == IFlowGraph::eFGT_MaterialFx)
        {
            QTreeWidgetItem* hGroup = stl::find_in_map(groups[1], pFlowGraph->GetGroupName(), NULL);
            if (!hGroup && !pFlowGraph->GetGroupName().isEmpty())
            {
                hGroup = InsertItem(pFlowGraph->GetGroupName(), m_imageList[IMG_FOLDER], materialFXFlowgraphs, TVI_SORT);
                groups[1][pFlowGraph->GetGroupName()] = hGroup;
                szGroupName = pFlowGraph->GetGroupName();
            }
            if (bHasError)
            {
                if (hGroup)
                {
                    hGroup->setIcon(0, m_imageList[IMG_FOLDER + IMG_ERROR_OFFSET]);
                }

                materialFXFlowgraphs->setIcon(0, m_imageList[IMG_FX + IMG_ERROR_OFFSET]);
                globalFlowgraphsGroup->setIcon(0, m_imageList[IMG_FOLDER + IMG_ERROR_OFFSET]);
            }
            if (!hGroup)
            {
                hGroup = materialFXFlowgraphs;
                szGroupName = g_MaterialFXFolderName;
            }
            else
            {
                szGroupName = pFlowGraph->GetGroupName();
            }

            items.push_back(SHGTCReloadItem(pFlowGraph->GetName(), szGroupName, hGroup, m_imageList[IMG_FX + img_offset], pFlowGraph));
        }
        else if (type == IFlowGraph::eFGT_FlowGraphComponent)
        {
            QTreeWidgetItem* hGroup = stl::find_in_map(groups[1], pFlowGraph->GetGroupName(), nullptr);
            if (!hGroup && !pFlowGraph->GetGroupName().isEmpty())
            {
                hGroup = InsertItem(pFlowGraph->GetGroupName(), m_imageList[IMG_FOLDER], componentEntityFlowgraphs, TVI_SORT);
                groups[1][pFlowGraph->GetGroupName()] = hGroup;
                szGroupName = pFlowGraph->GetGroupName();
            }
            if (bHasError)
            {
                if (hGroup)
                {
                    hGroup->setIcon(0, m_imageList[IMG_FOLDER + IMG_ERROR_OFFSET]);
                }

                componentEntityFlowgraphs->setIcon(0, m_imageList[IMG_FOLDER + IMG_ERROR_OFFSET]);
                globalFlowgraphsGroup->setIcon(0, m_imageList[IMG_FOLDER + IMG_ERROR_OFFSET]);
            }
            if (!hGroup)
            {
                hGroup = componentEntityFlowgraphs;
                szGroupName = g_ComponentEntityFolderName;
            }
            else
            {
                szGroupName = pFlowGraph->GetGroupName();
            }
            items.push_back(SHGTCReloadItem(pFlowGraph->GetName(), szGroupName, hGroup,
                    m_imageList[IMG_FOLDER + img_offset], pFlowGraph));
        }
        else
        {
            QTreeWidgetItem* hGroup = stl::find_in_map(groups[2], pFlowGraph->GetGroupName(), NULL);
            if (!hGroup && !pFlowGraph->GetGroupName().isEmpty())
            {
                hGroup = InsertItem(pFlowGraph->GetGroupName(), m_imageList[IMG_FOLDER], externalFileFlowgraphs, TVI_SORT);
                groups[2][pFlowGraph->GetGroupName()] = hGroup;
                szGroupName = pFlowGraph->GetGroupName();
            }
            if (bHasError)
            {
                if (hGroup)
                {
                    hGroup->setIcon(0, m_imageList[IMG_FOLDER + IMG_ERROR_OFFSET]);
                }

                externalFileFlowgraphs->setIcon(0, m_imageList[IMG_FOLDER + IMG_ERROR_OFFSET]);
            }
            if (!hGroup)
            {
                hGroup = externalFileFlowgraphs;
                szGroupName = QStringLiteral("External Files");
            }
            else
            {
                szGroupName = pFlowGraph->GetGroupName();
            }

            items.push_back(SHGTCReloadItem(pFlowGraph->GetName(), szGroupName, hGroup, m_imageList[pFlowGraph->IsEnabled() ? IMG_DEFAULT + img_offset : IMG_SMALLX + img_offset], pFlowGraph));
        }
    }

    std::sort(items.begin(), items.end());

    for (size_t i = 0; i < items.size(); ++i)
    {
        QTreeWidgetItem* hItem = items[i].Add(this);
        if (items[i].pEntity)
        {
            m_mapEntityToItem[items[i].pEntity] = hItem;
        }
        if (m_pCurrentGraph && (m_pCurrentGraph == items[i].pFlowGraph))
        {
            hSelectedItem = hItem;
        }
    }

    for (int i = 0; i < 5; ++i)
    {
        const auto& map = groups[i];
        for (auto item : map)
        {
            SortChildren(item.second);
        }
    }

    if (hSelectedItem)
    {
        hSelectedItem->setSelected(true);
    }
}

void CHyperGraphsTreeCtrl::SortChildren(QTreeWidgetItem* parent)
{
    parent->sortChildren(0, Qt::AscendingOrder);
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphsTreeCtrl::OnHyperGraphManagerEvent(EHyperGraphEvent event, IHyperGraph* pGraph, IHyperNode* pNode)
{
    if (m_bIgnoreReloads == true)
    {
        return;
    }

    switch (event)
    {
    case EHG_GRAPH_ADDED:
    case EHG_GRAPH_REMOVED:
    case EHG_GRAPH_OWNER_CHANGE:
        Reload();
        // Invalidate(TRUE);
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphsTreeCtrl::OnObjectEvent(CBaseObject* pObject, int nEvent)
{
    switch (nEvent)
    {
    case CBaseObject::ON_RENAME:
        if (qobject_cast<CEntityObject*>(pObject))
        {
            QTreeWidgetItem* hItem = stl::find_in_map(m_mapEntityToItem, (CEntityObject*)pObject, NULL);
            if (hItem)
            {
                // Rename this item.
                hItem->setText(0, pObject->GetName());
                QTreeWidgetItem* hParent = hItem->parent();
                if (hParent)
                {
                    SortChildren(hParent);
                }
            }
        }
        else if (qobject_cast<CPrefabObject*>(pObject))
        {
            QTreeWidgetItem* hItem = stl::find_in_map (m_mapPrefabToItem, (CPrefabObject*)pObject, NULL);
            if (hItem)
            {
                hItem->setText(0, pObject->GetName());
                QTreeWidgetItem* hParent = hItem->parent();
                if (hParent)
                {
                    SortChildren(hParent);
                }
                CHyperGraphDialog* pWnd = CHyperGraphDialog::instance();
                if (pWnd)
                {
                    pWnd->update();
                }
            }
        }
        break;


    case CBaseObject::ON_ADD:
        if (qobject_cast<CPrefabObject*>(pObject))
        {
            CPrefabObject* pPrefab = (CPrefabObject*) pObject;
            // get all children and see if one Entity is there, which we already know of.
            // not very efficient...
            struct Recurser
            {
                Recurser(std::map<CEntityObject*, QTreeWidgetItem*>& mapEntityToItem)
                    : m_mapEntityToItem(mapEntityToItem)
                    , m_bHasFG(false)
                    , m_bAbort(false) {}
                inline void DoIt(CBaseObject* pObject)
                {
                    if (qobject_cast<CEntityObject*>(pObject) == nullptr)
                    {
                        return;
                    }

                    QTreeWidgetItem* hEntityItem = stl::find_in_map(m_mapEntityToItem, (CEntityObject*)pObject, NULL);
                    m_bHasFG = (hEntityItem != 0);
                    m_bAbort = m_bHasFG;
                }
                void Recurse(CBaseObject* pObject)
                {
                    DoIt(pObject);
                    if (m_bAbort)
                    {
                        return;
                    }
                    int nChilds = pObject->GetChildCount();
                    for (int i = 0; i < nChilds; ++i)
                    {
                        Recurse(pObject->GetChild(i));
                        if (m_bAbort)
                        {
                            return;
                        }
                    }
                }
                std::map<CEntityObject*, QTreeWidgetItem*>& m_mapEntityToItem;
                bool m_bHasFG;
                bool m_bAbort;
            };

            Recurser recurser(m_mapEntityToItem);
            recurser.Recurse(pPrefab);
            if (recurser.m_bHasFG)
            {
                Reload();
            }
        }
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
namespace
{
    struct NodeFilter
    {
    public:
        NodeFilter(uint32 mask)
            : mask(mask) {}
        bool Visit (CHyperNode* pNode)
        {
            if (pNode->IsEditorSpecialNode())
            {
                return false;
            }
            CFlowNode* pFlowNode = static_cast<CFlowNode*> (pNode);
            if ((pFlowNode->GetCategory() & mask) == 0)
            {
                return false;
            }
            return true;

            // Only if the usage mask is set check if fulfilled -> this is an exclusive thing
            if ((mask & EFLN_USAGE_MASK) != 0 && (pFlowNode->GetUsageFlags() & mask) == 0)
            {
                return false;
            }
            return true;
        }
        uint32 mask;
    };
};

//////////////////////////////////////////////////////////////////////////
class CFlowGraphManagerPrototypeModel
    : public QAbstractQVariantTreeDataModel
{
public:
    CFlowGraphManagerPrototypeModel(QObject* parent = 0)
        : QAbstractQVariantTreeDataModel(parent)
        , m_mask(0) { }

    void SetComponentFilterMask(uint32 mask) { m_mask = mask; }
    void Reload();

    Qt::DropActions supportedDragActions() const Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex& index) const Q_DECL_OVERRIDE;

    int columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

    QStringList mimeTypes() const Q_DECL_OVERRIDE;
    QMimeData* mimeData(const QModelIndexList& indexes) const Q_DECL_OVERRIDE;

    friend class CFlowGraphManagerPrototypeFilteredModel;

    enum
    {
        CategoryRole = Qt::UserRole,
        ClassRole,
        FilteringRole,
    };

private:
    uint32 m_mask;
};


void CFlowGraphManagerPrototypeModel::Reload()
{
    beginResetModel();
    m_root.reset(new Folder("root"));

    std::map<QString, Folder*> groupMap;

    CFlowGraphManager* pMgr = GetIEditor()->GetFlowGraphManager();
    std::vector<_smart_ptr<CHyperNode> > prototypes;
    NodeFilter filter(m_mask);

    pMgr->GetPrototypesEx(prototypes, true, functor_ret(filter, &NodeFilter::Visit));
    for (int i = 0; i < prototypes.size(); i++)
    {
        const CHyperNode* pNode = prototypes[i];
        const CFlowNode* pFlowNode = static_cast<const CFlowNode*> (pNode);

        QString fullClassName = pFlowNode->GetUIClassName();

        Folder* pItemGroupRec = 0;
        int midPos = 0;
        int pos = 0;
        int len = fullClassName.length();
        bool bUsePrototypeName = false; // use real prototype name or stripped from fullClassName

        QString nodeShortName;
        QStringList tokens = fullClassName.split(":");
        QString groupName;

        // check if a group name is given. if not, fake 'Misc:' group
        if (tokens.size() < 2)
        {
            fullClassName = "Misc:" + fullClassName;
            tokens = fullClassName.split(":");
        }

        assert(tokens.size() > 1);
        // short node name without ':'. used for display in last column
        nodeShortName = tokens.back();
        tokens.pop_back();

        QString pathName = "";
        while (tokens.size())
        {
            groupName = tokens.front();
            pathName += groupName;
            pathName += ":";

            Folder* pGroupRec = stl::find_in_map(groupMap, pathName, 0);
            if (pGroupRec == 0)
            {
                pGroupRec = new Folder(groupName);

                if (pItemGroupRec != 0)
                {
                    pGroupRec->m_parent = pItemGroupRec;
                    pItemGroupRec->m_children.push_back(std::shared_ptr<Item>(pGroupRec));
                }
                else
                {
                    pGroupRec->m_parent = m_root.get();
                    m_root->m_children.push_back(std::shared_ptr<Item>(pGroupRec));
                }

                groupMap[pathName] = pGroupRec;
                pItemGroupRec = pGroupRec;
            }
            else
            {
                pItemGroupRec = pGroupRec;
            }

            tokens.pop_front();
        }

        std::shared_ptr<Item> pRec(new Item);
        pRec->m_data.insert(Qt::DisplayRole, nodeShortName);
        pRec->m_data.insert(CategoryRole, pFlowNode->GetCategoryName());
        pRec->m_data.insert(ClassRole, fullClassName);

        switch (pFlowNode->GetCategory())
        {
        case EFLN_APPROVED:
            pRec->m_data.insert(Qt::TextColorRole, QColor(104, 215, 142));
            break;
        case EFLN_ADVANCED:
            pRec->m_data.insert(Qt::TextColorRole, QColor(104, 162, 255));
            break;
        case EFLN_DEBUG:
            pRec->m_data.insert(Qt::TextColorRole, QColor(220, 180, 20));
            break;
        case EFLN_OBSOLETE:
            pRec->m_data.insert(Qt::TextColorRole, QColor(255, 0, 0));
            break;
        default:
            pRec->m_data.insert(Qt::TextColorRole, QColor(0, 0, 0));
            break;
        }

        assert(pItemGroupRec);
        pRec->m_parent = pItemGroupRec;
        pItemGroupRec->m_children.push_back(pRec);

        // FilteringRole data contains all searchable text
        QStringList filterStrings;
        filterStrings.append(fullClassName);
        filterStrings.append(pFlowNode->GetCategoryName());
        filterStrings.append(pFlowNode->GetDescription());
        pRec->m_data.insert(FilteringRole, filterStrings.join('\n'));
    }

    //  std::vector<Item*> worklist;
    //  worklist.push_back(m_root.get());
    //  while (!worklist.empty())
    //  {
    //      Item* item = worklist.back();
    //      worklist.pop_back();
    //      if (item->asFolder()) {
    //          for (auto child : item->asFolder()->m_children)
    //              worklist.push_back(child.get());
    //      } else {
    //          QStringList path;
    //          while (item)
    //          {
    //              path.push_front( item->m_data[Qt::DisplayRole].toString() );
    //              item = item->m_parent;
    //          }
    //          qDebug() << path;
    //      }
    //  }

    endResetModel();
}

int CFlowGraphManagerPrototypeModel::columnCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
    return 2;
}

Qt::ItemFlags CFlowGraphManagerPrototypeModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    Item* item = itemFromIndex(index);
    if (item && !item->asFolder())
    {
        flags |= Qt::ItemIsDragEnabled;
    }
    return flags;
}

QVariant CFlowGraphManagerPrototypeModel::data(const QModelIndex& index, int role /*= Qt::DisplayRole*/) const
{
    Item* item = itemFromIndex(index);
    if (0 == item)
    {
        return QVariant();
    }

    switch (role)
    {
    case Qt::DisplayRole:
        if (index.column() == 1)
        {
            role = CategoryRole;
        }
        break;
    case Qt::TextColorRole:
        break;
    case Qt::DecorationRole:
    {
        if (index.column() == 1)
        {
            return QVariant();
        }
        if (item->m_data.contains(role))
        {
            return item->m_data[role];
        }

        // it's the same for all except folders, just use the one
        static QPixmap decoration;
        if (decoration.isNull())
        {
            decoration = QPixmap(10, 10);
            QPainter p(&decoration);
            p.fillRect(decoration.rect(), QColor(255, 0, 0));
        }
        return decoration;
    }
    default:
        if (index.column() == 1)
        {
            return QVariant();
        }
        break;
    }

    if (item->m_data.contains(role))
    {
        return item->m_data[role];
    }
    return QVariant();
}

QVariant CFlowGraphManagerPrototypeModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
{
    if (role == Qt::DisplayRole)
    {
        if (section == 0)
        {
            return "NodeClass";
        }
        if (section == 1)
        {
            return "Category";
        }
    }
    return QAbstractItemModel::headerData(section, orientation, role);
}

Qt::DropActions CFlowGraphManagerPrototypeModel::supportedDragActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

QStringList CFlowGraphManagerPrototypeModel::mimeTypes() const
{
    QStringList types;
    types << "application/x-component-prototype";
    return types;
}

QMimeData* CFlowGraphManagerPrototypeModel::mimeData(const QModelIndexList& indexes) const
{
    QMimeData* mimeData = new QMimeData();
    QByteArray encodedData;

    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    Q_FOREACH(const QModelIndex &index, indexes)
    {
        if (index.isValid())
        {
            QString text = data(index, ClassRole).toString();
            stream << text;
        }
    }

    mimeData->setData("application/x-component-prototype", encodedData);
    return mimeData;
}

//////////////////////////////////////////////////////////////////////////
class CFlowGraphManagerPrototypeFilteredModel
    : public QSortFilterProxyModel
{
public:
    CFlowGraphManagerPrototypeFilteredModel(QObject* parent = 0)
        : QSortFilterProxyModel(parent)
    {
        setFilterKeyColumn(0);
        setFilterRole(CFlowGraphManagerPrototypeModel::FilteringRole);
        setFilterCaseSensitivity(Qt::CaseInsensitive);
        setSourceModel(m_sourceModel = new CFlowGraphManagerPrototypeModel(this));
    }
    void SetComponentFilterMask(uint32 mask) { m_sourceModel->SetComponentFilterMask(mask); }
    void Reload() { m_sourceModel->Reload(); }
    void SetSearchKeyword(const QString& words);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const Q_DECL_OVERRIDE;

private:
    CFlowGraphManagerPrototypeModel*    m_sourceModel;
};

void CFlowGraphManagerPrototypeFilteredModel::SetSearchKeyword(const QString& words)
{
    QStringList workList = words.split(" ", QString::SkipEmptyParts);
    setFilterRegExp("(" + workList.join("|") + ")");
}

bool CFlowGraphManagerPrototypeFilteredModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    if (filterRegExp().isEmpty())
    {
        return true;
    }

    QModelIndex sourceIndex = m_sourceModel->index(source_row, 0, source_parent);
    int nChildren = m_sourceModel->rowCount(sourceIndex);
    if (nChildren)
    {
        // hide empty folders
        // this is n^2, but do we care on small trees?
        // Alternative is to do as the original implementation, ie rebuild the whole tree
        // or maybe add another proxy model which would hide empty folders
        for (int i = 0; i < nChildren; i++)
        {
            bool filteredChild = filterAcceptsRow(i, sourceIndex);
            if (filteredChild)
            {
                return true;
            }
        }

        return false;
    }

    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}


//////////////////////////////////////////////////////////////////////////
QHyperGraphComponentsPanel::QHyperGraphComponentsPanel(QWidget* parent)
    : AzQtComponents::StyledDockWidget(parent)
    , m_model(0)
{
    QWidget* centralWidget = new QWidget;
    ui = new Ui::QHyperGraphComponentsPanel;
    ui->setupUi(centralWidget);
    setWidget(centralWidget);
    centralWidget->setFocusPolicy(Qt::StrongFocus);
    setWindowTitle(centralWidget->windowTitle());

    m_model = new CFlowGraphManagerPrototypeFilteredModel(this);
    ui->treeView->setModel(m_model);
    //  ui->treeView->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->treeView->header()->resizeSection(0, 180);

    connect(ui->lineEdit, &QLineEdit::textChanged, [=](const QString& str)
    {
        m_model->SetSearchKeyword(str);
    });

    ui->treeView->setContextMenuPolicy(Qt::ActionsContextMenu);
    ui->treeView->addAction(ui->actionShow_Header);
    ui->actionShow_Header->setChecked(true);
    connect(ui->actionShow_Header, &QAction::triggered, ui->treeView->header(), &QHeaderView::setVisible);

    ui->treeView->setFocusPolicy(Qt::StrongFocus);
}

void QHyperGraphComponentsPanel::SetComponentFilterMask(uint32 mask)
{
    m_model->SetComponentFilterMask(mask);
}

void QHyperGraphComponentsPanel::Reload()
{
    m_model->Reload();
}

//////////////////////////////////////////////////////////////////////////
class QHyperGraphTaskPanel
    : public AzQtComponents::StyledDockWidget
{
public:
    QHyperGraphTaskPanel(CHyperGraphDialog* parent);
    ~QHyperGraphTaskPanel();

    void SetSelectedNode(CHyperNode* node);
    void UpdateGraphProperties(CHyperGraph* graph)
    {
        ui->graphProperties->UpdateGraphProperties(graph);
    }

    CFlowGraphProperties* GetGraphProperties() { return ui->graphProperties; }
    CFlowGraphTokensCtrl* GetTokensCtrl() { return ui->graphTokens; }
    ReflectedPropertyControl* GetPropertyControl() { return ui->propertyCtrl; }

private:
    CHyperGraphDialog* m_dialog;
    QScopedPointer<Ui::QHyperGraphTaskPanel>    ui;
};

QHyperGraphTaskPanel::QHyperGraphTaskPanel(CHyperGraphDialog* parent)
    : AzQtComponents::StyledDockWidget(parent)
    , m_dialog(parent)
    , ui(new Ui::QHyperGraphTaskPanel)
{
    QWidget* centralWidget = new QWidget;
    ui->setupUi(centralWidget);
    ui->propertyCtrl->Setup(true, 135);
    setWidget(centralWidget);
    setWindowTitle(centralWidget->windowTitle());

    ui->selectedNodeProperties->setText("");
    ui->graphProperties->SetDialog(m_dialog);

    ui->propertyCtrl->ExpandAll();
}

QHyperGraphTaskPanel::~QHyperGraphTaskPanel()
{
}

void QHyperGraphTaskPanel::SetSelectedNode(CHyperNode* pNode)
{
    if (pNode)
    {
        ui->graphProperties->SetDescription(pNode->GetDescription());
    }
    else
    {
        ui->graphProperties->SetDescription("");
    }

    QString nodeInfo;
    if (pNode)
    {
        nodeInfo = QString(tr("Name: %1\n")).arg(pNode->GetName());
        nodeInfo += QString(tr(" Class: %1\n")).arg(pNode->GetClassName());

        if (!pNode->IsEditorSpecialNode())
        {
            CFlowNode* pFlowNode = static_cast<CFlowNode*> (pNode);

            if (gSettings.bFlowGraphShowNodeIDs)
            {
                nodeInfo += QString(tr(" nNodeID: %1\n")).arg(pFlowNode->GetId());
            }

            nodeInfo += QString(tr(" Category: %1\n")).arg(pFlowNode->GetCategoryName());
        }
    }
    ui->selectedNodeProperties->setText(nodeInfo);
}


//////////////////////////////////////////////////////////////////////////

class QBreakpointsPanel
    : public AzQtComponents::StyledDockWidget
{
public:
    QBreakpointsPanel(QWidget* parent);

    CBreakpointsTreeCtrl* treeCtrl() { return m_breakpointsTreeCtrl; }

private:
    CBreakpointsTreeCtrl* m_breakpointsTreeCtrl;
};

QBreakpointsPanel::QBreakpointsPanel(QWidget* parent)
    : AzQtComponents::StyledDockWidget(parent)
    , m_breakpointsTreeCtrl(new CBreakpointsTreeCtrl(this))
{
    setWindowTitle("Breakpoints");
    setWidget(m_breakpointsTreeCtrl);
}

//////////////////////////////////////////////////////////////////////////

// static
const GUID& CHyperGraphDialog::GetClassID()
{
    static const GUID guid = {
        0xde6e69f3, 0x8b53, 0x47de, { 0x89, 0xff, 0x11, 0x48, 0x58, 0x52, 0xb6, 0xc9 }
    };
    return guid;
}

void CHyperGraphDialog::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions options;
    options.sendViewPaneNameBackToAmazonAnalyticsServers = true;
    options.isLegacy = true;
    AzToolsFramework::RegisterViewPane<CHyperGraphDialog>(LyViewPane::LegacyFlowGraph, LyViewPane::CategoryTools, options);
}

CHyperGraphDialog* CHyperGraphDialog::instance()
{
    return s_instance;
}

//////////////////////////////////////////////////////////////////////////
CHyperGraphDialog::CHyperGraphDialog()
    : m_widget(0)
    , m_pNavGraph(0)
    , m_componentsTaskPanel(0)
{
    s_instance = this;
    GetIEditor()->RegisterNotifyListener(this);
    GetIEditor()->GetFlowGraphManager()->AddListener(this);

    m_pFlowGraphDebugger = GetIFlowGraphDebuggerPtr();

    OnInitDialog();
}

//////////////////////////////////////////////////////////////////////////
CHyperGraphDialog::~CHyperGraphDialog()
{
    if (GetIEditor()->GetFlowGraphManager())
    {
        GetIEditor()->GetFlowGraphManager()->RemoveListener(this);
    }

    m_pFlowGraphDebugger = GetIFlowGraphDebuggerPtr();

    GetIEditor()->UnregisterNotifyListener(this);
    s_instance = nullptr;
}

//////////////////////////////////////////////////////////////////////////
BOOL CHyperGraphDialog::OnInitDialog()
{
    QSettings settings;

    DWORD compView = settings.value("FlowGraph/ComponentView", EFLN_APPROVED | EFLN_ADVANCED | EFLN_DEBUG).toInt();
    m_componentsViewMask = compView;

    // Right pane needs to be created before left pane since left pane will receive window events right after creation
    // and some of the event handler ( OnGraphsSelChanged ) access objects that are created in CreateRightPane
    CreateRightPane();
    CreateLeftPane();

    // Create Widget
    if (!m_widget)
    {
        m_widget = new QHyperGraphWidget(this);
        m_widget->SetComponentViewMask(m_componentsViewMask);
        setCentralWidget(m_widget);

        m_widget->SetPropertyCtrl(m_taskPanel->GetPropertyControl());
    }

    // Add the menu bar
    CreateMenuBar();
    //  Create standart toolbar.
    QToolBar* toolbar = this->addToolBar("HyperGraph ToolBar");
    toolbar->setObjectName("toolbar");
    toolbar->setIconSize(QSize(32, 32));
    toolbar->setFloatable(false);
    m_playAction = toolbar->addAction(QIcon(":/HyperGraph/toolbar/play.png"), "Play", this, SLOT(OnPlay()));
    m_playAction->setCheckable(true);
    toolbar->addSeparator();
    toolbar->addAction(m_undoAction);
    toolbar->addAction(m_redoAction);
    toolbar->addSeparator();
    m_navBackAction = toolbar->addAction(QIcon(":/HyperGraph/toolbar/back.png"), "Back", this, SLOT(OnNavBack()));
    m_navForwardAction = toolbar->addAction(QIcon(":/HyperGraph/toolbar/forward.png"), "Forward", this, SLOT(OnNavForward()));
    toolbar->addSeparator();
    toolbar->addAction(m_debugToggleAction);
    toolbar->addAction(m_debugEraseAction);

    int paneLayoutVersion = settings.value("FlowGraph/FlowGraphLayoutVersion", 0).toInt();
    //  if (paneLayoutVersion == FlowGraphLayoutVersion)
    //  {
    //      CXTPDockingPaneLayout layout(GetDockingPaneManager());
    //      if (layout.Load(_T("FlowGraphLayout")))
    //      {
    //          if (layout.GetPaneList().GetCount() > 0)
    //              GetDockingPaneManager()->SetLayout(&layout);
    //      }
    //  }
    //  else
    //  {
    //      regMgr.WriteProfileInt(_T("FlowGraph"), _T("FlowGraphLayoutVersion"), FlowGraphLayoutVersion);
    //  }

    UpdateTitle(0);

    settings.beginGroup("CHyperGraphDialog");
    if (settings.contains("state"))
    {
        QByteArray state = settings.value("state").toByteArray();
        if (!state.isEmpty())
        {
            restoreState(state);
        }
    }

    return TRUE;
}

inline void RegisterLastActionForMetrics(QMenu* menu, const QString& metricsIdentifier)
{
    // for the time being, allow these metrics to be quickly and easily removed
    QSettings settings;
    if (settings.value("DisableMenuMetrics").toBool())
    {
        return;
    }

    using namespace AzToolsFramework;
    QList<QAction*> actions = menu->actions();
    QAction* lastAction = actions.last();
    if (lastAction != nullptr)
    {
        EditorMetricsEventsBus::Broadcast(&EditorMetricsEventsBus::Events::RegisterAction, lastAction, metricsIdentifier);
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::CreateMenuBar()
{
    QMenuBar* mb = this->menuBar();

    QMenu* fileMenu = mb->addMenu("&File");
    fileMenu->addAction("New", this, SLOT(OnFileNew()), QKeySequence::New);
    RegisterLastActionForMetrics(fileMenu, "FlowGraphFileNew");
    /*
    fileMenu->addAction("New AI Action...", this, SLOT(OnFileNewAction()));
    fileMenu->addAction("New Custom Action...", this, SLOT(OnFileNewCustomAction()));
    */
    {
        QMenu* subMenu = fileMenu->addMenu("New Flow Graph Module...");
        subMenu->addAction("Global", this, SLOT(OnFileNewGlobalFGModule()));
        RegisterLastActionForMetrics(subMenu, "FlowGraphFileNewModuleGlobal");
        subMenu->addAction("Level", this, SLOT(OnFileNewLevelFGModule()));
        RegisterLastActionForMetrics(subMenu, "FlowGraphFileNewModuleLevel");
        addActions(subMenu->actions());
    }
    fileMenu->addAction("New MaterialFX Graph...", this, SLOT(OnFileNewMatFXGraph()));
    RegisterLastActionForMetrics(fileMenu, "FlowGraphFileNewMaterialFXGraph");
    fileMenu->addAction("Open...", this, SLOT(OnFileOpen()), QKeySequence::Open);
    RegisterLastActionForMetrics(fileMenu, "FlowGraphFileOpen");
    fileMenu->addAction("&Save", this, SLOT(OnFileSave()), QKeySequence::Save);
    RegisterLastActionForMetrics(fileMenu, "FlowGraphFileSave");
    fileMenu->addAction("Save &As...", this, SLOT(OnFileSaveAs()));
    RegisterLastActionForMetrics(fileMenu, "FlowGraphFileSaveAs");
    fileMenu->addSeparator();
    fileMenu->addAction("&Import...", this, SLOT(OnFileImport()));
    RegisterLastActionForMetrics(fileMenu, "FlowGraphFileImport");
    fileMenu->addAction("&Export Selection...", this, SLOT(OnFileExportSelection()));
    RegisterLastActionForMetrics(fileMenu, "FlowGraphFileExportSelection");
    addActions(fileMenu->actions());

    QMenu* editMenu = mb->addMenu("&Edit");
    m_undoAction = editMenu->addAction(QIcon(":/HyperGraph/toolbar/undo.png"), "Undo", this, SLOT(OnEditUndo()), QKeySequence::Undo);
    RegisterLastActionForMetrics(editMenu, "FlowGraphEditUndo");
    m_redoAction = editMenu->addAction(QIcon(":/HyperGraph/toolbar/redo.png"), "Redo", this, SLOT(OnEditRedo()), QKeySequence::Redo);
    RegisterLastActionForMetrics(editMenu, "FlowGraphEditRedo");
    editMenu->addSeparator();
    editMenu->addAction(tr("Group"), m_widget, SLOT(OnCreateGroupFromSelection()));
    RegisterLastActionForMetrics(editMenu, "FlowGraphEditGroupSelection");
    editMenu->addAction(tr("UnGroup"), m_widget, SLOT(OnRemoveGroupsInSelection()));
    RegisterLastActionForMetrics(editMenu, "FlowGraphEditUngroupSelection");
    editMenu->addSeparator();
    editMenu->addAction(tr("Cut"), m_widget, SLOT(OnCommandCut()), QKeySequence::Cut);
    RegisterLastActionForMetrics(editMenu, "FlowGraphEditCut");
    editMenu->addAction(tr("Copy"), m_widget, SLOT(OnCommandCopy()), QKeySequence::Copy);
    RegisterLastActionForMetrics(editMenu, "FlowGraphEditCopy");
    editMenu->addAction(tr("Paste"), m_widget, SLOT(OnCommandPaste()), QKeySequence::Paste);
    RegisterLastActionForMetrics(editMenu, "FlowGraphEditPaste");
    editMenu->addAction(tr("Paste with Links"), m_widget, SLOT(OnCommandPasteWithLinks()));
    RegisterLastActionForMetrics(editMenu, "FlowGraphEditPasteWithLinks");
    editMenu->addAction(tr("Delete"), m_widget, SLOT(OnCommandDelete()), QKeySequence::Delete);
    RegisterLastActionForMetrics(editMenu, "FlowGraphEditDelete");
    editMenu->addSeparator();
    editMenu->addAction(tr("Quick Create Node"), m_widget, SLOT(OnShortcutQuickNode()), QKeySequence(Qt::Key_Q));
    RegisterLastActionForMetrics(editMenu, "FlowGraphEditQuickCreateNode");

    editMenu->addSeparator();
    editMenu->addAction("&Find", this, SLOT(OnFind()), QKeySequence::Find);
    RegisterLastActionForMetrics(editMenu, "FlowGraphEditFind");
    addActions(editMenu->actions());

    QMenu* viewMenu = mb->addMenu("&View");
    viewMenu->addAction(m_componentsTaskPanel->toggleViewAction());
    viewMenu->addAction(m_graphsTreeDock->toggleViewAction());
    viewMenu->addAction(m_taskPanel->toggleViewAction());
    viewMenu->addAction(m_breakpointsPanel->toggleViewAction());
    viewMenu->addAction(m_pSearchOptionsView->toggleViewAction());
    viewMenu->addAction(m_pSearchResultsCtrl->toggleViewAction());
    editMenu->addSeparator();
    editMenu->addAction(tr("Zoom In"), m_widget, SLOT(OnShortcutZoomIn()), QKeySequence(Qt::Key_Plus));
    RegisterLastActionForMetrics(viewMenu, "FlowGraphViewZoomIn");
    editMenu->addAction(tr("Zoom Out"), m_widget, SLOT(OnShortcutZoomOut()), QKeySequence(Qt::Key_Minus));
    RegisterLastActionForMetrics(viewMenu, "FlowGraphViewZoomOut");
    editMenu->addAction(tr("Refresh View"), m_widget, SLOT(OnShortcutRefresh()), QKeySequence(Qt::Key_R));
    RegisterLastActionForMetrics(viewMenu, "FlowGraphViewRefresh");
    {
        QMenu* menu = viewMenu->addMenu("Components");
        m_componentsApprovedAction = menu->addAction("Release", this, SLOT(OnComponentsViewToggle()));
        RegisterLastActionForMetrics(menu, "FlowGraphViewComponentsRelease");
        m_componentsApprovedAction->setCheckable(true);
        m_componentsApprovedAction->setData(EFLN_APPROVED);
        m_componentsAdvancedAction = menu->addAction("Advanced", this, SLOT(OnComponentsViewToggle()));
        RegisterLastActionForMetrics(menu, "FlowGraphViewComponentsAdvanced");
        m_componentsAdvancedAction->setCheckable(true);
        m_componentsAdvancedAction->setData(EFLN_ADVANCED);
        m_componentsDebugAction = menu->addAction("Debug", this, SLOT(OnComponentsViewToggle()));
        RegisterLastActionForMetrics(menu, "FlowGraphViewComponentsDebug");
        m_componentsDebugAction->setCheckable(true);
        m_componentsDebugAction->setData(EFLN_DEBUG);
        m_componentsObsoleteAction = menu->addAction("Obsolete", this, SLOT(OnComponentsViewToggle()));
        RegisterLastActionForMetrics(menu, "FlowGraphViewComponentsObsolete");
        m_componentsObsoleteAction->setCheckable(true);
        m_componentsObsoleteAction->setData(EFLN_OBSOLETE);
        addActions(menu->actions());

        OnComponentsViewUpdate();
    }
    m_viewMultiplayerAction = viewMenu->addAction("Multiplayer", this, SLOT(OnMultiPlayerViewToggle()));
    RegisterLastActionForMetrics(viewMenu, "FlowGraphViewMultiplayer");
    m_viewMultiplayerAction->setCheckable(true);
    if (m_taskPanel)
    {
        m_viewMultiplayerAction->setChecked(m_taskPanel->GetGraphProperties()->IsShowMultiPlayer() ? 1 : 0);
    }
    addActions(viewMenu->actions());

    QMenu* toolsMenu = mb->addMenu("T&ools");
    toolsMenu->addAction("Edit &Graph Tokens...", this, SLOT(OnEditGraphTokens()));
    RegisterLastActionForMetrics(toolsMenu, "FlowGraphToolsEditGraphTokens");
    toolsMenu->addAction("Edit &Module...", this, SLOT(OnEditModuleInputsOutputs()));
    RegisterLastActionForMetrics(toolsMenu, "FlowGraphToolsEditModule");
    addActions(toolsMenu->actions());

    QMenu* debugMenu = mb->addMenu("&Debug");
    m_debugToggleAction = debugMenu->addAction(QIcon(":/HyperGraph/toolbar/debug.png"), "Toggle Debugging", this, SLOT(OnToggleDebug()));
    RegisterLastActionForMetrics(debugMenu, "FlowGraphDebugToggle");
    m_debugToggleAction->setCheckable(true);
    m_debugEraseAction = debugMenu->addAction(QIcon(":/HyperGraph/toolbar/erase.png"), "Erase Debug Information", this, SLOT(OnEraseDebug()));
    RegisterLastActionForMetrics(debugMenu, "FlowGraphDebugEraseDebugInfo");
    {
        QMenu* menu = debugMenu->addMenu("Ignore Flowgraph Type");
        m_debugIgnoreAIAction = menu->addAction("AI Action", this, SLOT(OnDebugFlowgraphTypeToggle()));
        RegisterLastActionForMetrics(menu, "FlowGraphDebugIgnoreFlowGraphAIAction");
        m_debugIgnoreAIAction->setData(IFlowGraph::eFGT_AIAction);
        m_debugIgnoreAIAction->setCheckable(true);
        m_debugIgnoreModuleAction = menu->addAction("Module", this, SLOT(OnDebugFlowgraphTypeToggle()));
        RegisterLastActionForMetrics(menu, "FlowGraphDebugIgnoreFlowGraphModule");
        m_debugIgnoreModuleAction->setData(IFlowGraph::eFGT_Module);
        m_debugIgnoreModuleAction->setCheckable(true);
        m_debugIgnoreCustomAction = menu->addAction("Custom Action", this, SLOT(OnDebugFlowgraphTypeToggle()));
        RegisterLastActionForMetrics(menu, "FlowGraphDebugIgnoreFlowGraphCustomAction");
        m_debugIgnoreCustomAction->setData(IFlowGraph::eFGT_CustomAction);
        m_debugIgnoreCustomAction->setCheckable(true);
        m_debugIgnoreMaterialFXAction = menu->addAction("MaterialFX", this, SLOT(OnDebugFlowgraphTypeToggle()));
        RegisterLastActionForMetrics(menu, "FlowGraphDebugIgnoreFlowGraphMaterialFX");
        m_debugIgnoreMaterialFXAction->setData(IFlowGraph::eFGT_MaterialFx);
        m_debugIgnoreMaterialFXAction->setCheckable(true);
        addActions(debugMenu->actions());

        OnDebugFlowgraphTypeUpdate();
    }
    addActions(debugMenu->actions());
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::CreateRightPane()
{
    m_taskPanel = new QHyperGraphTaskPanel(this);
    m_taskPanel->setObjectName("m_taskPanel");
    addDockWidget(Qt::RightDockWidgetArea, m_taskPanel);

    m_pSearchOptionsView = new CFlowGraphSearchCtrl(this);
    m_pSearchOptionsView->setObjectName("m_pSearchOptionsView");
    addDockWidget(Qt::RightDockWidgetArea, m_pSearchOptionsView);

    m_pSearchResultsCtrl = new CFlowGraphSearchResultsCtrl(this);
    m_pSearchResultsCtrl->setObjectName("m_pSearchResultsCtrl");
    addDockWidget(Qt::RightDockWidgetArea, m_pSearchResultsCtrl);
    m_pSearchOptionsView->SetResultsCtrl(m_pSearchResultsCtrl);

    m_breakpointsPanel = new QBreakpointsPanel(this);
    m_breakpointsPanel->setObjectName("m_breakpointsPanel");
    addDockWidget(Qt::RightDockWidgetArea, m_breakpointsPanel);
    connect(m_breakpointsPanel->treeCtrl(), &QTreeWidget::itemClicked, this, &CHyperGraphDialog::OnBreakpointsClick);

    m_breakpointsPanel->treeCtrl()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_breakpointsPanel->treeCtrl(), &QTreeWidget::customContextMenuRequested, this, &CHyperGraphDialog::OnBreakpointsRightClick);
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::CreateLeftPane()
{
    m_componentsTaskPanel = new QHyperGraphComponentsPanel(this);
    m_componentsTaskPanel->setObjectName("m_componentsTaskPanel");
    addDockWidget(Qt::LeftDockWidgetArea, m_componentsTaskPanel);
    m_componentsTaskPanel->SetComponentFilterMask(m_componentsViewMask);
    m_componentsTaskPanel->Reload();

    m_graphsTreeDock = new AzQtComponents::StyledDockWidget(this);
    m_graphsTreeDock->setObjectName("m_graphsTreeDock");
    m_graphsTreeCtrl = new CHyperGraphsTreeCtrl(this);
    m_graphsTreeDock->setWidget(m_graphsTreeCtrl);
    m_graphsTreeDock->setWindowTitle("Graphs");
    addDockWidget(Qt::LeftDockWidgetArea, m_graphsTreeDock);
    m_graphsTreeCtrl->setSelectionMode(QTreeView::ExtendedSelection);
    m_graphsTreeCtrl->setSelectionBehavior(QTreeView::SelectRows);
    m_graphsTreeCtrl->setHeaderHidden(true);
    m_graphsTreeCtrl->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_graphsTreeCtrl, &QTreeWidget::customContextMenuRequested, this, &CHyperGraphDialog::OnGraphsRightClick);
    connect(m_graphsTreeCtrl, &QTreeWidget::itemSelectionChanged, this, &CHyperGraphDialog::OnGraphsSelChanged);
    connect(m_graphsTreeCtrl, &QTreeWidget::itemDoubleClicked, this, &CHyperGraphDialog::OnGraphsDblClick);
    m_graphsTreeCtrl->Reload();
}


void CHyperGraphDialog::closeEvent(QCloseEvent* event)
{
    QByteArray state = this->saveState();
    QSettings settings;
    settings.beginGroup("CHyperGraphDialog");
    settings.setValue("state", state);
    settings.endGroup();

    CFlowGraphSearchOptions::GetSearchOptions()->Serialize(false);
    settings.beginGroup("FlowGraph");
    settings.setValue("ComponentView", m_componentsViewMask);
    settings.endGroup();

    ClearGraph();

    if (GetIEditor()->GetFlowGraphManager())
    {
        GetIEditor()->GetFlowGraphManager()->RemoveListener(this);
    }

    GetIEditor()->UnregisterNotifyListener(this);

    SAFE_DELETE(m_pSearchOptionsView);
    SAFE_DELETE(m_pSearchResultsCtrl);
    SAFE_DELETE(m_taskPanel);       // KDAP_PORT: THIS EVENTUALLY CRASHES

    QMainWindow::closeEvent(event);
}

/////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnViewSelectionChange()
{
    std::vector<CHyperNode*> nodes;
    m_widget->GetSelectedNodes(nodes);
    if (nodes.size() <= 1)
    {
        m_taskPanel->SetSelectedNode(nodes.size() ? nodes[0] : 0);
    }

    int h = m_taskPanel->GetPropertyControl()->GetVisibleHeight();
    m_taskPanel->GetPropertyControl()->setVisible(h > 0);
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::SetGraph(CHyperGraph* pGraph, bool viewOnly /*= false*/)
{
    // CryLogAlways("CHyperGraphDialog::SetGraph: Switching to 0x%p", pGraph);
    bool isDifferentGraph = false;

    if (m_widget)
    {
        isDifferentGraph = m_widget->GetGraph() != pGraph;
        m_widget->SetGraph(pGraph);
    }

    // KDAB_PORT:
    // viewOnly was added to replicate functionality in places that accessed the old view directly to set it's graph
    // Not sure it should have but just replicating just in case.
    // Callers that set it to true are:
    //  CAIManager::SaveAndReloadActionGraphs
    //  CCustomActionsEditorManager::SaveAndReloadCustomActionGraphs()
    if (viewOnly)
    {
        return;
    }

    if (isDifferentGraph)
    {
        if (m_taskPanel)
        {
            m_taskPanel->GetGraphProperties()->SetGraph(pGraph);
            m_taskPanel->GetTokensCtrl()->SetGraph(pGraph);
        }
        m_graphsTreeCtrl->SetCurrentGraph(pGraph);

        ShowNotificationDialog();
    }

    UpdateTitle(pGraph);

    if (pGraph != 0)
    {
        std::list<CHyperGraph*>::iterator iter = std::find(m_graphList.begin(), m_graphList.end(), pGraph);
        if (iter != m_graphList.end())
        {
            m_graphIterCur = iter;
        }
        else
        {
            // maximum entry 20
            while (m_graphList.size() > 20)
            {
                m_graphList.pop_front();
            }
            m_graphIterCur = m_graphList.insert(m_graphList.end(), pGraph);
        }
    }
    else
    {
        m_graphIterCur = m_graphList.end();
    }
}

//////////////////////////////////////////////////////////////////////////
CHyperGraph* CHyperGraphDialog::GetGraph() const
{
    if (m_widget)
    {
        return m_widget->GetGraph();
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
CHyperGraph* CHyperGraphDialog::NewGraph()
{
    CHyperGraph* pGraph = static_cast<CHyperGraph*>(GetIEditor()->GetFlowGraphManager()->CreateGraph());

    if (pGraph)
    {
        pGraph->AddRef();
    }

    SetGraph(pGraph);

    return pGraph;
}


//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnFileNew()
{
    ClearGraph();
    // Make a new graph.
    NewGraph();
}

void CHyperGraphDialog::ShowNotificationDialog()
{
    if (!m_shownNotification)
    {
        if (gSettings.showFlowgraphNotification)
        {
            // Display a notification regarding the use of FlowGraph.
            FlowGraphNotificationDialog* flowGraphNotification = aznew FlowGraphNotificationDialog(this);
            flowGraphNotification->Show();

            m_shownNotification = true;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnFileOpen()
{
    if (!m_widget)
    {
        return;
    }

    QString filename;
    QString gameDataDir(Path::GetEditingGameDataFolder().c_str());
    if (CFileUtil::SelectFile (GRAPH_FILE_FILTER, gameDataDir, filename))   // native dialog
    {
        XmlNodeRef graphNode = XmlHelpers::LoadXmlFromFile(filename.toStdString().c_str());
        if (graphNode && graphNode->isTag("Graph"))
        {
            ClearGraph();
            CHyperGraph* currentGraph = NewGraph();
            bool ok = m_widget->GetGraph()->LoadInternal(graphNode, filename.toStdString().c_str());
            if (ok)
            {
                if (m_widget->GetGraph())
                {
                    QFileInfo info(filename);
                    m_widget->GetGraph()->SetName(info.baseName());
                }
                m_graphsTreeCtrl->Reload();
                m_widget->InvalidateView(true);
                m_widget->FitFlowGraphToView();
            }
            else
            {
                Warning("Cannot load FlowGraph from file '%s'.\nMake sure the file is actually a FlowGraph XML File.", filename.toStdString().c_str());
                delete currentGraph;
            }
        }
        else
        {
            Warning("Cannot load FlowGraph from file '%s'.\nMake sure the file is actually a FlowGraph XML File.", filename.toStdString().c_str());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnFileSave()
{
    if (!m_widget)
    {
        return;
    }
    CHyperGraph* pGraph = m_widget->GetGraph();
    if (!pGraph)
    {
        return;
    }

    QString filename = pGraph->GetFilename();
    Path::ReplaceExtension(filename, ".xml");
    if (filename.isEmpty())
    {
        OnFileSaveAs();
    }
    else
    {
        bool ok = false;

        assert(pGraph->IsFlowGraph());
        if (pGraph->IsFlowGraph())
        {
            CFlowGraph* pFlowGraph = static_cast<CFlowGraph*>(pGraph);
            if (pFlowGraph->GetIFlowGraph()->GetType() == IFlowGraph::eFGT_Module)
            {
                GetIEditor()->GetFlowGraphModuleManager()->SaveModuleGraph(pFlowGraph);
                ok = true;
            }
            else
            {
                QString srcAssetPath = Path::GamePathToFullPath(filename);
                ok = pGraph->Save(srcAssetPath.toUtf8().data());
            }
        }

        if (!ok)
        {
            Warning ("Cannot save FlowGraph to file '%s'.\nMake sure the path is correct.", filename.toUtf8().data());
        }

        // After saving custom action graph, flag it so in-memory copy in CCustomActionManager will be reloaded from xml
        ICustomAction* pCustomAction = pGraph->GetCustomAction();
        if (pCustomAction)
        {
            pCustomAction->Invalidate();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnFileSaveAs()
{
    if (!m_widget)
    {
        return;
    }
    CHyperGraph* pGraph = m_widget->GetGraph();
    if (!pGraph)
    {
        return;
    }

    QString filename = QString(pGraph->GetName()) + ".xml";
    QFileInfo info(pGraph->GetFilename());
    QString dir = info.path();
    if (dir.isEmpty())
    {
        dir = Path::GetEditingGameDataFolder().c_str();
    }
    if (CFileUtil::SelectSaveFile(GRAPH_FILE_FILTER, "xml", dir, filename))
    {
        QWaitCursor wait;

        bool ok = false;
        assert(pGraph->IsFlowGraph());
        if (pGraph->IsFlowGraph())
        {
            CFlowGraph* pFlowGraph = static_cast<CFlowGraph*>(pGraph);
            if (pFlowGraph->GetIFlowGraph()->GetType() == IFlowGraph::eFGT_Module)
            {
                GetIEditor()->GetFlowGraphModuleManager()->SaveModuleGraph(pFlowGraph);
                ok = true;
            }
            else
            {
                ok = pGraph->Save(filename.toStdString().c_str());
            }
        }

        if (!ok)
        {
            Warning("Cannot save FlowGraph to file '%s'.\nMake sure the path is correct.", filename.toStdString().c_str());
        }

        // After saving custom action graph, flag it so in-memory copy in CCustomActionManager will be reloaded from xml
        ICustomAction* pCustomAction = pGraph->GetCustomAction();
        if (pCustomAction)
        {
            pCustomAction->Invalidate();
        }
    }
}

void CHyperGraphDialog::OnFileNewGlobalFGModule()
{
    NewFGModule(IFlowGraphModule::eT_Global);
}

void CHyperGraphDialog::OnFileNewLevelFGModule()
{
    if (GetIEditor()->GetGameEngine()->IsLevelLoaded())
    {
        NewFGModule(IFlowGraphModule::eT_Level);
    }
}

void CHyperGraphDialog::NewFGModule(IFlowGraphModule::EType type)
{
    QString filename;
    CEditorFlowGraphModuleManager* pModuleManager = GetIEditor()->GetFlowGraphModuleManager();

    if (pModuleManager == NULL)
    {
        return;
    }

    if (pModuleManager->NewModule(filename, type))
    {
        IFlowGraphPtr pModuleGraph = pModuleManager->GetModuleFlowGraph(PathUtil::GetFileName(filename.toUtf8().data()));

        if (pModuleGraph)
        {
            CFlowGraphManager* pManager = GetIEditor()->GetFlowGraphManager();
            CFlowGraph* pFlowGraph = pManager->FindGraph(pModuleGraph);
            assert(pFlowGraph);
            if (pFlowGraph)
            {
                pFlowGraph->GetIFlowGraph()->SetType(IFlowGraph::eFGT_Module);
                pManager->OpenView(pFlowGraph);
                m_componentsTaskPanel->Reload();

                // Request source control add
                using SCCommandBus = AzToolsFramework::SourceControlCommandBus;
                SCCommandBus::Broadcast(&SCCommandBus::Events::RequestEdit, filename.toUtf8().data(), true, [](bool, const AzToolsFramework::SourceControlFileInfo&) {});
            }
        }
        else
        {
            SetGraph(NULL);
        }
    }
}

bool CHyperGraphDialog::RemoveModuleFile(const char* filePath)
{
    bool tryLocalDelete = false;
    bool promptLocalDelete = true;
    AzToolsFramework::SourceControlFileInfo sccFileInfo;

    if (CFileUtil::GetFileInfoFromSourceControl(filePath, sccFileInfo, this))
    {
        if (sccFileInfo.IsManaged())
        {
            bool acceptDelete = false;
            if (sccFileInfo.HasFlag(AzToolsFramework::SCF_OpenByUser))
            {
                // Files that are checked out already or marked for add, will need to be reverted, this is potentially destructive so we ask the user to confirm.
                acceptDelete = (QMessageBox::warning(this, tr("Confirm delete"), tr("\"%1\" is currently opened for edit, if you proceed any changes you may have made will be reverted before the file is deleted. Are you sure you want to delete this file and lose unsaved changes? ").arg(filePath), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes);
            }
            else
            {
                // The file is checked in, if the user accepts we will mark it for delete.
                acceptDelete = (QMessageBox::question(this, tr("Mark for delete"), tr("Are you sure you want to mark \"%1\" for deletion?").arg(filePath)) == QMessageBox::Yes);
            }

            return acceptDelete ? CFileUtil::DeleteFromSourceControl(filePath, this) : false;
        }

        // The file isn't under scc, lets try to delete it locally.
        tryLocalDelete = true;
    }
    else if (QMessageBox::warning(this, tr("Source Control Provider Unavailable"), tr("The source control provider cannot be reached for file status on \"%1\". Would you like to remove the local file? "
        "All unsaved changes will be lost.\n\nIf using source control, you will need to reconcile offline work later.").arg(filePath), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        // if we can not communicate with scc, give the option to forcefully remove the file.
        tryLocalDelete = true;
        promptLocalDelete = false;
    }

    if (tryLocalDelete)
    {
        // If the file is read-only, we will inform the user and will not delete.
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO, "FileIO is not initialized.");

        if (!fileIO)
        {
            return false;
        }

        bool acceptDelete = false;
        if (promptLocalDelete)
        {
            if (fileIO->IsReadOnly(filePath))
            {
                if (QMessageBox::warning(this, tr("File is read-only..."), tr("\"%1\" is read-only on disk, do you still wish to delete the file?").arg(filePath), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
                {
                    acceptDelete = true;
                    AZ::IO::SystemFile::SetWritable(filePath, true);
                }
            }
            else
            {
                acceptDelete = (QMessageBox::question(this, tr("Confirm delete"), tr("Are you sure you want to delete \"%1\" ?").arg(filePath)) == QMessageBox::Yes);
            }
        }

        if (!promptLocalDelete || acceptDelete)
        {
            return fileIO->Remove(filePath) != AZ::IO::ResultCode::Success;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnFileNewAction()
{
    QString filename;
    if (GetIEditor()->GetAI()->NewAction(filename, this))
    {
        IAIAction* pAction = gEnv->pAISystem->GetAIActionManager()->GetAIAction(PathUtil::GetFileName(filename.toUtf8().data()));
        if (pAction)
        {
            CFlowGraphManager* pManager = GetIEditor()->GetFlowGraphManager();
            CFlowGraph* pFlowGraph = pManager->FindGraphForAction(pAction);
            assert(pFlowGraph);
            if (pFlowGraph)
            {
                pFlowGraph->GetIFlowGraph()->SetType(IFlowGraph::eFGT_AIAction);
                pManager->OpenView(pFlowGraph);
            }
        }
        else
        {
            SetGraph(NULL);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnEditGraphTokens()
{
    if (!m_widget)
    {
        return;
    }

    if (CHyperGraph* pGraph = GetGraph())
    {
        if (pGraph->IsFlowGraph())
        {
            CFlowGraph* pFlowGraph = (CFlowGraph*)pGraph;
            IFlowGraph* pIFlowGraph = pFlowGraph->GetIFlowGraph();
            if (pIFlowGraph)
            {
                // get latest tokens from graph
                size_t tokenCount = pIFlowGraph->GetGraphTokenCount();
                TTokens newTokens;
                for (size_t i = 0; i < tokenCount; ++i)
                {
                    const IFlowGraph::SGraphToken* graphToken = pIFlowGraph->GetGraphToken(i);
                    STokenData newToken;
                    newToken.name = graphToken->name;
                    newToken.type = graphToken->type;
                    newTokens.push_back(newToken);
                }

                CFlowGraphTokensDlg dlg(this);
                dlg.SetTokenData(newTokens);
                if (dlg.exec() == QDialog::Accepted)
                {
                }

                // get token list from dialog, set on graph
                TTokens tokens = dlg.GetTokenData();

                pIFlowGraph->RemoveGraphTokens();

                for (size_t i = 0; i < tokens.size(); ++i)
                {
                    IFlowGraph::SGraphToken graphToken;
                    graphToken.name = tokens[i].name.toLocal8Bit().constData();
                    graphToken.type = tokens[i].type;

                    pIFlowGraph->AddGraphToken(graphToken);
                }
                m_taskPanel->GetTokensCtrl()->UpdateGraphProperties(pGraph);
                pFlowGraph->SendNotifyEvent(nullptr, EHG_GRAPH_TOKENS_UPDATED);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnEditModuleInputsOutputs()
{
    if (!m_widget)
    {
        return;
    }

    if (CHyperGraph* pGraph = m_widget->GetGraph())
    {
        if (pGraph->IsFlowGraph())
        {
            CFlowGraph* pFlowGraph = static_cast<CFlowGraph*>(pGraph);
            IFlowGraph* pIFlowGraph = pFlowGraph->GetIFlowGraph();
            if (pIFlowGraph)
            {
                IFlowGraph::EFlowGraphType type = pIFlowGraph->GetType();
                if (pIFlowGraph->GetType() == IFlowGraph::eFGT_Module)
                {
                    IFlowGraphModule* pModule = gEnv->pFlowSystem->GetIModuleManager()->GetModule(pGraph->GetName().toUtf8().data());

                    if (pModule)
                    {
                        CFlowGraphEditModuleDlg dlg(pModule, this);
                        if (dlg.exec() == QDialog::Accepted)
                        {
                            XmlNodeRef tempNode = gEnv->pSystem->CreateXmlNode("Graph");
                            pGraph->Serialize(tempNode, false);
                            pGraph->Serialize(tempNode, true);
                            pGraph->SetModified(true);
                            m_widget->InvalidateView(true);
                        }
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnFileNewCustomAction()
{
    QString filename;
    if (GetIEditor()->GetCustomActionManager()->NewCustomAction(filename))
    {
        ICustomActionManager* pCustomActionManager = GetISystem()->GetIGame()->GetIGameFramework()->GetICustomActionManager();
        CRY_ASSERT(pCustomActionManager != NULL);
        if (!pCustomActionManager)
        {
            return;
        }

        ICustomAction* pCustomAction = pCustomActionManager->GetCustomActionFromLibrary(PathUtil::GetFileName(filename.toUtf8().data()));
        if (pCustomAction)
        {
            CFlowGraphManager* pFlowGraphManager = GetIEditor()->GetFlowGraphManager();
            CFlowGraph* pFlowGraph = pFlowGraphManager->FindGraphForCustomAction(pCustomAction);
            assert(pFlowGraph);
            if (pFlowGraph)
            {
                pFlowGraphManager->OpenView(pFlowGraph);
            }
        }
        else
        {
            SetGraph(NULL);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnFileNewMatFXGraph()
{
    QString filename;
    CHyperGraph* pGraph = NULL;
    if (GetIEditor()->GetMatFxGraphManager()->NewMaterialFx(filename, &pGraph))
    {
        GetIEditor()->GetFlowGraphManager()->OpenView(pGraph);
    }
    else
    {
        SetGraph(NULL);
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnFileImport()
{
    if (!m_widget)
    {
        return;
    }
    CHyperGraph* pGraph = m_widget->GetGraph();
    if (!pGraph)
    {
        Warning ("Please first create a FlowGraph to which to import");
        return;
    }

    QString filename;
    // if (CFileUtil::SelectSingleFile( EFILE_TYPE_ANY,filename,GRAPH_FILE_FILTER )) // game dialog
    if (CFileUtil::SelectFile (GRAPH_FILE_FILTER, "", filename))  // native dialog
    {
        QWaitCursor wait;
        bool ok = m_widget->GetGraph()->Import(filename.toStdString().c_str());
        if (!ok)
        {
            Warning("Cannot import file '%s'.\nMake sure the file is actually a FlowGraph XML File.", filename.toStdString().c_str());
        }
        m_widget->InvalidateView(true);
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnFileExportSelection()
{
    if (!m_widget)
    {
        return;
    }
    CHyperGraph* pGraph = m_widget->GetGraph();
    if (!pGraph)
    {
        return;
    }

    std::vector<CHyperNode*> nodes;
    if (!m_widget->GetSelectedNodes(nodes))
    {
        Warning("You must have selected graph nodes to export");
        return;
    }

    QFileInfo info(QString(pGraph->GetName()) + ".xml");
    QString filename = info.fileName();
    if (CFileUtil::SelectSaveFile(GRAPH_FILE_FILTER, "xml", info.path(), filename))
    {
        QWaitCursor wait;
        CHyperGraphSerializer serializer(pGraph, 0);

        for (int i = 0; i < nodes.size(); i++)
        {
            CHyperNode* pNode = nodes[i];
            serializer.SaveNode(pNode, true);
        }
        if (nodes.size() > 0)
        {
            XmlNodeRef node = XmlHelpers::CreateXmlNode("Graph");
            serializer.Serialize(node, false, true, false, true);
            bool ok = XmlHelpers::SaveXmlNode(GetIEditor()->GetFileUtil(), node, filename.toStdString().c_str());
            if (!ok)
            {
                Warning("FlowGraph not saved to file '%s'", filename.toStdString().c_str());
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnFind()
{
    m_pSearchOptionsView->setVisible(true);
    m_pSearchOptionsView->setFocus();
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnComponentsViewToggle()
{
    m_componentsViewMask ^= qobject_cast<QAction*>(sender())->data().toInt();
    m_componentsTaskPanel->SetComponentFilterMask(m_componentsViewMask);
    m_componentsTaskPanel->Reload();
    m_widget->SetComponentViewMask(m_componentsViewMask);
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnComponentsViewUpdate()
{
    m_componentsApprovedAction->setChecked((m_componentsViewMask & m_componentsApprovedAction->data().toInt()) ? true : false);
    m_componentsAdvancedAction->setChecked((m_componentsViewMask & m_componentsAdvancedAction->data().toInt()) ? true : false);
    m_componentsDebugAction->setChecked((m_componentsViewMask & m_componentsDebugAction->data().toInt()) ? true : false);
    m_componentsObsoleteAction->setChecked((m_componentsViewMask & m_componentsObsoleteAction->data().toInt()) ? true : false);
}

void CHyperGraphDialog::OnDebugFlowgraphTypeToggle()
{
    if (!m_pFlowGraphDebugger)
    {
        return;
    }

    IFlowGraph::EFlowGraphType type = (IFlowGraph::EFlowGraphType) qobject_cast<QAction*>(sender())->data().toInt();

    const bool ignored = m_pFlowGraphDebugger->IsFlowgraphTypeIgnored(type);
    m_pFlowGraphDebugger->IgnoreFlowgraphType(type, !ignored);
    OnDebugFlowgraphTypeUpdate();
}

void CHyperGraphDialog::OnDebugFlowgraphTypeUpdate()
{
    if (!m_pFlowGraphDebugger)
    {
        return;
    }

    bool ignored = m_pFlowGraphDebugger->IsFlowgraphTypeIgnored(IFlowGraph::eFGT_AIAction);
    m_debugIgnoreAIAction->setChecked(ignored ? true : false);
    ignored = m_pFlowGraphDebugger->IsFlowgraphTypeIgnored(IFlowGraph::eFGT_Module);
    m_debugIgnoreModuleAction->setChecked(ignored ? true : false);
    ignored = m_pFlowGraphDebugger->IsFlowgraphTypeIgnored(IFlowGraph::eFGT_CustomAction);
    m_debugIgnoreCustomAction->setChecked(ignored ? true : false);
    ignored = m_pFlowGraphDebugger->IsFlowgraphTypeIgnored(IFlowGraph::eFGT_MaterialFx);
    m_debugIgnoreMaterialFXAction->setChecked(ignored ? true : false);
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::ClearGraph()
{
    /*
    CHyperGraph *pGraph = m_widget->GetGraph();
    if (pGraph && pGraph->IsModified())
    {
        if (QMessageBox( _T("Graph was modified, Save changes?"),MB_YESNO|MB_ICONQUESTION ) == IDYES)
        {
            OnFileSave();
        }
    }
    */
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnPlay()
{
    if (m_pFlowGraphDebugger)
    {
        m_pFlowGraphDebugger->EnableStepMode(false);
    }

    GetIEditor()->GetFlowGraphDebuggerEditor()->ResumeGame();
    OnUpdatePlay();
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnStop()
{
    GetIEditor()->GetGameEngine()->EnableFlowSystemUpdate(false);
    OnUpdatePlay();
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnToggleDebug()
{
    ICVar* pToggle = gEnv->pConsole->GetCVar("fg_iEnableFlowgraphNodeDebugging");
    if (pToggle)
    {
        pToggle->Set((pToggle->GetIVal() > 0) ? 0 : 1);
        m_debugToggleAction->setChecked(pToggle->GetIVal() > 0);
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnEraseDebug()
{
    std::list<CHyperGraph*>::iterator iter = m_graphList.begin();
    for (; iter != m_graphList.end(); ++iter)
    {
        CHyperGraph* pGraph = *iter;
        if (pGraph && pGraph->IsNodeActivationModified())
        {
            pGraph->ClearDebugPortActivation();

            IHyperGraphEnumerator* pEnum = pGraph->GetNodesEnumerator();
            for (IHyperNode* pINode = pEnum->GetFirst(); pINode; pINode = pEnum->GetNext())
            {
                CHyperNode* pNode = (CHyperNode*)pINode;
                pNode->Invalidate(true);
            }
            pEnum->Release();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnPause()
{
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnStep()
{
    if (m_pFlowGraphDebugger)
    {
        if (m_pFlowGraphDebugger->BreakpointHit())
        {
            m_pFlowGraphDebugger->EnableStepMode(true);
            GetIEditor()->GetFlowGraphDebuggerEditor()->ResumeGame();
        }
    }
    OnUpdatePlay();
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnUpdatePlay()
{
    m_playAction->setChecked(GetIEditor()->GetGameEngine()->IsFlowSystemUpdateEnabled());
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::GraphUpdateEnable(CFlowGraph* pFlowGraph)
{
    m_graphsTreeCtrl->Reload();
    QTreeWidgetItem* hItem = m_graphsTreeCtrl->GetTreeItem(pFlowGraph);
    if (0 == hItem)
    {
        return;
    }
    // this is way too slow and deletes the current open view incl. opened folders
    // that's why we use the stuff below. the tree ctrl from above needs
    // a major revamp.
    CEntityObject* pOwnerEntity = pFlowGraph->GetEntity();
    if (pOwnerEntity != 0)
    {
        int img = (pFlowGraph->IsEnabled() ? IMG_ENTITY : IMG_BIGX) + (pFlowGraph->HasError() ? IMG_ERROR_OFFSET : 0);
        m_graphsTreeCtrl->SetItemImage(hItem, img);
    }
    else if (pFlowGraph->GetAIAction() == 0)
    {
        int img = (pFlowGraph->IsEnabled() ? IMG_DEFAULT : IMG_SMALLX) + (pFlowGraph->HasError() ? IMG_ERROR_OFFSET : 0);
        m_graphsTreeCtrl->SetItemImage(hItem, img);
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnGraphsRightClick(const QPoint& pos)
{
    TDTreeItems chTreeItems = m_graphsTreeCtrl->selectedItems();

    // If no item is selected, we will try to select the item under the cursos.
    if (chTreeItems.empty())
    {
        QTreeWidgetItem* hCurrentItem = m_graphsTreeCtrl->itemAt(pos);
        if (hCurrentItem)
        {
            m_graphsTreeCtrl->selectionModel()->clear();
            hCurrentItem->setSelected(true);
            chTreeItems.push_back(hCurrentItem);
        }
        else
        {
            // If no item was under the cursos, there is nothing to be done here.
            return;
        }
    }

    QMenu menu;

    if (CreateContextMenu(menu, chTreeItems))
    {
        QAction* res = menu.exec(QCursor::pos());
        if (res)
        {
            int nCmd = res->data().toInt();
            ProcessMenu(nCmd, chTreeItems);
        }
    }
}

void CHyperGraphDialog::OnBreakpointsRightClick(const QPoint& pos)
{
    QTreeWidgetItem* hCurrentItem = m_breakpointsPanel->treeCtrl()->itemAt(pos);
    if (hCurrentItem)
    {
        QMenu menu;
        if (CreateBreakpointContextMenu(menu, hCurrentItem))
        {
            QAction* action = menu.exec(QCursor::pos());
            if (action)
            {
                int nCmd = action->data().toInt();
                ProcessBreakpointMenu(nCmd, hCurrentItem);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::EnableItems(QTreeWidgetItem* hItem, bool bEnable, bool bRecurse)
{
    for (int i = 0; i < hItem->childCount(); i++)
    {
        QTreeWidgetItem* hChildItem = hItem->child(i);
        if (hChildItem->childCount())
        {
            EnableItems(hChildItem, bEnable, bRecurse);
        }

        CHyperGraph* pGraph = (CHyperGraph*)m_graphsTreeCtrl->GetItemData(hChildItem);
        if (pGraph)
        {
            CFlowGraph* pFlowGraph = pGraph->IsFlowGraph() ? static_cast<CFlowGraph*> (pGraph) : 0;
            assert(pFlowGraph);
            if (pFlowGraph)
            {
                pFlowGraph->SetEnabled(bEnable);
                UpdateGraphProperties(pFlowGraph);
                // GraphUpdateEnable(pFlowGraph, hChildItem);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::RenameItems(QTreeWidgetItem* hItem, QString& newGroupName, bool bRecurse)
{
    for (int i = 0; i < hItem->childCount(); i++)
    {
        QTreeWidgetItem* hChildItem = hItem->child(i);
        if (hChildItem->childCount())
        {
            RenameItems(hChildItem, newGroupName, bRecurse);
        }

        CHyperGraph* pGraph = (CHyperGraph*)m_graphsTreeCtrl->GetItemData(hChildItem);
        if (pGraph)
        {
            CFlowGraph* pFlowGraph = pGraph->IsFlowGraph() ? static_cast<CFlowGraph*> (pGraph) : 0;
            assert (pFlowGraph);
            if (pFlowGraph)
            {
                pFlowGraph->SetGroupName(newGroupName);
                UpdateGraphProperties(pFlowGraph);
                // GraphUpdateEnable(pFlowGraph, hChildItem);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnGraphsDblClick(QTreeWidgetItem* hItem, int column)
{
    CHyperGraph* pGraph = (CHyperGraph*)m_graphsTreeCtrl->GetItemData(hItem);
    if (pGraph)
    {
        CEntityObject* pOwnerEntity = ((CFlowGraph*)pGraph)->GetEntity();
        if (pOwnerEntity)
        {
            CUndo undo("Select Object(s)");
            GetIEditor()->ClearSelection();
            GetIEditor()->SelectObject(pOwnerEntity);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnGraphsSelChanged()
{
    QList<QTreeWidgetItem*> selection = m_graphsTreeCtrl->selectedItems();
    if (selection.size())
    {
        CHyperGraph* pGraph = m_graphsTreeCtrl->GetItemData(selection.front());
        if (pGraph)
        {
            SetGraph(pGraph);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnMultiPlayerViewToggle()
{
    if (m_taskPanel)
    {
        m_taskPanel->GetGraphProperties()->ShowMultiPlayer(!m_taskPanel->GetGraphProperties()->IsShowMultiPlayer());
        m_viewMultiplayerAction->setChecked(m_taskPanel->GetGraphProperties()->IsShowMultiPlayer() ? 1 : 0);
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::ShowResultsPane(bool bShow, bool bFocus)
{
    m_pSearchResultsCtrl->setVisible(bShow);
}

//////////////////////////////////////////////////////////////////////////
// Called by the editor to notify the listener about the specified event.
void CHyperGraphDialog::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginSceneOpen:          // Sent when document is about to be opened.
        m_graphsTreeCtrl->SetIgnoreReloads(true);
        break;

    case eNotify_OnEndSceneOpen:            // Sent after document have been opened.
        m_graphsTreeCtrl->SetIgnoreReloads(false);
        m_graphsTreeCtrl->Reload();
        break;
    case eNotify_OnOpenGroup:
    case eNotify_OnCloseGroup:
    {
        // Some graphs may have changed / removed, serialize all FlowGraphs, and then restore state after reloading
        GetIEditor()->GetFlowGraphManager()->ReloadClasses();

        m_graphsTreeCtrl->Reload();
        // Check the current viewed graph and if it is part of a prefab which is closed, close the view of it
        CHyperGraph* pCurrentViewedGraph = m_widget ? m_widget->GetGraph() : nullptr;
        if (pCurrentViewedGraph)
        {
            if (pCurrentViewedGraph->IsFlowGraph())
            {
                CFlowGraph* pFlowGraph = static_cast<CFlowGraph*>(pCurrentViewedGraph);
                if (CEntityObject* pEntity = pFlowGraph->GetEntity())
                {
                    if (pEntity->IsPartOfPrefab() && !pEntity->GetPrefab()->IsOpen())
                    {
                        m_widget->SetGraph(nullptr);
                    }
                }
            }
        }
    }
    break;
    case eNotify_OnEnableFlowSystemUpdate:
    case eNotify_OnDisableFlowSystemUpdate:
    {
        if (m_widget)
        {
            m_widget->UpdateFrozen();
        }
    }
    break;
    }
}

void CHyperGraphDialog::UpdateGraphProperties(CHyperGraph* pGraph)
{
    // TODO: real listener concept
    m_taskPanel->GetGraphProperties()->UpdateGraphProperties(pGraph);
    m_taskPanel->GetTokensCtrl()->UpdateGraphProperties(pGraph);
    // TODO: real item update, not just this enable stuff
    if (pGraph->IsFlowGraph())
    {
        CFlowGraph* pFG = static_cast<CFlowGraph*> (pGraph);
        GraphUpdateEnable(pFG);
    }

    pGraph->SendNotifyEvent(nullptr, EHG_GRAPH_PROPERTIES_CHANGED);
}

void CHyperGraphDialog::UpdateTitle(CHyperGraph* pGraph)
{
    QString titleFormatter;
    if (pGraph)
    {
        titleFormatter += pGraph->GetFilename();
    }
    else
    {
        ;
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnUpdateNav()
{
    m_navForwardAction->setEnabled(m_graphList.size() > 0 && m_graphIterCur != --m_graphList.end() && m_graphIterCur != m_graphList.end());
    m_navBackAction->setEnabled(m_graphList.size() > 0 && m_graphIterCur != m_graphList.begin());
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnNavForward()
{
    if (m_pNavGraph != 0)
    {
        SetGraph(m_pNavGraph);
        m_pNavGraph = 0;
    }
    else if (m_graphList.size() > 0 && m_graphIterCur != --m_graphList.end() && m_graphIterCur != m_graphList.end())
    {
        std::list<CHyperGraph*>::iterator iter = m_graphIterCur;
        ++iter;
        CHyperGraph* pGraph = *(iter);
        SetGraph(pGraph);
    }
    OnUpdateNav();
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnNavBack()
{
    if (m_pNavGraph != 0)
    {
        SetGraph(m_pNavGraph);
        m_pNavGraph = 0;
    }
    else if (m_graphList.size() > 0 && m_graphIterCur != m_graphList.begin())
    {
        std::list<CHyperGraph*>::iterator iter = m_graphIterCur;
        --iter;
        CHyperGraph* pGraph = *(iter);
        SetGraph(pGraph);
    }
    OnUpdateNav();
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::UpdateNavHistory()
{
    // early return, when history is empty anyway [loading...]
    if (m_graphList.empty())
    {
        return;
    }

    CFlowGraphManager* pMgr = GetIEditor()->GetFlowGraphManager();
    const int fgCount = pMgr->GetFlowGraphCount();
    std::set<CHyperGraph*> graphs;
    for (int i = 0; i < fgCount; ++i)
    {
        graphs.insert(pMgr->GetFlowGraph(i));
    }

    // validate history
    bool adjustCur = false;
    std::list<CHyperGraph*>::iterator iter = m_graphList.begin();
    while (iter != m_graphList.end())
    {
        CHyperGraph* pGraph = *iter;
        if (graphs.find(pGraph) == graphs.end())
        {
            // remove graph from history
            std::list<CHyperGraph*>::iterator toDelete = iter;
            ++iter;
            if (m_graphIterCur == toDelete)
            {
                adjustCur = true;
            }

            m_graphList.erase(toDelete);
        }
        else
        {
            ++iter;
        }
    }

    // adjust current iterator in case the current graph got deleted
    if (adjustCur)
    {
        m_graphIterCur = m_graphList.end();
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::OnHyperGraphManagerEvent(EHyperGraphEvent event, IHyperGraph* pGraph, IHyperNode* pNode)
{
    switch (event)
    {
    case EHG_GRAPH_REMOVED:
    {
        UpdateNavHistory();
        break;
    }
    case EHG_GRAPH_UPDATE_FROZEN:
    {
        if (m_widget)
        {
            m_widget->UpdateFrozen();
        }
        break;
    }
    case EHG_GRAPH_CLEAR_SELECTION:
    {
        if (m_widget)
        {
            m_widget->ClearSelection();
            m_widget->InvalidateView(true);
        }
        break;
    }
    case EHG_GRAPH_INVALIDATE:
    {
        if (m_widget)
        {
            m_widget->InvalidateView(true);
        }
        break;
    }
    case EHG_CREATE_GLOBAL_FG_MODULE_FROM_SELECTION:
    {
        NewFGModule(IFlowGraphModule::eT_Global);
        break;
    }
    case EHG_CREATE_LEVEL_FG_MODULE_FROM_SELECTION:
    {
        NewFGModule(IFlowGraphModule::eT_Level);
        break;
    }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CHyperGraphDialog::CreateContextMenu(QMenu& roMenu, TDTreeItems& rchSelectedItems)
{
    enum EMenuFlags
    {
        eGraphValid = 0,
        eGraphEnabled,
        eIsOwnedByEntity,
        eIsAssociatedToFlowGraph,
        eIsEntitySubFolder,
        eIsEntitySubFolderOrFolder,
        eIsModule,
        eIsGlobalModuleFolder,
        eIsLevelModuleFolder,
        eIsDebuggingEnabled,
        eIsComponentEntity,
        eTotalEMenuFlags,
    };

    bool aboMenuFlags[eTotalEMenuFlags];

    memset(aboMenuFlags, 1, sizeof(bool) * eTotalEMenuFlags);

    bool boIsEnabled(false);
    int nTotalTreeItems(0);
    int nCurrentTreeItem(0);

    nTotalTreeItems = rchSelectedItems.size();
    for (nCurrentTreeItem = 0; nCurrentTreeItem < nTotalTreeItems; ++nCurrentTreeItem)
    {
        QTreeWidgetItem* rhCurrentTreeItem = rchSelectedItems[nCurrentTreeItem];
        if (!IsGraphEnabled(rhCurrentTreeItem, boIsEnabled))
        {
            aboMenuFlags[eGraphValid] = false;
        }
        else if (nCurrentTreeItem == 0)
        {
            aboMenuFlags[eGraphEnabled] = boIsEnabled;
        }
        else
        {
            if (aboMenuFlags[eGraphEnabled] != boIsEnabled)
            {
                aboMenuFlags[eGraphValid] = false;
            }
        }

        if (!IsOwnedByAnEntity(rhCurrentTreeItem))
        {
            aboMenuFlags[eIsOwnedByEntity] = false;
        }

        if (!IsGraphType(rhCurrentTreeItem, IFlowGraph::eFGT_Module))
        {
            aboMenuFlags[eIsModule] = false;
        }

        if (!IsAssociatedToFlowGraph(rhCurrentTreeItem))
        {
            aboMenuFlags[eIsAssociatedToFlowGraph] = false;
        }

        if (!IsAnEntitySubFolder(rhCurrentTreeItem))
        {
            aboMenuFlags[eIsEntitySubFolder] = false;
        }

        if (!IsAnEntityFolderOrSubFolder(rhCurrentTreeItem))
        {
            aboMenuFlags[eIsEntitySubFolderOrFolder] = false;
        }

        if (!IsEnabledForDebugging(rhCurrentTreeItem))
        {
            aboMenuFlags[eIsDebuggingEnabled] = false;
        }

        if (!IsComponentEntity(rhCurrentTreeItem))
        {
            aboMenuFlags[eIsComponentEntity] = false;
        }

        if (!IsGlobalModuleFolder(rhCurrentTreeItem))
        {
            aboMenuFlags[eIsGlobalModuleFolder] = false;
        }
        if (!IsLevelModuleFolder(rhCurrentTreeItem))
        {
            aboMenuFlags[eIsLevelModuleFolder] = false;
        }

        // Treat g_ComponentEntityFolderName as a non-entity subfolder to prevent the Rename/Move context menu from displaying.
        QString itemName = rhCurrentTreeItem->text(0);
        if (itemName.compare(g_ComponentEntityFolderName) == 0)
        {
            aboMenuFlags[eIsEntitySubFolder] = false;
        }
    }

    int nCurrentFlag(0);


    if (aboMenuFlags[eIsGlobalModuleFolder])
    {
        roMenu.addAction("New Global Flow Graph Module")->setData(ID_GRAPHS_NEW_GLOBAL_MODULE);
    }

    if (aboMenuFlags[eIsLevelModuleFolder])
    {
        roMenu.addAction("New Level Flow Graph Module")->setData(ID_GRAPHS_NEW_LEVEL_MODULE);
    }

    if (aboMenuFlags[eGraphValid])
    {
        if (!aboMenuFlags[eIsModule])
        {
            roMenu.addAction(aboMenuFlags[eGraphEnabled] ? "Enable" : "Disable")->setData(ID_GRAPHS_ENABLE);
        }

        // we can only rename the custom graphs found in Files node
        if (!aboMenuFlags[eIsOwnedByEntity])
        {
            if (!aboMenuFlags[eIsModule])
            {
                QString renameMessage = "Rename Graph";
                roMenu.addSeparator();
                roMenu.addAction(renameMessage)->setData(ID_GRAPHS_RENAME_GRAPH);
            }

            QString deleteMessage = "Delete Graph";

            if (aboMenuFlags[eIsModule])
            {
                deleteMessage = "Delete Module";
            }

            roMenu.addAction(deleteMessage)->setData(ID_GRAPHS_REMOVE_NONENTITY_GRAPH);

            if (!aboMenuFlags[eIsModule])
            {
                roMenu.addSeparator();
            }
        }
    }

    if (aboMenuFlags[eIsOwnedByEntity])
    {
        roMenu.addAction("Select Entity")->setData(ID_GRAPHS_SELECT_ENTITY);
        roMenu.addSeparator();
        roMenu.addAction("Delete Graph")->setData(ID_GRAPHS_REMOVE_GRAPH);
    }

    if (aboMenuFlags[eIsAssociatedToFlowGraph] && !aboMenuFlags[eIsModule] && !aboMenuFlags[eIsComponentEntity])
    {
        roMenu.addAction("Change Folder")->setData(ID_GRAPHS_CHANGE_FOLDER);
    }

    if (aboMenuFlags[eIsEntitySubFolder])
    {
        // Renaming does not make sense in batch considering the current usage of the interface.
        if (rchSelectedItems.size() == 1)
        {
            roMenu.addAction("Change Group Name")->setData(ID_GRAPHS_RENAME_FOLDER);
        }
    }

    if (aboMenuFlags[eIsEntitySubFolderOrFolder])
    {
        roMenu.addAction("Enable All")->setData(ID_GRAPHS_ENABLE_ALL);
        roMenu.addAction("Disable All")->setData(ID_GRAPHS_DISABLE_ALL);
    }

    if (aboMenuFlags[eGraphValid])
    {
        roMenu.addSeparator();
        QAction* a = roMenu.addAction("Enable (Break/Trace) points");
        a->setData(ID_GRAPHS_ENABLE_DEBUGGING);
        a->setEnabled(aboMenuFlags[eIsDebuggingEnabled]);
        a = roMenu.addAction("Disable (Break/Trace) points");
        a->setData(ID_GRAPHS_DISABLE_DEBUGGING);
        a->setEnabled(!aboMenuFlags[eIsDebuggingEnabled]);
    }

    // Don't show if menu is empty were added
    return !roMenu.isEmpty();
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::ProcessMenu(int nMenuOption, TDTreeItems& rchSelectedItem)
{
    std::vector<CEntityObject*> cSelectedEntities;
    std::vector<CHyperGraph*> cSelectedGraphs;
    std::vector<CFlowGraph*> selectedComponentEntityFlowGraphs;

    int nTotalSelectedItems = rchSelectedItem.size();
    for (int nCurrentSelectedItem = 0; nCurrentSelectedItem < nTotalSelectedItems; ++nCurrentSelectedItem)
    {
        QTreeWidgetItem*& rhCurrentSelectedItem = rchSelectedItem[nCurrentSelectedItem];
        CHyperGraph* pGraph = m_graphsTreeCtrl->GetItemData(rhCurrentSelectedItem);
        CFlowGraph* pFlowGraph = nullptr;
        CEntityObject* pOwnerEntity = nullptr;
        if (pGraph && pGraph->IsFlowGraph())
        {
            pFlowGraph = static_cast<CFlowGraph*>(pGraph);
            pOwnerEntity = pFlowGraph->GetEntity();
        }

        QString currentGroupName = rhCurrentSelectedItem->text(0);

        switch (nMenuOption)
        {
        case ID_GRAPHS_ENABLE_ALL:
        case ID_GRAPHS_DISABLE_ALL:
        {
            bool enabled (nMenuOption == ID_GRAPHS_ENABLE_ALL);
            m_graphsTreeCtrl->setUpdatesEnabled(false);
            EnableItems(rhCurrentSelectedItem, enabled, true);
            m_graphsTreeCtrl->setUpdatesEnabled(true);
        }
        break;
        case ID_GRAPHS_RENAME_FOLDER:
        {
            bool bIsAction = false;
            CFlowGraphManager* pFlowGraphManager = GetIEditor()->GetFlowGraphManager();
            std::set<QString> groupsSet;
            pFlowGraphManager->GetAvailableGroups(groupsSet, bIsAction);

            QString groupName;
            bool bDoNew  = true;
            bool bChange = false;

            if (groupsSet.size() > 0)
            {
                std::vector<QString> groups(groupsSet.begin(), groupsSet.end());

                CGenericSelectItemDialog gtDlg;
                if (bIsAction == false)
                {
                    gtDlg.setWindowTitle(tr("Choose New Group for the Flow Graph"));
                }
                else
                {
                    gtDlg.setWindowTitle(tr("Choose Group for the AIAction Graph"));
                }

                gtDlg.PreSelectItem(currentGroupName);
                gtDlg.SetItems(groups);
                gtDlg.AllowNew(true);
                switch (gtDlg.exec())
                {
                case QDialog::Accepted:
                    groupName = gtDlg.GetSelectedItem();
                    bChange = true;
                    bDoNew  = false;
                    break;
                case CGenericSelectItemDialog::New:
                    // bDoNew  = true; // is true anyway
                    break;
                default:
                    bDoNew = false;
                    break;
                }
            }

            if (bDoNew)
            {
                QString r = QInputDialog::getText(this, tr("Rename"),
                        bIsAction == false ? tr("Enter Group Name for the Flow Graph") : tr("Enter Group Name for the AIAction Graph"),
                        QLineEdit::Normal, currentGroupName);
                if (!r.isEmpty() && r != currentGroupName)
                {
                    bChange = true;
                    groupName = r;
                }
            }

            if (bChange && groupName != currentGroupName)
            {
                m_graphsTreeCtrl->SetIgnoreReloads(true);
                m_graphsTreeCtrl->setUpdatesEnabled(false);
                RenameItems(rhCurrentSelectedItem, groupName, true);
                m_graphsTreeCtrl->setUpdatesEnabled(true);
                m_graphsTreeCtrl->SetIgnoreReloads(false);
                m_graphsTreeCtrl->Reload();
            }
        }
        break;

        case ID_GRAPHS_SELECT_ENTITY:
        {
            if (pOwnerEntity)
            {
                CUndo undo("Select Object(s)");
                if (nCurrentSelectedItem == 0)
                {
                    GetIEditor()->ClearSelection();
                }
                GetIEditor()->SelectObject(pOwnerEntity);
            }
            /*
            CStringDlg dlg( "Rename Graph" );
            dlg.SetString( pGraph->GetName() );
            if (dlg.DoModal() == IDOK)
            {
            pGraph->SetName( dlg.GetString() );
            m_graphsTreeCtrl.SetItemText( hItem,dlg.GetString() );
            }
            */
        }
        break;
        case ID_GRAPHS_NEW_GLOBAL_MODULE:
        {
            OnFileNewGlobalFGModule();
        }
        break;
        case ID_GRAPHS_NEW_LEVEL_MODULE:
        {
            OnFileNewLevelFGModule();
        }
        break;
        case ID_GRAPHS_REMOVE_NONENTITY_GRAPH:
        {
            if (pFlowGraph)
            {
                bool bIsModule = (pFlowGraph->GetIFlowGraph()->GetType() == IFlowGraph::eFGT_Module);

                QString message = bIsModule ? tr("Delete Module: %1?").arg(pFlowGraph->GetName()) : tr("Delete Graph: %1?").arg(pFlowGraph->GetName());
                if (QMessageBox::question(this, tr("Confirm Delete"), message) == QMessageBox::Yes)
                {
                    CUndo undo("Delete Graph");

                    bool success = !bIsModule;

                    QString fullPath;
                    IFlowGraphModuleManager* pModuleManager = gEnv->pFlowSystem->GetIModuleManager();
                    if (bIsModule)
                    {
                        // Remove the module file
                        QString modulePath = pModuleManager->GetModulePath(pFlowGraph->GetName().toUtf8().constData());
                        fullPath = Path::GamePathToFullPath(modulePath);
                        success = RemoveModuleFile(fullPath.toUtf8().data());
                    }

                    if (success)
                    {
                        // Remove UI Elements (must be done before notifying the module manager)
                        QTreeWidgetItem* hAdjacentItem = nullptr;
                        m_graphsTreeCtrl->DeleteNonEntityItem(pGraph, hAdjacentItem);

                        CHyperGraph* pAdjacentGraph = nullptr;
                        // TODO: If we have a non-empty adjacent item, auto select it. LMBR-18624
                        if (hAdjacentItem && hAdjacentItem->childCount())
                        {
                            pAdjacentGraph = (CHyperGraph*)m_graphsTreeCtrl->GetItemData(hAdjacentItem);
                        }

                        if (pAdjacentGraph)
                        {
                            SetGraph(pAdjacentGraph);
                        }
                        else
                        {
                            if (m_widget)
                            {
                                m_widget->SetGraph(0);
                            }
                            m_taskPanel->GetGraphProperties()->SetGraph(0);
                            UpdateTitle(0);
                            m_graphIterCur = m_graphList.end();
                            if (hAdjacentItem)
                            {
                                hAdjacentItem->setSelected(true);
                            }
                        }

                        stl::find_and_erase(m_graphList, pGraph);

                        if (bIsModule)
                        {
                            // Finally Notify the module manager
                            pModuleManager->OnDeleteModuleXML(pFlowGraph->GetName().toUtf8().constData(), fullPath.toUtf8().data());
                        }
                        else
                        {
                            SAFE_RELEASE(pGraph);
                        }
                    }
                }
            }
        }
        break;
        case ID_GRAPHS_REMOVE_GRAPH:
        {
            if (pOwnerEntity)
            {
                cSelectedEntities.push_back(pOwnerEntity);
            }

            // If it's a flowgraph that is owned by a component entity, we need to save it so we can fire events to remove it with the rest of them
            if (QString::compare(pOwnerEntity->GetClassDesc()->ClassName(), "ComponentEntity") == 0)
            {
                selectedComponentEntityFlowGraphs.push_back(pFlowGraph);
            }

            // Hack to do the operation only when all data is gathered...
            if (nCurrentSelectedItem == nTotalSelectedItems - 1)
            {
                int nTotalEntities = MIN(cSelectedEntities.size(), 7);

                QString str = "Delete Flow Graph for ";
                str += nTotalEntities > 1 ? "Entities" : "Entity";
                str += " :\n";

                int nCurrentEntity(0);
                for (nCurrentEntity = 0; nCurrentEntity < nTotalEntities; ++nCurrentEntity)
                {
                    str += cSelectedEntities[nCurrentEntity]->GetName();
                    str += "\n";
                }

                if (nTotalEntities < cSelectedEntities.size())
                {
                    str += "...";
                }

                if (QMessageBox::question(this, "Confirm", str) == QMessageBox::Yes)
                {
                    CUndo undo("Delete Entity Graph");
                    nTotalEntities = cSelectedEntities.size();

                    QTreeWidgetItem* hAdjacentItem = NULL;
                    for (nCurrentEntity = 0; nCurrentEntity < nTotalEntities; ++nCurrentEntity)
                    {
                        if (m_graphsTreeCtrl->DeleteEntityItem(cSelectedEntities[nCurrentEntity], hAdjacentItem))
                        {
                            cSelectedEntities[nCurrentEntity]->RemoveFlowGraph(false);
                        }
                    }

                    for (auto componentEntityFlowGraphToRemove : selectedComponentEntityFlowGraphs)
                    {
                        FlowEntityId flowEntityId = componentEntityFlowGraphToRemove->GetIFlowGraph()->GetGraphEntity(0);
                        IFlowGraph* flowGraphToRemove = componentEntityFlowGraphToRemove->GetIFlowGraph();
                        EBUS_EVENT_ID(flowEntityId, FlowGraphEditorRequestsBus, RemoveFlowGraph, flowGraphToRemove, false);
                    }

                    CHyperGraph* pGraph = NULL;

                    // TODO: If we have a non-empty adjacent item, auto select it. LMBR-18624
                    if (hAdjacentItem && hAdjacentItem->childCount())
                    {
                        pGraph = (CHyperGraph*)m_graphsTreeCtrl->GetItemData(hAdjacentItem);
                    }

                    if (pGraph)
                    {
                        SetGraph(pGraph);
                    }
                    else
                    {
                        if (m_widget)
                        {
                            m_widget->SetGraph(0);
                        }
                        if (m_taskPanel)
                        {
                            m_taskPanel->GetGraphProperties()->SetGraph(0);
                            m_taskPanel->GetTokensCtrl()->SetGraph(0);
                        }
                        UpdateTitle(0);
                        m_graphIterCur = m_graphList.end();
                        if (hAdjacentItem)
                        {
                            hAdjacentItem->setSelected(true);
                        }
                    }
                }
            }
        }
        break;
        case ID_GRAPHS_ENABLE:
        {
            if (pFlowGraph)
            {
                // Toggle enabled of graph.
                pFlowGraph->SetEnabled(!pGraph->IsEnabled());
                // GraphUpdateEnable(pFlowGraph, hItem);
                UpdateGraphProperties(pGraph);
                m_graphsTreeCtrl->update();
            }
        }
        break;
        case ID_GRAPHS_RENAME_GRAPH:
        {
            if (pFlowGraph)
            {
                bool ok = false;
                QString newName = QInputDialog::getText(this, tr("Rename"), tr("Edit Flow Graph name"), QLineEdit::Normal, pFlowGraph->GetName(), &ok);
                if (ok)
                {
                    if (newName.isEmpty())
                    {
                        QMessageBox::warning(this, tr("Rename"), tr("You supplied an empty name, no change was made"));
                        break;
                    }

                    // rename flowgraph
                    pFlowGraph->SetName(newName);
                    UpdateGraphProperties(pGraph);
                    m_graphsTreeCtrl->update();
                    m_graphsTreeCtrl->Reload();
                    SetGraph(0);
                }
            }
        }
        break;
        case ID_GRAPHS_CHANGE_FOLDER:
        {
            if (pFlowGraph)
            {
                /*
                CStringDlg dlg( "Change Graph Folder" );
                dlg.SetString( pGraph->GetGroupName() );
                if (dlg.DoModal() == IDOK)
                {
                pGraph->SetGroupName( dlg.GetString() );
                m_graphsTreeCtrl.Reload();
                }
                */
                if (pGraph)
                {
                    cSelectedGraphs.push_back(pGraph);
                }

                if (nCurrentSelectedItem == nTotalSelectedItems - 1)
                {
                    bool bIsAction = pFlowGraph->GetAIAction();
                    CFlowGraphManager* pFlowGraphManager = (CFlowGraphManager*) pFlowGraph->GetManager();
                    std::set<QString> groupsSet;
                    pFlowGraphManager->GetAvailableGroups(groupsSet, bIsAction);

                    QString groupName;
                    bool bDoNew  = true;
                    bool bChange = false;

                    if (groupsSet.size() > 0)
                    {
                        std::vector<QString> groups(groupsSet.begin(), groupsSet.end());

                        CGenericSelectItemDialog gtDlg;
                        if (bIsAction == false)
                        {
                            gtDlg.setWindowTitle(tr("Choose Group for the Flow Graph"));
                        }
                        else
                        {
                            gtDlg.setWindowTitle(tr("Choose Group for the AIAction Graph(s)"));
                        }

                        gtDlg.PreSelectItem(pFlowGraph->GetGroupName());
                        gtDlg.SetItems(groups);
                        gtDlg.AllowNew(true);
                        switch (gtDlg.exec())
                        {
                        case QDialog::Accepted:
                            groupName = gtDlg.GetSelectedItem();
                            bChange = true;
                            bDoNew  = false;
                            break;
                        case CGenericSelectItemDialog::New:
                            // bDoNew  = true; // is true anyway
                            break;
                        default:
                            bDoNew = false;
                            break;
                        }
                    }

                    if (bDoNew)
                    {
                        QString r = QInputDialog::getText(this, tr("Rename"),
                                bIsAction == false ? tr("Enter Group Name for the Flow Graph(s)") : tr("Enter Group Name for the AIAction Graph(s)"), QLineEdit::Normal, pFlowGraph->GetGroupName());
                        if (!r.isEmpty() && r != pFlowGraph->GetGroupName())
                        {
                            bChange = true;
                            groupName = r;
                        }
                    }

                    int nTotalSelectedGraphs(0);
                    int nCurrentlySelectedGraph(0);

                    nTotalSelectedGraphs = cSelectedGraphs.size();
                    for (nCurrentlySelectedGraph = 0; nCurrentlySelectedGraph < nTotalSelectedGraphs; ++nCurrentlySelectedGraph)
                    {
                        CHyperGraph*&  rpoCurrentGraph = cSelectedGraphs[nCurrentlySelectedGraph];
                        if (bChange && groupName != rpoCurrentGraph->GetGroupName())
                        {
                            rpoCurrentGraph->SetGroupName(groupName);
                            m_graphsTreeCtrl->Reload();
                        }
                    }
                }
            }
        }
        break;
        case IDC_NEW:
            break;
        case ID_GRAPHS_ENABLE_DEBUGGING:
        {
            if (pFlowGraph && m_pFlowGraphDebugger)
            {
                m_pFlowGraphDebugger->RemoveIgnoredFlowgraph(pFlowGraph->GetIFlowGraph());
            }
        }
        break;
        case ID_GRAPHS_DISABLE_DEBUGGING:
        {
            if (pFlowGraph && m_pFlowGraphDebugger)
            {
                m_pFlowGraphDebugger->AddIgnoredFlowgraph(pFlowGraph->GetIFlowGraph());
            }
        }
        break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphDialog::DeleteItem(CFlowGraph* pGraph)
{
    QTreeWidgetItem* hAdjacentItem = NULL;
    if (CEntityObject* pEntity = pGraph->GetEntity())
    {
        m_graphsTreeCtrl->DeleteEntityItem(pEntity, hAdjacentItem);
    }
    else
    {
        m_graphsTreeCtrl->DeleteNonEntityItem(pGraph, hAdjacentItem);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CHyperGraphDialog::IsAnEntityFolderOrSubFolder(QTreeWidgetItem* hSelectedItem)
{
    QTreeWidgetItem*    hParentItem(NULL);
    QString     strGroupName;

    if (IsAssociatedToFlowGraph(hSelectedItem))
    {
        return false;
    }

    hParentItem = hSelectedItem->parent();
    strGroupName = hParentItem ? hSelectedItem->text(0) : "";

    if (
        (strGroupName == g_EntitiesFolderName)
        ||
        (hParentItem != 0 && hParentItem->text(0) == g_EntitiesFolderName)
        )
    {
        return true;
    }

    return false;
}
//////////////////////////////////////////////////////////////////////////
bool CHyperGraphDialog::IsAnEntitySubFolder(QTreeWidgetItem* hSelectedItem)
{
    QTreeWidgetItem*    hParentItem(NULL);
    QString     strGroupName;

    if (IsAssociatedToFlowGraph(hSelectedItem))
    {
        return false;
    }

    hParentItem = hSelectedItem->parent();
    strGroupName = hParentItem ? hParentItem->text(0) : "";

    if (hParentItem != 0 &&
        (strGroupName != g_AIActionsFolderName) &&
        (strGroupName != g_GlobalFlowgraphModulesFolderName) &&
        (strGroupName != g_LevelFlowgraphModulesFolderName) &&
        (strGroupName != g_ComponentEntityFolderName) &&
        (strGroupName != g_LevelFlowgraphs) &&
        (strGroupName != g_GlobalFlowgraphsFolderName)
        )
    {
        return true;
    }

    return false;
}
//////////////////////////////////////////////////////////////////////////
bool CHyperGraphDialog::IsGraphEnabled(QTreeWidgetItem* hSelectedItem, bool& boIsEnabled)
{
    CHyperGraph* pGraph = 0;
    CFlowGraph* pFlowGraph = 0;

    pGraph = (CHyperGraph*)m_graphsTreeCtrl->GetItemData(hSelectedItem);
    if (pGraph)
    {
        pFlowGraph = pGraph->IsFlowGraph() ? static_cast<CFlowGraph*> (pGraph) : 0;
    }

    if (pFlowGraph)
    {
        if (pFlowGraph->GetAIAction() == 0)
        {
            if (!pGraph->IsEnabled())
            {
                boIsEnabled = true;
            }
            else
            {
                boIsEnabled = false;
            }
            return true;
        }
    }

    return false;
}

bool CHyperGraphDialog::IsGraphType(QTreeWidgetItem* hSelectedItem, IFlowGraph::EFlowGraphType type)
{
    CHyperGraph* pGraph = 0;
    CFlowGraph* pFlowGraph = 0;
    CEntityObject* pOwnerEntity = 0;

    pGraph = (CHyperGraph*)m_graphsTreeCtrl->GetItemData(hSelectedItem);
    if (pGraph)
    {
        pFlowGraph = pGraph->IsFlowGraph() ? static_cast<CFlowGraph*> (pGraph) : 0;
    }

    if (pFlowGraph)
    {
        return pFlowGraph->GetIFlowGraph()->GetType() == type;
    }
    return false;
}

bool CHyperGraphDialog::IsEnabledForDebugging(QTreeWidgetItem* hSelectedItem)
{
    if (!m_pFlowGraphDebugger)
    {
        return false;
    }

    if (CHyperGraph* pHypergraph = (CHyperGraph*)m_graphsTreeCtrl->GetItemData(hSelectedItem))
    {
        if (pHypergraph->IsFlowGraph())
        {
            CFlowGraph* pFlowgraph = static_cast<CFlowGraph*>(pHypergraph);

            return m_pFlowGraphDebugger->IsFlowgraphIgnored(pFlowgraph->GetIFlowGraph()) || m_pFlowGraphDebugger->IsFlowgraphTypeIgnored(pFlowgraph->GetIFlowGraph()->GetType());
        }
    }

    return false;
}

bool CHyperGraphDialog::IsComponentEntity(QTreeWidgetItem* hSelectedItem)
{
    CHyperGraph* pGraph = 0;
    CFlowGraph* pFlowGraph = 0;
    CEntityObject* pOwnerEntity = 0;

    pGraph = (CHyperGraph*)m_graphsTreeCtrl->GetItemData(hSelectedItem);
    if (pGraph)
    {
        pFlowGraph = pGraph->IsFlowGraph() ? static_cast<CFlowGraph*> (pGraph) : 0;
    }

    if (pFlowGraph)
    {
        pOwnerEntity = pFlowGraph->GetEntity();
        if (pOwnerEntity && QString::compare(pOwnerEntity->GetClassDesc()->ClassName(), "ComponentEntity") == 0)
        {
            return true;
        }
    }
    return false;
}

bool CHyperGraphDialog::IsGlobalModuleFolder(QTreeWidgetItem* hSelectedItem)
{
    CHyperGraph* pGraph = m_graphsTreeCtrl->GetItemData(hSelectedItem);
    if (!pGraph || !pGraph->IsFlowGraph())
    {
        return false;
    }

    CFlowGraph* pFlowGraph = static_cast<CFlowGraph*>(pGraph);
    IFlowGraphModule* pModule = gEnv->pFlowSystem->GetIModuleManager()->GetModule(pFlowGraph->GetName().toUtf8().data());
    if (!pModule)
    {
        return false;
    }

    return pModule->GetType() == IFlowGraphModule::eT_Global;
}

bool CHyperGraphDialog::IsLevelModuleFolder(QTreeWidgetItem* hSelectedItem)
{
    CHyperGraph* pGraph = m_graphsTreeCtrl->GetItemData(hSelectedItem);
    if (!pGraph || !pGraph->IsFlowGraph())
    {
        return false;
    }

    CFlowGraph* pFlowGraph = static_cast<CFlowGraph*>(pGraph);
    IFlowGraphModule* pModule = gEnv->pFlowSystem->GetIModuleManager()->GetModule(pFlowGraph->GetName().toUtf8().data());
    if (!pModule)
    {
        return false;
    }

    return pModule->GetType() == IFlowGraphModule::eT_Level;
}

//////////////////////////////////////////////////////////////////////////
bool CHyperGraphDialog::IsOwnedByAnEntity(QTreeWidgetItem* hSelectedItem)
{
    CHyperGraph* pGraph = 0;
    CFlowGraph* pFlowGraph = 0;
    CEntityObject* pOwnerEntity = 0;

    pGraph = (CHyperGraph*)m_graphsTreeCtrl->GetItemData(hSelectedItem);
    if (pGraph)
    {
        pFlowGraph = pGraph->IsFlowGraph() ? static_cast<CFlowGraph*> (pGraph) : 0;
    }

    if (pFlowGraph)
    {
        pOwnerEntity = pFlowGraph->GetEntity();
        if (pOwnerEntity)
        {
            return true;
        }
    }
    return false;
}
//////////////////////////////////////////////////////////////////////////
bool CHyperGraphDialog::IsAssociatedToFlowGraph(QTreeWidgetItem* hSelectedItem)
{
    CHyperGraph* pGraph = (CHyperGraph*)m_graphsTreeCtrl->GetItemData(hSelectedItem);

    if (pGraph)
    {
        return pGraph->IsFlowGraph();
    }

    return false;
}

bool CHyperGraphDialog::CreateBreakpointContextMenu(QMenu& roMenu, QTreeWidgetItem* hItem)
{
    QVariant v = hItem->data(0, CBreakpointsTreeCtrl::BreakPointItemRole);
    if (!v.isValid())
    {
        return false;
    }

    SBreakpointItem breakpointItem = v.value<SBreakpointItem>();

    switch (breakpointItem.type)
    {
    case SBreakpointItem::eIT_Graph:
    {
        roMenu.addAction("Remove Breakpoints for Graph")->setData(ID_BREAKPOINT_REMOVE_GRAPH);
    }
    break;
    case SBreakpointItem::eIT_Node:
    {
        roMenu.addAction("Remove Breakpoints for Node")->setData(ID_BREAKPOINT_REMOVE_NODE);
    }
    break;
    case SBreakpointItem::eIT_Port:
    {
        if (m_pFlowGraphDebugger)
        {
            roMenu.addAction("Remove Breakpoint")->setData(ID_BREAKPOINT_REMOVE_PORT);
            roMenu.addSeparator();

            const bool bIsEnabled = m_pFlowGraphDebugger->IsBreakpointEnabled(breakpointItem.flowgraph, breakpointItem.addr);
            QAction* a = roMenu.addAction("Enabled");
            a->setData(ID_BREAKPOINT_ENABLE);
            a->setCheckable(true);
            a->setChecked(bIsEnabled);
            const bool bIsTracepoint = m_pFlowGraphDebugger->IsTracepoint(breakpointItem.flowgraph, breakpointItem.addr);
            a = roMenu.addAction("Tracepoint");
            a->setData(ID_TRACEPOINT_ENABLE);
            a->setCheckable(true);
            a->setChecked(bIsTracepoint);
        }
    }

    break;
    }

    return true;
}

void CHyperGraphDialog::ReloadBreakpoints()
{
    m_breakpointsPanel->treeCtrl()->FillBreakpoints();
}

void CHyperGraphDialog::ClearBreakpoints()
{
    m_breakpointsPanel->treeCtrl()->DeleteAllItems();
}

void CHyperGraphDialog::ProcessBreakpointMenu(int nMenuOption, QTreeWidgetItem* hItem)
{
    switch (nMenuOption)
    {
    case ID_BREAKPOINT_REMOVE_GRAPH:
    {
        m_breakpointsPanel->treeCtrl()->RemoveBreakpointsForGraph(hItem);
    }
    break;
    case ID_BREAKPOINT_REMOVE_NODE:
    {
        m_breakpointsPanel->treeCtrl()->RemoveBreakpointsForNode(hItem);
    }
    break;
    case ID_BREAKPOINT_REMOVE_PORT:
    {
        m_breakpointsPanel->treeCtrl()->RemoveBreakpointForPort(hItem);
    }
    break;
    case ID_BREAKPOINT_ENABLE:
    case ID_TRACEPOINT_ENABLE:
    {
        if (m_pFlowGraphDebugger)
        {
            QVariant v = hItem->data(0, CBreakpointsTreeCtrl::BreakPointItemRole);
            if (!v.isValid())
            {
                return;
            }
            SBreakpointItem breakpointItem = v.value<SBreakpointItem>();

            if (nMenuOption == ID_BREAKPOINT_ENABLE)
            {
                const bool bIsEnabled = m_pFlowGraphDebugger->IsBreakpointEnabled(breakpointItem.flowgraph, breakpointItem.addr);
                m_breakpointsPanel->treeCtrl()->EnableBreakpoint(hItem, !bIsEnabled);
            }
            else
            {
                const bool bIsTracepoint = m_pFlowGraphDebugger->IsTracepoint(breakpointItem.flowgraph, breakpointItem.addr);
                m_breakpointsPanel->treeCtrl()->EnableTracepoint(hItem, !bIsTracepoint);
            }
        }
    }
    break;
    }
    ;
}

void CHyperGraphDialog::OnBreakpointsClick(QTreeWidgetItem* item, int column)
{
    QVariant v = item->data(0, CBreakpointsTreeCtrl::BreakPointItemRole);
    if (v.isValid())
    {
        SBreakpointItem breakpointItem = v.value<SBreakpointItem>();
        CFlowGraph* pFlowgraph = GetIEditor()->GetFlowGraphManager()->FindGraph(breakpointItem.flowgraph);

        if (pFlowgraph == NULL)
        {
            return;
        }

        switch (breakpointItem.type)
        {
        case SBreakpointItem::eIT_Graph:
        {
            SetGraph(pFlowgraph);
            if (m_widget)
            {
                m_widget->FitFlowGraphToView();
            }
        }
        break;
        case SBreakpointItem::eIT_Node:
        case SBreakpointItem::eIT_Port:
        {
            SetGraph(pFlowgraph);

            CFlowNode* pNode = pFlowgraph->FindFlowNode(breakpointItem.addr.node);

            if (pNode)
            {
                if (m_widget)
                {
                    m_widget->CenterViewAroundNode(pNode);
                }
                pFlowgraph->UnselectAll();
                pNode->SetSelected(true);
            }
        }
        break;
        }
    }
}

void CHyperGraphDialog::CenterViewAroundNode(CHyperNode* poNode, bool boFitZoom /*= false*/, float fZoom /*= -1.0f*/)
{
    if (m_widget)
    {
        m_widget->CenterViewAroundNode(poNode, boFitZoom, fZoom);
    }
}

void CHyperGraphDialog::SetFocusOnView()
{
    if (m_widget)
    {
        m_widget->setFocus();
    }
}

void CHyperGraphDialog::InvalidateView(bool bComplete /*= false*/)
{
    if (m_widget)
    {
        m_widget->InvalidateView(bComplete);
    }
}

void CHyperGraphDialog::ShowAndSelectNode(CHyperNode* pToNode, bool bSelect /*= true*/)
{
    if (m_widget)
    {
        m_widget->ShowAndSelectNode(pToNode, bSelect);
    }
}



void CHyperGraphDialog::OnEditUndo()
{
    GetIEditor()->Undo();
}

void CHyperGraphDialog::OnEditRedo()
{
    GetIEditor()->Redo();
}


#include <HyperGraph/HyperGraphDialog.moc>
