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

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace NvCloth
{
    class ClothComponentMesh;

    class ClothDebugDisplay
        : private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        AZ_TYPE_INFO(ClothDebugDisplay, "{306A2A30-8BB1-4D0F-9776-324CA1D90ABE}");

        ClothDebugDisplay(ClothComponentMesh* clothComponentMesh);
        ~ClothDebugDisplay();

        bool IsDebugDrawEnabled() const;

    private:
        // AzFramework::EntityDebugDisplayEventBus::Handler overrides
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        void DisplayParticles(AzFramework::DebugDisplayRequests& debugDisplay);
        void DisplayWireCloth(AzFramework::DebugDisplayRequests& debugDisplay);
        void DisplayNormals(AzFramework::DebugDisplayRequests& debugDisplay, bool showTangents);
        void DisplayColliders(AzFramework::DebugDisplayRequests& debugDisplay);
        void DrawSphereCollider(AzFramework::DebugDisplayRequests& debugDisplay, float radius, const AZ::Transform& transform);
        void DrawCapsuleCollider(AzFramework::DebugDisplayRequests& debugDisplay, float radius, float height, const AZ::Transform& transform);

        ClothComponentMesh* m_clothComponentMesh = nullptr;
    };
} // namespace NvCloth
