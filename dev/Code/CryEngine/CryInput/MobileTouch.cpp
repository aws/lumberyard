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

#include "MobileTouch.h"

#include <IHardwareMouse.h>

#include <LyShine/IDraw2d.h>

namespace
{
    static int i_DebugShowTouches = 0;
    static int i_SimulateMouseEventsWithPrimaryTouch = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Creates a debug circle texture programatically
    ////////////////////////////////////////////////////////////////////////////////////////////////
    ITexture* GenerateDebugCircleTexture(const int texSize)
    {
        const int sizeHalf = texSize / 2;
        const float normalizer = 1.0f / static_cast<float>(sizeHalf * sizeHalf);

        uint32 data[texSize * texSize];

        for (int y = 0; y < texSize; ++y)
        {
            int posY = y * texSize;
            int diffY = (sizeHalf - y) * (sizeHalf - y);

            for (int x = 0; x < texSize; ++x)
            {
                int index = posY + x;

                int diffX = (sizeHalf - x) * (sizeHalf - x);
                float normal = static_cast<float>(diffX + diffY) * normalizer;

                if (normal > 1.0f)
                {
                    data[index] = 0x00000000; // blank
                }
                else
                {
                    uint8 value(255.0f * (1.0f - normal));

                    uint32 color = 0xC0000000; // slightly transparent
                    color |= (value << 16);
                    color |= (value << 8);
                    color |= value;

                    data[index] = color;
                }
            }
        }

        IRenderer* renderer = gEnv->pRenderer;
        int textureId = renderer->DownLoadToVideoMemory((uint8*)data, texSize, texSize, eTF_R8G8B8A8, eTF_R8G8B8A8, 1);
        return gEnv->pRenderer->EF_GetTextureByID(textureId);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CMobileTouch::CMobileTouch(IInput& input, const char* deviceName)
    : CInputDevice(input, deviceName)
{
    m_deviceType = eIDT_TouchScreen;
    m_mousePositionX = gEnv->pRenderer->GetWidth() / 2.0f;
    m_mousePositionY = gEnv->pRenderer->GetHeight() / 2.0f;

    MapSymbol(0, eKI_Touch0, "touch0");
    MapSymbol(1, eKI_Touch1, "touch1");
    MapSymbol(2, eKI_Touch2, "touch2");
    MapSymbol(3, eKI_Touch3, "touch3");
    MapSymbol(4, eKI_Touch4, "touch4");
    MapSymbol(5, eKI_Touch5, "touch5");
    MapSymbol(6, eKI_Touch6, "touch6");
    MapSymbol(7, eKI_Touch7, "touch7");
    MapSymbol(8, eKI_Touch8, "touch8");
    MapSymbol(9, eKI_Touch9, "touch9");

    MapSymbol(100, eKI_Mouse1, "mouse1")->deviceType = eIDT_Mouse;
    MapSymbol(101, eKI_MouseX, "maxis_x", SInputSymbol::RawAxis)->deviceType = eIDT_Mouse;
    MapSymbol(102, eKI_MouseY, "maxis_y", SInputSymbol::RawAxis)->deviceType = eIDT_Mouse;

    REGISTER_CVAR(i_SimulateMouseEventsWithPrimaryTouch, false, VF_NULL, "Simulated mouse events using the primary touch. Note that no touch input events will be sent if this is enabled, as some systems listen for both mouse and touch events\n");
    REGISTER_CVAR(i_DebugShowTouches, false, VF_DEV_ONLY, "0=off, 1=show the location and finger id of each touch\n");
    gEnv->pRenderer->AddRenderDebugListener(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CMobileTouch::~CMobileTouch()
{
    gEnv->pRenderer->RemoveRenderDebugListener(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CMobileTouch::Update(bool focus)
{
    // Process the filtered touch events.
    for (uint32_t i = 0; i < m_filteredTouchEvents.size(); ++i)
    {
        ProcessTouchEvent(m_filteredTouchEvents[i]);
    }

    // Clear the filtered touch events.
    m_filteredTouchEvents.clear();

    if (i_SimulateMouseEventsWithPrimaryTouch)
    {
        // Update the position of the mouse cursor.
#if !defined(AZ_FRAMEWORK_INPUT_ENABLED)
        gEnv->pHardwareMouse->SetHardwareMousePosition(m_mousePositionX, m_mousePositionX);
#endif // !defined(AZ_FRAMEWORK_INPUT_ENABLED)

        // Send one mouse event each frame to report it's last known position.
        SInputEvent event;
        event.deviceType = eIDT_Mouse;
        event.keyId = eKI_MousePosition;
        event.keyName = "mouse_pos";
        event.screenPosition.x = m_mousePositionX;
        event.screenPosition.y = m_mousePositionY;
        GetIInput().PostInputEvent(event);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CMobileTouch::OnTouchBegan(const TouchEvent& touchEvent)
{
    m_filteredTouchEvents.push_back(touchEvent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CMobileTouch::OnTouchMoved(const TouchEvent& touchEvent)
{
    // Because touches are sampled at a rate (~30-60fps) independent of the simulation
    // (which is necessary for the controls to remain responsive), when the simulation
    // frame rate drops we end up receiving multiple touch move events each frame for
    // the same finger, which (especially with multi-touch) can generate enough events
    // to cause the simulation frame rate to drop even further. To combat this we'll
    // filter/combine all touch move events received in the same frame, maintaining the
    // correct order of events by retaining the first move event in the queue, while
    // updating it's position to match that of the last event received for that finger.
    // This ensures we only receive one move event per finger per frame, while maintaining
    // both responsiveness and accuracy.
    for (uint32_t i = 0; i < m_filteredTouchEvents.size(); ++i)
    {
        TouchEvent& existingTouchEvent = m_filteredTouchEvents[i];
        if ((touchEvent.index == existingTouchEvent.index) &&
            (touchEvent.state == existingTouchEvent.state))
        {
            // Update the existing touch move and return.
            existingTouchEvent.xNormalized = touchEvent.xNormalized;
            existingTouchEvent.yNormalized = touchEvent.yNormalized;
            existingTouchEvent.pressure = touchEvent.pressure;
            return;
        }
    }

    // This is the first touch move for this finger index this frame.
    m_filteredTouchEvents.push_back(touchEvent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CMobileTouch::OnTouchEnded(const TouchEvent& touchEvent)
{
    m_filteredTouchEvents.push_back(touchEvent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CMobileTouch::ProcessTouchEvent(const TouchEvent& touchEvent)
{
    if (i_SimulateMouseEventsWithPrimaryTouch)
    {
        SimulateMouseEvent(touchEvent);
        return;
    }

    SInputSymbol* inputSymbol = DevSpecIdToSymbol(touchEvent.index);
    if (!inputSymbol)
    {
        // We only support touches 0 through 9
        return;
    }

    inputSymbol->screenPosition.x = touchEvent.xNormalized * gEnv->pRenderer->GetWidth();
    inputSymbol->screenPosition.y = touchEvent.yNormalized * gEnv->pRenderer->GetHeight();
    inputSymbol->value = touchEvent.pressure;

    switch (touchEvent.state)
    {
        case TouchEvent::State::Began:
        {
            inputSymbol->state = eIS_Pressed;
        }
        break;
        case TouchEvent::State::Moved:
        {
            inputSymbol->state = eIS_Changed;
        }
        break;
        case TouchEvent::State::Ended:
        {
            inputSymbol->state = eIS_Released;
        }
        break;
    }

    SInputEvent inputEvent;
    inputEvent.deviceIndex = GetDeviceIndex();
    inputSymbol->AssignTo(inputEvent, GetIInput().GetModifiers());
    GetIInput().PostInputEvent(inputEvent);

#if !defined(_RELEASE)
    const Vec2 pos(touchEvent.xNormalized, touchEvent.yNormalized);
    switch (touchEvent.state)
    {
        case TouchEvent::State::Began:
        case TouchEvent::State::Moved:
        {
            m_debugTouchesToDraw[touchEvent.index] = pos;
        }
        break;
        case TouchEvent::State::Ended:
        {
            m_debugTouchesToDraw.erase(touchEvent.index);
        }
        break;
    }
#endif // !defined(_RELEASE)
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CMobileTouch::SimulateMouseEvent(const TouchEvent& touchEvent)
{
    if (touchEvent.index != 0)
    {
        // We only simulate the mouse with the primary touch.
        return;
    }

    // Unlike a real mouse, we only receive move events while the touch is pressed, meaning there is
    // no limit to the distance that the simulated mouse can travel between one event and the last.
    // To account for this, we always want to send a mouse moved event right before a mouse button
    // pressed event. This 'fake' move event will most likely be located a significant distance away
    // from the last move event, but in practice it's normally best for this 'jump' to occur between
    // two move events rather than between a new pressed event and the last move event.
    const float newMousePositionX = touchEvent.xNormalized * gEnv->pRenderer->GetWidth();
    const float newMousePositionY = touchEvent.yNormalized * gEnv->pRenderer->GetHeight();

    const float deltaMousePositionX = newMousePositionX - m_mousePositionX;
    const float deltaMousePositionY = newMousePositionY - m_mousePositionY;

    m_mousePositionX = newMousePositionX;
    m_mousePositionY = newMousePositionY;

    SimulateMouseEvent(eKI_MouseX, eIS_Changed, m_mousePositionX, m_mousePositionY, deltaMousePositionX);
    SimulateMouseEvent(eKI_MouseY, eIS_Changed, m_mousePositionX, m_mousePositionY, deltaMousePositionY);

    switch (touchEvent.state)
    {
        case TouchEvent::State::Began:
        {
            SimulateMouseEvent(eKI_Mouse1, eIS_Pressed, m_mousePositionX, m_mousePositionY, 1.0f);
        }
        break;
        case TouchEvent::State::Moved:
        {
            // Already sent above.
        }
        break;
        case TouchEvent::State::Ended:
        {
            SimulateMouseEvent(eKI_Mouse1, eIS_Released, m_mousePositionX, m_mousePositionY, 0.0f);
        }
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CMobileTouch::SimulateMouseEvent(EKeyId keyId,
                                      EInputState inputState,
                                      float screenPositionX,
                                      float screenPositionY,
                                      float value)
{
    SInputSymbol* inputSymbol = IdToSymbol(keyId);
    inputSymbol->state = inputState;
    inputSymbol->screenPosition.x = screenPositionX;
    inputSymbol->screenPosition.y = screenPositionY;
    inputSymbol->value = value;

    SInputEvent inputEvent;
    inputEvent.deviceIndex = GetDeviceIndex();
    inputSymbol->AssignTo(inputEvent, GetIInput().GetModifiers());
    GetIInput().PostInputEvent(inputEvent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CMobileTouch::OnDebugDraw()
{
#if !defined(_RELEASE)

    // make sure the debug flag is set before continuing
    if (!i_DebugShowTouches)
    {
        return;
    }

    // generate the indicator texture
    const int texSize = 128;
    static ITexture* s_touchIndicator = nullptr;
    if (!s_touchIndicator)
    {
        s_touchIndicator = GenerateDebugCircleTexture(texSize);
    }

    IDraw2d* draw2d = Draw2dHelper::GetDraw2d();
    if (!draw2d)
    {
        return;
    }

    // setup the text rendering options
    IDraw2d::TextOptions textOptions = draw2d->GetDefaultTextOptions();
    textOptions.horizontalAlignment = IDraw2d::HAlign::Left;
    textOptions.verticalAlignment = IDraw2d::VAlign::Bottom;
    textOptions.dropShadowOffset = AZ::Vector2(1.0f, 1.0f);
    textOptions.dropShadowColor = AZ::Color(1.0f, 0.0f, 0.0f, 0.7f);

    const float textHeight = 20.0f;
    const float startingX = 5.0f;
    const float startingY = draw2d->GetViewportHeight() - (textHeight * 11.0f);
    const AZ::Vector2 imgSize(texSize, texSize);
    char buffer[32];

    // setup 2d render settings
    draw2d->BeginDraw2d();

    sprintf_s(buffer, "Touch Count: %u", m_debugTouchesToDraw.size());
    draw2d->DrawText(buffer, AZ::Vector2(startingX, startingY), 20, 1.0f, &textOptions);

    int touchCount = 0;
    for (auto touchIter = m_debugTouchesToDraw.begin();
         touchIter != m_debugTouchesToDraw.end();
         ++touchIter, ++touchCount)
    {
        sprintf_s(buffer, "- FingerID: %i", touchIter->first);
        draw2d->DrawText(buffer, AZ::Vector2(startingX, startingY + (20.0f * (touchCount + 1))), 20, 1.0f, &textOptions);

        // draw a circle where the touch is
        const AZ::Vector2 imgPos = AZ::Vector2(touchIter->second.x * draw2d->GetViewportWidth(), touchIter->second.y * draw2d->GetViewportHeight());
        draw2d->DrawImageAligned(s_touchIndicator->GetTextureID(), imgPos, imgSize, IDraw2d::HAlign::Center, IDraw2d::VAlign::Center, 1.0f);

        sprintf_s(buffer, "%i", touchIter->first);
        draw2d->DrawText(buffer, imgPos - (imgSize * 0.5f), 25.0, 1.0f, &textOptions);
    }

    // restore the render state
    draw2d->EndDraw2d();
#endif // !defined(_RELEASE)
}
