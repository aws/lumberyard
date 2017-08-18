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
#include "StdAfx.h"

#include "MultiOrderFilter.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// CMultiOrderFilter Functions
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
CMultiOrderFilter::CMultiOrderFilter(EMotionSensorFlags filterFlags, int order)
    : m_previousSamples(order, nullptr)
    , m_rollingSample(nullptr)
    , m_filterFlags(filterFlags)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CMultiOrderFilter::~CMultiOrderFilter()
{
    for (int i = 0; i < m_previousSamples.size(); ++i)
    {
        delete m_previousSamples[i];
    }
    delete m_rollingSample;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CMultiOrderFilter::Filter(SMotionSensorData& io_newSample, EMotionSensorFlags newSampleFlags)
{
    if (m_rollingSample == nullptr)
    {
        m_rollingSample = new SMotionSensorData(io_newSample);
        for (int i = 0; i < m_previousSamples.size(); ++i)
        {
            m_previousSamples[i] = new SMotionSensorData(io_newSample);
        }
        return;
    }

    const int filterFlags = m_filterFlags & newSampleFlags;
    if (filterFlags & eMSF_AccelerationRaw)
    {
        std::vector<const Vec3*> previousSamples(m_previousSamples.size(), nullptr);
        for (int i = 0; i < m_previousSamples.size(); ++i)
        {
            previousSamples[i] = &(m_previousSamples[i]->accelerationRaw);
        }
        FilterImpl(io_newSample.accelerationRaw, m_rollingSample->accelerationRaw, previousSamples);
    }
    if (filterFlags & eMSF_AccelerationUser)
    {
        std::vector<const Vec3*> previousSamples(m_previousSamples.size(), nullptr);
        for (int i = 0; i < m_previousSamples.size(); ++i)
        {
            previousSamples[i] = &(m_previousSamples[i]->accelerationUser);
        }
        FilterImpl(io_newSample.accelerationUser, m_rollingSample->accelerationUser, previousSamples);
    }
    if (filterFlags & eMSF_AccelerationGravity)
    {
        std::vector<const Vec3*> previousSamples(m_previousSamples.size(), nullptr);
        for (int i = 0; i < m_previousSamples.size(); ++i)
        {
            previousSamples[i] = &(m_previousSamples[i]->accelerationGravity);
        }
        FilterImpl(io_newSample.accelerationGravity, m_rollingSample->accelerationGravity, previousSamples);
    }
    if (filterFlags & eMSF_RotationRateRaw)
    {
        std::vector<const Vec3*> previousSamples(m_previousSamples.size(), nullptr);
        for (int i = 0; i < m_previousSamples.size(); ++i)
        {
            previousSamples[i] = &(m_previousSamples[i]->rotationRateRaw);
        }
        FilterImpl(io_newSample.rotationRateRaw, m_rollingSample->rotationRateRaw, previousSamples);
    }
    if (filterFlags & eMSF_RotationRateUnbiased)
    {
        std::vector<const Vec3*> previousSamples(m_previousSamples.size(), nullptr);
        for (int i = 0; i < m_previousSamples.size(); ++i)
        {
            previousSamples[i] = &(m_previousSamples[i]->rotationRateUnbiased);
        }
        FilterImpl(io_newSample.rotationRateUnbiased, m_rollingSample->rotationRateUnbiased, previousSamples);
    }
    if (filterFlags & eMSF_MagneticFieldRaw)
    {
        std::vector<const Vec3*> previousSamples(m_previousSamples.size(), nullptr);
        for (int i = 0; i < m_previousSamples.size(); ++i)
        {
            previousSamples[i] = &(m_previousSamples[i]->magneticFieldRaw);
        }
        FilterImpl(io_newSample.magneticFieldRaw, m_rollingSample->magneticFieldRaw, previousSamples);
    }
    if (filterFlags & eMSF_MagneticFieldUnbiased)
    {
        std::vector<const Vec3*> previousSamples(m_previousSamples.size(), nullptr);
        for (int i = 0; i < m_previousSamples.size(); ++i)
        {
            previousSamples[i] = &(m_previousSamples[i]->magneticFieldUnbiased);
        }
        FilterImpl(io_newSample.magneticFieldUnbiased, m_rollingSample->magneticFieldUnbiased, previousSamples);
    }
    if (filterFlags & eMSF_MagneticNorth)
    {
        std::vector<const Vec3*> previousSamples(m_previousSamples.size(), nullptr);
        for (int i = 0; i < m_previousSamples.size(); ++i)
        {
            previousSamples[i] = &(m_previousSamples[i]->magneticNorth);
        }
        FilterImpl(io_newSample.magneticNorth, m_rollingSample->magneticNorth, previousSamples);
    }
    if (filterFlags & eMSF_Orientation)
    {
        std::vector<const Quat*> previousSamples(m_previousSamples.size(), nullptr);
        for (int i = 0; i < m_previousSamples.size(); ++i)
        {
            previousSamples[i] = &(m_previousSamples[i]->orientation);
        }
        FilterImpl(io_newSample.orientation, m_rollingSample->orientation, previousSamples);
    }
    if (filterFlags & eMSF_OrientationDelta)
    {
        std::vector<const Quat*> previousSamples(m_previousSamples.size(), nullptr);
        for (int i = 0; i < m_previousSamples.size(); ++i)
        {
            previousSamples[i] = &(m_previousSamples[i]->orientationDelta);
        }
        FilterImpl(io_newSample.orientationDelta, m_rollingSample->orientationDelta, previousSamples);
    }

    SMotionSensorData* oldestSample = m_previousSamples.front();
    m_previousSamples.pop_front();
    m_previousSamples.push_back(m_rollingSample);
    m_rollingSample = oldestSample;
}
