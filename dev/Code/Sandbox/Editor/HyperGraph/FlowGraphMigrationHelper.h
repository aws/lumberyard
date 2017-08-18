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

#ifndef CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHMIGRATIONHELPER_H
#define CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHMIGRATIONHELPER_H
#pragma once


#include "IHyperGraph.h"

#include <map>

class CFlowGraphMigrationHelper
{
public:
    // forward decl of NodeEntry
    struct NodeEntry;

    // Condition interface
    struct ICondition
        : public _i_reference_target_t
    {
        // is the Condition fulfilled?
        virtual bool Fulfilled(XmlNodeRef node, NodeEntry& entry) = 0;
    };
    typedef _smart_ptr<ICondition> IConditionPtr;
    typedef std::vector<IConditionPtr> ConditionVector;

    typedef struct PortEntry
    {
        PortEntry()
            : transformFunc (0)
            , bRemapValue (false)
            , bIsOutput (false) {}
        QString newName;
        HSCRIPTFUNCTION transformFunc;
        bool    bRemapValue;
        bool    bIsOutput;
    } PortEntry;
    typedef std::map<QString, PortEntry, stl::less_stricmp<QString> > PortMappings;
    typedef std::map<QString, QString, stl::less_stricmp<QString> > InputValues;

    typedef struct NodeEntry
    {
        QString newName;
        ConditionVector conditions;
        PortMappings inputPortMappings;
        PortMappings outputPortMappings;
        InputValues inputValues;
    } NodeEntry;
    typedef std::map<QString, NodeEntry, stl::less_stricmp<QString> > NodeMappings;

    typedef struct ReportEntry
    {
        HyperNodeID nodeId;
        QString     description;
    } ReportEntry;

public:
    CFlowGraphMigrationHelper();
    ~CFlowGraphMigrationHelper();

    // Substitute (and alter) the node
    // node must be a Graph XML node
    // returns true if something changed, false otherwise
    bool Substitute(XmlNodeRef node);

    // Get the substitution report
    const std::vector<ReportEntry>& GetReport() const;

protected:
    bool LoadSubstitutions();
    void AddEntry(XmlNodeRef node);
    bool EvalConditions(XmlNodeRef node, NodeEntry& entry);
    void ReleaseLUAFuncs();

protected:
    bool m_bLoaded;
    bool m_bInitialized;
    NodeMappings m_nodeMappings;
    std::vector<ReportEntry> m_report;
    std::vector<HSCRIPTFUNCTION> m_transformFuncs;
};

#endif // CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHMIGRATIONHELPER_H
