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

#include "ParticleDialog.h"
#include <Particles/ui_ParticleDialog.h>

#include "StringDlg.h"

#include "ParticleManager.h"
#include "ParticleLibrary.h"
#include "ParticleItem.h"

#include "Objects/BrushObject.h"
#include "Objects/EntityObject.h"
#include "ViewManager.h"
#include "CryEditDoc.h"

// EditorCore
#include <Clipboard.h>
#include <Particles/ParticleUIDefinition.h>

#include <I3DEngine.h>
#include <ParticleParams.h>
#include <CryTypeInfo.h>
#include <IEntitySystem.h>

#include "IResourceSelectorHost.h"
#include "GenericSelectItemDialog.h"

#define CURVE_TYPE
#include "Util/VariableTypeInfo.h"

#include "ParticleParams_TypeInfo.h"
#include "Controls/PreviewModelCtrl.h"

#include "../Plugins/EditorUI_QT/Toolbar.h"

#include <QDebug>
#include <QKeyEvent>
#include <QSettings>

#define EDITOR_OBJECTS_PATH QString("Editor/Objects/")

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
class CParticlePickCallback
    : public IPickObjectCallback
{
public:
    CParticlePickCallback() { m_bActive = true; };
    //! Called when object picked.
    virtual void OnPick(CBaseObject* picked)
    {
        /*
        m_bActive = false;
        CParticleItem *pParticles = picked->GetParticle();
        if (pParticles)
            GetIEditor()->OpenParticleLibrary( pParticles );
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
bool CParticlePickCallback::m_bActive = false;
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// CParticleDialog implementation.
//////////////////////////////////////////////////////////////////////////
CParticleDialog::CParticleDialog(QWidget* pParent)
    : CBaseLibraryDialog(pParent)
    , m_ui(new Ui::ParticleDialog)
    , m_treeModel(new DefaultBaseLibraryDialogModel(this))
    , m_proxyModel(new BaseLibraryDialogProxyModel(this, m_treeModel, m_treeModel))
{
    s_poCurrentInstance = this;

    m_ui->setupUi(this);
    m_treeCtrl = m_ui->m_treeCtrl;
    m_treeCtrl->setModel(m_proxyModel);

    m_pPartManager = static_cast<CEditorParticleManager*>(GetIEditor()->GetParticleManager());
    m_pItemManager = m_pPartManager;

    m_sortRecursionType = SORT_RECURSION_ITEM;

    m_iRefreshDelay = -1;

    m_bRealtimePreviewUpdate = true;
    m_bForceReloadPropsCtrl = true;
    m_bAutoAssignToSelection = false;

    pParticleUI = new CParticleUIDefinition();

    // CONFETTI START: Leroy Sikkes
    // Initialize ignore event variable.
    m_bBlockParticleUpdate = false;
    // CONFETTI END

    OnInitDialog();

    GetTreeCtrl()->installEventFilter(this);
    GetTreeCtrl()->viewport()->installEventFilter(this);

    connect(m_ui->m_treeCtrl, &BaseLibraryDialogTree::droppedOnViewport, this, &CParticleDialog::droppedOnViewport);
}

CParticleDialog::~CParticleDialog()
{
    delete pParticleUI;
    OnDestroy();
}

CParticleDialog* CParticleDialog::s_poCurrentInstance = NULL;

//////////////////////////////////////////////////////////////////////////
void CParticleDialog::OnDestroy()
{
    QSettings settings("Amazon", "Lumberyard");
    settings.setValue(QLatin1String("Dialogs/Particles/HSplitter"), m_ui->m_wndHSplitter->sizes().value(0));
    settings.setValue(QLatin1String("Dialogs/Particles/VSplitter"), m_ui->m_wndVSplitter->sizes().value(0));

    s_poCurrentInstance = NULL;
    //ReleaseGeometry();
}

void CParticleDialog::showEvent(QShowEvent* event)
{
    QSettings settings("Amazon", "Lumberyard");
    const int HSplitter = settings.value(QLatin1String("Dialogs/Particles/HSplitter"), 200).toInt();
    const int VSplitter = settings.value(QLatin1String("Dialogs/Particles/VSplitter"), 200).toInt();
    m_ui->m_wndHSplitter->setSizes({ HSplitter, m_ui->m_wndHSplitter->width() - HSplitter });
    m_ui->m_wndVSplitter->setSizes({ VSplitter, m_ui->m_wndVSplitter->height() - VSplitter });
}

// CTVSelectKeyDialog message handlers
void CParticleDialog::OnInitDialog()
{
    InitToolbar(IDR_DB_PARTICLES_BAR);

    m_ui->m_previewCtrl->SetGrid(true);
    m_ui->m_previewCtrl->SetAxis(true, true);
    m_ui->m_previewCtrl->EnableUpdate(true);

    m_vars = pParticleUI->CreateVars();
    m_ui->m_propsCtrl->Setup();
    m_ui->m_propsCtrl->AddVarBlock(m_vars);
    m_ui->m_propsCtrl->setEnabled(false);

    ReloadLibs();

    connect(GetTreeCtrl(), &QTreeView::customContextMenuRequested, this, &CParticleDialog::OnNotifyTreeRClick);
    connect(GetTreeCtrl(), &QTreeView::clicked, this, &CParticleDialog::OnNotifyTreeLClick);
    connect(GetTreeCtrl()->selectionModel(), &QItemSelectionModel::currentChanged, this, &CParticleDialog::OnTvnSelchangedTree);
}

//////////////////////////////////////////////////////////////////////////
UINT CParticleDialog::GetDialogMenuID()
{
    return IDR_DB_ENTITY;
};

//////////////////////////////////////////////////////////////////////////
// Create the toolbar
void CParticleDialog::InitToolbar(UINT nToolbarResID)
{
    InitLibraryToolbar();

    connect(m_pItemToolBar->getAction("actionItemAdd"), &QAction::triggered, this, &CParticleDialog::OnAddItem);
    connect(m_pItemToolBar->getAction("actionItemAssign"), &QAction::triggered, this, &CParticleDialog::OnAssignToSelection);
    connect(m_pItemToolBar->getAction("actionItemGetProperties"), &QAction::triggered, this, &CParticleDialog::OnGetFromSelection);
    connect(m_pItemToolBar->getAction("actionItemReload"), &QAction::triggered, this, &CParticleDialog::OnReloadParticles);

    IToolbar* pLibToolBar = IToolbar::createToolBar(this, nToolbarResID);
    connect(pLibToolBar->getAction("actionParticleSpecialReset"), &QAction::triggered, this, &CParticleDialog::OnResetParticles);
    connect(pLibToolBar->getAction("actionParticleSpecialActivate"), &QAction::triggered, this, &CParticleDialog::OnEnable);
}

//////////////////////////////////////////////////////////////////////////
void CParticleDialog::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    if (event == eNotify_OnSelectionChange)
    {
        m_bAutoAssignToSelection = false;
    }
    // CONFETTI BEGIN: Leroy Sikkes
    // Update the particle dialog when the particle that is currently being edited has been changed.
    else if (event == eNotify_OnParticleUpdate)
    {
        if (m_pCurrentItem && m_pCurrentItem->IsModified())
        {
            m_iRefreshDelay = 10;
        }
    }
    else if (event == eNotify_OnIdleUpdate)
    {
        // Delay the refresh so that it does not make rapid particle updates feel sluggish.
        if (m_iRefreshDelay >= 0)
        {
            m_iRefreshDelay--;
            if (m_iRefreshDelay == 0)
            {
                // Reload variables & redraw window.
                m_bBlockParticleUpdate = true;
                pParticleUI->SetFromParticles(GetSelectedParticle());
                m_ui->m_propsCtrl->ReloadItems();

                m_bBlockParticleUpdate = false;
            }
        }
    }
    else if (event == eNotify_OnDataBaseUpdate)
    {
        ReloadItems();
        ReloadLibs();
    }
    // CONFETTI END

    CBaseLibraryDialog::OnEditorNotifyEvent(event);
}

//////////////////////////////////////////////////////////////////////////
QTreeView* CParticleDialog::GetTreeCtrl()
{
    return m_treeCtrl;
}

//////////////////////////////////////////////////////////////////////////
AbstractGroupProxyModel* CParticleDialog::GetProxyModel()
{
    return m_proxyModel;
}

//////////////////////////////////////////////////////////////////////////
QAbstractItemModel* CParticleDialog::GetSourceModel()
{
    return m_treeModel;
}

//////////////////////////////////////////////////////////////////////////
BaseLibraryDialogModel* CParticleDialog::GetSourceDialogModel()
{
    return m_treeModel;
}

//////////////////////////////////////////////////////////////////////////
bool CParticleDialog::AddItem(bool bFromParent)
{
    if (!m_pLibrary)
    {
        return false;
    }

    StringGroupDlg dlg(tr("New particle effect name"), this);

    if (bFromParent && m_pCurrentItem)
    {
        dlg.SetGroup(m_pCurrentItem->GetGroupName());
        dlg.SetString(m_pCurrentItem->GetShortName());
    }
    else
    {
        dlg.SetGroup(m_pCurrentItem ? (bFromParent ? m_pCurrentItem->GetGroupName() : m_pCurrentItem->GetName()) : m_selectedGroup);
    }

    if (dlg.exec() != QDialog::Accepted || dlg.GetString().isEmpty())
    {
        return false;
    }

    const QString fullName = m_pItemManager->MakeFullItemName(m_pLibrary, dlg.GetGroup(), dlg.GetString());
    if (m_pItemManager->FindItemByName(fullName))
    {
        QMessageBox::warning(this, QString(), tr("Item with name %1 already exists").arg(fullName));
        return false;
    }

    CUndo undo("Add particle library item");
    CParticleItem* pParticles = (CParticleItem*)m_pItemManager->CreateItem(m_pLibrary);
    if (!dlg.GetGroup().isEmpty())
    {
        const QString parentName = m_pLibrary->GetName() + QStringLiteral(".") + dlg.GetGroup();
        if (CParticleItem* pParent = (CParticleItem*)m_pPartManager->FindItemByName(parentName))
        {
            pParticles->SetParent(pParent);
        }
    }
    SetItemName(pParticles, dlg.GetGroup(), dlg.GetString());
    pParticles->Update();

    ReloadItems();
    SelectItem(pParticles);

    // CONFETTI START: Leroy Sikkes
    // Notify others that database changed.
    NotifyExceptSelf(eNotify_OnDataBaseUpdate);
    // CONFETTI END
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CParticleDialog::SelectItem(CBaseLibraryItem* item, bool bForceReload)
{
    bool bChanged = item != m_pCurrentItem || bForceReload;
    CBaseLibraryDialog::SelectItem(item, bForceReload);

    if (!item)
    {
        m_ui->m_propsCtrl->setEnabled(false);
        return;
    }

    if (!bChanged)
    {
        return;
    }

    m_ui->m_propsCtrl->setEnabled(true);

    m_ui->m_propsCtrl->EnableUpdateCallback(false);

    // Update variables.
    m_ui->m_propsCtrl->EnableUpdateCallback(false);

    // Reset the UI state only if a new particle system is selected
    if (!bForceReload)
    {
        pParticleUI->ResetUIState();
    }
    pParticleUI->SetFromParticles(GetSelectedParticle());
    m_ui->m_propsCtrl->EnableUpdateCallback(true);

    m_ui->m_propsCtrl->SetUpdateCallback(functor(*this, &CParticleDialog::OnUpdateProperties));
    m_ui->m_propsCtrl->EnableUpdateCallback(true);

    CParticleItem* pParticles = GetSelectedParticle();
    if (pParticles)
    {
        m_ui->m_previewCtrl->LoadParticleEffect(pParticles->GetEffect());
    }

    if (m_bForceReloadPropsCtrl)
    {
        m_ui->m_propsCtrl->ReloadItems();
        m_bForceReloadPropsCtrl = false;
    }

    if (m_bAutoAssignToSelection)
    {
        AssignToSelection();
    }
}

//////////////////////////////////////////////////////////////////////////
void CParticleDialog::Update()
{
}

//////////////////////////////////////////////////////////////////////////
void CParticleDialog::OnUpdateProperties(IVariable* var)
{
    CParticleItem* pParticles = GetSelectedParticle();
    if (!pParticles)
    {
        return;
    }

    if (!var || pParticleUI->SetToParticles(pParticles))
    {
        SelectItem(pParticles, true);
    }

    // Update visual cues of item and parents.
    CParticleItem* poEditedParticleItem(pParticles);
    for (; pParticles; pParticles = pParticles->GetParent())
    {
        pParticles->SetModified();
        const QModelIndex index = qobject_cast<BaseLibraryDialogProxyModel*>(GetTreeCtrl()->model())->index(pParticles);
        emit GetTreeCtrl()->dataChanged(index, index);
        poEditedParticleItem = pParticles;
    }

    // CONFETTI START: Leroy Sikkes
    // Notify other particle editors that a particle has been modified
    if (m_bBlockParticleUpdate == false)
    {
        NotifyExceptSelf(eNotify_OnParticleUpdate);
    }
    // CONFETTI END

    GetIEditor()->SetModifiedFlag();
}

//////////////////////////////////////////////////////////////////////////
CParticleItem* CParticleDialog::GetSelectedParticle()
{
    CBaseLibraryItem* pItem = m_pCurrentItem;
    return (CParticleItem*)pItem;
}

//////////////////////////////////////////////////////////////////////////
void CParticleDialog::OnAssignToSelection()
{
    m_bAutoAssignToSelection = !m_bAutoAssignToSelection;
    if (m_bAutoAssignToSelection)
    {
        AssignToSelection();
    }
    OnUpdateActions();
}

void CParticleDialog::AssignToSelection()
{
    CParticleItem* pParticles = GetSelectedParticle();
    if (!pParticles)
    {
        return;
    }

    CUndo undo("Assign ParticleEffect");

    CSelectionGroup* pSel = GetIEditor()->GetSelection();
    if (!pSel->IsEmpty())
    {
        for (int i = 0; i < pSel->GetCount(); i++)
        {
            AssignToEntity(pParticles, pSel->GetObject(i));
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CParticleDialog::Enable(bool bAll)
{
    CParticleItem* pParticles = GetSelectedParticle();
    if (!pParticles)
    {
        return;
    }

    CUndo undo("Enable/Disable Effect");
    if (bAll)
    {
        pParticles->DebugEnable(!pParticles->GetEnabledState());
    }
    else
    {
        pParticles->GetEffect()->SetEnabled(!(pParticles->GetEnabledState() & 1));
    }

    // Update variables.
    m_ui->m_propsCtrl->EnableUpdateCallback(false);
    pParticleUI->SetFromParticles(pParticles);
    m_ui->m_propsCtrl->EnableUpdateCallback(true);

    m_ui->m_propsCtrl->SetUpdateCallback(functor(*this, &CParticleDialog::OnUpdateProperties));
    m_ui->m_propsCtrl->EnableUpdateCallback(true);

    const QModelIndex index = qobject_cast<BaseLibraryDialogProxyModel*>(GetTreeCtrl()->model())->index(pParticles);
    emit GetTreeCtrl()->dataChanged(index, index);

    pParticles->Update();
}

//////////////////////////////////////////////////////////////////////////
void CParticleDialog::OnSelectAssignedObjects()
{
    //@FIXME
    /*
    CParticleItem *pParticles = GetSelectedParticle();
    if (!pParticles)
        return;

    CBaseObjectsArray objects;
    GetIEditor()->GetObjectManager()->GetObjects( objects );
    for (int i = 0; i < objects.size(); i++)
    {
        CBaseObject *pObject = objects[i];
        if (pObject->GetParticle() != pParticles)
            continue;
        if (pObject->IsHidden() || pObject->IsFrozen())
            continue;
        GetIEditor()->GetObjectManager()->SelectObject( pObject );
    }
    */
}

//////////////////////////////////////////////////////////////////////////
void CParticleDialog::OnReloadParticles()
{
    CUndo undo("Reload Particle");

    CParticleItem* pParticles = GetSelectedParticle();
    if (!pParticles)
    {
        return;
    }
    IParticleEffect* pEffect = pParticles->GetEffect();
    if (!pEffect)
    {
        return;
    }

    pEffect->Reload(false);
    pParticleUI->SetFromParticles(pParticles);

    // Update particles.
    pParticles->Update();

    SelectItem(pParticles, true);
    OnUpdateProperties(NULL);

    // CONFETTI START: Leroy Sikkes
    // Notify others that database changed.
    NotifyExceptSelf(eNotify_OnDataBaseUpdate);
    // CONFETTI END
}

void CParticleDialog::OnResetParticles()
{
    CUndo undo("Reset Particle");

    CParticleItem* pParticles = GetSelectedParticle();
    if (!pParticles)
    {
        return;
    }

    pParticleUI->ResetParticles(pParticles);

    SelectItem(pParticles, true);
    OnUpdateProperties(NULL);

    // CONFETTI START: Leroy Sikkes
    // Notify others that database changed.
    NotifyExceptSelf(eNotify_OnDataBaseUpdate);
    // CONFETTI END
}

CEntityObject* CParticleDialog::GetItemFromEntity()
{
    CSelectionGroup* pSel = GetIEditor()->GetSelection();
    for (int i = 0; i < pSel->GetCount(); i++)
    {
        CBaseObject* pObject = pSel->GetObject(i);
        if (qobject_cast<CEntityObject*>(pObject))
        {
            CEntityObject* pEntity = (CEntityObject*)pObject;
            if (pEntity->GetProperties())
            {
                IVariable* pVar = pEntity->GetProperties()->FindVariable("ParticleEffect");
                if (pVar)
                {
                    QString effect;
                    pVar->Get(effect);
                    {
                        CBaseLibraryItem* pItem = (CBaseLibraryItem*)m_pItemManager->LoadItemByName(effect);
                        if (pItem)
                        {
                            SelectItem(pItem);
                            return pEntity;
                        }
                    }
                }
            }
        }
    }
    return NULL;
}

const char * CParticleDialog::GetLibraryAssetTypeName() const
{
    return "Particles";
}

void CParticleDialog::OnGetFromSelection()
{
    GetItemFromEntity();
    m_bAutoAssignToSelection = false;
}

void CParticleDialog::OnExpandAll()
{
    ExpandCollapseAll(true);
}

void CParticleDialog::OnCollapseAll()
{
    ExpandCollapseAll(false);
}

bool CParticleDialog::eventFilter(QObject* watched, QEvent* event)
{
    if ((watched == GetTreeCtrl() || watched == GetTreeCtrl()->viewport()) && event->type() == QEvent::KeyPress)
    {
        keyPressEvent(static_cast<QKeyEvent*>(event));
        return event->isAccepted();
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CParticleDialog::AssignToEntity(CParticleItem* pItem, CBaseObject* pObject, Vec3 const* pPos)
{
    // Assign ParticleEffect field if it has one.
    // Otherwise, spawn/attach an emitter to the entity
    assert(pItem);
    assert(pObject);
    if (qobject_cast<CEntityObject*>(pObject))
    {
        CEntityObject* pEntity = (CEntityObject*)pObject;
        IVariable* pVar = 0;
        if (pEntity->GetProperties())
        {
            pVar = pEntity->GetProperties()->FindVariable("ParticleEffect");
            if (pVar && pVar->GetType() == IVariable::STRING)
            {
                // Set selected entity's ParticleEffect field.
                QString sEffect;
                pVar->Get(sEffect);
                pVar->Set(pItem->GetFullName());
                if (sEffect != "" && sEffect != pItem->GetFullName())
                {
                    if (CEntityScript* pScript = pEntity->GetScript())
                    {
                        pScript->SendEvent(pEntity->GetIEntity(), "Restart");
                    }
                }
            }
            else
            {
                // Create a new ParticleEffect entity on top of selected entity, attach to it.
                Vec3 pos;
                if (pPos)
                {
                    pos = *pPos;
                }
                else
                {
                    pos = pObject->GetPos();
                    AABB box;
                    pObject->GetLocalBounds(box);
                    pos.z += box.max.z;
                }

                CreateParticleEntity(pItem, pos, pObject);
            }
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
CBaseObject* CParticleDialog::CreateParticleEntity(CParticleItem* pItem, Vec3 const& pos, CBaseObject* pParent)
{
    if (!GetIEditor()->GetDocument()->IsDocumentReady())
    {
        return nullptr;
    }

    GetIEditor()->ClearSelection();
    CBaseObject* pObject = GetIEditor()->NewObject("ParticleEntity", "ParticleEffect");
    if (pObject)
    {
        // Set pos, offset by object size.
        AABB box;
        pObject->GetLocalBounds(box);
        pObject->SetPos(pos - Vec3(0, 0, box.min.z));
        pObject->SetRotation(Quat::CreateRotationXYZ(Ang3(0, 0, 0)));

        if (pParent)
        {
            pParent->AttachChild(pObject);
        }
        AssignToEntity(pItem, pObject);
        GetIEditor()->SelectObject(pObject);
    }
    return pObject;
}

//////////////////////////////////////////////////////////////////////////
void CParticleDialog::OnUpdateAssignToSelection()
{
    m_pItemToolBar->getAction("actionItemAssign")->setEnabled(GetSelectedParticle() && !GetIEditor()->GetSelection()->IsEmpty());
    m_pItemToolBar->getAction("actionItemAssign")->setCheckable(true);
    m_pItemToolBar->getAction("actionItemAssign")->setChecked(m_bAutoAssignToSelection);
}

//////////////////////////////////////////////////////////////////////////
void CParticleDialog::OnUpdateObjectSelected()
{
    m_pItemToolBar->getAction("actionItemGetProperties")->setEnabled(!GetIEditor()->GetSelection()->IsEmpty());
}

void CParticleDialog::keyPressEvent(QKeyEvent* event)
{
    if (event->modifiers() & Qt::ControlModifier)
    {
        if (event->key() == Qt::Key_Space)
        {
            Enable(event->modifiers() & Qt::ShiftModifier);
            event->accept();
            return;
        }
    }
    CBaseLibraryDialog::keyPressEvent(event);
}

//////////////////////////////////////////////////////////////////////////
void CParticleDialog::dragEnterEvent(QDragEnterEvent* event)
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

void CParticleDialog::dragMoveEvent(QDragMoveEvent* event)
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
void CParticleDialog::dropEvent(QDropEvent* event)
{
    // Dropped inside tree.
    DropToItem(GetTreeCtrl()->indexAt(event->pos()), GetTreeCtrl()->currentIndex(), static_cast<CParticleItem*>(GetTreeCtrl()->currentIndex().data(BaseLibraryItemRole).value<CBaseLibraryItem*>()));
    event->accept();
}

void CParticleDialog::droppedOnViewport(const QModelIndexList& indexes, CViewport* viewport)
{
    // Not dropped inside tree.
    CUndo undo("Assign ParticleEffect");

    QPoint vp = QCursor::pos();
    viewport->ScreenToClient(vp);
    CParticleItem* pParticles = static_cast<CParticleItem*>(indexes.first().data(BaseLibraryItemRole).value<CBaseLibraryItem*>());

    // Drag and drop into one of views.
    HitContext  hit;
    if (viewport->HitTest(vp, hit))
    {
        Vec3 hitpos = hit.raySrc + hit.rayDir * hit.dist;
        if (hit.object && AssignToEntity(pParticles, hit.object, &hitpos))
        {
            ; // done
        }
        else
        {
            // Place emitter at hit location.
            hitpos = viewport->SnapToGrid(hitpos);
            CreateParticleEntity(pParticles, hitpos);
        }
    }
    else
    {
        // Snap to terrain.
        bool hitTerrain;
        Vec3 pos = viewport->ViewToWorld(vp, &hitTerrain);
        if (hitTerrain)
        {
            pos.z = GetIEditor()->GetTerrainElevation(pos.x, pos.y);
        }
        pos = viewport->SnapToGrid(pos);
        CreateParticleEntity(pParticles, pos);
    }
}

//////////////////////////////////////////////////////////////////////////
void CParticleDialog::OnNotifyTreeRClick(const QPoint& point)
{
    CParticleItem* pParticles = 0;

    // Find node under mouse.
    // Select the item that is at the point myPoint.
    const QModelIndex hItem = GetTreeCtrl()->indexAt(point);
    pParticles = static_cast<CParticleItem*>(hItem.data(BaseLibraryItemRole).value<CBaseLibraryItem*>());

    if (!pParticles)
    {
        QMenu expandMenu;

        connect(expandMenu.addAction(tr("Expand all")), &QAction::triggered, this, &CParticleDialog::OnExpandAll);
        connect(expandMenu.addAction(tr("Collapse all")), &QAction::triggered, this, &CParticleDialog::OnCollapseAll);

        expandMenu.exec(GetTreeCtrl()->mapToGlobal(point));
        return;
    }

    SelectItem(pParticles);

    // Create pop up menu.
    QMenu menu;

    if (pParticles)
    {
        CClipboard clipboard(this);
        bool bNoPaste = clipboard.IsEmpty();

        connect(menu.addAction(tr("Cut")), &QAction::triggered, this, &CParticleDialog::OnCut);
        connect(menu.addAction(tr("Copy")), &QAction::triggered, this, &CParticleDialog::OnCopy);
        QAction* paste = menu.addAction(tr("Paste"));
        paste->setEnabled(!bNoPaste);
        connect(paste, &QAction::triggered, this, &CParticleDialog::OnPaste);
        connect(menu.addAction(tr("Clone")), &QAction::triggered, this, &CParticleDialog::OnClone);
        connect(menu.addAction(tr("Copy Path")), &QAction::triggered, this, &CParticleDialog::OnCopyPath);
        menu.addSeparator();
        connect(menu.addAction(tr("Rename")), &QAction::triggered, this, &CParticleDialog::OnRenameItem);
        connect(menu.addAction(tr("Delete")), &QAction::triggered, this, &CParticleDialog::OnRemoveItem);
        connect(menu.addAction(tr("Reset")), &QAction::triggered, this, &CParticleDialog::OnResetParticles);
        connect(menu.addAction(pParticles->GetEnabledState() & 1 ? tr("Disable \tCtrl+Space)") : tr("Enable\tCtrl+Space")), &QAction::triggered, this, &CParticleDialog::OnEnable);
        connect(menu.addAction(pParticles->GetEnabledState() & 1 ? tr("Disable All\tCtrl+Shift+Space)") : tr("Enable All\tCtrl+Shift+Space")), &QAction::triggered, this, &CParticleDialog::OnEnableAll);
        menu.addSeparator();
        connect(menu.addAction(tr("Assign to Selected Objects")), &QAction::triggered, this, &CParticleDialog::OnAssignToSelection);
    }

    menu.exec(GetTreeCtrl()->mapToGlobal(point));
}

//////////////////////////////////////////////////////////////////////////
void CParticleDialog::OnNotifyTreeLClick(const QModelIndex& item)
{
    //Essentially re-start playback of the selected particle system

    CParticleItem* pParticles = static_cast<CParticleItem*>(item.data(BaseLibraryItemRole).value<CBaseLibraryItem*>());

    if (pParticles)
    {
        SelectItem(pParticles, true); //force re-start
    }
}

//////////////////////////////////////////////////////////////////////////
void CParticleDialog::OnTvnSelchangedTree(const QModelIndex& current, const QModelIndex& previous)
{
    OnSelChangedItemTree(current, previous);
}

//////////////////////////////////////////////////////////////////////////
void CParticleDialog::OnPick()
{
    if (!CParticlePickCallback::IsActive())
    {
        GetIEditor()->PickObject(new CParticlePickCallback, 0, "Pick Object to Select Particle");
    }
    else
    {
        GetIEditor()->CancelPick();
    }
}

//////////////////////////////////////////////////////////////////////////
void CParticleDialog::OnUpdatePick()
{
    /* currently no pick action
    if (CParticlePickCallback::IsActive())
    {
        pCmdUI->SetCheck(1);
    }
    else
    {
        pCmdUI->SetCheck(0);
    }*/
}

//////////////////////////////////////////////////////////////////////////
void CParticleDialog::OnCopy()
{
    CParticleItem* pParticles = GetSelectedParticle();
    if (pParticles)
    {
        XmlNodeRef node = XmlHelpers::CreateXmlNode("Particles");
        CBaseLibraryItem::SerializeContext ctx(node, false);
        ctx.bCopyPaste = true;

        CClipboard clipboard(this);
        pParticles->Serialize(ctx);
        clipboard.Put(node);
    }
}

//////////////////////////////////////////////////////////////////////////
void CParticleDialog::OnPaste()
{
    if (!m_pLibrary)
    {
        return;
    }

    CParticleItem* pItem = GetSelectedParticle();
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

    if (strcmp(node->getTag(), "Particles") == 0)
    {
        CUndo undo("Add particle library item");
        node->delAttr("Name");

        m_pPartManager->PasteToParticleItem(pItem, node, true);
        ReloadItems();
        SelectItem(pItem);
    }

    // CONFETTI START: Leroy Sikkes
    // Notify others that database changed.
    NotifyExceptSelf(eNotify_OnDataBaseUpdate);
    // CONFETTI END
}

//////////////////////////////////////////////////////////////////////////
void CParticleDialog::OnClone()
{
    if (!m_pLibrary || !m_pCurrentItem)
    {
        return;
    }

    OnCopy();
    if (AddItem(true))
    {
        OnPaste();
    }

    // CONFETTI START: Leroy Sikkes
    // Notify others that database changed.
    NotifyExceptSelf(eNotify_OnDataBaseUpdate);
    // CONFETTI END
}

//////////////////////////////////////////////////////////////////////////
void CParticleDialog::DropToItem(const QModelIndex& item, const QModelIndex& dragged, CParticleItem* pParticles)
{
    pParticles->GetLibrary()->SetModified();

    TSmartPtr<CParticleItem> pHolder = pParticles; // Make usre its not release while we remove and add it back.

    if (!item.isValid())
    {
        // Detach from parent.
        pParticles->SetParent(NULL);

        ReloadItems();
        SelectItem(pParticles);
        // CONFETTI START: Steven Landers
        // Notify others that database changed.
        NotifyExceptSelf(eNotify_OnDataBaseUpdate);
        // CONFETTI END
        return;
    }

    CParticleItem* pTargetItem = static_cast<CParticleItem*>(item.data(BaseLibraryItemRole).value<CBaseLibraryItem*>());
    if (!pTargetItem)
    {
        // This is group.

        // Detach from parent.
        pParticles->SetParent(NULL);

        // Move item to different group.
        const QString groupName = GetItemFullName(item, ".");
        SetItemName(pParticles, groupName, pParticles->GetShortName());

        // CONFETTI START: Steven Landers
        // Notify others that database changed.
        NotifyExceptSelf(eNotify_OnDataBaseUpdate);
        // CONFETTI END
        return;
    }

    // Ignore itself.
    if (pTargetItem == pParticles)
    {
        return;
    }

    // Move to new parent.
    pParticles->SetParent(pTargetItem);

    ReloadItems();
    SelectItem(pParticles);
    // CONFETTI START: Steven Landers
    // Notify others that database changed.
    NotifyExceptSelf(eNotify_OnDataBaseUpdate);
    // CONFETTI END
}

void CParticleDialog::ReleasePreviewControl()
{
    m_ui->m_previewCtrl->ReleaseObject();
    m_ui->m_previewCtrl->Update(true);
}

void CParticleDialog::OnUpdateActions()
{
    CBaseLibraryDialog::OnUpdateActions();

    OnUpdateAssignToSelection();
    OnUpdateObjectSelected();
    OnUpdatePick();
}

//////////////////////////////////////////////////////////////////////////
void CParticleDialog::ExpandCollapseAll(bool expand)
{
    if (expand)
    {
        m_ui->m_treeCtrl->expandAll();
    }
    else
    {
        m_ui->m_treeCtrl->collapseAll();
    }
}

//////////////////////////////////////////////////////////////////////////
CParticleDialog* CParticleDialog::GetCurrentInstance()
{
    return s_poCurrentInstance;
}

//////////////////////////////////////////////////////////////////////////
QString ParticleResourceSelector(const SResourceSelectorContext& x, const QString& previousValue)
{
    QStringList items;
    const char* openLibraryText = "[ Open Particle Database ]";
    items.push_back(openLibraryText);
    {
        CEditorParticleManager* pParticleManager = static_cast<CEditorParticleManager*>(GetIEditor()->GetParticleManager());

        IDataBaseItemEnumerator* pEnumerator = pParticleManager->GetItemEnumerator();
        for (IDataBaseItem* pItem = pEnumerator->GetFirst(); pItem; pItem = pEnumerator->GetNext())
        {
            items.push_back(pItem->GetFullName());
        }
        pEnumerator->Release();
    }

    CGenericSelectItemDialog dialog;
    dialog.SetMode(CGenericSelectItemDialog::eMODE_TREE);
    dialog.SetItems(items);
    dialog.SetTreeSeparator(".");

    dialog.PreSelectItem(previousValue);
    if (dialog.exec() == QDialog::Accepted)
    {
        if (dialog.GetSelectedItem() == openLibraryText)
        {
            GetIEditor()->OpenDataBaseLibrary(EDB_TYPE_PARTICLE);
        }
        else
        {
            return dialog.GetSelectedItem().toUtf8().constData();
        }
    }
    return previousValue;
}
REGISTER_RESOURCE_SELECTOR("Particle", ParticleResourceSelector, "Editor/UI/Icons/cloud.png")

#include <Particles/ParticleDialog.moc>
