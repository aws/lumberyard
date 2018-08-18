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


#include "CryLegacy_precompiled.h"
#include "EntityPoolDefinition.h"
#include "EntitySystem.h"
#include "EntityClassRegistry.h"

#ifdef ENTITY_POOL_SIGNATURE_TESTING
    #include "Entity.h"
    #include "EntityPoolSignature.h"
#endif //ENTITY_POOL_SIGNATURE_TESTING

//////////////////////////////////////////////////////////////////////////
CEntityPoolDefinition::CEntityPoolDefinition(TEntityPoolDefinitionId id)
    : m_Id(id)
    , m_bHasAI(false)
    , m_bDefaultBookmarked(false)
    , m_bForcedBookmarked(false)
    , m_uMaxSize(0)
    , m_uCount(0)
{
}

//////////////////////////////////////////////////////////////////////////
void CEntityPoolDefinition::GetMemoryStatistics(ICrySizer* pSizer) const
{
    pSizer->Add(*this);
    pSizer->AddContainer(m_ContainClasses);

#ifdef ENTITY_POOL_SIGNATURE_TESTING
    pSizer->AddContainer(m_SignatureTests);
#endif //ENTITY_POOL_SIGNATURE_TESTING
}

//////////////////////////////////////////////////////////////////////////
void CEntityPoolDefinition::OnLevelLoadStart()
{
    // Start at the max size of the definition. The level may adjust it based on its own settings.
    m_uCount = m_uMaxSize;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityPoolDefinition::ContainsEntityClass(IEntityClass* pClass) const
{
    return stl::find(m_ContainClasses, pClass);
}

//////////////////////////////////////////////////////////////////////////
bool CEntityPoolDefinition::LoadFromXml(CEntitySystem* pEntitySystem, XmlNodeRef pDefinitionNode)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);

    assert(pEntitySystem);
    assert(pDefinitionNode);

    bool bResult = false;

    IEntityClassRegistry* pClassRegistry = pEntitySystem->GetClassRegistry();
    assert(pClassRegistry);

    if (pDefinitionNode)
    {
        m_sName = pDefinitionNode->getAttr("name");
        if (m_sName.empty())
        {
            m_sName = "<Unknown>";
        }

        m_sDefaultClass = pDefinitionNode->getAttr("emptyClass");
        bResult = !(m_sDefaultClass.empty());
        if (!bResult)
        {
            EntityWarning("[Entity Pool] LoadFromXml: \'emptyClass\' is empty or missing.");
        }

        pDefinitionNode->getAttr("hasAI", m_bHasAI);
        pDefinitionNode->getAttr("maxSize", m_uMaxSize);
        pDefinitionNode->getAttr("defaultBookmarked", m_bDefaultBookmarked);
        pDefinitionNode->getAttr("forcedBookmarked", m_bForcedBookmarked);

        XmlNodeRef pContainsNode = pDefinitionNode->findChild("Contains");
        if (bResult && pContainsNode)
        {
            const int iChildCount = pContainsNode->getChildCount();
            m_ContainClasses.reserve(iChildCount);

            bResult = (iChildCount > 0);
            if (!bResult)
            {
                EntityWarning("[Entity Pool] LoadFromXml: \'Contains\' node is empty. You need to define at least one valid class!");
            }

            for (int iChild = 0; bResult && iChild < iChildCount; ++iChild)
            {
                XmlNodeRef pClassNode = pContainsNode->getChild(iChild);
                if (!pClassNode || !pClassNode->isTag("Class"))
                {
                    continue;
                }

                const char* szClass = pClassNode->getContent();
                IEntityClass* pClass = pClassRegistry->FindClass(szClass);
                if (pClass)
                {
                    m_ContainClasses.push_back(pClass);
                }
                else
                {
                    EntityWarning("[Entity Pool] LoadFromXml: Class \'%s\' is not a valid Entity Class.", szClass);
                }
            }
        }
        else
        {
            EntityWarning("[Entity Pool] LoadFromXml: \'Contains\' node is empty or missing. You need to define at least one valid class!");
        }
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityPoolDefinition::ParseLevelXml(XmlNodeRef pLevelNode)
{
    assert(pLevelNode && pLevelNode->isTag(m_sName.c_str()));

    uint32 uLevelCount = 0;
    const bool bHasAttr = pLevelNode->getAttr("count", uLevelCount);
    if (bHasAttr)
    {
        m_uCount = min(uLevelCount, m_uMaxSize);
    }

    return bHasAttr;
}

#ifdef ENTITY_POOL_SIGNATURE_TESTING

//////////////////////////////////////////////////////////////////////////
bool CEntityPoolDefinition::TestEntityClassSignature(CEntitySystem* pEntitySystem, IEntityClass* pClass, const CEntityPoolSignature& poolSignature) const
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);

    assert(pEntitySystem);
    assert(pClass);

    // Check if already tested
    TSignatureTests::const_iterator itResult = m_SignatureTests.find(pClass);
    if (itResult != m_SignatureTests.end())
    {
        return itResult->second;
    }

    bool bResult = ContainsEntityClass(pClass);

    if (bResult && CVar::es_TestPoolSignatures != 0)
    {
        bResult = false;

        // Instantiate an entity of this class type and build its signature, then compare it against the given signature
        SEntitySpawnParams testSpawnParams;
        testSpawnParams.pClass = pClass;
        IEntity* pTestEntity = pEntitySystem->SpawnEntity(testSpawnParams);
        assert(pTestEntity);

        if (pTestEntity)
        {
            CEntityPoolSignature entitySignature;
            bResult = (entitySignature.CalculateFromEntity(pTestEntity) && entitySignature == poolSignature);

            // Store result
            m_SignatureTests[pClass] = bResult;

            pEntitySystem->RemoveEntity(pTestEntity->GetId(), true);
        }
    }

    return bResult;
}

#endif //ENTITY_POOL_SIGNATURE_TESTING

#ifdef ENTITY_POOL_DEBUGGING

//////////////////////////////////////////////////////////////////////////
void CEntityPoolDefinition::DebugDraw(IRenderer* pRenderer, float& fColumnX, float& fColumnY) const
{
    assert(pRenderer);

    const float colWhite[] = {1.0f, 1.0f, 1.0f, 1.0f};
    const float colGreen[]  = {0.0f, 1.0f, 0.0f, 1.0f};
    const float colRed[]    = {1.0f, 0.0f, 0.0f, 1.0f};

    string sDebugName;
    sDebugName.Format("%s (Max: %u)%s", m_sName.c_str(), m_uMaxSize, m_bForcedBookmarked ? " [FORCED]" : "");

    pRenderer->Draw2dLabel(fColumnX + 15.0f, fColumnY, 1.0f, m_bForcedBookmarked ? colRed : colWhite, false, sDebugName.c_str());
    fColumnY += 12.0f;

    // Output current validity tests for the classes
    TContainClasses::const_iterator itClass = m_ContainClasses.begin();
    TContainClasses::const_iterator itClassEnd = m_ContainClasses.end();
    for (; itClass != itClassEnd; ++itClass)
    {
        const IEntityClass* pClass = *itClass;
        if (!pClass)
        {
            continue;
        }

        // Check if valid, not valid or not tested
        const float* pColor = colWhite;
#ifdef ENTITY_POOL_SIGNATURE_TESTING
        TSignatureTests::const_iterator itResult = m_SignatureTests.find(const_cast<IEntityClass*>(pClass));
        if (itResult != m_SignatureTests.end())
        {
            pColor = (itResult->second == true ? colGreen : colRed);
        }
#endif //ENTITY_POOL_SIGNATURE_TESTING

        pRenderer->Draw2dLabel(fColumnX + 17.5f, fColumnY, 0.8f, pColor, false, pClass->GetName());
        fColumnY += 10.0f;
    }
}

#endif //ENTITY_POOL_DEBUGGING
