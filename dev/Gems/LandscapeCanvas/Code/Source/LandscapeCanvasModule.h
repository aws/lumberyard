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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

namespace LandscapeCanvas
{
    class LandscapeCanvasModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(LandscapeCanvasModule, "{5D830FCF-C244-43E4-92A0-D192470009CD}", AZ::Module);
        AZ_CLASS_ALLOCATOR(LandscapeCanvasModule, AZ::SystemAllocator, 0);

        LandscapeCanvasModule();

        /**
        * Add required SystemComponents to the SystemEntity.
        */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
