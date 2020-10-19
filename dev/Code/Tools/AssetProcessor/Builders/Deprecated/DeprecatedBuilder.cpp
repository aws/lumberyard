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

// This is a deprecated Builder.  It no longer exports Builder functions that
// Asset Processor uses to register and call into.
// Instead of removing the old module from the build, we build this stub dll
// to overwrite any stale versions of the Builder module that may exist on disk.

#include <AzCore/PlatformDef.h>

extern "C" AZ_DLL_EXPORT void DummyExportFunction()
{
    // This function exists only to generate and overwrite any stale binaries present.
    // This library is deprecated and will be removed.
}
