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
#include "GraphStructures.h"

unsigned GraphNode::maxID = 0;
std::vector<unsigned> GraphNode::freeIDs;

GraphNode::GraphNode(IAISystem::ENavigationType type, const Vec3& inpos, unsigned int _ID)
    : navType(type)
    , pos(inpos)
{
    firstLinkIndex = 0;
    fCostFromStart = fInvalidCost;
    fEstimatedCostToGoal = fInvalidCost;
    prevPathNodeIndex = 0;
    mark = 0;
    nRefCount = 0;
    if (_ID == 0)
    {
        if (freeIDs.empty())
        {
            ID = maxID + 1;
        }
        else
        {
            ID = freeIDs.back();
            freeIDs.pop_back();
        }
    }
    else
    {
        ID = _ID;
    }
    if (ID > maxID)
    {
        maxID = ID;
    }
    tag = 0;
}

GraphNode::~GraphNode()
{
    freeIDs.push_back(ID);
}
