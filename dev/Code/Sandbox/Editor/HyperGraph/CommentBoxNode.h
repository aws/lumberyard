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

#ifndef CRYINCLUDE_EDITOR_HYPERGRAPH_COMMENTBOXNODE_H
#define CRYINCLUDE_EDITOR_HYPERGRAPH_COMMENTBOXNODE_H
#pragma once


#include "HyperGraphNode.h"

class CCommentBoxNode
    : public CHyperNode
{
public:
    static const char* GetClassType()
    {
        return "_commentbox";
    }
    CCommentBoxNode();
    ~CCommentBoxNode();
    void Init();
    void Done();
    CHyperNode* Clone();
    void Serialize(XmlNodeRef& node, bool bLoading, CObjectArchive* ar) override;

    void SetResizeBorderRect(const QRectF& newRelBordersRect) override;
    virtual void SetBorderRect(const QRectF& newAbsBordersRect);
    bool IsGridBound() override { return true; }
    const QRectF* GetResizeBorderRect() const override
    {
        return &m_resizeBorderRect;
    }
    void OnZoomChange(float zoom) override;
    void OnInputsChanged() override;
    bool IsEditorSpecialNode() override { return true; }
    bool IsFlowNode() override { return false; }
    bool IsTooltipShowable() override { return false; }

    void SetPos(const QPointF& pos) override;

    enum FontSize
    {
        FontSize_Small = 1,
        FontSize_Medium = 2,
        FontSize_Large = 3
    };

private:
    void OnPossibleSizeChange();
    QRectF m_resizeBorderRect;
};

#endif // CRYINCLUDE_EDITOR_HYPERGRAPH_COMMENTBOXNODE_H
