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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Node to record in-game video clips.


#include "CryLegacy_precompiled.h"

#include <IPlatformOS.h>
#include "UnicodeFunctions.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>

#if defined(AZ_RESTRICTED_PLATFORM) || defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#undef AZ_RESTRICTED_SECTION
#define FLOWCLIPCAPTURENODES_CPP_SECTION_1 1
#define FLOWCLIPCAPTURENODES_CPP_SECTION_2 2
#define FLOWCLIPCAPTURENODES_CPP_SECTION_3 3
#endif

// ------------------------------------------------------------------------
class CClipCaptureManagement
    : public CFlowBaseNode<eNCT_Singleton>
{
public:
    CClipCaptureManagement(SActivationInfo* pActInfo)
    {
    }

    ~CClipCaptureManagement()
    {
    }

    void Serialize(SActivationInfo*, TSerialize ser)
    {
    }

    void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    enum EInputs
    {
        eI_Capture = 0,
        eI_DurationBefore,
        eI_DurationAfter,
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION FLOWCLIPCAPTURENODES_CPP_SECTION_1
#include AZ_RESTRICTED_FILE(FlowClipCaptureNodes_cpp, AZ_RESTRICTED_PLATFORM)
#endif
        eI_ClipId,
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION FLOWCLIPCAPTURENODES_CPP_SECTION_2
#include AZ_RESTRICTED_FILE(FlowClipCaptureNodes_cpp, AZ_RESTRICTED_PLATFORM)
#endif
        eI_LocalizedClipName,
        eI_Metadata,
    };

    enum EOutputs
    {
        eO_BeganCapture = 0,
        eO_Error,
    };

    virtual void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] = {
            InputPortConfig_AnyType("Capture", _HELP("Begins Capturing a Clip")),
#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#if defined(TOOLS_SUPPORT_XBONE)
#define AZ_RESTRICTED_SECTION FLOWCLIPCAPTURENODES_CPP_SECTION_3
#include AZ_RESTRICTED_FILE(FlowClipCaptureNodes_cpp, TOOLS_SUPPORT_XBONE)
#endif
#if defined(TOOLS_SUPPORT_PS4)
#define AZ_RESTRICTED_SECTION FLOWCLIPCAPTURENODES_CPP_SECTION_3
#include AZ_RESTRICTED_FILE(FlowClipCaptureNodes_cpp, TOOLS_SUPPORT_PS4)
#endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
            InputPortConfig<float> ("DurationAftere", 10.0f, _HELP("")),
            InputPortConfig<string>("ClipName",_HELP("")),
            InputPortConfig<string>("LocalizedClipName", _HELP("")),
#endif

            InputPortConfig<string>("Metadata", _HELP("Optional. Use it for instance to tag clips")),
            {0}
        };

        static const SOutputPortConfig outputs[] = {
            OutputPortConfig_Void("BeganCapture", _HELP("The clip's capture has begun")),
            OutputPortConfig_Void("Error", _HELP("An error happened during the capturing process")),
            {0}
        };

        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Allows capturing clips while the game is running and (in Xbox One) save them locally or in the cloud");
        config.SetCategory(EFLN_APPROVED);
    }

#define ENABLE_GAMEDVR_NODE
    // Note: Be aware that the node may be placed in the levels in such a way that all users get a similar
    // recording or the same user gets many similar recordings. This may be unacceptable. In case you want to
    // disable video clip recording via flow graph nodes w/o removing the nodes,
    // please comment out #define ENABLE_GAMEDVR_NODE (above this comment)

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
#ifdef ENABLE_GAMEDVR_NODE
        switch (event)
        {
        case eFE_Activate:
        {
            if (IPlatformOS::IClipCaptureOS* pClipCapture = gEnv->pSystem->GetPlatformOS()->GetClipCapture())
            {
                if (pActInfo && IsPortActive(pActInfo, eI_Capture))
                {
                    const IPlatformOS::IClipCaptureOS::SSpan span(GetPortFloat(pActInfo, eI_DurationBefore), GetPortFloat(pActInfo, eI_DurationAfter));
                    const string& clipName = GetPortString(pActInfo, eI_ClipId);
                    const string& toast = GetPortString(pActInfo, eI_LocalizedClipName);
                    const string& metadata = GetPortString(pActInfo, eI_Metadata);

                    string localizedToast;
                    wstring wideToast;
                    gEnv->pSystem->GetLocalizationManager()->LocalizeString(toast, localizedToast);
                    Unicode::Convert(wideToast, localizedToast);

                    IPlatformOS::IClipCaptureOS::SClipTextInfo clipTextInfo(
                        clipName.c_str(),
                        wideToast.c_str(),
                        metadata.c_str());

                    const bool ok = pClipCapture->RecordClip(clipTextInfo, span);

                    ActivateOutput(pActInfo, ok ? eO_BeganCapture : eO_Error, true);
                }
            }
        }
        }
#endif
    }

#undef ENABLE_GAMEDVR_NODE
};

REGISTER_FLOW_NODE("Video:ClipCapture", CClipCaptureManagement);
