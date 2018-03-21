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
#include "EntityPanel.h"
#include "AIWavePanel.h"
#include "ShapePanel.h"
#include "Objects/EntityObject.h"
#include "Objects/ShapeObject.h"
#include "Objects/AIWave.h"
#include "StringDlg.h"

#include "CryEditDoc.h"
#include "Mission.h"
#include "MissionScript.h"
#include "EntityPrototype.h"
#include "QtViewPaneManager.h"

#include "GenericSelectItemDialog.h"

#include <HyperGraph/FlowGraphManager.h>
#include <HyperGraph/FlowGraph.h>
#include <HyperGraph/FlowGraphHelpers.h>

#include <HyperGraph/HyperGraphDialog.h>
#include <HyperGraph/FlowGraphSearchCtrl.h>
#include <TrackView/TrackViewDialog.h>

#include <ui_EntityPanel.h>
#include <ui_EntityEventsPanel.h>
#include "QtUtil.h"

#include <QInputDialog>
#include <QMenu>
#include <QTreeWidgetItem>

/////////////////////////////////////////////////////////////////////////////
// CEntityPanel dialog


CEntityPanel::CEntityPanel(QWidget* pParent /*=nullptr*/)
    : QWidget(pParent)
    , ui(new Ui::CEntityPanel)
{
    m_entity = 0;
    ui->setupUi(this);

    m_editScriptButton = ui->EDITSCRIPT;
    m_reloadScriptButton = ui->RELOADSCRIPT;

    m_prototypeButton = ui->PROTOTYPE;
    m_flowGraphOpenBtn = ui->OPENFLOWGRAPH;
    m_flowGraphRemoveBtn = ui->REMOVEFLOWGRAPH;
    m_flowGraphListBtn = ui->LIST_ENTITY_FLOWGRAPHS;

    m_physicsBtn[0] = ui->GETPHYSICS;
    m_physicsBtn[1] = ui->RESETPHYSICS;

    m_trackViewSequenceButton = ui->TRACKVIEW_SEQUENCE;

    m_frame = ui->FRAME2;

    connect(m_editScriptButton, &QPushButton::clicked, this, &CEntityPanel::OnEditScript);
    connect(m_reloadScriptButton, &QPushButton::clicked, this, &CEntityPanel::OnReloadScript);
    connect(ui->FILE_COMMANDS, &QPushButton::clicked, this, &CEntityPanel::OnFileCommands);
    connect(m_prototypeButton, &QPushButton::clicked, this, &CEntityPanel::OnPrototype);
    connect(m_flowGraphOpenBtn, &QPushButton::clicked, this, &CEntityPanel::OnBnClickedOpenFlowGraph);
    connect(m_flowGraphRemoveBtn, &QPushButton::clicked, this, &CEntityPanel::OnBnClickedRemoveFlowGraph);
    connect(m_flowGraphListBtn, &QPushButton::clicked, this, &CEntityPanel::OnBnClickedListFlowGraphs);
    connect(m_physicsBtn[0], &QPushButton::clicked, this, &CEntityPanel::OnBnClickedGetphysics);
    connect(m_physicsBtn[1], &QPushButton::clicked, this, &CEntityPanel::OnBnClickedResetphysics);
    connect(m_trackViewSequenceButton, &QPushButton::clicked, this, &CEntityPanel::OnBnClickedTrackViewSequence);
}

/////////////////////////////////////////////////////////////////////////////
// CEntityPanel message handlers
void CEntityPanel::SetEntity(CEntityObject* entity)
{
    assert(entity);

    m_entity = entity;

    if (m_entity != NULL && m_entity->GetScript())
    {
        ui->SCRIPT_NAME->setText(m_entity->GetScript()->GetFile());
    }

    if (!qobject_cast<CAITerritoryObject*>(entity) && !qobject_cast<CAIWaveObject*>(entity))
    {
        if (qobject_cast<CAITerritoryPanel*>(this) || qobject_cast<CAIWavePanel*>(this))
        {
            return;
        }

        if (m_entity != NULL && !entity->GetPrototype())
        {
            m_prototypeButton->setEnabled(false);
            m_prototypeButton->setText(tr("Entity Archetype"));
        }
        else
        {
            m_prototypeButton->setEnabled(true);
            m_prototypeButton->setText(entity->GetPrototype()->GetFullName());
        }
    }

    if (m_entity != NULL && m_entity->GetFlowGraph())
    {
        m_flowGraphOpenBtn->setText(tr("Open"));
        m_flowGraphOpenBtn->setEnabled(true);
        m_flowGraphRemoveBtn->setEnabled(true);
    }
    else
    {
        m_flowGraphOpenBtn->setText(tr("Create"));
        m_flowGraphOpenBtn->setEnabled(true);
        m_flowGraphRemoveBtn->setEnabled(false);
    }

    if (m_trackViewSequenceButton->isVisible())
    {
        CTrackViewAnimNode* pAnimNode = nullptr;
        m_trackViewSequenceButton->setText(tr("Sequence"));
        if (m_entity != nullptr && (pAnimNode = GetIEditor()->GetSequenceManager()->GetActiveAnimNode(entity)) && pAnimNode->GetSequence())
        {
            m_trackViewSequenceButton->setEnabled(true);
        }
        else
        {
            m_trackViewSequenceButton->setEnabled(false);
        }
    }
}


void CEntityPanel::OnEditScript()
{
    assert(m_entity != 0);
    CEntityScript* script = m_entity->GetScript();

    AZStd::string cmd = AZStd::string::format("general.launch_lua_editor \'%s\'", script->GetFile().toUtf8().data());
    GetIEditor()->ExecuteCommand(cmd.c_str());
}

void CEntityPanel::OnReloadScript()
{
    assert(m_entity != 0);

    m_entity->OnMenuReloadScripts();
}

void CEntityPanel::OnFileCommands()
{
    assert(m_entity != 0);
    CEntityScript* pScript = m_entity->GetScript();
    if (pScript)
    {
        CFileUtil::PopupQMenu(Path::GetFile(pScript->GetFile()), Path::GetPath(pScript->GetFile()), this);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityPanel::OnPrototype()
{
    // Go to the entity prototype.
    // Open corresponding prototype.
    if (m_entity)
    {
        if (m_entity->GetPrototype())
        {
            GetIEditor()->OpenDataBaseLibrary(EDB_TYPE_ENTITY_ARCHETYPE, m_entity->GetPrototype());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityPanel::OnBnClickedTrackViewSequence()
{
    if (m_entity)
    {
        CTrackViewAnimNodeBundle bundle = GetIEditor()->GetSequenceManager()->GetAllRelatedAnimNodes(m_entity);
        CTrackViewAnimNodeBundle finalBundle;

        if (bundle.GetCount() > 0)
        {
            QMenu menu;
            unsigned int id = 1;

            for (unsigned int i = 0; i < bundle.GetCount(); ++i)
            {
                CTrackViewAnimNode* pNode = bundle.GetNode(i);

                if (pNode->GetSequence())
                {
                    menu.addAction(QtUtil::ToQString(pNode->GetSequence()->GetName()))->setData(id);
                    finalBundle.AppendAnimNode(pNode);
                    id++;   // KDAB_PORT original code never incremented, so first one was always chosen. Right/Wrong?
                }
            }

            QAction* res = menu.exec(QCursor::pos());
            int chosen = res ? (res->data().toInt() - 1) : -1;
            if (chosen >= 0)
            {
                QtViewPaneManager::instance()->OpenPane(LyViewPane::TrackView);
                CTrackViewDialog* pTVDlg = CTrackViewDialog::GetCurrentInstance();
                if (pTVDlg)
                {
                    GetIEditor()->GetAnimation()->SetSequence(finalBundle.GetNode(chosen)->GetSequence(), false, false);
                    CTrackViewAnimNode* pNode = finalBundle.GetNode(chosen);

                    if (pNode)
                    {
                        pNode->SetSelected(true);
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityPanel::OnBnClickedGetphysics()
{
    if (m_entity)
    {
        CUndo undo("Accept Physics State");
        m_entity->AcceptPhysicsState();
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityPanel::OnBnClickedResetphysics()
{
    if (m_entity)
    {
        CUndo undo("Reset Physics State");
        m_entity->ResetPhysicsState();
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityPanel::OnBnClickedOpenFlowGraph()
{
    if (m_entity)
    {
        if (!m_entity->GetFlowGraph())
        {
            m_entity->CreateFlowGraphWithGroupDialog();
        }
        else
        {
            // Flow graph already present.
            m_entity->OpenFlowGraph("");
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityPanel::OnBnClickedRemoveFlowGraph()
{
    if (m_entity)
    {
        if (m_entity->GetFlowGraph())
        {
            CUndo undo("Remove Flow graph");
            QString str(tr("Remove Flow Graph for Entity %1?").arg(m_entity->GetName()));
            if (QMessageBox::question(this, "Confirm", str, QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
            {
                m_entity->RemoveFlowGraph();
                SetEntity(m_entity);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityPanel::OnBnClickedListFlowGraphs()
{
    std::vector<CFlowGraph*> flowgraphs;
    CFlowGraph* entityFG = 0;
    FlowGraphHelpers::FindGraphsForEntity(m_entity, flowgraphs, entityFG);
    if (flowgraphs.size() > 0)
    {
        QMenu menu;
        unsigned int id = 1;
        std::vector<CFlowGraph*>::const_iterator iter (flowgraphs.begin());
        while (iter != flowgraphs.end())
        {
            QString name;
            FlowGraphHelpers::GetHumanName(*iter, name);
            if (*iter == entityFG)
            {
                name += " <GraphEntity>";
                menu.addAction(name)->setData(id);
                if (flowgraphs.size() > 1)
                {
                    menu.addSeparator();
                }
            }
            else
            {
                menu.addAction(name)->setData(id);
            }
            ++id;
            ++iter;
        }

        QAction* res = menu.exec(QCursor::pos());
        int chosen = res ? (res->data().toInt() - 1) : -1;
        if (chosen >= 0)
        {
            GetIEditor()->GetFlowGraphManager()->OpenView(flowgraphs[chosen]);
            CHyperGraphDialog* pHGDlg = CHyperGraphDialog::instance();
            if (pHGDlg)
            {
                CFlowGraphSearchCtrl* pSC = pHGDlg->GetSearchControl();
                if (pSC)
                {
                    CFlowGraphSearchOptions* pOpts = CFlowGraphSearchOptions::GetSearchOptions();
                    pOpts->m_bIncludeEntities = true;
                    pOpts->m_findSpecial = CFlowGraphSearchOptions::eFLS_None;
                    pOpts->m_LookinIndex = CFlowGraphSearchOptions::eFL_Current;
                    pSC->Find(m_entity->GetName(), false, true, true);
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CEntityEventsPanel dialog
CEntityEventsPanel::CEntityEventsPanel(QWidget* pParent /*=nullptr*/)
    : QWidget(pParent)
    , ui(new Ui::CEntityEventsPanel)
{
    ui->setupUi(this);

    m_entity = 0;
    m_pickTool = 0;

    m_sendEvent = ui->EVENT_SEND;
    connect(m_sendEvent, &QPushButton::clicked, this, &CEntityEventsPanel::OnEventSend);
    m_runButton = ui->RUN_METHOD;
    connect(m_runButton, &QPushButton::clicked, this, &CEntityEventsPanel::OnRunMethod);
    m_gotoMethodBtn = ui->GOTO_METHOD;
    connect(m_gotoMethodBtn, &QPushButton::clicked, this, &CEntityEventsPanel::OnGotoMethod);
    m_addMethodBtn = ui->ADD_METHOD;
    connect(m_addMethodBtn, &QPushButton::clicked, this, &CEntityEventsPanel::OnAddMethod);
    m_removeButton = ui->EVENT_REMOVE;
    connect(m_removeButton, &QPushButton::clicked, this, &CEntityEventsPanel::OnEventRemove);
    m_eventTree = ui->EVENTTREE;
    m_eventTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_eventTree, &QTreeWidget::customContextMenuRequested, this, &CEntityEventsPanel::OnRclickEventTree);
    connect(m_eventTree, &QTreeWidget::itemDoubleClicked, this, &CEntityEventsPanel::OnDblClickEventTree);
    connect(m_eventTree, &QTreeWidget::itemSelectionChanged, this, &CEntityEventsPanel::OnSelChangedEventTree);
    m_pickButton = ui->EVENT_ADD;
    //connect(m_pickButton, &QPushButton::clicked, this, &CEntityEventsPanel::OnEventAdd);
    //m_pickButton.SetPickCallback(this, _T("Pick Target Entity for Event"), RUNTIME_CLASS(CEntityObject));
    m_addMissionBtn = ui->EVENT_ADDMISSION;
    connect(m_addMissionBtn, &QPushButton::clicked, this, &CEntityEventsPanel::OnBnAddMission);
    m_methods = ui->METHODS;
    connect(m_methods, &QListWidget::itemDoubleClicked, this, &CEntityEventsPanel::OnDblclkMethods);
    connect(m_methods, &QListWidget::itemSelectionChanged, this, &CEntityEventsPanel::OnSelChangedMethods);
}

CEntityEventsPanel::~CEntityEventsPanel()
{
    if (m_pickTool == GetIEditor()->GetEditTool())
    {
        GetIEditor()->SetEditTool(0);
    }
    m_pickTool = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CEntityEventsPanel message handlers
void CEntityEventsPanel::SetEntity(CEntityObject* entity)
{
    assert(entity);

    m_entity = entity;
    ReloadMethods();
    ReloadEvents();
}

void CEntityEventsPanel::ReloadMethods()
{
    assert(m_entity != 0);

    // Parse entity lua file.
    CEntityScript* script = m_entity->GetScript();

    // Since method CEntityScriptDialog::SetScript, checks for script, we are checking here too.
    if (!script)
    {
        return;
    }

    m_methods->clear();
    ///if (script->Load( m_entity->GetEntityClass() ))
    {
        for (int i = 0; i < script->GetMethodCount(); i++)
        {
            m_methods->addItem(script->GetMethod(i));
        }
    }
}

void CEntityEventsPanel::OnSelChangedMethods()
{
    m_selectedMethod = m_methods->selectedItems().size() ? m_methods->selectedItems().front()->text() : QStringLiteral("");
}

void CEntityEventsPanel::OnDblclkMethods(QListWidgetItem* item)
{
    GotoMethod(m_selectedMethod);
}

void CEntityEventsPanel::OnRunMethod()
{
    assert(m_entity != 0);

    CEntityScript* script = m_entity->GetScript();
    if (m_entity->GetIEntity())
    {
        script->RunMethod(m_entity->GetIEntity(), m_selectedMethod);
    }
}

void CEntityEventsPanel::GotoMethod(const QString& method)
{
    assert(m_entity != 0);
    CEntityScript* script = m_entity->GetScript();
    script->GotoMethod(method);
}

void CEntityEventsPanel::OnGotoMethod()
{
    GotoMethod(m_selectedMethod);
}

void CEntityEventsPanel::OnAddMethod()
{
    assert(m_entity != 0);

    bool ok;
    QString method = QInputDialog::getText(this, tr("Add Method"), tr("Enter Method Name:"), QLineEdit::Normal, QStringLiteral(""), &ok);
    if (ok && !method.isEmpty())
    {
        if (m_methods->findItems(method, Qt::MatchExactly).isEmpty())
        {
            CEntityScript* script = m_entity->GetScript();

            script->AddMethod(method);
            script->GotoMethod(method);

            script->Reload();
            m_entity->Reload(true);
        }
    }
}

void CEntityEventsPanel::ReloadEvents()
{
    assert(m_entity != 0);
    CEntityScript* script = m_entity->GetScript();

    int i;

    // Reload events tree.
    m_eventTree->clear();
    for (i = 0; i < script->GetEventCount(); i++)
    {
        QString sourceEvent = script->GetEvent(i);
        QTreeWidgetItem* hRootItem = new QTreeWidgetItem();
        hRootItem->setText(0, QString("On %1").arg(sourceEvent));
        m_eventTree->addTopLevelItem(hRootItem);

        bool haveEvents = false;
        for (int j = 0; j < m_entity->GetEventTargetCount(); j++)
        {
            QString targetName;
            CEntityEventTarget& et = m_entity->GetEventTarget(j);
            if (sourceEvent.compare(et.sourceEvent, Qt::CaseInsensitive) != 0)
            {
                continue;
            }

            if (et.target)
            {
                targetName = et.target->GetName();
            }
            else
            {
                targetName = "Mission";
            }

            targetName += QString(" [%1]").arg(et.event);
            QTreeWidgetItem* hEventItem = new QTreeWidgetItem(hRootItem);
            hEventItem->setText(0, targetName);
            hEventItem->setIcon(0, QIcon(":/Panels/EntityEventsPanel/res/icon_dot.png"));
            hEventItem->setData(0, Qt::UserRole, j);

            haveEvents = true;
        }
        if (haveEvents)
        {
            hRootItem->setExpanded(true);
            QFont f = hRootItem->font(0);
            f.setBold(true);
            hRootItem->setFont(0, f);
        }
    }
    m_pickButton->setEnabled(false);
    m_removeButton->setEnabled(false);
    m_sendEvent->setEnabled(false);
    m_addMissionBtn->setEnabled(false);
    m_currentTrgEventId = -1;
    m_currentSourceEvent = "";
}

void CEntityEventsPanel::OnEventAdd()
{
    // KDAB_PORT never called
    assert(m_entity != 0);
    //m_entity->PickEntity();

    if (m_pickTool)
    {
        // If pick tool already enabled, disable it.
        OnCancelPick();
    }

    GetIEditor()->PickObject(this, &CEntityObject::staticMetaObject, "Pick Target Entity for Event");
    m_pickButton->setChecked(true);
}

//////////////////////////////////////////////////////////////////////////
void CEntityEventsPanel::OnEventRemove()
{
    if (m_currentTrgEventId >= 0)
    {
        {
            CUndo undo("Remove Event Target");
            m_entity->RemoveEventTarget(m_currentTrgEventId);
        }
        ReloadEvents();
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityEventsPanel::OnBnAddMission()
{
    assert(m_entity);

    if (!m_currentSourceEvent.isEmpty())
    {
        CMissionScript* script = GetIEditor()->GetDocument()->GetCurrentMission()->GetScript();
        if (!script)
        {
            return;
        }

        if (script->GetEventCount() < 1)
        {
            return;
        }

        // Popup Menu with Event selection.
        QMenu menu;
        for (int i = 0; i < script->GetEventCount(); i++)
        {
            menu.addAction(script->GetEvent(i))->setData(i + 1);
        }

        QAction* action = menu.exec(QCursor::pos());
        int res = action ? action->data().toInt() : 0;
        if (res > 0 && res < script->GetEventCount() + 1)
        {
            CUndo undo("Change Event");

            QString event = script->GetEvent(res - 1);
            m_entity->AddEventTarget(0, event, m_currentSourceEvent);
            // Update script event table.
            if (m_entity->GetScript())
            {
                m_entity->GetScript()->SetEventsTable(m_entity);
            }
            ReloadEvents();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityEventsPanel::OnPick(CBaseObject* picked)
{
    m_pickTool = 0;

    CEntityObject* pickedEntity = (CEntityObject*)picked;
    if (!pickedEntity)
    {
        return;
    }

    m_pickButton->setChecked(false);
    if (pickedEntity->GetScript()->GetEventCount() > 0)
    {
        if (!m_currentSourceEvent.isEmpty())
        {
            CUndo undo("Add Event Target");
            m_entity->AddEventTarget(pickedEntity, pickedEntity->GetScript()->GetEvent(0), m_currentSourceEvent);
            if (m_entity->GetScript())
            {
                m_entity->GetScript()->SetEventsTable(m_entity);
            }
            ReloadEvents();
        }
    }
}

void CEntityEventsPanel::OnCancelPick()
{
    m_pickButton->setChecked(false);
    m_pickTool = 0;
}

void CEntityEventsPanel::OnSelChangedEventTree()
{
    assert(m_entity != 0);

    QTreeWidgetItem* selectedItem = m_eventTree->selectedItems().empty() ? nullptr : m_eventTree->selectedItems().front();
    QString str;
    if (selectedItem)
    {
        m_currentSourceEvent = selectedItem->text(0);
        m_currentTrgEventId = -1;

        //////////////////////////////////////////////////////////////////////////
        // Timur: Old system disabled for now.
        //////////////////////////////////////////////////////////////////////////
        //m_pickButton->setEnabled(true);
        m_removeButton->setEnabled(false);
        m_sendEvent->setEnabled(true);
        m_addMissionBtn->setEnabled(true);

        if (selectedItem->data(0, Qt::UserRole).isValid())
        {
            int id = selectedItem->data(0, Qt::UserRole).toInt();
            m_currentSourceEvent = m_entity->GetEventTarget(id).sourceEvent;
            m_currentTrgEventId = id;
            m_pickButton->setEnabled(false);
            m_removeButton->setEnabled(true);
            m_sendEvent->setEnabled(true);
            m_addMissionBtn->setEnabled(false);
        }
    }
}

void CEntityEventsPanel::OnRclickEventTree()
{
    // TODO: Add your control notification handler code here
    if (m_currentTrgEventId >= 0)
    {
        CEntityScript* script = 0;
        CMissionScript* missionScript = 0;
        int eventCount = 0;

        // Popup Menu with Event selection.
        QMenu menu;

        CBaseObject* trgObject = m_entity->GetEventTarget(m_currentTrgEventId).target;
        if (trgObject != 0)
        {
            CEntityObject* targetEntity = (CEntityObject*)trgObject;
            if (!targetEntity)
            {
                return;
            }

            script = targetEntity->GetScript();
            if (!script)
            {
                return;
            }

            eventCount = script->GetEventCount();

            for (int i = 0; i < eventCount; i++)
            {
                menu.addAction(script->GetEvent(i))->setData(i + 1);
            }
        }
        else
        {
            missionScript = GetIEditor()->GetDocument()->GetCurrentMission()->GetScript();
            if (!missionScript)
            {
                return;
            }
            eventCount = missionScript->GetEventCount();
            for (int i = 0; i < eventCount; i++)
            {
                menu.addAction(missionScript->GetEvent(i))->setData(i + 1);
            }
        }

        QAction* action = menu.exec(QCursor::pos());
        int res = action ? action->data().toInt() : 0;
        if (res > 0 && res < eventCount + 1)
        {
            CUndo undo("Change Event");
            QString event;
            if (script)
            {
                event = script->GetEvent(res - 1);
            }
            else if (missionScript)
            {
                event = missionScript->GetEvent(res - 1);
            }

            m_entity->GetEventTarget(m_currentTrgEventId).event = event;
            // Update script event table.
            if (m_entity->GetScript())
            {
                m_entity->GetScript()->SetEventsTable(m_entity);
            }
            ReloadEvents();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityEventsPanel::OnDblClickEventTree()
{
    /*
    if (m_currentTrgEventId >= 0)
    {
        CBaseObject *trgObject = m_entity->GetEventTarget(m_currentTrgEventId).target;
        if (trgObject != 0)
        {
            CUndo undo("Select Object" );
            GetIEditor()->ClearSelection();
            GetIEditor()->SelectObject( trgObject );
        }
    }*/
    if (!m_currentSourceEvent.isEmpty())
    {
        CEntityScript* script = m_entity->GetScript();
        if (m_entity->GetIEntity())
        {
            script->SendEvent(m_entity->GetIEntity(), m_currentSourceEvent);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEntityEventsPanel::OnEventSend()
{
    if (!m_currentSourceEvent.isEmpty())
    {
        CEntityScript* script = m_entity->GetScript();
        if (m_entity->GetIEntity())
        {
            script->SendEvent(m_entity->GetIEntity(), m_currentSourceEvent);
        }
    }
}


#include <EntityPanel.moc>
