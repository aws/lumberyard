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

// Description : Pool definition to associate an id with settings


#ifndef CRYINCLUDE_CRYENTITYSYSTEM_ENTITYPOOLDEFINITION_H
#define CRYINCLUDE_CRYENTITYSYSTEM_ENTITYPOOLDEFINITION_H
#pragma once


#include "EntityPoolCommon.h"

class CEntityPoolDefinition
{
public:
    CEntityPoolDefinition(TEntityPoolDefinitionId id = INVALID_ENTITY_POOL_DEFINITION);
    void GetMemoryStatistics(ICrySizer* pSizer) const;

    void OnLevelLoadStart();

    TEntityPoolDefinitionId GetId() const { return m_Id; }
    bool HasAI() const { return m_bHasAI; }
    bool IsDefaultBookmarked() const { return m_bDefaultBookmarked; }
    bool IsForcedBookmarked() const { return m_bForcedBookmarked; }
    uint32 GetMaxSize() const { return m_uMaxSize; }
    uint32 GetDesiredPoolCount() const { return m_uCount; }
    const string& GetName() const { return m_sName; }
    const string& GetDefaultClass() const { return m_sDefaultClass; }

    bool ContainsEntityClass(IEntityClass* pClass) const;

    bool LoadFromXml(CEntitySystem* pEntitySystem, XmlNodeRef pDefinitionNode);
    bool ParseLevelXml(XmlNodeRef pLevelNode);

#ifdef ENTITY_POOL_SIGNATURE_TESTING
    bool TestEntityClassSignature(CEntitySystem* pEntitySystem, IEntityClass* pClass, const class CEntityPoolSignature& poolSignature) const;
#endif //ENTITY_POOL_SIGNATURE_TESTING

#ifdef ENTITY_POOL_DEBUGGING
    void DebugDraw(IRenderer* pRenderer, float& fColumnX, float& fColumnY) const;
#endif //ENTITY_POOL_DEBUGGING

private:
    TEntityPoolDefinitionId m_Id;

    bool m_bHasAI;
    bool m_bDefaultBookmarked;
    bool m_bForcedBookmarked;
    uint32 m_uMaxSize;
    uint32 m_uCount;
    string m_sName;
    string m_sDefaultClass;

    typedef std::vector<IEntityClass*> TContainClasses;
    TContainClasses m_ContainClasses;

#ifdef ENTITY_POOL_SIGNATURE_TESTING
    typedef std::map<IEntityClass*, bool> TSignatureTests;
    mutable TSignatureTests m_SignatureTests;
#endif //ENTITY_POOL_SIGNATURE_TESTING
};

#endif // CRYINCLUDE_CRYENTITYSYSTEM_ENTITYPOOLDEFINITION_H
