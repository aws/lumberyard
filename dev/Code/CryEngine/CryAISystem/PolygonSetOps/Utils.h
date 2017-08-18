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

#ifndef CRYINCLUDE_CRYAISYSTEM_POLYGONSETOPS_UTILS_H
#define CRYINCLUDE_CRYAISYSTEM_POLYGONSETOPS_UTILS_H
#pragma once

#include "ISystem.h"

typedef Vec2d Vector2d;

// needed to use Vector2d in a map
template<class F>
bool operator<(const Vec2_tpl<F>& op1, const Vec2_tpl<F>& op2)
{
    if (op1.x < op2.x)
    {
        return true;
    }
    else if (op1.x > op2.x)
    {
        return false;
    }
    if (op1.y < op2.y)
    {
        return true;
    }
    else
    {
        return false;
    }
}


#endif // CRYINCLUDE_CRYAISYSTEM_POLYGONSETOPS_UTILS_H

