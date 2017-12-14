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

#include <AzCore/std/string/string_view.h>

namespace AZ
{
    class SerializeContext;
    class BehaviorContext;
}

namespace GraphicsReflectContext
{
    // Class for exposing Utilitiy nodes to ScriptCanvas/Lua
    class Utilities
    {
    public:
        AZ_TYPE_INFO(Utilities, "{CE7A4621-7E17-4DA3-801B-0CFC7B4E0563}")

        static bool ClipCapture(float durationBefore, float durationAfter, AZStd::string_view clipName, const AZStd::string_view localizedClipName, AZStd::string_view metadata);

        static void ReflectBehaviorContext(AZ::BehaviorContext *behaviorContext);
    };
}
