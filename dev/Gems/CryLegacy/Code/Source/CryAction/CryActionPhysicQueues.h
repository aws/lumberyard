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

#ifndef CRYINCLUDE_CRYACTION_CRYACTIONPHYSICQUEUES_H
#define CRYINCLUDE_CRYACTION_CRYACTIONPHYSICQUEUES_H
#pragma once

#include <RayCastQueue.h>
#include <IntersectionTestQueue.h>

class CCryActionPhysicQueues
{
public:
    typedef RayCastQueue<41> CryActionRayCaster;
    typedef IntersectionTestQueue<43> CryActionIntersectionTester;

    CCryActionPhysicQueues()
    {
        m_rayCaster.SetQuota(8);
        m_intersectionTester.SetQuota(6);
    }

    CryActionRayCaster& GetRayCaster() { return m_rayCaster; }
    CryActionIntersectionTester& GetIntersectionTester() { return m_intersectionTester; }

    void Update(float frameTime)
    {
        m_rayCaster.Update(frameTime);
        m_intersectionTester.Update(frameTime);
    }

private:
    CryActionRayCaster  m_rayCaster;
    CryActionIntersectionTester m_intersectionTester;
};

#endif // CRYINCLUDE_CRYACTION_CRYACTIONPHYSICQUEUES_H
