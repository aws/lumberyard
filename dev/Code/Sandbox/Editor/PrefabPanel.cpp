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
#include "PrefabPanel.h"
#include <ui_PrefabPanel.h>

#include "Prefabs/PrefabManager.h"
#include "Objects/PrefabObject.h"
#include "Prefabs/PrefabItem.h"

#include <QItemSelectionModel>
#include <QMenu>
#include <QMessageBox>


class CUndoGroupObjectOpenClose
    : public IUndoObject
{
public:
    CUndoGroupObjectOpenClose(CPrefabObject* prefabObj)
    {
        m_prefabObject = prefabObj;
        m_bOpenForUndo = m_prefabObject->IsOpen();
    }
protected:
    virtual int GetSize() { return sizeof(*this); }; // Return size of xml state.
    virtual QString GetDescription() { return "Prefab's Open/Close"; };

    virtual void Undo(bool bUndo)
    {
        m_bRedo = bUndo;
        Apply(m_bOpenForUndo);
    }
    virtual void Redo()
    {
        if (m_bRedo)
        {
            Apply(!m_bOpenForUndo);
        }
    }

    void Apply(bool bOpen)
    {
        if (m_prefabObject)
        {
            if (bOpen)
            {
                m_prefabObject->Open();
            }
            else
            {
                m_prefabObject->Close();
            }
        }
    }

private:

    CPrefabObjectPtr m_prefabObject;
    bool m_bRedo;
    bool m_bOpenForUndo;
};


#define COLUMN_NAME 0
#define COLUMN_TYPE 1

CPrefabPanel::CPrefabPanel(QWidget* pParent /*=nullptr*/)
    : QWidget(pParent)
    , m_type(0)
    , m_changePivotMode(false)
    , ui(new Ui::CPrefabPanel)
    , m_updatingObjectList(false)
{
    ui->setupUi(this);
    m_pPrefabObject = 0;

    GetIEditor()->GetObjectManager()->AddObjectEventListener(functor(*this, &CPrefabPanel::OnObjectEvent));

    connect(ui->PREFAB, &QPushButton::clicked, this, &CPrefabPanel::OnBnClickedGotoPrefab);
    connect(ui->REMOVE, &QPushButton::clicked, this, &CPrefabPanel::OnBnClickedDelete);
    connect(ui->CLONE_SELECTED, &QPushButton::clicked, this, &CPrefabPanel::OnBnClickedCloneSelected);
    connect(ui->CLONE_ALL, &QPushButton::clicked, this, &CPrefabPanel::OnBnClickedCloneAll);
    connect(ui->EXTRACT_SELECTED, &QPushButton::clicked, this, &CPrefabPanel::OnBnClickedExtractSelected);
    connect(ui->EXTRACT_ALL, &QPushButton::clicked, this, &CPrefabPanel::OnBnClickedExtractAll);
    connect(ui->OPEN, &QPushButton::clicked, this, &CPrefabPanel::OnBnClickedOpen);
    connect(ui->CHANGE_PIVOT, &QPushButton::clicked, this, &CPrefabPanel::OnBnClickedChangePivot);
    connect(ui->CLOSE, &QPushButton::clicked, this, &CPrefabPanel::OnBnClickedClose);
    connect(ui->CLOSEALL, &QPushButton::clicked, this, &CPrefabPanel::OnBnClickedCloseAll);
    connect(ui->OPENALL, &QPushButton::clicked, this, &CPrefabPanel::OnBnClickedOpenAll);

    connect(ui->OBJECTS, &QWidget::customContextMenuRequested, this, &CPrefabPanel::OnContextMenu);
    connect(ui->OBJECTS, &QTableWidget::itemSelectionChanged, this, &CPrefabPanel::OnItemchangedObjects);
    connect(ui->OBJECTS, &QTableWidget::itemDoubleClicked, this, &CPrefabPanel::OnDblclkObjects);

    m_prefabButtons.push_back(ui->PICK_NEW);
    m_prefabButtons.push_back(ui->REMOVE);
    m_prefabButtons.push_back(ui->CLONE_SELECTED);
    m_prefabButtons.push_back(ui->CLONE_ALL);
    m_prefabButtons.push_back(ui->EXTRACT_SELECTED);
    m_prefabButtons.push_back(ui->EXTRACT_ALL);
    m_prefabButtons.push_back(ui->OPEN);
    m_prefabButtons.push_back(ui->CLOSE);
    m_prefabButtons.push_back(ui->CHANGE_PIVOT);

    ui->PICK_NEW->SetPickCallback(this, tr("Pick Object To Attach"));
    ui->REMOVE->setEnabled(false);
    ui->EXTRACT_SELECTED->setEnabled(false);
    ui->CLONE_SELECTED->setEnabled(false);
}

CPrefabPanel::~CPrefabPanel()
{
    m_prefabButtons.clear();
    SetPrefabPivotModeOnCurrentEditTool(false);
    GetIEditor()->GetObjectManager()->RemoveObjectEventListener(functor(*this, &CPrefabPanel::OnObjectEvent));
}

//////////////////////////////////////////////////////////////////////////
void CPrefabPanel::OnContextMenu()
{
    std::vector<CBaseObject*> selectedObjects;
    int selectedCount = GetSelectedObjects(selectedObjects);
    if (selectedCount < 1)
    {
        return;
    }

    QMenu menu;
    enum
    {
        eCmd_Delete = 1,
        eCmd_Clone,
        eCmd_Extract,
        eCmd_Pivot,
        eCmd_Last
    };

    menu.addAction(tr("Delete Object(s)"))->setData(eCmd_Delete);
    menu.addAction(tr("Clone Object(s)"))->setData(eCmd_Clone);
    menu.addAction(tr("Extract Object(s)"))->setData(eCmd_Extract);

    if (selectedCount == 1)
    {
        menu.addSeparator();
        menu.addAction(tr("Use Pivot for Prefab"))->setData(eCmd_Pivot);
    }

    QAction* action = menu.exec(QCursor::pos());
    int nResult = action ? action->data().toInt() : -1;

    switch (nResult)
    {
    case eCmd_Delete:
        OnBnClickedDelete();
        break;

    case eCmd_Clone:
        OnBnClickedCloneSelected();
        break;

    case eCmd_Extract:
        OnBnClickedExtractSelected();
        break;

    case eCmd_Pivot:
        if (QMessageBox::question(this, tr("Apply new pivot"), tr("This will affect all instances of this prefab in your level.\n\nDo you want to continue?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
        {
            m_pPrefabObject->SetPivot(selectedObjects[0]->GetWorldPos());
        }
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabPanel::SetObject(CPrefabObject* object)
{
    assert(object);
    m_pPrefabObject = object;

    ReloadObjects();
}

//////////////////////////////////////////////////////////////////////////
void CPrefabPanel::OnObjectEvent(CBaseObject* pObject, int nEvent)
{
    if (nEvent == CBaseObject::ON_PREFAB_CHANGED)
    {
        if (pObject->GetPrefab() == m_pPrefabObject || pObject == m_pPrefabObject)
        {
            ReloadObjects();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabPanel::RecursivelyGetAllPrefabChilds(CBaseObject* obj, std::vector<ChildRecord>& childs, int level)
{
    if (!obj)
    {
        return;
    }

    for (int i = 0; i < obj->GetChildCount(); i++)
    {
        CBaseObject* pChild = obj->GetChild(i);
        if (pChild->CheckFlags(OBJFLAG_PREFAB) && pChild->GetGroup() == pChild->GetPrefab())
        {
            ChildRecord rec;
            rec.pObject = pChild;
            rec.level = level;
            rec.bSelected = false;
            childs.push_back(rec);
            if (!qobject_cast<CPrefabObject*>(pChild))
            {
                RecursivelyGetAllPrefabChilds(pChild, childs, level + 1);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabPanel::ReloadListCtrl()
{
    m_updatingObjectList = true;
    ui->OBJECTS->clear();
    ui->OBJECTS->setColumnCount(2);
    ui->OBJECTS->setRowCount(m_childObjects.size());
    ui->OBJECTS->setHorizontalHeaderLabels({tr("Name"), tr("Type")});
    for (int i = 0; i < m_childObjects.size(); i++)
    {
        const ChildRecord& record = m_childObjects[i];

        // KDAB_PORT original code sets an "indent = record.level" on this item
        // see https://msdn.microsoft.com/en-us/library/windows/desktop/bb774760(v=vs.85).aspx
        // use variable width pixmap, size arbitrary since original is unknown
        QString text = record.pObject->GetName();
        QTableWidgetItem* item = new QTableWidgetItem(text);
        if (record.level)
        {
            QPixmap pm(12 * record.level, 12);
            pm.fill(Qt::transparent);
            item->setIcon(QIcon(pm));
        }
        ui->OBJECTS->setItem(i, 0, item);
        item = new QTableWidgetItem(record.pObject->GetTypeDescription());
        ui->OBJECTS->setItem(i, 1, item);
    }
    m_updatingObjectList = false;
}

//////////////////////////////////////////////////////////////////////////
void CPrefabPanel::ReloadObjects()
{
    m_childObjects.clear();
    RecursivelyGetAllPrefabChilds(m_pPrefabObject, m_childObjects, 0);

    ReloadListCtrl();

    if (m_pPrefabObject)
    {
        if (m_pPrefabObject->IsOpen())
        {
            ui->OPEN->setEnabled(false);
            ui->CLOSE->setEnabled(true);
        }
        else
        {
            ui->OPEN->setEnabled(true);
            ui->CLOSE->setEnabled(false);
        }

        BOOL bEnableButtons = m_childObjects.empty() ? FALSE : TRUE;
        ui->CLONE_ALL->setEnabled(bEnableButtons);
        ui->EXTRACT_ALL->setEnabled(bEnableButtons);

        CPrefabItem* pItem = m_pPrefabObject->GetPrefab();
        if (pItem)
        {
            int numObjects = m_childObjects.size();
            ui->PREFAB_NAME->setText(tr("%1 (%2 Object%3)").arg(pItem->GetFullName()).arg(numObjects).arg(numObjects > 1 ? "s" : ""));

            int numInstances = GetIEditor()->GetPrefabManager()->GetPrefabInstanceCount(pItem);
            ui->STATIC1->setTitle(tr("Local (%1 Instance%2)").arg(numInstances).arg(numInstances > 1 ? "s" : ""));
        }
    }
    // Multiple prefab selection
    else
    {
        ui->OBJECTS->setEnabled(false);
        ui->PREFAB_NAME->setText(tr("Prefab Selection"));
        ui->PREFAB_NAME->setEnabled(false);
        ui->PICK_NEW->setEnabled(false);
        ui->EXTRACT_SELECTED->setEnabled(false);
        ui->REMOVE->setEnabled(false);
        ui->PREFAB->setEnabled(false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabPanel::OnItemchangedObjects()
{
    if (m_updatingObjectList)
    {
        return;
    }

    QModelIndexList selection = ui->OBJECTS->selectionModel()->selectedRows();
    BOOL bEnableButtons = selection.size();
    ui->REMOVE->setEnabled(bEnableButtons);
    ui->EXTRACT_SELECTED->setEnabled(bEnableButtons);
    ui->CLONE_SELECTED->setEnabled(bEnableButtons);

    for (int i = 0; i < ui->OBJECTS->rowCount(); ++i)
    {
        m_childObjects[i].bSelected = false;
    }
    for (int i = 0; i < selection.size(); ++i)
    {
        m_childObjects[selection[i].row()].bSelected = true;
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabPanel::OnDblclkObjects(QTableWidgetItem* item)
{
    ChildRecord& record = m_childObjects[item->row()];
    CBaseObject* pObject = record.pObject;
    if (pObject)
    {
        GetIEditor()->ClearSelection();
        GetIEditor()->SelectObject(pObject);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabPanel::OnBnClickedCloneSelected()
{
    if (!m_pPrefabObject)
    {
        return;
    }

    std::vector<CBaseObject*> objectsToClone;

    if (GetSelectedObjects(objectsToClone) > 0)
    {
        GetIEditor()->GetPrefabManager()->CloneObjectsFromPrefabs(objectsToClone);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabPanel::OnBnClickedCloneAll()
{
    std::vector<CPrefabObject*>pPrefabs;
    if (m_pPrefabObject)
    {
        pPrefabs.push_back(m_pPrefabObject);
    }
    else if (CSelectionGroup* pSelection = GetIEditor()->GetSelection())
    {
        for (int i = 0, count = pSelection->GetCount(); i < count; i++)
        {
            if (qobject_cast<CPrefabObject*>(pSelection->GetObject(i)))
            {
                CPrefabObject* pPrefab = static_cast<CPrefabObject*>(pSelection->GetObject(i));
                pPrefabs.push_back(pPrefab);
            }
        }
    }

    if (!pPrefabs.empty())
    {
        GetIEditor()->GetPrefabManager()->CloneAllFromPrefabs(pPrefabs);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabPanel::OnBnClickedExtractSelected()
{
    if (!m_pPrefabObject)
    {
        return;
    }

    std::vector<CBaseObject*> objectsToExtract;

    if (GetSelectedObjects(objectsToExtract) > 0)
    {
        GetIEditor()->GetPrefabManager()->ExtractObjectsFromPrefabs(objectsToExtract);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabPanel::OnBnClickedExtractAll()
{
    if (CryMessageBox("Are you sure you want to extract all objects from this prefab?", "Extract objects", MB_YESNO | MB_ICONWARNING) == IDNO)
    {
        return;
    }

    std::vector<CPrefabObject*>pPrefabs;
    if (m_pPrefabObject)
    {
        pPrefabs.push_back(m_pPrefabObject);
    }
    else if (CSelectionGroup* pSelection = GetIEditor()->GetSelection())
    {
        for (int i = 0, count = pSelection->GetCount(); i < count; i++)
        {
            if (qobject_cast<CPrefabObject*>(pSelection->GetObject(i)))
            {
                CPrefabObject* pPrefab = static_cast<CPrefabObject*>(pSelection->GetObject(i));
                pPrefabs.push_back(pPrefab);
            }
        }
    }

    if (!pPrefabs.empty())
    {
        GetIEditor()->GetPrefabManager()->ExtractAllFromPrefabs(pPrefabs);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabPanel::OnBnClickedGotoPrefab()
{
    if (m_pPrefabObject)
    {
        GetIEditor()->OpenDataBaseLibrary(EDB_TYPE_PREFAB, m_pPrefabObject->GetPrefab());
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabPanel::OnBnClickedOpen()
{
    if (m_pPrefabObject && !m_pPrefabObject->IsOpen())
    {
        std::vector<CPrefabObject*> selectedPrefabs(1, m_pPrefabObject);
        if (GetIEditor()->GetPrefabManager()->OpenPrefabs(selectedPrefabs, "Open prefab object"))
        {
            ReloadObjects();
        }
    }
    else if (CSelectionGroup* pSelection = GetIEditor()->GetSelection())
    {
        const int count = pSelection->GetCount();
        std::vector<CPrefabObject*> selectedPrefabs(count);

        for (int i = 0; i < count; ++i)
        {
            selectedPrefabs[i] = static_cast<CPrefabObject*>(pSelection->GetObject(i));
        }

        if (GetIEditor()->GetPrefabManager()->OpenPrefabs(selectedPrefabs, "Open selected prefab object"))
        {
            ReloadObjects();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabPanel::OnBnClickedChangePivot()
{
    if (!m_changePivotMode)
    {
        m_changePivotMode = true;
        ui->CHANGE_PIVOT->setText(tr("Pivot Mode is ON"));
    }
    else
    {
        m_changePivotMode = false;
        ui->CHANGE_PIVOT->setText(tr("Pivot Mode is OFF"));
    }

    SetPrefabPivotModeOnCurrentEditTool(m_changePivotMode);
}

//////////////////////////////////////////////////////////////////////////
void CPrefabPanel::OnBnClickedClose()
{
    if (m_pPrefabObject && m_pPrefabObject->IsOpen())
    {
        std::vector<CPrefabObject*> selectedPrefabs(1, m_pPrefabObject);
        if (GetIEditor()->GetPrefabManager()->ClosePrefabs(selectedPrefabs, "Close prefab object"))
        {
            ReloadObjects();
        }
    }
    else if (CSelectionGroup* pSelection = GetIEditor()->GetSelection())
    {
        const int count = pSelection->GetCount();
        std::vector<CPrefabObject*> selectedPrefabs(count);

        for (int i = 0; i < count; ++i)
        {
            selectedPrefabs[i] = static_cast<CPrefabObject*>(pSelection->GetObject(i));
        }

        if (GetIEditor()->GetPrefabManager()->ClosePrefabs(selectedPrefabs, "Close selected prefab objects"))
        {
            ReloadObjects();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabPanel::OnBnClickedDelete()
{
    if (!m_pPrefabObject)
    {
        return;
    }

    CUndo undo("Delete Object(s)");
    std::vector<CBaseObject*> selectedObjects;
    int objectCount = GetSelectedObjects(selectedObjects);

    for (int i = 0; i < objectCount; ++i)
    {
        GetIEditor()->GetObjectManager()->DeleteObject(selectedObjects[i]);
    }

    if (objectCount > 0)
    {
        ReloadObjects();
        GetIEditor()->GetObjectManager()->InvalidateVisibleList();
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabPanel::OnPick(CBaseObject* picked)
{
    if (m_pPrefabObject)
    {
        if (!m_pPrefabObject->CanObjectBeAddedAsMember(picked))
        {
            Warning("Object %s is already part of a prefab (%s)", picked->GetName().toUtf8().constData(), picked->GetPrefab()->GetName().toUtf8().constData());
            return;
        }

        // TO DO notification for GD change
        TBaseObjects objects;

        CUndo undo("Add Object To Prefab");
        if (CUndo::IsRecording())
        {
            // If this is not a group add all the attached children to the prefab,
            // otherwise the group children adding is handled by the AddMember in CPrefabObject
            if (!qobject_cast<CGroup*>(picked))
            {
                objects.reserve(picked->GetChildCount() + 1);
                objects.push_back(picked);
                picked->GetAllChildren(objects);
            }
            else
            {
                objects.push_back(picked);
            }

            CUndo::Record(new CUndoAddObjectsToPrefab(m_pPrefabObject, objects));
        }

        for (int i = 0; i < objects.size(); i++)
        {
            m_pPrefabObject->AddMember(objects[i]);
        }

        ReloadObjects();
    }
}

//////////////////////////////////////////////////////////////////////////
bool CPrefabPanel::OnPickFilter(CBaseObject* picked)
{
    if (picked->CheckFlags(OBJFLAG_PREFAB) || picked == m_pPrefabObject)
    {
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CPrefabPanel::OnCancelPick()
{
}

//////////////////////////////////////////////////////////////////////////
void CPrefabPanel::OnBnClickedCloseAll()
{
    std::vector<CPrefabObject*> prefabObjects;
    GetAllPrefabObjects(prefabObjects);

    if (GetIEditor()->GetPrefabManager()->ClosePrefabs(prefabObjects, "Close all prefab objects"))
    {
        ReloadObjects();
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabPanel::OnBnClickedOpenAll()
{
    std::vector<CPrefabObject*> prefabObjects;
    GetAllPrefabObjects(prefabObjects);

    if (GetIEditor()->GetPrefabManager()->OpenPrefabs(prefabObjects, "Open all prefab objects"))
    {
        ReloadObjects();
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabPanel::GetAllPrefabObjects(std::vector<CPrefabObject*>& outPrefabObjects)
{
    std::vector<CBaseObject*> prefabObjects;
    GetIEditor()->GetObjectManager()->FindObjectsOfType(&CPrefabObject::staticMetaObject, prefabObjects);
    if (prefabObjects.empty())
    {
        return;
    }

    bool bOpenMoreThanOne = false;
    for (int i = 0, iCount(prefabObjects.size()); i < iCount; ++i)
    {
        CPrefabObject* pPrefabObj = (CPrefabObject*)prefabObjects[i];
        if (pPrefabObj == NULL)
        {
            continue;
        }
        if (m_pPrefabObject && pPrefabObj->GetPrefab() != m_pPrefabObject->GetPrefab())
        {
            continue;
        }
        outPrefabObjects.push_back(pPrefabObj);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabPanel::SetPrefabPivotModeOnCurrentEditTool(bool pivotModeOn)
{
    if (m_pPrefabObject)
    {
        m_pPrefabObject->SetChangePivotMode(pivotModeOn);
    }
}

//////////////////////////////////////////////////////////////////////////
int CPrefabPanel::GetSelectedObjects(std::vector<CBaseObject*>& selectedObjects)
{
    selectedObjects.clear();

    for (int i = 0; i < m_childObjects.size(); ++i)
    {
        if (m_childObjects[i].bSelected)
        {
            selectedObjects.push_back(m_childObjects[i].pObject);
        }
    }

    return selectedObjects.size();
}
