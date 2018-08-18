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

#include "ParameterFactory.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>

#include "BoolParameter.h"
#include "ColorParameter.h"
#include "FloatParameter.h"
#include "FloatSliderParameter.h"
#include "FloatSpinnerParameter.h"
#include "GroupParameter.h"
#include "IntParameter.h"
#include "IntSliderParameter.h"
#include "IntSpinnerParameter.h"
#include "Parameter.h"
#include "RotationParameter.h"
#include "StringParameter.h"
#include "TagParameter.h"
#include "ValueParameter.h"
#include "Vector2Parameter.h"
#include "Vector3GizmoParameter.h"
#include "Vector3Parameter.h"
#include "Vector4Parameter.h"

namespace EMotionFX
{
    void ParameterFactory::ReflectParameterTypes(AZ::ReflectContext* context)
    {
        Parameter::Reflect(context);
        GroupParameter::Reflect(context);
        ValueParameter::Reflect(context);
        BoolParameter::Reflect(context);
        ColorParameter::Reflect(context);
        FloatParameter::Reflect(context);
        FloatSliderParameter::Reflect(context);
        FloatSpinnerParameter::Reflect(context);
        IntParameter::Reflect(context);
        IntSliderParameter::Reflect(context);
        IntSpinnerParameter::Reflect(context);
        RotationParameter::Reflect(context);
        StringParameter::Reflect(context);
        TagParameter::Reflect(context);
        Vector2Parameter::Reflect(context);
        Vector3Parameter::Reflect(context);
        Vector3GizmoParameter::Reflect(context);
        Vector4Parameter::Reflect(context);
    }

    AZStd::vector<AZ::TypeId> ParameterFactory::GetParameterTypes()
    {
        return {
                   azrtti_typeid<FloatSliderParameter>(),
                   azrtti_typeid<FloatSpinnerParameter>(),
                   azrtti_typeid<BoolParameter>(),
                   azrtti_typeid<TagParameter>(),
                   azrtti_typeid<IntSliderParameter>(),
                   azrtti_typeid<IntSpinnerParameter>(),
                   azrtti_typeid<Vector2Parameter>(),
                   azrtti_typeid<Vector3Parameter>(),
                   azrtti_typeid<Vector3GizmoParameter>(),
                   azrtti_typeid<Vector4Parameter>(),
                   azrtti_typeid<StringParameter>(),
                   azrtti_typeid<ColorParameter>(),
                   azrtti_typeid<RotationParameter>(),
                   azrtti_typeid<GroupParameter>()
        };
    }

    Parameter* ParameterFactory::Create(const AZ::TypeId& type)
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return nullptr;
        }

        const AZ::SerializeContext::ClassData* classData = context->FindClassData(type);
        void* parameterInstance = classData->m_factory->Create(classData->m_name);
        return reinterpret_cast<Parameter*>(parameterInstance);
    }
}
