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

#include <stdint.h>
#include <Cry_Vector2.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class IRecognizer
    {
    public:
        virtual ~IRecognizer() {}

        // Recognizers with lower priority (negative values allowed) are processed first.
        virtual int32_t GetPriority() const = 0;

        // Called when a mouse button or finger on a touch screen is initially pressed.
        // Return true to consume the underlying input event, preventing it from being
        // processed by other lower-priority gesture recognizers or input listeners.
        virtual bool OnPressedEvent(const Vec2& screenPositionPixels, uint32_t pointerIndex) = 0;

        // Called each frame a mouse button or finger on a touch screen remains down/pressed.
        // Return true to consume the underlying input event, preventing it from being
        // processed by other lower-priority gesture recognizers or input listeners.
        virtual bool OnDownEvent(const Vec2& screenPositionPixels, uint32_t pointerIndex) = 0;

        // Called when a mouse button or finger on a touch screen is released.
        // Return true to consume the underlying input event, preventing it from being
        // processed by other lower-priority gesture recognizers or input listeners.
        virtual bool OnReleasedEvent(const Vec2& screenPositionPixels, uint32_t pointerIndex) = 0;

    protected:
        // Using Vec2 directly as a member of derived recognizer classes results in
        // linker warnings because Vec2 doesn't have dll import/export specifiers.
        struct ScreenPosition
        {
            ScreenPosition()
                : x(0.0f)
                , y(0.0f) {}
            ScreenPosition(const Vec2& vec2)
                : x(vec2.x)
                , y(vec2.y) {}
            operator Vec2() const
            {
                return Vec2(x, y);
            }

            float x;
            float y;
        };
    };
}
