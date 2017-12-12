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

#pragma once

#include "UICommon.h"

template<class _Tool>
class PropertyTreePanel
    : public CD::QPropertyTreeWidget
    , public IBasePanel
{
public:
    PropertyTreePanel(_Tool* pTool)
        : m_pTool(pTool)
    {
        CreatePropertyTree(m_pTool, [=](bool continuous){ m_pTool->OnChangeParameter(continuous); });
        CD::LoadSettings(Serialization::SStruct(*m_pTool), m_pTool->ToolClass());
        CD::SetAttributeWidget(m_pTool, this);
        Update();
    }

    void Done() override
    {
        CD::SaveSettings(Serialization::SStruct(*m_pTool), m_pTool->ToolClass());
        CD::RemoveAttributeWidget(m_pTool);
    }

    void Update() override
    {
        m_pPropertyTree->revert();
    }

private:

    _Tool* m_pTool;
};