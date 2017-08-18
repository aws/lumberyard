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

#pragma once
#include "ManualResetEvent.h"

class UniqueManualEvent
{
public:
    UniqueManualEvent(ManualResetEvent* manualResetEvent, bool hasControl)
        : m_manualResetEvent(manualResetEvent),
        m_hasControl(hasControl)
    {

    }

    //! Indicates if the current thread has control of the event and is blocking other threads from proceeding
    bool HasControl() const
    {
        return m_hasControl;
    }

    void Set()
    {
        if (m_hasControl)
        {
            m_manualResetEvent->Set();
            m_hasControl = false;
        }
    }

    ~UniqueManualEvent()
    {
        Set();
    }

private:
    bool m_hasControl;
    ManualResetEvent* m_manualResetEvent;
};