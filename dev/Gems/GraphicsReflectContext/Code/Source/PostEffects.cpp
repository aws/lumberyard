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

#include "PostEffects.h"

#include <I3DEngine.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <MathConversion.h>
#include <IPostEffectGroup.h>
#include <I3DEngine.h>
#include <ColorGradingBus.h>

namespace GraphicsReflectContext
{
    void PostEffects::EnableWaterDroplets(float amount)
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("WaterDroplets_Amount", amount);
    }

    void PostEffects::DisableWaterDroplets()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("WaterDroplets_Amount", 0.0f);
    }

    void PostEffects::EnableDepthOfField(float focusDistance, float FocusRange, float blurAmount, float maxCoC, float centerWeight)
    {
        I3DEngine* pEngine = gEnv->p3DEngine;
        pEngine->GetPostEffectBaseGroup()->SetParam("Dof_User_Active", true);
        pEngine->GetPostEffectBaseGroup()->SetParam("Dof_User_FocusDistance", focusDistance);
        pEngine->GetPostEffectBaseGroup()->SetParam("Dof_User_FocusRange", FocusRange);
        pEngine->GetPostEffectBaseGroup()->SetParam("Dof_User_BlurAmount", blurAmount);
        pEngine->GetPostEffectBaseGroup()->SetParam("Dof_MaxCoC", maxCoC);
        pEngine->GetPostEffectBaseGroup()->SetParam("Dof_CenterWeight", centerWeight);
    }

    void PostEffects::DisableDepthOfField()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("Dof_User_Active", 0.0f);
    }

    void PostEffects::EnableBlur(float amount)
    {
        //Note: FilterBlurring_Type is not used in CFilterBlurring. So it's not exposed to user here.
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("FilterBlurring_Amount", amount);
    }

    void PostEffects::DisableBlur()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("FilterBlurring_Amount", 0.0f);
    }

    void PostEffects::EnableRadialBlur(float amount, const AZ::Vector2& screenPos, float radius)
    {
        ICVar* motionBlur = gEnv->pConsole->GetCVar("r_MotionBlur");
        AZ_Warning("Script", motionBlur && motionBlur->GetIVal(), "r_MotionBlur cvar must be enabled for Radial Blur effect to work.");

        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("FilterRadialBlurring_Amount", amount);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("FilterRadialBlurring_ScreenPosX", screenPos.GetX());
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("FilterRadialBlurring_ScreenPosY", screenPos.GetY());
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("FilterRadialBlurring_Radius", radius);
    }

    void PostEffects::DisableRadialBlur()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("FilterRadialBlurring_Amount", 0.f);
    }




    void PostEffects::EnableColorCorrection(float colorC, float colorM, float colorY, float colorK, float brightness,
        float contrast, float saturation, float colorHue)
    {
        I3DEngine* pEngine = gEnv->p3DEngine;
        pEngine->GetPostEffectBaseGroup()->SetParam("Global_User_ColorC", colorC);
        pEngine->GetPostEffectBaseGroup()->SetParam("Global_User_ColorM", colorM);
        pEngine->GetPostEffectBaseGroup()->SetParam("Global_User_ColorY", colorY);
        pEngine->GetPostEffectBaseGroup()->SetParam("Global_User_ColorK", colorK);
        pEngine->GetPostEffectBaseGroup()->SetParam("Global_User_Brightness", brightness);
        pEngine->GetPostEffectBaseGroup()->SetParam("Global_User_Contrast", contrast);
        pEngine->GetPostEffectBaseGroup()->SetParam("Global_User_Saturation", saturation);
        pEngine->GetPostEffectBaseGroup()->SetParam("Global_User_ColorHue", colorHue);
    }

    void PostEffects::DisableColorCorrection()
    {
        I3DEngine* pEngine = gEnv->p3DEngine;

        //Disable effect. Set to default values.
        pEngine->GetPostEffectBaseGroup()->SetParam("Global_User_ColorC", 0.0f);
        pEngine->GetPostEffectBaseGroup()->SetParam("Global_User_ColorM", 0.0f);
        pEngine->GetPostEffectBaseGroup()->SetParam("Global_User_ColorY", 0.0f);
        pEngine->GetPostEffectBaseGroup()->SetParam("Global_User_ColorK", 0.0f);
        pEngine->GetPostEffectBaseGroup()->SetParam("Global_User_Brightness", 1.0f);
        pEngine->GetPostEffectBaseGroup()->SetParam("Global_User_Contrast", 1.0f);
        pEngine->GetPostEffectBaseGroup()->SetParam("Global_User_Saturation", 1.0f);
        pEngine->GetPostEffectBaseGroup()->SetParam("Global_User_ColorHue", 0.0f);
    }

    void PostEffects::EnableFrost(float amount, float centerAmount)
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("ScreenFrost_Amount", amount);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("ScreenFrost_CenterAmount", centerAmount);
    }

    void PostEffects::DisableFrost()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("ScreenFrost_Amount", 0.0f);
    }
         
    void PostEffects::EnableWaterFlow(float amount)
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("WaterFlow_Amount", amount);
    }

    void PostEffects::DisableWaterFlow()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("WaterFlow_Amount", 0.0f);
    }

         
    void PostEffects::EnableGhosting(float amount)
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("ImageGhosting_Amount", amount);
    }

    void PostEffects::DisableGhosting()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("ImageGhosting_Amount", 0.0f);
    }

         
    void PostEffects::EnableVolumetricScattering(float amount, float tiling, float speed, const AZ::Color& color)
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VolumetricScattering_Amount", amount);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VolumetricScattering_Tilling", tiling);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VolumetricScattering_Speed", speed);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("clr_VolumetricScattering_Color", AZColorToLYVec4(color));
        // VolumetricScattering_Type isn't actually used.
    }

    void PostEffects::DisableVolumetricScattering()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VolumetricScattering_Amount", 0.0f);
    }

         
    void PostEffects::EnableRainDrops(float amount, float spawnTimeDistance, float size, float sizeVariation)
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("RainDrops_Amount", amount);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("RainDrops_SpawnTimeDistance", spawnTimeDistance);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("RainDrops_Size", size);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("RainDrops_SizeVariation", sizeVariation);
        // All the "Moisture_" params shown in Flow Graph aren't actually used anywhere
    }

    void PostEffects::DisableRainDrops()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("RainDrops_Amount", 0.0f);
    }

         
    void PostEffects::EnableSharpen(float amount)
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("FilterSharpening_Amount", amount);
    }

    void PostEffects::DisableSharpen()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("FilterSharpening_Amount", 1.0f);
    }
             
    void PostEffects::EnableDirectionalBlur(const AZ::Vector2& direction)
    {
        ICVar* motionBlur = gEnv->pConsole->GetCVar("r_MotionBlur");
        AZ_Warning("Script", motionBlur && motionBlur->GetIVal(), "r_MotionBlur cvar must be enabled for Directional Blur effect to work.");

        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("Global_DirectionalBlur_Vec", Vec4(direction.GetX(), direction.GetY(), 0.0f, 0.0f));
    }

    void PostEffects::DisableDirectionalBlur()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("Global_DirectionalBlur_Vec", Vec4(0.f, 0.f, 0.f, 0.0f));
    }

         
    void PostEffects::EnableVisualArtifacts(float vsync, float vsyncFrequency, float interlacing, float interlacingTile, float interlacingRot, float syncWavePhase, float syncWaveFrequency, float syncWaveAmplitude, float chromaShift, float grain, const AZ::Color& colorTint, const AZStd::string& maskTextureName)
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_Vsync", vsync);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_VsyncFreq", vsyncFrequency);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_Interlacing", interlacing);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_InterlacingTile", interlacingTile);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_InterlacingRot", interlacingRot);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_SyncWavePhase", syncWavePhase);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_SyncWaveFreq", syncWaveFrequency);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_SyncWaveAmplitude", syncWaveAmplitude);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("FilterArtifacts_ChromaShift", chromaShift);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("FilterArtifacts_Grain", grain);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("clr_VisualArtifacts_ColorTint", AZColorToLYVec4(colorTint));
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("tex_VisualArtifacts_Mask", maskTextureName);
    }

    void PostEffects::DisableVisualArtifacts()
    {
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_ColotTint", Vec4(1.0f, 1.0f, 1.0f, 1.0f));   // This is only included because the legacy FilterVisualArtifacts node had it. It might not be necessary.
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("clr_VisualArtifacts_ColorTint", Vec4(1.0f, 1.0f, 1.0f, 1.0f));
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_Vsync", 0.0f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_VsyncFreq", 1.0f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_Interlacing", 0.0f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_InterlacingTile", 1.0f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_InterlacingRot", 0.0f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_Pixelation", 0.0f);    // This is only included because the legacy FilterVisualArtifacts node had it. It might not be necessary.
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_Noise", 0.0f);         // This is only included because the legacy FilterVisualArtifacts node had it. It might not be necessary.
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_SyncWavePhase", 0.0f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_SyncWaveFreq", 0.0f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("VisualArtifacts_SyncWaveAmplitude", 0.0f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("FilterArtifacts_ChromaShift", 0.0f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("FilterArtifacts_Grain", 0.0f);
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("FilterArtifacts_GrainTile", 1.0f); // This is only included because the legacy FilterVisualArtifacts node had it. It might not be necessary.
        gEnv->p3DEngine->GetPostEffectBaseGroup()->SetParam("tex_VisualArtifacts_Mask", AZStd::string());
    }

    void PostEffects::EnableEffectGroup(AZStd::string_view groupName)
    {
        auto group = GetGroup(groupName.data());
        if (group)
        {
            group->SetEnable(true);
        }
    }
    
    void PostEffects::DisableEffectGroup(AZStd::string_view groupName)
    {
        auto group = GetGroup(groupName.data());
        if (group)
        {
            group->SetEnable(false);
        }
    }
    
    void PostEffects::ApplyEffectGroupAtPosition(AZStd::string_view groupName, const AZ::Vector3& position)
    {
        auto group = GetGroup(groupName.data());
        if (group && group->GetEnable())
        {
            group->ApplyAtPosition(AZVec3ToLYVec3(position));
        }
    }

    IPostEffectGroup* PostEffects::GetGroup(const char* groupName)
    {
        return gEnv->p3DEngine->GetPostEffectGroups()->GetGroup(groupName);
    }
    
    void PostEffects::SetColorChart(AZStd::string_view textureName, float fadeTime)
    {
        const uint32 COLORCHART_TEXFLAGS = FT_NOMIPS | FT_DONT_STREAM | FT_STATE_CLAMP;
        gEnv->pRenderer->EF_LoadTexture(textureName.data(), COLORCHART_TEXFLAGS);
        AZ::ColorGradingRequestBus::Broadcast(&AZ::ColorGradingRequests::FadeInColorChart, textureName, fadeTime);
    }

    void PostEffects::ReflectBehaviorContext(AZ::BehaviorContext *behaviorContext)
    {
        //effect water droplets
        behaviorContext->Class<PostEffects>("PostEffects")
            ->Attribute(AZ::Script::Attributes::Category, "Rendering")
            ->Method("EnableWaterDroplets", 
                &PostEffects::EnableWaterDroplets, 
                { {
                    { "Amount", "Amount of water",   behaviorContext->MakeDefaultValue(5.0f) }
                } } )
            ->Attribute(AZ::Script::Attributes::ToolTip, "Draws sheets of water running down the screen, like when the camera emerges from a pool of water")
            ->Method("DisableWaterDroplets", &PostEffects::DisableWaterDroplets)

            ->Method("EnableDepthOfField",
                &PostEffects::EnableDepthOfField,
                { { 
                    { "FocusDistance",  "Set focus distance",                  behaviorContext->MakeDefaultValue(3.5f) },
                    { "FocusRange",     "Set focus range",                     behaviorContext->MakeDefaultValue(5.0f) },
                    { "BlurAmount",     "Set blurring amount",                 behaviorContext->MakeDefaultValue(1.0f) },
                    { "ScaleCoC",       "Set circle of confusion scale",       behaviorContext->MakeDefaultValue(12.0f) },
                    { "CenterWeight",   "Set central samples weight",          behaviorContext->MakeDefaultValue(1.0f) }
                } } )
            ->Attribute(AZ::Script::Attributes::ToolTip, "Applies a depth-of-field effect")
            ->Method("DisableDepthOfField", &PostEffects::DisableDepthOfField)

            ->Method("EnableBlur",
                &PostEffects::EnableBlur,
                { {
                    { "Amount", "Amount of blurring",           behaviorContext->MakeDefaultValue(1.0f) }
                } } )
            ->Attribute(AZ::Script::Attributes::ToolTip, "Applies a blur filter")
            ->Method("DisableBlur", &PostEffects::DisableBlur)

            ->Method("EnableRadialBlur",
                &PostEffects::EnableRadialBlur,
                { {
                    { "Amount",     "Amount of blurring",                        behaviorContext->MakeDefaultValue(1.0f) },
                    { "ScreenPos",  "Screen position of the center of the blur", behaviorContext->MakeDefaultValue(AZ::Vector2(0.5f, 0.5f)) },
                    { "Radius",     "Blurring radius",                           behaviorContext->MakeDefaultValue(1.0f) }
                } } )
            ->Attribute(AZ::Script::Attributes::ToolTip, "Applies a radial blur filter")
            ->Method("DisableRadialBlur", &PostEffects::DisableRadialBlur)

            ->Method("EnableColorCorrection",
                &PostEffects::EnableColorCorrection,
                //name and tooltips
                { {
                    { "Cyan",       "Adjust Cyan to enhance color of scene",                                    behaviorContext->MakeDefaultValue(0.0f) },
                    { "Magenta",    "Adjust Magenta to enhance color of scene",                                 behaviorContext->MakeDefaultValue(0.0f) },
                    { "Yellow",     "Adjust Yellow to enhance color of scene",                                  behaviorContext->MakeDefaultValue(0.0f) },
                    { "Luminance",  "Adjust Luminance to enhance the light intensity of scene",                 behaviorContext->MakeDefaultValue(0.0f) },
                    { "Brightness", "Adjust Brightness to enhance light and darkness of scene",                 behaviorContext->MakeDefaultValue(1.0f) },
                    { "Contrast",   "Adjust Contrast to enhance the bias of highlights and shadows of scene",   behaviorContext->MakeDefaultValue(1.0f) },
                    { "Saturation", "Adjust Saturation to enhance the color intensity of scene",                behaviorContext->MakeDefaultValue(1.0f) },
                    { "Hue",        "Adjust Hue to enhance the color globally",                                 behaviorContext->MakeDefaultValue(0.0f) }
                } } )
            ->Attribute(AZ::Script::Attributes::ToolTip, "Sets color correction parameters")
            ->Method("DisableColorCorrection", &PostEffects::DisableColorCorrection)
                    
            ->Method("EnableFrost",
                &PostEffects::EnableFrost,
                { {
                    { "Amount",        "Amount of frost",                     behaviorContext->MakeDefaultValue(0.0f) },
                    { "CenterAmount",  "Amount of frost at center of screen", behaviorContext->MakeDefaultValue(1.0f) }
                } } )
            ->Attribute(AZ::Script::Attributes::ToolTip, "Displays splats of frost on the screen")
            ->Method("DisableFrost", &PostEffects::DisableFrost)
                    
            ->Method("EnableWaterFlow",
                &PostEffects::EnableWaterFlow,
                { {
                    { "Amount",  "Amount of water",   behaviorContext->MakeDefaultValue(1.0f) }
                } } )
            ->Attribute(AZ::Script::Attributes::ToolTip, "Draws water flowing down the entire screen")
            ->Method("DisableWaterFlow", &PostEffects::DisableWaterFlow)
                    
            ->Method("EnableGhosting",
                &PostEffects::EnableGhosting,
                { {
                    { "Amount",  "Strength of ghosting effect",   behaviorContext->MakeDefaultValue(0.0f) }
                } } )
            ->Attribute(AZ::Script::Attributes::ToolTip, "Ghosting effect")
            ->Method("DisableGhosting", &PostEffects::DisableGhosting)
                    
            ->Method("EnableVolumetricScattering",
                &PostEffects::EnableVolumetricScattering,
                { {
                    { "Amount", "Sets volumetric scattering amount",           behaviorContext->MakeDefaultValue(1.0f) },
                    { "Tiling", "Sets volumetric scattering tiling",           behaviorContext->MakeDefaultValue(1.0f) },
                    { "Speed",  "Sets volumetric scattering animation speed",  behaviorContext->MakeDefaultValue(1.0f) },
                    { "Color",  "Sets volumetric scattering color tint",       behaviorContext->MakeDefaultValue(AZ::Color(0.6f, 0.75f, 1.0f, 1.0f)) }
                } } )
            ->Attribute(AZ::Script::Attributes::ToolTip, "Volumetric scattering effect")
            ->Method("DisableVolumetricScattering", &PostEffects::DisableVolumetricScattering)
                    
            ->Method("EnableRainDrops",
                &PostEffects::EnableRainDrops,
                { {
                    { "Amount",            "Amount of rain drops",                 behaviorContext->MakeDefaultValue(1.0f) },
                    { "SpawTimeDistance",  "Sets rain drops spawn time distance",  behaviorContext->MakeDefaultValue(0.35f) },
                    { "Size",              "Sets rain drops size",                 behaviorContext->MakeDefaultValue(5.0f) },
                    { "SizeVariation",     "Sets rain drops size variation",       behaviorContext->MakeDefaultValue(2.5f) }
                } } )
            ->Attribute(AZ::Script::Attributes::ToolTip, "Draws small drops of rain on the screen")
            ->Method("DisableRainDrops", &PostEffects::DisableRainDrops)
                    
            ->Method("EnableSharpen",
                &PostEffects::EnableSharpen,
                { {
                    { "Amount",  "Amount of sharpening",  behaviorContext->MakeDefaultValue(1.0f) }
                } } )
            ->Attribute(AZ::Script::Attributes::ToolTip, "Applies a sharpen filter")
            ->Method("DisableSharpen", &PostEffects::DisableSharpen)

            ->Method("EnableDirectionalBlur",
                &PostEffects::EnableDirectionalBlur,
                { {
                    { "Direction",  "Indicates the direction (in screen space) and strength of the blur",  behaviorContext->MakeDefaultValue(AZ::Vector2(0.0f, 0.0f)) }
                } } )
            ->Attribute(AZ::Script::Attributes::ToolTip, "Applies a directional blur filter")
            ->Method("DisableDirectionalBlur", &PostEffects::DisableDirectionalBlur)

            ->Method("EnableVisualArtifacts",
                &PostEffects::EnableVisualArtifacts,
                { {
                    { "VSync",               "Amount of vsync visible",        behaviorContext->MakeDefaultValue(0.0f) },
                    { "VSyncFrequency",      "Vsync frequency",                behaviorContext->MakeDefaultValue(1.0f) },
                    { "Interlacing",         "Amount of interlacing visible",  behaviorContext->MakeDefaultValue(0.0f) },
                    { "InterlacingTiling",   "Interlacing tiling",             behaviorContext->MakeDefaultValue(1.0f) },
                    { "InterlacingRotation", "Interlacing rotation",           behaviorContext->MakeDefaultValue(0.0f) },
                    { "SyncWavePhase",       "Sync wave phase",                behaviorContext->MakeDefaultValue(0.0f) },
                    { "SyncWaveFrequency",   "Sync wave frequency",            behaviorContext->MakeDefaultValue(0.0f) },
                    { "SyncWaveAmplitude",   "Amount of vsync visible",        behaviorContext->MakeDefaultValue(0.0f) },
                    { "ChromaShift",         "Chroma shift",                   behaviorContext->MakeDefaultValue(0.0f) },
                    { "Grain",               "Image grain amount",             behaviorContext->MakeDefaultValue(0.0f) },
                    { "ColorTint",           "Color tint",                     behaviorContext->MakeDefaultValue(AZ::Color(1.0f, 1.0f, 1.0f, 1.0f)) },
                    { "TextureName",         "Visual artifacts mask texture" }
                } } )
            ->Attribute(AZ::Script::Attributes::ToolTip, "Applies visual artifacts that are commonly assocaited with CRT monitors")
            ->Method("DisableVisualArtifacts", &PostEffects::DisableVisualArtifacts)

            ->Method("EnableEffectGroup",
                &PostEffects::EnableEffectGroup,
                { {
                    { "GroupName",  "Game path of the post effect group xml",  behaviorContext->MakeDefaultValue(AZStd::string_view()) }
                } })
            ->Attribute(AZ::Script::Attributes::ToolTip, "Enables a post effect group")

            ->Method("DisableEffectGroup",
                &PostEffects::DisableEffectGroup,
                { {
                    { "GroupName",  "Game path of the post effect group xml",  behaviorContext->MakeDefaultValue(AZStd::string_view()) }
                } })
            ->Attribute(AZ::Script::Attributes::ToolTip, "Disables a post effect group")

            ->Method("ApplyEffectGroupAtPosition",
                &PostEffects::ApplyEffectGroupAtPosition,
                { {
                    { "GroupName",  "Game path of the post effect group xml",         behaviorContext->MakeDefaultValue(AZStd::string_view()) },
                    { "Position",   "Used to calculate the distance between camera",  behaviorContext->MakeDefaultValue(AZ::Vector3()) },
                } })
            ->Attribute(AZ::Script::Attributes::ToolTip, 
                "Applies an effect group at a specific position in the world. Only applies to effect groups with the fadeDistance attribute set. Must be called every frame to maintain the effect.")
                    
            ->Method("SetColorChart",
                &PostEffects::SetColorChart,
                { {
                    { "TextureName",  "The name of a color chart texture",                 behaviorContext->MakeDefaultValue(AZStd::string_view()) },
                    { "FadeTime",     "Number of seconds to fade into the color grading",  behaviorContext->MakeDefaultValue(0.0f) },
                } })
            ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All) // Exclude from ScriptCanvas because ScriptCanvas already has ScreenFaderNode
            ->Attribute(AZ::Script::Attributes::ToolTip, "Applies a color chart texture for color grading")
        ;    
    }
}
