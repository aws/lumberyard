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

#include <AzFramework/API/ApplicationAPI_android.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_android.h>

#include <AzCore/Android/AndroidEnv.h>

#include <android/input.h>
#include <android/keycodes.h>
#include <android_native_app_glue.h>


////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    namespace
    {
        // this callback is triggered on the same thread the events are pumped
        static int32_t InputHandler(android_app* app, AInputEvent* event)
        {
            RawInputNotificationBusAndroid::Broadcast(&RawInputNotificationsAndroid::OnRawInputEvent, event);
            return 0;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class ApplicationAndroid
        : public Application::Implementation
        , public AndroidLifecycleEvents::Bus::Handler
        , public AndroidAppRequests::Bus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        AZ_CLASS_ALLOCATOR(ApplicationAndroid, AZ::SystemAllocator, 0);
        ApplicationAndroid();
        ~ApplicationAndroid() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // AndroidAppRequests
        void SetAppState(android_app* appState) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // AndroidLifecycleEvents
        void OnLostFocus() override;
        void OnGainedFocus() override;
        void OnPause() override;
        void OnResume() override;
        void OnDestroy() override;
        void OnLowMemory() override;
        void OnWindowInit() override;
        void OnWindowDestroy() override;
        void OnWindowRedrawNeeded() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Application::Implementation
        void PumpSystemEventLoopOnce() override;
        void PumpSystemEventLoopUntilEmpty() override;

    private:
        android_app* m_appState;
        ApplicationLifecycleEvents::Event m_lastEvent;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    Application::Implementation* Application::Implementation::Create()
    {
        return aznew ApplicationAndroid();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ApplicationAndroid::ApplicationAndroid()
        : m_appState(nullptr)
        , m_lastEvent(ApplicationLifecycleEvents::Event::None)
    {
        AndroidLifecycleEvents::Bus::Handler::BusConnect();
        AndroidAppRequests::Bus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ApplicationAndroid::~ApplicationAndroid()
    {
        AndroidAppRequests::Bus::Handler::BusDisconnect();
        AndroidLifecycleEvents::Bus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAndroid::SetAppState(android_app* appState)
    {
        AZ_Assert(!m_appState, "Duplicate call to setting the Android application state!");
        m_appState = appState;
        if (m_appState)
        {
            m_appState->onInputEvent = InputHandler;
        }
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAndroid::OnLostFocus()
    {
        EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnApplicationConstrained, m_lastEvent);
        m_lastEvent = ApplicationLifecycleEvents::Event::Constrain;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAndroid::OnGainedFocus()
    {
        EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnApplicationUnconstrained, m_lastEvent);
        m_lastEvent = ApplicationLifecycleEvents::Event::Unconstrain;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAndroid::OnPause()
    {

        EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnApplicationSuspended, m_lastEvent);
        m_lastEvent = ApplicationLifecycleEvents::Event::Suspend;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAndroid::OnResume()
    {
        EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnApplicationResumed, m_lastEvent);
        m_lastEvent = ApplicationLifecycleEvents::Event::Resume;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAndroid::OnDestroy()
    {
        EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnMobileApplicationWillTerminate);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAndroid::OnLowMemory()
    {
        EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnMobileApplicationLowMemoryWarning);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAndroid::OnWindowInit()
    {
        EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnApplicationWindowCreated);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAndroid::OnWindowDestroy()
    {
        EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnApplicationWindowDestroy);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAndroid::OnWindowRedrawNeeded()
    {
        EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnApplicationWindowRedrawNeeded);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAndroid::PumpSystemEventLoopOnce()
    {
        AZ_ErrorOnce("ApplicationAndroid", m_appState, "The Android application state is not valid, unable to pump the event loop properly.");
        if (m_appState)
        {
            int events;
            android_poll_source* source;
            AZ::Android::AndroidEnv* androidEnv = AZ::Android::AndroidEnv::Get();

            // passing a negative value will cause the function to block till it recieves an event
            if (ALooper_pollOnce(androidEnv->IsRunning() ? 0 : -1, NULL, &events, reinterpret_cast<void**>(&source)) >= 0)
            {
                if (source != NULL)
                {
                    source->process(m_appState, source);
                }
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAndroid::PumpSystemEventLoopUntilEmpty()
    {
        AZ_ErrorOnce("ApplicationAndroid", m_appState, "The Android application state is not valid, unable to pump the event loop properly.");
        if (m_appState)
        {
            int events;
            android_poll_source* source;
            AZ::Android::AndroidEnv* androidEnv = AZ::Android::AndroidEnv::Get();

            // passing a negative value will cause the function to block till it recieves an event
            while (ALooper_pollAll(androidEnv->IsRunning() ? 0 : -1, NULL, &events, reinterpret_cast<void**>(&source)) >= 0)
            {
                if (source != NULL)
                {
                    source->process(m_appState, source);
                }

                if (m_appState->destroyRequested != 0)
                {
                    ApplicationRequests::Bus::Broadcast(&ApplicationRequests::ExitMainLoop);
                    break;
                }
            }
        }
    }
} // namespace AzFramework
