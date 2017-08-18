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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_MONITOR_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_MONITOR_H
#pragma once

#include <QMutex>

class Monitor
{
public:
    class LockHandleInitializer
    {
        friend class Monitor;
        friend class LockHandle;
    public:
        QMutex* pCriticalSection;
    private:
        LockHandleInitializer();
        LockHandleInitializer(const LockHandleInitializer&);
        LockHandleInitializer& operator=(const LockHandleInitializer&);
    };

public:
    class LockHandle
    {
    public:
        ~LockHandle();
        LockHandle(const LockHandleInitializer& init);

    private:
        LockHandle();
        LockHandle& operator=(const LockHandle&);

        QMutex* pCriticalSection;
    };

    Monitor();
    virtual ~Monitor();

protected:
    LockHandleInitializer Lock();

private:
    QMutex criticalSection;
};

#define LOCK_MONITOR() LockHandle lh = this->Lock()
#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_MONITOR_H
