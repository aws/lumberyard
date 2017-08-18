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

#include "SingleOrderFilter.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// CSingleOrderFilter Functions
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
CSingleOrderFilter::CSingleOrderFilter(EMotionSensorFlags filterFlags)
    : m_rollingSample(nullptr)
    , m_filterFlags(filterFlags)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CSingleOrderFilter::~CSingleOrderFilter()
{
    delete m_rollingSample;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CSingleOrderFilter::Filter(SMotionSensorData& io_newSample, EMotionSensorFlags newSampleFlags)
{
    if (m_rollingSample == nullptr)
    {
        m_rollingSample = new SMotionSensorData(io_newSample);
        return;
    }

    const int filterFlags = m_filterFlags & newSampleFlags;
    if (filterFlags & eMSF_AccelerationRaw)
    {
        FilterImpl(io_newSample.accelerationRaw, m_rollingSample->accelerationRaw);
    }
    if (filterFlags & eMSF_AccelerationUser)
    {
        FilterImpl(io_newSample.accelerationUser, m_rollingSample->accelerationUser);
    }
    if (filterFlags & eMSF_AccelerationGravity)
    {
        FilterImpl(io_newSample.accelerationGravity, m_rollingSample->accelerationGravity);
    }
    if (filterFlags & eMSF_RotationRateRaw)
    {
        FilterImpl(io_newSample.rotationRateRaw, m_rollingSample->rotationRateRaw);
    }
    if (filterFlags & eMSF_RotationRateUnbiased)
    {
        FilterImpl(io_newSample.rotationRateUnbiased, m_rollingSample->rotationRateUnbiased);
    }
    if (filterFlags & eMSF_MagneticFieldRaw)
    {
        FilterImpl(io_newSample.magneticFieldRaw, m_rollingSample->magneticFieldRaw);
    }
    if (filterFlags & eMSF_MagneticFieldUnbiased)
    {
        FilterImpl(io_newSample.magneticFieldUnbiased, m_rollingSample->magneticFieldUnbiased);
    }
    if (filterFlags & eMSF_MagneticNorth)
    {
        FilterImpl(io_newSample.magneticNorth, m_rollingSample->magneticNorth);
    }
    if (filterFlags & eMSF_Orientation)
    {
        FilterImpl(io_newSample.orientation, m_rollingSample->orientation);
    }
    if (filterFlags & eMSF_OrientationDelta)
    {
        FilterImpl(io_newSample.orientationDelta, m_rollingSample->orientationDelta);
    }
}
