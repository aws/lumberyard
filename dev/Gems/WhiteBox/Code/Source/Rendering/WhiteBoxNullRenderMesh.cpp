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

#include "WhiteBox_precompiled.h"

#include "WhiteBoxNullRenderMesh.h"

namespace WhiteBox
{
    void WhiteBoxNullRenderMesh::BuildMesh(const WhiteBoxRenderData& renderData, const AZ::Transform& worldFromLocal)
    {
        // noop
    }

    void WhiteBoxNullRenderMesh::UpdateTransform(const AZ::Transform& worldFromLocal)
    {
        // noop
    }

    void WhiteBoxNullRenderMesh::UpdateMaterial(const WhiteBoxMaterial& material)
    {
        // noop
    }

    bool WhiteBoxNullRenderMesh::IsVisible() const
    {
        return false;
    }

    void WhiteBoxNullRenderMesh::SetVisiblity(bool visibility)
    {
        // noop
    }
} // namespace WhiteBox
