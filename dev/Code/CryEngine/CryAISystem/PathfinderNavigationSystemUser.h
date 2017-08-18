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

// Description : This class acts as a shell to allow the mnm pathfinder to
//               correctly respects the safe update of the navigation
//               system.


#ifndef PATHFINDERNAVIGATIONSYSTEMUSER_H
#define PATHFINDERNAVIGATIONSYSTEMUSER_H

#include "INavigationSystem.h"
#include "MNMPathfinder.h"

namespace MNM
{
    class PathfinderNavigationSystemUser
        : public INavigationSystemUser
    {
    public:
        virtual ~PathfinderNavigationSystemUser() {}

        CMNMPathfinder* GetPathfinderImplementation() { return &m_pathfinder; }

        // INavigationSystemUser
        virtual void Reset() { m_pathfinder.Reset(); }
        virtual void UpdateForSynchronousOrAsynchronousReadingOperation() { m_pathfinder.Update(); }
        virtual void UpdateForSynchronousWritingOperations() {};
        virtual void CompleteRunningTasks() { m_pathfinder.WaitForJobsToFinish(); }
        // ~INavigationSystemUser

    private:
        CMNMPathfinder m_pathfinder;
    };
}

#endif // PATHFINDERNAVIGATIONSYSTEMUSER_H