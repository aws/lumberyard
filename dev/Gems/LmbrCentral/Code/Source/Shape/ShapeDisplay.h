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

#include <AzCore/Math/Color.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Viewport/ViewportColors.h>

namespace LmbrCentral
{
    /**
     * Common properties of how shape debug drawing can be rendererd.
     */
    struct ShapeDrawParams
    {
        AZ::Color m_shapeColor; ///< Color of underlying shape.
        AZ::Color m_wireColor; ///< Color of wireframe edges of shapes.
        bool m_filled; ///< Whether the shape should be rendered filled, or wireframe only.
    };

    const ShapeDrawParams g_defaultShapeDrawParams = { 
        AzFramework::ViewportColors::DeselectedColor, 
        AzFramework::ViewportColors::WireColor, 
        true 
    };

    /**
     * @brief Helper function to be used when drawing debug shapes - called from DisplayEntity on 
     * the EntityDebugDisplayEventBus.
     * @param handled Did we display anything.
     * @param canDraw Functor to decide should the shape be drawn or not.
     * @param drawShape Functor to draw a specific shape (box/capsule/sphere etc).
     * @param worldFromLocal Transform of object in world space, push to matrix stack and render shape in local space.
     */
    template<typename CanDraw, typename DrawShape>
    void DisplayShape(bool& handled, CanDraw&& canDraw, DrawShape&& drawShape, const AZ::Transform& worldFromLocal)
    {
        if (!canDraw())
        {
            return;
        }

        handled = true;

        AzFramework::EntityDebugDisplayRequests* displayContext = AzFramework::EntityDebugDisplayRequestBus::FindFirstHandler();
        AZ_Assert(displayContext, "Invalid display context.");

        // only uniform scale is supported in physics so the debug visuals reflect this fact
        AZ::Transform worldFromLocalWithUniformScale = worldFromLocal;
        const AZ::Vector3 scale = worldFromLocalWithUniformScale.ExtractScale();
        worldFromLocalWithUniformScale.MultiplyByScale(AZ::Vector3(scale.GetMaxElement()));
        
        displayContext->PushMatrix(worldFromLocalWithUniformScale);
        drawShape(displayContext);
        displayContext->PopMatrix();
    }
}