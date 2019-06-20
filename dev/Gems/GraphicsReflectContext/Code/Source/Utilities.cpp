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

#include "Precompiled.h"
#include "Utilities.h"
#include <AzCore/RTTI/BehaviorContext.h>
#include <IPlatformOS.h>

// You can disable video clip recording without removing the nodes from your scripts by 
// removing the following "#define ENABLE_GAMEDVR_SCRIPTING" line
#define ENABLE_GAMEDVR_NODE

namespace GraphicsReflectContext
{

    bool Utilities::ClipCapture(float durationBefore, float durationAfter, AZStd::string_view clipName, AZStd::string_view localizedClipName, AZStd::string_view metadata)
    {
        bool success = false;

#ifdef ENABLE_GAMEDVR_NODE
        if (IPlatformOS::IClipCaptureOS* pClipCapture = gEnv->pSystem->GetPlatformOS()->GetClipCapture())
        {
            const IPlatformOS::IClipCaptureOS::SSpan span(durationBefore, durationAfter);

            string localizedToast;
            wstring wideToast;
            gEnv->pSystem->GetLocalizationManager()->LocalizeString(localizedClipName.data(), localizedToast);
            Unicode::Convert(wideToast, localizedToast);

            IPlatformOS::IClipCaptureOS::SClipTextInfo clipTextInfo(clipName.data(), wideToast.c_str(), metadata.data());

            success = pClipCapture->RecordClip(clipTextInfo, span);
        }
#endif //ENABLE_GAMEDVR_NODE

        return success;
    }


    void Utilities::ReflectBehaviorContext(AZ::BehaviorContext *behaviorContext)
    {

        behaviorContext->Class<Utilities>("Utilities")
            ->Attribute(AZ::Script::Attributes::Category, "Rendering")
            ->Method("ClipCapture",
                &Utilities::ClipCapture,
                { {
                    { "DurationBefore",    "Record this many seconds from before the capture was triggered",   behaviorContext->MakeDefaultValue(20.0f) },
                    { "DurationAfter",     "Record this many seconds after the capture is triggered",          behaviorContext->MakeDefaultValue(10.0f) },
                    { "ClipName",          "Usage details are platform-specific",                              behaviorContext->MakeDefaultValue(AZStd::string_view()) },
                    { "LocalizedClipName", "Usage details are platform-specific",                              behaviorContext->MakeDefaultValue(AZStd::string_view()) },
                    { "Metadata",          "Optional. Use it for instance to tag clips",                       behaviorContext->MakeDefaultValue(AZStd::string_view()) },
                } } )
            ->Attribute(AZ::Script::Attributes::ToolTip, "Allows capturing video clips on consoles while the game is running and save them locally or in the cloud")
        ;

    }
}

#undef ENABLE_GAMEDVR_NODE