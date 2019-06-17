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

#include <IGem.h>
#include <SurfaceData/SurfaceDataTagProviderRequestBus.h>

namespace RoadsAndRivers
{
    class RoadsAndRiversConverter;

    class RoadsAndRiversModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(RoadsAndRiversModule, "{82112478-1108-41FE-B37D-65ADC6BCB45A}", CryHooksModule);
        AZ_CLASS_ALLOCATOR(RoadsAndRiversModule, AZ::SystemAllocator, 0);

        RoadsAndRiversModule();
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;

    private:
#if defined(ROADS_RIVERS_EDITOR)
        AZStd::unique_ptr<RoadsAndRiversConverter> m_legacyConverter;
#endif
    };

    class RoadsAndRiversSystemComponent
        : public AZ::Component
        , private SurfaceData::SurfaceDataTagProviderRequestBus::Handler
    {
    public:
        AZ_COMPONENT(RoadsAndRiversSystemComponent, "{3561D3D4-2104-45E9-BF5A-4ED640907069}");

        static void Reflect(AZ::ReflectContext* context);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        ////////////////////////////////////////////////////////////////////////
        // SurfaceDataTagProviderRequestBus
        void GetRegisteredSurfaceTagNames(SurfaceData::SurfaceTagNameSet& names) const override;
    };
}