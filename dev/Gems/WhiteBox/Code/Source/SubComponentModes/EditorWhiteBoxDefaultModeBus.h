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

#include <AzCore/Component/ComponentBus.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
    //! Request bus for White Box ComponentMode operations while in 'default' mode.
    class EditorWhiteBoxDefaultModeRequests : public AZ::EntityComponentBus
    {
    public:
        //! Create a polygon scale modifier for the given polygon handles.
        virtual void CreatePolygonScaleModifier(const Api::PolygonHandle& polygonHandle) = 0;
        //! Create an edge scale modifier for the given edge handle.
        virtual void CreateEdgeScaleModifier(Api::EdgeHandle edgeHandle) = 0;
        //! Assign whatever polygon is currently hovered to the selected polygon translation modifier.
        virtual void AssignSelectedPolygonTranslationModifier() = 0;
        //! Assign whatever edge is currently hovered to the selected edge translation modifier.
        virtual void AssignSelectedEdgeTranslationModifier() = 0;
        //! Assign whatever vertex is currently hovered to the vertex selection modifier.
        virtual void AssignSelectedVertexSelectionModifier() = 0;
        //! Refresh (rebuild) the polygon scale modifier.
        //! The internal manipulator will be rebuilt from the current white box handles stored in the modifier.
        virtual void RefreshPolygonScaleModifier() = 0;
        //! Refresh (rebuild) the edge scale modifier.
        //! The internal manipulator will be rebuilt from the current white box handles stored in the modifier.
        virtual void RefreshEdgeScaleModifier() = 0;
        //! Refresh (rebuild) the polygon translation modifier.
        //! The internal manipulator will be rebuilt from the current white box handles stored in the modifier.
        virtual void RefreshPolygonTranslationModifier() = 0;
        //! Refresh (rebuild) the edge translation modifier.
        //! The internal manipulator will be rebuilt from the current white box handles stored in the modifier.
        virtual void RefreshEdgeTranslationModifier() = 0;
        //! Refresh (rebuild) the vertex selection modifier.
        //! The internal manipulator will be rebuilt from the current white box handles stored in the modifier.
        virtual void RefreshVertexSelectionModifier() = 0;

    protected:
        ~EditorWhiteBoxDefaultModeRequests() = default;
    };

    using EditorWhiteBoxDefaultModeRequestBus = AZ::EBus<EditorWhiteBoxDefaultModeRequests>;
} // namespace WhiteBox
