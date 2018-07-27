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

#ifndef CRYINCLUDE_CRYAISYSTEM_FACTIONS_FACTIONMAP_H
#define CRYINCLUDE_CRYAISYSTEM_FACTIONS_FACTIONMAP_H
#pragma once


#include <IFactionMap.h>


class CFactionMap
    : public IFactionMap
{
public:
    enum
    {
        MaxFactionCount = 32,
    };

    CFactionMap();

    virtual uint32 GetFactionCount() const;
    virtual const char* GetFactionName(uint8 faction) const;
    virtual uint8 GetFactionID(const char* name) const;

    void Clear();
    void Reload();

    bool LoadConfig(const char* fileName);

    uint8 CreateFaction(const char* factionName);

    void SetReaction(uint8 factionOne, uint8 factionTwo, IFactionMap::ReactionType reaction);
    IFactionMap::ReactionType GetReaction(uint8 factionOne, uint8 factionTwo) const;

    void Serialize(TSerialize ser);

private:
    bool GetReactionType(const char* reactionName, ReactionType* reactionType) const;
    uint8 m_reactions[MaxFactionCount][MaxFactionCount];

    typedef AZStd::unordered_map<uint8, string> FactionNames;
    FactionNames m_names;

    typedef AZStd::unordered_map<string, uint8, stl::hash_string_caseless<string>, stl::equality_string_caseless<string> > FactionIds;
    FactionIds m_ids;

    typedef std::vector<string> FileNames;
    FileNames m_fileNames;
};

#endif // CRYINCLUDE_CRYAISYSTEM_FACTIONS_FACTIONMAP_H
