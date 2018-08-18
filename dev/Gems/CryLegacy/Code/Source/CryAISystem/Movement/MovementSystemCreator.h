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

#ifndef CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTSYSTEMCREATOR_H
#define CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTSYSTEMCREATOR_H
#pragma once

// The creator is used as a middle step, so that the code that wants
// to create the movement system doesn't necessarily have to include
// the movement system header and be recompiled every time it changes.
class MovementSystemCreator
{
public:
    struct IMovementSystem* CreateMovementSystem() const;
};

#endif // CRYINCLUDE_CRYAISYSTEM_MOVEMENT_MOVEMENTSYSTEMCREATOR_H
