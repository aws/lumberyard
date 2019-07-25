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

#include <AzFramework/API/ApplicationAPI_appletv.h>
#include <AzFramework/Application/Application.h>

#include <UIKit/UIKit.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class ApplicationAppleTV
        : public Application::Implementation
        , public AppleTVLifecycleEvents::Bus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        AZ_CLASS_ALLOCATOR(ApplicationAppleTV, AZ::SystemAllocator, 0);
        ApplicationAppleTV();
        ~ApplicationAppleTV() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // AppleTVLifecycleEvents
        void OnWillResignActive() override;
        void OnDidBecomeActive() override;
        void OnDidEnterBackground() override;
        void OnWillEnterForeground() override;
        void OnWillTerminate() override;
        void OnDidReceiveMemoryWarning() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Application::Implementation
        void PumpSystemEventLoopOnce() override;
        void PumpSystemEventLoopUntilEmpty() override;

    private:
        ApplicationLifecycleEvents::Event m_lastEvent;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    Application::Implementation* Application::Implementation::Create()
    {
        return aznew ApplicationAppleTV();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const char* Application::Implementation::GetAppRootPath()
    {
        return nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ApplicationAppleTV::ApplicationAppleTV()
        : m_lastEvent(ApplicationLifecycleEvents::Event::None)
    {
        AppleTVLifecycleEvents::Bus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ApplicationAppleTV::~ApplicationAppleTV()
    {
        AppleTVLifecycleEvents::Bus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAppleTV::OnWillResignActive()
    {
        EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnApplicationConstrained, m_lastEvent);
        m_lastEvent = ApplicationLifecycleEvents::Event::Constrain;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAppleTV::OnDidBecomeActive()
    {
        EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnApplicationUnconstrained, m_lastEvent);
        m_lastEvent = ApplicationLifecycleEvents::Event::Unconstrain;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAppleTV::OnDidEnterBackground()
    {
        EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnApplicationSuspended, m_lastEvent);
        m_lastEvent = ApplicationLifecycleEvents::Event::Suspend;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAppleTV::OnWillEnterForeground()
    {
        EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnApplicationResumed, m_lastEvent);
        m_lastEvent = ApplicationLifecycleEvents::Event::Resume;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAppleTV::OnWillTerminate()
    {
        EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnMobileApplicationWillTerminate);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAppleTV::OnDidReceiveMemoryWarning()
    {
        EBUS_EVENT(ApplicationLifecycleEvents::Bus, OnMobileApplicationLowMemoryWarning);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAppleTV::PumpSystemEventLoopOnce()
    {
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.0, TRUE);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationAppleTV::PumpSystemEventLoopUntilEmpty()
    {
        SInt32 result;
        do
        {
            result = CFRunLoopRunInMode(kCFRunLoopDefaultMode, DBL_EPSILON, TRUE);
        }
        while (result == kCFRunLoopRunHandledSource);
    }
} // namespace AzFramework
