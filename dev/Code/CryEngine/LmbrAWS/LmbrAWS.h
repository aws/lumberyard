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

#ifdef LMBRAWS_EXPORTS
#define LMBRAWS_API DLL_EXPORT
#else
#define LMBRAWS_API DLL_IMPORT
#endif

class ILmbrAWS;

namespace LmbrAWS
{
    class IClientManager;
    class MaglevConfig;
} // namespace LmbrAWS

// Returns gEnv->pLmbrAWS
LMBRAWS_API ILmbrAWS* GetLmbrAWS();

namespace LmbrAWS
{
    // This method is a shortcut for GetLmbrAWS()->GetClientManager().
    LMBRAWS_API IClientManager* GetClientManager();
    LMBRAWS_API MaglevConfig* GetMaglevConfig();
}

