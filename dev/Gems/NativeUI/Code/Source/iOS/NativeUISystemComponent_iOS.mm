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

#include <ISystem.h>
#include <NativeUISystemComponent.h>

#include <AzFramework/API/ApplicationAPI.h>

#import <UIKit/UIKit.h>

namespace NativeUI
{
    AZStd::string SystemComponent::DisplayBlockingDialog(const AZStd::string& title, const AZStd::string& message, const AZStd::vector<AZStd::string>& options) const
    {
        __block AZStd::string userSelection = "";
        
        NSString* nsTitle = [NSString stringWithUTF8String:title.c_str()];
        NSString* nsMessage = [NSString stringWithUTF8String:message.c_str()];
        UIAlertController* alert = [UIAlertController alertControllerWithTitle:nsTitle message:nsMessage preferredStyle:UIAlertControllerStyleAlert];
        
        for (int i = 0; i < options.size(); i++)
        {
            UIAlertAction* okAction = [UIAlertAction actionWithTitle:[NSString stringWithUTF8String:options[i].c_str()] style:UIAlertActionStyleDefault handler:^(UIAlertAction* action) { userSelection = [action.title UTF8String]; }];
            [alert addAction:okAction];
        }
        
        UIWindow* window = [[UIApplication sharedApplication] keyWindow];
        UIViewController* rootViewController = window ? window.rootViewController : nullptr;
        if (!rootViewController)
        {
            return "";
        }
        
        if (gEnv->mMainThreadId != CryGetCurrentThreadId())
        {
            #if !defined(_RELEASE)
                __block dispatch_semaphore_t blockSem = dispatch_semaphore_create(0);
                // Dialog boxes need to be triggered from the main thread.
                CFRunLoopPerformBlock(CFRunLoopGetMain(), kCFRunLoopDefaultMode, ^(){
                    [rootViewController presentViewController:alert animated:YES completion:nil];
                    while (userSelection.empty())
                    {
                        AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::PumpSystemEventLoopOnce);
                    }
                    dispatch_semaphore_signal(blockSem);
                });
                // Wait till the user responds to the message box.
                dispatch_semaphore_wait(blockSem, DISPATCH_TIME_FOREVER);
            #endif
        }
        else
        {
            [rootViewController presentViewController:alert animated:YES completion:nil];
        }
        
        while (userSelection.empty())
        {
            AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::PumpSystemEventLoopOnce);
        }
        
        return userSelection;
    }
}
