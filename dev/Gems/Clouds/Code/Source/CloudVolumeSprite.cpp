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

#include "CloudVolumeSprite.h"
#include "CloudVolumeTexture.h"
#include "CloudVolumeRenderElement.h"

namespace CloudsGem
{
    CloudVolumeSprite::CloudVolumeSprite()
    {
        m_worldMatrix.SetIdentity();
        m_localBoundingBox.Reset();
        m_worldBoundingBox.Reset();

        m_pREImposter = new CloudImposterRenderElement();
        m_renderElement = new CloudRenderElement();
    }

    CloudVolumeSprite::~CloudVolumeSprite()
    {
        SAFE_DELETE(m_pREImposter);
        SAFE_DELETE(m_renderElement);
    }

    void CloudVolumeSprite::Update(const Matrix34& worldMatrix, const Vec3& /*offset*/)
    {
        m_worldMatrix = worldMatrix;
        m_worldBoundingBox.SetTransformedAABB(m_worldMatrix, m_localBoundingBox);
    }

    void CloudVolumeSprite::Refresh(CloudParticleData& cloudData, const Matrix34& worldMatrix)
    {
        Matrix33 rotation;
        worldMatrix.GetRotation33(rotation);

        m_worldMatrix = worldMatrix;
        m_localBoundingBox = cloudData.GetBounds();
        m_worldBoundingBox.SetTransformedAABB(worldMatrix, m_localBoundingBox);
        const SMinMaxBox box{ rotation * m_localBoundingBox.min, rotation * m_localBoundingBox.max};
        m_renderElement->SetParticles(cloudData.GetParticles(), box);
    }

    void CloudVolumeSprite::Render(const SRendParams& params, const SRenderingPassInfo& passInfo, float alpha, int isAfterWater)
    {
        IRenderer* pRenderer = gEnv->pRenderer;
        CRenderObject* renderObject = pRenderer->EF_GetObject_Temp(passInfo.ThreadID());
        if (renderObject)
        {
            SRenderObjData* renderObjectData = pRenderer->EF_GetObjData(renderObject, true, passInfo.ThreadID());
            renderObjectData->SetRenderElement(m_pREImposter->GetRE());
            renderObjectData->m_fTempVars[0] = m_worldMatrix.GetColumn(0).GetLength();
            renderObject->m_II.m_Matrix = m_worldMatrix;
            renderObject->m_fSort = 0;
            renderObject->m_fDistance = params.fDistance;
            renderObject->m_II.m_AmbColor = params.AmbientColor;
            renderObject->m_fAlpha = params.fAlpha * alpha;

            SShaderItem& shaderItem = params.pMaterial ? params.pMaterial->GetShaderItem(0) : m_material->GetShaderItem(0);
            pRenderer->EF_AddEf(m_renderElement->GetRE(), shaderItem, renderObject, passInfo, EFSLIST_TRANSP, isAfterWater, SRendItemSorter(params.rendItemSorter));
        }
    }
}