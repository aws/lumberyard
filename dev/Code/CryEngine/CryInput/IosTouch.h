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
#ifndef CRYINCLUDE_CRYINPUT_IOSTOUCH_H
#define CRYINCLUDE_CRYINPUT_IOSTOUCH_H
#pragma once

#include "MobileTouch.h"

#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_ios.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class CIosTouch
    : public CMobileTouch
    , public AzFramework::RawInputNotificationBusIos::Handler
{
public:
    CIosTouch(IInput& input);
    ~CIosTouch() override;

protected:
    void OnRawTouchEventBegan(const UITouch* uiTouch) override;
    void OnRawTouchEventMoved(const UITouch* uiTouch) override;
    void OnRawTouchEventEnded(const UITouch* uiTouch) override;
    TouchEvent InitTouchEvent(const UITouch* uiTouch, uint32_t index, TouchEvent::State state);

private:
    AZStd::vector<const UITouch*> m_activeTouches;
};

#endif // CRYINCLUDE_CRYINPUT_IOSTOUCH_H
