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
#include "BehaviorContextObjectPtr.h"

#include <AzCore/RTTI/ReflectContext.h>
#include "BehaviorContextObject.h"

namespace ScriptCanvas
{
    /// This class does nothing but allow BehaviorContextObjectPtr to be serialized within an any class.
    class BehaviorContextObjectPtrReflector
    {
    public:
        AZ_TYPE_INFO(BehaviorContextObjectPtr, "{93EC97EC-939C-44F6-ADBE-D82DC0AE432C}");

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<BehaviorContextObjectPtrReflector>()
                    ->Version(0)
                    ->Field("m_reflected", &BehaviorContextObjectPtrReflector::m_reflected);
            }
        }

        BehaviorContextObjectPtr m_reflected;
    };

    void BehaviorContextObjectPtrReflect(AZ::ReflectContext* context)
    {
        BehaviorContextObjectPtrReflector::Reflect(context);
    }
}