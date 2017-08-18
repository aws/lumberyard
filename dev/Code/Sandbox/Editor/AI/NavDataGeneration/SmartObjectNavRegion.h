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

#ifndef CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_SMARTOBJECTNAVREGION_H
#define CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_SMARTOBJECTNAVREGION_H
#pragma once


#include "NavRegion.h"

class CGraph;

/// Handles all graph operations that relate to the smart objects aspect
class CSmartObjectNavRegion
    : public CNavRegion
{
public:
    CSmartObjectNavRegion(CGraph* pGraph)
    {
        m_pGraph = pGraph;
    }
    virtual ~CSmartObjectNavRegion() {}

    /// inherited
    virtual unsigned GetEnclosing(const Vec3& pos, float passRadius = 0.0f, unsigned startIndex = 0,
        float range = -1.0f, Vec3* closestValid = 0, bool returnSuspect = false, const char* requesterName = "")
    {
        return 0;
    }

    /// Serialise the _modifications_ since load-time
    virtual void Serialize(TSerialize ser)
    {
        //ser.BeginGroup("SmartObjectNavRegion");
        //ser.EndGroup();
    }

    /// inherited
    virtual void Clear()
    {
        // remove all smart object nodes
        m_pGraph->DeleteGraph(IAISystem::NAV_SMARTOBJECT);

        //      CAllNodesContainer& allNodes = m_pGraph->GetAllNodes();
        //      CAllNodesContainer::Iterator it( allNodes, IAISystem::NAV_SMARTOBJECT );
        //      GraphNode* pNode;
        //      while ( pNode = it.GetNode() )
        //      {
        //          m_pGraph->Disconnect( pNode, true );
        //          it = allNodes.Erase( it );
        //      }
    }

    /// inherited
    virtual size_t MemStats()
    {
        size_t size = sizeof(*this);
        if (m_pGraph)
        {
            size += m_pGraph->NodeMemStats(IAISystem::NAV_SMARTOBJECT);
        }
        return size;
    }

private:
    CGraph* m_pGraph;
};

#endif // CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_SMARTOBJECTNAVREGION_H
