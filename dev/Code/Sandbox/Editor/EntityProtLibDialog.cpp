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
#include "EntityProtLibDialog.h"

#include "Controls/PreviewModelCtrl.h"
#include "Objects/EntityScript.h"
#include "Objects/ProtEntityObject.h"
#include "StringDlg.h"

#include "EntityPrototypeManager.h"
#include "EntityPrototypeLibrary.h"
#include "EntityPrototype.h"
#include "SelectEntityClsDialog.h"
#include "Clipboard.h"
#include "ViewManager.h"

#include <IEntitySystem.h>

#include "Material/MaterialManager.h"

#include "../Plugins/EditorUI_QT/Toolbar.h"

#include <Controls/ReflectedPropertyControl/ReflectedPropertyCtrl.h>

#include <QLabel>
#include <QMenu>
#include <QSplitter>
#include <QVBoxLayout>

#include <QDebug>



#if (_WIN32_WINNT < 0x0600)

#define TVM_SETEXTENDEDSTYLE      (TV_FIRST + 44)
#define TreeView_SetExtendedStyle(hwnd, dw, mask) \
    (DWORD)SNDMSG((hwnd), TVM_SETEXTENDEDSTYLE, mask, dw)

#define TVM_GETEXTENDEDSTYLE      (TV_FIRST + 45)
#define TreeView_GetExtendedStyle(hwnd) \
    (DWORD)SNDMSG((hwnd), TVM_GETEXTENDEDSTYLE, 0, 0)

#define TVS_EX_MULTISELECT          0x0002
#define TVS_EX_DOUBLEBUFFER         0x0004
#define TVS_EX_NOINDENTSTATE        0x0008
#define TVS_EX_RICHTOOLTIP          0x0010
#define TVS_EX_AUTOHSCROLL          0x0020
#define TVS_EX_FADEINOUTEXPANDOS    0x0040
#define TVS_EX_PARTIALCHECKBOXES    0x0080
#define TVS_EX_EXCLUSIONCHECKBOXES  0x0100
#define TVS_EX_DIMMEDCHECKBOXES     0x0200
#define TVS_EX_DRAWIMAGEASYNC       0x0400

#endif



//////////////////////////////////////////////////////////////////////////
// CEntityProtLibDialog implementation.
//////////////////////////////////////////////////////////////////////////
CEntityProtLibDialog::CEntityProtLibDialog(QWidget* pParent)
    : CBaseLibraryDialog(IDD_DB_ENTITY, pParent)
    , m_treeModel(new DefaultBaseLibraryDialogModel(this))
    , m_proxyModel(new BaseLibraryDialogProxyModel(this, m_treeModel, m_treeModel))
{
    m_entity = 0;
    m_pEntityManager = GetIEditor()->GetEntityProtManager();
    m_pItemManager = m_pEntityManager;
    m_pItemManager->AddListener(this);
    m_bEntityPlaying = false;
    m_bShowDescription = false;

    // Sort items using full recursion.
    m_sortRecursionType = SORT_RECURSION_FULL;

    m_previewCtrl = nullptr;

    OnInitDialog();
}

CEntityProtLibDialog::~CEntityProtLibDialog()
{
    m_pItemManager->RemoveListener(this);
    delete m_previewCtrl;
}

// CTVSelectKeyDialog message handlers
void CEntityProtLibDialog::OnInitDialog()
{
    CBaseLibraryDialog::OnInitDialog();

    QWidget* mainWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(mainWidget);
    setCentralWidget(mainWidget);

    m_pEntitySystem = GetIEditor()->GetSystem()->GetIEntitySystem();

    InitToolbar(IDR_DB_LIBRARY_ITEM_BAR);

    // Create left panel tree control.
    m_treeCtrl = new BaseLibraryDialogTree(this);
    m_treeCtrl->setModel(m_proxyModel);
    GetTreeCtrl()->setContextMenuPolicy(Qt::CustomContextMenu);
    GetTreeCtrl()->setHeaderHidden(true);
    GetTreeCtrl()->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    GetTreeCtrl()->setDragDropMode(QAbstractItemView::DragOnly);

    connect(GetTreeCtrl(), &QTreeView::customContextMenuRequested, this, &CEntityProtLibDialog::OnNotifyTreeRClick);
    connect(GetTreeCtrl()->selectionModel(), &QItemSelectionModel::currentChanged, this, &CEntityProtLibDialog::OnSelChangedItemTree);
    connect(qobject_cast<BaseLibraryDialogTree*>(GetTreeCtrl()), &BaseLibraryDialogTree::droppedOnViewport, this, &CEntityProtLibDialog::droppedOnViewport);

    // Create context menu and shortcut keys for tree control
    m_treeContextMenu = new QMenu(GetTreeCtrl());

    connect(m_treeContextMenu, &QMenu::aboutToShow, this, &CEntityProtLibDialog::OnUpdateActions);

    m_cutAction = m_treeContextMenu->addAction(tr("Cut"));
    m_cutAction->setShortcut(QKeySequence::Cut);
    connect(m_cutAction, &QAction::triggered, this, &CEntityProtLibDialog::OnCut);

    // Copy and paste are already connected when creating the toolbars
    m_copyAction->setShortcut(QKeySequence::Copy);
    m_treeContextMenu->addAction(m_copyAction);

    m_pasteAction->setShortcut(QKeySequence::Paste);
    m_treeContextMenu->addAction(m_pasteAction);

    m_cloneAction = m_treeContextMenu->addAction(tr("Clone"));
    m_cloneAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_D));
    connect(m_cloneAction, &QAction::triggered, this, &CEntityProtLibDialog::OnClone);

    QAction* copyPath = m_treeContextMenu->addAction(tr("Copy Path"));
    connect(copyPath, &QAction::triggered, this, &CEntityProtLibDialog::OnCopyPath);

    m_treeContextMenu->addSeparator();

    QAction* rename = m_treeContextMenu->addAction(tr("Rename"));
    connect(rename, &QAction::triggered, this, &CEntityProtLibDialog::OnRenameItem);

    QAction* deleteAction = m_treeContextMenu->addAction(tr("Delete"));
    deleteAction->setShortcut(QKeySequence(Qt::Key_Delete));
    connect(deleteAction, &QAction::triggered, this, &CEntityProtLibDialog::OnRemoveItem);

    m_treeContextMenu->addSeparator();

    m_assignAction = m_treeContextMenu->addAction(tr("Assign to Selected Objects"));
    connect(m_assignAction, &QAction::triggered, this, &CEntityProtLibDialog::OnAssignToSelection);

    m_selectAction = m_treeContextMenu->addAction(tr("Select Assigned Objects"));
    connect(m_selectAction, &QAction::triggered, this, &CEntityProtLibDialog::OnSelectAssignedObjects);

    QAction* reload = m_treeContextMenu->addAction(tr("Reload"));
    connect(reload, &QAction::triggered, this, &CEntityProtLibDialog::OnReloadEntityScript);

    QAction* change = m_treeContextMenu->addAction(tr("Change Entity Class"));
    connect(change, &QAction::triggered, this, &CEntityProtLibDialog::OnChangeEntityClass);

    QAction* insert = new QAction(GetTreeCtrl());
    insert->setShortcut(Qt::Key_Insert);
    connect(insert, &QAction::triggered, this, &CEntityProtLibDialog::OnAddItem);

    GetTreeCtrl()->addActions(m_treeContextMenu->actions());
    GetTreeCtrl()->addAction(insert);

    //int h2 = rc.Height()/2;
    int h2 = 30;

    m_wndVSplitter = new QSplitter(Qt::Horizontal, this);
    m_wndHSplitter = new QWidget(this);
    m_wndScriptPreviewSplitter = new QSplitter(Qt::Horizontal, this);
    m_wndSplitter4 = new QSplitter(Qt::Vertical, this);

    // Attach it to the control
    m_wndVSplitter->addWidget(GetTreeCtrl());

    m_previewCtrl = new CPreviewModelCtrl(m_wndScriptPreviewSplitter);

    //m_previewCtrl.Create( WS_VISIBLE|WS_CHILD,rc,&m_wndHSplitter,1 );
    m_propsCtrl = new ReflectedPropertyControl(this);
    m_propsCtrl->Setup();
    m_objectPropsCtrl = new ReflectedPropertyControl(this);
    m_objectPropsCtrl->Setup();

    //m_descriptionEditBox.Create( ES_MULTILINE|ES_WANTRETURN|WS_CHILD|WS_TABSTOP,rc,this,IDC_DESCRIPTION );
    //m_descriptionEditBox.ShowWindow( SW_HIDE );

    m_wndCaptionEntityClass = new QLabel(this);

    const QColor borderColor = palette().color(QPalette::Light);
    const QColor backgroundColor = palette().color(QPalette::Shadow);
    const QColor textColor = palette().color(QPalette::Window);

    const QString labelStyleSheet = QString::fromLatin1(
            "QLabel"
            "{"
            "	color: %1;"
            "	background-color: %2;"
            "	border: 4px solid %3;"
            "	font-weight: bold;"
            "}").arg(textColor.name(), backgroundColor.name(), borderColor.name());
    m_wndCaptionEntityClass->setStyleSheet(labelStyleSheet);
    m_wndCaptionEntityClass->setMinimumHeight(m_wndCaptionEntityClass->minimumSizeHint().height() + 4 * 2);

    m_wndSplitter4->addWidget(m_objectPropsCtrl);
    m_wndSplitter4->addWidget(m_previewCtrl);
    m_wndSplitter4->setSizes({ 240, 160 });

    m_wndScriptPreviewSplitter->addWidget(m_propsCtrl);
    m_wndScriptPreviewSplitter->addWidget(m_wndSplitter4);
    m_wndScriptPreviewSplitter->setSizes({ 260, 240 });

    //m_descriptionEditBox.ShowWindow( SW_SHOW );
    m_bShowDescription = false;

    //m_wndHSplitter.SetPane( 0,0,&m_descriptionEditBox,CSize(100,h2) );
    QVBoxLayout* wndHSplitterLayout = new QVBoxLayout(m_wndHSplitter);
    wndHSplitterLayout->addWidget(m_wndCaptionEntityClass);
    m_wndScriptPreviewSplitter->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    wndHSplitterLayout->addWidget(m_wndScriptPreviewSplitter);

    m_wndVSplitter->addWidget(GetTreeCtrl());
    m_wndVSplitter->addWidget(m_wndHSplitter);
    m_wndVSplitter->setSizes({ 80, 320 });
    //m_wndVSplitter.CreateView( 1,1,

    mainLayout->addWidget(m_wndVSplitter);

    ReloadLibs();
    ReloadItems();
}

//////////////////////////////////////////////////////////////////////////
UINT CEntityProtLibDialog::GetDialogMenuID()
{
    return IDR_DB_ENTITY;
};

//////////////////////////////////////////////////////////////////////////
// Create the toolbar
void CEntityProtLibDialog::InitToolbar(UINT nToolbarResID)
{
    InitLibraryToolbar();

    connect(m_pItemToolBar->getAction("actionItemAdd"), &QAction::triggered, this, &CEntityProtLibDialog::OnAddPrototype);
    connect(m_pItemToolBar->getAction("actionItemAssign"), &QAction::triggered, this, &CEntityProtLibDialog::OnAssignToSelection);
    connect(m_pItemToolBar->getAction("actionItemGetProperties"), &QAction::triggered, this, &CEntityProtLibDialog::OnShowDescription);
    connect(m_pItemToolBar->getAction("actionItemReload"), &QAction::triggered, this, &CEntityProtLibDialog::OnReloadEntityScript);

    m_pItemToolBar->getAction("actionItemGetProperties")->setEnabled(false);

    InitItemToolbar();
}

//////////////////////////////////////////////////////////////////////////
void CEntityProtLibDialog::OnAddPrototype()
{
    if (!m_pLibrary)
    {
        return;
    }
    QString library = m_selectedLib;
    if (library.isEmpty())
    {
        return;
    }
    QString entityClass = SelectEntityClass();
    if (entityClass.isEmpty())
    {
        return;
    }

    StringGroupDlg dlg(tr("New Entity Name"), this);
    dlg.SetGroup(m_selectedGroup);
    dlg.SetString(entityClass);
    if (dlg.exec() != QDialog::Accepted || dlg.GetString().isEmpty())
    {
        return;
    }

    CUndo undo("Add entity prototype library item");
    CEntityPrototype* prototype = (CEntityPrototype*)m_pEntityManager->CreateItem(m_pLibrary);

    // Make prototype name.
    SetItemName(prototype, dlg.GetGroup(), dlg.GetString());
    // Assign entity class to prototype.
    prototype->SetEntityClassName(entityClass);

    ReloadItems();
    SelectItem(prototype);
}

//////////////////////////////////////////////////////////////////////////
QString CEntityProtLibDialog::SelectEntityClass()
{
    CSelectEntityClsDialog dlg;
    if (dlg.exec() == QDialog::Accepted)
    {
        return dlg.GetEntityClass();
    }
    return "";
}

bool CEntityProtLibDialog::MarkOrUnmarkPropertyField(IVariable* varArchetype, IVariable* varEntScript)
{
    bool bUpdated = false;

    if (varArchetype && varEntScript && varArchetype->GetName() == varEntScript->GetName())
    {
        // Determine if flags or name needs to change
        QString strArchetypeValue = varArchetype->GetDisplayValue();
        QString strEntScriptValue = varEntScript->GetDisplayValue();
        int     nOldFlags = varArchetype->GetFlags();
        int     nNewFlags = nOldFlags;
        if (strArchetypeValue == strEntScriptValue)
        {
            nNewFlags &= ~IVariable::UI_BOLD;
        }
        else
        {
            nNewFlags |= IVariable::UI_BOLD;
        }

        // Update anything that changed
        if (nOldFlags != nNewFlags)
        {
            varArchetype->SetFlags(nNewFlags);
            bUpdated = true;
        }

        // Set/Clear the script default shown in conjunction with the tooltip.
        ReflectedPropertyItem* pItem = m_propsCtrl->FindItemByVar(varArchetype);
        assert(pItem);
        if (pItem)
        {
            // Clear script default if property matches; set it otherwise.
            if (strArchetypeValue == strEntScriptValue)
            {
                pItem->ClearScriptDefault();
            }
            else
            {
                pItem->SetScriptDefault(strEntScriptValue);
            }
        }
    }

    return bUpdated;
}

void CEntityProtLibDialog::MarkFieldsThatDifferFromScriptDefaults(IVariable* varArchetype, IVariable* varEntScript)
{
    if (varArchetype && varEntScript && varArchetype->GetName() == varEntScript->GetName())
    {
        if (varArchetype->GetNumVariables() > 0  &&  varEntScript->GetNumVariables() > 0)
        {
            for (int i = 0; i < varArchetype->GetNumVariables(); i++)
            {
                IVariable*  varArchetype2 = varArchetype->GetVariable(i);
                QString     strNameArchetype2 = varArchetype2 ? varArchetype2->GetName() : "{unknown}";
                IVariable*  varEntScript2 = NULL;
                QString     strNameEntScript2 = "<unknown>";
                for (int j = 0; strNameArchetype2 != strNameEntScript2 && j < varEntScript->GetNumVariables(); j++)
                {
                    varEntScript2 = varEntScript->GetVariable(j);
                    strNameEntScript2 = varEntScript2 ? varEntScript2->GetName() : "<unknown>";
                }
                MarkFieldsThatDifferFromScriptDefaults(varArchetype2, varEntScript2);
            }
        }
        else if (varArchetype->GetNumVariables() == 0 && varEntScript->GetNumVariables() == 0)
        {
            MarkOrUnmarkPropertyField(varArchetype, varEntScript);
        }
    }
}

void CEntityProtLibDialog::MarkFieldsThatDifferFromScriptDefaults(CVarBlock* pArchetypeVarBlock, CVarBlock* pEntScriptVarBlock)
{
    if (pArchetypeVarBlock && pEntScriptVarBlock)
    {
        for (int i = 0; i < pArchetypeVarBlock->GetNumVariables(); i++)
        {
            IVariable* varArchetype = pArchetypeVarBlock->GetVariable(i);
            IVariable* varEntScript = pEntScriptVarBlock->FindVariable(varArchetype ? varArchetype->GetName().toUtf8().data() : "");
            MarkFieldsThatDifferFromScriptDefaults(varArchetype, varEntScript);
        }
    }
}

const char* CEntityProtLibDialog::GetLibraryAssetTypeName() const
{
    return "Entity Prototype Library";
}

//////////////////////////////////////////////////////////////////////////
void CEntityProtLibDialog::SelectItem(CBaseLibraryItem* item, bool bForceReload)
{
    bool bChanged = item != m_pCurrentItem || bForceReload;
    CBaseLibraryDialog::SelectItem(item, bForceReload);

    if (!bChanged)
    {
        return;
    }

    CEntityPrototype* prototype = (CEntityPrototype*)item;

    // Empty preview control.
    m_previewCtrl->SetEntity(0);
    m_previewCtrl->LoadFile("", false);

    m_wndCaptionEntityClass->clear();
    m_propsCtrl->RemoveAllItems();
    m_objectPropsCtrl->RemoveAllItems();
    if (prototype && prototype->GetProperties())
    {
        QString scriptName;
        if (prototype->GetScript())
        {
            scriptName = prototype->GetScript()->GetName();
            QString caption = scriptName + "   :   " + item->GetFullName();
            if (m_wndCaptionEntityClass)
            {
                m_wndCaptionEntityClass->setText(caption);
            }
        }
        m_propsCtrl->AddVarBlock(prototype->GetProperties(), "Class Properties");
        m_propsCtrl->ExpandAll();
        m_propsCtrl->SetUpdateCallback(functor(*this, &CEntityProtLibDialog::OnUpdateProperties));

        CVarBlock* pArchetypeVarBlock = prototype->GetProperties();
        CVarBlock* pEntScriptVarBlock = prototype->GetScript() ? prototype->GetScript()->GetDefaultProperties() : NULL;
        MarkFieldsThatDifferFromScriptDefaults(pArchetypeVarBlock, pEntScriptVarBlock);

        m_objectPropsCtrl->AddVarBlock(prototype->GetObjectVarBlock(), "Object Params");
        m_objectPropsCtrl->ExpandAll();
        m_objectPropsCtrl->SetUpdateCallback(functor(*this, &CEntityProtLibDialog::OnUpdateProperties));
    }

    if (prototype)
    {
        //m_descriptionEditBox.SetWindowText( prototype->GetDescription() );
    }

    UpdatePreview();
}

//////////////////////////////////////////////////////////////////////////
void CEntityProtLibDialog::UpdatePreview()
{
    CEntityPrototype* prototype = GetSelectedPrototype();
    if (!prototype)
    {
        return;
    }

    CEntityScript* script = prototype->GetScript();
    if (!script)
    {
        return;
    }
    // Load script if its not loaded yet.
    if (!script->IsValid())
    {
        if (!script->Load())
        {
            return;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Make visual object for this entity.
    //////////////////////////////////////////////////////////////////////////
    m_visualObject = "";
    m_PrototypeMaterial = "";

    CVarBlock* pObjectVarBlock = prototype->GetProperties();
    if (pObjectVarBlock)
    {
        IVariable* pPrototypeMaterial = pObjectVarBlock->FindVariable("PrototypeMaterial");
        if (pPrototypeMaterial)
        {
            pPrototypeMaterial->Get(m_PrototypeMaterial);
        }
    }


    CVarBlock* pProperties = prototype->GetProperties();
    if (pProperties)
    {
        IVariable* pVar = pProperties->FindVariable("object_Model", true);
        if (pVar)
        {
            pVar->Get(m_visualObject);
        }

        if (m_visualObject.isEmpty())
        {
            pVar = pProperties->FindVariable("fileModel", true);
            if (pVar)
            {
                pVar->Get(m_visualObject);
            }
        }
        if (m_visualObject.isEmpty())
        {
            pVar = pProperties->FindVariable("objModel", true);
            if (pVar)
            {
                pVar->Get(m_visualObject);
            }
        }
    }
    if (m_visualObject.isEmpty())
    {
        m_visualObject = script->GetVisualObject();
    }

    if (!m_visualObject.isEmpty() && m_visualObject != m_previewCtrl->GetLoadedFile())
    {
        m_previewCtrl->LoadFile(m_visualObject, false);
        m_previewCtrl->FitToScreen();
    }

    if (!m_PrototypeMaterial.isEmpty())
    {
        CMaterial* poPrototypeMaterial = GetIEditor()->GetMaterialManager()->LoadMaterial(m_PrototypeMaterial);
        m_previewCtrl->SetMaterial(poPrototypeMaterial);
    }

    m_previewCtrl->SetGrid(true);
}

//////////////////////////////////////////////////////////////////////////
QTreeView* CEntityProtLibDialog::GetTreeCtrl()
{
    return m_treeCtrl;
}

//////////////////////////////////////////////////////////////////////////
AbstractGroupProxyModel* CEntityProtLibDialog::GetProxyModel()
{
    return m_proxyModel;
}

//////////////////////////////////////////////////////////////////////////
QAbstractItemModel* CEntityProtLibDialog::GetSourceModel()
{
    return m_treeModel;
}

//////////////////////////////////////////////////////////////////////////
BaseLibraryDialogModel* CEntityProtLibDialog::GetSourceDialogModel()
{
    return m_treeModel;
}

//////////////////////////////////////////////////////////////////////////
void CEntityProtLibDialog::SpawnEntity(CEntityPrototype* prototype)
{
    assert(prototype);

    ReleaseEntity();

    CEntityScript* script = prototype->GetScript();
    if (!script)
    {
        return;
    }
    // Load script if its not loaded yet.
    if (!script->IsValid())
    {
        if (!script->Load())
        {
            return;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityProtLibDialog::ReleaseEntity()
{
    m_visualObject = "";
    m_PrototypeMaterial = "";
    /*
    if (m_entity)
    {
        m_entity->SetDestroyable(true);
        m_pEntitySystem->RemoveEntity( m_entity->GetId(),true );
    }
    */
    m_entity = 0;
}

//////////////////////////////////////////////////////////////////////////
void CEntityProtLibDialog::Update()
{
    if (!m_bEntityPlaying)
    {
        return;
    }

    // Update preview control.
    if (m_entity)
    {
        m_previewCtrl->Update();
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityProtLibDialog::OnUpdateProperties(IVariable* var)
{
    CEntityPrototype* prototype = GetSelectedPrototype();
    if (prototype != 0)
    {
        // Mark prototype library modified.
        prototype->GetLibrary()->SetModified();

        CEntityScript* script = prototype->GetScript();
        CVarBlock* props = prototype->GetProperties();
        if (script && props && m_entity != 0)
        {
            // Set entity properties.
            script->CopyPropertiesToScriptTable(m_entity, props, true);
        }
        prototype->Update();

        CVarBlock* pEntScriptVarBlock = script ? script->GetDefaultProperties() : NULL;
        IVariable* varEntScript = pEntScriptVarBlock ? pEntScriptVarBlock->FindVariable(var ? var->GetName().toUtf8().data() : "") : NULL;
        if (var && varEntScript && MarkOrUnmarkPropertyField(var, varEntScript))
        {
            // If flags have been changed for this field, make sure the item gets redrawn.
            ReflectedPropertyItem* pItem = m_propsCtrl->FindItemByVar(var);
            assert(pItem);
            if (pItem)
            {
                m_propsCtrl->InvalidateCtrl();
            }
        }
    }

    if (var && var->GetType() == IVariable::STRING)
    {
        // When model is updated we need to reload preview.
        UpdatePreview();
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityProtLibDialog::OnShowDescription()
{
    /*
        m_previewCtrl.SetDlgCtrlID(100);
        m_wndScriptPreviewSplitter.SetPane( 0,1,&m_descriptionEditBox,CSize(200,100) );
        m_wndScriptPreviewSplitter.RecalcLayout();
        m_descriptionEditBox.ShowWindow( SW_SHOW );
        m_previewCtrl.ShowWindow( SW_HIDE );
        m_bShowDescription = true;
        */
}

//////////////////////////////////////////////////////////////////////////
void CEntityProtLibDialog::OnPlay()
{
    m_bEntityPlaying = !m_bEntityPlaying;

    /*
    if (m_bShowDescription)
    {
        m_descriptionEditBox.SetDlgCtrlID(101);
        m_wndScriptPreviewSplitter.SetPane( 0,1,&m_previewCtrl,CSize(200,100) );
        m_descriptionEditBox.ShowWindow( SW_HIDE );
        m_previewCtrl.ShowWindow( SW_SHOW );
        m_bShowDescription = false;
        m_wndScriptPreviewSplitter.RecalcLayout();
    }
    */
}

//////////////////////////////////////////////////////////////////////////
void CEntityProtLibDialog::OnUpdatePlay()
{
    /* no play button anymore
    if (m_bEntityPlaying)
        pCmdUI->SetCheck(TRUE);
    else
        pCmdUI->SetCheck(FALSE);
        */
}

//////////////////////////////////////////////////////////////////////////
void CEntityProtLibDialog::OnReloadEntityScript()
{
    CEntityPrototype* prototype = GetSelectedPrototype();
    if (prototype)
    {
        prototype->Reload();
        TSmartPtr<CEntityPrototype> sel = prototype;
        SelectItem(prototype);
    }
}

//////////////////////////////////////////////////////////////////////////
CEntityPrototype* CEntityProtLibDialog::GetSelectedPrototype()
{
    CBaseLibraryItem* item = m_pCurrentItem;
    return (CEntityPrototype*)item;
}

//////////////////////////////////////////////////////////////////////////
void CEntityProtLibDialog::OnAssignToSelection()
{
    CEntityPrototype* pPrototype = GetSelectedPrototype();
    if (!pPrototype)
    {
        return;
    }

    CUndo undo("Assign Archetype");

    CSelectionGroup* pSel = GetIEditor()->GetSelection();
    if (!pSel->IsEmpty())
    {
        for (int i = 0; i < pSel->GetCount(); i++)
        {
            CBaseObject* pObject = pSel->GetObject(i);
            if (qobject_cast<CProtEntityObject*>(pObject))
            {
                CProtEntityObject* pProtEntity = (CProtEntityObject*)pObject;
                pProtEntity->SetPrototype(pPrototype, true);
            }
        }

        GetIEditor()->GetObjectManager()->EndEditParams();
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityProtLibDialog::OnSelectAssignedObjects()
{
    CEntityPrototype* pItem = GetSelectedPrototype();
    if (!pItem)
    {
        return;
    }

    CBaseObjectsArray objects;
    GetIEditor()->GetObjectManager()->GetObjects(objects);
    for (int i = 0; i < objects.size(); i++)
    {
        CBaseObject* pObject = objects[i];
        if (qobject_cast<CProtEntityObject*>(pObject))
        {
            CProtEntityObject* protEntity = (CProtEntityObject*)pObject;
            if (protEntity->GetPrototype() != pItem)
            {
                continue;
            }
            if (pObject->IsHidden() || pObject->IsFrozen())
            {
                continue;
            }
            GetIEditor()->GetObjectManager()->SelectObject(pObject);
        }
    }
}

#define ID_CMD_CHANGECLASS 2

//////////////////////////////////////////////////////////////////////////
void CEntityProtLibDialog::OnNotifyTreeRClick(const QPoint& point)
{
    CBaseLibraryItem* pItem = 0;

    // Find node under mouse.
    // Select the item that is at the point myPoint.
    const QModelIndex hItem = GetTreeCtrl()->indexAt(point);
    pItem = hItem.data(BaseLibraryItemRole).value<CBaseLibraryItem*>();

    if (!pItem)
    {
        return;
    }

    SelectItem(pItem);

    // Create pop up menu.
    if (!pItem)
    {
        return;
    }

    CClipboard clipboard(this);
    m_pasteAction->setEnabled(!clipboard.IsEmpty());

    m_treeContextMenu->exec(QCursor::pos());
}

//////////////////////////////////////////////////////////////////////////
void CEntityProtLibDialog::droppedOnViewport(const QModelIndexList& indexes, CViewport* viewport)
{
    bool bHit = false;
    QPoint vp = QCursor::pos();
    viewport->ScreenToClient(vp);

    CEntityPrototype* item = static_cast<CEntityPrototype*>(indexes.first().data(BaseLibraryItemRole).value<CBaseLibraryItem*>());

    // Drag and drop into one of views.
    // Start object creation.
    HitContext hit;
    if (viewport->HitTest(vp, hit))
    {
        if (hit.object)
        {
            if (qobject_cast<CProtEntityObject*>(hit.object))
            {
                bHit = true;
                CUndo undo("Assign Archetype");
                ((CProtEntityObject*)hit.object)->SetPrototype(item, false);
            }
        }
    }
    if (!bHit)
    {
        CUndo undo("Create EntityPrototype");
        QString guid = GuidUtil::ToString(item->GetGUID());
        GetIEditor()->StartObjectCreation("EntityArchetype", guid);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityProtLibDialog::OnCopy()
{
    CBaseLibraryItem* pItem = m_pCurrentItem;
    if (pItem)
    {
        CClipboard clipboard(this);
        XmlNodeRef node = XmlHelpers::CreateXmlNode("EntityPrototype");
        CBaseLibraryItem::SerializeContext ctx(node, false);
        ctx.bCopyPaste = true;
        pItem->Serialize(ctx);
        clipboard.Put(node);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityProtLibDialog::OnPaste()
{
    if (!m_pLibrary)
    {
        return;
    }

    CClipboard clipboard(this);
    XmlNodeRef node = clipboard.Get();
    if (!node)
    {
        return;
    }

    if (strcmp(node->getTag(), "EntityPrototype") == 0)
    {
        // This is material node.
        CUndo undo("Add entity prototype library item");
        CBaseLibrary* pLib = m_pLibrary;
        CEntityPrototype* pItem = m_pEntityManager->LoadPrototype((CEntityPrototypeLibrary*)pLib, node);
        if (pItem)
        {
            ReloadItems();
            SelectItem(pItem);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityProtLibDialog::OnChangeEntityClass()
{
    CBaseLibraryItem* pItem = m_pCurrentItem;
    if (pItem)
    {
        CEntityPrototype* pPrototype = (CEntityPrototype*)pItem;
        QString entityClass = SelectEntityClass();
        if (entityClass.isEmpty())
        {
            return;
        }

        pPrototype->SetEntityClassName(entityClass);

        //////////////////////////////////////////////////////////////////////////
        // Reload entity prototype on all entities.
        //////////////////////////////////////////////////////////////////////////
        CBaseObjectsArray objects;
        GetIEditor()->GetObjectManager()->GetObjects(objects);
        for (int i = 0; i < objects.size(); i++)
        {
            CBaseObject* pObject = objects[i];
            if (qobject_cast<CProtEntityObject*>(pObject))
            {
                CProtEntityObject* pProtEntity = (CProtEntityObject*)pObject;
                if (pProtEntity->GetPrototype() == pPrototype)
                {
                    pProtEntity->SetPrototype(pPrototype, true);
                }
            }
        }
        //////////////////////////////////////////////////////////////////////////
        SelectItem(pItem, true /*bForceReload*/);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityProtLibDialog::OnRemoveLibrary()
{
    QString library = m_selectedLib;
    if (library.isEmpty())
    {
        return;
    }

    if (QMessageBox::question(this, QString(), tr("All objects from this library will be deleted from the level.\r\nAre you sure you want to delete this library?")) == QMessageBox::Yes)
    {
        CBaseObjectsArray objects;
        GetIEditor()->GetObjectManager()->GetObjects(objects);
        for (int i = 0; i < objects.size(); i++)
        {
            CBaseObject* pObject = objects[i];
            if (qobject_cast<CProtEntityObject*>(pObject))
            {
                CProtEntityObject* pProtEntity = (CProtEntityObject*)pObject;
                if (pProtEntity && pProtEntity->GetPrototype() && pProtEntity->GetPrototype()->GetLibrary())
                {
                    QString name = pProtEntity->GetPrototype()->GetLibrary()->GetName();
                    if (name == library)
                    {
                        GetIEditor()->GetObjectManager()->DeleteObject(pObject);
                    }
                }
            }
        }
        CBaseLibraryDialog::OnRemoveLibrary();
    }
}

#include <EntityProtLibDialog.moc>
