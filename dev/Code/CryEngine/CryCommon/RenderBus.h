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

#include <AzCore/EBus/EBus.h>

namespace AZ
{
    /**
    * This bus serves as a way for non-rendering systems to react to events
    * that occur inside the renderer. For now these events will probably be implemented by
    * things like CSystem and CryAction. In the future the idea is that these can be implemented
    * by a user's GameComponent.
    */
    class RenderNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~RenderNotifications() = default;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        //////////////////////////////////////////////////////////////////////////

        /**
        * This event gets posted at the end of CD3D9Renderer's EF_Scene3D method.
        * CSystem (in SystemRenderer.cpp) uses this to render the console, aux geom and UI
        * in a manner that will make sure the render calls end up as part of the scene's render.
        * This is important or else those render calls won't show up properly in VR.
        */
        virtual void OnScene3DEnd() {};

        /**
        * This event gets posted at the beginning of CD3D9Renderer's FreeResources method, before the resources have been freed.
        */
        virtual void OnRendererFreeResources() {};
    };

    using RenderNotificationsBus = AZ::EBus<RenderNotifications>;
}