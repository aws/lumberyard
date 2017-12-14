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

#include <AzFramework/Input/Devices/VirtualKeyboard/InputDeviceVirtualKeyboard.h>
#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_android.h>

#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzCore/Android/JNI/Object.h>
#include <AzCore/Android/Utils.h>

#include <android/input.h>
#include <android/keycodes.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    void JNI_OnRawVirtualKeyEvent(JNIEnv* jniEnv, jobject objectRef, int keyCode, int keyAction)
    {
        RawInputNotificationBusAndroid::Broadcast(
            &RawInputNotificationsAndroid::OnRawInputVirtualKeyboardEvent,
            keyCode,
            keyAction);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void JNI_OnRawTextEvent(JNIEnv* jniEnv, jobject objectRef, jstring stringUTF16)
    {
        const char* charsModifiedUTF8 = jniEnv->GetStringUTFChars(stringUTF16, nullptr);
        RawInputNotificationBusAndroid::Broadcast(&RawInputNotificationsAndroid::OnRawInputTextEvent,
                                                  charsModifiedUTF8);
        jniEnv->ReleaseStringUTFChars(stringUTF16, charsModifiedUTF8);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Platform specific implementation for android virtual keyboard input devices
    class InputDeviceVirtualKeyboardAndroid : public InputDeviceVirtualKeyboard::Implementation
                                            , public RawInputNotificationBusAndroid::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceVirtualKeyboardAndroid, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputDevice Reference to the input device being implemented
        InputDeviceVirtualKeyboardAndroid(InputDeviceVirtualKeyboard& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceVirtualKeyboardAndroid() override;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceVirtualKeyboard::Implementation::IsConnected
        bool IsConnected() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceVirtualKeyboard::Implementation::HasTextEntryStarted
        bool HasTextEntryStarted() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceVirtualKeyboard::Implementation::TextEntryStart
        void TextEntryStart(const InputTextEntryRequests::VirtualKeyboardOptions& options) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceVirtualKeyboard::Implementation::TextEntryStop
        void TextEntryStop() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceVirtualKeyboard::Implementation::TickInputDevice
        void TickInputDevice() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::RawInputNotificationsAndroid::OnRawInputTextEvent
        void OnRawInputTextEvent(const char* charsModifiedUTF8) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::RawInputNotificationsAndroid::OnRawInputVirtualKeyboardEvent
        void OnRawInputVirtualKeyboardEvent(int keyCode, int keyAction) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Raw input events on android can (and likely will) be dispatched from a thread other than
        //! main, and InputDeviceVirtualKeyboard::Implementation::QueueRawCommandEvent is not thread
        //! safe. So we'll store them in m_threadAwareRawCommandEvents to process in TickInputDevice
        //! on the main thread, ensuring all access is locked by m_threadAwareRawCommandEventsMutex.
        ///@{
        AZStd::vector<InputChannelId> m_threadAwareRawCommandEvents;
        AZStd::mutex                  m_threadAwareRawCommandEventsMutex;
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Raw input events on android can (and likely will) be dispatched from a thread other than
        //! main, and InputDeviceVirtualKeyboard::Implementation::QueueRawTextEvent is not thread
        //! safe. So we'll store them in m_threadAwareRawTextEvents to process in TickInputDevice
        //! on the main thread, ensuring all access is locked by m_threadAwareRawTextEventsMutex.
        ///@{
        AZStd::vector<AZStd::string> m_threadAwareRawTextEvents;
        AZStd::mutex                 m_threadAwareRawTextEventsMutex;
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Unique pointer to the keyboard handler JNI object
        AZStd::unique_ptr<AZ::Android::JNI::Object> m_keyboardHandler;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceVirtualKeyboard::Implementation* InputDeviceVirtualKeyboard::Implementation::Create(
        InputDeviceVirtualKeyboard& inputDevice)
    {
        return aznew InputDeviceVirtualKeyboardAndroid(inputDevice);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceVirtualKeyboardAndroid::InputDeviceVirtualKeyboardAndroid(
        InputDeviceVirtualKeyboard& inputDevice)
        : InputDeviceVirtualKeyboard::Implementation(inputDevice)
    {
        // Get the keyboard handler from the activity
        AZ::Android::JNI::Object activity(AZ::Android::Utils::GetActivityClassRef(),
                                             AZ::Android::Utils::GetActivityRef());
        activity.RegisterMethod("GetKeyboardHandler", "()Lcom/amazon/lumberyard/input/KeyboardHandler;");
        jobject keyboardRef = activity.InvokeObjectMethod<jobject>("GetKeyboardHandler");
        jclass keyboardClass = AZ::Android::JNI::LoadClass("com/amazon/lumberyard/input/KeyboardHandler");

        // Initialize the keyboard handler
        m_keyboardHandler.reset(aznew AZ::Android::JNI::Object(keyboardClass, keyboardRef, true));
        m_keyboardHandler->RegisterMethod("ShowTextInput", "()V");
        m_keyboardHandler->RegisterMethod("HideTextInput", "()V");
        m_keyboardHandler->RegisterMethod("IsShowing", "()Z");
        m_keyboardHandler->RegisterNativeMethods(
        {
            { "SendKeyCode", "(II)V", (void*)JNI_OnRawVirtualKeyEvent },
            { "SendUnicodeText", "(Ljava/lang/String;)V", (void*)JNI_OnRawTextEvent }
        });

        // Connect to the raw input notifications bus
        RawInputNotificationBusAndroid::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceVirtualKeyboardAndroid::~InputDeviceVirtualKeyboardAndroid()
    {
        // Disconnect from the raw input notifications bus
        RawInputNotificationBusAndroid::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceVirtualKeyboardAndroid::IsConnected() const
    {
        // Virtual keyboard input is always available
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceVirtualKeyboardAndroid::HasTextEntryStarted() const
    {
        return m_keyboardHandler ? m_keyboardHandler->InvokeBooleanMethod("IsShowing") == JNI_TRUE : false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceVirtualKeyboardAndroid::TextEntryStart(const InputTextEntryRequests::VirtualKeyboardOptions&)
    {
        if (m_keyboardHandler && !m_keyboardHandler->InvokeBooleanMethod("IsShowing"))
        {
            m_keyboardHandler->InvokeVoidMethod("ShowTextInput");
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceVirtualKeyboardAndroid::TextEntryStop()
    {
        if (m_keyboardHandler && m_keyboardHandler->InvokeBooleanMethod("IsShowing"))
        {
            m_keyboardHandler->InvokeVoidMethod("HideTextInput");
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceVirtualKeyboardAndroid::TickInputDevice()
    {
        // The input event loop is pumped by another thread on android so all raw input events this
        // frame have already been dispatched. But they are all queued until ProcessRawEventQueues
        // is called below so that all raw input events are processed at the same time every frame.

        // Because m_threadAwareRawCommandEvents is updated from another thread but also needs to be
        // processed and cleared in this function, swapping it with an empty vector kills two birds
        // with one stone in a very efficient and elegant manner (kudos to scottr for the idea).
        AZStd::vector<InputChannelId> rawCommandEvents;
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_threadAwareRawCommandEventsMutex);
            rawCommandEvents.swap(m_threadAwareRawCommandEvents);
        }

        // Queue the raw command events that were received over the last frame
        for (const InputChannelId& rawCommandEvent : rawCommandEvents)
        {
            QueueRawCommandEvent(rawCommandEvent);
        }

        // Because m_threadAwareRawTextEvents is updated from another thread, but also needs to be
        // processed and cleared in this function, swapping it with an empty vector kills two birds
        // with one stone in a very efficient and elegant manner (kudos to scottr for the idea).
        AZStd::vector<AZStd::string> rawTextEvents;
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_threadAwareRawTextEventsMutex);
            rawTextEvents.swap(m_threadAwareRawTextEvents);
        }

        // Queue the raw text events that were received over the last frame
        for (const AZStd::string& rawTextEvent : rawTextEvents)
        {
            QueueRawTextEvent(rawTextEvent);
        }

        // Process the raw event queues once each frame
        ProcessRawEventQueues();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceVirtualKeyboardAndroid::OnRawInputTextEvent(const char* charsModifiedUTF8)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_threadAwareRawTextEventsMutex);
        m_threadAwareRawTextEvents.push_back(AZStd::string(charsModifiedUTF8));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceVirtualKeyboardAndroid::OnRawInputVirtualKeyboardEvent(int keyCode, int keyAction)
    {
        // We only care about key down actions
        if (keyAction != AKEY_EVENT_ACTION_DOWN)
        {
            return;
        }

        switch (keyCode)
        {
            case AKEYCODE_BACK:
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_threadAwareRawCommandEventsMutex);
                m_threadAwareRawCommandEvents.push_back(AzFramework::InputDeviceVirtualKeyboard::Command::NavigationBack);
            }
            break;
            case AKEYCODE_CLEAR:
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_threadAwareRawCommandEventsMutex);
                m_threadAwareRawCommandEvents.push_back(AzFramework::InputDeviceVirtualKeyboard::Command::EditClear);
            }
            break;
            case AKEYCODE_ENTER:
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_threadAwareRawCommandEventsMutex);
                m_threadAwareRawCommandEvents.push_back(AzFramework::InputDeviceVirtualKeyboard::Command::EditEnter);
            }
            break;
            case AKEYCODE_DEL:
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_threadAwareRawTextEventsMutex);
                m_threadAwareRawTextEvents.push_back(AZStd::string("\b"));
            }
            break;
            case AKEYCODE_SPACE:
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_threadAwareRawTextEventsMutex);
                m_threadAwareRawTextEvents.push_back(AZStd::string(" "));
            }
            break;
            default:
            {
                // Do nothing
            }
            break;
        }
    }
} // namespace AzFramework
