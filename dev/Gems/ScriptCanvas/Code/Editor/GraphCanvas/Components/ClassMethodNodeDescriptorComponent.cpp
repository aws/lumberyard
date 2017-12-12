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
#include "precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>

#include "ClassMethodNodeDescriptorComponent.h"

namespace ScriptCanvasEditor
{
    ///////////////////////////////////////
    // ClassMethodNodeDescriptorComponent
    ///////////////////////////////////////
    
    void ClassMethodNodeDescriptorComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);
        
        if (serializeContext)
        {
            serializeContext->Class<ClassMethodNodeDescriptorComponent, NodeDescriptorComponent>()
                ->Version(1)
                ->Field("ClassName", &ClassMethodNodeDescriptorComponent::m_className)
                ->Field("MethodName", &ClassMethodNodeDescriptorComponent::m_methodName)
            ;
        }
    }
    
    ClassMethodNodeDescriptorComponent::ClassMethodNodeDescriptorComponent(const AZStd::string& className, const AZStd::string& methodName)
        : NodeDescriptorComponent(NodeDescriptorType::ClassMethod)
        , m_className(className)
        , m_methodName(methodName)
    {		
    }
}