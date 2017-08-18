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

#ifndef CRYINCLUDE_CRYACTION_NETWORK_NETWORKCVARS_H
#define CRYINCLUDE_CRYACTION_NETWORK_NETWORKCVARS_H
#pragma once

#include "IConsole.h"

class CNetworkCVars
{
public:
    int BreakageLog;
    float VoiceVolume;
    float PhysSyncPingSmooth;
    float PhysSyncLagSmooth;
    int PhysDebug;
    int BreakTimeoutFrames;
    float   BreakMaxWorldSize;
    float BreakWorldOffsetX;
    float BreakWorldOffsetY;
    int sv_LoadAllLayersForResList;

    static ILINE CNetworkCVars& Get()
    {
        CRY_ASSERT(s_pThis);
        return *s_pThis;
    }

private:
    friend class CCryAction; // Our only creator

    CNetworkCVars(); // singleton stuff
    ~CNetworkCVars();
    CNetworkCVars(const CNetworkCVars&);
    CNetworkCVars& operator= (const CNetworkCVars&);

    static CNetworkCVars* s_pThis;
};

#endif // CRYINCLUDE_CRYACTION_NETWORK_NETWORKCVARS_H
