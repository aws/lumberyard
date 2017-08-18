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
#ifndef CRYINCLUDE_CRYINPUT_SINGLEORDERFILTER_H
#define CRYINCLUDE_CRYINPUT_SINGLEORDERFILTER_H
#pragma once

#include "IInput.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// A filter where all sample data components depend only on their previous sample to be filtered.
////////////////////////////////////////////////////////////////////////////////////////////////////
struct CSingleOrderFilter
    : public IMotionSensorFilter
{
public:
    CSingleOrderFilter(EMotionSensorFlags filterFlags);
    virtual ~CSingleOrderFilter();

    virtual void Filter(SMotionSensorData& io_newSample, EMotionSensorFlags newSampleFlags);

protected:
    virtual void FilterImpl(float& io_newSample, float& io_rollingSample) const = 0;
    virtual void FilterImpl(Vec3& io_newSample, Vec3& io_rollingSample) const = 0;
    virtual void FilterImpl(Quat& io_newSample, Quat& io_rollingSample) const = 0;

private:
    SMotionSensorData* m_rollingSample;
    const EMotionSensorFlags m_filterFlags;
};

#endif // CRYINCLUDE_CRYINPUT_SINGLEORDERFILTER_H
