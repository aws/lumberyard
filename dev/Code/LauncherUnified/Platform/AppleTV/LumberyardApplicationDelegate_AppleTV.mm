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

#include <Launcher_precompiled.h>
#include <Launcher.h>

#include <../Common/Apple/Launcher_Apple.h>
#include <../Common/UnixLike/Launcher_UnixLike.h>

#include <AzFramework/API/ApplicationAPI_Platform.h>

#include <AzCore/IO/SystemFile.h> // for AZ_MAX_PATH_LEN

#include <SystemUtilsApple.h>

#import <UIKit/UIKit.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace
{
    const char* GetAppWriteStoragePath()
    {
        static char pathToApplicationPersistentStorage[AZ_MAX_PATH_LEN] = { 0 };

        // Unlike Mac where we have unrestricted access to the filesystem, iOS apps are sandboxed such
        // that you can only access a pre-defined set of directories. Further restrictions are placed
        // on AppleTV such that we can only actually write to the "Libraray/Caches" directory, which
        // unfortunately is not guaranteed to be persisted. Essentially the OS is free to delete it
        // at any time, although the docs suggest this will never happen while the app is running.
        // If persistent storage is required it must be done using iCloud.
        // https://developer.apple.com/library/mac/documentation/FileManagement/Conceptual/FileSystemProgrammingGuide/FileSystemOverview/FileSystemOverview.html
        SystemUtilsApple::GetPathToUserCachesDirectory(pathToApplicationPersistentStorage, AZ_MAX_PATH_LEN);
        return pathToApplicationPersistentStorage;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
@interface LumberyardApplicationDelegate_AppleTV : NSObject<UIApplicationDelegate> {}
@end    // LumberyardApplicationDelegate_AppleTV Interface

////////////////////////////////////////////////////////////////////////////////////////////////////
@implementation LumberyardApplicationDelegate_AppleTV

////////////////////////////////////////////////////////////////////////////////////////////////////
- (int)runLumberyardApplication
{
    using namespace LumberyardLauncher;

    PlatformMainInfo mainInfo;
    mainInfo.m_updateResourceLimits = IncreaseResourceLimits;
    mainInfo.m_appResourcesPath = GetAppResourcePath();
    mainInfo.m_appWriteStoragePath = GetAppWriteStoragePath();
    mainInfo.m_additionalVfsResolution = "\t- Check that usbmuxconnect is running and not reporting any errors when connecting to localhost";

    NSArray* commandLine = [[NSProcessInfo processInfo] arguments];

    for (size_t argIndex = 0; argIndex < [commandLine count]; ++argIndex)
    {
        NSString* arg = commandLine[argIndex];
        if (!mainInfo.AddArgument([arg UTF8String]))
        {
            return static_cast<int>(ReturnCode::ErrCommandLine); 
        }
    }

    ReturnCode status = Run(mainInfo);
    return static_cast<int>(status);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
- (void)launchLumberyardApplication
{
    const int exitCode = [self runLumberyardApplication];
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
    AzFramework::AppleTVLifecycleEvents::Bus::Broadcast(
        &AzFramework::AppleTVLifecycleEvents::Bus::Events::OnWillResignActive);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
- (void)applicationDidEnterBackground:(UIApplication*)application
{
    AzFramework::AppleTVLifecycleEvents::Bus::Broadcast(
        &AzFramework::AppleTVLifecycleEvents::Bus::Events::OnDidEnterBackground);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
- (void)applicationWillEnterForeground:(UIApplication*)application
{
    AzFramework::AppleTVLifecycleEvents::Bus::Broadcast(
        &AzFramework::AppleTVLifecycleEvents::Bus::Events::OnWillEnterForeground);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
- (void)applicationDidBecomeActive:(UIApplication*)application
{
    AzFramework::AppleTVLifecycleEvents::Bus::Broadcast(
        &AzFramework::AppleTVLifecycleEvents::Bus::Events::OnDidBecomeActive);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
- (void)applicationWillTerminate:(UIApplication *)application
{
    AzFramework::AppleTVLifecycleEvents::Bus::Broadcast(
        &AzFramework::AppleTVLifecycleEvents::Bus::Events::OnWillTerminate);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
    AzFramework::AppleTVLifecycleEvents::Bus::Broadcast(
        &AzFramework::AppleTVLifecycleEvents::Bus::Events::OnDidReceiveMemoryWarning);
}

@end // LumberyardApplicationDelegate_AppleTV Implementation
