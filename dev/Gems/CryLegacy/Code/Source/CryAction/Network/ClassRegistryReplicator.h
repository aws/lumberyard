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

#ifndef CRYINCLUDE_CRYACTION_NETWORK_CLASSREGISTRYREPLICATOR_H
#define CRYINCLUDE_CRYACTION_NETWORK_CLASSREGISTRYREPLICATOR_H
#pragma once

class CClassRegistryReplicator
{
public:
    CClassRegistryReplicator() { Reset(); }

    bool RegisterClassName(const string& name, uint16 id);
    bool ClassNameFromId(string& name, uint16 id) const;
    bool ClassIdFromName(uint16& id, const string& name) const;
    size_t NumClassIds() { return m_classIdToName.size(); }
    void Reset();
    uint32 GetHash();
    void DumpClasses();

    void GetMemoryStatistics(ICrySizer* s) const;

private:
    uint16 m_numClassIds;
    std::vector<string> m_classIdToName;
    std::map<string, uint16> m_classNameToId;
};

#endif // CRYINCLUDE_CRYACTION_NETWORK_CLASSREGISTRYREPLICATOR_H
