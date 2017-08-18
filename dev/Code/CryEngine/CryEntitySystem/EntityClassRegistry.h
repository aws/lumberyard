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

#ifndef CRYINCLUDE_CRYENTITYSYSTEM_ENTITYCLASSREGISTRY_H
#define CRYINCLUDE_CRYENTITYSYSTEM_ENTITYCLASSREGISTRY_H
#pragma once

#include <CryListenerSet.h>
#include <IEntityClass.h>

//////////////////////////////////////////////////////////////////////////
// Description:
//    Standard implementation of the IEntityClassRegistry interface.
//////////////////////////////////////////////////////////////////////////
class CEntityClassRegistry
    : public IEntityClassRegistry
{
public:
    CEntityClassRegistry();
    ~CEntityClassRegistry();

    bool RegisterEntityClass(IEntityClass* pClass) override;
    bool UnregisterEntityClass(IEntityClass* pClass) override;

    IEntityClass* FindClass(const char* sClassName) const;
    IEntityClass* GetDefaultClass() const;

    IEntityClass* RegisterStdClass(const SEntityClassDesc& entityClassDesc);

    void RegisterListener(IEntityClassRegistryListener* pListener);
    void UnregisterListener(IEntityClassRegistryListener* pListener);

    void LoadClasses(const char* sRootPath, bool bOnlyNewClasses = false);

    //////////////////////////////////////////////////////////////////////////
    // Registry iterator.
    //////////////////////////////////////////////////////////////////////////
    void IteratorMoveFirst();
    IEntityClass* IteratorNext();
    int GetClassCount() const { return m_mapClassName.size(); };

    void InitializeDefaultClasses();

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
        pSizer->AddObject(m_pDefaultClass);
        pSizer->AddContainer(m_mapClassName);
    }

private:
    void LoadClassDescription(XmlNodeRef& root, bool bOnlyNewClasses);

    void NotifyListeners(EEntityClassRegistryEvent event, const IEntityClass* pEntityClass);

    typedef std::map<string, IEntityClass*> ClassNameMap;
    ClassNameMap m_mapClassName;

    IEntityClass* m_pDefaultClass;

    ISystem* m_pSystem;
    ClassNameMap::iterator m_currentMapIterator;

    typedef CListenerSet<IEntityClassRegistryListener*> TListenerSet;
    TListenerSet    m_listeners;
};



#endif // CRYINCLUDE_CRYENTITYSYSTEM_ENTITYCLASSREGISTRY_H

