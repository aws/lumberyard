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

#include <MacLumberyardApplication.h>

#include <AzFramework/API/ApplicationAPI_darwin.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
@implementation MacLumberyardApplicationDelegate

////////////////////////////////////////////////////////////////////////////////////////////////////
- (void)applicationWillBecomeActive: (NSNotification*)notification
{
    EBUS_EVENT(AzFramework::DarwinLifecycleEvents::Bus, OnWillBecomeActive);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
- (void)applicationDidBecomeActive: (NSNotification*)notification
{
    EBUS_EVENT(AzFramework::DarwinLifecycleEvents::Bus, OnDidBecomeActive);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
- (void)applicationWillResignActive: (NSNotification*)notification
{
    EBUS_EVENT(AzFramework::DarwinLifecycleEvents::Bus, OnWillResignActive);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
- (void)applicationDidResignActive: (NSNotification*)notification
{
    EBUS_EVENT(AzFramework::DarwinLifecycleEvents::Bus, OnDidResignActive);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
- (void)applicationWillTerminate: (NSNotification*)notification
{
    EBUS_EVENT(AzFramework::DarwinLifecycleEvents::Bus, OnWillTerminate);
}

@end // MacLumberyardApplicationDelegate Implementation
