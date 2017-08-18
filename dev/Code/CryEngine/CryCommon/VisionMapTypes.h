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

#ifndef CRYINCLUDE_CRYCOMMON_VISIONMAPTYPES_H
#define CRYINCLUDE_CRYCOMMON_VISIONMAPTYPES_H
#pragma once

// This must not exceed 31
enum VisionMapTypes
{
    General           = 1 << 0,
    AliveAgent        = 1 << 1,
    DeadAgent         = 1 << 2,
    Player            = 1 << 3,
    Interesting       = 1 << 4,
    SearchSpots       = 1 << 5,
};

#endif // CRYINCLUDE_CRYCOMMON_VISIONMAPTYPES_H
