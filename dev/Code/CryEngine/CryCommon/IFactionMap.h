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

#ifndef CRYINCLUDE_CRYCOMMON_IFACTIONMAP_H
#define CRYINCLUDE_CRYCOMMON_IFACTIONMAP_H
#pragma once



struct IFactionMap
{
    enum ReactionType
    {
        Hostile = 0, // intentionally from most-hostile to most-friendly
        Neutral,
        Friendly,
    };

    enum
    {
        InvalidFactionID = 0xff,
    };

    // <interfuscator:shuffle>
    virtual ~IFactionMap(){}
    virtual uint32 GetFactionCount() const = 0;
    virtual const char* GetFactionName(uint8 fraction) const = 0;
    virtual uint8 GetFactionID(const char* name) const = 0;

    virtual void SetReaction(uint8 factionOne, uint8 factionTwo, IFactionMap::ReactionType reaction) = 0;
    virtual IFactionMap::ReactionType GetReaction(uint8 factionOne, uint8 factionTwo) const = 0;
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_IFACTIONMAP_H
