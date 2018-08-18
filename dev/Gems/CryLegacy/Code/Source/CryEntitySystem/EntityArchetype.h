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

#ifndef CRYINCLUDE_CRYENTITYSYSTEM_ENTITYARCHETYPE_H
#define CRYINCLUDE_CRYENTITYSYSTEM_ENTITYARCHETYPE_H
#pragma once

#include <IScriptSystem.h>

class CEntityClass;

//////////////////////////////////////////////////////////////////////////
class CEntityArchetype
    : public IEntityArchetype
    , public _i_reference_target_t
{
public:
    CEntityArchetype(IEntityClass* pClass);

    //////////////////////////////////////////////////////////////////////////
    // IEntityArchetype
    //////////////////////////////////////////////////////////////////////////
    virtual IEntityClass* GetClass() const { return m_pClass; };
    const char* GetName() const { return m_name.c_str(); };
    IScriptTable* GetProperties() { return m_pProperties; };
    XmlNodeRef GetObjectVars() { return m_ObjectVars; };
    void LoadFromXML(XmlNodeRef& propertiesNode, XmlNodeRef& objectVarsNode);
    //////////////////////////////////////////////////////////////////////////

    void SetName(const string& sName) { m_name = sName; };

private:
    string m_name;
    SmartScriptTable m_pProperties;
    XmlNodeRef m_ObjectVars;
    IEntityClass* m_pClass;
};

//////////////////////////////////////////////////////////////////////////
// Manages collection of the entity archetypes.
//////////////////////////////////////////////////////////////////////////
class CEntityArchetypeManager
{
public:
    IEntityArchetype* CreateArchetype(IEntityClass* pClass, const char* sArchetype);
    IEntityArchetype* FindArchetype(const char* sArchetype);
    IEntityArchetype* LoadArchetype(const char* sArchetype);

    void Reset();

private:
    bool LoadLibrary(const string& library);
    string GetLibraryFromName(const string& sArchetypeName);

    typedef std::map<const char*, _smart_ptr<CEntityArchetype>, stl::less_stricmp<const char*> > ArchetypesNameMap;
    ArchetypesNameMap m_nameToArchetypeMap;

    DynArray<string> m_loadedLibs;
};

#endif // CRYINCLUDE_CRYENTITYSYSTEM_ENTITYARCHETYPE_H
