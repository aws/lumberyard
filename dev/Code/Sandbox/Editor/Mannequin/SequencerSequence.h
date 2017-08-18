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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_SEQUENCERSEQUENCE_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_SEQUENCERSEQUENCE_H
#pragma once



#include "ISequencerSystem.h"

class CSequencerSequence
    : public _i_reference_target_t
{
public:
    CSequencerSequence();
    virtual ~CSequencerSequence();

    void SetName(const char* name);
    const char* GetName() const;

    void SetTimeRange(Range timeRange);
    Range GetTimeRange() { return m_timeRange; }

    int GetNodeCount() const;
    CSequencerNode* GetNode(int index) const;

    CSequencerNode* FindNodeByName(const char* sNodeName);
    void ReorderNode(CSequencerNode* node, CSequencerNode* pPivotNode, bool next);

    bool AddNode(CSequencerNode* node);
    void RemoveNode(CSequencerNode* node);

    void RemoveAll();

    void UpdateKeys();

private:
    typedef std::vector< _smart_ptr<CSequencerNode> > SequencerNodes;
    SequencerNodes m_nodes;

    string m_name;
    Range m_timeRange;
};

#endif // CRYINCLUDE_EDITOR_MANNEQUIN_SEQUENCERSEQUENCE_H
