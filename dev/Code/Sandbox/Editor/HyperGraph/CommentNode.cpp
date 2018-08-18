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
#include "CommentNode.h"
#include "QHyperNodePainter_Comment.h"

static QHyperNodePainter_Comment qpainter;

CCommentNode::CCommentNode()
{
    SetClass(GetClassType());
    m_pqPainter = &qpainter;
    m_name = "This is a comment";
}

void CCommentNode::Init()
{
}

void CCommentNode::Done()
{
}

CHyperNode* CCommentNode::Clone()
{
    CCommentNode* pNode = new CCommentNode();
    pNode->CopyFrom(*this);
    return pNode;
}

void CCommentNode::OnZoomChange(float zoom)
{
    if (m_qdispList.Empty())
    {
        Invalidate(true);
    }
}
