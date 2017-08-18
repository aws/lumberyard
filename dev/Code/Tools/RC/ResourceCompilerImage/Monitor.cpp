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

#include <platform.h>

#include <stdafx.h>
#include "Monitor.h"

Monitor::Monitor()
    : criticalSection(QMutex::Recursive)
{
}

Monitor::~Monitor()
{
}

Monitor::LockHandleInitializer Monitor::Lock()
{
    LockHandleInitializer lock;

    // This code checks to see whether this is null - this is because in one application
    // (at the time of writing the only application) of this class was in a window proc
    // that used GetWindowLong to retrieve the instance pointer, which was sometimes
    // uninitialized. It just is easier to handle this situation here.
    if (this != 0)
    {
        lock.pCriticalSection = &this->criticalSection;
    }
    return lock;
}

Monitor::LockHandleInitializer::LockHandleInitializer()
    :   pCriticalSection(0)
{
}

Monitor::LockHandleInitializer::LockHandleInitializer(const LockHandleInitializer& other)
{
    this->pCriticalSection = other.pCriticalSection;
}

Monitor::LockHandle::LockHandle(const LockHandleInitializer& init)
{
    this->pCriticalSection = init.pCriticalSection;
    if (this->pCriticalSection)
    {
        this->pCriticalSection->lock();
    }
}

Monitor::LockHandle::~LockHandle()
{
    if (this->pCriticalSection != 0)
    {
        this->pCriticalSection->unlock();
    }
}
