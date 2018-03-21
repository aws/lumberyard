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
#include "VehiclePartsPanel.h"
#include <Vehicles/ui_VehiclePartsPanel.h>
#include <Vehicles/ui_VehicleScaleDialog.h>
#include <Vehicles/ui_WheelMasterDialog.h>

#include <I3DEngine.h>
#include <IStatObj.h>
#include <ICryAnimation.h>

#include "VehicleEditorDialog.h"
#include "VehiclePrototype.h"
#include "VehicleHelperObject.h"
#include "VehiclePart.h"
#include "VehicleSeat.h"
#include "VehicleData.h"
#include "VehicleWeapon.h"
#include "VehicleComp.h"

#include <IVehicleSystem.h>

#include <QKeyEvent>
#include <QMenu>

const static bool SELECTED = true;
const static bool NOT_SELECTED = false;
const static bool UNIQUE = true;
const static bool NOT_UNIQUE = false;
const static bool EDIT_LABEL = true;
const static bool NOT_EDIT_LABEL = false;
const static bool FROM_ASSET = true;
const static bool FROM_EDITOR = false;

class VehicleTreeWidgetItem
    : public QTreeWidgetItem
{
public:
    explicit VehicleTreeWidgetItem(CBaseObject* obj, QTreeWidgetItem* parent = nullptr)
        : QTreeWidgetItem(QStringList(obj->GetName()))
    {
        setData(0, Qt::UserRole, QVariant::fromValue(obj));
        setIcon(0, QPixmap(QStringLiteral(":/VehicleEditorDialog/veed_tree-%1.png").arg(IVeedObject::GetVeedObject(obj)->GetIconIndex(), 2, 10, QLatin1Char('0'))));
        setFlags(flags() | Qt::ItemIsEditable);

        if (parent)
        {
            parent->addChild(this);
        }
    }

    explicit VehicleTreeWidgetItem(CVehiclePrototype* prot)
        : QTreeWidgetItem(QStringList(prot->GetCEntity()->GetEntityClass()))
    {
        setData(0, Qt::UserRole, QVariant::fromValue<CBaseObject*>(prot));
        setIcon(0, QPixmap(QStringLiteral(":/VehicleEditorDialog/veed_tree-%1.png").arg(IVeedObject::GetVeedObject(prot)->GetIconIndex(), 2, 10, QLatin1Char('0'))));
        setExpanded(true);
    }
};

class VehiclePartModel
    : public QAbstractListModel
{
public:
    VehiclePartModel(std::vector<CVehiclePart*>* items, QObject* parent = nullptr)
        : QAbstractListModel(parent)
        , m_items(items)
    {
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return (parent.isValid() || m_items == nullptr) ? 0 : m_items->size();
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.row() >= rowCount(index.parent()))
        {
            return QVariant();
        }

        auto item = m_items->at(index.row());
        switch (role)
        {
        case Qt::DisplayRole:
            return item->GetName();
        case Qt::UserRole:
            return QVariant::fromValue<CBaseObject*>(item);
        default:
            return QVariant();
        }
    }

private:
    std::vector<CVehiclePart*>* m_items;
};

//////////////////////////////////////////////////////////////////////////////
// CWheelMasterDialog implementation
//////////////////////////////////////////////////////////////////////////////

CWheelMasterDialog::CWheelMasterDialog(IVariable* pVar, QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , m_pVar(pVar->Clone(true))
    , m_ui(new Ui::WheelMasterDialog)
    , m_initialized(false)
{
    m_ui->setupUi(this);
    m_ui->m_propsCtrl->Setup();
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    m_pWheels = 0;

    m_ui->m_boxes0->setChecked(false); // uncheck axle

    for (int i = 1; i < NUM_BOXES; ++i)
    {
        findChild<QCheckBox*>(QStringLiteral("m_boxes%1").arg(i))->setChecked(true);  // check the rest
    }

    m_ui->m_toggleAll->setChecked(true);

    connect(m_ui->m_boxes0, &QCheckBox::clicked, this, &CWheelMasterDialog::OnCheckBoxClicked);
    connect(m_ui->m_boxes1, &QCheckBox::clicked, this, &CWheelMasterDialog::OnCheckBoxClicked);
    connect(m_ui->m_boxes2, &QCheckBox::clicked, this, &CWheelMasterDialog::OnCheckBoxClicked);
    connect(m_ui->m_boxes3, &QCheckBox::clicked, this, &CWheelMasterDialog::OnCheckBoxClicked);
    connect(m_ui->m_boxes4, &QCheckBox::clicked, this, &CWheelMasterDialog::OnCheckBoxClicked);
    connect(m_ui->m_boxes5, &QCheckBox::clicked, this, &CWheelMasterDialog::OnCheckBoxClicked);
    connect(m_ui->m_boxes6, &QCheckBox::clicked, this, &CWheelMasterDialog::OnCheckBoxClicked);
    connect(m_ui->m_boxes7, &QCheckBox::clicked, this, &CWheelMasterDialog::OnCheckBoxClicked);
    connect(m_ui->m_boxes8, &QCheckBox::clicked, this, &CWheelMasterDialog::OnCheckBoxClicked);
    connect(m_ui->m_buttonApply, &QPushButton::clicked, this, &CWheelMasterDialog::OnApplyWheels);
    connect(m_ui->m_toggleAll, &QCheckBox::clicked, this, &CWheelMasterDialog::OnToggleAll);
}

CWheelMasterDialog::~CWheelMasterDialog()
{
}

void CWheelMasterDialog::showEvent(QShowEvent *event)
{
    if (m_initialized)
    {
        return;
    }

    m_initialized = true;

    CVarBlock block;
    block.AddRef();
    block.AddVariable(m_pVar);

    m_ui->m_propsCtrl->AddVarBlock(&block);

    ReflectedPropertyItem* pItem = m_ui->m_propsCtrl->FindItemByVar(m_pVar);
    m_ui->m_propsCtrl->Expand(pItem, true);
    m_ui->m_propsCtrl->Expand(pItem->GetChild(0), true);

    ApplyCheckBoxes();

    // fill wheel listbox
    m_ui->m_wheelList->setModel(new VehiclePartModel(m_pWheels, this));
}

void CWheelMasterDialog::OnCheckBoxClicked()
{
    ApplyCheckBoxes();
}

void CWheelMasterDialog::ApplyCheckBoxes()
{
    ReflectedPropertyItem* pItem = m_ui->m_propsCtrl->FindItemByVar(m_pVar->GetVariable(0));
    assert(pItem);

    for (int i = 0; i < NUM_BOXES && i < pItem->GetChildCount(); ++i)
    {
        IVariable* pVar = pItem->GetChild(i)->GetVariable();

        if (!findChild<QCheckBox*>(QStringLiteral("m_boxes%1").arg(i))->isChecked())
        {
            pVar->SetFlags(pVar->GetFlags() | IVariable::UI_DISABLED);
        }
        else
        {
            pVar->SetFlags(pVar->GetFlags() & ~IVariable::UI_DISABLED);
        }
    }

    m_ui->m_propsCtrl->InvalidateCtrl();
}


void CWheelMasterDialog::OnApplyWheels()
{
    // apply (active) master values to all selected wheels
    if (m_ui->m_wheelList->selectionModel()->hasSelection())
    {
        const QModelIndexList items = m_ui->m_wheelList->selectionModel()->selectedRows();

        for (auto item : items)
        {
            CVehiclePart* pWheel = static_cast<CVehiclePart*>(item.data(Qt::UserRole).value<CBaseObject*>());

            IVariable* pPartVar = IVeedObject::GetVeedObject(pWheel)->GetVariable();
            IVariable* pSubVar = GetChildVar(pPartVar, "SubPartWheel");

            if (!pSubVar)
            {
                Log("Warning, SubPartWheel not found!");
                continue;
            }

            IVariable* pEditMaster = m_pVar->GetVariable(0);

            for (int i = 0; i < pEditMaster->GetNumVariables() && i < NUM_BOXES; ++i)
            {
                if (findChild<QCheckBox*>(QStringLiteral("m_boxes%1").arg(i))->isChecked())
                {
                    QString val;
                    pEditMaster->GetVariable(i)->Get(val);

                    // get variable from wheel to update
                    QString varName = pEditMaster->GetVariable(i)->GetName();
                    IVariable* pVar = GetChildVar(pSubVar, varName.toUtf8().data());

                    if (pVar)
                    {
                        pVar->Set(val);
                        VeedLog("%s: applying %s [%s]", pWheel->GetName(), varName.toUtf8().data(), val);
                    }
                    else
                    {
                        Log("%s: Variable %s not found", pWheel->GetName(), varName.toUtf8().data());
                    }
                }
            }
        }
    }
}

void CWheelMasterDialog::OnToggleAll()
{
    for (int i = 1; i < NUM_BOXES; ++i) // don't toggle axle box
    {
        findChild<QCheckBox*>(QStringLiteral("m_boxes%1").arg(i))->setChecked(m_ui->m_toggleAll->isChecked());
    }

    ApplyCheckBoxes();
}


//////////////////////////////////////////////////////////////////////////////
// VehicleScaleDlg implementation
//////////////////////////////////////////////////////////////////////////////
class CVehicleScaleDialog
    : public QDialog
{
public:
    CVehicleScaleDialog(QWidget* pParent = NULL)
        : QDialog(pParent)
        , m_ui(new Ui::VehicleScaleDialog)
    {
        m_ui->setupUi(this);
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
        m_ui->m_editScale->setValue(1.0);
    }
    float GetScale(){ return m_ui->m_editScale->value(); }

private:
    QScopedPointer<Ui::VehicleScaleDialog> m_ui;
};

//////////////////////////////////////////////////////////////////////////////
// VehiclePartsPanel implementation
//////////////////////////////////////////////////////////////////////////////

CVehiclePartsPanel::CVehiclePartsPanel(CVehicleEditorDialog* pDialog)
    : QWidget(pDialog)
    , m_pVehicle(0)
    , m_pSelItem(0)
    , m_partToTree(pDialog->m_partToTree)
    , m_ui(new Ui::VehiclePartsPanel)
{
    m_ui->setupUi(this);
    m_ui->m_propsCtrl->Setup();
    m_ui->m_treeCtrl->setContextMenuPolicy(Qt::CustomContextMenu);

    m_ui->m_treeCtrl->installEventFilter(this);
    m_ui->m_treeCtrl->viewport()->installEventFilter(this);

    m_ui->m_vSplitter->setSizes({ 250, 150 });
    m_ui->m_hSplitter->setSizes({ 180, 200 });

    connect(m_ui->m_bSeats, &QCheckBox::toggled, this, &CVehiclePartsPanel::ShowSeats);
    connect(m_ui->m_bWheels, &QCheckBox::toggled, this, &CVehiclePartsPanel::ShowWheels);
    connect(m_ui->m_bComps, &QCheckBox::toggled, this, &CVehiclePartsPanel::ShowComps);
    connect(m_ui->m_bVeedHelpers, &QCheckBox::toggled, this, &CVehiclePartsPanel::ShowVeedHelpers);
    connect(m_ui->m_bAssetHelpers, &QCheckBox::toggled, this, &CVehiclePartsPanel::ShowAssetHelpers);

    connect(m_ui->m_treeCtrl, &QWidget::customContextMenuRequested, this, &CVehiclePartsPanel::OnItemRClick);
    connect(m_ui->m_treeCtrl, &QTreeWidget::itemChanged, this, &CVehiclePartsPanel::OnHelperRenameDone);
    connect(m_ui->m_treeCtrl->selectionModel(), &QItemSelectionModel::currentChanged, this, &CVehiclePartsPanel::OnSelect);

    assert(pDialog); // dialog is needed
    m_pDialog = pDialog;

    m_pObjMan = GetIEditor()->GetObjectManager();
    m_pMainPart = 0;
    m_pRootVar = 0;
}

CVehiclePartsPanel::~CVehiclePartsPanel()
{
}

//////////////////////////////////////////////////////////////////////////
bool CVehiclePartsPanel::eventFilter(QObject* watched, QEvent* event)
{
    // Key press in items tree view.
    if ((watched == m_ui->m_treeCtrl || watched == m_ui->m_treeCtrl->viewport()) && event->type() == QEvent::KeyPress)
    {
        QKeyEvent* e = static_cast<QKeyEvent*>(event);
        //if (e->matches(QKeySequence::Copy))
        //{
        //  OnCopy();    // Ctrl+C
        //}
        //if (e->matches(QKeySequence::Paste))
        //{
        //  OnPaste(); // Ctrl+V
        //}

        if (e->matches(QKeySequence::Delete))
        {
            OnDeleteItem();
        }

        if (e->key() == Qt::Key_F2 && e->modifiers() == Qt::NoModifier)
        {
            //OnRenameItem();
        }
    }
    return false;
}


//////////////////////////////////////////////////////////////////////////////
// called when selection in tree control changes
void CVehiclePartsPanel::OnSelect(const QModelIndex& index)
{
    if (!index.isValid())
    {
        return;
    }

    m_ui->m_propsCtrl->RemoveAllItems();
    m_ui->m_propsCtrl->SetCallbackOnNonModified(false);
    m_ui->m_propsCtrl->SetUpdateCallback(functor(*this, &CVehiclePartsPanel::OnApplyChanges));

    CBaseObject* pObj = index.data(Qt::UserRole).value<CBaseObject*>();

    if (!pObj || qobject_cast<CVehiclePrototype*>(pObj))
    {
        return;
    }

    IVeedObject* pVO = IVeedObject::GetVeedObject(pObj);

    // open properties of this obj
    IVariable* pVar = pVO->GetVariable();

    if (pVar)
    {
        CVarBlock* block = new CVarBlock();
        block->AddVariable(pVar);
        m_ui->m_propsCtrl->AddVarBlock(block);
        m_ui->m_propsCtrl->ExpandAll();
        m_ui->m_propsCtrl->EnableUpdateCallback(false);

        for (int i = 0; i < pVar->GetNumVariables(); ++i)
        {
            IVariable* pSubVar = pVar->GetVariable(i);
            ReflectedPropertyItem* pItem = m_ui->m_propsCtrl->FindItemByVar(pSubVar);

            m_pDialog->OnPropsSelChanged(pItem);
        }

        m_ui->m_propsCtrl->EnableUpdateCallback(true);

        // if its a Part, also expand SubVariable
        if (qobject_cast<CVehiclePart*>(pObj))
        {
            if (IVariable* pSubVar = GetChildVar(pVar, ((CVehiclePart*)pObj)->GetPartClass().toUtf8().data()))
            {
                ReflectedPropertyItem* pItem = m_ui->m_propsCtrl->FindItemByVar(pSubVar);

                if (pItem)
                {
                    m_ui->m_propsCtrl->Expand(pItem, true);
                }
            }

            pVO->OnTreeSelection();
        }
    }

    if (!pObj->IsSelected())
    {
        GetIEditor()->ClearSelection();
        GetIEditor()->SelectObject(pObj);
    }
}

//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::OnItemRClick(const QPoint& point)
{
    if (!m_pVehicle)
    {
        return;
    }

    // Show helper menu.
    CVehiclePart* pPart = 0;


    // Select the item that is at the point myPoint.
    const QModelIndex hItem = m_ui->m_treeCtrl->indexAt(point);

    if (hItem.isValid())
    {
        m_ui->m_treeCtrl->setCurrentIndex(hItem);
        CBaseObject* pObj = hItem.data(Qt::UserRole).value<CBaseObject*>();

        if (!pObj)
        {
            return;
        }

        m_pSelItem = pObj;

        // Create pop up menu.
        QMenu menu;

        // insert common items
        //connect(menu.addAction(tr("Select")), &QAction::triggered, this, &CVehiclePartsPanel::OnPartSelect);

        // check what kind of item we're clicking on
        if (qobject_cast<CVehiclePrototype*>(pObj))
        {
            connect(menu.addAction(tr("Add Part")), &QAction::triggered, this, &CVehiclePartsPanel::OnPartNew);
            connect(menu.addAction(tr("Add Seat")), &QAction::triggered, this, &CVehiclePartsPanel::OnSeatNew);
            connect(menu.addAction(tr("Add Component")), &QAction::triggered, this, &CVehiclePartsPanel::OnComponentNew);
            connect(menu.addAction(tr("Add Helper")), &QAction::triggered, this, &CVehiclePartsPanel::OnHelperNew);
            menu.addSeparator();
            connect(menu.addAction(tr("Update Scale")), &QAction::triggered, this, &CVehiclePartsPanel::OnScaleHelpers);
        }
        else if (qobject_cast<CVehiclePart*>(pObj))
        {
            connect(menu.addAction(tr("Rename")), &QAction::triggered, this, &CVehiclePartsPanel::OnHelperRename);
            connect(menu.addAction(tr("Delete")), &QAction::triggered, this, &CVehiclePartsPanel::OnDeleteItem);

            CVehiclePart* pPart = (CVehiclePart*)pObj;

            if (!pPart->IsLeaf())
            {
                menu.addSeparator();
                connect(menu.addAction(tr("Add Part")), &QAction::triggered, this, &CVehiclePartsPanel::OnPartNew);
                connect(menu.addAction(tr("Add Seat")), &QAction::triggered, this, &CVehiclePartsPanel::OnSeatNew);
            }
            else
            {
                if (pPart->GetPartClass() == PARTCLASS_WHEEL)
                {
                    menu.addSeparator();
                    connect(menu.addAction(tr("Edit Wheel Master")), &QAction::triggered, this, &CVehiclePartsPanel::OnEditWheelMaster);
                }
            }
        }
        else if (qobject_cast<CVehicleSeat*>(pObj))
        {
            connect(menu.addAction(tr("Rename")), &QAction::triggered, this, &CVehiclePartsPanel::OnHelperRename);
            connect(menu.addAction(tr("Delete")), &QAction::triggered, this, &CVehiclePartsPanel::OnDeleteItem);
            menu.addSeparator();
            connect(menu.addAction(tr("Add Primary Weapon")), &QAction::triggered, this, &CVehiclePartsPanel::OnPrimaryWeaponNew);
        }
        else if (qobject_cast<CVehicleHelper*>(pObj))
        {
            if (!((CVehicleHelper*)pObj)->IsFromGeometry())
            {
                connect(menu.addAction(tr("Rename")), &QAction::triggered, this, &CVehiclePartsPanel::OnHelperRename);
                connect(menu.addAction(tr("Delete")), &QAction::triggered, this, &CVehiclePartsPanel::OnDeleteItem);
            }
        }
        else if (qobject_cast<CVehicleComponent*>(pObj))
        {
            connect(menu.addAction(tr("Rename")), &QAction::triggered, this, &CVehiclePartsPanel::OnHelperRename);
            connect(menu.addAction(tr("Delete")), &QAction::triggered, this, &CVehiclePartsPanel::OnDeleteItem);
        }
        else
        {
            Log("unknown item clicked");
        }

        menu.exec(mapToGlobal(point));
    }



    /*CClipboard clipboard(this);
    bool bNoPaste = clipboard.IsEmpty();
    int pasteFlags = 0;
    if (bNoPaste)
    pasteFlags |= MF_GRAYED;*/
}

//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::OnPartSelect()
{
    const QModelIndex hSelItem = m_ui->m_treeCtrl->currentIndex();
    CBaseObject* pSelPart = hSelItem.data(Qt::UserRole).value<CBaseObject*>();

    if (pSelPart)
    {
        GetIEditor()->ClearSelection();
        GetIEditor()->SelectObject(pSelPart);
    }
}

//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::OnPartNew()
{
    CUndoSuspend susp;
    assert(m_pSelItem);

    // part creation

    QTreeWidgetItem* hSelItem = m_ui->m_treeCtrl->currentItem();
    CBaseObject* pParent = hSelItem->data(0, Qt::UserRole).value<CBaseObject*>();

    // create obj
    CVehiclePart* pObj = (CVehiclePart*)GetIEditor()->GetObjectManager()->NewObject("VehiclePart");

    if (pObj)
    {
        //pObj->SetHidden(true);
        pObj->SetName("NewPart");
        pObj->SetWorldTM(m_pVehicle->GetWorldTM());
        pObj->SetVehicle(m_pVehicle);
        pObj->UpdateObjectFromVar();

        if (pParent)
        {
            if (pParent == m_pVehicle)
            {
                ((CVehiclePrototype*)pParent)->AddPart(pObj);
            }
            else if (qobject_cast<CVehiclePart*>(pParent))
            {
                ((CVehiclePart*)pParent)->AddPart(pObj);
            }

            QTreeWidgetItem* hItem = InsertTreeItem(pObj, hSelItem, true);
            m_ui->m_treeCtrl->editItem(hItem);
        }

        pObj->AddEventListener(functor(*this, &CVehiclePartsPanel::OnObjectEvent));

        GetIEditor()->SetModifiedFlag();
    }
}

//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::OnComponentNew()
{
    CUndoSuspend susp;
    assert(m_pSelItem);

    QTreeWidgetItem* hSelItem = m_ui->m_treeCtrl->currentItem();
    CVehiclePrototype* pParent = static_cast<CVehiclePrototype*>(hSelItem->data(0, Qt::UserRole).value<CBaseObject*>());

    // create obj
    CVehicleComponent* pComp = (CVehicleComponent*)GetIEditor()->GetObjectManager()->NewObject("VehicleComponent");

    if (pComp)
    {
        pComp->SetVehicle(m_pVehicle);
        pComp->CreateVariable();

        if (pParent)
        {
            pParent->AddComponent(pComp);
            pComp->ResetPosition();

            QTreeWidgetItem* hItem = InsertTreeItem(pComp, hSelItem, true);
            m_ui->m_treeCtrl->editItem(hItem);
        }

        pComp->UpdateObjectFromVar();

        pComp->AddEventListener(functor(*this, &CVehiclePartsPanel::OnObjectEvent));

        GetIEditor()->SetModifiedFlag();
    }
}

//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::OnSeatNew()
{
    CUndoSuspend susp;
    assert(m_pSelItem);

    QTreeWidgetItem* hSelItem = m_ui->m_treeCtrl->currentItem();
    CBaseObject* pParent = hSelItem->data(0, Qt::UserRole).value<CBaseObject*>();

    // create obj
    CVehicleSeat* pSeat = (CVehicleSeat*)GetIEditor()->GetObjectManager()->NewObject("VehicleSeat");

    if (pSeat)
    {
        pSeat->SetHidden(true);
        pSeat->SetVehicle(m_pVehicle);
        pSeat->UpdateObjectFromVar();

        if (pParent)
        {
            pParent->AttachChild(pSeat);
            pSeat->UpdateVarFromObject();

            QTreeWidgetItem* hItem = InsertTreeItem(pSeat, hSelItem, true);
            m_ui->m_treeCtrl->editItem(hItem);
        }

        pSeat->AddEventListener(functor(*this, &CVehiclePartsPanel::OnObjectEvent));

        GetIEditor()->SetModifiedFlag();
    }
}

//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::OnPrimaryWeaponNew()
{
    CVehicleSeat* pSeat = static_cast<CVehicleSeat*>(m_ui->m_treeCtrl->currentIndex().data(Qt::UserRole).value<CBaseObject*>());

    if (pSeat)
    {
        CreateWeapon(WEAPON_PRIMARY, pSeat);
    }
}

//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::OnSecondaryWeaponNew()
{
    CVehicleSeat* pSeat = static_cast<CVehicleSeat*>(m_ui->m_treeCtrl->currentIndex().data(Qt::UserRole).value<CBaseObject*>());

    if (pSeat)
    {
        CreateWeapon(WEAPON_SECONDARY, pSeat);
    }
}


//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::OnHelperNew()
{
    CUndoSuspend susp;
    assert(m_pSelItem);

    // helper creation:
    // - create Helper object in editor
    // - create tree item

    // place helper above vehicle
    AABB bbox;
    m_pVehicle->GetBoundBox(bbox);
    Vec3 pos = m_pVehicle->GetWorldTM().GetTranslation() + Vec3(0, 0, bbox.GetSize().z + 0.25f);

    // create obj
    CVehicleHelper* pHelper = CreateHelperObject(pos, Vec3(FORWARD_DIRECTION), "NewHelper", UNIQUE, SELECTED, EDIT_LABEL, FROM_EDITOR);

    GetIEditor()->SetModifiedFlag();
}


//////////////////////////////////////////////////////////////////////////////
QTreeWidgetItem* CVehiclePartsPanel::InsertTreeItem(CBaseObject* pObj, CBaseObject* pParent, bool select /*=false*/)
{
    QTreeWidgetItem* hParentItem = stl::find_in_map(m_partToTree, pParent, STreeItem()).item;

    if (hParentItem)
    {
        return InsertTreeItem(pObj, hParentItem, select);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
QTreeWidgetItem* CVehiclePartsPanel::InsertTreeItem(CBaseObject* pObj, QTreeWidgetItem* hParent, bool select /*=false*/)
{
    IVeedObject* pVO = IVeedObject::GetVeedObject(pObj);

    if (!pVO)
    {
        return NULL;
    }

    QTreeWidgetItem* hItem = new VehicleTreeWidgetItem(pObj, hParent);
    hItem->setExpanded(true);

    if (select)
    {
        m_ui->m_treeCtrl->setCurrentItem(hItem);
        m_pSelItem = pObj;
    }

    //m_treeCtrl.EditLabel(hItem);

    m_partToTree[pObj] = STreeItem(hItem, hParent);

    return hItem;
}


//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::OnDeleteItem()
{
    CUndoSuspend susp;

    const QModelIndex hItem = m_ui->m_treeCtrl->currentIndex();

    if (!hItem.isValid())
    {
        return;
    }

    CBaseObject* pObj = hItem.data(Qt::UserRole).value<CBaseObject*>();

    if (pObj == m_pVehicle) // don't delete vehicle itself
    {
        return;
    }

    DeleteObject(pObj);
    DeleteTreeItem(m_partToTree.find(pObj));

    m_pSelItem = 0;
}


//////////////////////////////////////////////////////////////////////////////
CVehiclePart* CVehiclePartsPanel::GetPartForHelper(CVehicleHelper* pHelper)
{
    TPartToTreeMap::iterator it = m_partToTree.find((CBaseObject*)pHelper);

    if (it != m_partToTree.end())
    {
        QTreeWidgetItem* hParent = it->second.parent;
        return static_cast<CVehiclePart*>(hParent->data(0, Qt::UserRole).value<CBaseObject*>());
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::DeleteObject(CBaseObject* pObj)
{
    CUndoSuspend susp;
    // delete editor object
    pObj->RemoveEventListener(functor(*this, &CVehiclePartsPanel::OnObjectEvent));
    GetIEditor()->DeleteObject(pObj);
}


//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::OnHelperRename()
{
    m_ui->m_treeCtrl->edit(m_ui->m_treeCtrl->currentIndex());
}

//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::OnHelperRenameDone(QTreeWidgetItem* hItem)
{
    CUndoSuspend susp;
    const QString text = hItem->text(0);
    CBaseObject* pObj = hItem->data(0, Qt::UserRole).value<CBaseObject*>();
    if (text.isEmpty())
    {
        hItem->setText(0, pObj->GetName());
        return;
    }

    pObj->SetName(text);

    if (IVeedObject* pVO = IVeedObject::GetVeedObject(pObj))
    {
        pVO->UpdateVarFromObject();
    }

    //hItem->SortChildren(0, Qt::AscendingOrder);

    /*NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
    TVITEM tvitem = (TVITEM)pNMTreeView->itemNew;
    HTREEITEM hItem = tvitem.hItem;*/
}

//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::ExpandProps(ReflectedPropertyItem* pItem, bool expand /*=true*/)
{
    // expand all children and their children
    for (int i = 0; i < pItem->GetChildCount(); ++i)
    {
        ReflectedPropertyItem* pChild = pItem->GetChild(i);
        m_ui->m_propsCtrl->Expand(pChild, true);

        for (int j = 0; j < pChild->GetChildCount(); ++j)
        {
            m_ui->m_propsCtrl->Expand(pChild->GetChild(j), true);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
IVariable* CVehiclePartsPanel::GetVariable()
{
    return m_pRootVar;
}

//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::UpdateVehiclePrototype(CVehiclePrototype* pProt)
{
    CUndoSuspend susp;

    // delete parts of current vehicle, if present
    m_pDialog->DeleteEditorObjects();

    assert(pProt && pProt->GetVariable());
    m_pVehicle = pProt;
    m_pRootVar = pProt->GetVehicleData()->GetRoot()->Clone(true);

    m_ui->m_propsCtrl->SetVehicleVar(m_pRootVar);

    ReloadTreeCtrl();

    ShowAssetHelpers(m_ui->m_bAssetHelpers->isChecked());

    m_pVehicle->AddEventListener(functor(*this, &CVehiclePartsPanel::OnObjectEvent));

    GetIEditor()->SetModifiedFlag();

#ifdef _DEBUG
    DumpPartTreeMap();
#endif
}

//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::ReloadTreeCtrl()
{
    if (m_pVehicle)
    {
        m_ui->m_treeCtrl->clear();

        int icon = IVeedObject::GetVeedObject(m_pVehicle)->GetIconIndex();
        m_hVehicle = new VehicleTreeWidgetItem(m_pVehicle);
        m_ui->m_treeCtrl->addTopLevelItem(m_hVehicle);

        FillParts(GetVariable()); // parse parts in vehicle xml
        FillHelpers(GetVariable());    // add all helpers
        FillAssetHelpers();
        FillComps(GetVariable()); // add components
        FillSeats(); // add seats to tree
    }
}

//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::OnApplyChanges(IVariable* pVar)
{
    bool bReloadTree = false;

    if (pVar)
    {
        if (QString::compare(pVar->GetName(), "name", Qt::CaseInsensitive) == 0)
        {
            const QModelIndex hItem = m_ui->m_treeCtrl->currentIndex();
            assert(hItem.isValid());

            m_ui->m_treeCtrl->model()->setData(hItem, pVar->GetDisplayValue());
        }
        else if (QString::compare(pVar->GetName(), "part", Qt::CaseInsensitive) == 0)
        {
            bReloadTree = true;
        }
    }

    UpdateVariables();

    if (bReloadTree)
    {
        ReloadTreeCtrl();
    }
}

//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::UpdateVariables()
{
    // push workspace data into main tree
    ReplaceChildVars(m_pRootVar, m_pVehicle->GetVariable());
}

//////////////////////////////////////////////////////////////////////////////
CVehicleHelper* CVehiclePartsPanel::CreateHelperObject(const Vec3& pos, const Vec3& dir, const QString& name, bool unique, bool select, bool editLabel, bool isFromAsset, IVariable* pHelperVar /*=0*/)
{
    CUndoSuspend susp;

    if (name.isEmpty()) // don't allow to create nameless helpers
    {
        return 0;
    }

    static IObjectManager* pObjMan = GetIEditor()->GetObjectManager();
    CVehicleHelper* pObj = (CVehicleHelper*)pObjMan->NewObject("VehicleHelper");

    if (qobject_cast<CVehicleHelper*>(pObj))
    {
        pObj->SetHidden(false);

        if (unique)
        {
            pObj->SetUniqueName(name);
        }
        else
        {
            pObj->SetName(name);
        }

        if (select)
        {
            //GetIEditor()->ClearSelection();
            //GetIEditor()->SelectObject(pObj);
        }

        Vec3 direction = (dir.GetLengthSquared() > 0) ? dir.GetNormalized() : Vec3(0, 1, 0);
        Matrix34 tm = Matrix33::CreateRotationVDir(m_pVehicle->GetWorldTM().TransformVector(direction));

        pObj->SetIsFromGeometry(isFromAsset);

        tm.SetTranslation(pos);
        pObj->SetWorldTM(tm);

        QTreeWidgetItem* hItem = InsertTreeItem(pObj, m_hVehicle, select);

        if (editLabel)
        {
            m_ui->m_treeCtrl->editItem(hItem);
        }

        m_pVehicle->AddHelper(pObj, pHelperVar);

        pObj->AddEventListener(functor(*this, &CVehiclePartsPanel::OnObjectEvent));

        return pObj;
    }

    if (pObj)
    {
        delete pObj;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::CreateHelpersFromStatObj(IStatObj* pObj, CVehiclePart* pParent)
{
    CUndoSuspend susp;

    if (!pObj)
    {
        return;
    }

    // parse all subobjects, check if helper, and create
    for (int j = 0; j < pObj->GetSubObjectCount(); ++j)
    {
        IStatObj::SSubObject* pSubObj = pObj->GetSubObject(j);

        //Log("sub <%s> type %i", pSubObj->name, pSubObj->nType);
        if (pSubObj->nType == STATIC_SUB_OBJECT_DUMMY)
        {
            // check if there's already a helper with that name on this part -> skip
            if (m_pVehicle->GetHelper(pSubObj->name.c_str()))
            {
                //Log("helper %s already loaded from xml, skipping", pSubObj->name);
                continue;
            }

            Vec3 vPos = pObj->GetHelperPos(pSubObj->name);

            if (vPos.IsZero())
            {
                continue;
            }

            vPos = m_pVehicle->GetCEntity()->GetIEntity()->GetWorldTM().TransformPoint(vPos);
            QString name(pSubObj->name);
            CVehicleHelper* pHelper = CreateHelperObject(vPos, Vec3(FORWARD_DIRECTION), name, NOT_UNIQUE, NOT_SELECTED, NOT_EDIT_LABEL, FROM_ASSET);
        }
    }

    GetIEditor()->SetModifiedFlag();
}

//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::ShowObject(TPartToTreeMap::iterator it, bool bShow)
{
    // hide/show object and remove/insert tree item
    CBaseObject* pObj = it->first;
    pObj->SetHidden(!bShow);

    if (!bShow)
    {
        // delete item from tree and set to 0 in map
        delete it->second.item;
        it->second.item = 0;
    }
    else if (it->second.item == 0)
    {
        // if currently hidden, insert beneath parent item
        it->second.item = new VehicleTreeWidgetItem(pObj, it->second.parent);

        if (!it->second.item)
        {
            Log("InsertItem for %s failed!", pObj->GetName());
        }
    }
}


//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::ShowVeedHelpers(bool bShow)
{
    for (TPartToTreeMap::iterator it = m_partToTree.begin(); it != m_partToTree.end(); ++it)
    {
        if (qobject_cast<CVehicleHelper*>((*it).first))
        {
            CVehicleHelper* pHelper = (CVehicleHelper*)it->first;

            if (!pHelper->IsFromGeometry())
            {
                ShowObject(it, bShow);
            }
        }
    }
}


//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::ShowSeats(bool bShow)
{
    for (TPartToTreeMap::iterator it = m_partToTree.begin(); it != m_partToTree.end(); ++it)
    {
        if (qobject_cast<CVehicleSeat*>((*it).first))
        {
            ShowObject(it, bShow);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::ShowComps(bool bShow)
{
    for (TPartToTreeMap::iterator it = m_partToTree.begin(); it != m_partToTree.end(); ++it)
    {
        if (qobject_cast<CVehicleComponent*>((*it).first))
        {
            ShowObject(it, bShow);
        }
    }
}


//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::ShowWheels(bool bShow)
{
    for (TPartToTreeMap::iterator it = m_partToTree.begin(); it != m_partToTree.end(); ++it)
    {
        if (qobject_cast<CVehiclePart*>((*it).first))
        {
            CVehiclePart* pPart = (CVehiclePart*)it->first;

            if (pPart->GetPartClass() == PARTCLASS_WHEEL)
            {
                ShowObject(it, bShow);
            }
        }
    }
}


//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::ShowAssetHelpers(bool bShow)
{
    CUndoSuspend susp;

    if (bShow)
    {
        // TODO: Automatically recreating list asset helpers when vehicle geometry changes.
        // Need callback when an animated part is created.
        // Currently will need to check/uncheck the box for the update to happen.
        RemoveAssetHelpers();
        FillAssetHelpers();
    }
    else
    {
        HideAssetHelpers();
    }
}

//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::DeleteTreeItem(TPartToTreeMap::iterator it)
{
    // delete tree item, erase from map
    delete (*it).second.item;
    m_partToTree.erase(it);
}

//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::FillAssetHelpers()
{
    CUndoSuspend susp;

    if (m_pVehicle == NULL)
    {
        return;
    }


    IEntity* pEntity = m_pVehicle->GetCEntity()->GetIEntity();

    IVehicleSystem* pVehicleSystem = NULL;
    IGame* pGame = gEnv->pGame;

    if (pGame != NULL)
    {
        IGameFramework* pGameFramework = pGame->GetIGameFramework();

        if (pGameFramework != NULL)
        {
            pVehicleSystem = pGameFramework->GetIVehicleSystem();
        }
    }

    if (pVehicleSystem == NULL)
    {
        return;
    }


    IVehicle* pVehicle = pVehicleSystem->GetVehicle(pEntity->GetId());
    assert(pVehicle != NULL);

    if (pVehicle == NULL)
    {
        return;
    }

    for (int partIndex = 0; partIndex < pVehicle->GetPartCount(); partIndex++)
    {
        IVehiclePart* pPart = pVehicle->GetPart(partIndex);

        const TVehicleObjectId ANIMATED_PART_ID = pVehicleSystem->GetVehicleObjectId("Animated");
        TVehicleObjectId partId = pPart->GetId();
        bool isAnimatedPart = (partId == ANIMATED_PART_ID);

        if (isAnimatedPart)
        {
            IEntity* pPartEntity = pPart->GetEntity();

            if (pPartEntity != NULL)
            {
                int slot = pPart->GetSlot();
                ICharacterInstance* pCharacter = pPartEntity->GetCharacter(slot);

                if (pCharacter != NULL)
                {
                    ISkeletonPose* pSkeletonPose = pCharacter->GetISkeletonPose();
                    IDefaultSkeleton& rIDefaultSkeleton = pCharacter->GetIDefaultSkeleton();
                    assert(pSkeletonPose != NULL);

                    int32 numJoints = rIDefaultSkeleton.GetJointCount();

                    for (int32 jointId = 0; jointId < numJoints; jointId++)
                    {
                        QString jointName = rIDefaultSkeleton.GetJointNameByID(jointId);

                        const QuatT& joint = pSkeletonPose->GetRelJointByID(jointId);

                        const Vec3& localPosition = joint.t;
                        const Vec3 position = m_pVehicle->GetWorldTM().TransformPoint(localPosition);

                        const Vec3& direction = joint.GetColumn1();

                        CVehicleHelper* pHelper = CreateHelperObject(position, direction, jointName, NOT_UNIQUE, NOT_SELECTED, NOT_EDIT_LABEL, FROM_ASSET);
                        pHelper->UpdateVarFromObject();
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::RemoveAssetHelpers()
{
    for (TPartToTreeMap::iterator it = m_partToTree.begin(); it != m_partToTree.end(); )
    {
        bool found = false;

        if (qobject_cast<CVehicleHelper*>((*it).first))
        {
            CVehicleHelper* pHelper = (CVehicleHelper*)(it->first);

            if (pHelper->IsFromGeometry())
            {
                DeleteObject(pHelper);
                DeleteTreeItem(it++);
                found = true;
            }
        }

        if (!found)
        {
            ++it;
        }
    }

    GetIEditor()->SetModifiedFlag();
}

void CVehiclePartsPanel::HideAssetHelpers()
{
    for (TPartToTreeMap::iterator it = m_partToTree.begin(); it != m_partToTree.end(); )
    {
        bool found = false;

        if (qobject_cast<CVehicleHelper*>((*it).first))
        {
            CVehicleHelper* pHelper = (CVehicleHelper*)(it->first);

            if (pHelper->IsFromGeometry())
            {
                DeleteTreeItem(it++);
                pHelper->SetHidden(true);
                found = true;
            }
        }

        if (!found)
        {
            ++it;
        }
    }

    GetIEditor()->SetModifiedFlag();
}

//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::FillSeats()
{
    CUndoSuspend susp;
    // get seats, create objs, add to treeview
    QTreeWidgetItem* hRoot = m_hVehicle;

    IVariable* pData = GetVariable();

    IVariable* pSeatsVar = GetChildVar(pData, "Seats", false);

    if (pSeatsVar)
    {
        for (int i = 0; i < pSeatsVar->GetNumVariables(); ++i)
        {
            IVariable* pSeatVar = pSeatsVar->GetVariable(i);

            // adds missing child vars
            CVehicleData::FillDefaults(pSeatVar, "Seat", GetChildVar(CVehicleData::GetDefaultVar(), "Seats"));

            // create new seat object
            CVehicleSeat* pSeatObj = (CVehicleSeat*)GetIEditor()->GetObjectManager()->NewObject("VehicleSeat");

            if (!pSeatObj)
            {
                continue;
            }

            pSeatObj->SetHidden(true);
            pSeatObj->SetVehicle(m_pVehicle);
            pSeatObj->SetVariable(pSeatVar);

            QTreeWidgetItem* hParentItem = nullptr;

            // attach to vehicle or parent part, if present
            bool bPart = false;

            if (IVariable* pPartVar = GetChildVar(pSeatVar, "part"))
            {
                QString sPart;
                pPartVar->Get(sPart);
                CVehiclePart* pPartObj = FindPart(sPart);

                if (pPartObj)
                {
                    VeedLog("Attaching Seat to part %s", sPart);
                    pPartObj->AttachChild(pSeatObj);
                    hParentItem = stl::find_in_map(m_partToTree, pPartObj, STreeItem()).item;
                    bPart = true;
                }
            }

            if (!bPart)
            {
                // fallback
                VeedLog("no part found for seat, attaching to vehicle");
                m_pVehicle->AttachChild(pSeatObj);
                hParentItem = m_hVehicle;
            }

            assert(hParentItem);
            QTreeWidgetItem* hItem = InsertTreeItem(pSeatObj, hParentItem);
            pSeatObj->AddEventListener(functor(*this, &CVehiclePartsPanel::OnObjectEvent));

            // create weapon objects
            if (IVariable* pWeapons = GetChildVar(pSeatVar, "Weapons", true))
            {
                if (IVariable* pPrim = GetChildVar(pWeapons, "Primary"))
                {
                    for (int i = 0; i < pPrim->GetNumVariables(); ++i)
                    {
                        CreateWeapon(WEAPON_PRIMARY, pSeatObj, pPrim->GetVariable(i));
                    }
                }

                if (IVariable* pSec = GetChildVar(pWeapons, "Secondary"))
                {
                    for (int i = 0; i < pSec->GetNumVariables(); ++i)
                    {
                        CreateWeapon(WEAPON_SECONDARY, pSeatObj, pSec->GetVariable(i));
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
CVehicleWeapon* CVehiclePartsPanel::CreateWeapon(int weaponType, CVehicleSeat* pSeat, IVariable* pVar /*=0*/)
{
    CUndoSuspend susp;

    IObjectManager* pObjMan = GetIEditor()->GetObjectManager();
    CVehicleWeapon* pObj = (CVehicleWeapon*)pObjMan->NewObject("VehicleWeapon");

    if (pObj)
    {
        pSeat->AddWeapon(weaponType, pObj, pVar);
        pObj->SetHidden(true);

        InsertTreeItem(pObj, pSeat);

        ReloadPropsCtrl();

        pObj->AddEventListener(functor(*this, &CVehiclePartsPanel::OnObjectEvent));
        return pObj;
    }

    return NULL;
}


//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::DeleteSeats()
{
    DeleteTreeObjects<CVehicleSeat*>();
}

//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::DumpPartTreeMap()
{
    int i = 0;
    VeedLog("PartToTreeMap is:");

    for (TPartToTreeMap::iterator it = m_partToTree.begin(); it != m_partToTree.end(); ++it)
    {
        VeedLog("[%i] %s", i++, (*it).first->GetName());
    }
}

//////////////////////////////////////////////////////////////////////////////
CVehiclePart* CVehiclePartsPanel::FindPart(const QString& name)
{
    for (TPartToTreeMap::iterator it = m_partToTree.begin(); it != m_partToTree.end(); ++it)
    {
        CBaseObject* pObj = (*it).first;

        if (qobject_cast<CVehiclePart*>(pObj) && pObj->GetName() == name)
        {
            return (CVehiclePart*)pObj;
        }
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////////
CVehiclePart* CVehiclePartsPanel::GetMainPart()
{
    for (TPartToTreeMap::iterator it = m_partToTree.begin(); it != m_partToTree.end(); ++it)
    {
        if (qobject_cast<CVehiclePart*>((*it).first) && ((CVehiclePart*)it->first)->IsMainPart())
        {
            return (CVehiclePart*)it->first;
        }
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::FillComps(IVariablePtr pData)
{
    if (!m_pVehicle)
    {
        return;
    }

    CUndoSuspend susp;

    QTreeWidgetItem* hRoot = m_hVehicle;

    IVariable* pComps = GetChildVar(pData, "Components");

    if (pComps)
    {
        int numChildren = pComps->GetNumVariables();

        for (int i = 0; i < numChildren; ++i)
        {
            IVariable* pVar = pComps->GetVariable(i);

            // adds missing child vars
            CVehicleData::FillDefaults(pVar, "Component", GetChildVar(CVehicleData::GetDefaultVar(), "Components"));

            // create new comp object
            CVehicleComponent* pComp = (CVehicleComponent*)GetIEditor()->GetObjectManager()->NewObject("VehicleComponent");

            if (!pComp)
            {
                continue;
            }

            pComp->SetVehicle(m_pVehicle);

            m_pVehicle->AttachChild(pComp);
            pComp->SetVariable(pVar);

            QTreeWidgetItem* hItem = InsertTreeItem(pComp, hRoot);
            pComp->AddEventListener(functor(*this, &CVehiclePartsPanel::OnObjectEvent));
        }
    }

    ShowComps(m_ui->m_bComps->isChecked());
}


//////////////////////////////////////////////////////////////////////////////
int CVehiclePartsPanel::FillParts(IVariablePtr pData)
{
    CUndoSuspend susp;

    if (!m_pVehicle)
    {
        return 0;
    }

    QTreeWidgetItem* hRoot = m_hVehicle;

    IVariable* pParts = GetChildVar(pData, "Parts", false);

    if (pParts && pParts->GetNumVariables() > 0)
    {
        if (IVariable* pFirstPart = pParts->GetVariable(0))
        {
            // mark first part as main
            IVariable* pVar = GetChildVar(pFirstPart, "mainPart", false);

            if (!pVar)
            {
                pVar = new CVariable<bool>();
                pVar->SetName("mainPart");
                pFirstPart->AddVariable(pVar);
            }

            pVar->Set(true);
        }

        AddParts(pParts, m_pVehicle);
    }

    // show/hide stuff according to display settings
    ShowWheels(m_ui->m_bWheels->isChecked());

    //GetIEditor()->SetModifiedFlag();
    hRoot->sortChildren(0, Qt::AscendingOrder);

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::AddParts(IVariable* pParts, CBaseObject* pParent)
{
    assert(pParts);

    if (!pParts || !pParent)
    {
        Warning("[CVehiclePartsPanel::AddParts] ERROR: called with NULL pointer!");
        return;
    }

    for (int i = 0; i < pParts->GetNumVariables(); ++i)
    {
        IVariable* pPartVar = pParts->GetVariable(i);

        if (!pPartVar)
        {
            Error("[CVehiclePartsPanel::AddParts] ERROR: pPartVar is NULL!");
        }

        // skip unnamed parts
        if (0 == GetChildVar(pPartVar, "name", false))
        {
            continue;
        }

        // create new part object
        CVehiclePart* pPartObj = (CVehiclePart*)GetIEditor()->GetObjectManager()->NewObject("VehiclePart");

        if (!pPartObj)
        {
            continue;
        }

        pPartObj->SetWorldTM(m_pVehicle->GetWorldTM());
        //pPartObj->SetHidden(true);
        pPartObj->SetVehicle(m_pVehicle);
        pPartObj->SetVariable(pPartVar);

        QTreeWidgetItem* hParentItem = nullptr;

        // attach to vehicle or parent part, if present
        if (pParent == m_pVehicle)
        {
            m_pVehicle->AttachChild(pPartObj);
            hParentItem = m_hVehicle;
        }
        else
        {
            VeedLog("Attaching part %s to parent %s", pPartObj->GetName(), pParent->GetName());
            pParent->AttachChild(pPartObj);
            hParentItem = stl::find_in_map(m_partToTree, pParent, STreeItem()).item;
            assert(hParentItem);
        }


        if (IVariable* pMainVar = GetChildVar(pPartVar, "mainPart", false))
        {
            pPartObj->SetMainPart(true);
            m_pMainPart = pPartObj;
        }

        QTreeWidgetItem* hNewItem = new VehicleTreeWidgetItem(pPartObj, hParentItem);
        m_partToTree[pPartObj] = STreeItem(hNewItem, hParentItem);

        pPartObj->AddEventListener(functor(*this, &CVehiclePartsPanel::OnObjectEvent));

        // check if the part has a child parts table
        if (IVariable* pChildParts = GetChildVar(pPartVar, pParts->GetName().toUtf8().data(), false))
        {
            AddParts(pChildParts, pPartObj); // add children
        }

        // create the helper objects for this part
        if (IVariable* pHelpers = GetChildVar(pPartVar, "Helpers"))
        {
            int nOrigHelpers = pHelpers->GetNumVariables();

            for (int h = 0; h < nOrigHelpers; ++h)
            {
                IVariable* pHelper = pHelpers->GetVariable(h);
                IVariable* pName = GetChildVar(pHelper, "name");
                IVariable* pPos = GetChildVar(pHelper, "position");

                if (pName && pPos)
                {
                    QString name("UNKNOWN");
                    Vec3 pos;
                    pName->Get(name);
                    pPos->Get(pos);

                    if (IEntity* pEnt = m_pVehicle->GetCEntity()->GetIEntity())
                    {
                        pos = pEnt->GetWorldTM().TransformPoint(pos);
                    }

                    Vec3 dir(FORWARD_DIRECTION);

                    if (IVariable* pDir = GetChildVar(pHelper, "direction"))
                    {
                        pDir->Get(dir);
                    }

                    CreateHelperObject(pos, dir, name, NOT_UNIQUE, NOT_SELECTED, NOT_EDIT_LABEL, FROM_EDITOR, pHelper);
                }
            }
        }

        // sort children of new part
        hNewItem->sortChildren(0, Qt::AscendingOrder);
    }
}

//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::FillHelpers(IVariablePtr pData)
{
    IVariable* pHelpers = GetChildVar(pData, "Helpers", false);

    if (pHelpers && pHelpers->GetNumVariables() > 0)
    {
        for (int i = 0; i < pHelpers->GetNumVariables(); ++i)
        {
            IVariable* pHelperVar = pHelpers->GetVariable(i);

            if (!pHelperVar)
            {
                Error("[CVehiclePartsPanel::AddHelpers] ERROR: pHelperVar is NULL!");
            }

            IVariable* pName = GetChildVar(pHelperVar, "name");
            IVariable* pPos = GetChildVar(pHelperVar, "position");

            if (pName && pPos)
            {
                QString name("UNKNOWN");
                Vec3 pos;
                pName->Get(name);
                pPos->Get(pos);

                if (IEntity* pEnt = m_pVehicle->GetCEntity()->GetIEntity())
                {
                    pos = pEnt->GetWorldTM().TransformPoint(pos);
                }

                Vec3 dir(FORWARD_DIRECTION);

                if (IVariable* pDir = GetChildVar(pHelperVar, "direction"))
                {
                    pDir->Get(dir);
                }

                CreateHelperObject(pos, dir, name, NOT_UNIQUE, NOT_SELECTED, NOT_EDIT_LABEL, FROM_EDITOR, pHelperVar);
            }
        }
    }

    ShowVeedHelpers(m_ui->m_bVeedHelpers->isChecked());
    ShowAssetHelpers(m_ui->m_bAssetHelpers->isChecked());
}

//////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::OnPaneClose()
{
    ShowAssetHelpers(false);
}

//////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::OnObjectEvent(CBaseObject* object, int event)
{
    if (object == m_pVehicle)
    {
        if (event == CBaseObject::ON_DELETE)
        {
            m_ui->m_treeCtrl->clear();
            m_ui->m_propsCtrl->RemoveAllItems();
        }

        return;
    }

    if (event == CBaseObject::ON_RENAME)
    {
        if (QTreeWidgetItem* hItem = (stl::find_in_map(m_partToTree, object, STreeItem())).item)
        {
            hItem->setText(0, object->GetName());
        }
    }
    else if (event == CBaseObject::ON_DELETE)
    {
        // remove from tree
        if (QTreeWidgetItem* hItem = (stl::find_in_map(m_partToTree, object, STreeItem())).item)
        {
            delete hItem;
        }

        m_partToTree.erase(object);
    }
    else if (event == CBaseObject::ON_SELECT)
    {
        if (QTreeWidgetItem* hItem = (stl::find_in_map(m_partToTree, object, STreeItem())).item)
        {
            m_ui->m_treeCtrl->setCurrentItem(hItem);
            m_pSelItem = object;
        }
    }
    else if (event == CBaseObject::ON_CHILDATTACHED)
    {
        // check if we created this child ourself (then we know it already)
        CBaseObject* pChild = object->GetChild(object->GetChildCount() - 1);

        if (m_partToTree.find(pChild) == m_partToTree.end())
        {
            // currently only helpers need to be handled
            if (qobject_cast<CVehiclePart*>(object) && qobject_cast<CVehicleHelper*>(pChild))
            {
                QTreeWidgetItem* hItem = InsertTreeItem(pChild, object, false);
                pChild->AddEventListener(functor(*this, &CVehiclePartsPanel::OnObjectEvent));
            }

            UpdateVariables();
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::closeEvent(QCloseEvent* event)
{
    OnPaneClose();
    QWidget::closeEvent(event);
}


//////////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::ReloadPropsCtrl()
{
    if (m_ui->m_propsCtrl->GetRootItem() && m_ui->m_propsCtrl->GetRootItem()->GetChildCount() > 0)
    {
        if (IVariablePtr pVar = m_ui->m_propsCtrl->GetRootItem()->GetChild(0)->GetVariable())
        {
            m_ui->m_propsCtrl->RemoveAllItems();
            CVarBlock* block = new CVarBlock();
            block->AddVariable(pVar);
            m_ui->m_propsCtrl->AddVarBlock(block);
            m_ui->m_propsCtrl->ExpandAll();
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::OnSetPartClass(IVariable* pVar)
{
}

//////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::OnEditWheelMaster()
{
    IVariable* pMaster = GetOrCreateChildVar(GetVariable(), "WheelMaster");
    GetOrCreateChildVar(pMaster, "SubPartWheel", true, true);

    CWheelMasterDialog dlg(pMaster, this);

    // gather wheel variables
    std::vector<CVehiclePart*> wheels;
    GetWheels(wheels);
    dlg.SetWheels(&wheels);

    if (dlg.exec() == QDialog::Accepted)
    {
        IVariable* pOrigMaster = pMaster->GetVariable(0);
        IVariable* pEditMaster = dlg.GetVariable()->GetVariable(0);

        // save master
        ReplaceChildVars(pEditMaster, pOrigMaster);
    }
}

//////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::GetWheels(std::vector<CVehiclePart*>& wheels)
{
    wheels.clear();

    for (TPartToTreeMap::iterator it = m_partToTree.begin(); it != m_partToTree.end(); ++it)
    {
        if (qobject_cast<CVehiclePart*>(it->first))
        {
            CVehiclePart* pPart = (CVehiclePart*)it->first;

            if (pPart->GetPartClass() == PARTCLASS_WHEEL)
            {
                wheels.push_back(pPart);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::NotifyObjectsDeletion(CVehiclePrototype* pProt)
{
    for (TPartToTreeMap::iterator it = m_partToTree.begin(); it != m_partToTree.end(); ++it)
    {
        (*it).first->RemoveEventListener(functor(*this, &CVehiclePartsPanel::OnObjectEvent));
    }
}


//////////////////////////////////////////////////////////////////////////
void CVehiclePartsPanel::OnScaleHelpers()
{
    CVehicleScaleDialog dlg(this);
    float scale = 0.f;

    if (dlg.exec() != QDialog::Accepted)
    {
        return;
    }

    scale = dlg.GetScale();

    if (scale <= 0.f)
    {
        CryLog("[VehiclePartsPanel]: scale %f invalid", scale);
        return;
    }

    for (TPartToTreeMap::iterator it = m_partToTree.begin(); it != m_partToTree.end(); ++it)
    {
        IVeedObject* pObj = IVeedObject::GetVeedObject(it->first);

        if (pObj)
        {
            pObj->UpdateScale(scale);
        }
    }
}


#include <Vehicles/VehiclePartsPanel.moc>
