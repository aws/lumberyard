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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYCOMMON_THREADPROFILERSUPPORT_H
#define CRYINCLUDE_CRYCOMMON_THREADPROFILERSUPPORT_H
#pragma once


#define ENABLE_THREADPROFILERSUPPORT 0

#if ENABLE_THREADPROFILERSUPPORT

#include "libittnotify.h"

#pragma comment( lib, "libittnotify" )

class CThreadProfilerEvent
{
public:
    CThreadProfilerEvent(const char* name)
        : m_event(__itt_event_create(const_cast<char*>(name), strlen(name))) {}

    class Instance
    {
    public:
        ILINE Instance(const CThreadProfilerEvent& evt, bool enter = true)
            : m_event(evt.m_event)
            , m_enter(enter)
        {
            if (m_enter)
            {
                __itt_event_start(m_event);
            }
        }
        ILINE ~Instance()
        {
            if (m_enter)
            {
                __itt_event_end(m_event);
            }
        }

    private:
        Instance(const Instance&);
        Instance& operator=(const Instance&);

        __itt_event m_event;
        bool m_enter;
    };

private:
    __itt_event m_event;
};

#else // ENABLE_THREADPROFILERSUPPORT

class CThreadProfilerEvent
{
public:
    ILINE CThreadProfilerEvent(const char*) {}
    class Instance
    {
    public:
        ILINE Instance(const CThreadProfilerEvent&, bool enter = true) {}

    private:
        Instance(const Instance&);
        Instance& operator=(const Instance&);
    };
};

#endif // ENABLE_THREADPROFILERSUPPORT

#endif // CRYINCLUDE_CRYCOMMON_THREADPROFILERSUPPORT_H
