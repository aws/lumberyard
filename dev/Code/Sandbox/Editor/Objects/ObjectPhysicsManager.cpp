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
#include "ObjectPhysicsManager.h"
#include "BaseObject.h"
#include "GameEngine.h"
#include "Util/BoostPythonHelpers.h"

#define MAX_OBJECTS_PHYS_SIMULATION_TIME (5)

namespace
{
    void PyPhysicsResetState(const char* objectName)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(objectName);
        if (pObject)
        {
            pObject->OnEvent(EVENT_PHYSICS_RESETSTATE);
        }
    }

    void PyPhysicsGetState(const char* objectName)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(objectName);
        if (pObject)
        {
            pObject->OnEvent(EVENT_PHYSICS_GETSTATE);
        }
    }

    void PyPhysicsSimulateSelection()
    {
        CObjectPhysicsManager* pPhysicsManager
            = static_cast<CObjectManager*>(GetIEditor()->GetObjectManager())->GetPhysicsManager();
        if (pPhysicsManager)
        {
            pPhysicsManager->SimulateSelectedObjectsPositions();
        }
    }
}

REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyPhysicsResetState, physics, reset_state,
    "Signals that physics state must be reset on the object.",
    "physics.reset_state(str entityName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyPhysicsGetState, physics, get_state,
    "Signals that the object should accept its physical state from game.",
    "physics.get_state(str entityName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyPhysicsSimulateSelection, physics, simulate_selection,
    "Applies the physics simulation to the selected objects.",
    "physics.simulate_selection()");

//////////////////////////////////////////////////////////////////////////
CObjectPhysicsManager::CObjectPhysicsManager()
{
    CommandManagerHelper::RegisterCommand(GetIEditor()->GetCommandManager(),
        "physics", "simulate_objects", "", "",
        functor(*this, &CObjectPhysicsManager::Command_SimulateObjects));
    CommandManagerHelper::RegisterCommand(GetIEditor()->GetCommandManager(),
        "physics", "reset_objects_state", "", "",
        functor(*this, &CObjectPhysicsManager::Command_ResetPhysicsState));
    CommandManagerHelper::RegisterCommand(GetIEditor()->GetCommandManager(),
        "physics", "get_objects_state", "", "",
        functor(*this, &CObjectPhysicsManager::Command_GetPhysicsState));

    m_fStartObjectSimulationTime = 0;
    m_bSimulatingObjects = false;
    m_wasSimObjects = 0;
}

//////////////////////////////////////////////////////////////////////////
CObjectPhysicsManager::~CObjectPhysicsManager()
{

}

//////////////////////////////////////////////////////////////////////////
void CObjectPhysicsManager::Command_SimulateObjects()
{
    SimulateSelectedObjectsPositions();
}

/////////////////////////////////////////////////////////////////////////
void CObjectPhysicsManager::Command_ResetPhysicsState()
{
    CSelectionGroup* pSelection = GetIEditor()->GetSelection();
    for (int i = 0; i < pSelection->GetCount(); i++)
    {
        pSelection->GetObject(i)->OnEvent(EVENT_PHYSICS_RESETSTATE);
    }
}

/////////////////////////////////////////////////////////////////////////
void CObjectPhysicsManager::Command_GetPhysicsState()
{
    CSelectionGroup* pSelection = GetIEditor()->GetSelection();
    for (int i = 0; i < pSelection->GetCount(); i++)
    {
        pSelection->GetObject(i)->OnEvent(EVENT_PHYSICS_GETSTATE);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectPhysicsManager::Update()
{
    if (m_bSimulatingObjects)
    {
        UpdateSimulatingObjects();
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectPhysicsManager::SimulateSelectedObjectsPositions()
{
    CSelectionGroup* pSel = GetIEditor()->GetObjectManager()->GetSelection();
    if (pSel->IsEmpty())
    {
        return;
    }

    if (GetIEditor()->GetGameEngine()->GetSimulationMode())
    {
        return;
    }

    GetIEditor()->GetGameEngine()->SetSimulationMode(true, true);

    m_simObjects.clear();
    for (int i = 0; i < pSel->GetCount(); i++)
    {
        CBaseObject* pObject = pSel->GetObject(i);
        IPhysicalEntity* pPhysEntity = pObject->GetCollisionEntity();
        if (!pPhysEntity)
        {
            continue;
        }

        pe_action_awake aa;
        aa.bAwake = 1;
        pPhysEntity->Action(&aa);

        m_simObjects.push_back(pSel->GetObject(i));
    }
    m_wasSimObjects = m_simObjects.size();

    m_fStartObjectSimulationTime = GetISystem()->GetITimer()->GetAsyncCurTime();
    m_bSimulatingObjects = true;
}

//////////////////////////////////////////////////////////////////////////
void CObjectPhysicsManager::UpdateSimulatingObjects()
{
    {
        CUndo undo("Simulate");
        for (int i = 0; i < m_simObjects.size(); )
        {
            CBaseObject* pObject = m_simObjects[i];
            IPhysicalEntity* pPhysEntity = pObject->GetCollisionEntity();
            if (pPhysEntity)
            {
                pe_status_awake awake_status;
                if (!pPhysEntity->GetStatus(&awake_status))
                {
                    // When object went sleeping, record his position.
                    pObject->OnEvent(EVENT_PHYSICS_GETSTATE);

                    m_simObjects.erase(m_simObjects.begin() + i);
                    continue;
                }
            }
            i++;
        }
    }

    float curTime = GetISystem()->GetITimer()->GetAsyncCurTime();
    float runningTime = (curTime - m_fStartObjectSimulationTime);

    if (m_simObjects.empty() || (runningTime > MAX_OBJECTS_PHYS_SIMULATION_TIME))
    {
        m_fStartObjectSimulationTime = 0;
        m_bSimulatingObjects = false;
        GetIEditor()->GetGameEngine()->SetSimulationMode(false, true);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectPhysicsManager::PrepareForExport()
{
    // Clear the collision class set, ready for objects to register
    // their collision classes
    m_collisionClasses.clear();
    m_collisionClassExportId = 0;
    // First collision-class IS always the default one
    RegisterCollisionClass(SCollisionClass(0, 0));
}

//////////////////////////////////////////////////////////////////////////
bool operator == (const SCollisionClass& lhs, const SCollisionClass& rhs)
{
    return lhs.type == rhs.type && lhs.ignore == rhs.ignore;
}

//////////////////////////////////////////////////////////////////////////
int CObjectPhysicsManager::RegisterCollisionClass(const SCollisionClass& collclass)
{
    TCollisionClassVector::iterator it = std::find(m_collisionClasses.begin(), m_collisionClasses.end(), collclass);
    if (it == m_collisionClasses.end())
    {
        m_collisionClasses.push_back(collclass);
        return m_collisionClasses.size() - 1;
    }
    return it - m_collisionClasses.begin();
}

//////////////////////////////////////////////////////////////////////////
int CObjectPhysicsManager::GetCollisionClassId(const SCollisionClass& collclass)
{
    TCollisionClassVector::iterator it = std::find(m_collisionClasses.begin(), m_collisionClasses.end(), collclass);
    if (it == m_collisionClasses.end())
    {
        return 0;
    }
    return it - m_collisionClasses.begin();
}

//////////////////////////////////////////////////////////////////////////
void CObjectPhysicsManager::SerializeCollisionClasses(CXmlArchive& xmlAr)
{
    if (!xmlAr.bLoading)
    {
        // Storing
        CLogFile::WriteLine("Storing Collision Classes ...");

        XmlNodeRef root = xmlAr.root->newChild("CollisionClasses");
        int count = m_collisionClasses.size();
        for (int i = 0; i < count; i++)
        {
            SCollisionClass& cc = m_collisionClasses[i];
            XmlNodeRef xmlCC = root->newChild("CollisionClass");
            xmlCC->setAttr("type", cc.type);
            xmlCC->setAttr("ignore", cc.ignore);
        }
    }
}



