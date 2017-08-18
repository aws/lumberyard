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
#include "StdAfx.h"

#include "AndroidTouch.h"

#include <SDL.h>
#include <SDL_touch.h>

#define TOUCH_MAX_PEEP      32
#define TAP_MAX_PEEP        32

////////////////////////////////////////////////////////////////////////////////////////////////////
CAndroidTouch::CAndroidTouch(IInput& input)
    : CMobileTouch(input, "Android Touch")
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CAndroidTouch::~CAndroidTouch()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// this update only works under the assumption that SDL_PumpEvents is called from CAndroidInput
////////////////////////////////////////////////////////////////////////////////////////////////////
void CAndroidTouch::Update(bool focus)
{
    // Get all the standard sdl touch events that have been sent since the last call to this Update.
    SDL_Event sdlTouchEventList[TOUCH_MAX_PEEP];
    int sdlTouchEventCount = SDL_PeepEvents(sdlTouchEventList, TOUCH_MAX_PEEP, SDL_GETEVENT, SDL_FINGERDOWN, SDL_FINGERMOTION);
    if (sdlTouchEventCount == -1)
    {
        gEnv->pLog->LogError("SDL_GETEVENT error: %s", SDL_GetError());
        return;
    }

    // For each sdl touch event, send a corresponding generic touch event.
    for (int i = 0; i < sdlTouchEventCount; ++i)
    {
        SDL_TouchFingerEvent& sdlTouchEvent = sdlTouchEventList[i].tfinger;
        TouchEvent touchEvent(sdlTouchEvent.x,
                              sdlTouchEvent.y,
                              sdlTouchEvent.pressure,
                              sdlTouchEvent.fingerId,
                              TouchEvent::State::Began);

        // This is a little strange to be sending EBus events that the parent class has
        // subscribed to, and we could simply call the functions directly, but doing this
        // provides other systems with the opportunity to hook into the events if needed.
        switch (sdlTouchEvent.type)
        {
            case SDL_FINGERDOWN:
            {
                touchEvent.state = TouchEvent::State::Began;
                OnTouchBegan(touchEvent);
            }
            break;
            case SDL_FINGERMOTION:
            {
                touchEvent.state = TouchEvent::State::Moved;
                OnTouchMoved(touchEvent);
            }
            break;
            case SDL_FINGERUP:
            {
                touchEvent.state = TouchEvent::State::Ended;
                OnTouchEnded(touchEvent);
            }
            break;
        }
    }

    // Call the parent update to process the generic touch events.
    CMobileTouch::Update(focus);
}
