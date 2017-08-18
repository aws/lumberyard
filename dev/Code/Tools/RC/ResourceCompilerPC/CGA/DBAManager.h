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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_DBAMANAGER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_DBAMANAGER_H
#pragma once



struct IPakSystem;
class ICryXML;

class DBAManagerEntry
{
public:
    const string& GetPath() const { return m_path; }
    void SetPath(const string& path) { m_path = path; }

    size_t GetAnimationCount() const { return m_animations.size(); }
    const string& GetAnimationPath(const size_t index) const { return m_animations[index]; }
private:
    string m_path;
    std::vector<string> m_animations;
    friend class DBAManager;
};

// DBAManager stores list of DBA entries loaded from DBATable.xml.
// It is an obsolete way of storing animation data so it is superseded by
// SDBATable, which uses AnimationFilter to filter animations by rules rather
// than plain lists.
class DBAManager
{
public:
    DBAManager(IPakSystem* pPakSystem, ICryXML* pXmlParser);

    bool LoadDBATable(const string& filename);

    size_t GetDBACount() const { return m_dbaEntries.size(); }
    const DBAManagerEntry& GetDBAEntry(const size_t index) const { return m_dbaEntries[index]; }

    const char* FindDBAPath(const char* const animationName) const;

    const string& GetDBATablePath() const { return m_dbaTablePath; }

private:
    IPakSystem* m_pPakSystem;
    ICryXML* m_pXmlParser;

    string m_dbaTablePath;

    typedef std::map<string, size_t> AnimationNameToDBAIndexMap;
    AnimationNameToDBAIndexMap m_animationToDbaIndex;

    std::vector<DBAManagerEntry> m_dbaEntries;
};


#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_DBAMANAGER_H
