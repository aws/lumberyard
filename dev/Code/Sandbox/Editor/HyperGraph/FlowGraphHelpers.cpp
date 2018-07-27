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
#include "FlowGraphHelpers.h"

#include <Objects/EntityObject.h>
#include <IAIAction.h>
#include <HyperGraph/FlowGraphManager.h>
#include <HyperGraph/FlowGraph.h>

namespace {
    void GetRealName(CFlowGraph* pFG, QString& outName)
    {
        CEntityObject* pEntity = pFG->GetEntity();
        IAIAction* pAIAction = pFG->GetAIAction();
        outName = pFG->GetGroupName();
        if (!outName.isEmpty())
        {
            outName += QStringLiteral(":");
        }
        if (pEntity)
        {
            outName += pEntity->GetName();
        }
        else if (pAIAction)
        {
            outName += pAIAction->GetName();
            outName += QStringLiteral(" <AI>");
        }
        else
        {
            outName += pFG->GetName();
        }
    }

    struct CmpByName
    {
        bool operator() (CFlowGraph* lhs, CFlowGraph* rhs) const
        {
            QString lName;
            QString rName;
            GetRealName(lhs, lName);
            GetRealName(rhs, rName);
            return lName.compare(rName, Qt::CaseInsensitive) < 0;
        }
    };
}

//////////////////////////////////////////////////////////////////////////
namespace FlowGraphHelpers
{
    void GetHumanName(CFlowGraph* pFlowGraph, QString& outName)
    {
        GetRealName(pFlowGraph, outName);
    }

    void FindGraphsForEntity(CEntityObject* pEntity, std::vector<CFlowGraph*>& outFlowGraphs, CFlowGraph*& outEntityFG)
    {
        if (pEntity)
        {
            typedef std::vector<TFlowNodeId> TNodeIdVec;
            typedef std::map<CFlowGraph*, TNodeIdVec, std::less<CFlowGraph*> > TFGMap;
            TFGMap fgMap;
            CFlowGraph* pEntityGraph = NULL;

            IEntitySystem* pEntSys = gEnv->pEntitySystem;
            EntityId myId = pEntity->GetEntityId();
            CFlowGraphManager* pFGMgr = GetIEditor()->GetFlowGraphManager();
            int numFG = pFGMgr->GetFlowGraphCount();
            for (int i = 0; i < numFG; ++i)
            {
                CFlowGraph* pFG = pFGMgr->GetFlowGraph(i);

                // AI Action may not reference actual entities
                if (pFG->GetAIAction() != 0)
                {
                    continue;
                }

                IFlowGraphPtr pGameFG = pFG->GetIFlowGraph();
                if (pGameFG == 0)
                {
                    if (gEnv->pSystem->GetIGame())
                    {
                        CryLogAlways("FlowGraphHelpers::FindGraphsForEntity: No Game FG for FlowGraph 0x%p", pFG);
                    }
                }
                else
                {
                    if (pGameFG->GetGraphEntity(0) == myId ||
                        pGameFG->GetGraphEntity(1) == myId)
                    {
                        pEntityGraph = pFG;
                        fgMap[pFG].push_back(InvalidFlowNodeId);
                        //                  CryLogAlways("entity as graph entity: %p\n",pFG);
                    }
                    IFlowNodeIteratorPtr nodeIter (pGameFG->CreateNodeIterator());
                    TFlowNodeId nodeId;
                    while (IFlowNodeData* pNodeData = nodeIter->Next(nodeId))
                    {
                        EntityId id = pGameFG->GetEntityId(nodeId);
                        if (myId == id && nodeId != InvalidFlowNodeId)
                        {
                            //                  CryLogAlways("  node entity for id %d: %p\n",nodeId, pEntSys->GetEntity(id));
                            fgMap[pFG].push_back(nodeId);
                        }
                    }
                }
            }

            //      CryLogAlways("found %d unique graphs",fgMap.size());

            typedef std::vector<CFlowGraph*> TFGVec;
            TFGVec fgSortedVec;
            fgSortedVec.reserve(fgMap.size());

            // if there's an entity graph, put it in front
            if (pEntityGraph != NULL)
            {
                fgSortedVec.push_back(pEntityGraph);
            }

            // fill in the rest
            for (TFGMap::const_iterator iter = fgMap.begin(); iter != fgMap.end(); ++iter)
            {
                if ((*iter).first != pEntityGraph)
                {
                    fgSortedVec.push_back((*iter).first);
                }
            }

            // sort rest of list by name
            if (fgSortedVec.size() > 1)
            {
                if (pEntityGraph != NULL)
                {
                    std::sort(fgSortedVec.begin() + 1, fgSortedVec.end(), CmpByName());
                }
                else
                {
                    std::sort(fgSortedVec.begin(), fgSortedVec.end(), CmpByName());
                }
            }

            outFlowGraphs.clear();
            outFlowGraphs.insert(outFlowGraphs.end(), fgSortedVec.begin(), fgSortedVec.end());

            outEntityFG = pEntityGraph;
        }
        else
        {
            outFlowGraphs.resize(0);
            outEntityFG = 0;
        }
    }
};