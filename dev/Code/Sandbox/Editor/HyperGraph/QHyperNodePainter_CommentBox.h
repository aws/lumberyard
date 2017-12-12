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
//////////////////////////////////////////////////////////////////////////
// draw object for CommentBox nodes
//////////////////////////////////////////////////////////////////////////

#ifndef CRYINCLUDE_EDITOR_HYPERGRAPH_QHYPERNODEPAINTER_COMMENTBOX_H
#define CRYINCLUDE_EDITOR_HYPERGRAPH_QHYPERNODEPAINTER_COMMENTBOX_H
#pragma once


#include "IQHyperNodePainter.h"

class QHyperNodePainter_CommentBox
    : public IQHyperNodePainter
{
public:
    QHyperNodePainter_CommentBox()
        : m_brushSolid(Qt::black)
        , m_brushTransparent(Qt::black)
    {}
    virtual void Paint(CHyperNode* pNode, QDisplayList* pList);
    static float& AccessStaticVar_ZoomFactor() { static float m_zoomFactor(1.f); return m_zoomFactor; }

private:
    QBrush m_brushSolid;
    QBrush m_brushTransparent;
};

#endif // CRYINCLUDE_EDITOR_HYPERGRAPH_QHYPERNODEPAINTER_COMMENTBOX_H
