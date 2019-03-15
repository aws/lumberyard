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
#include <AzCore/Android/JNI/Object.h>
#include <AzCore/Android/JNI/scoped_ref.h>
#include <AzCore/Android/Utils.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/conditional_variable.h>

#include <android/input.h>
#include <android/keycodes.h>
#include <android_native_app_glue.h>


////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    namespace
    {
        class PermissionRequestResultNotification
            : public AZ::EBusTraits
        {
        public:
            using Bus = AZ::EBus<PermissionRequestResultNotification>;

            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

            virtual ~PermissionRequestResultNotification() = default;

            virtual void OnRequestPermissionsResult(bool granted) { AZ_UNUSED(granted); };
        };


        void JNI_OnRequestPermissionsResult(JNIEnv* env, jobject obj, bool granted)
        {
            AzFramework::PermissionRequestResultNotification::Bus::Broadcast(&AzFramework::PermissionRequestResultNotification::OnRequestPermissionsResult, granted);
        }

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
        , public PermissionRequestResultNotification::Bus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        AZ_CLASS_ALLOCATOR(ApplicationAndroid, AZ::SystemAllocator, 0);
        ApplicationAndroid();
        ~ApplicationAndroid() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // AndroidAppRequests
        void SetAppState(android_app* appState) override;
        bool RequestPermission(const AZStd::string& permission, const AZStd::string& rationale) override;

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
        // PermissionRequestResultNotification
        void OnRequestPermissionsResult(bool granted) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Application::Implementation
        void PumpSystemEventLoopOnce() override;
        void PumpSystemEventLoopUntilEmpty() override;

    private:
        android_app* m_appState;
        ApplicationLifecycleEvents::Event m_lastEvent;

        AZStd::atomic<bool> m_requestResponseReceived;
        AZStd::unique_ptr<AZ::Android::JNI::Object> m_lumberyardActivity;
        AZStd::condition_variable m_conditionVar;
        AZStd::mutex m_mutex;
        bool m_permissionGranted;
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
        m_lumberyardActivity.reset(aznew AZ::Android::JNI::Object(AZ::Android::Utils::GetActivityClassRef(), AZ::Android::Utils::GetActivityRef()));

        m_lumberyardActivity->RegisterNativeMethods(
        { { "nativeOnRequestPermissionsResult", "(Z)V", (void*)JNI_OnRequestPermissionsResult } }
        );

        m_lumberyardActivity->RegisterMethod("RequestPermission", "(Ljava/lang/String;Ljava/lang/String;)V");

        AndroidLifecycleEvents::Bus::Handler::BusConnect();
        AndroidAppRequests::Bus::Handler::BusConnect();
        PermissionRequestResultNotification::Bus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ApplicationAndroid::~ApplicationAndroid()
    {
        m_lumberyardActivity.reset();
        PermissionRequestResultNotification::Bus::Handler::BusDisconnect();
        AndroidAppRequests::Bus::Handler::BusDisconnect();
        AndroidLifecycleEvents::Bus::Handler::BusDisconnect();
        m_conditionVar.notify_all();
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
    bool ApplicationAndroid::RequestPermission(const AZStd::string& permission, const AZStd::string& rationale)
    {
        AZ::Android::JNI::scoped_ref<jstring> permissionString(AZ::Android::JNI::ConvertStringToJstring(permission));
        AZ::Android::JNI::scoped_ref<jstring> permissionRationaleString(AZ::Android::JNI::ConvertStringToJstring(rationale));

        m_requestResponseReceived = false;
        m_permissionGranted = false;

        m_lumberyardActivity->InvokeVoidMethod("RequestPermission", permissionString.get(), permissionRationaleString.get());

        bool looperExistsForThread = (ALooper_forThread() != nullptr);

        // Make sure a looper exists for thread before pumping events.
        // For threads that are not the main thread, we just block and
        // the events will be pumped by the main thread and unblock this
        // thread when the user responds.
        if (looperExistsForThread)
        {
            while (!m_requestResponseReceived.load())
            {
                PumpSystemEventLoopOnce();
            }
        }
        else
        {
            AZStd::unique_lock<AZStd::mutex> lock(m_mutex);
            m_conditionVar.wait(lock, [&] { return m_requestResponseReceived.load(); });
        }

        return m_permissionGranted;
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
    void ApplicationAndroid::OnRequestPermissionsResult(bool granted)
    {
        m_permissionGranted = granted;
        m_requestResponseReceived = true;
        m_conditionVar.notify_all();
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
