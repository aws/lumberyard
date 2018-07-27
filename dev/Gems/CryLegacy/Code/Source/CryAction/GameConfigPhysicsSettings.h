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
#ifndef GAMECONFIGPHYSICSSETTINGS_H
#define GAMECONFIGPHYSICSSETTINGS_H
#pragma once

#include "GamePhysicsSettings.h"

// Loads physics settings (currently only collision class names) from a collision_classes cvar
struct GameConfigPhysicsSettings
    : public GamePhysicsSettings
{
public:

    void Init() override;

    const char* GetCollisionClassName(unsigned int bitIndex) override;

private:

    string m_collisionClasses[IGamePhysicsSettings::kNumCollisionClasses];
};

#endif