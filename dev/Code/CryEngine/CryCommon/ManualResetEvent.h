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

#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/condition_variable.h>

class ManualResetEvent
{
public:
    ManualResetEvent()
        : m_flag(false),
        m_mutex(),
        m_conditionVariable()
    {
    }

    void Wait()
    {
        AZStd::unique_lock<AZStd::mutex> lock(m_mutex);
        m_conditionVariable.wait(lock, [this]
        {
            return m_flag == true;
        });
    }

    void Set()
    {
        {
            AZStd::unique_lock<AZStd::mutex> lock(m_mutex);
            m_flag = true;
        }

        m_conditionVariable.notify_all();
    }

    void Unset()
    {
        AZStd::unique_lock<AZStd::mutex> lock(m_mutex);
        m_flag = false;
    }

    bool IsSet() const
    {
        AZStd::unique_lock<AZStd::mutex> lock(m_mutex);
        return m_flag;
    }

private:
    bool m_flag;
    mutable AZStd::mutex m_mutex;
    AZStd::condition_variable m_conditionVariable;
};