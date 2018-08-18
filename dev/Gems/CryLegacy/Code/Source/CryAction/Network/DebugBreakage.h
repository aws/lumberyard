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

#ifndef CRYINCLUDE_CRYACTION_NETWORK_DEBUGBREAKAGE_H
#define CRYINCLUDE_CRYACTION_NETWORK_DEBUGBREAKAGE_H
#pragma once

// Enable logging of breakage events and breakage serialisation
#define DEBUG_NET_BREAKAGE 0


#ifndef XYZ
#define XYZ(v) (v).x, (v).y, (v).z
#endif


#if !defined(_RELEASE) && DEBUG_NET_BREAKAGE

#include "StringUtils.h"

#define LOGBREAK(...) CryLogAlways("brk: " __VA_ARGS__)

static void LOGBREAK_STATOBJ(IStatObj* pObj)
{
    if (pObj)
    {
        unsigned int breakable = pObj->GetBreakableByGame();
        int idBreakable = pObj->GetIDMatBreakable();
        Vec3 bbmin = pObj->GetBoxMin();
        Vec3 bbmax = pObj->GetBoxMax();
        Vec3 vegCentre = pObj->GetVegCenter();
        const char* filePath = pObj->GetFilePath();
        const char* geoName = pObj->GetGeoName();
        int subObjCount = pObj->GetSubObjectCount();
        LOGBREAK("StatObj: bbmin = (%f, %f, %f) bbmax = (%f, %f, %f)", XYZ(bbmin), XYZ(bbmax));
        LOGBREAK("StatObj: vegCentre = (%f, %f, %f)", XYZ(vegCentre));
        LOGBREAK("StatObj: breakable = %d, idBreakable = %d, subObjCount = %d", breakable, idBreakable, subObjCount);
        LOGBREAK("StatObj: filePath = '%s', geoName = '%s'", filePath, geoName);
        LOGBREAK("StatObj: filePathHash = %d", CryStringUtils::CalculateHash(filePath));
    }
}

#else

#define LOGBREAK(...)
#define LOGBREAK_STATOBJ(x)

#endif


#endif // CRYINCLUDE_CRYACTION_NETWORK_DEBUGBREAKAGE_H
