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

#include <AppleLauncher.h>

#include <AzFramework/API/ApplicationAPI_appletv.h>

#include <AzCore/IO/SystemFile.h> // for AZ_MAX_PATH_LEN

#include <SystemUtilsApple.h>

#import <UIKit/UIKit.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
@interface AppleTVLumberyardApplicationDelegate : NSObject<UIApplicationDelegate> {}
@end    // AppleTVLumberyardApplicationDelegate Interface

////////////////////////////////////////////////////////////////////////////////////////////////////
@implementation AppleTVLumberyardApplicationDelegate

////////////////////////////////////////////////////////////////////////////////////////////////////
- (void)launchLumberyardApplication
{
    // Unlike Mac where we have unrestricted access to the filesystem, iOS apps are sandboxed such
    // that you can only access a pre-defined set of directories. Further restrictions are placed
    // on AppleTV such that we can only actually write to the "Libraray/Caches" directory, which
    // unfortunately is not guaranteed to be persisted. Essentially the OS is free to delete it
    // at any time, although the docs suggest this will never happen while the app is running.
    // If persistent storage is required it must be done using iCloud.
    // https://developer.apple.com/library/mac/documentation/FileManagement/Conceptual/FileSystemProgrammingGuide/FileSystemOverview/FileSystemOverview.html
    char pathToApplicationPersistentStorage[AZ_MAX_PATH_LEN] = { 0 };
    SystemUtilsApple::GetPathToUserCachesDirectory(pathToApplicationPersistentStorage, AZ_MAX_PATH_LEN);

    const int exitCode = AppleLauncher::Launch("", pathToApplicationPersistentStorage);
    exit(exitCode);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions
{
    [self performSelector:@selector(launchLumberyardApplication) withObject:nil afterDelay:0.0];
    return YES;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
- (void)applicationWillResignActive:(UIApplication*)application
{
    EBUS_EVENT(AzFramework::AppleTVLifecycleEvents::Bus, OnWillResignActive);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
- (void)applicationDidEnterBackground:(UIApplication*)application
{
    EBUS_EVENT(AzFramework::AppleTVLifecycleEvents::Bus, OnDidEnterBackground);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
- (void)applicationWillEnterForeground:(UIApplication*)application
{
    EBUS_EVENT(AzFramework::AppleTVLifecycleEvents::Bus, OnWillEnterForeground);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
- (void)applicationDidBecomeActive:(UIApplication*)application
{
    EBUS_EVENT(AzFramework::AppleTVLifecycleEvents::Bus, OnDidBecomeActive);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
- (void)applicationWillTerminate:(UIApplication *)application
{
    EBUS_EVENT(AzFramework::AppleTVLifecycleEvents::Bus, OnWillTerminate);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
    EBUS_EVENT(AzFramework::AppleTVLifecycleEvents::Bus, OnDidReceiveMemoryWarning);
}

@end // AppleTVLumberyardApplicationDelegate Implementation
