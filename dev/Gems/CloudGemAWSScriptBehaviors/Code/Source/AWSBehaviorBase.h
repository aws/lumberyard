
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

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <CloudGemFramework/AwsApiRequestJob.h>

// this just provides a convenient template to avoid the necessary boilerplate when you derive from AWSBehaviorBase
#define AWSBEHAVIOR_DEFINITION(className, guidString) \
    AZ_TYPE_INFO(className, guidString) \
    AZ_CLASS_ALLOCATOR(className, AZ::SystemAllocator, 0) \
    void ReflectSerialization(AZ::SerializeContext* serializeContext) override; \
    void ReflectBehaviors(AZ::BehaviorContext* behaviorContext) override; \
    void ReflectEditParameters(AZ::EditContext* editContext) override; \
    className(); \

namespace CloudGemAWSScriptBehaviors
{
    class AWSBehaviorBase
    {
    public:
        virtual ~AWSBehaviorBase() {}

        virtual void ReflectSerialization(AZ::SerializeContext* reflectContext) = 0;
        virtual void ReflectBehaviors(AZ::BehaviorContext* behaviorContext) = 0;
        virtual void ReflectEditParameters(AZ::EditContext* editContext) = 0;

        virtual void Init() {}
        virtual void Activate() {}
        virtual void Deactivate() {}
    };
}
