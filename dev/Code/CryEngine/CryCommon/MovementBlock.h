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

#ifndef __MOVEMENTBLOCK_H__
#define __MOVEMENTBLOCK_H__

#include <IPathfinder.h>

struct IMovementActor;
struct MovementUpdateContext;
class MovementStyle;

namespace Movement
{
    struct Block
    {
        enum Status
        {
            Running,
            Finished,
            CantBeFinished,
        };

        // Allocate and deallocate memory in the ctor and dtor only.
        // End will not be called when resetting the movement system
        // as the entities have been erased at that point and a
        // healthy movement actor cannot be passed in.
        virtual ~Block() {}
        virtual void Begin(IMovementActor& actor) {}
        virtual void End(IMovementActor& actor) {}
        virtual Status Update(const MovementUpdateContext& context) { return Finished; }
        virtual bool InterruptibleNow() const { return false; }
        virtual const char* GetName() const = 0;
    };

    typedef AZStd::shared_ptr<Movement::Block> BlockPtr;
    typedef Functor3wRet<const INavPath&, const PathPointDescriptor::OffMeshLinkData&, const MovementStyle&, BlockPtr> CustomNavigationBlockCreatorFunction;
}

#endif // __MOVEMENTBLOCK_H__