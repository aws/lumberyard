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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description: System "Hardware mouse" cursor with reference counter.
//              This is needed because Menus / HUD / Profiler / or whatever
//              can use the cursor not at the same time be successively
//              => We need to know when to enable/disable the cursor.

#pragma once

#include <AzCore/PlatformDef.h>

#include <LyShine/Bus/UiCursorBus.h>

//-----------------------------------------------------------------------------------------------------
/// @cond EXCLUDE_DOCS
/// @deprecated Use UiCursorBus instead.
//struct AZ_DEPRECATED(IHardwareMouse, "IHardwareMouse has been deprecated, use UiCursorBus instead.")
struct IHardwareMouse
{
    virtual ~IHardwareMouse() {}
    virtual void Release() { delete this; }

    /// @deprecated Use UiCursorBus::IncrementVisibleCounter instead.
    AZ_DEPRECATED(virtual void IncrementCounter(), "IHardwareMouse has been deprecated, use UiCursorBus instead.")
    {
        UiCursorBus::Broadcast(&UiCursorInterface::IncrementVisibleCounter);
    }

    /// @deprecated Use UiCursorBus::DecrementVisibleCounter instead.
    AZ_DEPRECATED(virtual void DecrementCounter(), "IHardwareMouse has been deprecated, use UiCursorBus instead.")
    {
        UiCursorBus::Broadcast(&UiCursorInterface::DecrementVisibleCounter);
    }

    /// @deprecated Use UiCursorBus::IsUiCursorVisible instead.
    AZ_DEPRECATED(virtual bool IsHidden(), "IHardwareMouse has been deprecated, use UiCursorBus instead.")
    {
        bool isCursorVisible = false;
        UiCursorBus::BroadcastResult(isCursorVisible, &UiCursorInterface::IsUiCursorVisible);
        return isCursorVisible;
    }

    /// @deprecated Use UiCursorBus::SetUiCursor instead.
    AZ_DEPRECATED(virtual bool SetCursor(const char* cursorPath), "IHardwareMouse has been deprecated, use UiCursorBus instead.")
    {
        UiCursorBus::Broadcast(&UiCursorInterface::SetUiCursor, cursorPath);
        return true;
    }
};
/// @endcond
