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

#pragma once

#include <AzCore/base.h>

#if !defined(AZ_STREAMER_ADD_EXTRA_PROFILING_INFO)
#   if defined(_RELEASE)
#       define AZ_STREAMER_ADD_EXTRA_PROFILING_INFO 0
#   else
#       define AZ_STREAMER_ADD_EXTRA_PROFILING_INFO 1
#   endif
#endif

namespace AZ
{
    namespace IO
    {
        //! The number of entries in the statistics window. Larger number will measure over a longer time and will be more
        //! accurate, but also less responsive to sudden changes.
        static const size_t s_statisticsWindowSize = 128;
    }
}