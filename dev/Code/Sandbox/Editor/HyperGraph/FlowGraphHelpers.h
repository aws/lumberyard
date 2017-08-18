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

// Description : some helpers for flowgraphs


#ifndef CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHHELPERS_H
#define CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHHELPERS_H
#pragma once


#include <vector>

class CEntityObject;
class CFlowGraph;

namespace FlowGraphHelpers
{
    // Description:
    //     Get human readable name for the FlowGraph
    //     incl. group, entity, AI action
    // Arguments:
    //     pFlowGraph - Ptr to flowgraph
    //     outName    - CString with human name
    // Return Value:
    //     none
    SANDBOX_API void GetHumanName(CFlowGraph* pFlowGraph, QString& outName);

    // Description:
    //     Get all flowgraphs an entity is used in
    // Arguments:
    //     pEntity            - Entity
    //     outFlowGraphs      - Vector of all flowgraphs (sorted by HumanName)
    //     outEntityFlowGraph - If the entity is owner of a flowgraph this is the pointer to
    // Return Value:
    //     none
    SANDBOX_API void FindGraphsForEntity(CEntityObject* pEntity, std::vector<CFlowGraph*>& outFlowGraphs, CFlowGraph*& outEntityFlowGraph);
};

#endif // CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHHELPERS_H
