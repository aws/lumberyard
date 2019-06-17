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
#include "SelectObjectDlg.h"
#include "ObjectSelectorTableView.h"
#include "ObjectSelectorModel.h"
#include <ui_SelectObjectDlg.h>

#include "Objects/PrefabObject.h"
#include "Objects/EntityObject.h"
#include "Objects/BrushObject.h"
#include "Objects/GeomEntity.h"
#include "DisplaySettings.h"
#include "ViewManager.h"
#include "Material/Material.h"
#include "LinkTool.h"
#include "HyperGraph/FlowGraphHelpers.h"
#include "HyperGraph/FlowGraphManager.h"
#include "HyperGraph/FlowGraph.h"
#include "HyperGraph/FlowGraphNode.h"
#include "Objects/ObjectLayerManager.h"
#include "TrackView/TrackViewSequence.h"
#include "UserMessageDefines.h"
#include "TrackView/TrackViewSequenceManager.h"

#include <QRadioButton>
#include <QList>
#include <QCheckBox>
#include <QLabel>
#include <QMessageBox>
#include <QDebug>


#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>

#ifndef MAXUINT
#define MAXUINT ((UINT) ~((UINT)0))
#endif

#ifndef MAXINT
#define MAXINT ((INT)(MAXUINT >> 1))
#endif

namespace
{
    // Local class to ensure, in the destructor, that any model
    // items whose selection states have changed get updated in the UI
    class SelectObjectModelSynchronizer
    {
    public:
        SelectObjectModelSynchronizer(ObjectSelectorModel* model) : m_model(model)
        {
            auto objects = m_model->GetObjects();
            for (CBaseObject* obj : objects)
            {
                if (obj->IsSelected())
                {
                    m_selectedObjects.insert(obj);
                }
            }
        }

        ~SelectObjectModelSynchronizer()
        {
            // toggle the visual for any that changed
            auto objects = m_model->GetObjects();
            for (CBaseObject* obj : objects)
            {
                bool selectedNow = obj->IsSelected();
                bool selectedThen = m_selectedObjects.find(obj) != m_selectedObjects.end();

                if (selectedNow != selectedThen)
                {
                    m_model->EmitDataChanged(obj);
                }
            }
        }

    private:
        ObjectSelectorModel* m_model;
        AZStd::set<CBaseObject*> m_selectedObjects;
    };
}

CSelectObjectDlg* CSelectObjectDlg::m_instance = nullptr;
CSelectObjectDlg::CSelectObjectDlg(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::SELECT_OBJECT)
    , m_model(new ObjectSelectorModel(this))
{
    CEditTool* pTool = GetIEditor()->GetEditTool();
    m_bIsLinkTool = pTool && qobject_cast<CLinkTool*>(pTool);
    m_model->SetIsLinkTool(m_bIsLinkTool);

    ui->setupUi(this);
    ui->tableView->setModel(m_model.data());

    connect(ui->displayModeVisible, &QRadioButton::toggled, this, &CSelectObjectDlg::UpdateDisplayMode);
    connect(ui->displayModeFrozen, &QRadioButton::toggled, this, &CSelectObjectDlg::UpdateDisplayMode);
    connect(ui->displayModeHidden, &QRadioButton::toggled, this, &CSelectObjectDlg::UpdateDisplayMode);

    connect(ui->entityType, &QCheckBox::stateChanged, this, &CSelectObjectDlg::UpdateObjectMask);
    connect(ui->brushType, &QCheckBox::stateChanged, this, &CSelectObjectDlg::UpdateObjectMask);
    connect(ui->prefabType, &QCheckBox::stateChanged, this, &CSelectObjectDlg::UpdateObjectMask);
    connect(ui->tagpointType, &QCheckBox::stateChanged, this, &CSelectObjectDlg::UpdateObjectMask);
    connect(ui->aiPointType, &QCheckBox::stateChanged, this, &CSelectObjectDlg::UpdateObjectMask);
    connect(ui->groupType, &QCheckBox::stateChanged, this, &CSelectObjectDlg::UpdateObjectMask);
    connect(ui->volumeType, &QCheckBox::stateChanged, this, &CSelectObjectDlg::UpdateObjectMask);
    connect(ui->shapesType, &QCheckBox::stateChanged, this, &CSelectObjectDlg::UpdateObjectMask);
    connect(ui->otherType, &QCheckBox::stateChanged, this, &CSelectObjectDlg::UpdateObjectMask);
    connect(ui->solidType, &QCheckBox::stateChanged, this, &CSelectObjectDlg::UpdateObjectMask);

    connect(ui->select, &QPushButton::clicked, this, &CSelectObjectDlg::Select);
    connect(ui->selectAll, &QPushButton::clicked, this, &CSelectObjectDlg::SelectAll);
    connect(ui->selectNone, &QPushButton::clicked, this, &CSelectObjectDlg::SelectNone);
    connect(ui->invertSelection, &QPushButton::clicked, this, &CSelectObjectDlg::InvertSelection);

    connect(ui->selectAllTypes, &QPushButton::clicked, this, &CSelectObjectDlg::SelectAllTypes);
    connect(ui->selectNoneTypes, &QPushButton::clicked, this, &CSelectObjectDlg::SelectNoneTypes);
    connect(ui->invertTypes, &QPushButton::clicked, this, &CSelectObjectDlg::InvertTypes);

    connect(ui->hideSelected, &QPushButton::clicked, this, &CSelectObjectDlg::HideSelected);
    connect(ui->freezeSelected, &QPushButton::clicked, this, &CSelectObjectDlg::FreezeSelected);
    connect(ui->deleteSelected, &QPushButton::clicked, this, &CSelectObjectDlg::DeleteSelected);

    connect(ui->refresh, &QPushButton::clicked, this, &CSelectObjectDlg::Refresh);
    connect(ui->filterByValue, &QPushButton::clicked, this, &CSelectObjectDlg::FilterByValue);
    connect(ui->filterByName, &QPushButton::clicked, this, &CSelectObjectDlg::FilterByName);

    connect(ui->searchAllObjects, &QCheckBox::stateChanged, this, &CSelectObjectDlg::SearchAllObjectsToggled);
    connect(ui->autoSelect, &QCheckBox::stateChanged, this, &CSelectObjectDlg::AutoSelectToggled);
    connect(ui->displayAsTree, &QCheckBox::stateChanged, this, &CSelectObjectDlg::DisplayAsTreeToggled);

    connect(ui->fastSelectLineEdit, &QLineEdit::textChanged, this, &CSelectObjectDlg::FastSelectFilterChanged);
    connect(ui->propertyLineEdit, &QLineEdit::textChanged, this, &CSelectObjectDlg::FastSelectFilterChanged);
    connect(m_model.data(), &ObjectSelectorModel::countChanged, this, &CSelectObjectDlg::UpdateCountLabel);

    connect(ui->tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CSelectObjectDlg::OnVisualSelectionChanged);
    connect(ui->tableView, &QTableView::doubleClicked, this, &CSelectObjectDlg::OnDoubleClick);

    UpdateObjectMask();
    UpdateDisplayMode();
    UpdateCountLabel();
    GetIEditor()->GetFlowGraphManager()->AddListener(this);
    GetIEditor()->GetObjectManager()->GetLayersManager()->AddUpdateListener(functor(*this, &CSelectObjectDlg::OnLayerUpdate));
    GetIEditor()->RegisterNotifyListener(this);
    GetIEditor()->GetObjectManager()->AddObjectEventListener(functor(*this, &CSelectObjectDlg::OnObjectEvent));
    connect(&m_invalidateTimer, &QTimer::timeout, this, &CSelectObjectDlg::InvalidateFilter);
    m_invalidateTimer.setSingleShot(true);
}

CSelectObjectDlg::~CSelectObjectDlg()
{
    GetIEditor()->UnregisterNotifyListener(this);
    GetIEditor()->GetFlowGraphManager()->RemoveListener(this);
    GetIEditor()->GetObjectManager()->RemoveObjectEventListener(functor(*this, &CSelectObjectDlg::OnObjectEvent));
    m_instance = nullptr;
}

void CSelectObjectDlg::UpdateDisplayMode()
{
    if (ui->displayModeVisible->isChecked())
    {
        m_model->SetDisplayMode(ObjectSelectorModel::DisplayModeVisible);
        ui->hideSelected->setText(tr("Hide"));
        ui->freezeSelected->setText(tr("Freeze"));
        ui->deleteSelected->setText(tr("Delete Selected"));
    }
    else if (ui->displayModeFrozen->isChecked())
    {
        m_model->SetDisplayMode(ObjectSelectorModel::DisplayModeFrozen);
        ui->hideSelected->setText(tr("Hide"));
        ui->freezeSelected->setText(tr("Unfreeze"));
        ui->deleteSelected->setText(tr("Delete Selected"));
    }
    else if (ui->displayModeHidden->isChecked())
    {
        m_model->SetDisplayMode(ObjectSelectorModel::DisplayModeHidden);
        ui->hideSelected->setText(tr("Unhide"));
        ui->freezeSelected->setText(tr("Freeze"));
        ui->deleteSelected->setText(tr("Delete Selected"));
    }
}

QList<QCheckBox*> CSelectObjectDlg::ObjectTypeCheckBoxes() const
{
    static QList<QCheckBox*> list;
    if (list.isEmpty())
    {
        list << ui->entityType  << ui->brushType << ui->prefabType << ui->tagpointType
        << ui->aiPointType << ui->groupType << ui->volumeType << ui->shapesType
        << ui->otherType  << ui->solidType;
    }

    return list;
}

void CSelectObjectDlg::UpdateObjectMask()
{
    if (m_disableInvalidateFilter) // Performance optimization
    {
        return;
    }

    int mask = 0;
    if (ui->entityType->isChecked())
    {
        mask |= OBJTYPE_ENTITY;
    }

    if (ui->brushType->isChecked())
    {
        mask |= OBJTYPE_BRUSH;
    }

    if (ui->prefabType->isChecked())
    {
        mask |= OBJTYPE_PREFAB;
    }

    if (ui->tagpointType->isChecked())
    {
        mask |= OBJTYPE_TAGPOINT;
    }

    if (ui->aiPointType->isChecked())
    {
        mask |= OBJTYPE_AIPOINT;
    }

    if (ui->groupType->isChecked())
    {
        mask |= OBJTYPE_GROUP;
    }

    if (ui->volumeType->isChecked())
    {
        mask |= OBJTYPE_VOLUME;
    }

    if (ui->shapesType->isChecked())
    {
        mask |= OBJTYPE_SHAPE;
    }

    if (ui->otherType->isChecked())
    {
        mask |= ~(OBJTYPE_ANY_DEFINED);
    }

    if (ui->solidType->isChecked())
    {
        mask |= OBJTYPE_SOLID;
    }

    m_model->SetObjectTypeMask(mask);
}

void CSelectObjectDlg::InvertSelection()
{
    SelectObjectModelSynchronizer stateSynchronizer(m_model.data());

    m_bIgnoreCallbacks = true;
    m_ignoreSelectionChanged = true; // Performance optimization
    auto objects = m_model->GetObjects();
    const int size = objects.size();
    ui->tableView->selectionModel()->clear();
    for (size_t i = 0; i < size; ++i)
    {
        QModelIndex index = m_model->index(i, 0);
        const bool objIsSelected = objects[i]->IsSelected();
        if (!objIsSelected)
        {
            ui->tableView->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }
        AutoSelectItemObject(i);
    }

    if (!m_picking && !m_bAutoselect)
    {
        ApplyListSelectionToObjectManager();
    }
    ui->tableView->setFocus(Qt::OtherFocusReason);
    m_bIgnoreCallbacks = false;
    m_ignoreSelectionChanged = false;
    UpdateCountLabel();
    UpdateButtonsEnabledState();
}

void CSelectObjectDlg::SelectAll()
{
    SelectObjectModelSynchronizer stateSynchronizer(m_model.data());

    auto objects = m_model->GetObjects();
    const int size = objects.size();
    if (size > 0)
    {
        m_ignoreSelectionChanged = true; // Performance optimization
        QModelIndex first = m_model->index(0, 0);
        QModelIndex last = m_model->index(size - 1, ObjectSelectorModel::NumberOfColumns - 1);
        ui->tableView->selectionModel()->select(QItemSelection(first, last), QItemSelectionModel::Select);
        m_bIgnoreCallbacks = true;

        for (size_t i = 0; i < size; ++i)
        {
            AutoSelectItemObject(i);
        }
        if (!m_picking && !m_bAutoselect)
        {
            ApplyListSelectionToObjectManager();
        }
        m_bIgnoreCallbacks = false;
        ui->tableView->setFocus(Qt::OtherFocusReason);
        m_ignoreSelectionChanged = false;
        UpdateButtonsEnabledState();
        UpdateCountLabel();
    }
}

void CSelectObjectDlg::ApplyListSelectionToObjectManager()
{
    m_bIgnoreCallbacks = true;

    if (m_picking || !m_bAutoselect || m_bIsLinkTool)
    {
        if (!m_picking && !m_bIsLinkTool)
        {
            GetIEditor()->ClearSelection();
        }

        auto objects = m_model->GetObjects();
        const int size = objects.size();
        for (size_t i = 0; i < size; ++i)
        {
            if (isRowSelected(i))
            {
                CBaseObject* obj = objects[i];

                if (m_bIsLinkTool)
                {
                    CEditTool* pTool = GetIEditor()->GetEditTool();
                    if (pTool && qobject_cast<CLinkTool*>(pTool))
                    {
                        ((CLinkTool*)pTool)->LinkSelectedToParent(obj);
                        GetIEditor()->SetEditTool(0);
                        break;
                    }
                }
                else
                {
                    // Cancel any active tool (terrain modification, etc.) & return to select/move/etc.
                    CCryEditApp::instance()->OnEditEscape();

                    CPrefabObject* pPrefabObject = obj->GetPrefab();
                    bool bIsClosedPrefab = (pPrefabObject && !pPrefabObject->IsOpen());

                    GetIEditor()->GetObjectManager()->SelectObject(bIsClosedPrefab ? pPrefabObject : obj);
                    if (m_picking)
                    {
                        break;
                    }
                }
            }
        }
    }
    m_bIgnoreCallbacks = false;
}

void CSelectObjectDlg::AutoSelectItemObject(int iItem)
{
    if (!m_bAutoselect || iItem < 0)
    {
        return;
    }

    if (m_picking || m_model->GetDisplayMode() != ObjectSelectorModel::DisplayModeVisible)
    {
        return;
    }
    m_bIgnoreCallbacks = true;

    auto objects = m_model->GetObjects();
    if (iItem >= objects.size())
    {
        return;
    }

    CBaseObject* pObject = objects[iItem];
    if (pObject)
    {
        if (isRowSelected(iItem))
        {
            CGroup* pParentGroup = pObject->GetGroup();

            if (pParentGroup)
            {
                if (!pParentGroup->IsOpen())
                {
                    GetIEditor()->GetObjectManager()->SelectObject(pParentGroup);
                }
                else
                {
                    GetIEditor()->GetObjectManager()->SelectObject(pObject);
                }
            }
            else
            {
                GetIEditor()->GetObjectManager()->SelectObject(pObject);
            }
        }
        else
        {
            GetIEditor()->GetObjectManager()->UnselectObject(pObject);
        }
    }
    m_bIgnoreCallbacks = false;
}

void CSelectObjectDlg::SelectNone()
{
    SelectObjectModelSynchronizer stateSynchronizer(m_model.data());

    m_ignoreSelectionChanged = true; // Performance optimization
    // Select here means the "Select" object property, not table view selection
    auto objects = m_model->GetObjects();
    const auto count = objects.size();
    if (count > 0)   // For some reason, SelectNone visually selects all items in the table
    {
        QModelIndex first = m_model->index(0, 0);
        QModelIndex last = m_model->index(count - 1, ObjectSelectorModel::NumberOfColumns - 1);
        ui->tableView->selectionModel()->select(QItemSelection(first, last), QItemSelectionModel::Select);
    }

    m_bIgnoreCallbacks = true;

    for (size_t i = 0; i < count; ++i)
    {
        AutoSelectItemObject(i);
    }

    if (!m_picking && !m_bAutoselect)
    {
        ApplyListSelectionToObjectManager();
    }

    m_bIgnoreCallbacks = false;
    GetIEditor()->ClearSelection();
    GetIEditor()->SetEditTool(nullptr);
    ui->tableView->setFocus(Qt::OtherFocusReason);
    UpdateCountLabel();
    m_ignoreSelectionChanged = false;
    UpdateButtonsEnabledState();
}

void CSelectObjectDlg::Select()
{
    SelectObjectModelSynchronizer stateSynchronizer(m_model.data());

    ApplyListSelectionToObjectManager();
    UpdateCountLabel();
}

void CSelectObjectDlg::SelectAllTypes()
{
    m_disableInvalidateFilter = true; // Performance optimization
    auto checkBoxes = ObjectTypeCheckBoxes();
    bool changeWasMade = false; // Performance optimization
    for (QCheckBox* b : checkBoxes)
    {
        if (!b->isChecked())
        {
            changeWasMade = true;
            b->setChecked(true);
        }
    }
    m_disableInvalidateFilter = false;

    if (changeWasMade)
    {
        UpdateObjectMask();
    }
}

void CSelectObjectDlg::SelectNoneTypes()
{
    m_disableInvalidateFilter = true; // Performance optimization
    auto checkBoxes = ObjectTypeCheckBoxes();
    bool changeWasMade = false; // Performance optimization
    for (QCheckBox* b : checkBoxes)
    {
        if (b->isChecked())
        {
            changeWasMade = true;
            b->setChecked(false);
        }
    }
    m_disableInvalidateFilter = false;
    if (changeWasMade)
    {
        UpdateObjectMask();
    }
}

void CSelectObjectDlg::InvertTypes()
{
    m_disableInvalidateFilter = true; // Performance optimization
    auto checkBoxes = ObjectTypeCheckBoxes();
    for (QCheckBox* b : checkBoxes)
    {
        b->setChecked(!b->isChecked());
    }
    m_disableInvalidateFilter = false;
    UpdateObjectMask();
}

void CSelectObjectDlg::HideSelected()
{
    auto objects = m_model->GetObjects();
    const auto count = objects.size();
    for (size_t i = 0; i < count; ++i)
    {
        if (isRowSelected(i))
        {
            CBaseObject* obj = objects[i];
            const bool hiddenMode = m_model->GetDisplayMode() == ObjectSelectorModel::DisplayModeHidden;
            GetIEditor()->GetObjectManager()->HideObject(obj, !hiddenMode);
            m_model->EmitDataChanged(obj);
        }
    }
}

void CSelectObjectDlg::FreezeSelected()
{
    // Freeze/Unfreeze selected objects.
    auto objects = m_model->GetObjects();
    const auto count = objects.size();
    for (size_t i = 0; i < count; ++i)
    {
        if (isRowSelected(i))
        {
            CBaseObject* obj = objects[i];
            const bool frozenMode = m_model->GetDisplayMode() == ObjectSelectorModel::DisplayModeFrozen;
            GetIEditor()->GetObjectManager()->FreezeObject(obj, !frozenMode);
            m_model->EmitDataChanged(obj);
        }
    }
}

void CSelectObjectDlg::DeleteSelected()
{
    QMessageBox box(this);
    box.setText(tr("Are you sure you want to delete the selected objects?"));
    box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    if (box.exec() != QMessageBox::Yes)
    {
        return;
    }

    // Navigation triggered - Button Click "Delete Selected" in Object Selector
    AzToolsFramework::EditorMetricsEventsBusAction editorMetricsEventsBusActionWrapper(AzToolsFramework::EditorMetricsEventsBusTraits::NavigationTrigger::ButtonClick);

    auto objects = m_model->GetObjects();
    const auto count = objects.size();
    std::vector<CBaseObject*> cDeleteList;

    cDeleteList.reserve(count);

    // Builds the list of objects to delete...
    for (size_t i = 0; i < count; ++i)
    {
        if (isRowSelected(i))
        {
            cDeleteList.push_back(objects[i]);
        }
    }

    // Deletes the objects.
    GetIEditor()->BeginUndo();
    for (CBaseObject* obj : cDeleteList)
    {
        GetIEditor()->GetObjectManager()->DeleteObject(obj);
    }
    GetIEditor()->AcceptUndo("Delete Objects");
    GetIEditor()->SetModifiedFlag();
    GetIEditor()->SetModifiedModule(eModifiedBrushes);
}

void CSelectObjectDlg::Refresh()
{
    m_model->Reload(true);
}

void CSelectObjectDlg::FilterByName()
{
    m_model->SetMatchPropertyName(true);
}

void CSelectObjectDlg::FilterByValue()
{
    m_model->SetMatchPropertyName(false);
}

void CSelectObjectDlg::SearchAllObjectsToggled()
{
    m_model->SetSearchInsideObjects(ui->searchAllObjects->isChecked());
}

void CSelectObjectDlg::AutoSelectToggled()
{
    m_bAutoselect = !m_bAutoselect;
}

void CSelectObjectDlg::DisplayAsTreeToggled()
{
    const bool enable = ui->displayAsTree->isChecked();
    ui->tableView->EnableTreeMode(enable);
    ui->tableView->setSortingEnabled(!enable);
    m_model->EnableTreeMode(enable);
}

void CSelectObjectDlg::FastSelectFilterChanged()
{
    if (m_model->rowCount() < 1000)
    {
        InvalidateFilter();
    }
    else
    {
        // Performance optimization
        m_invalidateTimer.start(650);
    }
}

void CSelectObjectDlg::InvalidateFilter()
{
    m_model->SetFilterText(ui->fastSelectLineEdit->text());
    m_model->SetProperties(ui->propertyLineEdit->text());
}

void CSelectObjectDlg::UpdateCountLabel()
{
    const auto objects = m_model->GetObjects();
    int numSelected = 0;
    for (auto obj : objects)
    {
        if (obj->IsSelected())
        {
            numSelected++;
        }
    }

    ui->countLabel->setText(tr("%1 Objects Listed, %2 Selected%3").arg(objects.size()).arg(numSelected).arg(m_model->IsTrackViewModified() ? QString(" [TrackView column may contain stale data!]") : QString()));
}

void CSelectObjectDlg::UpdateButtonsEnabledState()
{
    const bool hasSelection = ui->tableView->selectionModel()->hasSelection();
    ui->deleteSelected->setEnabled(hasSelection);
    ui->hideSelected->setEnabled(hasSelection);
    ui->freezeSelected->setEnabled(hasSelection);
}

void CSelectObjectDlg::OnVisualSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    if (m_ignoreSelectionChanged)
    {
        return;
    }

    UpdateButtonsEnabledState();
    if (m_bAutoselect)
    {
        foreach(const QModelIndex &index, selected.indexes()) 
        {
            CBaseObject* obj = index.data(ObjectSelectorModel::ObjectRole).value<CBaseObject*>();
            if (obj && !obj->IsSelected())
            {
                AutoSelectItemObject(index.row());
            }
        }

        if (QApplication::keyboardModifiers() & Qt::ControlModifier)
        {
            // Auto select only does auto unselect if ctrl key is pressed
            foreach(const QModelIndex &index, deselected.indexes()) 
            {
                CBaseObject* obj = index.data(ObjectSelectorModel::ObjectRole).value<CBaseObject*>();
                if (obj && obj->IsSelected())
                {
                    AutoSelectItemObject(index.row());
                }
            }
        }
    }
}

bool CSelectObjectDlg::isRowSelected(int row) const
{
    if (row < 0 || row >= m_model->rowCount())
    {
        return false;
    }

    return ui->tableView->selectionModel()->isRowSelected(row, QModelIndex());
}

void CSelectObjectDlg::OnDoubleClick(const QModelIndex& idx)
{
    if (!m_bAutoselect)
    {
        auto obj1 = idx.data(ObjectSelectorModel::ObjectRole).value<CBaseObject*>();
        if (obj1 && !obj1->IsSelected() && obj1->IsSelectable())
        {
            SelectObjectModelSynchronizer stateSynchronizer(m_model.data());

            ApplyListSelectionToObjectManager();

            // We have to call this explicitly because we ignore object events
            // when applying the list selection
            UpdateCountLabel();
        }
    }
}

void CSelectObjectDlg::setTrackViewModified(bool modified)
{
    if (m_model->IsTrackViewModified() != modified)
    {
        m_model->SetTrackViewModified(modified);
        UpdateCountLabel();
    }
}

void CSelectObjectDlg::OnEditorNotifyEvent(EEditorNotifyEvent ev)
{
    switch (ev)
    {
    case eNotify_OnBeginNewScene:
        m_model->Clear();
        break;
    case eNotify_OnCloseScene:
        m_model->Clear();
        break;
    case eNotify_OnBeginSceneOpen:
        m_bIgnoreCallbacks = true;
        break;
    case eNotify_OnEndSceneOpen:
        m_bIgnoreCallbacks = false;
        m_model->Reload(true);
        break;

    case eNotify_OnIdleUpdate:
        if (m_bLayerModified)
        {
            m_model->Reload(false);
            m_bLayerModified = false;
        }

        m_model->UpdateFlowGraphs();
        break;
    case eNotify_OnEditToolChange:
    {
        bool needRefill = false;
        CEditTool* pTool = GetIEditor()->GetEditTool();

        if (m_bIsLinkTool || m_picking)
        {
            ui->tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
            needRefill = true;
            ui->select->setText(tr("Select"));
        }

        m_bIsLinkTool = false;
        m_picking = GetIEditor()->IsPicking();

        if (pTool && qobject_cast<CLinkTool*>(pTool))
        {
            m_bIsLinkTool = true;
            setWindowTitle(tr("Select Parent Object"));
            ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
            needRefill = true;
            ui->select->setText(tr("Link"));
        }

        if (m_picking)
        {
            setWindowTitle(tr("Pick Object"));
            ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
            needRefill = true;
            ui->select->setText(tr("Pick"));
        }

        if (needRefill)
        {
            m_model->Reload(false);
        }
    }
    break;

    case eNotify_OnReloadTrackView:
        setTrackViewModified(true);
        break;
    }
}

void CSelectObjectDlg::OnHyperGraphManagerEvent(EHyperGraphEvent event, IHyperGraph* pGraph, IHyperNode* pINode)
{
    if (!m_bIgnoreCallbacks)
    {
        m_model->OnHyperGraphManagerEvent(event, pGraph, pINode);
    }
}

void CSelectObjectDlg::showEvent(QShowEvent* ev)
{
    if (!m_initialized)
    {
        m_initialized = true;
        m_picking = GetIEditor()->IsPicking();

        CEditTool* pTool = GetIEditor()->GetEditTool();
        if (pTool && qobject_cast<CLinkTool*>(pTool))
        {
            m_bIsLinkTool = true;
        }

        if (GetIEditor()->IsPicking())
        {
            setWindowTitle(tr("Pick Object"));
            ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        }
        else if (m_bIsLinkTool)
        {
            setWindowTitle(tr("Select Parent Object"));
            ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        }
        else
        {
            setWindowTitle(tr(LyViewPane::LegacyObjectSelector));
        }
        m_model->Reload(true);
    }

    QWidget::showEvent(ev);
}

void CSelectObjectDlg::OnLayerUpdate(int ev, CObjectLayer* pLayer)
{
    switch (ev)
    {
    case CObjectLayerManager::ON_LAYER_MODIFY:
        m_bLayerModified = true;
        break;
    }
}

CSelectObjectDlg* CSelectObjectDlg::GetInstance()
{
    if (!m_instance)
    {
        m_instance = new CSelectObjectDlg();
    }
    return m_instance;
}

void CSelectObjectDlg::OnObjectEvent(CBaseObject* pObject, int ev)
{
    if (m_bIgnoreCallbacks)
    {
        return;
    }

    switch (ev)
    {
    case CBaseObject::ON_ADD:
        m_model->AddObject(pObject, 0);
        break;
    case CBaseObject::ON_DELETE:
        m_model->RemoveObject(pObject);
        break;
    case CBaseObject::ON_RENAME:
        m_model->EmitDataChanged(pObject);
        break;
    case CBaseObject::ON_SELECT:
    {
        // select and ensure visible
        const int row = m_model->IndexOfObject(pObject);
        if (row != -1)
        {
            QModelIndex index = m_model->index(row, 0);
            ui->tableView->selectionModel()->select(index, QItemSelectionModel::Rows);
            ui->tableView->scrollTo(index);
            m_model->EmitDataChanged(index);
            UpdateCountLabel();
        }
        break;
    }
    case CBaseObject::ON_UNSELECT:
    {
        const int row = m_model->IndexOfObject(pObject);
        if (row != -1)
        {
            QModelIndex index = m_model->index(row, 0);
            ui->tableView->selectionModel()->select(index, QItemSelectionModel::Rows | QItemSelectionModel::Clear);
            m_model->EmitDataChanged(index);
            UpdateCountLabel();
        }
        break;
    }
    }
}

void CSelectObjectDlg::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions opts;
    opts.paneRect = QRect(0, 0, 750, 780);
    opts.shortcut = QKeySequence(Qt::CTRL + Qt::Key_T);
    opts.sendViewPaneNameBackToAmazonAnalyticsServers = true;
    opts.isLegacy = true;

    AzToolsFramework::RegisterViewPane<CSelectObjectDlg>(LyViewPane::LegacyObjectSelector, LyViewPane::CategoryTools, opts);
}

#include <SelectObjectDlg.moc>
