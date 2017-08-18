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

#include "stdafx.h"

#if defined(OFFLINE_COMPUTATION)

#include <PRT/ITransferConfigurator.h>
#include <PRT/ISHMaterial.h>
#include <PRT/SimpleIndexedMesh.h>

void NSH::NTransfer::ITransferConfigurator::AddDirectScalarCoefficientValue
(
    NSH::SCoeffList_tpl<NSH::TScalarCoeff>& rCoeffListDirect,
    const NSH::TScalarCoeff::TComponentType& crValue,
    const NSH::CSample_tpl<NSH::SCoeffList_tpl<NSH::TScalarCoeff> >& crSample
) const
{
    for (size_t j = 0; j < rCoeffListDirect.size(); ++j)
    {
        rCoeffListDirect[j] += (TScalarCoeff::TComponentType)(crValue * crSample.Coeffs()[j]);
    }
}

void NSH::NTransfer::ITransferConfigurator::ProcessRayCastingResult(NSH::NTransfer::SRayResultData& rRayResultData) const
{
    //iterate all hits sorted in ascending order
    const TRayResultVec::const_iterator cEnd = rRayResultData.crRayResultVec.end();
    bool firstElement = true;
    bool recordForInterreflection;
    rRayResultData.rApplyTransparency = false;
    for (TRayResultVec::const_iterator iter = rRayResultData.crRayResultVec.begin(); iter != cEnd; ++iter)
    {
        const SRayResult& crRayRes = *iter;
        recordForInterreflection = true;
        TRGBCoeffType& rIncidentIntensity = rRayResultData.rIncidentIntensity;
        const NSH::NMaterial::ISHMaterial& rHitMat = crRayRes.pMesh->GetMaterialByFaceID(crRayRes.faceID);  //cache material reference
        //if it is a transparency supporting material, use transparent ray colour if it is transparent, otherwise it just continues the loop and records the hit
        if (rHitMat.HasTransparencyTransfer())
        {
            rRayResultData.rContinueLoop = false;
            //hit material supports transparency
            //get diffuse value multiplied by transparency at hit point, used as incident intensity
            //use updated intensity for next ray
            rIncidentIntensity =
                rHitMat.DiffuseIntensity
                (
                    rIncidentIntensity,
                    crRayRes.faceID,
                    crRayRes.baryCoord,
                    rRayResultData.crRayDir,
                    false,  //no cos
                    false,  //no material application, will be done in shader or at interreflection retrieval time
                    true,       //abs cos since we don't need any backface rejection here
                    true        //apply transparency
                );
            const float scIntensityThreshold = 0.1f;//threshold for not considering intensity
            const float scInverseIntensityThreshold = 1.f - scIntensityThreshold;//threshold for not considering hit
            const float incidentIntensitySum = (float)(rIncidentIntensity.x + rIncidentIntensity.y + rIncidentIntensity.z);
            if (incidentIntensitySum < scIntensityThreshold)//to low to consider
            {
                rRayResultData.rContinueLoop = true;//ray hits something, vertex not visible from sample
            }
            else
            if (incidentIntensitySum > scInverseIntensityThreshold)
            {
                recordForInterreflection = false;    //almost completely transparent, don't treat as a hit
            }
            rRayResultData.rApplyTransparency = true;
        }
        else
        {
            //one non transparency hit is enough to block the entire ray
            rIncidentIntensity = TRGBCoeffType(0., 0., 0.);//non transparent material, nothing comes through
            rRayResultData.rContinueLoop = true;
        }
        //record hit, used for subsequent interreflection passes
        if (rRayResultData.cIsOnUpperHemisphere && firstElement && recordForInterreflection)
        {
            firstElement = false;
            rRayResultData.pRayRes = (SRayResult*)&crRayRes;    //result to be stored for interreflection
        }
        if (rRayResultData.rContinueLoop)
        {
            break;//ray almost blocked entirely, break here
        }
    }
}

void NSH::NTransfer::ITransferConfigurator::TransformRayCastingResult(NSH::NTransfer::SRayProcessedData& rRayProcessedData) const
{
    const CSample_tpl<SCoeffList_tpl<TScalarCoeff> >& crSample = rRayProcessedData.crSample;
    const TCartesianCoord& crSampleCartCoord = crSample.GetCartesianPos();
    TRGBCoeffType& rIncidentIntensity = rRayProcessedData.rIncidentIntensity;
    const STransferParameters& crParameters = rRayProcessedData.crParameters;
    SCoeffList_tpl<TScalarCoeff>& rCoeffListDirect      = rRayProcessedData.rCoeffListDirect;
    const bool cAddGroundPlaneBehaviour = (crParameters.groundPlaneBlockValue != 1.0f);

    const TRGBCoeffType cNeutralIntensity(1., 1., 1.);

    if (cAddGroundPlaneBehaviour && crSampleCartCoord.z < 0)
    {
        //ray fired toward ground, treat partly as blocked
        rIncidentIntensity *= crParameters.groundPlaneBlockValue;
    }
    if (crParameters.bumpGranularity)
    {
        //treat differently if there was a transparency modification
        if (rRayProcessedData.cApplyTransparency)
        {
            //formular is as follows:
            //    rRayProcessedData.rVis = component_sum(rIncidentIntensity*vertex_mat) / component_sum(vertex_mat)
            //fetch material property at vertex pos
            const TRGBCoeffType rgbCoeffVertex =
                rRayProcessedData.crMat.DiffuseIntensity(rRayProcessedData.crPos, crSampleCartCoord, rRayProcessedData.crTexCoord, cNeutralIntensity, crSampleCartCoord, false, true, false);
            const double cVertexMatComponentSum             = rgbCoeffVertex.x + rgbCoeffVertex.y + rgbCoeffVertex.z;
            const double cIncidentVertexComponentSum    = rgbCoeffVertex.x * rIncidentIntensity.x + rgbCoeffVertex.y * rIncidentIntensity.y + rgbCoeffVertex.z * rIncidentIntensity.z;
            const double cVisFactor = cIncidentVertexComponentSum / cVertexMatComponentSum;
            assert(cVisFactor <= 1.0);

            rRayProcessedData.rVis += (float)cVisFactor;
            const float cAdjustedVis = std::max(crParameters.minDirectBumpCoeffVisibility, (float)cVisFactor);//make sure minimum visibility is set
            AddDirectScalarCoefficientValue(rCoeffListDirect, cAdjustedVis, crSample);
        }
        else
        {
            if (cAddGroundPlaneBehaviour && crSampleCartCoord.z < 0)
            {
                rRayProcessedData.rVis += crParameters.groundPlaneBlockValue;
                const float cAdjustedVis = std::max(crParameters.minDirectBumpCoeffVisibility, crParameters.groundPlaneBlockValue);//make sure minimum visibility is set
                AddDirectScalarCoefficientValue(rCoeffListDirect, cAdjustedVis, crSample);
            }
            else
            {
                rRayProcessedData.rVis += cNeutralIntensity.x;
                AddDirectScalarCoefficientValue(rCoeffListDirect, 1.0, crSample);
            }
        }
    }
    else
    {
        rRayProcessedData.rVis += cNeutralIntensity.x;
        //work at vertex level, no material application, will be done in shader or at interreflection retrieval time
        TRGBCoeffType rgbCoeffDirect = rRayProcessedData.crMat.DiffuseIntensity(rRayProcessedData.crPos, rRayProcessedData.crNormal, rRayProcessedData.crTexCoord, rIncidentIntensity, crSampleCartCoord, true, false, false);
        AddDirectScalarCoefficientValue(rCoeffListDirect, rgbCoeffDirect.x, crSample);
    }
}

#endif