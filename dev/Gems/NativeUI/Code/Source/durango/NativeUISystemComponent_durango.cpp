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

#include "NativeUI_precompiled.h"

#include <NativeUISystemComponent.h>

#include <AzCore/std/parallel/binary_semaphore.h>
#include <agile.h>
#include <ppltasks.h>

namespace
{  
    Platform::String^ ConvertStringToPlatformString(const AZStd::string& str)
    {
        AZStd::wstring w_str = AZStd::wstring(str.begin(), str.end());
        const wchar_t* wChar = w_str.c_str();
        return (ref new Platform::String(wChar));
    }
}

namespace NativeUI
{
    AZStd::string SystemComponent::DisplayBlockingDialog(const AZStd::string& title, const AZStd::string& message, const AZStd::vector<AZStd::string>& options) const
    {
        if (options.size() > 3)
        {
            AZ_Assert(false, "Cannot create dialog box with more than 3 buttons");
            return "";
        }

        Platform::String^ messageStr = ConvertStringToPlatformString(message);
        Platform::String^ titleStr = ConvertStringToPlatformString(title);
        Windows::UI::Popups::MessageDialog^ msg = ref new Windows::UI::Popups::MessageDialog(messageStr, titleStr);

        for (auto& currOption : options)
        {
            Platform::String^ label = ConvertStringToPlatformString(currOption);
            Windows::UI::Popups::UICommand^ command = ref new Windows::UI::Popups::UICommand(label);
            msg->Commands->Append(command);
        }

        msg->DefaultCommandIndex = 0;
        msg->CancelCommandIndex = 0;

        AZStd::binary_semaphore selectionComplete;
        AZStd::string buttonChosen = "";
        Concurrency::create_task(msg->ShowAsync()).then([&](Windows::UI::Popups::IUICommand^ command)
        {
            AZStd::wstring commandStr(command->Label->Data());
            buttonChosen = AZStd::string(commandStr.begin(), commandStr.end());
            selectionComplete.release();
        });

        selectionComplete.acquire();

        return buttonChosen;
    }
}
