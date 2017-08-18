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
#ifndef CRYINCLUDE_EDITOR_HYPERGRAPH_QHYPERNODEPAINTER_BLACKBOX_H
#define CRYINCLUDE_EDITOR_HYPERGRAPH_QHYPERNODEPAINTER_BLACKBOX_H
#pragma once


#include "QHyperNodePainterBase.h"

class QHyperNodePainter_BlackBox
    : public QHyperNodePainterBase
{
public:
    virtual void Paint(CHyperNode* pNode, QDisplayList* pList);

    QHyperNodePainter_BlackBox();

private:
    //! Color of the large rectangle when it isn't selected (Selected color is a linearly scaled value)
    QBrush m_groupFillBrush;

    //! Pen used to draw the border of the Blackbox
    QPen m_penBorder;
};

#endif // CRYINCLUDE_EDITOR_HYPERGRAPH_QHYPERNODEPAINTER_BLACKBOX_H
