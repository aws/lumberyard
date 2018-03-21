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

#include "Environment.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <IRenderer.h>
#include <ISystem.h>
#include <I3DEngine.h>
#include <ITimeOfDay.h>
#include <MathConversion.h>

namespace GraphicsReflectContext
{
    void Environment::SetSkyboxMaterial(LmbrCentral::MaterialHandle material)
    {
        if (material.m_material)
        {
            gEnv->p3DEngine->SetSkyMaterial(material.m_material);
        }
        else
        {
            AZ_Error(nullptr, false, "SetSkyboxMaterial failed: Material is null");
        }
    }

    void Environment::SetSkyboxAngle(float angle)
    {
        gEnv->p3DEngine->SetGlobalParameter(E3DPARAM_SKY_SKYBOX_ANGLE, angle);
    }

    void Environment::SetSkyboxStretch(float amount)
    {
        gEnv->p3DEngine->SetGlobalParameter(E3DPARAM_SKY_SKYBOX_STRETCHING, amount);
    }

    float Environment::GetMoonLatitude()
    {
        Vec3 vec(0);
        gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SKY_MOONROTATION, vec);
        return vec.x;
    }
    
    void Environment::SetMoonLatitude(float latitude, bool forceUpdate)
    {
        Vec3 vec(0);
        gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SKY_MOONROTATION, vec);
        vec.x = latitude;
        gEnv->p3DEngine->SetGlobalParameter(E3DPARAM_SKY_MOONROTATION, vec);
        ITimeOfDay* tod = gEnv->p3DEngine->GetTimeOfDay();
        if (forceUpdate && tod)
        {
            tod->Update(true, true);
        }
    }

    float Environment::GetMoonLongitude()
    {
        Vec3 vec(0);
        gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SKY_MOONROTATION, vec);
        return vec.y;
    }
    
    void Environment::SetMoonLongitude(float longitude, bool forceUpdate)
    {
        Vec3 vec(0);
        gEnv->p3DEngine->GetGlobalParameter(E3DPARAM_SKY_MOONROTATION, vec);
        vec.y = longitude;
        gEnv->p3DEngine->SetGlobalParameter(E3DPARAM_SKY_MOONROTATION, vec);
        ITimeOfDay* tod = gEnv->p3DEngine->GetTimeOfDay();
        if (forceUpdate && tod)
        {
            tod->Update(true, true);
        }
    }

    float Environment::GetSunLatitude()
    {
        ITimeOfDay* tod = gEnv->p3DEngine->GetTimeOfDay();
        if (tod)
        {
            return tod->GetSunLatitude();
        }
        return 0;
    }
    
    void Environment::SetSunLatitude(float latitude, bool forceUpdate)
    {
        ITimeOfDay* tod = gEnv->p3DEngine->GetTimeOfDay();
        if (tod)
        {
            float longitude = tod->GetSunLongitude();
            tod->SetSunPos(longitude, latitude);
            tod->Update(true, forceUpdate);
        }
    }

    float Environment::GetSunLongitude()
    {
        ITimeOfDay* tod = gEnv->p3DEngine->GetTimeOfDay();
        if (tod)
        {
            return tod->GetSunLongitude();
        }
        return 0;
    }
    
    void Environment::SetSunLongitude(float longitude, bool forceUpdate)
    {
        ITimeOfDay* tod = gEnv->p3DEngine->GetTimeOfDay();
        if (tod)
        {
            float latitude = tod->GetSunLatitude();
            tod->SetSunPos(longitude, latitude);
            tod->Update(true, forceUpdate);
        }
    }

    AZ::Vector3 Environment::GetWindDirection()
    {
        const Vec3 windDir = gEnv->p3DEngine->GetGlobalWind(false);
        return LYVec3ToAZVec3(windDir);
    }

    void Environment::SetWindDirection(const AZ::Vector3& windDir)
    {
        const Vec3 windDirVec = AZVec3ToLYVec3(windDir);
        gEnv->p3DEngine->SetWind(windDirVec);
    }

    void Environment::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<Environment>()
                ->Attribute(AZ::Script::Attributes::Category, "Rendering")
                ->Method("SetSkyboxMaterial", &SetSkyboxMaterial, { { { "Material", "" } } })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Set the sky to use a skybox with the given material")
                ->Method("SetSkyboxAngle", &SetSkyboxAngle, { { { "Angle", "Rotation degrees" } } })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Set the rotation angle of the skybox, around world Z")
                ->Method("SetSkyboxStretch", &SetSkyboxStretch, { { { "Amount", "Vertical stretch/scale factor" } } })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Sets a vertical stretch factor for the skybox")
                ->Method("SetMoonLatitude",
                    &SetMoonLatitude,
                    { {
                        { "Latitude",       "Moon's Latitude to be set ",                                    behaviorContext->MakeDefaultValue(0.0f) },
                        { "ForceUpdate",    "Indicates whether the whole sky should be updated immediately", behaviorContext->MakeDefaultValue(false) }
                        } })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Sets Moon's Latitude in the sky")
                ->Method("GetMoonLatitude", &GetMoonLatitude, { {} })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Gets Moon's Latitude in the sky")
                ->Method("SetMoonLongitude",
                    &SetMoonLongitude,
                    { {
                        { "Longitude",      "Moon's Longitude to be set ",                                   behaviorContext->MakeDefaultValue(0.0f) },
                        { "ForceUpdate",    "Indicates whether the whole sky should be updated immediately", behaviorContext->MakeDefaultValue(false) }
                        } })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Sets Moon's Longitude in the sky")
                ->Method("GetMoonLongitude", &GetMoonLongitude, { {} })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Gets Moon's Longitude in the sky")
                ->Method("SetSunLatitude",
                    &SetSunLatitude,
                    { {
                        { "Latitude",       "Sun's Latitude to be set ",                                     behaviorContext->MakeDefaultValue(0.0f) },
                        { "ForceUpdate",    "Indicates whether the whole sky should be updated immediately", behaviorContext->MakeDefaultValue(false) }
                        } })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Sets Sun's Latitude in the sky")
                ->Method("GetSunLatitude", &GetSunLatitude, { {} })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Gets Sun's Latitude in the sky")
                ->Method("SetSunLongitude",
                    &SetSunLongitude,
                    { {
                        { "Longitude",      "Sun's Longitude to be set ",                                    behaviorContext->MakeDefaultValue(0.0f) },
                        { "ForceUpdate",    "Indicates whether the whole sky should be updated immediately", behaviorContext->MakeDefaultValue(false) }
                        } })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Sets Sun's Longitude in the sky")
                ->Method("GetSunLongitude", &GetSunLongitude, { {} })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Gets Sun's Longitude in the sky")
                ->Method("SetWindDirection",
                    &SetWindDirection,
                     { { 
                        { "Wind Direction", "Global wind direction to be set",                            behaviorContext->MakeDefaultValue(AZ::Vector3::CreateZero())}
                        } })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Sets global wind's direction")
                ->Method("GetWindDirection", &GetWindDirection, { {} })
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Gets global wind's direction")
                ;
        }
    }
}

