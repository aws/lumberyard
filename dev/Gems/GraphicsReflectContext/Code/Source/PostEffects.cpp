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

#include <IPostEffectGroup.h>



namespace GraphicsReflectContext
{
    void PostEffects::SetWaterDropletsParams(bool enable, float amount)
    {
        I3DEngine* pEngine = gEnv->p3DEngine;
        if (enable)
        {
            pEngine->GetPostEffectBaseGroup()->SetParam("WaterDroplets_Amount", amount);
        }
        else
        {
            pEngine->GetPostEffectBaseGroup()->SetParam("WaterDroplets_Amount", 0.0f);
        }
    }

    void PostEffects::SetDepthOfFieldParams(bool Active, float focusDistance, float FocusRange, float blurAmount, float maxCoC, float centerWeight)
    {
        I3DEngine* pEngine = gEnv->p3DEngine;
        pEngine->GetPostEffectBaseGroup()->SetParam("Dof_User_Active", Active);
        pEngine->GetPostEffectBaseGroup()->SetParam("Dof_User_FocusDistance", focusDistance);
        pEngine->GetPostEffectBaseGroup()->SetParam("Dof_User_FocusRange", FocusRange);
        pEngine->GetPostEffectBaseGroup()->SetParam("Dof_User_BlurAmount", blurAmount);
        pEngine->GetPostEffectBaseGroup()->SetParam("Dof_MaxCoC", maxCoC);
        pEngine->GetPostEffectBaseGroup()->SetParam("Dof_CenterWeight", centerWeight);
    }


    void PostEffects::SetFilterBlurParams(bool enable, float amount)
    {
        I3DEngine* pEngine = gEnv->p3DEngine; 
        if (enable)
        {
            //Note: FilterBlurring_Type is not used in CFilterBlurring. So it's not exposed to user here.
            pEngine->GetPostEffectBaseGroup()->SetParam("FilterBlurring_Amount", amount);
        }
        else
        {
            pEngine->GetPostEffectBaseGroup()->SetParam("FilterBlurring_Amount", 0.0f);
        }
    }

    void PostEffects::SetColorCorrectionParams(bool enable, float colorC, float colorM, float colorY, float colorK, float brightness,
        float contrast, float saturation, float colorHue)
    {
        I3DEngine* pEngine = gEnv->p3DEngine;
        if (enable)
        {
            pEngine->GetPostEffectBaseGroup()->SetParam("Global_User_ColorC", colorC);
            pEngine->GetPostEffectBaseGroup()->SetParam("Global_User_ColorM", colorM);
            pEngine->GetPostEffectBaseGroup()->SetParam("Global_User_ColorY", colorY);
            pEngine->GetPostEffectBaseGroup()->SetParam("Global_User_ColorK", colorK);
            pEngine->GetPostEffectBaseGroup()->SetParam("Global_User_Brightness", brightness);
            pEngine->GetPostEffectBaseGroup()->SetParam("Global_User_Contrast", contrast);
            pEngine->GetPostEffectBaseGroup()->SetParam("Global_User_Saturation", saturation);
            pEngine->GetPostEffectBaseGroup()->SetParam("Global_User_ColorHue", colorHue);
        }
        else
        {
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
    }

    void PostEffects::ReflectBehaviorContext(AZ::BehaviorContext *behaviorContext)
    {
        //effect water droplets
        behaviorContext->Class<PostEffects>("PostEffects")
            ->Attribute(AZ::Script::Attributes::Category, "Rendering")
            ->Method("EffectWaterDroplets", 
                &PostEffects::SetWaterDropletsParams, 
                { {
                    { "Enable", "Enables water droplets effect",    aznew AZ::BehaviorDefaultValue(false) },
                    { "Amount", "Amount of water",                  aznew AZ::BehaviorDefaultValue(5.0f) }
                } } )
            ->Attribute(AZ::Script::Attributes::ToolTip, "Water droplets effect - Use when camera goes out of water or similar extreme condition")

            ->Method("EffectDepthOfField",
                &PostEffects::SetDepthOfFieldParams,
                //name and tooltips
                { { 
                    { "EnableDof",      "Enables depth of field (dof) effect", aznew AZ::BehaviorDefaultValue(false) },
                    { "FocusDistance",  "Set focus distance",                  aznew AZ::BehaviorDefaultValue(3.5f) },
                    { "FocusRange",     "Set focus range",                     aznew AZ::BehaviorDefaultValue(5.0f) },
                    { "BlurAmount",     "Set blurring amount",                 aznew AZ::BehaviorDefaultValue(1.0f) },
                    { "ScaleCoC",       "Set circle of confusion scale",       aznew AZ::BehaviorDefaultValue(12.0f) },
                    { "CenterWeight",   "Set central samples weight",          aznew AZ::BehaviorDefaultValue(1.0f) }
                } } )
            ->Attribute(AZ::Script::Attributes::ToolTip, "Depth of field effect")

            ->Method("EffectFilterBlur",
                &PostEffects::SetFilterBlurParams,
                //name and tooltips
                { {
                    { "Enable", "Enables filter blur effect",   aznew AZ::BehaviorDefaultValue(false) },
                    { "Amount", "Amount of blurring",           aznew AZ::BehaviorDefaultValue(0.0f) }
                } } )
            ->Attribute(AZ::Script::Attributes::ToolTip, "Sets blur (Gaussian blur) filter. ")

            ->Method("EffectColorCorrection",
                &PostEffects::SetColorCorrectionParams,
                //name and tooltips
                { {
                    { "Enable",     "Enables color correction",                                                 aznew AZ::BehaviorDefaultValue(false) },
                    { "Cyan",       "Adjust Cyan to enhance color of scene",                                    aznew AZ::BehaviorDefaultValue(0.0f) },
                    { "Magenta",    "Adjust Magenta to enhance color of scene",                                 aznew AZ::BehaviorDefaultValue(0.0f) },
                    { "Yellow",     "Adjust Yellow to enhance color of scene",                                  aznew AZ::BehaviorDefaultValue(0.0f) },
                    { "Luminance",  "Adjust Luminance to enhance the light intensity of scene",                 aznew AZ::BehaviorDefaultValue(0.0f) },
                    { "Brightness", "Adjust Brightness to enhance light and darkness of scene",                 aznew AZ::BehaviorDefaultValue(1.0f) },
                    { "Contrast",   "Adjust Contrast to enhance the bias of highlights and shadows of scene",   aznew AZ::BehaviorDefaultValue(1.0f) },
                    { "Saturation", "Adjust Saturation to enhance the color intensity of scene",                aznew AZ::BehaviorDefaultValue(1.0f) },
                    { "Hue", "Adjust Hue to enhance the color globally",                                        aznew AZ::BehaviorDefaultValue(0.0f) }
                } } )
            ->Attribute(AZ::Script::Attributes::ToolTip, "Sets color correction parameters")
        ;    
    }
}
