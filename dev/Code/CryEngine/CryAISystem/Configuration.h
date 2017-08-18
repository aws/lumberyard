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

// Description : Simple struct for storing fundamental AI system settings

#ifndef CRYINCLUDE_CRYAISYSTEM_CONFIGURATION_H
#define CRYINCLUDE_CRYAISYSTEM_CONFIGURATION_H
#pragma once

// These values should not change and are carefully chosen but arbitrary
enum EConfigCompatibilityMode
{
    ECCM_NONE = 0,
    ECCM_CRYSIS = 2,
    ECCM_GAME04 = 4,
    ECCM_WARFACE = 7,
    ECCM_CRYSIS2 = 8
};


struct SConfiguration
{
    SConfiguration()
    {
    }

    EConfigCompatibilityMode eCompatibilityMode;

    // Should probably include logging and debugging flags
};


#endif // CRYINCLUDE_CRYAISYSTEM_CONFIGURATION_H
