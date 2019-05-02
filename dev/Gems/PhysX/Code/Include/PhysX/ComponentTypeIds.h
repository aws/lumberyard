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

#include <AzCore/RTTI/TypeInfo.h>

namespace PhysX
{
    ///
    /// The type ID of runtime component PhysX::TerrainComponent.
    ///
    static const AZ::TypeId TerrainComponentTypeId("{51D9CFD0-842D-4263-880C-A38171FC2D2F}");

    ///
    /// The type ID of editor component PhysX::EditorTerrainComponent.
    ///
    static const AZ::TypeId EditorTerrainComponentTypeId("{84CC2375-927A-4DB9-8040-10768BDBCB44}");
}