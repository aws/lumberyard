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

#if ENABLE_CRY_PHYSICS

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/Component/TransformBus.h>
#include <LmbrCentral/Physics/WindVolumeRequestBus.h>
#include "WindVolume.h"

struct IPhysicalEntity;

namespace LmbrCentral
{
    /**
     * WindVolume component for a wind volume
     */
    class WindVolumeComponent
        : public AZ::Component
        , public WindVolume
    {
    public:
        AZ_COMPONENT(WindVolumeComponent, "{7E97DA82-94EB-46B7-B5EB-8B72727E3D7E}");
        static void Reflect(AZ::ReflectContext* context);

        WindVolumeComponent() = default;
        explicit WindVolumeComponent(const WindVolumeConfiguration& configuration);
        ~WindVolumeComponent() override = default;

        // WindVolume
        WindVolumeConfiguration& GetConfiguration() override { return m_configuration; }

    protected:

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        WindVolumeConfiguration m_configuration;    ///< Configuration of the wind volume
    };
} // namespace LmbrCentral

#endif // ENABLE_CRY_PHYSICS
