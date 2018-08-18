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
#include "AIPointPanel.h"

#include "Objects/AiPoint.h"
#include "IAgent.h"

#include "GameEngine.h"
#include "AI/NavDataGeneration/Navigation.h"

#include <QStringListModel>

#include <ui_AIPointPanel.h>

static CNavigation* GetNavigation ()
{
    return GetIEditor()->GetGameEngine()->GetNavigation();
}

static CGraph* GetGraph ()
{
    return GetNavigation()->GetGraph();
}

// CAIPointPanel dialog

CAIPointPanel::CAIPointPanel(QWidget* pParent /* = nullptr */)
    : QWidget(pParent)
    , m_ui(new Ui::AIPointPanel)
    , m_object(nullptr)
    , m_pickCallback(this, false)
    , m_pickImpassCallback(this, true)
{
    m_ui->setupUi(this);

    m_linksModel = new QStringListModel(this);
    m_ui->nodesListView->setModel(m_linksModel);

    m_ui->pickButton->SetPickCallback(&m_pickCallback, tr("Pick AIPoint to Link"), 0);
    m_ui->pickImpassButton->SetPickCallback(&m_pickImpassCallback, tr("Pick AIPoint to Link Impass"), 0);

    connect(m_ui->regenerateLinksButton, &QPushButton::clicked, this, &CAIPointPanel::OnBnClickedRegenLinks);
    connect(m_ui->selectButton, &QPushButton::clicked, this, &CAIPointPanel::OnBnClickedSelect);
    connect(m_ui->removeButton, &QPushButton::clicked, this, &CAIPointPanel::OnBnClickedRemove);
    connect(m_ui->removeAllButton, &QPushButton::clicked, this, &CAIPointPanel::OnBnClickedRemoveAll);
    connect(m_ui->removeAllInAreaButton, &QPushButton::clicked, this, &CAIPointPanel::OnBnClickedRemoveAllInArea);
    connect(m_ui->nodesListView, &QListView::doubleClicked, this, &CAIPointPanel::OnBnClickedSelect);
    connect(m_ui->waypointRadio, &QRadioButton::clicked, this, &CAIPointPanel::OnBnClickedWaypoint);
    connect(m_ui->hideRadio, &QRadioButton::clicked, this, &CAIPointPanel::OnBnClickedHidepoint);
    connect(m_ui->secondaryHideRadio, &QRadioButton::clicked, this, &CAIPointPanel::OnBnClickedHidepointSecondary);
    connect(m_ui->entryExitRadio, &QRadioButton::clicked, this, &CAIPointPanel::OnBnClickedEntrypoint);
    connect(m_ui->exitOnlyRadio, &QRadioButton::clicked, this, &CAIPointPanel::OnBnClickedExitpoint);
    connect(m_ui->humanRadio, &QRadioButton::clicked, this, &CAIPointPanel::OnBnClickedHuman);
    connect(m_ui->threeDimensionalSurfaceRadio, &QRadioButton::clicked, this, &CAIPointPanel::OnBnClicked3dsurface);
}

CAIPointPanel::~CAIPointPanel()
{
}

void CAIPointPanel::SetObject(CAIPoint* object)
{
    if (m_object)
    {
        for (int i = 0; i < m_object->GetLinkCount(); i++)
        {
            m_object->SelectLink(i, false);
        }
    }

    assert(object);
    if (!object)
    {
        return;
    }

    EnableLinkedWaypointsUI();

    m_object = object;
    switch (object->GetAIType())
    {
    case EAIPOINT_WAYPOINT:
        m_ui->waypointRadio->toggle();
        break;
    case EAIPOINT_HIDE:
        m_ui->hideRadio->toggle();
        break;
    case EAIPOINT_HIDESECONDARY:
        m_ui->secondaryHideRadio->toggle();
        break;
    case EAIPOINT_ENTRYEXIT:
        m_ui->entryExitRadio->toggle();
        break;
    case EAIPOINT_EXITONLY:
        m_ui->exitOnlyRadio->toggle();
        break;
    }

    switch (object->GetAINavigationType())
    {
    case EAINAVIGATION_HUMAN:
        m_ui->humanRadio->toggle();
        break;
    case EAINAVIGATION_3DSURFACE:
        m_ui->threeDimensionalSurfaceRadio->toggle();
        break;
    }

    m_ui->removableCheck->setCheckState(m_object->GetRemovable() ? Qt::Checked : Qt::Unchecked);

    ReloadLinks();
}

//////////////////////////////////////////////////////////////////////////
void CAIPointPanel::StartPick()
{
    // Simulate click on pick button.
    m_ui->pickButton->clicked();
}

void CAIPointPanel::StartPickImpass()
{
    // Simulate click on pick button.
    m_ui->pickImpassButton->clicked();
}

void CAIPointPanel::ReloadLinks()
{
    QStringList links;
    for (int i = 0; i < m_object->GetLinkCount(); i++)
    {
        CAIPoint* obj = m_object->GetLink(i);
        if (obj)
        {
            links.append(obj->GetName());
        }
    }

    m_ui->nodesListView->selectionModel()->disconnect(this);
    m_linksModel->setStringList(links);
    connect(m_ui->nodesListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CAIPointPanel::OnSelectionChanged);
    OnSelectionChanged({}, {});
    m_ui->removeAllButton->setEnabled(!links.isEmpty());
}

//////////////////////////////////////////////////////////////////////////
void CAIPointPanel::OnBnClickedSelect()
{
    assert(m_object);
    QModelIndex index = m_ui->nodesListView->currentIndex();
    if (index.isValid())
    {
        CBaseObject* obj = m_object->GetLink(index.row());
        if (obj)
        {
            GetIEditor()->ClearSelection();
            GetIEditor()->SelectObject(obj);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAIPointPanel::OnBnClickedRegenLinks()
{
    if (m_object)
    {
        m_object->RegenLinks();
    }
    else
    {
        CSelectionGroup* pSelectionGroup = GetIEditor()->GetSelection();

        for (int i = 0, n = pSelectionGroup->GetCount(); i < n; ++i)
        {
            CBaseObject* pBaseObject = pSelectionGroup->GetObject(i);
            if (!qobject_cast<CAIPoint*>(pBaseObject))
            {
                continue;
            }

            CAIPoint* pAIPoint = static_cast<CAIPoint*>(pBaseObject);
            pAIPoint->RegenLinks();
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CAIPointPanel::OnBnClickedRemove()
{
    assert(m_object);
    QModelIndex index = m_ui->nodesListView->currentIndex();
    if (index.isValid())
    {
        CUndo undo("Unlink AIPoint");
        CAIPoint* obj = m_object->GetLink(index.row());
        if (obj)
        {
            m_object->RemoveLink(obj);
        }
        //      m_object->RegenLinks();
        ReloadLinks();
    }
}

//////////////////////////////////////////////////////////////////////////
void CAIPointPanel::OnBnClickedRemoveAll()
{
    assert(m_object);
    m_object->RemoveAllLinks();
    //  m_object->RegenLinks();
    CUndo undo("Unlink AIPoint (all)");
    ReloadLinks();
}

//////////////////////////////////////////////////////////////////////////
void CAIPointPanel::OnBnClickedRemoveAllInArea()
{
    CGraph* aiGraph = GetNavigation()->GetGraph();
    if (!aiGraph)
    {
        return;
    }

    assert(m_object);

    if (!m_object->GetGraphNode())
    {
        return;
    }
    if (!(aiGraph->GetNavType(m_object->GetGraphNode()) & (IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_WAYPOINT_3DSURFACE)))
    {
        return;
    }
    int nBuildingID = aiGraph->GetBuildingIDFromWaypointNode(m_object->GetGraphNode());
    if (nBuildingID < 0)
    {
        return;
    }

    CBaseObjectsArray allObjects;
    GetIEditor()->GetObjectManager()->GetObjects(allObjects);
    std::vector<CAIPoint*> objectsInBuilding;

    unsigned nObjects = allObjects.size();
    for (unsigned i = 0; i < nObjects; ++i)
    {
        CBaseObject* baseObj = allObjects[i];
        if (qobject_cast<CAIPoint*>(baseObj))
        {
            CAIPoint* AIPoint = (CAIPoint*) baseObj;
            const GraphNode* pNode = AIPoint->GetGraphNode();
            if (!pNode)
            {
                continue;
            }
            if (!(aiGraph->GetNavType(pNode) & (IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_WAYPOINT_3DSURFACE)))
            {
                continue;
            }
            if (aiGraph->GetBuildingIDFromWaypointNode(pNode) != nBuildingID)
            {
                continue;
            }

            objectsInBuilding.push_back(AIPoint);
        }
    }

    unsigned nObjectsInBuilding = objectsInBuilding.size();
    for (unsigned i = 0; i < nObjectsInBuilding; ++i)
    {
        CUndo undo("Unlink AIPoint (all in area)");
        CAIPoint* obj = objectsInBuilding[i];
        obj->RemoveAllLinks();
    }
    if (nObjectsInBuilding > 0)
    {
        objectsInBuilding[0]->RegenLinks();
    }
    ReloadLinks();
}

//////////////////////////////////////////////////////////////////////////
void CAIPointPanel::OnPick(bool impass, CBaseObject* picked)
{
    assert(m_object);
    CUndo undo("Link AIPoint");
    m_object->AddLink((CAIPoint*)picked, !impass);
    //  m_object->RegenLinks();
    ReloadLinks();

    //
    //  m_entityName.SetWindowText( picked->GetName() );
}

//////////////////////////////////////////////////////////////////////////
bool CAIPointPanel::OnPickFilter(bool impass, CBaseObject* picked)
{
    assert(picked != 0);
    return picked != m_object && qobject_cast<CAIPoint*>(picked);
}

//////////////////////////////////////////////////////////////////////////
void CAIPointPanel::OnCancelPick(bool impass)
{
}

//////////////////////////////////////////////////////////////////////////
void CAIPointPanel::OnBnClickedWaypoint()
{
    SetAIType(EAIPOINT_WAYPOINT);
}

//////////////////////////////////////////////////////////////////////////
void CAIPointPanel::OnBnClickedEntrypoint()
{
    SetAIType(EAIPOINT_ENTRYEXIT);
}

//////////////////////////////////////////////////////////////////////////
void CAIPointPanel::OnBnClickedExitpoint()
{
    SetAIType(EAIPOINT_EXITONLY);
}

//////////////////////////////////////////////////////////////////////////
void CAIPointPanel::OnBnClickedHidepoint()
{
    SetAIType(EAIPOINT_HIDE);
    //      SW_ON_OBJ_MOD(m_object);
}

//////////////////////////////////////////////////////////////////////////
void CAIPointPanel::OnBnClickedHidepointSecondary()
{
    SetAIType(EAIPOINT_HIDESECONDARY);
    //      SW_ON_OBJ_MOD(m_object);
}


//////////////////////////////////////////////////////////////////////////
void CAIPointPanel::OnLbnLinksSelChange()
{
    assert(m_object);
    QModelIndex index = m_ui->nodesListView->currentIndex();
    if (index.isValid())
    {
        // Unselect all others.
        for (int i = 0; i < m_object->GetLinkCount(); i++)
        {
            if (index.row() == i)
            {
                m_object->SelectLink(i, true);
            }
            else
            {
                m_object->SelectLink(i, false);
            }
        }
    }
}

void CAIPointPanel::OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    bool enabled = !selected.indexes().isEmpty();
    m_ui->selectButton->setEnabled(enabled);
    m_ui->removeButton->setEnabled(enabled);
}

//////////////////////////////////////////////////////////////////////////
void CAIPointPanel::OnBnClickedRemovable()
{
    if (m_object)
    {
        m_object->MakeRemovable(!m_object->GetRemovable());
    }
    else
    {
        bool bRemovable = m_ui->removableCheck->checkState() == Qt::Checked;

        CSelectionGroup* pSelectionGroup = GetIEditor()->GetSelection();

        for (int i = 0, n = pSelectionGroup->GetCount(); i < n; ++i)
        {
            CBaseObject* pBaseObject = pSelectionGroup->GetObject(i);
            if (!qobject_cast<CAIPoint*>(pBaseObject))
            {
                continue;
            }

            CAIPoint* pAIPoint = static_cast<CAIPoint*>(pBaseObject);
            pAIPoint->MakeRemovable(bRemovable);
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CAIPointPanel::OnBnClickedHuman()
{
    SetAINavigationType(EAINAVIGATION_HUMAN);
}

//////////////////////////////////////////////////////////////////////////
void CAIPointPanel::OnBnClicked3dsurface()
{
    SetAINavigationType(EAINAVIGATION_3DSURFACE);
}

//////////////////////////////////////////////////////////////////////////
void CAIPointPanel::SetAIType(EAIPointType eAIPointType)
{
    CUndo undo("AIPoint Type");

    if (m_object)
    {
        m_object->SetAIType(eAIPointType);
    }
    else
    {
        CSelectionGroup* pSelectionGroup = GetIEditor()->GetSelection();

        for (int i = 0, n = pSelectionGroup->GetCount(); i < n; ++i)
        {
            CBaseObject* pBaseObject = pSelectionGroup->GetObject(i);
            if (!qobject_cast<CAIPoint*>(pBaseObject))
            {
                continue;
            }

            CAIPoint* pAIPoint = static_cast<CAIPoint*>(pBaseObject);
            pAIPoint->SetAIType(eAIPointType);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAIPointPanel::SetAINavigationType(EAINavigationType eAINavigationType)
{
    CUndo undo("AIPoint Nav Type");

    if (m_object)
    {
        m_object->SetAINavigationType(eAINavigationType);
    }
    else
    {
        CSelectionGroup* pSelectionGroup = GetIEditor()->GetSelection();

        for (int i = 0, n = pSelectionGroup->GetCount(); i < n; ++i)
        {
            CBaseObject* pBaseObject = pSelectionGroup->GetObject(i);
            if (!qobject_cast<CAIPoint*>(pBaseObject))
            {
                continue;
            }

            CAIPoint* pAIPoint = static_cast<CAIPoint*>(pBaseObject);
            pAIPoint->SetAINavigationType(eAINavigationType);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CAIPointPanel::EnableLinkedWaypointsUI()
{
    m_ui->nodesListView->setEnabled(true);
    m_ui->pickButton->setEnabled(true);
    m_ui->pickImpassButton->setEnabled(true);
    m_ui->removeAllInAreaButton->setEnabled(true);
}

#include <AIPointPanel.moc>
