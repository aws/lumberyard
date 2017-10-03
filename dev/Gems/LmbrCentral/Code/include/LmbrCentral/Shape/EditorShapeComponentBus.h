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

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Component/ComponentBus.h>

namespace AZ
{
    class BehaviorContext;
}

namespace LmbrCentral
{
    /*!
    * Services provided by the Editor Shape Component
    */
    class EditorShapeComponentRequests
        : public AZ::ComponentBus
    {
    public:

        /**
        * \brief Sets the shape color
        * \param solidColor the color to be used for drawing solid shapes
        */
        virtual void SetShapeColor(const AZ::Vector4& solidColor) = 0;

        /**
        * \brief Sets the wireframe shape color
        * \param wireColor the color to be used for drawing shapes in wireframe
        */
        virtual void SetShapeWireframeColor(const AZ::Vector4& wireColor)  = 0;
    };

    // Bus to service the Shape component requests event group
    using EditorShapeComponentRequestsBus = AZ::EBus<EditorShapeComponentRequests>;
} // namespace LmbrCentral