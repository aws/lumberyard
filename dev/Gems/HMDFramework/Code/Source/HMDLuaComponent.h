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

#include <AzCore/Component/Component.h>

/*
    This class exposes a bunch of functionality from the HMD related buses to the Lua scripting system.
    Not everything is exposed. We don't want Lua to have access to anything related to sending frame updates or 
    accessing GPU textures. This mostly exposes things like controller and HMD tracking info, debug info, etc. 
*/

namespace AZ
{
    namespace VR
    {
        class HMDLuaComponent :
            public Component
        {
        public:
            AZ_COMPONENT(HMDLuaComponent, "{A09F1CFD-8360-46E1-ADED-CB1E804A2E18}");

            HMDLuaComponent() {};
            ~HMDLuaComponent() override {};

            void Activate() override {};
            void Deactivate() override {};

            static void Reflect(ReflectContext* context);
        };
    }
}
