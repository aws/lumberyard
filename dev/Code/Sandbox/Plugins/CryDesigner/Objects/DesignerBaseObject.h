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
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   DesignerBaseObject.h
//  Created:     12/June/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "Core/ModelCompiler.h"
#include "Core/Model.h"

template<class T>
class DesignerBaseObject
    : public T
{
public:

    DesignerBaseObject()
    {
        m_pModel = new CD::Model;
    }

    void Done() override;

    void SetCompiler(CD::ModelCompiler* pCompiler);
    virtual CD::ModelCompiler* GetCompiler() const;

    CD::SMainContext MainContext() const { return CD::SMainContext((CBaseObject*)this, GetCompiler(), GetModel()); }

    void SetModel(CD::Model* pModel);
    CD::Model* GetModel() const { return m_pModel; }
    void Merge(CBaseObject* pTargetObj, CD::Model* pTargetModel);

    void UpdateEngineNode();
    IStatObj* GetIStatObj() override;

    virtual void UpdateGameResource() {};

    bool QueryNearestPos(const BrushVec3& worldPos, BrushVec3& outPos) const;
    void DrawDimensions(DisplayContext& dc, AABB* pMergedBoundBox) override {}

    void Validate(IErrorReport* report) override;
    bool IsEmpty() const { return m_pModel->IsEmpty(0); }

protected:

    _smart_ptr<CD::Model> m_pModel;
    mutable _smart_ptr<CD::ModelCompiler> m_pCompiler;
};

#include "DesignerBaseObject_Impl.h"