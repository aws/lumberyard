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

#include "System.h"

#include <AzCore/std/string/regex.h>

namespace CryPakInternal
{
    // Report missing files in the pak when they are loaded
    void ReportFileMissingFromPak(const char *szPath, SSystemCVars cvars);

    // Whether we should ignore this missing file
    bool IsIgnored(const char *szPath);

    // Do not report missing LOD files if no CGF files depend on them
    // Do not report missing .cgfm files since they're not actually created and used in Lumberyard
    // This checking prevents our missing dependency scanner from having a lot of false positives on these files
    bool IgnoreCGFDependencies(const char *szPath);
}