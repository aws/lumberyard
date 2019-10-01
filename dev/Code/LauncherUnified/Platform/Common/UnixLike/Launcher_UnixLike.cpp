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
#include <Launcher_precompiled.h>

#include "Launcher_UnixLike.h"

#include <AzCore/Debug/Trace.h>

#include <sys/resource.h>
#include <sys/types.h>


namespace
{
    // return true if the limit was updated and setlimit needs to be called, false otherwise
    typedef bool(*ResourceLimitUpdater)(rlimit&);

    bool IncreaseMaxToInfinity(rlimit& limit)
    {
        if (limit.rlim_max != RLIM_INFINITY)
        {
            limit.rlim_max = RLIM_INFINITY;
            return true;
        }
        return false;
    }

    bool IncreaseCurrentToMax(rlimit& limit)
    {
        if (limit.rlim_cur < limit.rlim_max)
        {
            limit.rlim_cur = limit.rlim_max;
            return true;
        }
        return false;
    }

    bool IncreaseResourceLimit(int resource, ResourceLimitUpdater updateLimit)
    {
        rlimit limit;
        if (getrlimit(resource, &limit) != 0)
        {
            AZ_Error("Launcher", false, "[ERROR] Failed to get limit for resource %d.  Error: %s", resource, strerror(errno));
            return false;
        }

        if (updateLimit(limit))
        {
            if (setrlimit(resource, &limit) != 0)
            {
                AZ_Error("Launcher", false, "[ERROR] Failed to update resource limit for resource %d.  Error: %s", resource, strerror(errno));
                return false;
            }
        }
        return true;
    }
}


namespace LumberyardLauncher
{
    bool IncreaseResourceLimits()
    {
        return (IncreaseResourceLimit(RLIMIT_CORE, IncreaseMaxToInfinity)
            && IncreaseResourceLimit(RLIMIT_STACK, IncreaseCurrentToMax));
    }
}
