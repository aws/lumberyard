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

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/string/string.h>


namespace AZ
{
    class ReflectContext;
}

namespace StarterGameGem
{
    /*!
    * Wrapper for U.I utility functions exposed to Lua for StarterGame.
    */
    class StarterGameUIUtility
    {
    public:
        AZ_TYPE_INFO(StarterGameUIUtility, "{08156E44-0F12-4A78-984E-97BB7F36FD8A}");
        AZ_CLASS_ALLOCATOR(StarterGameUIUtility, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflection);

        static void UIFaderControl(const AZ::EntityId& canvasEntityID, const int& faderID, const float& fadeValue, const float& fadeTime);
        static void UIScrollControl(const AZ::EntityId& canvasEntityID, const int& scrollID, const float& value);
        static void UISliderControl(const AZ::EntityId& canvasEntityID, const int& sliderID, const float& value);
        static void UIElementMover(const AZ::EntityId& canvasEntityID, const int& elementID, const AZ::Vector2& value);
        static void UIElementScaler(const AZ::EntityId& canvasEntityID, const int& elementID, const AZ::Vector2& value);
        static void UIElementRotator(const AZ::EntityId& canvasEntityID, const int& elementID, const float& value);
        static void UITextSetter(const AZ::EntityId& canvasEntityID, const int& textID, const AZStd::string& value);
        static bool GetElementPosOnScreen(const AZ::EntityId& entityId, AZ::Vector2& value);
        // this is almost replication of the UiCanvasInterface version, but uses IDs and callable from script
        static AZ::EntityId CloneElement(const AZ::EntityId& sourceEntity, const AZ::EntityId& parentEntity);
        static void DeleteElement(const AZ::EntityId& entityID);

        enum class StarterGameUIButtonType : AZ::u32
        {
            StarterGameUIButtonType_Invalid,

            StarterGameUIButtonType_Blank,
            StarterGameUIButtonType_Text,

            StarterGameUIButtonType_A = 101,
            StarterGameUIButtonType_B,
            StarterGameUIButtonType_X,
            StarterGameUIButtonType_Y,
            StarterGameUIButtonType_DUp,
            StarterGameUIButtonType_DDown,
            StarterGameUIButtonType_DLeft,
            StarterGameUIButtonType_DRight,

            StarterGameUIButtonType_Cross = 201,
            StarterGameUIButtonType_Circle,
            StarterGameUIButtonType_Square,
            StarterGameUIButtonType_Triangle,
        };

        enum class StarterGameUIActionType : AZ::u32
        {
            StarterGameUIActionType_Invalid,

            StarterGameUIActionType_Start,
            StarterGameUIActionType_NavLeft,
            StarterGameUIActionType_NavRight,
            StarterGameUIActionType_NavUp,
            StarterGameUIActionType_NavDown,
            StarterGameUIActionType_Confirm,
            StarterGameUIActionType_CancelBack,
            StarterGameUIActionType_TabLeft,
            StarterGameUIActionType_TabRight,
            StarterGameUIActionType_Pause,
            StarterGameUIActionType_SlideLeft,
            StarterGameUIActionType_SlideRight,
            StarterGameUIActionType_Use,
            StarterGameUIActionType_TimeOfDayNext,
            StarterGameUIActionType_TimeOfDayPrev,
            StarterGameUIActionType_AAModeNext,
            StarterGameUIActionType_AAModePrev,
        };
    };


    struct TrackerParams
    {
        AZ_TYPE_INFO(TrackerParams, "{82553A0A-9BA6-46CE-A038-1904DA2ADF09}");
        AZ_CLASS_ALLOCATOR(TrackerParams, AZ::SystemAllocator, 0);

        TrackerParams()
            : m_trackedItem()
            , m_graphic("")
            , m_visible(true)
        {}

        AZ::EntityId m_trackedItem;
        AZStd::string m_graphic;
        bool m_visible;
    };
} // namespace StarterGameGem
