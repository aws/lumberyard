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
#ifndef CRYINCLUDE_CRYINPUT_SIMPLEHIGHPASSFILTER_H
#define CRYINCLUDE_CRYINPUT_SIMPLEHIGHPASSFILTER_H
#pragma once

#include "SingleOrderFilter.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
struct CSimpleHighPassFilter
    : public CSingleOrderFilter
{
public:
    CSimpleHighPassFilter(float filterConstant, EMotionSensorFlags filterFlags);
    virtual ~CSimpleHighPassFilter();

protected:
    virtual void FilterImpl(float& io_newSample, float& io_rollingSample) const;
    virtual void FilterImpl(Vec3& io_newSample, Vec3& io_rollingSample) const;
    virtual void FilterImpl(Quat& io_newSample, Quat& io_rollingSample) const;

private:
    const float m_filterConstant;
};

#endif // CRYINCLUDE_CRYINPUT_SIMPLEHIGHPASSFILTER_H
