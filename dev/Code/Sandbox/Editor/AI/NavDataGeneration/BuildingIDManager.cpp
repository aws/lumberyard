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
#include "BuildingIDManager.h"
#include "AILog.h"

//===================================================================
// CBuildingIDManager
//===================================================================
CBuildingIDManager::CBuildingIDManager()
{
}

//===================================================================
// ~CBuildingIDManager
//===================================================================
CBuildingIDManager::~CBuildingIDManager()
{
}

//===================================================================
// MakeSomeIDsIfNecessary
//===================================================================
void CBuildingIDManager::MakeSomeIDsIfNecessary()
{
    const int numToMake = 64;
    if (m_availableIDs.empty())
    {
        int minFreeID = m_allocatedIDs.size();
        for (int i = numToMake; i-- != 0; )
        {
            m_availableIDs.push_back(minFreeID + i);
        }
    }
}

//===================================================================
// GetId
//===================================================================
int CBuildingIDManager::GetId()
{
    MakeSomeIDsIfNecessary();

    int ID = m_availableIDs.back();
    m_availableIDs.pop_back();

    AIAssert(m_allocatedIDs.find(ID) == m_allocatedIDs.end());

    m_allocatedIDs.insert(ID);

    return ID;
}



//===================================================================
// FreeId
//===================================================================
void CBuildingIDManager::FreeId(int nID)
{
    if (nID < 0)
    {
        return;
    }
    std::set<int>::iterator it = m_allocatedIDs.find(nID);
    if (it == m_allocatedIDs.end())
    {
        // not an assert because we're not in control of the input
        AIWarning("CBuildingIDManager::FreeId ID %d not found", nID);
    }
    else
    {
        m_allocatedIDs.erase(it);
        m_availableIDs.push_back(nID);
    }
}

//===================================================================
// FreeAll
//===================================================================
void CBuildingIDManager::FreeAll(void)
{
    m_allocatedIDs.clear();
    m_availableIDs.clear();
}
