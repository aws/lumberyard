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
                ;
        }
    }
}

