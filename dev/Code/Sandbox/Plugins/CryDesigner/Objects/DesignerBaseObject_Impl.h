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

#include "Tools/DesignerTool.h"
#include "Core/BrushHelper.h"

template<class T>
void DesignerBaseObject<T>::Done()
{
    if (m_pCompiler)
    {
        m_pCompiler->DeleteAllRenderNodes();
        m_pCompiler = NULL;
    }

    T::Done();

    DesignerTool* pDesignerTool = CD::GetDesigner();
    if (!IsEmpty() && pDesignerTool && pDesignerTool->GetBaseObject() == this)
    {
        GetIEditor()->SetEditTool(NULL);
    }
}

template<class T>
void DesignerBaseObject<T>::SetCompiler(CD::ModelCompiler* pCompiler)
{
    DESIGNER_ASSERT(pCompiler);
    m_pCompiler = pCompiler;
}

template<class T>
CD::ModelCompiler* DesignerBaseObject<T>::GetCompiler() const
{
    if (!m_pCompiler)
    {
        m_pCompiler = new CD::ModelCompiler(CD::eCompiler_General);
    }
    return m_pCompiler;
}

template<class T>
void DesignerBaseObject<T>::SetModel(CD::Model* pModel)
{
    DESIGNER_ASSERT(pModel);
    m_pModel = pModel;
}

template<class T>
void DesignerBaseObject<T>::Merge(CBaseObject* pTargetObj, CD::Model* pTargetModel)
{
    if (pTargetObj == NULL || pTargetModel == NULL)
    {
        return;
    }

    MODEL_SHELF_RECONSTRUCTOR(GetModel());
    MODEL_SHELF_RECONSTRUCTOR_POSTFIX(pTargetModel, 1);

    BrushVec3 offset = pTargetObj->GetPos() - GetWorldTM().GetTranslation();
    Matrix34 targetTM(pTargetObj->GetWorldTM());
    targetTM.SetTranslation(offset);

    GetModel()->RecordUndo("Designer : Merge", this);

    for (CD::ShelfID shelfID = 0; shelfID < CD::kMaxShelfCount; ++shelfID)
    {
        GetModel()->SetShelf(shelfID);
        pTargetModel->SetShelf(shelfID);
        for (int i = 0, nPolygonSize(pTargetModel->GetPolygonCount()); i < nPolygonSize; ++i)
        {
            CD::PolygonPtr pTargetPolygon = pTargetModel->GetPolygon(i)->Clone();
            pTargetPolygon->Transform(targetTM);
            GetModel()->AddPolygon(pTargetPolygon, CD::eOpType_Add);
        }
    }
}

template<class T>
void DesignerBaseObject<T>::UpdateEngineNode()
{
    if (GetCompiler() == NULL)
    {
        return;
    }
    GetCompiler()->Compile(this, GetModel(), 0, true);
}

template<class T>
bool DesignerBaseObject<T>::QueryNearestPos(const BrushVec3& worldPos, BrushVec3& outPos) const
{
    MODEL_SHELF_RECONSTRUCTOR(GetModel());
    GetModel()->SetShelf(0);

    BrushVec3 modelPos = GetWorldTM().GetInverted().TransformPoint(worldPos);
    BrushVec3 outModelPos;
    if (GetModel()->QueryNearestPosFromBoundary(modelPos, outModelPos))
    {
        outPos = GetWorldTM().TransformPoint(outModelPos);
        return true;
    }
    return false;
}

template<class T>
void DesignerBaseObject<T>::Validate(IErrorReport* report)
{
    CBaseObject::Validate(report);
    if (!GetModel() || GetModel()->IsEmpty(0))
    {
        CErrorRecord err;
        err.error = QObject::tr("Empty Designer Object");
        err.pObject = this;
        report->ReportError(err);
    }
    else if (!CD::CheckIfHasValidModel(this))
    {
        CErrorRecord err;
        err.error = QObject::tr("This designer object consists of only edges not polygons.");
        err.pObject = this;
        report->ReportError(err);
    }
}

template<class T>
IStatObj* DesignerBaseObject<T>::GetIStatObj()
{
    if (!GetCompiler())
    {
        return NULL;
    }

    _smart_ptr<IStatObj> pStatObj = NULL;
    if (!GetCompiler()->GetIStatObj(&pStatObj))
    {
        return NULL;
    }

    return pStatObj;
}