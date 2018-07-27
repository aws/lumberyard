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
#include "PrefabDialog.h"

#include "StringDlg.h"

#include "PrefabManager.h"
#include "PrefabLibrary.h"
#include "PrefabItem.h"

#include "Objects/PrefabObject.h"

#include "ViewManager.h"
#include "Clipboard.h"

#include <I3DEngine.h>
//#include <IEntityRenderState.h>
#include <IEntitySystem.h>

#include "../Plugins/EditorUI_QT/Toolbar.h"

#include <QDropEvent>
#include <QMenu>
#include <QSettings>
#include <QSplitter>
#include <QVBoxLayout>

#include <QDebug>

//////////////////////////////////////////////////////////////////////////
class CPrefabPickCallback
    : public IPickObjectCallback
{
public:
    CPrefabPickCallback() { m_bActive = true; };
    //! Called when object picked.
    virtual void OnPick(CBaseObject* picked)
    {
        /*
        m_bActive = false;
        CPrefabItem *pParticles = picked->GetParticle();
        if (pParticles)
            GetIEditor()->OpenPrefabLibrary( pParticles );
            */
        delete this;
    }
    //! Called when pick mode cancelled.
    virtual void OnCancelPick()
    {
        m_bActive = false;
        delete this;
    }
    //! Return true if specified object is pickable.
    virtual bool OnPickFilter(CBaseObject* filterObject)
    {
        /*
        // Check if object have material.
        if (filterObject->GetParticle())
            return true;
        */
        return false;
    }
    static bool IsActive() { return m_bActive; };
private:
    static bool m_bActive;
};
bool CPrefabPickCallback::m_bActive = false;
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// CPrefabDialog implementation.
//////////////////////////////////////////////////////////////////////////
CPrefabDialog::CPrefabDialog(QWidget* pParent)
    : CBaseLibraryDialog(IDD_DB_ENTITY, pParent)
    , m_treeModel(new DefaultBaseLibraryDialogModel(this))
    , m_proxyModel(new BaseLibraryDialogProxyModel(this, m_treeModel, m_treeModel))
{
    GetIEditor()->GetObjectManager()->AddObjectEventListener(functor(*this, &CPrefabDialog::OnObjectEvent));
    m_pPrefabManager = GetIEditor()->GetPrefabManager();
    m_pItemManager = m_pPrefabManager;

    OnInitDialog();
}

CPrefabDialog::~CPrefabDialog()
{
    QSettings settings("Amazon", "Lumberyard");
    settings.setValue(QLatin1String("Dialogs/PrefabsEditor/VSplitter"), m_wndVSplitter->sizes().value(1));

    GetIEditor()->GetObjectManager()->RemoveObjectEventListener(functor(*this, &CPrefabDialog::OnObjectEvent));
}

// CTVSelectKeyDialog message handlers
void CPrefabDialog::OnInitDialog()
{
    CBaseLibraryDialog::OnInitDialog();

    InitLibraryToolbar();

    m_itemAssignAction = m_pItemToolBar->getAction("actionItemAssign");
    m_itemGetPropertiesAction = m_pItemToolBar->getAction("actionItemGetProperties");
    connect(m_pItemToolBar->getAction("actionItemAdd"), &QAction::triggered, this, &CPrefabDialog::OnAddItem);
    connect(m_itemAssignAction, &QAction::triggered, this, &CPrefabDialog::OnAssignToSelection);
    connect(m_itemGetPropertiesAction, &QAction::triggered, this, &CPrefabDialog::OnGetFromSelection);

    QWidget* mainWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(mainWidget);
    setCentralWidget(mainWidget);

    m_treeCtrl = new BaseLibraryDialogTree(this);
    m_treeCtrl->setModel(m_proxyModel);
    GetTreeCtrl()->setContextMenuPolicy(Qt::CustomContextMenu);
    GetTreeCtrl()->setHeaderHidden(true);
    GetTreeCtrl()->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    GetTreeCtrl()->setDragDropMode(QAbstractItemView::DragOnly);

    // The tree only allows single selection
    connect(GetTreeCtrl()->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this](const QItemSelection& in, const QItemSelection& out) {
        auto inIndices = in.indexes();
        auto outIndices = out.indexes();
        OnSelChangedItemTree(inIndices.isEmpty() ? QModelIndex() : inIndices.first(), outIndices.isEmpty() ? QModelIndex() : outIndices.first());
    });

    connect(GetTreeCtrl(), &QTreeView::customContextMenuRequested, this, &CPrefabDialog::OnNotifyMtlTreeRClick);
    connect(qobject_cast<BaseLibraryDialogTree*>(GetTreeCtrl()), &BaseLibraryDialogTree::droppedOnViewport, this, &CPrefabDialog::droppedOnViewport);

    m_treeContextMenu = new QMenu(GetTreeCtrl());
    connect(m_treeContextMenu, &QMenu::aboutToShow, this, &CPrefabDialog::OnUpdateActions);

    m_cutAction = m_treeContextMenu->addAction(tr("Cut"));
    m_cutAction->setShortcut(QKeySequence::Cut);
    connect(m_cutAction, &QAction::triggered, this, &CPrefabDialog::OnCut);

    m_copyAction = m_treeContextMenu->addAction(tr("Copy"));
    m_copyAction->setShortcut(QKeySequence::Copy);
    connect(m_copyAction, &QAction::triggered, this, &CPrefabDialog::OnCopy);

    m_pasteAction = m_treeContextMenu->addAction(tr("Paste"));
    m_pasteAction->setShortcut(QKeySequence::Paste);
    connect(m_pasteAction, &QAction::triggered, this, &CPrefabDialog::OnPaste);

    m_cloneAction = m_treeContextMenu->addAction(tr("Clone"));
    m_cloneAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_D));
    connect(m_cloneAction, &QAction::triggered, this, &CPrefabDialog::OnClone);

    m_treeContextMenu->addSeparator();

    QAction* rename = m_treeContextMenu->addAction(tr("Rename"));
    connect(rename, &QAction::triggered, this, &CPrefabDialog::OnRenameItem);

    QAction* deleteAction = m_treeContextMenu->addAction(tr("Delete"));
    deleteAction->setShortcut(QKeySequence(Qt::Key_Delete));
    connect(deleteAction, &QAction::triggered, this, &CPrefabDialog::OnRemoveItem);

    m_treeContextMenu->addSeparator();

    QAction* replace = m_treeContextMenu->addAction(tr("Replace Prefab with Selected Objects"));
    connect(replace, &QAction::triggered, this, &CPrefabDialog::OnMakeFromSelection);

    m_treeContextMenu->addAction(m_pItemToolBar->getAction("actionItemAssign"));

    m_selectAction = m_treeContextMenu->addAction(tr("Select all Instances of this Prefab"));
    connect(m_selectAction, &QAction::triggered, this, &CPrefabDialog::OnSelectAssignedObjects);

    QAction* insert = new QAction(GetTreeCtrl());
    insert->setShortcut(Qt::Key_Insert);
    connect(insert, &QAction::triggered, this, &CPrefabDialog::OnAddItem);

    GetTreeCtrl()->addActions(m_treeContextMenu->actions());
    GetTreeCtrl()->addAction(insert);

    int h2 = 200;

    QSettings settings("Amazon", "Lumberyard");
    const int VSplitter = settings.value(QLatin1String("Dialogs/PrefabsEditor/VSplitter"), h2).toInt();

    m_wndVSplitter = new QSplitter(Qt::Horizontal, this);

    m_propsCtrl = new ReflectedPropertyControl(this);
    m_propsCtrl->Setup();

    m_wndVSplitter->addWidget(GetTreeCtrl());
    m_wndVSplitter->addWidget(m_propsCtrl);
    m_wndVSplitter->setSizes({ 80, VSplitter });

    mainLayout->addWidget(m_wndVSplitter);

    ReloadLibs();
}

//////////////////////////////////////////////////////////////////////////
UINT CPrefabDialog::GetDialogMenuID()
{
    return IDR_DB_ENTITY;
};

//////////////////////////////////////////////////////////////////////////
// Create the toolbar
void CPrefabDialog::InitToolbar(UINT nToolbarResID)
{
    /*
    VERIFY( m_toolbar.CreateEx(this, TBSTYLE_FLAT|TBSTYLE_WRAPABLE,
        WS_CHILD|WS_VISIBLE|CBRS_TOP|CBRS_TOOLTIPS|CBRS_FLYBY|CBRS_SIZE_DYNAMIC) );
    VERIFY( m_toolbar.LoadToolBar24(IDR_DB_PREFAB_BAR) );

    // Resize the toolbar
    CRect rc;
    GetClientRect(rc);
    m_toolbar.SetWindowPos(NULL, 0, 0, rc.right, 70, SWP_NOZORDER);
    CSize sz = m_toolbar.CalcDynamicLayout(TRUE,TRUE);

    CBaseLibraryDialog::InitToolbar(nToolbarResID);
    */
}

//////////////////////////////////////////////////////////////////////////
void CPrefabDialog::OnAddItem()
{
    if (!m_pLibrary)
    {
        return;
    }

    QString groupName(m_selectedGroup);
    QString itemName;

    if (!PromptForPrefabName(groupName, itemName))
    {
        return; // cancelled!
    }

    CUndo undo("Add prefab library item");
    CPrefabItem* pItem = (CPrefabItem*)m_pItemManager->CreateItem(m_pLibrary);

    // Make prototype name.
    SetItemName(pItem, groupName, itemName);
    pItem->Update();

    ReloadItems();
    SelectItem(pItem);
}

//////////////////////////////////////////////////////////////////////////
void CPrefabDialog::SelectItem(CBaseLibraryItem* item, bool bForceReload)
{
    bool bChanged = item != m_pCurrentItem || bForceReload;
    CBaseLibraryDialog::SelectItem(item, bForceReload);

    if (!bChanged)
    {
        return;
    }

    if (!item)
    {
        m_propsCtrl->setEnabled(false);
        return;
    }

    m_propsCtrl->setEnabled(true);
    m_propsCtrl->EnableUpdateCallback(false);

    // Render preview geometry with current material
    CPrefabItem* prefab = GetSelectedPrefab();

    //AssignMtlToGeometry();

    // Update variables.
    m_propsCtrl->EnableUpdateCallback(false);
    //gParticleUI.SetFromParticles( pParticles );
    m_propsCtrl->EnableUpdateCallback(true);

    //m_propsCtrl.SetUpdateCallback( functor(*this,OnUpdateProperties) );
    m_propsCtrl->EnableUpdateCallback(true);
}

//////////////////////////////////////////////////////////////////////////
QTreeView* CPrefabDialog::GetTreeCtrl()
{
    return m_treeCtrl;
}

//////////////////////////////////////////////////////////////////////////
AbstractGroupProxyModel* CPrefabDialog::GetProxyModel()
{
    return m_proxyModel;
}

//////////////////////////////////////////////////////////////////////////
QAbstractItemModel* CPrefabDialog::GetSourceModel()
{
    return m_treeModel;
}

//////////////////////////////////////////////////////////////////////////
BaseLibraryDialogModel* CPrefabDialog::GetSourceDialogModel()
{
    return m_treeModel;
}

const char * CPrefabDialog::GetLibraryAssetTypeName() const
{
    return "Prefabs Library";
}

//////////////////////////////////////////////////////////////////////////
void CPrefabDialog::Update()
{
}

//////////////////////////////////////////////////////////////////////////
void CPrefabDialog::OnUpdateProperties(IVariable* var)
{
    CPrefabItem* pPrefab = GetSelectedPrefab();
    if (!pPrefab)
    {
        return;
    }

    //gParticleUI.SetToParticles( pParticles );

    //AssignMtlToGeometry();

    GetIEditor()->SetModifiedFlag();
}

//////////////////////////////////////////////////////////////////////////
CPrefabItem* CPrefabDialog::GetSelectedPrefab()
{
    CBaseLibraryItem* pItem = m_pCurrentItem;
    return (CPrefabItem*)pItem;
}

//////////////////////////////////////////////////////////////////////////
void CPrefabDialog::OnAssignToSelection()
{
    CPrefabItem* pPrefab = GetSelectedPrefab();
    if (!pPrefab)
    {
        return;
    }

    CUndo undo("Assign Prefab");

    CSelectionGroup* pSel = GetIEditor()->GetSelection();
    if (!pSel->IsEmpty())
    {
        for (int i = 0; i < pSel->GetCount(); i++)
        {
            CBaseObject* pObject = pSel->GetObject(i);
            if (qobject_cast<CPrefabObject*>(pObject))
            {
                CPrefabObject* pPrefabObject = (CPrefabObject*)pObject;
                pPrefabObject->SetPrefab(pPrefab, false);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabDialog::OnSelectAssignedObjects()
{
    CPrefabItem* pPrefab = GetSelectedPrefab();
    if (!pPrefab)
    {
        return;
    }

    CBaseObjectsArray objects;

    GetIEditor()->GetObjectManager()->FindObjectsOfType(&CPrefabObject::staticMetaObject, objects);
    if (!objects.empty())
    {
        GetIEditor()->ClearSelection();
    }

    for (int i = 0; i < objects.size(); i++)
    {
        CBaseObject* pObject = objects[i];
        CPrefabObject* pPrefabObject = static_cast<CPrefabObject*>(pObject);
        if (pPrefabObject->GetPrefab() == pPrefab)
        {
            GetIEditor()->SelectObject(pPrefabObject);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabDialog::OnGetFromSelection()
{
    if (!m_pLibrary)
    {
        return;
    }

    CSelectionGroup* pSel = GetIEditor()->GetSelection();
    if (pSel->IsEmpty())
    {
        return;
    }

    if (pSel->GetCount() == 1)
    {
        CBaseObject* pObject = pSel->GetObject(0);
        if (qobject_cast<CPrefabObject*>(pObject))
        {
            CPrefabObject* pPrefabObject = (CPrefabObject*)pObject;
            SelectItem(pPrefabObject->GetPrefab());
            return;
        }
    }

    QString groupName(m_selectedGroup);
    QString itemName;

    if (!PromptForPrefabName(groupName, itemName))
    {
        return; // cancelled!
    }

    CPrefabItem* pPrefab = (CPrefabItem*)m_pItemManager->CreateItem(m_pLibrary);
    SetItemName(pPrefab, groupName, itemName);

    // Serialize these objects into prefab.
    pPrefab->MakeFromSelection(*pSel);


    ReloadItems();
    SelectItem(pPrefab);
}

//////////////////////////////////////////////////////////////////////////
void CPrefabDialog::OnMakeFromSelection()
{
    if (!m_pLibrary)
    {
        return;
    }

    CPrefabItem* pPrefab = GetSelectedPrefab();
    if (!pPrefab)
    {
        return;
    }

    CSelectionGroup* pSel = GetIEditor()->GetSelection();
    if (pSel->IsEmpty())
    {
        return;
    }

    GUID stItemGuid = pPrefab->GetGUID();
    for (int i = 0; i < pSel->GetCount(); ++i)
    {
        CBaseObject* pObject = pSel->GetObject(i);
        if (qobject_cast<CPrefabObject*>(pObject))
        {
            CPrefabObject* pPrefabObject = (CPrefabObject*)pObject;
            CPrefabItem* pPerfabObjectItem = pPrefabObject->GetPrefab();
            if (pPerfabObjectItem->GetGUID() == stItemGuid)
            {
                GetIEditor()->GetPrefabManager()->AddSelectionToPrefab();
                return;
            }
        }
    }

    // Serialize these objects into prefab.
    pPrefab->MakeFromSelection(*pSel);
    UpdatePrefabObjects(pPrefab);

    ReloadItems();
    SelectItem(pPrefab);
}

//////////////////////////////////////////////////////////////////////////
void CPrefabDialog::UpdatePrefabObjects(CPrefabItem* pPrefab)
{
    assert(pPrefab);
    // Update all existing prefabs.
    CBaseObjectsArray objects;
    GetIEditor()->GetObjectManager()->GetObjects(objects);

    std::vector< _smart_ptr<CPrefabObject> > prefabObjects;

    for (int i = 0; i < objects.size(); i++)
    {
        CBaseObject* pObject = objects[i];
        if (qobject_cast<CPrefabObject*>(pObject))
        {
            CPrefabObject* pPrefabObject = (CPrefabObject*)pObject;
            if (pPrefabObject->GetPrefab() == pPrefab)
            {
                prefabObjects.push_back(pPrefabObject);
            }
        }
    }

    for (int i = 0, iPrefabObjectCount(prefabObjects.size()); i < iPrefabObjectCount; ++i)
    {
        prefabObjects[i]->SetPrefab(pPrefab, true);
    }
}

void CPrefabDialog::OnUpdateActions()
{
    if (m_itemAssignAction)
    {
        const bool enable = GetSelectedPrefab() && !GetIEditor()->GetSelection()->IsEmpty();
        m_itemAssignAction->setEnabled(enable);
    }

    if (m_itemGetPropertiesAction)
    {
        const bool enable = !GetIEditor()->GetSelection()->IsEmpty();
        m_itemGetPropertiesAction->setEnabled(enable);
    }

    CBaseLibraryDialog::OnUpdateActions();
}

//////////////////////////////////////////////////////////////////////////
void CPrefabDialog::dragEnterEvent(QDragEnterEvent* event)
{
    const QModelIndex index = GetTreeCtrl()->indexAt(event->pos());
    if (event->source() == GetTreeCtrl())
    {
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void CPrefabDialog::dragMoveEvent(QDragMoveEvent* event)
{
    const QModelIndex index = GetTreeCtrl()->indexAt(event->pos());
    if (event->source() == GetTreeCtrl() && ((index.isValid() && !GetTreeCtrl()->selectionModel()->isSelected(index)) || !index.isValid()))
    {
        event->accept(GetTreeCtrl()->visualRect(index));
    }
    else
    {
        event->ignore();
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabDialog::dropEvent(QDropEvent* event)
{
    // Dropped inside tree.
    DropToItem(GetTreeCtrl()->indexAt(event->pos()), GetTreeCtrl()->currentIndex(), static_cast<CPrefabItem*>(GetTreeCtrl()->currentIndex().data(BaseLibraryItemRole).value<CBaseLibraryItem*>()));
    event->accept();
}

void CPrefabDialog::droppedOnViewport(const QModelIndexList& indexes, CViewport* viewport)
{
    // Not droped inside tree.
    CUndo undo("Create Prefab");
    CPrefabItem* item = static_cast<CPrefabItem*>(indexes.first().data(BaseLibraryItemRole).value<CBaseLibraryItem*>());
    QString guid = GuidUtil::ToString(item->GetGUID());
    GetIEditor()->StartObjectCreation(item->GetPrefabObjectClassName(), guid);
}

//////////////////////////////////////////////////////////////////////////
void CPrefabDialog::OnObjectEvent(CBaseObject* pObject, int nEvent)
{
    if (m_pLibrary)
    {
        if (qobject_cast<CPrefabObject*>(pObject))
        {
            if (nEvent == CBaseObject::ON_ADD || nEvent == CBaseObject::ON_DELETE)
            {
                ReloadItems();

                CPrefabObject* pPrefabObject = (CPrefabObject*)pObject;
                for (int i = 0; i < m_pLibrary->GetItemCount(); ++i)
                {
                    CPrefabItem* pItem = (CPrefabItem*)m_pLibrary->GetItem(i);
                    if (pItem && pItem->GetGUID() == pPrefabObject->GetPrefabGuid())
                    {
                        SelectItem(pItem);
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabDialog::OnNotifyMtlTreeRClick(const QPoint& point)
{
    CBaseLibraryItem* pParticles = 0;

    // Find node under mouse.
    // Select the item that is at the point myPoint.
    const QModelIndex item = GetTreeCtrl()->indexAt(point);
    pParticles = item.data(BaseLibraryItemRole).value<CBaseLibraryItem*>();

    if (!pParticles)
    {
        return;
    }

    SelectItem(pParticles);

    // Create pop up menu.
    CClipboard clipboard(this);
    m_pasteAction->setEnabled(!clipboard.IsEmpty());
    m_treeContextMenu->exec(QCursor::pos());
}

//////////////////////////////////////////////////////////////////////////
void CPrefabDialog::OnCopy()
{
    CPrefabItem* pItem = GetSelectedPrefab();
    if (pItem)
    {
        XmlNodeRef node = XmlHelpers::CreateXmlNode("Prefab");
        CBaseLibraryItem::SerializeContext ctx(node, false);
        ctx.bCopyPaste = true;

        CClipboard clipboard(this);
        pItem->Serialize(ctx);
        clipboard.Put(node);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabDialog::OnPaste()
{
    if (!m_pLibrary)
    {
        return;
    }

    CPrefabItem* pItem = GetSelectedPrefab();
    if (!pItem)
    {
        return;
    }

    CClipboard clipboard(this);
    if (clipboard.IsEmpty())
    {
        return;
    }
    XmlNodeRef node = clipboard.Get();
    if (!node)
    {
        return;
    }

    if (strcmp(node->getTag(), "Prefab") == 0)
    {
        // This is material node.
        CUndo undo("Add prefab library item");
        CBaseLibraryItem* pItem = (CBaseLibraryItem*)m_pPrefabManager->CreateItem(m_pLibrary);
        if (pItem)
        {
            CBaseLibraryItem::SerializeContext serCtx(node, true);
            serCtx.bCopyPaste = true;
            pItem->Serialize(serCtx);
            pItem->SetName(m_pPrefabManager->MakeUniqueItemName(pItem->GetName()));
            ReloadItems();
            SelectItem(pItem);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabDialog::DropToItem(const QModelIndex& item, const QModelIndex& dragged, CPrefabItem* pParticles)
{
    pParticles->GetLibrary()->SetModified();

    TSmartPtr<CPrefabItem> pHolder = pParticles; // Make usre its not release while we remove and add it back.

    /*
    if (!hItem)
    {
        // Detach from parent.
        if (pParticles->GetParent())
            pParticles->GetParent()->RemoveChild( pParticles );

        ReloadItems();
        SelectItem( pParticles );
        return;
    }

    CPrefabItem* pTargetItem = (CPrefabItem*)m_treeCtrl.GetItemData(hItem);
    if (!pTargetItem)
    {
        // This is group.

        // Detach from parent.
        if (pParticles->GetParent())
            pParticles->GetParent()->RemoveChild( pParticles );

        // Move item to different group.
        CString groupName = m_treeCtrl.GetItemText(hItem);
        SetItemName( pParticles,groupName,pParticles->GetShortName() );

        m_treeCtrl.DeleteItem( hSrcItem );
        InsertItemToTree( pParticles,hItem );
        return;
    }
    // Ignore itself or immidiate target.
    if (pTargetItem == pParticles || pTargetItem == pParticles->GetParent())
        return;



    // Detach from parent.
    if (pParticles->GetParent())
        pParticles->GetParent()->RemoveChild( pParticles );

    // Attach to target.
    pTargetItem->AddChild( pParticles );

    ReloadItems();
    SelectItem( pParticles );
    */
}

//////////////////////////////////////////////////////////////////////////
CPrefabItem* CPrefabDialog::GetPrefabFromSelection()
{
    CPrefabItem* pItem = GetSelectedPrefab();
    OnGetFromSelection();
    CPrefabItem* pItemNew = GetSelectedPrefab();
    if (pItemNew != pItem)
    {
        return pItemNew;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
bool CPrefabDialog::PromptForPrefabName(QString& inOutGroupName, QString& inOutItemName)
{
    StringGroupDlg dlg(tr("New Prefab Name"), this);
    dlg.SetGroup(inOutGroupName);
    dlg.SetString(inOutItemName);

    // Give the user multiple attempts in order to find a name that can be accepted, without losing their current input.
    // this will loop and keep trying until the checks pass, or they hit ESC or CANCEL
    int dlgResult = dlg.exec();
    while (dlgResult == QDialog::Accepted)
    {
        if (dlg.GetString().isEmpty())
        {
            CryMessageBox("Please pick a name for the new Prefab", "Invalid name", MB_OK | MB_ICONERROR);
            dlgResult = dlg.exec();
            continue;
        }

        QString fullName = m_pItemManager->MakeFullItemName(m_pLibrary, dlg.GetGroup(), dlg.GetString());
        if (m_pItemManager->FindItemByName(fullName))
        {
            QString message = tr("Item with name %1 already exists").arg(fullName);
            CryMessageBox(message.toUtf8().constData(), "Invalid name", MB_OK | MB_ICONERROR);
            dlgResult = dlg.exec();
            continue;
        }
        // if we get here, it means that the above tests passed
        break;
    }

    if (dlgResult != QDialog::Accepted)
    {
        return false;
    }

    inOutGroupName = dlg.GetGroup();
    inOutItemName = dlg.GetString();

    return true;
}

#include <Prefabs/PrefabDialog.moc>
