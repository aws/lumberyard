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

// This plugin has been deprecated.  It will not have
// the plugin entry point and can safely be removed.
// Instead of removing the dll entirely, we leave
// this empty dll here just to make sure that no old,
// stale versions of this plugin exists on disk.

#include <AzCore/PlatformDef.h>

extern "C" AZ_DLL_EXPORT void DummyExportFunction()
{
    // This function exists only to generate and overwrite any stale binaries present.
    // This library is deprecated and will be removed.
}
