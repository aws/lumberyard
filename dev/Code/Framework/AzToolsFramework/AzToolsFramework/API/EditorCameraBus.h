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

namespace Camera
{
    class EditorCameraRequests : public AZ::EBusTraits
    {
    public:
        using Bus = AZ::EBus<EditorCameraRequests>;
        virtual ~EditorCameraRequests() = default;

        /*!
        * Sets the view from the entity's perspective
        */
        virtual void SetViewFromEntityPerspective(const AZ::EntityId& /*entityId*/) {}
    };

    using EditorCameraRequestBus = AZ::EBus<EditorCameraRequests>;

    class EditorCameraSystemRequests : public AZ::EBusTraits
    {
    public:
        using Bus = AZ::EBus<EditorCameraSystemRequests>;
        virtual ~EditorCameraSystemRequests() = default;

        virtual void CreateCameraEntityFromViewport() {}
    };
    using EditorCameraSystemRequestBus = AZ::EBus<EditorCameraSystemRequests>;

} // namespace Camera