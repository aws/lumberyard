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
#ifndef CRYINCLUDE_EDITOR_HYPERGRAPH_QHYPERNODEPAINTER_DEFAULT_H
#define CRYINCLUDE_EDITOR_HYPERGRAPH_QHYPERNODEPAINTER_DEFAULT_H
#pragma once


#include "IQHyperNodePainter.h"

class QHyperNodePainter_Default
    : public IQHyperNodePainter
{
public:
    virtual void Paint(CHyperNode* pNode, QDisplayList* pList);
private:
    void AddDownArrow(QDisplayList* pList, const QPointF& p, const QPen& pPen);
    void CheckBreakpoint(IFlowGraphPtr pFlowGraph, const SFlowAddress& addr, bool& bIsBreakPoint, bool& bIsTracepoint);
};

#endif // CRYINCLUDE_EDITOR_HYPERGRAPH_QHYPERNODEPAINTER_DEFAULT_H
