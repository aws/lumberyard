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

#include <IMaterial.h>
#include <AzCore/RTTI/TypeInfo.h>

namespace AZ
{
    class BehaviorContext;
}

namespace LmbrCentral
{
    //! Wraps a IMaterial pointer in a way that BehaviorContext can use it
    class MaterialHandle
    {
    public:
        AZ_TYPE_INFO(MaterialHandle, "{BF659DC6-ACDD-4062-A52E-4EC053286F4F}")
        _smart_ptr<IMaterial> m_material;

        static void Reflect(AZ::BehaviorContext* behaviorContext);
    };

}
