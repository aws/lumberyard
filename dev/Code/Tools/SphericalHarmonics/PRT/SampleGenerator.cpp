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
#include "SampleGenerator.h"
#include <Cry_Geo.h>

using NSH::NIcosahedron::g_cIsocahedronFaceCount;

const uint32 NSH::NSampleGen::CLinearSampleOrganizer::ConvertIntoSHAndReOrganize
(
    const TVec2D* cpRandomSamples,
    const uint32 cRandomSampleCount,
    const NSH::SDescriptor& rSHDescriptor
)
{
    m_RandomSamples.resize(cRandomSampleCount); //size according to generated sample count
    //now map the 2D samples into spherical co-ordinates, also generate their position on the sphere in 3D
    uint32 i = 0;
    for (; i < cRandomSampleCount; i++)
    {
        m_RandomSamples[i] = CSample_tpl<SCoeffList_tpl<TScalarCoeff> >
            (
                rSHDescriptor,
                TPolarCoord(2.0 * ACos(Sqrt(1.0 - cpRandomSamples[i].x)), 2.0 * g_cPi * cpRandomSamples[i].y),
                (TSampleHandle)i
            );
        const TPolarCoord& coord = m_RandomSamples[i].GetPolarPos();
        // pre-calculate the SH basis function values at this sample
        for (int l = 0, c = 0; l < rSHDescriptor.Bands; l++)
        {
            for (int m = -l; m <= l; m++, c++)
            {
                m_RandomSamples[i].Coeffs()[c] = (TScalarCoeff::TComponentType)NLegendre::Y(l, m, coord.theta, coord.phi, NLegendre::SLEGENDRE_CALC_OPTIMIZED());
            }
        }
    }
    return i;
}

/************************************************************************************************************************************************/

NSH::NSampleGen::CIsocahedronSampleOrganizer::CIsocahedronSampleOrganizer(const uint32 cSamplesToHold, const int32 cMinSampleCountToRetrieve)
{
    //compute levels to hold so that at least a certain sample count is hold by triangle leaf nodes
    const uint8 s_cSamplesPerLeaf = 10;
    //find subdivision levels
    uint8 level = 1;
    int count = cSamplesToHold / g_cIsocahedronFaceCount / s_cSamplesPerLeaf;
    while ((count >>= 2) > 1)//4 leafs per node
    {
        level++;
    }
    //now make sure the number of bins are below the min number of samples to retrieve
    if (cMinSampleCountToRetrieve > 0)
    {
        while (20 * pow(4.0, level) > cMinSampleCountToRetrieve)
        {
            level--;
        }
    }
    m_SubdivisionLevels = std::max(level, (uint8)1); //make sure at least one subdivision level
    //construct the isocahedron hierarchy
    for (uint8 i = 0; i < g_cIsocahedronFaceCount; ++i)
    {
        const Triangle_tpl<double>& tri = m_IsoManager.GetFace(i);
        m_SubTriManagers[i].Construct(m_SubdivisionLevels, tri.v0, tri.v1, tri.v2);
        m_SubTriManagers[i].SetHandleEncodeFunction(&HandleEncodeFunction);
    }
}

const uint32 NSH::NSampleGen::CIsocahedronSampleOrganizer::ConvertIntoSHAndReOrganize
(
    const TVec2D* cpRandomSamples,
    const uint32 cRandomSampleCount,
    const NSH::SDescriptor& rSHDescriptor
)
{
    TSampleVec samples;
    samples.reserve(cRandomSampleCount);    //size according to generated sample count

    // now map the 2D samples into spherical co-ordinates, also generate their position on the sphere in 3D
    for (size_t i = 0; i < cRandomSampleCount; i++)
    {
        samples.push_back(TSampleType
            (
                rSHDescriptor,
                TPolarCoord(2.0 * ACos(Sqrt(1.0 - cpRandomSamples[i].x)), 2.0 * g_cPi * cpRandomSamples[i].y))
            );
    }
    //now add into isocahedron and triangle hierarchy
    uint32 samplesInserted = 0;
    for (size_t i = 0; i < cRandomSampleCount; i++)
    {
        TSampleType& sample = samples[i];
        const TCartesianCoord& crPos = sample.GetCartesianPos();
        const int8 faceIndex = m_IsoManager.GetFaceIndex(crPos);
        if (faceIndex == -1)
        {
            GetSHLog().LogWarning("Couldn't find proper isocahedron face for sample index %d\n", (int)i);
            continue;
        }
        samplesInserted++;
        assert(faceIndex < g_cIsocahedronFaceCount);
        if (!m_SubTriManagers[faceIndex].AddSample(sample, faceIndex, m_SubdivisionLevels))
        {
            samplesInserted--;
        }
    }
    const uint32 failedSamples = (uint32)(cRandomSampleCount - samplesInserted);
    if (failedSamples != 0)
    {
        GetSHLog().LogWarning("Sample Generator: %d samples could not been inserted, %d samples successfully inserted\n", failedSamples, samplesInserted);
    }
    return samplesInserted;
}

void NSH::NSampleGen::CIsocahedronSampleOrganizer::GetSamples(TScalarVecVec& rSamples) const
{
    //gather all samples across isocahedron faces
    rSamples.clear();
    for (int i = 0; i < g_cIsocahedronFaceCount; ++i)
    {
        TScalarVecVec v;
        m_SubTriManagers[i].GetSphereSamples(v);
        TScalarVecVec::TDataVec& rVectorData = v.GetData();
        const TScalarVecVec::TDataVec::iterator cEnd = rVectorData.end();
        for (TScalarVecVec::TDataVec::iterator iter = rVectorData.begin(); iter != cEnd; ++iter)
        {
            const TScalarVecVec::TElemVec* pVec = *iter;
            if (!pVec->empty())
            {
                rSamples.push_back(*iter);//gather all pointers to vector elements
            }
        }
    }
}

/************************************************************************************************************************************************/