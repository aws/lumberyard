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
#ifndef CRYINCLUDE_CRYINPUT_MULTIORDERFILTER_H
#define CRYINCLUDE_CRYINPUT_MULTIORDERFILTER_H
#pragma once

#include "IInput.h"

#include <deque>
#include <vector>

////////////////////////////////////////////////////////////////////////////////////////////////////
// A filter where all sample data components depend on their n=order previous samples to be filtered.
////////////////////////////////////////////////////////////////////////////////////////////////////
struct CMultiOrderFilter
    : public IMotionSensorFilter
{
public:
    CMultiOrderFilter(EMotionSensorFlags filterFlags, int order);
    virtual ~CMultiOrderFilter();

    virtual void Filter(SMotionSensorData& io_newSample, EMotionSensorFlags newSampleFlags);

protected:
    virtual void FilterImpl(float& io_newSample,
        float& o_rollingSample,
        const std::vector<float> previousSamples) const = 0;
    virtual void FilterImpl(Vec3& io_newSample,
        Vec3& o_rollingSample,
        const std::vector<const Vec3*> previousSamples) const = 0;
    virtual void FilterImpl(Quat& io_newSample,
        Quat& o_rollingSample,
        const std::vector<const Quat*> previousSamples) const = 0;

private:
    std::deque<SMotionSensorData*> m_previousSamples;
    SMotionSensorData* m_rollingSample;
    const EMotionSensorFlags m_filterFlags;
};

#endif // CRYINCLUDE_CRYINPUT_MULTIORDERFILTER_H
