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

#include "StdAfx.h"
#include "FlowGraphVariables.h"
#include "FlowGraph.h"
#include "FlowGraphNode.h"
#include "Objects/EntityObject.h"
#include "Util/Variable.h"

#include "UIEnumsDatabase.h"

#include <IMovieSystem.h>
#include <ICryAnimation.h>
#include <IDialogSystem.h>

#include <IGameFramework.h>
#include <ICryMannequin.h>
#include <IAttachment.h>

#include <ICommunicationManager.h>
#include <IVehicleSystem.h>
#include "Components/IComponentRender.h"

namespace
{
    typedef IVariable::IGetCustomItems::SItem SItem;

    //////////////////////////////////////////////////////////////////////////
    bool ChooseDialog(IVariable* pVar, std::vector<SItem>& outItems, QString& outText)
    {
        IDialogSystem* pDS = gEnv->pDialogSystem;
        if (!pDS)
        {
            return false;
        }

        IDialogScriptIteratorPtr pIter = pDS->CreateScriptIterator();
        if (!pIter)
        {
            return false;
        }

        IDialogScriptIterator::SDialogScript script;
        while (pIter->Next(script))
        {
            SItem item;
            item.name = script.id;
            item.desc = QObject::tr("Script: ");
            item.desc += script.id;
            if (script.bIsLegacyExcel)
            {
                item.desc += QObject::tr(" [Excel]");
            }
            item.desc += QObject::tr(" - Required Actors: %1").arg(script.numRequiredActors);
            if (script.desc && script.desc[0] != '\0')
            {
                item.desc += QObject::tr("\r\nDescription: ");
                item.desc += script.desc;
            }
            outItems.push_back(item);
        }
        outText = QObject::tr("Choose Dialog");
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    bool ChooseSequence(IVariable* pVar, std::vector<SItem>& outItems, QString& outText)
    {
        IMovieSystem* pMovieSys = GetIEditor()->GetMovieSystem();
        for (int i = 0; i < pMovieSys->GetNumSequences(); ++i)
        {
            IAnimSequence* pSeq = pMovieSys->GetSequence(i);
            string seqName = pSeq->GetName();
            outItems.push_back(SItem(seqName.c_str()));
        }
        outText = QObject::tr("Choose Sequence");
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    template<typename T>
    bool GetValue(CFlowNodeGetCustomItemsBase* pGetCustomItems, const QString& portName, T& value)
    {
        CFlowNode* pFlowNode = pGetCustomItems->GetFlowNode();
        if (pFlowNode == 0)
        {
            return false;
        }

        assert (pFlowNode != 0);

        // Get the CryAction FlowGraph
        IFlowGraph* pGameFG = pFlowNode->GetIFlowGraph();

        CHyperNodePort* pPort = pFlowNode->FindPort(portName, true);
        if (!pPort)
        {
            return false;
        }
        const TFlowInputData* pFlowData = pGameFG->GetInputValue(pFlowNode->GetFlowNodeId(), pPort->nPortIndex);
        if (!pFlowData)
        {
            return false;
        }
        assert (pFlowData != 0);

        const bool success = pFlowData->GetValueWithConversion(value);
        return success;
    }

    typedef std::map<QString, QString> TParamMap;
    bool GetParamMap(CFlowNodeGetCustomItemsBase* pGetCustomItems, TParamMap& outMap)
    {
        CFlowNode* pFlowNode = pGetCustomItems->GetFlowNode();
        assert (pFlowNode != 0);

        const QString& uiConfig = pGetCustomItems->GetUIConfig();
        // fill in Enum Pairs
        const QString& values = uiConfig;

        for (auto resToken : values.split(" ,", QString::SkipEmptyParts))
        {
            QString str = resToken;
            QString value = str;
            int pos_e = str.indexOf('=');
            if (pos_e >= 0)
            {
                value = str.mid(pos_e + 1);
                str = str.mid(0, pos_e);
            }
            outMap[str] = value;
        }
        return outMap.empty() == false;
    }

    IEntity* GetRefEntityByUIConfig(CFlowNodeGetCustomItemsBase* pGetCustomItems, const QString& refEntity)
    {
        CFlowNode* pFlowNode = pGetCustomItems->GetFlowNode();
        assert (pFlowNode != 0);

        // If the node is part of an AI Action Graph we always use the selected entity
        CFlowGraph* pFlowGraph = static_cast<CFlowGraph*> (pFlowNode->GetGraph());
        if (pFlowGraph && pFlowGraph->GetAIAction())
        {
            IEntity* pSelEntity = 0;
            CBaseObject* pObject = GetIEditor()->GetSelectedObject();
            if (qobject_cast<CEntityObject*>(pObject))
            {
                CEntityObject* pCEntity = static_cast<CEntityObject*> (pObject);
                pSelEntity = pCEntity->GetIEntity();
            }
            if (pSelEntity == 0)
            {
                Warning(QObject::tr("FlowGraph: Please select first an Entity to be used as reference for this AI Action.").toUtf8().data());
            }
            return pSelEntity;
        }

        // Get the CryAction FlowGraph
        IFlowGraph* pGameFG = pGetCustomItems->GetFlowNode()->GetIFlowGraph();

        FlowEntityId targetEntityId;
        QString targetEntityPort;
        QString uiConfig = pGetCustomItems->GetUIConfig();
        int pos = uiConfig.indexOf(refEntity);
        if (pos >= 0)
        {
            targetEntityPort = uiConfig.mid(pos + refEntity.length());
        }
        else
        {
            targetEntityPort = QStringLiteral("entityId");
        }

        if (targetEntityPort == QStringLiteral("entityId"))
        {
            // use normal target entity of flownode as target
            targetEntityId = pGameFG->GetEntityId(pFlowNode->GetFlowNodeId());
            if (targetEntityId == 0)
            {
                Warning("FlowGraph: No valid Target Entity assigned to node");
                return 0;
            }
        }
        else
        {
            // use entity in port targetEntity as target
            CHyperNodePort* pPort = pFlowNode->FindPort(targetEntityPort, true);
            if (!pPort)
            {
                Warning("FlowGraphVariables.cpp: Internal error - Cannot resolve port '%s' on ref-lookup. Check C++ Code!", targetEntityPort.toUtf8().data());
                return 0;
            }
            const TFlowInputData* pFlowData = pGameFG->GetInputValue(pFlowNode->GetFlowNodeId(), pPort->nPortIndex);
            assert (pFlowData != 0);
            const bool success = pFlowData->GetValueWithConversion(targetEntityId);
            if (!success || targetEntityId == 0)
            {
                Warning("FlowGraph: No valid Target Entity set on port '%s'", targetEntityPort.toUtf8().data());
                return 0;
            }
        }

        IEntity* pEntity = gEnv->pEntitySystem->GetEntity(targetEntityId);
        if (!pEntity)
        {
            Warning("FlowGraph: Cannot find entity with id %u, set on port '%s'", targetEntityId, targetEntityPort.toUtf8().data());
            return 0;
        }

        return pEntity;
    }

    //////////////////////////////////////////////////////////////////////////
    bool ChooseCharacterAnimation(IVariable* pVar, std::vector<SItem>& outItems, QString& outText)
    {
        CFlowNodeGetCustomItemsBase* pGetCustomItems = static_cast<CFlowNodeGetCustomItemsBase*> (pVar->GetUserData().value<void *>());
        assert (pGetCustomItems != 0);

        IEntity* pEntity = GetRefEntityByUIConfig(pGetCustomItems, "ref_entity=");
        if (pEntity == 0)
        {
            return false;
        }

        ICharacterInstance* pCharacter = pEntity->GetCharacter(0);
        if (pCharacter == 0)
        {
            return false;
        }

        IAnimationSet* pAnimSet = pCharacter->GetIAnimationSet();
        if (pAnimSet == 0)
        {
            return false;
        }

        const uint32 numAnims = pAnimSet->GetAnimationCount();
        for (int i = 0; i < numAnims; ++i)
        {
            const char* pName = pAnimSet->GetNameByAnimID(i);
            if (pName != 0)
            {
                outItems.push_back(SItem(pName));
            }
        }
        outText = QObject::tr("Choose Animation");
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    bool ChooseBone(IVariable* pVar, std::vector<SItem>& outItems, QString& outText)
    {
        CFlowNodeGetCustomItemsBase* pGetCustomItems = static_cast<CFlowNodeGetCustomItemsBase*> (pVar->GetUserData().value<void *>());
        assert (pGetCustomItems != 0);

        IEntity* pEntity = GetRefEntityByUIConfig(pGetCustomItems, "ref_entity=");
        if (pEntity == 0)
        {
            return false;
        }

        ICharacterInstance* pCharacter = pEntity->GetCharacter(0);
        if (pCharacter == 0)
        {
            return false;
        }

        IDefaultSkeleton& rIDefaultSkeleton = pCharacter->GetIDefaultSkeleton();
        const uint32 numJoints = rIDefaultSkeleton.GetJointCount();
        for (uint32 i = 0; i < numJoints; ++i)
        {
            const char* pName = rIDefaultSkeleton.GetJointNameByID(i);
            if (pName != 0)
            {
                outItems.push_back(SItem(pName));
            }
        }
        outText = QObject::tr("Choose Bone");
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    bool ChooseAttachment(IVariable* pVar, std::vector<SItem>& outItems, QString& outText)
    {
        CFlowNodeGetCustomItemsBase* pGetCustomItems = static_cast<CFlowNodeGetCustomItemsBase*> (pVar->GetUserData().value<void *>());
        assert (pGetCustomItems != 0);

        IEntity* pEntity = GetRefEntityByUIConfig(pGetCustomItems, "ref_entity=");
        if (pEntity == 0)
        {
            return false;
        }

        ICharacterInstance* pCharacter = pEntity->GetCharacter(0);
        if (pCharacter == 0)
        {
            return false;
        }

        IAttachmentManager* pMgr = pCharacter->GetIAttachmentManager();
        if (pMgr == 0)
        {
            return false;
        }

        const int32 numAttachment = pMgr->GetAttachmentCount();
        for (uint32 i = 0; i < numAttachment; ++i)
        {
            IAttachment* pAttachment = pMgr->GetInterfaceByIndex(i);
            if (pAttachment != 0)
            {
                outItems.push_back(SItem(pAttachment->GetName()));
            }
        }
        outText = QObject::tr("Choose Attachment");
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    bool FragmentIDContainsLoopingFragments(const IActionController* pActionController, FragmentID fragmentID)
    {
        const IScope* pScope = pActionController->GetScope(0);
        if (!pScope)
        {
            return false;
        }
        const IAnimationDatabase* pDatabase = &pScope->GetDatabase();
        if (!pDatabase)
        {
            return false;
        }

        const uint32 numTagSets = pDatabase->GetTotalTagSets(fragmentID);
        for (uint32 tagSetIndex = 0; tagSetIndex < numTagSets; ++tagSetIndex)
        {
            SFragTagState tagSetTagState;
            uint32 numOptions = pDatabase->GetTagSetInfo(fragmentID, tagSetIndex, tagSetTagState);
            if (tagSetTagState.fragmentTags == TAG_STATE_EMPTY)
            {
                for (uint32 optionIndex = 0; optionIndex < numOptions; ++optionIndex)
                {
                    const CFragment* pFragment = pDatabase->GetEntry(fragmentID, tagSetIndex, optionIndex);
                    if (!pFragment->m_animLayers.empty())
                    {
                        const TAnimClipSequence& firstClipSequence = pFragment->m_animLayers[0];
                        if (!firstClipSequence.empty())
                        {
                            const SAnimClip& lastAnimClip = firstClipSequence[firstClipSequence.size() - 1];
                            if (lastAnimClip.animation.flags & CA_LOOP_ANIMATION)
                            {
                                return true;
                            }
                        }
                    }
                }
            }
        }

        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    // method is one of [0->All FragmentIDs, 1->OneShot, 2->Looping]
    bool IsFragmentIDAllowedForMethod(const IActionController* pActionController, FragmentID fragmentID, unsigned int method)
    {
        const SControllerDef& controllerDef = pActionController->GetContext().controllerDef;
        const CTagDefinition& fragmentIDs = controllerDef.m_fragmentIDs;

        if (!fragmentIDs.IsValidTagID(fragmentID))
        {
            return false;
        }

        if (method == 0)
        {
            return true;
        }

        if (method > 2)
        {
            return false;
        }

        const bool isPersistent = controllerDef.GetFragmentDef(fragmentID).flags & SFragmentDef::PERSISTENT;
        const bool isLooping = isPersistent || FragmentIDContainsLoopingFragments(pActionController, fragmentID);
        const bool shouldBeLooping = (method == 2);
        return (shouldBeLooping == isLooping);
    }

    // Chooses from AnimationState
    // UIConfig can contain a 'ref_entity=[portname]' entry to specify the entity for AnimationGraph lookups
    // if bUseEx is true, an optional [signal] port will be queried to choose from [0->States/FragmentIDs, 1->Signal, 2->Action]
    bool DoChooseAnimationState(IVariable* pVar, std::vector<SItem>& outItems, QString& outText, bool bUseEx)
    {
        CFlowNodeGetCustomItemsBase* pGetCustomItems = static_cast<CFlowNodeGetCustomItemsBase*> (pVar->GetUserData().value<void *>());
        assert (pGetCustomItems != 0);

        IEntity* pEntity = GetRefEntityByUIConfig(pGetCustomItems, "ref_entity=");
        if (pEntity == 0)
        {
            return false;
        }

        int method = 0;
        if (bUseEx)
        {
            if (!GetValue(pGetCustomItems, "signal", method))
            {
                bool oneShot = true;
                GetValue(pGetCustomItems, "OneShot", oneShot);
                method = oneShot ? 1 : 2;
            }
        }

        if (gEnv->pGame && gEnv->pGame->GetIGameFramework())
        {
            IMannequin& mannequin = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
            IActionController* pActionController = mannequin.FindActionController(*pEntity);
            if (pActionController)
            {
                const CTagDefinition& fragmentIDs = pActionController->GetContext().controllerDef.m_fragmentIDs;
                for (FragmentID fragmentID = 0; fragmentID < fragmentIDs.GetNum(); ++fragmentID)
                {
                    if (IsFragmentIDAllowedForMethod(pActionController, fragmentID, method))
                    {
                        const char* szFragmentName = fragmentIDs.GetTagName(fragmentID);
                        outItems.push_back(SItem(szFragmentName));
                    }
                }
                outText = QObject::tr("Choose Mannequin FragmentID");
                return true;
            }
        }

        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    bool ChooseAnimationState(IVariable* pVar, std::vector<SItem>& outItems, QString& outText)
    {
        return DoChooseAnimationState(pVar, outItems, outText, false);
    }

    //////////////////////////////////////////////////////////////////////////
    bool ChooseAnimationStateEx(IVariable* pVar, std::vector<SItem>& outItems, QString& outText)
    {
        return DoChooseAnimationState(pVar, outItems, outText, true);
    }

    //////////////////////////////////////////////////////////////////////////
    bool ChooseEntityProperties(IVariable* pVar, std::vector<SItem>& outItems, QString& outText)
    {
        CFlowNodeGetCustomItemsBase* pGetCustomItems = static_cast<CFlowNodeGetCustomItemsBase*>(pVar->GetUserData().value<void *>());
        CRY_ASSERT(pGetCustomItems != NULL);

        IEntity* pEntity = GetRefEntityByUIConfig(pGetCustomItems, "ref_entity=");
        if (pEntity == NULL)
        {
            return false;
        }

        CBaseObject* pObj = GetIEditor()->GetObjectManager()->FindObject(pEntity->GetName());

        if (pObj)
        {
            CEntityObject* pEntityObject = (CEntityObject*)pObj;
            if (pEntityObject->GetProperties())
            {
                CVarBlock* pEntityProperties = pEntityObject->GetProperties();
                if (pEntityObject->GetPrototype())
                {
                    pEntityProperties = pEntityObject->GetPrototype()->GetProperties();
                }

                for (int i = 0; i < pEntityProperties->GetNumVariables(); i++)
                {
                    IVariable* currVar = pEntityProperties->GetVariable(i);

                    if (currVar->GetNumVariables() > 0)
                    {
                        for (int j = 0; j < currVar->GetNumVariables(); j++)
                        {
                            outItems.push_back(SItem(string().Format("Properties:%s:%s", currVar->GetName().toUtf8().constData(), currVar->GetVariable(j)->GetName().toUtf8().constData()).c_str()));
                        }
                    }
                    else
                    {
                        outItems.push_back(SItem(string().Format("Properties:%s", pEntityProperties->GetVariable(i)->GetName().toUtf8().constData()).c_str()));
                    }
                }

                if (pEntityObject->GetProperties2())
                {
                    CVarBlock* pEntityProperties2 = pEntityObject->GetProperties2();

                    for (int i = 0; i < pEntityProperties2->GetNumVariables(); i++)
                    {
                        outItems.push_back(SItem(string().Format("Properties2:%s", pEntityProperties->GetVariable(i)->GetName().toUtf8().constData()).c_str()));
                    }
                }
            }
        }
        outText = QObject::tr("Choose Entity Property");
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    bool ChooseVehicleParts(IVariable* pVar, std::vector<SItem>& outItems, QString& outText)
    {
        CFlowNodeGetCustomItemsBase* pGetCustomItems = static_cast<CFlowNodeGetCustomItemsBase*>(pVar->GetUserData().value<void *>());
        CRY_ASSERT(pGetCustomItems != NULL);

        IEntity* pEntity = GetRefEntityByUIConfig(pGetCustomItems, "ref_entity=");
        if (pEntity == NULL)
        {
            return false;
        }

        IVehicleSystem* pVehicleSystem = gEnv->pGame->GetIGameFramework()->GetIVehicleSystem();
        IVehicle* pVehicle = pVehicleSystem->GetVehicle(pEntity->GetId());

        if (!pVehicleSystem || !pVehicle)
        {
            return false;
        }

        // get components
        if (pVehicle->GetComponentCount() > 0)
        {
            outItems.push_back(SItem("Components:All"));
        }

        for (int i = 0; i < pVehicle->GetComponentCount(); i++)
        {
            outItems.push_back(SItem(string().Format("Components:%s", pVehicle->GetComponent(i)->GetComponentName()).c_str()));
        }

        // get wheels
        if (pVehicle->GetWheelCount() > 0)
        {
            outItems.push_back(SItem("Wheels:All"));
        }

        for (int j = 0; j < pVehicle->GetWheelCount(); j++)
        {
            outItems.push_back(SItem(string().Format("Wheels:%s", pVehicle->GetWheelPart(j)->GetName()).c_str()));
        }

        // get weapons
        if (pVehicle->GetWeaponCount() > 0)
        {
            outItems.push_back(SItem("Weapons:All"));
        }

        for (int k = 0; k < pVehicle->GetWeaponCount(); k++)
        {
            outItems.push_back(SItem(string().Format("Weapons:%s", gEnv->pEntitySystem->GetEntity(pVehicle->GetWeaponId(k))->GetName())));
        }

        // get seats
        if (pVehicle->GetSeatCount() > 0)
        {
            outItems.push_back(SItem("Seats:All"));
        }

        for (uint32 l = 0; l < pVehicle->GetSeatCount(); l++)
        {
            outItems.push_back(SItem(string().Format("Seats:%s", pVehicle->GetSeatById(l + 1)->GetSeatName())));
        }

        outText = QObject::tr("Choose Vehicle Parts");
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    bool GetMatParams(IEntity* pEntity, int slot, int subMtlId, DynArrayRef<SShaderParam>** outParams)
    {
        IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
        if (pRenderComponent == 0)
        {
            return false;
        }

        _smart_ptr<IMaterial> pMtl = pRenderComponent->GetRenderMaterial(slot);
        if (pMtl == 0)
        {
            Warning("EntityMaterialShaderParam: Entity '%s' [%d] has no material at slot %d", pEntity->GetName(), pEntity->GetId(), slot);
            return false;
        }

        pMtl = (subMtlId > 0 && pMtl->GetSubMtlCount() > 0) ? pMtl->GetSubMtl(subMtlId) : pMtl;
        if (pMtl == 0)
        {
            Warning("EntityMaterialShaderParam: Entity '%s' [%d] has no sub-material %d at slot %d", pEntity->GetName(), pEntity->GetId(), subMtlId, slot);
            return false;
        }

        const SShaderItem& shaderItem = pMtl->GetShaderItem();
        IRenderShaderResources* pRendRes = shaderItem.m_pShaderResources;
        if (pRendRes == 0)
        {
            Warning("EntityMaterialShaderParam: Material '%s' has no shader resources ", pMtl->GetName());
            return false;
        }
        *outParams = &pRendRes->GetParameters();
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    void FillOutParamsByShaderParams(std::vector<SItem>& outItems, DynArrayRef<SShaderParam>* params, int limit)
    {
        if (limit == 1)
        {
            outItems.push_back(SItem("diffuse"));
            outItems.push_back(SItem("specular"));
            outItems.push_back(SItem("emissive_color"));
        }
        else if (limit == 2)
        {
            outItems.push_back(SItem("emissive_intensity"));
            outItems.push_back(SItem("shininess"));
            outItems.push_back(SItem("alpha"));
            outItems.push_back(SItem("opacity"));
        }

        // fill in params
        SItem item;
        for (int i = 0; i < params->size(); ++i)
        {
            SShaderParam& param = (*params)[i];
            EParamType paramType = param.m_Type;
            if (limit == 1 && paramType != eType_VECTOR && paramType != eType_FCOLOR)
            {
                continue;
            }
            else if (limit == 2 && paramType != eType_FLOAT && paramType != eType_INT && paramType != eType_SHORT && paramType != eType_BOOL)
            {
                continue;
            }
            item.name = param.m_Name;
            outItems.push_back(item);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    bool ChooseMatParamSlot(IVariable* pVar, std::vector<SItem>& outItems, QString& outText)
    {
        CFlowNodeGetCustomItemsBase* pGetCustomItems = static_cast<CFlowNodeGetCustomItemsBase*> (pVar->GetUserData().value<void *>());
        assert (pGetCustomItems != 0);

        IEntity* pEntity = GetRefEntityByUIConfig(pGetCustomItems, "ref_entity=");
        if (pEntity == 0)
        {
            return false;
        }
        TParamMap paramMap;
        GetParamMap(pGetCustomItems, paramMap);

        QString& slotString = paramMap["slot_ref"];
        QString& subString = paramMap["sub_ref"];
        QString& paramTypeString = paramMap["param"];
        int slot = -1;
        int subMtlId = -1;
        if (GetValue(pGetCustomItems, slotString, slot) == false)
        {
            return false;
        }
        if (GetValue(pGetCustomItems, subString, subMtlId) == false)
        {
            return false;
        }

        int limit = 0;
        if (paramTypeString == "vec")
        {
            limit = 1;
        }
        else if (paramTypeString == "float")
        {
            limit = 2;
        }

        DynArrayRef<SShaderParam>* params;
        if (GetMatParams(pEntity, slot, subMtlId, &params) == false)
        {
            return false;
        }

        FillOutParamsByShaderParams(outItems, params, limit);

        outText = QObject::tr("Choose Param");
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    bool ChooseMatParamName(IVariable* pVar, std::vector<SItem>& outItems, QString& outText)
    {
        CFlowNodeGetCustomItemsBase* pGetCustomItems = static_cast<CFlowNodeGetCustomItemsBase*> (pVar->GetUserData().value<void *>());
        assert (pGetCustomItems != 0);

        TParamMap paramMap;
        GetParamMap(pGetCustomItems, paramMap);

        QString& nameString = paramMap["name_ref"];
        QString& subString = paramMap["sub_ref"];
        QString& paramTypeString = paramMap["param"];
        string  matName;
        int subMtlId = -1;
        if (GetValue(pGetCustomItems, nameString, matName) == false || matName.empty())
        {
            Warning("MaterialParam: No/Invalid Material given.");
            return false;
        }
        if (GetValue(pGetCustomItems, subString, subMtlId) == false)
        {
            return false;
        }

        int limit = 0;
        if (paramTypeString == "vec")
        {
            limit = 1;
        }
        else if (paramTypeString == "float")
        {
            limit = 2;
        }

        _smart_ptr<IMaterial> pMtl = gEnv->p3DEngine->GetMaterialManager()->FindMaterial(matName);
        if (pMtl == 0)
        {
            Warning("MaterialParam: Material '%s' not found.", matName.c_str());
            return false;
        }

        pMtl = (subMtlId >= 0 && pMtl->GetSubMtlCount() > 0) ? pMtl->GetSubMtl(subMtlId) : pMtl;
        if (pMtl == 0)
        {
            Warning("MaterialParam: Material '%s' has no sub-material %d ", matName.c_str(), subMtlId);
            return false;
        }

        const SShaderItem& shaderItem = pMtl->GetShaderItem();
        IRenderShaderResources* pRendRes = shaderItem.m_pShaderResources;
        if (pRendRes == 0)
        {
            Warning("MaterialParam: Material '%s' has no shader resources ", pMtl->GetName());
            return false;
        }
        DynArrayRef<SShaderParam>* params = &pRendRes->GetParameters();

        FillOutParamsByShaderParams(outItems, params, limit);

        outText = QObject::tr("Choose Param");
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    bool ChooseMatParamCharAttachment(IVariable* pVar, std::vector<SItem>& outItems, QString& outText)
    {
        CFlowNodeGetCustomItemsBase* pGetCustomItems = static_cast<CFlowNodeGetCustomItemsBase*> (pVar->GetUserData().value<void *>());
        assert (pGetCustomItems != 0);

        IEntity* pEntity = GetRefEntityByUIConfig(pGetCustomItems, "ref_entity=");
        if (pEntity == 0)
        {
            return false;
        }
        TParamMap paramMap;
        GetParamMap(pGetCustomItems, paramMap);

        QString& slotString = paramMap["charslot_ref"];
        QString& attachString = paramMap["attachment_ref"];
        QString& paramTypeString = paramMap["param"];
        QString& subString = paramMap["sub_ref"];
        int charSlot = -1;
        int subMtlId = -1;
        if (GetValue(pGetCustomItems, slotString, charSlot) == false)
        {
            return false;
        }
        if (GetValue(pGetCustomItems, subString, subMtlId) == false)
        {
            return false;
        }
        string attachmentName;
        if (GetValue(pGetCustomItems, attachString, attachmentName) == false || attachmentName.empty())
        {
            Warning("MaterialParam: No/Invalid Attachment given.");
            return false;
        }

        ICharacterInstance* pCharacter = pEntity->GetCharacter(charSlot);
        if (pCharacter == 0)
        {
            return false;
        }

        IAttachmentManager* pMgr = pCharacter->GetIAttachmentManager();
        if (pMgr == 0)
        {
            return false;
        }

        IAttachment* pAttachment = pMgr->GetInterfaceByName(attachmentName.c_str());
        if (pAttachment == 0)
        {
            return false;
        }
        IAttachmentObject* pAttachmentObject = pAttachment->GetIAttachmentObject();
        if (pAttachmentObject == 0)
        {
            return false;
        }

        _smart_ptr<IMaterial> pMtl = (_smart_ptr<IMaterial>)pAttachmentObject->GetBaseMaterial();
        if (pMtl == 0)
        {
            return false;
        }

        int limit = 0;
        if (paramTypeString == "vec")
        {
            limit = 1;
        }
        else if (paramTypeString == "float")
        {
            limit = 2;
        }

        string matName = pMtl->GetName();

        pMtl = (subMtlId >= 0 && pMtl->GetSubMtlCount() > 0) ? pMtl->GetSubMtl(subMtlId) : pMtl;
        if (pMtl == 0)
        {
            Warning("MaterialParam: Material '%s' has no sub-material %d ", matName.c_str(), subMtlId);
            return false;
        }

        const SShaderItem& shaderItem = pMtl->GetShaderItem();
        IRenderShaderResources* pRendRes = shaderItem.m_pShaderResources;
        if (pRendRes == 0)
        {
            Warning("MaterialParam: Material '%s' has no shader resources ", pMtl->GetName());
            return false;
        }
        DynArrayRef<SShaderParam>* params = &pRendRes->GetParameters();

        FillOutParamsByShaderParams(outItems, params, limit);

        outText = QObject::tr("Choose Param");
        return true;
    }

    bool ChooseFormationName(IVariable* pVar, std::vector<SItem>& outItems, QString& outText)
    {
        unsigned int formationNameCount = 0;
        const char* formationNames[32];
        gEnv->pAISystem->EnumerateFormationNames(32, formationNames, &formationNameCount);
        for (unsigned int i = 0; i < formationNameCount; ++i)
        {
            outItems.push_back(SItem(formationNames[i]));
        }
        outText = QObject::tr("Choose Formation Name");
        return true;
    }

    bool ChooseCommunicationManagerVariableName(IVariable* pVar, std::vector<SItem>& outItems, QString& outText)
    {
        size_t variableNameCount = 0;
        const static size_t kMaxVariableNamesAllowed = 64;
        const char* variableNames[kMaxVariableNamesAllowed];
        gEnv->pAISystem->GetCommunicationManager()->GetVariablesNames(variableNames, kMaxVariableNamesAllowed, variableNameCount);
        for (unsigned int i = 0; i < variableNameCount; ++i)
        {
            outItems.push_back(SItem(variableNames[i]));
        }
        outText = QObject::tr("Choose Communication Variable Name");
        return true;
    }
};

struct CFlowNodeCustomItemsImpl
    : public CFlowNodeGetCustomItemsBase
{
    typedef bool (* TGetItemsFct) (IVariable*, std::vector<SItem>&, QString&);
    CFlowNodeCustomItemsImpl(const TGetItemsFct& fct, bool tree, const char* sep)
        : m_fct(fct)
        , m_tree(tree)
        , m_sep(sep) {}
    virtual bool GetItems (IVariable* pVar, std::vector<SItem>& items, QString& outDialogTitle) { return m_fct(pVar, items, outDialogTitle); }
    virtual bool UseTree() { return m_tree; }
    virtual const char* GetTreeSeparator() { return m_sep; }
    TGetItemsFct m_fct;
    bool m_tree;
    const char* m_sep;
};

#ifdef FGVARIABLES_REAL_CLONE
#define DECL_CHOOSE_EX_IMPL(NAME, FCT, TREE, SEP)                                                                                                                       \
    struct  C##NAME                                                                                                                                                     \
        : public CFlowNodeCustomItemsImpl                                                                                                                               \
    {                                                                                                                                                                   \
        C##NAME()                                                                                                                                                       \
            : CFlowNodeCustomItemsImpl(&FCT, TREE, SEP) {}                                                                                                              \
        virtual CFlowNodeGetCustomItemsBase* Clone() const { C##NAME * pNew = new C##NAME(); pNew->m_pFlowNode = m_pFlowNode; pNew->m_config = m_config; return pNew; } \
    };
#else
#define DECL_CHOOSE_EX_IMPL(NAME, FCT, TREE, SEP)                                                 \
    struct  C##NAME                                                                               \
        : public CFlowNodeCustomItemsImpl                                                         \
    {                                                                                             \
        C##NAME()                                                                                 \
            : CFlowNodeCustomItemsImpl(&FCT, TREE, SEP) {}                                        \
        virtual CFlowNodeGetCustomItemsBase* Clone() const { return const_cast<C##NAME*>(this); } \
    };
#endif

#define DECL_CHOOSE_EX(FOO, TREE, SEP) DECL_CHOOSE_EX_IMPL(FOO, FOO, TREE, SEP)
#define DECL_CHOOSE(FOO) DECL_CHOOSE_EX(FOO, false, "")

DECL_CHOOSE(ChooseCharacterAnimation);
DECL_CHOOSE(ChooseAnimationState);
DECL_CHOOSE(ChooseAnimationStateEx);
DECL_CHOOSE(ChooseBone);
DECL_CHOOSE(ChooseAttachment);
DECL_CHOOSE_EX(ChooseDialog, true, ".");
DECL_CHOOSE(ChooseMatParamSlot);
DECL_CHOOSE(ChooseMatParamName);
DECL_CHOOSE(ChooseMatParamCharAttachment);
DECL_CHOOSE(ChooseFormationName);
DECL_CHOOSE(ChooseCommunicationManagerVariableName);
DECL_CHOOSE_EX(ChooseVehicleParts, true, ":");
DECL_CHOOSE_EX(ChooseEntityProperties, true, ":");

// And here's the map...
// Used to specify specialized property editors for input ports.
// If the name starts with one of these prefixes the data type
// of the port will be changed to allow the specialized editor.
// The name will be shown without the prefix.
// if datatype is IVariable::DT_USERITEMCB a GenericSelectItem dialog is used
// and the callback should provide the selectable items (optionally context
// [==node]-sensitive items) e.g. Animation Lookup
static const FlowGraphVariables::MapEntry prefix_dataType_table[] =
{
    { "clr_",         IVariable::DT_COLOR, 0 },
    { "color_",       IVariable::DT_COLOR, 0 },

    { "tex_",         IVariable::DT_TEXTURE, 0 },
    { "texture_",     IVariable::DT_TEXTURE, 0 },

    { "obj_",         IVariable::DT_OBJECT, 0 },
    { "object_",      IVariable::DT_OBJECT, 0 },

    { "file_",        IVariable::DT_FILE, 0 },

    { "text_",        IVariable::DT_LOCAL_STRING, 0 },
    { "equip_",       IVariable::DT_EQUIP, 0 },
    { "reverbpreset_", IVariable::DT_REVERBPRESET, 0 },

    { "aianchor_",    IVariable::DT_AI_ANCHOR, 0 },
    { "aibehavior_",  IVariable::DT_AI_BEHAVIOR, 0 },
#ifdef USE_DEPRECATED_AI_CHARACTER_SYSTEM
    { "aicharacter_", IVariable::DT_AI_CHARACTER, 0 },
#endif
    { "aipfpropertieslist_", IVariable::DT_AI_PFPROPERTIESLIST, 0 },
    { "aientityclasses_", IVariable::DT_AIENTITYCLASSES, 0 },
    { "aiterritory_", IVariable::DT_AITERRITORY, 0 },
    { "aiwave_",      IVariable::DT_AIWAVE, 0 },
    { "soclass_",     IVariable::DT_SOCLASS, 0 },
    { "soclasses_",   IVariable::DT_SOCLASSES, 0 },
    { "sostate_",     IVariable::DT_SOSTATE, 0 },
    { "sostates_",    IVariable::DT_SOSTATES, 0 },
    { "sopattern_",   IVariable::DT_SOSTATEPATTERN, 0 },
    { "soaction_",    IVariable::DT_SOACTION, 0 },
    { "sohelper_",    IVariable::DT_SOHELPER, 0 },
    { "sonavhelper_", IVariable::DT_SONAVHELPER, 0 },
    { "soanimhelper_", IVariable::DT_SOANIMHELPER, 0 },
    { "soevent_",     IVariable::DT_SOEVENT, 0 },
    { "customaction_",    IVariable::DT_CUSTOMACTION, 0 },
    { "gametoken_",   IVariable::DT_GAMETOKEN, 0 },
    { "mat_",         IVariable::DT_MATERIAL, 0 },
    { "seq_",         IVariable::DT_SEQUENCE, 0 },
    { "mission_",     IVariable::DT_MISSIONOBJ, 0 },
    { "anim_",        IVariable::DT_USERITEMCB, &FlowGraphVariables::CreateIt<CChooseCharacterAnimation> },
    { "animstate_",   IVariable::DT_USERITEMCB, &FlowGraphVariables::CreateIt<CChooseAnimationState> },
    { "animstateEx_", IVariable::DT_USERITEMCB, &FlowGraphVariables::CreateIt<CChooseAnimationStateEx> },
    { "bone_",        IVariable::DT_USERITEMCB, &FlowGraphVariables::CreateIt<CChooseBone> },
    { "attachment_",  IVariable::DT_USERITEMCB, &FlowGraphVariables::CreateIt<CChooseAttachment> },
    { "dialog_",      IVariable::DT_USERITEMCB, &FlowGraphVariables::CreateIt<CChooseDialog> },
    { "matparamslot_",      IVariable::DT_USERITEMCB, &FlowGraphVariables::CreateIt<CChooseMatParamSlot> },
    { "matparamname_",      IVariable::DT_USERITEMCB, &FlowGraphVariables::CreateIt<CChooseMatParamName> },
    { "matparamcharatt_",      IVariable::DT_USERITEMCB, &FlowGraphVariables::CreateIt<CChooseMatParamCharAttachment> },
    { "seqid_",       IVariable::DT_SEQUENCE_ID, 0 },
    { "lightanimation_",    IVariable::DT_LIGHT_ANIMATION, 0 },
    { "formation_", IVariable::DT_USERITEMCB, &FlowGraphVariables::CreateIt<CChooseFormationName> },
    { "communicationVariable_", IVariable::DT_USERITEMCB, &FlowGraphVariables::CreateIt<CChooseCommunicationManagerVariableName> },
    { "vehicleParts_", IVariable::DT_USERITEMCB, &FlowGraphVariables::CreateIt<CChooseVehicleParts> },
    { "entityProperties_", IVariable::DT_USERITEMCB, &FlowGraphVariables::CreateIt<CChooseEntityProperties> },
    { "geomcache_",   IVariable::DT_GEOM_CACHE, 0 },
    { "audioTrigger_",   IVariable::DT_AUDIO_TRIGGER, 0 },
    { "audioSwitch_",   IVariable::DT_AUDIO_SWITCH, 0 },
    { "audioSwitchState_",   IVariable::DT_AUDIO_SWITCH_STATE, 0 },
    { "audioRTPC_",   IVariable::DT_AUDIO_RTPC, 0 },
    { "audioEnvironment_",   IVariable::DT_AUDIO_ENVIRONMENT, 0 },
    { "audioPreloadRequest_",   IVariable::DT_AUDIO_PRELOAD_REQUEST, 0 },
    { "uiElement_", IVariable::DT_UI_ELEMENT, 0 },
};
static const int prefix_dataType_numEntries = sizeof(prefix_dataType_table) / sizeof(FlowGraphVariables::MapEntry);


namespace FlowGraphVariables
{
    const MapEntry* FindEntry(const char* sPrefix)
    {
        for (int i = 0; i < prefix_dataType_numEntries; ++i)
        {
            if (strstr(sPrefix, prefix_dataType_table[i].sPrefix) == sPrefix)
            {
                return &prefix_dataType_table[i];
            }
        }
        return 0;
    }
};

//////////////////////////////////////////////////////////////////////////
CUIEnumsDatabase_SEnum* CVariableFlowNodeDynamicEnum::GetSEnum()
{
    CFlowNodeGetCustomItemsBase* pGetCustomItems = static_cast<CFlowNodeGetCustomItemsBase*> (this->GetUserData().value<void *>());
    if (pGetCustomItems == 0)
    {
        return 0;
    }

    assert (pGetCustomItems != 0);
    // CryLogAlways("CVariableFlowNodeDynamicEnum::GetSEnum 0x%p pGetCustomItems=0x%p FlowNode=0x%p", this, pGetCustomItems, pGetCustomItems->GetFlowNode());

    string enumName;
    bool ok = ::GetValue(pGetCustomItems, m_refPort, enumName);
    if (!ok)
    {
        return 0;
    }

    QString enumKey;
    enumKey = m_refFormatString.replace("%s", "%1").arg(enumName.c_str());
    CUIEnumsDatabase_SEnum* pEnum = GetIEditor()->GetUIEnumsDatabase()->FindEnum(enumKey);
    return pEnum;
}

//////////////////////////////////////////////////////////////////////////
CUIEnumsDatabase_SEnum* CVariableFlowNodeDefinedEnum::GetSEnum()
{
    CFlowNodeGetCustomItemsBase* pGetCustomItems = static_cast<CFlowNodeGetCustomItemsBase*> (this->GetUserData().value<void *>());
    if (pGetCustomItems == 0)
    {
        return 0;
    }

    assert (pGetCustomItems != 0);
    // CryLogAlways("CVariableFlowNodeDefinedEnum::GetSEnum 0x%p pGetCustomItems=0x%p FlowNode=0x%p", this, pGetCustomItems, pGetCustomItems->GetFlowNode());

    CFlowNode* pCFlowNode = pGetCustomItems->GetFlowNode();
    if (pCFlowNode)
    {
        IFlowGraph* pGraph = pCFlowNode->GetIFlowGraph();
        IFlowNodeData* pData = pGraph ? pGraph->GetNodeData(pCFlowNode->GetFlowNodeId()) : NULL;
        IFlowNode* pIFlowNode = pData ? pData->GetNode() : NULL;

        if (pIFlowNode)
        {
            CEntityObject* pNodeEntity = pCFlowNode->GetEntity();
            IEntity* pNodeIEntity = pNodeEntity ? pNodeEntity->GetIEntity() : NULL;

            SFlowNodeConfig config;
            pData->GetConfiguration(config);
            const uint32 portId = m_portId - ((config.nFlags & EFLN_TARGET_ENTITY) ? 1 : 0);

            string outEnumName;
            if (!pIFlowNode->GetPortGlobalEnum(portId, pNodeIEntity, m_refFormatString.toUtf8().data(), outEnumName))
            {
                outEnumName = m_refFormatString.toUtf8().data();
            }

            CUIEnumsDatabase_SEnum* pEnum = GetIEditor()->GetUIEnumsDatabase()->FindEnum(outEnumName.c_str());
            return pEnum;
        }
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
CVariableFlowNodeCustomEnumBase::CDynamicEnumList::CDynamicEnumList(CVariableFlowNodeCustomEnumBase* pDynEnum)
{
    m_pDynEnum = pDynEnum;
    // CryLogAlways("CDynamicEnumList::ctor 0x%p", this);
}

//////////////////////////////////////////////////////////////////////////
CVariableFlowNodeCustomEnumBase::CDynamicEnumList::~CDynamicEnumList()
{
    // CryLogAlways("CDynamicEnumList::dtor 0x%p", this);
}

//! Get the name of specified value in enumeration.
QString CVariableFlowNodeCustomEnumBase::CDynamicEnumList::GetItemName(uint index)
{
    struct CUIEnumsDatabase_SEnum* pEnum = m_pDynEnum->GetSEnum();
    if (!pEnum || index >= pEnum->strings.size())
    {
        return QString();
    }
    return pEnum->strings[index];
}

//////////////////////////////////////////////////////////////////////////
QString CVariableFlowNodeCustomEnumBase::CDynamicEnumList::NameToValue(const QString& name)
{
    struct CUIEnumsDatabase_SEnum* pEnum = m_pDynEnum->GetSEnum();
    if (pEnum == 0)
    {
        return "";
    }
    return pEnum->NameToValue(name);
}

//////////////////////////////////////////////////////////////////////////
QString CVariableFlowNodeCustomEnumBase::CDynamicEnumList::ValueToName(const QString& name)
{
    struct CUIEnumsDatabase_SEnum* pEnum = m_pDynEnum->GetSEnum();
    if (pEnum == 0)
    {
        return "";
    }
    return pEnum->ValueToName(name);
}


