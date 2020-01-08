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

#include <AzCore/Component/ComponentApplication.h>

namespace AZ
{
    /// Calculates the Bin folder name where the application is running from (off of the engine root folder)
    void ComponentApplication::PlatformCalculateBinFolder()
    {
        azstrcpy(m_binFolder, AZ_ARRAY_SIZE(m_binFolder), "");
    }
}
