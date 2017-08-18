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
#include "AndroidKeyboard.h"

#include <AzCore/Android/Utils.h>
#include <AzCore/Android/JNI/Object.h>

#include <android/input.h>
#include <android/keycodes.h>


namespace
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void Java_OnKeyEvent(JNIEnv* jniEnv, jobject objectRef, int keyCode, int action)
    {
        // This event is sent when multiple key ups/downs of the same key happen in a quick succession or
        // when some extended ASCII/Unicode characters are sent. In either scenario the event is useless to us.
        if (action == AKEY_EVENT_ACTION_MULTIPLE)
        {
            return;
        }

        switch (keyCode)
        {
            case AKEYCODE_BACK:
            case AKEYCODE_ENTER:
            {
                SInputEvent event;
                event.deviceType = eIDT_Keyboard;

                if (keyCode == AKEYCODE_BACK)
                {
                    event.keyId = eKI_Android_Back;
                    event.keyName = "back";
                }
                else
                {
                    event.keyId = eKI_Enter;
                    event.keyName = "enter";
                }

                if (action == AKEY_EVENT_ACTION_DOWN)
                {
                    event.state = eIS_Pressed;
                    event.value = 1.0f;
                }
                else
                {
                    event.state = eIS_Released;
                    event.value = 0.0f;
                }

                EBUS_EVENT(AzInput::RawVirtualKeyboardNotificationsBus, OnKeyEvent, event);
                break;
            }
            case AKEYCODE_DEL:
            {
                if (action == AKEY_EVENT_ACTION_DOWN)
                {
                    EBUS_EVENT(AzInput::RawVirtualKeyboardNotificationsBus, OnUnicodeEvent, AzInput::UnicodeEvent('\b'));
                }
                break;
            }
            case AKEYCODE_SPACE:
            {
                if (action == AKEY_EVENT_ACTION_DOWN)
                {
                    EBUS_EVENT(AzInput::RawVirtualKeyboardNotificationsBus, OnUnicodeEvent, AzInput::UnicodeEvent(' '));
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void Java_OnUnicodeEvent(JNIEnv* jniEnv, jobject objectRef, jstring unicodeString)
    {
        const char* value = jniEnv->GetStringUTFChars(unicodeString, nullptr);

        // Iterate over and send an event for each unicode codepoint of the new text.
        for (Unicode::CIterator<const char*, false> it(value); *it != 0; ++it)
        {
            const uint32_t codepoint = *it;
            EBUS_EVENT(AzInput::RawVirtualKeyboardNotificationsBus, OnUnicodeEvent, AzInput::UnicodeEvent(codepoint));
        }

        jniEnv->ReleaseStringUTFChars(unicodeString, value);
    }
}


// ----
// CAndroidKeyboard (public)
// ----

////////////////////////////////////////////////////////////////////////////////////////////////////
CAndroidKeyboard::CAndroidKeyboard(IInput& input)
    : CInputDevice(input, "Android Keyboard")

    , m_keyboardHandler()

    , m_unicodeEventsMutex()
    , m_unicodeEvents()

    , m_keyEventsMutex()
    , m_keyEvents()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CAndroidKeyboard::~CAndroidKeyboard()
{
    AzInput::RawVirtualKeyboardNotificationsBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CAndroidKeyboard::Init()
{
    // get the keyboard handler form the activity
    AZ::Android::JNI::Object activity(AZ::Android::Utils::GetActivityClassRef(), AZ::Android::Utils::GetActivityRef());

    activity.RegisterMethod("GetKeyboardHandler", "()Lcom/amazon/lumberyard/input/KeyboardHandler;");
    jobject keyboardRef = activity.InvokeObjectMethod<jobject>("GetKeyboardHandler");

    jclass keyboardClass = AZ::Android::JNI::LoadClass("com/amazon/lumberyard/input/KeyboardHandler");

    // initialize the native keyboard handler
    m_keyboardHandler.reset(aznew AZ::Android::JNI::Object(keyboardClass, keyboardRef, true));

    m_keyboardHandler->RegisterMethod("ShowTextInput", "()V");
    m_keyboardHandler->RegisterMethod("HideTextInput", "()V");
    m_keyboardHandler->RegisterMethod("IsShowing", "()Z");

    m_keyboardHandler->RegisterNativeMethods({
        { "SendKeyCode", "(II)V", (void*)Java_OnKeyEvent },
        { "SendUnicodeText", "(Ljava/lang/String;)V", (void*)Java_OnUnicodeEvent }
    });

    AzInput::RawVirtualKeyboardNotificationsBus::Handler::BusConnect();

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CAndroidKeyboard::Update(bool)
{
    // copy the contents of the events vectors to minimize time locked and process them lock free
    AZStd::vector<AzInput::UnicodeEvent> unicodeEvents;
    {
        AZStd::lock_guard<AZStd::mutex> unicodeLock(m_unicodeEventsMutex);
        unicodeEvents.swap(m_unicodeEvents);
    }

    for (int i = 0; i < unicodeEvents.size(); ++i)
    {
        SUnicodeEvent event(unicodeEvents[i].m_unicodeCodepoint);
        gEnv->pInput->PostUnicodeEvent(event);
    }


    AZStd::vector<SInputEvent> keyEvents;
    {
        AZStd::lock_guard<AZStd::mutex> keyLock(m_keyEventsMutex);
        keyEvents.swap(m_keyEvents);
    }

    for (int i = 0; i < keyEvents.size(); ++i)
    {
        gEnv->pInput->PostInputEvent(keyEvents[i]);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CAndroidKeyboard::OnUnicodeEvent(const AzInput::UnicodeEvent& unicodeEvent)
{
    AZStd::lock_guard<AZStd::mutex> unicodeLock(m_unicodeEventsMutex);
    m_unicodeEvents.push_back(unicodeEvent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CAndroidKeyboard::OnKeyEvent(const SInputEvent& keyEvent)
{
    AZStd::lock_guard<AZStd::mutex> keyLock(m_keyEventsMutex);
    m_keyEvents.push_back(keyEvent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CAndroidKeyboard::StartTextInput(const Vec2& inputRectTopLeft, const Vec2& inputRectBottomRight)
{
    if (m_keyboardHandler)
    {
        if (m_keyboardHandler->InvokeBooleanMethod("IsShowing"))
        {
            AZ_TracePrintf("LMBR", "Keyboard is already showing");
        }
        else
        {
            m_keyboardHandler->InvokeVoidMethod("ShowTextInput");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CAndroidKeyboard::StopTextInput()
{
    if (m_keyboardHandler)
    {
        if (!m_keyboardHandler->InvokeBooleanMethod("IsShowing"))
        {
            AZ_TracePrintf("LMBR", "Keyboard is NOT showing");
        }
        else
        {
            m_keyboardHandler->InvokeVoidMethod("HideTextInput");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CAndroidKeyboard::IsScreenKeyboardShowing() const
{
    if (m_keyboardHandler)
    {
        return (m_keyboardHandler->InvokeBooleanMethod("IsShowing") == JNI_TRUE);
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CAndroidKeyboard::IsScreenKeyboardSupported() const
{
    return true;
}
