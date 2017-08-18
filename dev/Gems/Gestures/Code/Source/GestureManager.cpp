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
#include "GestureManager.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <Gestures/IGestureRecognizer.h>
#ifdef SIMULATE_MULTI_TOUCH
#include <IGameFramework.h>
#endif

namespace
{
#ifdef SIMULATE_MULTI_TOUCH
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    IPersistentDebug* GetPersistentDebug()
    {
        if (auto game = gEnv->pGame)
        {
            if (auto framework = game->GetIGameFramework())
            {
                return framework->GetIPersistentDebug();
            }
        }
        return nullptr;
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Gestures::Manager::Reflect(AZ::ReflectContext* context)
{
    if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serialize->Class<Manager, AZ::Component>()
            ->Version(0)
            ->SerializerForEmptyClass();

        if (AZ::EditContext* ec = serialize->GetEditContext())
        {
            ec->Class<Manager>("Gestures Manager", "")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Input")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Gestures::Manager::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
{
    provided.push_back(AZ_CRC("GesturesService"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Gestures::Manager::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
{
    incompatible.push_back(AZ_CRC("GesturesService"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Gestures::Manager::Manager()
#ifdef SIMULATE_MULTI_TOUCH
    : m_simulator(*this)
#endif
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Gestures::Manager::Activate()
{
    GesturesBus::Handler::BusConnect();
    CrySystemEventBus::Handler::BusConnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Gestures::Manager::Deactivate()
{
    GesturesBus::Handler::BusDisconnect();
    CrySystemEventBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Gestures::Manager::OnCrySystemInitialized(ISystem& system, const SSystemInitParams&)
{
    if (system.GetGlobalEnvironment()->pInput)
    {
        system.GetGlobalEnvironment()->pInput->AddEventListener(this);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Gestures::Manager::OnCrySystemShutdown(ISystem& system)
{
    if (system.GetGlobalEnvironment()->pInput)
    {
        system.GetGlobalEnvironment()->pInput->RemoveEventListener(this);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CompareGestureRecognizer(const Gestures::IRecognizer* a, const Gestures::IRecognizer* b)
{
    return (a && b) ? a->GetPriority() < b->GetPriority() : false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Gestures::Manager::Register(Gestures::IRecognizer& recognizer)
{
    if (std::find(m_recognizers.begin(), m_recognizers.end(), &recognizer) == m_recognizers.end())
    {
        m_recognizers.push_back(&recognizer);
        sort(m_recognizers.begin(), m_recognizers.end(), CompareGestureRecognizer);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Gestures::Manager::Deregister(Gestures::IRecognizer& recognizer)
{
    const auto& it = std::find(m_recognizers.begin(), m_recognizers.end(), &recognizer);
    if (it != m_recognizers.end())
    {
        m_recognizers.erase(it);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Gestures::Manager::OnInputEvent(const SInputEvent& inputEvent)
{
#ifdef SIMULATE_MULTI_TOUCH
    m_simulator.Update(inputEvent);
#endif

    // convert mouse and touch to common zero-based pointer index
    uint32_t index = 0;
    if (inputEvent.keyId >= eKI_Mouse1 &&
        inputEvent.keyId <= eKI_Mouse8)
    {
        index = inputEvent.keyId - eKI_Mouse1;
    }
    else if (inputEvent.keyId >= eKI_Touch0 &&
             inputEvent.keyId <= eKI_Touch9)
    {
        index = inputEvent.keyId - eKI_Touch0;
    }
    else
    {
        return false;
    }

    bool processed = false;

    switch (inputEvent.state)
    {
    case eIS_Pressed:
    {
        processed = OnPressedEvent(inputEvent.screenPosition, index);
    }
    break;
    case eIS_Down:
    {
        processed = OnDownEvent(inputEvent.screenPosition, index);
    }
    break;
    case eIS_Released:
    {
        processed = OnReleasedEvent(inputEvent.screenPosition, index);
    }
    break;
    }

#ifdef SIMULATE_MULTI_TOUCH
    processed |= m_simulator.SimulateEvents(inputEvent, index);
#endif

    return processed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Gestures::Manager::OnPressedEvent(const Vec2& screenPosition, uint32_t pointerIndex)
{
    for (Gestures::IRecognizer* recognizer : m_recognizers)
    {
        const bool consumeEvent = recognizer->OnPressedEvent(screenPosition, pointerIndex);
        if (consumeEvent)
        {
            return true;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Gestures::Manager::OnDownEvent(const Vec2& screenPosition, uint32_t pointerIndex)
{
    for (Gestures::IRecognizer* recognizer : m_recognizers)
    {
        const bool consumeEvent = recognizer->OnDownEvent(screenPosition, pointerIndex);
        if (consumeEvent)
        {
            return true;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Gestures::Manager::OnReleasedEvent(const Vec2& screenPosition, uint32_t pointerIndex)
{
    for (Gestures::IRecognizer* recognizer : m_recognizers)
    {
        const bool consumeEvent = recognizer->OnReleasedEvent(screenPosition, pointerIndex);
        if (consumeEvent)
        {
            return true;
        }
    }

    return false;
}

#ifdef SIMULATE_MULTI_TOUCH
////////////////////////////////////////////////////////////////////////////////////////////////////
Gestures::Manager::MultiTouchSimulator::MultiTouchSimulator(Gestures::Manager& mgr)
    : m_mgr(mgr)
    , m_simulating(false)
    , m_shifting(false)
    , m_index0IsDown(false)
    , m_reflectPoint(ZERO)
    , m_mousePosition(ZERO)
    , m_shiftOffset(ZERO)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Gestures::Manager::MultiTouchSimulator::Update(const SInputEvent& inputEvent)
{
    // Always cache the mouse position
    if (inputEvent.deviceType == EInputDeviceType::eIDT_Mouse &&
        inputEvent.keyId == EKeyId::eKI_MousePosition)
    {
        m_mousePosition = inputEvent.screenPosition;

        if (m_shifting)
        {
            m_reflectPoint = m_mousePosition + m_shiftOffset;
        }

        DebugDraw();
        return;
    }

    // LEFT ALT key enables touch simulation
    else if (inputEvent.deviceType == EInputDeviceType::eIDT_Keyboard &&
             inputEvent.keyId == EKeyId::eKI_LAlt)
    {
        // LEFT ALT was pressed
        if (inputEvent.state == EInputState::eIS_Pressed)
        {
            m_reflectPoint = m_mousePosition;
            m_simulating = true;

            // if index 0 is already pressed/down, pressing ALT simulates index 1 being pressed
            if (m_index0IsDown)
            {
                m_mgr.OnPressedEvent(m_mousePosition, 1);
            }
        }

        // LEFT ALT was released
        else if (inputEvent.state == EInputState::eIS_Released)
        {
            if (m_simulating)
            {
                m_simulating = false;

                // releasing the LEFT ALT key while simulating will simulate index 1 being released
                const Vec2 reflectedPosition = ReflectPoint(inputEvent.screenPosition);
                m_mgr.OnReleasedEvent(reflectedPosition, 1);
            }
        }
    }

    // SHIFT key will translate the current reflection point relative to the mouse position
    else if (inputEvent.deviceType == EInputDeviceType::eIDT_Keyboard &&
             inputEvent.keyId == EKeyId::eKI_LShift)
    {
        if (inputEvent.state == EInputState::eIS_Pressed)
        {
            if (!m_shifting)
            {
                m_shiftOffset = m_reflectPoint - m_mousePosition;
                m_shifting = true;
            }
        }
        else if (inputEvent.state == EInputState::eIS_Released)
        {
            if (m_shifting)
            {
                m_shiftOffset = Vec2(ZERO);
                m_shifting = false;
            }
        }
    }

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool Gestures::Manager::MultiTouchSimulator::SimulateEvents(const SInputEvent& inputEvent, uint32_t pointerIndex)
{
    // when shifting, move the reflection point relative to pointer index 0
    if (pointerIndex == 0 && m_shifting)
    {
        m_reflectPoint = inputEvent.screenPosition + m_shiftOffset;
    }

    // cache pressed state of the primary pointer
    m_index0IsDown = (pointerIndex == 0) && (inputEvent.state == eIS_Pressed || inputEvent.state == eIS_Down);

    // simulate index 1 events when an index 0 event occurs
    if (pointerIndex == 0 && m_simulating)
    {
        const Vec2 reflectedPosition = ReflectPoint(inputEvent.screenPosition);

        switch (inputEvent.state)
        {
        case eIS_Pressed:
        {
            return m_mgr.OnPressedEvent(reflectedPosition, 1);
        }
        break;
        case eIS_Down:
        {
            return m_mgr.OnDownEvent(reflectedPosition, 1);
        }
        break;
        case eIS_Released:
        {
            return m_mgr.OnReleasedEvent(reflectedPosition, 1);
        }
        break;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void Gestures::Manager::MultiTouchSimulator::DebugDraw()
{
    const ColorF pointer0Color = ColorF(1.0f, 0.0f, 0.0f);
    const ColorF pointer1Color = ColorF(1.0f, 0.0f, 0.0f);
    const ColorF reflectionPointColor = ColorF(1.0f, 0.0f, 0.0f);
    const float indicatorSize = 20.0f;
    const float debugLifetime = 1.0f;

    if (auto persistentDebug = GetPersistentDebug())
    {
        if (m_simulating)
        {
            persistentDebug->Begin("Gestures_Manager_ReflectPoint", true);
            persistentDebug->Add2DCircle(m_reflectPoint.x, m_reflectPoint.y, indicatorSize, reflectionPointColor, debugLifetime);
            persistentDebug->Add2DCircle(m_mousePosition.x, m_mousePosition.y, indicatorSize, pointer0Color, debugLifetime);

            Vec2 pointer1Pos = ReflectPoint(m_mousePosition);
            persistentDebug->Add2DCircle(pointer1Pos.x, pointer1Pos.y, indicatorSize, pointer1Color, debugLifetime);
        }
    }
}
#endif