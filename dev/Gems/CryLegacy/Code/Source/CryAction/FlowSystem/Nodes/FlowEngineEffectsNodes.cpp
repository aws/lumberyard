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

#include "CryLegacy_precompiled.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include "IPostEffectGroup.h"


#define ADD_COMMON_INPUTS()                                                         \
    InputPortConfig<bool>("Enable", false, _HELP("Enables node"), _HELP("Enable")), \
    InputPortConfig<bool>("Disable", true, _HELP("Disables node"), _HELP("Disable"))


// Note: why is there a Enable + Disable ? Quite redundant

////////////////////////////////////////////////////////////////////////////////////////////////////
// Get custom value helper functions
////////////////////////////////////////////////////////////////////////////////////////////////////

typedef CFlowBaseNode<eNCT_Singleton> TBaseNodeClass;

static float GetCustomValue(const SFlowNodeConfig& config, TBaseNodeClass* pFlowBaseNode, bool bIsEnabled, IFlowNode::SActivationInfo* pActInfo, int portIndex, bool& isOk)
{
    float customValue = 0.0f;
    if (pFlowBaseNode && pActInfo && isOk)
    {
        if (bIsEnabled)
        {
            isOk = GetPortAny(pActInfo, portIndex).GetValueWithConversion(customValue);
        }
        else
        {
            isOk = config.pInputPorts[portIndex].defaultData.GetValueWithConversion(customValue);
        }
    }
    return customValue;
}

static void GetCustomValueVec3(Vec3& customValue, const SFlowNodeConfig& config, TBaseNodeClass* pFlowBaseNode, bool bIsEnabled, IFlowNode::SActivationInfo* pActInfo, int portIndex, bool& isOk)
{
    if (pFlowBaseNode && pActInfo && isOk)
    {
        if (bIsEnabled)
        {
            isOk = GetPortAny(pActInfo, portIndex).GetValueWithConversion(customValue);
        }
        else
        {
            isOk = config.pInputPorts[portIndex].defaultData.GetValueWithConversion(customValue);
        }
    }
}

static void GetCustomValueString(string& customValue, const SFlowNodeConfig& config, TBaseNodeClass* pFlowBaseNode, bool bIsEnabled, IFlowNode::SActivationInfo* pActInfo, int portIndex, bool& isOk)
{
    if (pFlowBaseNode && pActInfo && isOk)
    {
        if (bIsEnabled)
        {
            isOk = GetPortAny(pActInfo, portIndex).GetValueWithConversion(customValue);
        }
        else
        {
            isOk = config.pInputPorts[portIndex].defaultData.GetValueWithConversion(customValue);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Color correction
////////////////////////////////////////////////////////////////////////////////////////////////////

struct SColorCorrection
{
    static void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] =
        {
            ADD_COMMON_INPUTS(),
            InputPortConfig<float>("Global_User_ColorC", 0.0f, _HELP("Cyan"), _HELP("Cyan")),
            InputPortConfig<float>("Global_User_ColorM", 0.0f, _HELP("Magenta"), _HELP("Magenta")),
            InputPortConfig<float>("Global_User_ColorY", 0.0f, _HELP("Yellow"), _HELP("Yellow")),
            InputPortConfig<float>("Global_User_ColorK", 0.0f, _HELP("Luminance"), _HELP("Luminance")),
            InputPortConfig<float>("Global_User_Brightness", 1.0f, _HELP("Brightness"), _HELP("Brightness")),
            InputPortConfig<float>("Global_User_Contrast", 1.0f, _HELP("Contrast"), _HELP("Contrast")),
            InputPortConfig<float>("Global_User_Saturation", 1.0f, _HELP("Saturation"), _HELP("Saturation")),
            InputPortConfig<float>("Global_User_ColorHue", 0.0f, _HELP("Hue"), _HELP("Hue")),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {   // None!
            {0}
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Sets color correction parameters");
    }

    static void Disable()
    {
        SFlowNodeConfig config;
        GetConfiguration(config);

        for (int i = 2; config.pInputPorts[i].name; ++i)
        {
            if (config.pInputPorts[i].defaultData.GetType() == eFDT_Float ||
                config.pInputPorts[i].defaultData.GetType() == eFDT_Double)
            {
                float fVal;
                bool ok = config.pInputPorts[i].defaultData.GetValueWithConversion(fVal);
                if (ok)
                {
                    gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam(config.pInputPorts[i].name, fVal);
                }
            }
        }
    }

    static bool CustomSetData(const SFlowNodeConfig& config, TBaseNodeClass* pFlowBaseNode, bool bIsEnabled, IFlowNode::SActivationInfo* pActInfo)
    {
        return false;
    }

    static void Serialize(IFlowNode::SActivationInfo* actInfo, TSerialize ser)
    {
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Filters
////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
struct SFilterBlur
{
    static void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] =
        {
            ADD_COMMON_INPUTS(),
            InputPortConfig<int>("FilterBlurring_Type", 0, _HELP("Set blur type, 0 is Gaussian"), _HELP("Type")),
            InputPortConfig<float>("FilterBlurring_Amount", 0.0f, _HELP("Amount of blurring"), _HELP("Amount")),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {   // None!
            {0}
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Sets blur filter");
    }

    static void Disable()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("FilterBlurring_Amount", 0.f);
    }

    static bool CustomSetData(const SFlowNodeConfig& config, TBaseNodeClass* pFlowBaseNode, bool bIsEnabled, IFlowNode::SActivationInfo* pActInfo)
    {
        return false;
    }

    static void Serialize(IFlowNode::SActivationInfo* actInfo, TSerialize ser)
    {
    }
};

//////////////////////////////////////////////////////////////////////////
struct SFilterRadialBlur
{
    static void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] =
        {
            ADD_COMMON_INPUTS(),
            InputPortConfig<float>("FilterRadialBlurring_Amount", 0.0f, _HELP("Amount of blurring"), _HELP("Amount")),
            InputPortConfig<float>("FilterRadialBlurring_ScreenPosX", 0.5f, _HELP("Horizontal screen position"), _HELP("ScreenPosX")),
            InputPortConfig<float>("FilterRadialBlurring_ScreenPosY", 0.5f, _HELP("Vertical screen position"), _HELP("ScreenPosY")),
            InputPortConfig<float>("FilterRadialBlurring_Radius", 1.0f, _HELP("Blurring radius"), _HELP("BlurringRadius")),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {   // None!
            {0}
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Sets radial blur filter");
    }

    static void Disable()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("FilterRadialBlurring_Amount", 0.f);
    }

    static bool CustomSetData(const SFlowNodeConfig& config, TBaseNodeClass* pFlowBaseNode, bool bIsEnabled, IFlowNode::SActivationInfo* pActInfo)
    {
        return false;
    }

    static void Serialize(IFlowNode::SActivationInfo* actInfo, TSerialize ser)
    {
    }
};

//////////////////////////////////////////////////////////////////////////
struct SFilterSharpen
{
    static void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] =
        {
            ADD_COMMON_INPUTS(),
            InputPortConfig<int>("FilterSharpening_Type", 0, _HELP("Sets sharpening filter, 0 is Unsharp Mask"), _HELP("Type")),
            InputPortConfig<float>("FilterSharpening_Amount", 1.0f, _HELP("Amount of sharpening"), _HELP("Amount")),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {   // None!
            {0}
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Sets sharpen filter");
    }

    static void Disable()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("FilterSharpening_Amount", 1.0f);
    }

    static bool CustomSetData(const SFlowNodeConfig& config, TBaseNodeClass* pFlowBaseNode, bool bIsEnabled, IFlowNode::SActivationInfo* pActInfo)
    {
        return false;
    }

    static void Serialize(IFlowNode::SActivationInfo* actInfo, TSerialize ser)
    {
    }
};

//////////////////////////////////////////////////////////////////////////
struct SFilterChromaShift
{
    static void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] =
        {
            ADD_COMMON_INPUTS(),
            InputPortConfig<float>("FilterChromaShift_User_Amount", 0.0f, _HELP("Amount of chroma shift"), _HELP("Amount")),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {   // None!
            {0}
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Sets chroma shift filter");
    }

    static void Disable()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("FilterChromaShift_User_Amount", 0.f);
    }

    static bool CustomSetData(const SFlowNodeConfig& config, TBaseNodeClass* pFlowBaseNode, bool bIsEnabled, IFlowNode::SActivationInfo* pActInfo)
    {
        return false;
    }

    static void Serialize(IFlowNode::SActivationInfo* actInfo, TSerialize ser)
    {
    }
};

//////////////////////////////////////////////////////////////////////////
struct SFilterGrain
{
    static void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] =
        {
            ADD_COMMON_INPUTS(),
            InputPortConfig<float>("FilterGrain_Amount", 0.0f, _HELP("Amount of grain"), _HELP("Amount")),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {   // None!
            {0}
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Sets grain filter");
    }

    static void Disable()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("FilterGrain_Amount", 0.f);
    }

    static bool CustomSetData(const SFlowNodeConfig& config, TBaseNodeClass* pFlowBaseNode, bool bIsEnabled, IFlowNode::SActivationInfo* pActInfo)
    {
        return false;
    }

    static void Serialize(IFlowNode::SActivationInfo* actInfo, TSerialize ser)
    {
    }
};

//////////////////////////////////////////////////////////////////////////
struct SFilterVisualArtifacts
{
    static void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] =
        {
            ADD_COMMON_INPUTS(),

            InputPortConfig<float>("VisualArtifacts_Vsync", 0.0f, _HELP("Amount of vsync visible"), _HELP("VSync")),
            InputPortConfig<float>("VisualArtifacts_VsyncFreq", 1.0f, _HELP("Vsync frequency"), _HELP("VSync frequency")),

            InputPortConfig<float>("VisualArtifacts_Interlacing", 0.0f, _HELP("Amount of interlacing visible"), _HELP("Interlacing")),
            InputPortConfig<float>("VisualArtifacts_InterlacingTile", 1.0f, _HELP("Interlacing tilling"), _HELP("Interlacing tiling")),
            InputPortConfig<float>("VisualArtifacts_InterlacingRot", 0.0f, _HELP("Interlacing rotation"), _HELP("Interlacing rotation")),

            InputPortConfig<float>("VisualArtifacts_SyncWavePhase", 0.0f, _HELP("Sync wave phase"), _HELP("Sync wave phase")),
            InputPortConfig<float>("VisualArtifacts_SyncWaveFreq", 0.0f, _HELP("Sync wave frequency"), _HELP("Sync wave frequency")),
            InputPortConfig<float>("VisualArtifacts_SyncWaveAmplitude", 0.0f, _HELP("Sync wave amplitude"), _HELP("Sync wave amplitude")),

            InputPortConfig<float>("FilterArtifacts_ChromaShift", 0.0f, _HELP("Chroma shift"), _HELP("Chroma shift")),

            InputPortConfig<float>("FilterArtifacts_Grain", 0.0f, _HELP("Image grain amount"), _HELP("Grain")),

            InputPortConfig<Vec3>("clr_VisualArtifacts_ColorTint", Vec3(1.0f, 1.0f, 1.0f), _HELP("Color tinting"), _HELP("Color tinting")),

            InputPortConfig<string> ("tex_VisualArtifacts_Mask", _HELP("Texture Name")),

            {0}
        };

        static const SOutputPortConfig outputs[] =
        {   // None!
            {0}
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Sets image visual artefacts parameters");
    }

    static void Disable()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("tex_VisualArtifacts_Mask", 0.f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_ColotTint", Vec4(1.0f, 1.0f, 1.0f, 1.0f));
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_Vsync", 0.0f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_VsyncFreq", 1.0f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_Interlacing", 0.0f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_InterlacingTile", 1.0f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_InterlacingRot", 0.0f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_Pixelation", 0.0f);    // this doesn't have a port
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_Noise", 0.0f);         // this doesn't have a port
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_SyncWavePhase", 0.0f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_SyncWaveFreq", 0.0f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_SyncWaveAmplitude", 0.0f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("FilterArtifacts_ChromaShift", 0.0f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("FilterArtifacts_Grain", 0.0f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("FilterArtifacts_GrainTile", 1.0f);
    }

    static bool CustomSetData(const SFlowNodeConfig& config, TBaseNodeClass* pFlowBaseNode, bool bIsEnabled, IFlowNode::SActivationInfo* pActInfo)
    {
        return false;
    }

    static void Serialize(IFlowNode::SActivationInfo* actInfo, TSerialize ser)
    {
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// Effects
////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
struct SEffectFrost
{
    static void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] =
        {
            ADD_COMMON_INPUTS(),
            InputPortConfig<float>("ScreenFrost_Amount", 0.0f, _HELP("Amount of frost"), _HELP("Amount")),
            InputPortConfig<float>("ScreenFrost_CenterAmount", 1.0f, _HELP("Amount of frost at center of screen"), _HELP("CenterAmount")),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {   // None!
            {0}
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Freezing effect");
    }

    static void Disable()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("ScreenFrost_Amount", 0.f);
    }

    static bool CustomSetData(const SFlowNodeConfig& config, TBaseNodeClass* pFlowBaseNode, bool bIsEnabled, IFlowNode::SActivationInfo* pActInfo)
    {
        return false;
    }

    static void Serialize(IFlowNode::SActivationInfo* actInfo, TSerialize ser)
    {
    }
};

//////////////////////////////////////////////////////////////////////////
struct SEffectCondensation
{
    static void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] =
        {
            ADD_COMMON_INPUTS(),
            InputPortConfig<float>("ScreenCondensation_Amount", 0.0f, _HELP("Amount of condensation"), _HELP("Amount")),
            InputPortConfig<float>("ScreenCondensation_CenterAmount", 1.0f, _HELP("Amount of condensation at center of screen"), _HELP("CenterAmount")),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {   // None!
            {0}
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Condensation effect");
        config.SetCategory(EFLN_OBSOLETE);
    }

    static void Disable()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("ScreenCondensation_Amount", 0.f);
    }

    static bool CustomSetData(const SFlowNodeConfig& config, TBaseNodeClass* pFlowBaseNode, bool bIsEnabled, IFlowNode::SActivationInfo* pActInfo)
    {
        return false;
    }

    static void Serialize(IFlowNode::SActivationInfo* actInfo, TSerialize ser)
    {
    }
};

//////////////////////////////////////////////////////////////////////////
struct SEffectWaterDroplets
{
    static void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] =
        {
            ADD_COMMON_INPUTS(),
            InputPortConfig<float>("WaterDroplets_Amount", 0.0f, _HELP("Amount of water"), _HELP("Amount")),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {   // None!
            {0}
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Water droplets effect - Use when camera goes out of water or similar extreme condition");
    }

    static void Disable()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("WaterDroplets_Amount", 0.f);
    }

    static bool CustomSetData(const SFlowNodeConfig& config, TBaseNodeClass* pFlowBaseNode, bool bIsEnabled, IFlowNode::SActivationInfo* pActInfo)
    {
        return false;
    }

    static void Serialize(IFlowNode::SActivationInfo* actInfo, TSerialize ser)
    {
    }
};

//////////////////////////////////////////////////////////////////////////
struct SEffectWaterFlow
{
    static void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] =
        {
            ADD_COMMON_INPUTS(),
            InputPortConfig<float>("WaterFlow_Amount", 0.0f, _HELP("Amount of water"), _HELP("Amount")),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {   // None!
            {0}
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Water flow effect - Use when camera receiving splashes of water or similar");
    }

    static void Disable()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("WaterFlow_Amount", 0.f);
    }

    static bool CustomSetData(const SFlowNodeConfig& config, TBaseNodeClass* pFlowBaseNode, bool bIsEnabled, IFlowNode::SActivationInfo* pActInfo)
    {
        return false;
    }

    static void Serialize(IFlowNode::SActivationInfo* actInfo, TSerialize ser)
    {
    }
};

//////////////////////////////////////////////////////////////////////////
struct SEffectBloodSplats
{
    static void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] =
        {
            ADD_COMMON_INPUTS(),
            InputPortConfig<int>("BloodSplats_Type", 0, _HELP("Blood type: human or alien"), _HELP("Type")),
            InputPortConfig<float>("BloodSplats_Amount", 0.0f, _HELP("Amount of visible blood"), _HELP("Amount")),
            InputPortConfig<bool>("BloodSplats_Spawn", false, _HELP("Spawn more blood particles"), _HELP("Spawn")),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {   // None!
            {0}
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Blood splats effect");
    }

    static void Disable()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("BloodSplats_Amount", 0.f);
    }

    static bool CustomSetData(const SFlowNodeConfig& config, TBaseNodeClass* pFlowBaseNode, bool bIsEnabled, IFlowNode::SActivationInfo* pActInfo)
    {
        return false;
    }

    static void Serialize(IFlowNode::SActivationInfo* actInfo, TSerialize ser)
    {
    }
};

//////////////////////////////////////////////////////////////////////////
struct SEffectDof
{
    static void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] =
        {
            ADD_COMMON_INPUTS(),
            InputPortConfig<bool>("Dof_User_Active", false, _HELP("Enables depth of field (dof) effect"), _HELP("EnableDof")),
            InputPortConfig<float>("Dof_User_FocusDistance", 3.5f, _HELP("Set focus distance"), _HELP("FocusDistance")),
            InputPortConfig<float>("Dof_User_FocusRange", 5.0f, _HELP("Set focus range"), _HELP("FocusRange")),
            InputPortConfig<float>("Dof_User_BlurAmount", 1.0f, _HELP("Set blurring amount"), _HELP("BlurAmount")),
            InputPortConfig<float>("Dof_MaxCoC", 12.0f, _HELP("Set circle of confusion scale"), _HELP("ScaleCoC")),
            InputPortConfig<float>("Dof_CenterWeight", 1.0f, _HELP("Set central samples weight"), _HELP("CenterWeight")),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {   // None!
            {0}
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Depth of field effect");
    }

    static void Disable()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("Dof_User_Active", 0.f);
    }

    static bool CustomSetData(const SFlowNodeConfig& config, TBaseNodeClass* pFlowBaseNode, bool bIsEnabled, IFlowNode::SActivationInfo* pActInfo)
    {
        return false;
    }

    static void Serialize(IFlowNode::SActivationInfo* actInfo, TSerialize ser)
    {
    }
};

//////////////////////////////////////////////////////////////////////////
struct SEffectDirectionalBlur
{
    static void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] =
        {
            ADD_COMMON_INPUTS(),
            InputPortConfig<Vec3>("Global_DirectionalBlur_Vec", Vec3(0.0f, 0.0f, 0.0f), _HELP("Sets directional blur direction"), _HELP("Direction")),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {   // None!
            {0}
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Directional Blur effect");
    }

    static void Disable()
    {
        Vec4 vec(0.f, 0.f, 0.f, 1.f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("Global_DirectionalBlur_Vec", vec);
    }

    static bool CustomSetData(const SFlowNodeConfig& config, TBaseNodeClass* pFlowBaseNode, bool bIsEnabled, IFlowNode::SActivationInfo* pActInfo)
    {
        return false;
    }

    static void Serialize(IFlowNode::SActivationInfo* actInfo, TSerialize ser)
    {
    }
};

//////////////////////////////////////////////////////////////////////////
struct SEffectAlienInterference
{
    static void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] =
        {
            ADD_COMMON_INPUTS(),
            InputPortConfig<float>("AlienInterference_Amount", 0.0f, _HELP("Sets alien interference amount"), _HELP("Amount")),
            InputPortConfig<Vec3>("clr_AlienInterference_Color", Vec3(0.85f, 0.95f, 1.25f) * 0.5f, _HELP("Sets alien interference color"), _HELP("Color")),

            {0}
        };

        static const SOutputPortConfig outputs[] =
        {   // None!
            {0}
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Alien interference effect");
    }

    static void Disable()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("AlienInterference_Amount", 0.f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("clr_AlienInterference_TintColor", Vec4(Vec3(0.85f, 0.95f, 1.25f) * 0.5f, 1.0f));
    }

    static bool CustomSetData(const SFlowNodeConfig& config, TBaseNodeClass* pFlowBaseNode, bool bIsEnabled, IFlowNode::SActivationInfo* pActInfo)
    {
        return false;
    }

    static void Serialize(IFlowNode::SActivationInfo* actInfo, TSerialize ser)
    {
    }
};

struct SEffectGhostVision
{
    static void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig<bool> ("GhostVision_Bool1", false, "User defined bool", "UserBool1"),
            InputPortConfig<bool> ("GhostVision_Bool2", false, "User defined bool", "UserBool2"),
            InputPortConfig<bool> ("GhostVision_Bool3", false, "User defined bool", "UserBool3"),
            InputPortConfig<float>("GhostVision_Amount1", 0.0f, "User defined value", "FloatValue1"),
            InputPortConfig<float> ("GhostVision_Amount2", 0.0f, "User defined value", "FloatValue2"),
            InputPortConfig<float> ("GhostVision_Amount3", 0.0f, "User defined value", "FloatValue3"),
            InputPortConfig<Vec3>("clr_GhostVision_Color", Vec3(0.85f, 0.95f, 1.25f) * 0.5f, "Sets volumetric scattering amount", "Color"),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {
            {0}
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Ghost Vision Effect");
    }

    static void Disable()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("GhostVision_Bool1", 0.f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("GhostVision_Bool2", 0.f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("GhostVision_Bool3", 0.f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("User_FloatValue_1", 0.f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("User_FloatValue_2", 0.f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("User_FloatValue_3", 0.f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("clr_GhostVision_TintColor", Vec4(Vec3(0.85f, 0.95f, 1.25f) * 0.5f, 1.0f));
    }

    static bool CustomSetData(const SFlowNodeConfig& config, TBaseNodeClass* pFlowBaseNode, bool bIsEnabled, IFlowNode::SActivationInfo* pActInfo) { return false; }
    static void Serialize(IFlowNode::SActivationInfo* pActInfo, TSerialize ser){}
};

struct SEffectRainDrops
{
    static void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] =
        {
            ADD_COMMON_INPUTS(),
            InputPortConfig<float>("RainDrops_Amount", 0.0f, _HELP("Sets rain drops visibility"), _HELP("Amount")),
            InputPortConfig<float>("RainDrops_SpawnTimeDistance", 0.35f, _HELP("Sets rain drops spawn time distance"), _HELP("Spawn Time Distance")),
            InputPortConfig<float>("RainDrops_Size", 5.0f, _HELP("Sets rain drops size"), _HELP("Size")),
            InputPortConfig<float>("RainDrops_SizeVariation", 2.5f, _HELP("Sets rain drops size variation"), _HELP("Size Variation")),

            InputPortConfig<float>("Moisture_Amount", 0.0f, _HELP("Sets moisture visibility/area size"), _HELP("Moisture Amount")),
            InputPortConfig<float>("Moisture_Hardness", 10.0f, _HELP("Sets noise texture blending factor"), _HELP("Moisture Hardness")),
            InputPortConfig<float>("Moisture_DropletAmount", 7.0f, _HELP("Sets droplet texture blending factor"), _HELP("Moisture Droplet Amount")),
            InputPortConfig<float>("Moisture_Variation", 0.2f, _HELP("Sets moisture variation"), _HELP("Moisture Variation")),
            InputPortConfig<float>("Moisture_Speed", 1.0f, _HELP("Sets moisture animation speed"), _HELP("Moisture Speed")),
            InputPortConfig<float>("Moisture_FogAmount", 1.0f, _HELP("Sets amount of fog in moisture"), _HELP("Moisture Fog Amount")),

            {0}
        };

        static const SOutputPortConfig outputs[] =
        {   // None!
            {0}
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Rain drops effect");
    }

    static void Disable()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("RainDrops_Amount", 0.f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("Moisture_Amount", 0.f);
    }

    static bool CustomSetData(const SFlowNodeConfig& config, TBaseNodeClass* pFlowBaseNode, bool bIsEnabled, IFlowNode::SActivationInfo* pActInfo)
    {
        return false;
    }

    static void Serialize(IFlowNode::SActivationInfo* actInfo, TSerialize ser)
    {
    }
};

//////////////////////////////////////////////////////////////////////////
struct SEffectVolumetricScattering
{
    static void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] =
        {
            ADD_COMMON_INPUTS(),
            InputPortConfig<float>("VolumetricScattering_Amount", 0.0f, _HELP("Sets volumetric scattering amount"), _HELP("Amount")),
            InputPortConfig<float>("VolumetricScattering_Tilling", 1.0f, _HELP("Sets volumetric scattering tilling"), _HELP("Tilling")),
            InputPortConfig<float>("VolumetricScattering_Speed", 1.0f, _HELP("Sets volumetric scattering animation speed"), _HELP("Speed")),
            InputPortConfig<Vec3>("clr_VolumetricScattering_Color", Vec3(0.6f, 0.75f, 1.0f), _HELP("Sets volumetric scattering amount"), _HELP("Color")),
            InputPortConfig<int>("VolumetricScattering_Type", 0, _HELP("Set volumetric scattering type"), _HELP("Type")),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {   // None!
            {0}
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Volumetric scattering effect");
    }

    static void Disable()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VolumetricScattering_Amount", 0.f);
    }

    static bool CustomSetData(const SFlowNodeConfig& config, TBaseNodeClass* pFlowBaseNode, bool bIsEnabled, IFlowNode::SActivationInfo* pActInfo)
    {
        return false;
    }

    static void Serialize(IFlowNode::SActivationInfo* actInfo, TSerialize ser)
    {
    }
};

//////////////////////////////////////////////////////////////////////////
struct SEffectGhosting
{
    static void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] =
        {
            ADD_COMMON_INPUTS(),
            InputPortConfig<float>("ImageGhosting_Amount", 0.0f, _HELP("Enables ghosting effect"), _HELP("GhostingAmount")),
            {0}
        };

        static const SOutputPortConfig outputs[] =
        {   // None!
            {0}
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Ghosting effect");
    }

    static void Disable()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("Scratches_Strength", 0.f);
    }

    static bool CustomSetData(const SFlowNodeConfig& config, TBaseNodeClass* pFlowBaseNode, bool bIsEnabled, IFlowNode::SActivationInfo* pActInfo)
    {
        return false;
    }

    static void Serialize(IFlowNode::SActivationInfo* actInfo, TSerialize ser)
    {
    }
};

//////////////////////////////////////////////////////////////////////////

template<class T>
class CFlowImageNode
    : public TBaseNodeClass
{
    enum InputPorts
    {
        Enabled,
        Disabled,
    };

public:
    CFlowImageNode(SActivationInfo* pActInfo)
    {
    }

    ~CFlowImageNode() override
    {
    }

    // It's now a singleton node, so Clone must NOT be implemented!

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        T::GetConfiguration(config);
        if (!(config.nFlags & EFLN_OBSOLETE))
        {
            config.SetCategory(EFLN_ADVANCED);
        }
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        if (event != eFE_Activate)
        {
            return;
        }

        if (!InputEntityIsLocalPlayer(pActInfo))
        {
            return;
        }

        bool bIsEnabled = IsPortActive(pActInfo, InputPorts::Enabled) && GetPortBool(pActInfo, InputPorts::Enabled);
        const bool bIsDisabled = IsPortActive(pActInfo, InputPorts::Disabled) && GetPortBool(pActInfo, InputPorts::Disabled);

        if (!bIsDisabled && GetPortBool(pActInfo, InputPorts::Enabled))  // this is to support when effects parameters are changed after the effect was activated. would be easier to just make sure that the bool value of both inputs is always complementary, but that conflicts on how triggers work, and other nodes already rely on that way of working.
        {
            bIsEnabled = true;
        }

        if (!bIsEnabled)
        {
            T::Disable();
            return;
        }

        SFlowNodeConfig config;
        T::GetConfiguration(config);
        if (T::CustomSetData(config, this, bIsEnabled, pActInfo) == false)
        {
            I3DEngine* pEngine = gEnv->p3DEngine;

            for (int i = 2; config.pInputPorts[i].name; ++i)
            {
                const TFlowInputData& anyVal = GetPortAny(pActInfo, i);

                switch (anyVal.GetType())
                {
                case eFDT_Vec3:
                {
                    Vec3 pVal(GetPortVec3(pActInfo, i));
                    if (!bIsEnabled)
                    {
                        config.pInputPorts[i].defaultData.GetValueWithConversion(pVal);
                    }
                    pEngine->GetPostEffectBaseGroup()->SetParam(config.pInputPorts[i].name, Vec4(pVal, 1));
                    break;
                }

                case eFDT_String:
                {
                    const string& pszString = GetPortString(pActInfo, i);
                    pEngine->GetPostEffectBaseGroup()->SetParam(config.pInputPorts[i].name, pszString.c_str());
                    break;
                }

                default:
                {
                    float fVal;
                    bool ok(anyVal.GetValueWithConversion(fVal));

                    if (!bIsEnabled)
                    {
                        ok = config.pInputPorts[i].defaultData.GetValueWithConversion(fVal);
                    }

                    if (ok)
                    {
                        pEngine->GetPostEffectBaseGroup()->SetParam(config.pInputPorts[i].name, fVal);
                    }

                    break;
                }
                }
            }
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    void Serialize(IFlowNode::SActivationInfo* actInfo, TSerialize ser) override
    {
        T::Serialize(actInfo, ser);
    }
};


//////////////////////////////////////////////////////////////////////////////////////
class CFlowNode_SetShadowMode
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate,
        ShadowMode,
    };

public:
    CFlowNode_SetShadowMode(SActivationInfo* pActInfo)
    {
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<int>("ShadowMode", 0, _HELP("Shadow mode to activate: 0 = Normal, 1 = HighQuality"), _HELP("ShadowMode"), _UICONFIG("enum_int:Normal=0,HighQuality=1")), // the values needs to coincide with EShadowMode
            {0}
        };

        static const SOutputPortConfig out_config [] =
        {   // None!
            {0}
        };

        config.sDescription = _HELP("Sets the shadow mode for the 3D Engine.\nNormal = 0\nHigh Quality = 1");
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                EShadowMode shadowMode = EShadowMode(GetPortInt(pActInfo, InputPorts::ShadowMode));
                gEnv->p3DEngine->SetShadowMode(shadowMode);
            }
            break;
        }
        }
    }
};

class FlowNode_EffectGroup
    : public TBaseNodeClass
{
public:
    FlowNode_EffectGroup(SActivationInfo* pActInfo)
    {
    }

    enum Inputs
    {
        In_Enabled = 0,
        In_Disabled,
        In_GroupName,
    };

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig inputs[] =
        {
            InputPortConfig_Void("Enabled", "Enables node", "Enabled"),
            InputPortConfig_Void("Disabled", "Disables node", "Disabled"),
            InputPortConfig<string>("GroupName", _HELP("Effect group name")),
            { 0 }
        };

        static const SOutputPortConfig outputs[] =
        {
            { 0 }
        };

        config.nFlags |= EFLN_TARGET_ENTITY;
        config.pInputPorts = inputs;
        config.pOutputPorts = outputs;
        config.sDescription = _HELP("Enables an effect group");
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_PrecacheResources: // Load from XML if effect group isn't loaded
        {
            GetGroup(pActInfo);
            break;
        }
        case eFE_Activate:
        {
            auto group = GetGroup(pActInfo);
            if (group)
            {
                if (IsPortActive(pActInfo, In_Enabled))
                {
                    group->SetEnable(true);
                }
                if (IsPortActive(pActInfo, In_Disabled))
                {
                    group->SetEnable(false);
                }
            }
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, group && group->GetFadeDistance());
            break;
        }
        case eFE_Update: // Apply effect at distance of entity from camera
        {
            if (pActInfo->pEntity)
            {
                auto group = GetGroup(pActInfo);
                if (group)
                {
                    group->ApplyAtPosition(pActInfo->pEntity->GetWorldPos());
                }
            }
            break;
        }
        case eFE_Uninitialize:
        {
            // Forcably disable the PostEffectGroup when its flowgraph node is uninitialized
            auto group = GetGroup(pActInfo);
            if (group)
            {
                group->SetEnable(false);
            }
            break;
        }
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

private:
    IPostEffectGroup* GetGroup(SActivationInfo* pActInfo)
    {
        return gEnv->p3DEngine->GetPostEffectGroups()->GetGroup(GetPortString(pActInfo, In_GroupName).c_str());
    }
};


//////////////////////////////////////////////////////////////////////////
/// Image Flow Node Registration
//////////////////////////////////////////////////////////////////////////

REGISTER_FLOW_NODE_EX("Image:ColorCorrection", CFlowImageNode<SColorCorrection>, SColorCorrection);
REGISTER_FLOW_NODE_EX("Image:EffectFrost", CFlowImageNode<SEffectFrost>, SEffectFrost);
REGISTER_FLOW_NODE_EX("Image:EffectCondensation", CFlowImageNode<SEffectCondensation>, SEffectCondensation);
REGISTER_FLOW_NODE_EX("Image:EffectWaterDroplets", CFlowImageNode<SEffectWaterDroplets>, SEffectWaterDroplets);
REGISTER_FLOW_NODE_EX("Image:EffectWaterFlow", CFlowImageNode<SEffectWaterFlow>, SEffectWaterFlow);
REGISTER_FLOW_NODE_EX("Image:EffectBloodSplats", CFlowImageNode<SEffectBloodSplats>, SEffectBloodSplats);
REGISTER_FLOW_NODE_EX("Image:EffectGhosting", CFlowImageNode<SEffectGhosting>, SEffectGhosting);
REGISTER_FLOW_NODE_EX("Image:EffectDepthOfField", CFlowImageNode<SEffectDof>, SEffectDof);
REGISTER_FLOW_NODE_EX("Image:EffectVolumetricScattering", CFlowImageNode<SEffectVolumetricScattering>, SEffectVolumetricScattering);
REGISTER_FLOW_NODE_EX("Image:EffectAlienInterference", CFlowImageNode<SEffectAlienInterference>, SEffectAlienInterference);
REGISTER_FLOW_NODE_EX("Image:EffectRainDrops", CFlowImageNode<SEffectRainDrops>, SEffectRainDrops);
REGISTER_FLOW_NODE_EX("Image:FilterBlur", CFlowImageNode<SFilterBlur>, SFilterBlur);
REGISTER_FLOW_NODE_EX("Image:FilterRadialBlur", CFlowImageNode<SFilterRadialBlur>, SFilterRadialBlur);
REGISTER_FLOW_NODE_EX("Image:FilterSharpen", CFlowImageNode<SFilterSharpen>, SFilterSharpen);
REGISTER_FLOW_NODE_EX("Image:FilterChromaShift", CFlowImageNode<SFilterChromaShift>, SFilterChromaShift);
REGISTER_FLOW_NODE_EX("Image:FilterGrain", CFlowImageNode<SFilterGrain>, SFilterGrain);
REGISTER_FLOW_NODE_EX("Image:FilterDirectionalBlur", CFlowImageNode<SEffectDirectionalBlur>, SEffectDirectionalBlur);
REGISTER_FLOW_NODE_EX("Image:FilterVisualArtifacts", CFlowImageNode<SFilterVisualArtifacts>, SFilterVisualArtifacts);
REGISTER_FLOW_NODE("Image:SetShadowMode", CFlowNode_SetShadowMode);
REGISTER_FLOW_NODE("Image:EffectGroup", FlowNode_EffectGroup);
