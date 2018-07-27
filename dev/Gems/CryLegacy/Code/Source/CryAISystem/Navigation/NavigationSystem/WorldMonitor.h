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

#ifndef CRYINCLUDE_CRYAISYSTEM_NAVIGATION_NAVIGATIONSYSTEM_WORLDMONITOR_H
#define CRYINCLUDE_CRYAISYSTEM_NAVIGATION_NAVIGATIONSYSTEM_WORLDMONITOR_H
#pragma once

class WorldMonitor
{
public:
    typedef Functor1<const AABB&> Callback;

    WorldMonitor();
    WorldMonitor(const Callback& callback);

    void Start();
    void Stop();

    static int StateChangeHandler(const EventPhys* pPhysEvent);
    static int EntityRemovedHandler(const EventPhys* pPhysEvent);

protected:

    bool IsEnabled() const;

    Callback m_callback;

    bool     m_enabled;
};

#endif // CRYINCLUDE_CRYAISYSTEM_NAVIGATION_NAVIGATIONSYSTEM_WORLDMONITOR_H
