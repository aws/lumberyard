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

// Forward Declares
struct ImDrawData;
struct ImVec2;

namespace ImGui
{
    class OtherActiveImGuiRequests : public AZ::EBusTraits
    {
    public:
        virtual ~OtherActiveImGuiRequests() = default;

        virtual void RenderImGuiBuffers(ImDrawData* drawData, const ImVec2& scaleRects) = 0;

        virtual AZ::u32 GetBackBufferWidth() = 0;
        virtual AZ::u32 GetBackBufferHeight() = 0;
    };

    //! ImGuiManager hands over drawing of ImGui content via this bus.
    using OtherActiveImGuiRequestBus = AZ::EBus<OtherActiveImGuiRequests>;

    class OtherActiveImGuiNotifications : public AZ::EBusTraits
    {
    public:
        virtual ~OtherActiveImGuiNotifications() = default;

        virtual void OnImGuiContextCreated() = 0;
    };

    using OtherActiveImGuiNotificationBus = AZ::EBus<OtherActiveImGuiNotifications>;
}
