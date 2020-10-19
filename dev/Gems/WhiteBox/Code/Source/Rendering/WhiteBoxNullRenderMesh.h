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

#include "WhiteBoxRenderMeshInterface.h"

namespace WhiteBox
{
    class WhiteBoxNullRenderMesh : public RenderMeshInterface
    {
    public:
        // RenderMeshInterface ...
        void BuildMesh(const WhiteBoxRenderData& renderData, const AZ::Transform& worldFromLocal) override;
        void UpdateTransform(const AZ::Transform& worldFromLocal) override;
        void UpdateMaterial(const WhiteBoxMaterial& material) override;
        bool IsVisible() const override;
        void SetVisiblity(bool visibility) override;
    };
} // namespace WhiteBox
