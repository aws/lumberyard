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

namespace AZ
{
    class Transform;
}

namespace WhiteBox
{
    struct WhiteBoxMaterial;
    struct WhiteBoxRenderData;

    class RenderMeshInterface
    {
    public:
        virtual ~RenderMeshInterface() = 0;

        //! Take White Box render data and populate the render mesh from it.
        virtual void BuildMesh(const WhiteBoxRenderData& renderData, const AZ::Transform& worldFromLocal) = 0;

        //! Update the transform of the render mesh.
        virtual void UpdateTransform(const AZ::Transform& worldFromLocal) = 0;

        //! Update the material of the render mesh.
        virtual void UpdateMaterial(const WhiteBoxMaterial& material) = 0;

        //! Set the White Box mesh visible (true) or invisible (false).
        virtual void SetVisiblity(bool visibility) = 0;
    };
} // namespace WhiteBox
