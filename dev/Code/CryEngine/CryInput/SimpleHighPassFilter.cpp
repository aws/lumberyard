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

#include "SimpleHighPassFilter.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// CSimpleHighPassFilter Functions
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
CSimpleHighPassFilter::CSimpleHighPassFilter(float filterConstant, EMotionSensorFlags filterFlags)
    : CSingleOrderFilter(filterFlags)
    , m_filterConstant(filterConstant)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CSimpleHighPassFilter::~CSimpleHighPassFilter()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CSimpleHighPassFilter::FilterImpl(float& io_newSample, float& io_rollingSample) const
{
    io_rollingSample = (io_newSample * m_filterConstant) + (io_rollingSample * (1.0f - m_filterConstant));
    io_newSample -= io_rollingSample;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CSimpleHighPassFilter::FilterImpl(Vec3& io_newSample, Vec3& io_rollingSample) const
{
    io_rollingSample = (io_newSample * m_filterConstant) + (io_rollingSample * (1.0f - m_filterConstant));
    io_newSample -= io_rollingSample;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CSimpleHighPassFilter::FilterImpl(Quat& io_newSample, Quat& io_rollingSample) const
{
    io_rollingSample.SetSlerp(io_rollingSample, io_newSample, m_filterConstant);
    io_newSample -= io_rollingSample;
    io_newSample.Normalize();
}
