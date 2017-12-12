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
#ifndef AZ_UNITY_BUILD

#include <AzCore/Math/VectorFloat.h>

using namespace AZ;

const VectorFloat AZ::g_simdTolerance(0.01f);
const VectorFloat AZ::g_fltMax(AZ_FLT_MAX);
const VectorFloat AZ::g_fltEps(AZ_FLT_EPSILON);
const VectorFloat AZ::g_fltEpsSq(AZ_FLT_EPSILON* AZ_FLT_EPSILON);

#endif // #ifndef AZ_UNITY_BUILD