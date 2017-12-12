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

#ifndef CRYINCLUDE_CRYCOMMON_CRYCOMMONREFLECTION_H
#define CRYINCLUDE_CRYCOMMON_CRYCOMMONREFLECTION_H
#pragma once

#include <AzCore/Serialization/SerializeContext.h>

namespace spline
{
    template<>
    inline void SplineKey<Vec2>::Reflect(AZ::SerializeContext* serializeContext)
    {
        serializeContext->Class<SplineKey<Vec2> >()
            ->Version(1)
            ->Field("time", &SplineKey<Vec2>::time)
            ->Field("flags", &SplineKey<Vec2>::flags)
            ->Field("value", &SplineKey<Vec2>::value)
            ->Field("ds", &SplineKey<Vec2>::ds)
            ->Field("dd", &SplineKey<Vec2>::dd);
    }
} // namespace spline

#endif // CRYINCLUDE_CRYCOMMON_CRYCOMMONREFLECTION_H
