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

#ifndef CRYINCLUDE_EDITOR_HYPERGRAPH_QUICKSEARCHNODE_H
#define CRYINCLUDE_EDITOR_HYPERGRAPH_QUICKSEARCHNODE_H
#pragma once


#include "HyperGraphNode.h"

class CQuickSearchNode
    : public CHyperNode
{
public:
    static const char* GetClassType()
    {
        return "QuickSearch";
    }
    CQuickSearchNode(void);
    virtual ~CQuickSearchNode(void);

    // CHyperNode overwrites
    void Init();
    void Done();
    CHyperNode* Clone();
    virtual bool IsEditorSpecialNode() { return true; }
    virtual bool IsFlowNode() { return false; }
    void SetSearchResultCount(int count){m_iSearchResultCount = count; };
    int GetSearchResultCount(){return m_iSearchResultCount; };

    void SetIndex(int index){m_iIndex = index; };
    int GetIndex(){return m_iIndex; };

private:
    int m_iSearchResultCount;
    int m_iIndex;
};

#endif // CRYINCLUDE_EDITOR_HYPERGRAPH_QUICKSEARCHNODE_H
