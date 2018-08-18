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

#include "CryLegacy_precompiled.h"
#include <IRenderAuxGeom.h>
#include "ModelMesh.h"
#include "SocketSimulation.h"
#include "AttachmentPRow.h"
#include "AttachmentVCloth.h"
#include "AttachmentManager.h"
#include "CharacterInstance.h"
#include "CharacterManager.h"
#include "StringUtils.h"

namespace BoneNames
{
    const char* BlendWeightPendulum = "all_blendWeightPendulum";
}

uint32 CAttachmentManager::LoadAttachmentList(const char* pathname)
{
    XmlNodeRef nodeAttachList   = g_pISystem->LoadXmlFromFile(pathname);
    if (nodeAttachList == 0)
    {
        return 0;
    }

    const char* AttachListTag       = nodeAttachList->getTag();
    if (strcmp(AttachListTag, "AttachmentList"))
    {
        return 0;
    }

    RemoveAllAttachments();

    DynArray<CharacterAttachment> arrAttachments;
    uint32 numChildren = nodeAttachList->getChildCount();
    arrAttachments.resize(numChildren);
    uint32 numValidAttachments = ParseXMLAttachmentList(&arrAttachments[0], numChildren, nodeAttachList);
    if (numValidAttachments)
    {
        arrAttachments.resize(numValidAttachments);
        InitAttachmentList(arrAttachments, m_pSkelInstance->GetFilePath(), 0, -1);
    }
    return 1;
};


//-----------------------------------------------------------
//parse attachment-list
//-----------------------------------------------------------
uint32 CAttachmentManager::ParseXMLAttachmentList(CharacterAttachment* parrAttachments, uint32 numAttachments, XmlNodeRef nodeAttachements)
{
    uint32 numValidAttachments = 0;
    for (uint32 i = 0; i < numAttachments; i++)
    {
        CharacterAttachment attach;
        XmlNodeRef nodeAttach = nodeAttachements->getChild(i);
        const char* AttachTag = nodeAttach->getTag();
        if (strcmp(AttachTag, "Attachment"))
        {
            continue; //invalid
        }
        stack_string Type = nodeAttach->getAttr("Type");
        if (Type == "CA_BONE")
        {
            attach.m_Type = CA_BONE;
        }
        if (Type == "CA_FACE")
        {
            attach.m_Type = CA_FACE;
        }
        if (Type == "CA_SKIN")
        {
            attach.m_Type = CA_SKIN;
        }
        if (Type == "CA_PROX")
        {
            attach.m_Type = CA_PROX;
        }
        if (Type == "CA_PROW")
        {
            attach.m_Type = CA_PROW;
        }
        if (Type == "CA_VCLOTH")
        {
            attach.m_Type = CA_VCLOTH;
        }
        if (attach.m_Type == 0xDeadBeef)
        {
            continue;   //invalid
        }
        string AName = nodeAttach->getAttr("AName");
        CryStringUtils::UnifyFilePath(AName);
        attach.m_strAttachmentName = AName;

        nodeAttach->getAttr("Rotation", attach.m_AbsoluteDefault.q);
        nodeAttach->getAttr("Position", attach.m_AbsoluteDefault.t);

        attach.m_relRotation = nodeAttach->getAttr("RelRotation", attach.m_RelativeDefault.q);
        attach.m_relPosition = nodeAttach->getAttr("RelPosition", attach.m_RelativeDefault.t);

        attach.m_strJointName = nodeAttach->getAttr("BoneName");
        attach.m_strBindingPath = nodeAttach->getAttr("Binding");
        CryStringUtils::UnifyFilePath(attach.m_strBindingPath);
        attach.m_strSimBindingPath = nodeAttach->getAttr("SimBinding");   //only used for the cloth simulation mesh

        nodeAttach->getAttr("ProxyParams", attach.m_ProxyParams);
        nodeAttach->getAttr("ProxyPurpose", attach.m_ProxyPurpose);

        const char* fileExt = PathUtil::GetExt(attach.m_strBindingPath);
        const bool isCGF = (0 == _stricmp(fileExt, CRY_GEOMETRY_FILE_EXT));
        if (isCGF)
        {
            attach.m_pStaticObject = gEnv->p3DEngine->LoadStatObjAutoRef(attach.m_strBindingPath.c_str(), 0, 0, true);
        }

        if (nodeAttach->haveAttr("Material"))
        {
            const char* material = nodeAttach->getAttr("Material");
            attach.m_pMaterial = g_pISystem->GetI3DEngine()->GetMaterialManager()->LoadMaterial(material, false);
        }
        if (nodeAttach->haveAttr("MaterialLOD0"))
        {
            const char* material0 = nodeAttach->getAttr("MaterialLOD0");
            attach.m_parrMaterial[0] = g_pISystem->GetI3DEngine()->GetMaterialManager()->LoadMaterial(material0, false);
        }
        if (nodeAttach->haveAttr("MaterialLOD1"))
        {
            const char* material1 = nodeAttach->getAttr("MaterialLOD1");
            attach.m_parrMaterial[1] = g_pISystem->GetI3DEngine()->GetMaterialManager()->LoadMaterial(material1, false);
        }
        if (nodeAttach->haveAttr("MaterialLOD2"))
        {
            const char* material2 = nodeAttach->getAttr("MaterialLOD2");
            attach.m_parrMaterial[2] = g_pISystem->GetI3DEngine()->GetMaterialManager()->LoadMaterial(material2, false);
        }
        if (nodeAttach->haveAttr("MaterialLOD3"))
        {
            const char* material3 = nodeAttach->getAttr("MaterialLOD3");
            attach.m_parrMaterial[3] = g_pISystem->GetI3DEngine()->GetMaterialManager()->LoadMaterial(material3, false);
        }
        if (nodeAttach->haveAttr("MaterialLOD4"))
        {
            const char* material4 = nodeAttach->getAttr("MaterialLOD4");
            attach.m_parrMaterial[4] = g_pISystem->GetI3DEngine()->GetMaterialManager()->LoadMaterial(material4, false);
        }
        if (nodeAttach->haveAttr("MaterialLOD5"))
        {
            const char* material5 = nodeAttach->getAttr("MaterialLOD5");
            attach.m_parrMaterial[5] = g_pISystem->GetI3DEngine()->GetMaterialManager()->LoadMaterial(material5, false);
        }

        uint32 isPendulum = 0;
        nodeAttach->getAttr("PA_PendulumType", isPendulum);
        if (isPendulum == SimulationParams::PENDULUM_CONE || isPendulum == SimulationParams::PENDULUM_HINGE_PLANE || isPendulum == SimulationParams::PENDULUM_HALF_CONE)
        {
            attach.ap.m_nClampType = SimulationParams::ClampType(isPendulum);
            nodeAttach->getAttr("PA_FPS", attach.ap.m_nSimFPS);
            nodeAttach->getAttr("PA_Redirect", attach.ap.m_useRedirect);
            nodeAttach->getAttr("PA_MaxAngle", attach.ap.m_fMaxAngle);
            nodeAttach->getAttr("PA_HRotation", attach.ap.m_vDiskRotation.x);

            nodeAttach->getAttr("PA_Mass", attach.ap.m_fMass);
            nodeAttach->getAttr("PA_Gravity", attach.ap.m_fGravity);
            nodeAttach->getAttr("PA_Damping", attach.ap.m_fDamping);
            nodeAttach->getAttr("PA_Stiffness", attach.ap.m_fStiffness);

            nodeAttach->getAttr("PA_PivotOffset", attach.ap.m_vPivotOffset);
            nodeAttach->getAttr("PA_PendulumOffset", attach.ap.m_vSimulationAxis);
            nodeAttach->getAttr("PA_SimulationAxis", attach.ap.m_vSimulationAxis);
            nodeAttach->getAttr("PA_StiffnessTarget", attach.ap.m_vStiffnessTarget);

            nodeAttach->getAttr("PA_CapsuleX", attach.ap.m_vCapsule.x);
            nodeAttach->getAttr("PA_CapsuleY", attach.ap.m_vCapsule.y);
            nodeAttach->getAttr("PA_ProjectionType", attach.ap.m_nProjectionType);
            attach.ap.m_strDirTransJoint = nodeAttach->getAttr("PA_DirTransJointName");
            uint32 IsIdentical = _stricmp(attach.ap.m_strDirTransJoint.c_str(), attach.m_strJointName.c_str()) == 0;
            if (attach.ap.m_strDirTransJoint.length() && IsIdentical)
            {
                attach.ap.m_strDirTransJoint.reset();
            }

            char proxytag[] = "PA_Proxy00";
            for (uint32 i = 0; i < SimulationParams::MaxCollisionProxies; i++)
            {
                CCryName strProxyName = CCryName(nodeAttach->getAttr(proxytag));
                proxytag[9]++;
                if (strProxyName.empty())
                {
                    continue;
                }
                attach.ap.m_arrProxyNames.push_back(strProxyName);
            }
        }

        uint32 isSpring = 0;
        nodeAttach->getAttr("SA_SpringType", isSpring);
        if (isSpring)
        {
            attach.ap.m_nClampType = SimulationParams::SPRING_ELLIPSOID;
            nodeAttach->getAttr("SA_FPS", attach.ap.m_nSimFPS);
            nodeAttach->getAttr("SA_Radius", attach.ap.m_fRadius);
            nodeAttach->getAttr("SA_ScaleZP", attach.ap.m_vSphereScale.x);
            nodeAttach->getAttr("SA_ScaleZN", attach.ap.m_vSphereScale.y);
            nodeAttach->getAttr("SA_DiskRotX", attach.ap.m_vDiskRotation.x);
            nodeAttach->getAttr("SA_DiskRotZ", attach.ap.m_vDiskRotation.y);
            nodeAttach->getAttr("SA_HRotation", attach.ap.m_vDiskRotation.x);  //just for backwards compatibility

            nodeAttach->getAttr("SA_Redirect", attach.ap.m_useRedirect);

            nodeAttach->getAttr("SA_Mass", attach.ap.m_fMass);
            nodeAttach->getAttr("SA_Gravity", attach.ap.m_fGravity);
            nodeAttach->getAttr("SA_Damping", attach.ap.m_fDamping);
            nodeAttach->getAttr("SA_Stiffness", attach.ap.m_fStiffness);

            nodeAttach->getAttr("SA_PivotOffset", attach.ap.m_vPivotOffset);
            nodeAttach->getAttr("SA_StiffnessTarget", attach.ap.m_vStiffnessTarget);

            attach.ap.m_vCapsule.x = 0;
            nodeAttach->getAttr("SA_CapsuleY", attach.ap.m_vCapsule.y);
            nodeAttach->getAttr("SA_ProjectionType", attach.ap.m_nProjectionType);

            char proxytag[] = "SA_Proxy00";
            for (uint32 i = 0; i < SimulationParams::MaxCollisionProxies; i++)
            {
                CCryName strProxyName = CCryName(nodeAttach->getAttr(proxytag));
                proxytag[9]++;
                if (strProxyName.empty())
                {
                    continue;
                }
                attach.ap.m_arrProxyNames.push_back(strProxyName);
            }
        }

        uint32 IsProjection = 0;
        nodeAttach->getAttr("P_Projection", IsProjection);
        if (IsProjection)
        {
            attach.ap.m_nClampType = SimulationParams::TRANSLATIONAL_PROJECTION;
            attach.ap.m_useRedirect = 1;
            nodeAttach->getAttr("P_ProjectionType", attach.ap.m_nProjectionType);
            attach.ap.m_strDirTransJoint = nodeAttach->getAttr("P_DirTransJointName");
            uint32 IsIdentical = _stricmp(attach.ap.m_strDirTransJoint.c_str(), attach.m_strJointName.c_str()) == 0;
            if (attach.ap.m_strDirTransJoint.length() && IsIdentical)
            {
                attach.ap.m_strDirTransJoint.reset();
            }

            nodeAttach->getAttr("P_TranslationAxis", attach.ap.m_vSimulationAxis);

            nodeAttach->getAttr("P_CapsuleX", attach.ap.m_vCapsule.x);
            nodeAttach->getAttr("P_CapsuleY", attach.ap.m_vCapsule.y);

            nodeAttach->getAttr("P_PivotOffset", attach.ap.m_vPivotOffset);

            char proxytag[] = "P_Proxy00";
            for (uint32 i = 0; i < SimulationParams::MaxCollisionProxies; i++)
            {
                CCryName strProxyName = CCryName(nodeAttach->getAttr(proxytag));
                proxytag[8]++;
                if (strProxyName.empty())
                {
                    continue;
                }
                attach.ap.m_arrProxyNames.push_back(strProxyName);
            }
        }


        attach.ap.m_strProcFunction = nodeAttach->getAttr("ProcFunction");

        if (nodeAttach->haveAttr("PhysPropType"))
        {
            memset(&attach.m_AttPhysInfo, 0, sizeof(attach.m_AttPhysInfo));
            stack_string propType = nodeAttach->getAttr("PhysPropType");

            int32 nRopeOrGrid = !strcmp(propType, "Rope") ? 0 : (!strcmp(propType, "Cloth") ? 1 : -1);
            DynArray<SJointProperty> jp = CDefaultSkeleton::GetPhysInfoProperties_ROPE(attach.m_AttPhysInfo[0], nRopeOrGrid); //just write the names into jp

            for (int nLod = 0; nLod < 2; nLod++)
            {
                bool lodUsed = false;
                uint32 numRopeJoints = jp.size();
                for (uint32 idx = 1; idx < numRopeJoints; idx++)
                {
                    char buf[32];
                    sprintf_s(buf, "lod%d_%s", nLod, jp[idx].name);
                    if (jp[idx].type == 0)
                    {
                        lodUsed |= nodeAttach->getAttr(buf, jp[idx].fval);
                    }
                    else
                    {
                        lodUsed |= nodeAttach->getAttr(buf, jp[idx].bval);
                    }
                }
                if (lodUsed)
                {
                    CDefaultSkeleton::ParsePhysInfoProperties_ROPE(attach.m_AttPhysInfo[nLod], jp);  //just init m_PhysInfo from jp
                }
            }
        }

        uint32 flags;
        if (nodeAttach->getAttr("Flags", flags))
        {
            attach.m_AttFlags = flags;
        }

        if (attach.m_Type == CA_VCLOTH)
        {
            nodeAttach->getAttr("hide", attach.clothParams.hide);
            //nodeAttach->getAttr( "debug", attach.clothParams.debug );
            nodeAttach->getAttr("thickness", attach.clothParams.thickness);
            nodeAttach->getAttr("collsionDamping", attach.clothParams.collisionDamping);
            //nodeAttach->getAttr( "SA_Mass", attach.clothParams.dragDamping );
            nodeAttach->getAttr("stretchStiffness", attach.clothParams.stretchStiffness);
            nodeAttach->getAttr("shearStiffness", attach.clothParams.shearStiffness);
            nodeAttach->getAttr("bendStiffness", attach.clothParams.bendStiffness);
            nodeAttach->getAttr("numIterations", attach.clothParams.numIterations);
            nodeAttach->getAttr("timeStep", attach.clothParams.timeStep);
            nodeAttach->getAttr("rigidDamping", attach.clothParams.rigidDamping);
            nodeAttach->getAttr("translationBlend", attach.clothParams.translationBlend);
            nodeAttach->getAttr("rotationBlend", attach.clothParams.rotationBlend);
            nodeAttach->getAttr("friction", attach.clothParams.friction);
            nodeAttach->getAttr("pullStiffness", attach.clothParams.pullStiffness);
            nodeAttach->getAttr("SA_Mass", attach.clothParams.tolerance);
            nodeAttach->getAttr("maxBlendWeight", attach.clothParams.maxBlendWeight);
            nodeAttach->getAttr("maxAnimDistance", attach.clothParams.maxAnimDistance);
            //nodeAttach->getAttr( "SA_Mass", attach.clothParams.windBlend );
            //nodeAttach->getAttr( "SA_Mass", attach.clothParams.collDampingRange );
            nodeAttach->getAttr("stiffnessGradient", attach.clothParams.stiffnessGradient);
            nodeAttach->getAttr("halfStretchIterations", attach.clothParams.halfStretchIterations);
            nodeAttach->getAttr("isMainCharacter", attach.clothParams.isMainCharacter);
            attach.clothParams.simMeshName = nodeAttach->getAttr("simMeshName");
            attach.clothParams.renderMeshName = nodeAttach->getAttr("renderMeshName");
            attach.clothParams.simBinding = nodeAttach->getAttr("SimBinding");
            attach.clothParams.renderBinding = nodeAttach->getAttr("Binding");
        }

        if (attach.m_Type == CA_PROW)
        {
            attach.m_strRowJointName = nodeAttach->getAttr("RowJointName");

            uint32 nPendulumClampMode = 0;
            uint32 isPendulum = nodeAttach->getAttr("ROW_ClampMode", nPendulumClampMode);
            if (isPendulum)
            {
                attach.rowap.m_nClampMode = RowSimulationParams::ClampMode(nPendulumClampMode);
                nodeAttach->getAttr("ROW_FPS",                  attach.rowap.m_nSimFPS);
                nodeAttach->getAttr("ROW_ConeAngle",            attach.rowap.m_fConeAngle);
                nodeAttach->getAttr("ROW_ConeRotation",         attach.rowap.m_vConeRotation);

                nodeAttach->getAttr("ROW_Mass",                 attach.rowap.m_fMass);
                nodeAttach->getAttr("ROW_Gravity",              attach.rowap.m_fGravity);
                nodeAttach->getAttr("ROW_Damping",              attach.rowap.m_fDamping);
                nodeAttach->getAttr("ROW_JointSpring",          attach.rowap.m_fJointSpring);
                nodeAttach->getAttr("ROW_RodLength",            attach.rowap.m_fRodLength);
                nodeAttach->getAttr("ROW_StiffnessTarget",      attach.rowap.m_vStiffnessTarget);
                nodeAttach->getAttr("ROW_Turbulence",           attach.rowap.m_vTurbulence);
                nodeAttach->getAttr("ROW_MaxVelocity",          attach.rowap.m_fMaxVelocity);

                nodeAttach->getAttr("ROW_WorldSpaceDamping",    attach.rowap.m_worldSpaceDamping);

                nodeAttach->getAttr("ROW_Cycle",                attach.rowap.m_cycle);
                nodeAttach->getAttr("ROW_RelaxLoops",           attach.rowap.m_relaxationLoops);
                nodeAttach->getAttr("ROW_Stretch",              attach.rowap.m_fStretch);

                nodeAttach->getAttr("ROW_CapsuleX",             attach.rowap.m_vCapsule.x);
                nodeAttach->getAttr("ROW_CapsuleY",             attach.rowap.m_vCapsule.y);
                nodeAttach->getAttr("ROW_ProjectionType",       attach.rowap.m_nProjectionType);

                char proxytag[] = "ROW_Proxy00";
                for (uint32 i = 0; i < SimulationParams::MaxCollisionProxies; i++)
                {
                    CCryName strProxyName = CCryName(nodeAttach->getAttr(proxytag));
                    proxytag[10]++;
                    if (strProxyName.empty())
                    {
                        continue;
                    }
                    attach.rowap.m_arrProxyNames.push_back(strProxyName);
                }
            }
        }
        parrAttachments[numValidAttachments++] = attach;
    }
    return numValidAttachments;
}

//-----------------------------------------------------------
//init attachment-list
//-----------------------------------------------------------
void CAttachmentManager::InitAttachmentList(const DynArray<CharacterAttachment>& parrAttachments, const AZStd::string& pathname, uint32 nLoadingFlags, int nKeepModelInMemory)
{
    uint32 nLogWarnings = (nLoadingFlags & CA_DisableLogWarnings) == 0;
    CSkeletonPose& rSkelPose = (CSkeletonPose&)*m_pSkelInstance->GetISkeletonPose();
    CDefaultSkeleton& rDefaultSkeleton = *m_pSkelInstance->m_pDefaultSkeleton;

    bool bHasVertexAnimation = false;
    bool bUseMatrixSkinning = false;

    CCharInstance* pICharacter = m_pSkelInstance;
    uint32 numAttachments = parrAttachments.size();

    for (uint32 i = 0; i < numAttachments; i++)
    {
        // copy attachment, so we don't rely on vector-s memory
        const CharacterAttachment& attach = parrAttachments[i];
        uint32 nSizeOfBindingFilePath = attach.m_strBindingPath.size();

        const char* fileExt = PathUtil::GetExt(attach.m_strBindingPath);
        bool IsCDF = (0 == _stricmp(fileExt, "cdf"));
        bool IsCGA = (0 == _stricmp(fileExt, "cga"));
        bool IsCGF = (0 == _stricmp(fileExt, "cgf"));

        bool IsSKEL = (0 == _stricmp(fileExt, CRY_SKEL_FILE_EXT));
        bool IsSKIN = (0 == _stricmp(fileExt, CRY_SKIN_FILE_EXT));

        if (attach.m_Type == CA_BONE)
        {
            CAttachmentBONE* pAttachment = (CAttachmentBONE*)CreateAttachment(attach.m_strAttachmentName, CA_BONE, attach.m_strJointName.c_str());
            if (pAttachment == 0)
            {
                continue;
            }

            QuatT defaultTransform = rDefaultSkeleton.GetDefaultAbsJointByID(pAttachment->m_nJointID);
            QuatT qt = attach.m_AbsoluteDefault;
            QuatT rel = defaultTransform.GetInverted() * qt;
            if (attach.m_relPosition)
            {
                rel.t = attach.m_RelativeDefault.t;
            }
            if (attach.m_relRotation)
            {
                rel.q = attach.m_RelativeDefault.q;
            }
            qt = defaultTransform * rel;
            pAttachment->SetAttAbsoluteDefault(qt);

            pAttachment->SetFlags(attach.m_AttFlags);
            pAttachment->HideAttachment(attach.m_AttFlags & FLAGS_ATTACH_HIDE_ATTACHMENT);
            SimulationParams& ap = pAttachment->GetSimulationParams();
            ap = attach.ap;
            CSkeletonPose& rSkelPose = (CSkeletonPose&)*m_pSkelInstance->GetISkeletonPose();

            if (IsSKEL || IsCGA || IsCDF)
            {
                ICharacterInstance* pIChildCharacter = g_pCharacterManager->CreateInstance(attach.m_strBindingPath, nLoadingFlags);
                if (pIChildCharacter == 0 && nSizeOfBindingFilePath && nLogWarnings)
                {
                    g_pILog->LogError ("CryAnimation: no character as attachment created: %s", pathname.c_str());
                }

                if (pIChildCharacter)
                {
                    CSKELAttachment* pCharacterAttachment = new CSKELAttachment();
                    pCharacterAttachment->m_pCharInstance  = pIChildCharacter;
                    IAttachmentObject* pIAttachmentObject = (IAttachmentObject*)pCharacterAttachment;
                    pAttachment->AddBinding(pIAttachmentObject);
                    if (attach.m_pMaterial && pIAttachmentObject)
                    {
                        pIAttachmentObject->SetReplacementMaterial(attach.m_pMaterial);
                    }
                    bHasVertexAnimation |= pIChildCharacter->HasVertexAnimation();
                    bUseMatrixSkinning |= pIChildCharacter->UseMatrixSkinning();
                }
            }
            if (IsCGF)
            {
                _smart_ptr<IStatObj> pIStatObj = g_pISystem->GetI3DEngine()->LoadStatObjAutoRef(attach.m_strBindingPath, 0, 0, true, 0);
                if (pIStatObj == 0 && nSizeOfBindingFilePath && nLogWarnings)
                {
                    g_pILog->LogError ("CryAnimation: no static object as attachment created: %s", pathname.c_str());
                }

                if (pIStatObj)
                {
                    CCGFAttachment* pStatAttachment = new CCGFAttachment();
                    pStatAttachment->pObj  = pIStatObj;
                    IAttachmentObject* pIAttachmentObject = (IAttachmentObject*)pStatAttachment;
                    pAttachment->AddBinding(pIAttachmentObject);
                    if (attach.m_pMaterial && pIAttachmentObject)
                    {
                        pIAttachmentObject->SetReplacementMaterial(attach.m_pMaterial);
                    }
                }
            }


            //this should have its own type; its not really an attachment
            for (int nLod = 0; nLod < 2; nLod++)
            {
                if (*(alias_cast<const int*>(attach.m_AttPhysInfo[nLod].spring_angle)) == 0x12345678)
                {
                    pICharacter->m_SkeletonPose.m_physics.SetJointPhysInfo(pAttachment->GetJointID(), attach.m_AttPhysInfo[nLod], nLod);
                }
            }
        }

        //-----------------------------------------------------------------------------------

        if (attach.m_Type == CA_FACE)
        {
            CAttachmentFACE* pAttachment = (CAttachmentFACE*)CreateAttachment(attach.m_strAttachmentName, CA_FACE, 0, false);
            if (pAttachment == 0)
            {
                continue;
            }

            pAttachment->SetAttAbsoluteDefault(attach.m_AbsoluteDefault);
            pAttachment->SetFlags(attach.m_AttFlags);
            pAttachment->HideAttachment(attach.m_AttFlags & FLAGS_ATTACH_HIDE_ATTACHMENT);
            SimulationParams& ap = pAttachment->GetSimulationParams();
            ap = attach.ap;

            if (IsSKEL || IsCGA || IsCDF)
            {
                ICharacterInstance* pIChildCharacter = g_pCharacterManager->CreateInstance(attach.m_strBindingPath, nLoadingFlags);
                if (pIChildCharacter == 0 && nSizeOfBindingFilePath && nLogWarnings)
                {
                    g_pILog->LogError ("CryAnimation: no character created: %s", pathname.c_str());
                }

                if (pIChildCharacter)
                {
                    CSKELAttachment* pCharacterAttachment = new CSKELAttachment();
                    pCharacterAttachment->m_pCharInstance  = pIChildCharacter;
                    IAttachmentObject* pIAttachmentObject = (IAttachmentObject*)pCharacterAttachment;
                    pAttachment->AddBinding(pIAttachmentObject);
                    if (attach.m_pMaterial && pIAttachmentObject)
                    {
                        pIAttachmentObject->SetReplacementMaterial(attach.m_pMaterial);
                    }
                }
            }
            if (IsCGF)
            {
                _smart_ptr<IStatObj> pIStatObj = g_pISystem->GetI3DEngine()->LoadStatObjAutoRef(attach.m_strBindingPath, 0, 0, true, 0);
                if (pIStatObj == 0 && nSizeOfBindingFilePath && nLogWarnings)
                {
                    g_pILog->LogError ("CryAnimation: no static object as attachment created: %s", pathname.c_str());
                }

                if (pIStatObj)
                {
                    CCGFAttachment* pStatAttachment = new CCGFAttachment();
                    pStatAttachment->pObj  = pIStatObj;
                    IAttachmentObject* pIAttachmentObject = (IAttachmentObject*)pStatAttachment;
                    pAttachment->AddBinding(pIAttachmentObject);
                    if (attach.m_pMaterial && pIAttachmentObject)
                    {
                        pIAttachmentObject->SetReplacementMaterial(attach.m_pMaterial);
                    }
                }
            }
        }

        //-----------------------------------------------------------------------------------

        if (attach.m_Type == CA_SKIN)
        {
            CAttachmentSKIN* pAttachment = (CAttachmentSKIN*)CreateAttachment(attach.m_strAttachmentName, CA_SKIN);
            if (pAttachment == 0)
            {
                continue;
            }

            if (IsSKIN == 0 && attach.m_strBindingPath.size() > 0)
            {
                g_pILog->LogError ("CryAnimation: a skin-attachment must be a SKIN-file. You can't use this file: '%s' in attachment '%s'", attach.m_strBindingPath.c_str(), attach.m_strAttachmentName.c_str());
            }

            if (IsSKIN)
            {
                _smart_ptr<ISkin> pIModelSKIN = g_pCharacterManager->LoadModelSKINAutoRef(attach.m_strBindingPath, nLoadingFlags);
                if (pIModelSKIN == nullptr && nLogWarnings)
                {
                    g_pILog->LogError ("CryAnimation: skin-attachment not created: CDF: %s  SKIN: %s", pathname.c_str(), attach.m_strBindingPath.c_str());
                }

                if (pIModelSKIN)
                {
                    CSKINAttachment* pSkinInstance = new CSKINAttachment();
                    pSkinInstance->m_pIAttachmentSkin = pAttachment;
                    IAttachmentObject* pIAttachmentObject = (IAttachmentObject*)pSkinInstance;
                    if (pAttachment->AddBinding(pIAttachmentObject, pIModelSKIN, nLoadingFlags))
                    {
                        pAttachment->SetFlags(attach.m_AttFlags);
                        bHasVertexAnimation |= (attach.m_AttFlags & FLAGS_ATTACH_SW_SKINNING) != 0;
                        bUseMatrixSkinning |= (attach.m_AttFlags & FLAGS_ATTACH_MATRIX_SKINNING) != 0;
                        pAttachment->HideAttachment(attach.m_AttFlags & FLAGS_ATTACH_HIDE_ATTACHMENT);

                        if (pIAttachmentObject)
                        {
                            if (attach.m_pMaterial)
                            {
                                for (uint32 nLOD = 0; nLOD < g_nMaxGeomLodLevels; nLOD++)
                                {
                                    pIAttachmentObject->SetReplacementMaterial(attach.m_pMaterial, nLOD);
                                }
                            }
                            for (uint32 nLOD = 0; nLOD < g_nMaxGeomLodLevels; nLOD++)
                            {
                                if (attach.m_parrMaterial[nLOD])
                                {
                                    pIAttachmentObject->SetReplacementMaterial(attach.m_parrMaterial[nLOD], nLOD);
                                }
                            }

                            pAttachment->m_vertexAnimation.ClearFrameStates();
                            if (const CModelMesh* pModelMesh = ((CSkin*)pIModelSKIN.get())->GetModelMesh(0))
                            {
                                pAttachment->m_vertexAnimation.CreateFrameStates(pModelMesh->m_softwareMesh.GetVertexFrames(), *(pICharacter->m_pDefaultSkeleton.get()));
                            }
                        }
                    }

                    if (nKeepModelInMemory == 0 || nKeepModelInMemory == 1)
                    {
                        ISkin* pISKIN = pIModelSKIN.get();
                        CSkin* pCSKIN = (CSkin*)pISKIN;
                        if (nKeepModelInMemory)
                        {
                            pCSKIN->SetKeepInMemory(true);
                        }
                        else
                        {
                            pCSKIN->SetKeepInMemory(false);
                        }
                    }
                }
            }
        }

        //-------------------------------------------------------------------------

        if (attach.m_Type == CA_PROX)
        {
            QuatT qt = attach.m_AbsoluteDefault;
            int32 jointid = rDefaultSkeleton.GetJointIDByName(attach.m_strJointName);
            if (jointid >= 0)
            {
                QuatT defaultTransform = rDefaultSkeleton.GetDefaultAbsJointByID(jointid);
                QuatT rel = defaultTransform.GetInverted() * qt;
                if (attach.m_relPosition)
                {
                    rel.t = attach.m_RelativeDefault.t;
                }
                if (attach.m_relRotation)
                {
                    rel.q = attach.m_RelativeDefault.q;
                }
                qt = defaultTransform * rel;
            }
            CProxy proxy;
            proxy.m_pAttachmentManager   = this;
            proxy.m_strProxyName         = attach.m_strAttachmentName;
            proxy.m_nProxyCRC32          = CCrc32::ComputeLowercase(proxy.m_strProxyName.c_str());
            proxy.m_strJointName         = attach.m_strJointName;
            proxy.m_ProxyAbsoluteDefault = qt;
            proxy.m_params               = attach.m_ProxyParams;
            proxy.m_nPurpose             = attach.m_ProxyPurpose;
            m_arrProxies.push_back(proxy);
        }

        //-------------------------------------------------------------------------
        if (attach.m_Type == CA_PROW)
        {
            CAttachmentPROW* pCAttachmentPROW = (CAttachmentPROW*)CreateAttachment(attach.m_strAttachmentName, CA_PROW, attach.m_strRowJointName.c_str());
            if (pCAttachmentPROW == 0)
            {
                continue;
            }
            (RowSimulationParams&)pCAttachmentPROW->m_rowparams = attach.rowap;
        }
        //-------------------------------------------------------------------------
        if (attach.m_Type == CA_VCLOTH)
        {
            CAttachmentVCLOTH* pCAttachmentVCloth = (CAttachmentVCLOTH*)CreateAttachment(attach.m_strAttachmentName, CA_VCLOTH);
            if (pCAttachmentVCloth == 0)
            {
                continue;
            }

            if (IsSKIN == 0)
            {
                g_pILog->LogError ("CryAnimation: a rendermesh for vertex-cloth must be a SKIN-file. You can't use this file: %s", pathname.c_str());
            }

            const char* fileExt = PathUtil::GetExt(attach.m_strSimBindingPath);
            bool IsSimSKIN = (0 == _stricmp(fileExt, CRY_SKIN_FILE_EXT));
            if (IsSimSKIN == 0)
            {
                g_pILog->LogError ("CryAnimation: a simulation-mesh must be a SKIN-file. You can't use this file: %s", pathname.c_str());
            }

            if (IsSKIN && IsSimSKIN)
            {
                _smart_ptr<ISkin> pIModelSKIN = g_pCharacterManager->LoadModelSKINAutoRef(attach.m_strBindingPath, nLoadingFlags);
                if (pIModelSKIN == 0 && nLogWarnings)
                {
                    g_pILog->LogError ("CryAnimation: skin-attachment not created: CDF: %s  SKIN: %s", pathname.c_str(), attach.m_strBindingPath.c_str());
                }

                _smart_ptr<ISkin> pIModelSimSKIN = g_pCharacterManager->LoadModelSKINAutoRef(attach.m_strSimBindingPath, nLoadingFlags);
                if (pIModelSimSKIN == 0 && nLogWarnings)
                {
                    g_pILog->LogError ("CryAnimation: skin-attachment not created: CDF: %s  SKIN: %s", pathname.c_str(), attach.m_strSimBindingPath.c_str());
                }

                if (pIModelSKIN && pIModelSimSKIN)
                {
                    CSKINAttachment* pSkinInstance = new CSKINAttachment();
                    pSkinInstance->m_pIAttachmentSkin = pCAttachmentVCloth;
                    IAttachmentObject* pIAttachmentObject = (IAttachmentObject*)pSkinInstance;
                    if (pCAttachmentVCloth->AddBinding(pIAttachmentObject, pIModelSKIN, nLoadingFlags))
                    {
                        pCAttachmentVCloth->SetFlags(attach.m_AttFlags | FLAGS_ATTACH_SW_SKINNING);
                        bHasVertexAnimation = true;
                        pCAttachmentVCloth->HideAttachment(attach.m_AttFlags & FLAGS_ATTACH_HIDE_ATTACHMENT);

                        if (pIAttachmentObject)
                        {
                            if (attach.m_pMaterial)
                            {
                                for (uint32 nLOD = 0; nLOD < g_nMaxGeomLodLevels; nLOD++)
                                {
                                    pIAttachmentObject->SetReplacementMaterial(attach.m_pMaterial, nLOD);
                                }
                            }
                            for (uint32 nLOD = 0; nLOD < g_nMaxGeomLodLevels; nLOD++)
                            {
                                if (attach.m_parrMaterial[nLOD])
                                {
                                    pIAttachmentObject->SetReplacementMaterial(attach.m_parrMaterial[nLOD], nLOD);
                                }
                            }

                            if (const CModelMesh* pModelMesh = ((CSkin*)pIModelSKIN.get())->GetModelMesh(0))
                            {
                                pCAttachmentVCloth->m_vertexAnimation.CreateFrameStates(pModelMesh->m_softwareMesh.GetVertexFrames(), *(pICharacter->m_pDefaultSkeleton.get()));
                            }
                        }
                        pCAttachmentVCloth->AddSimBinding(*pIModelSimSKIN.get(), nLoadingFlags);
                        pCAttachmentVCloth->AddClothParams(attach.clothParams);
                        pCAttachmentVCloth->ComputeClothCacheKey();
                    }

                    if (nKeepModelInMemory == 0 || nKeepModelInMemory == 1)
                    {
                        static_cast<CSkin*>(pIModelSKIN.get())->SetKeepInMemory(nKeepModelInMemory != 0);
                        static_cast<CSkin*>(pIModelSimSKIN.get())->SetKeepInMemory(nKeepModelInMemory != 0);
                    }
                }
            }
        }
    }
    pICharacter->m_bHasVertexAnimation = bHasVertexAnimation;
    pICharacter->m_bUseMatrixSkinning = bUseMatrixSkinning;

    uint32 count = GetAttachmentCount();
    ProjectAllAttachment();
    VerifyProxyLinks();

    uint32 nproxies = m_arrProxies.size();
    for (uint32 i = 0; i < nproxies; i++)
    {
        int idx = m_arrProxies[i].m_nJointID;
        m_arrProxies[i].m_ProxyModelRelative = rDefaultSkeleton.GetDefaultAbsJointByID(idx) * m_arrProxies[i].m_ProxyRelativeDefault;
        m_arrProxies[i].m_ProxyModelRelativePrev = m_arrProxies[i].m_ProxyModelRelative;
    }

    uint32 numAttachmnets = GetAttachmentCount();
    for (uint32 i = 0; i < numAttachmnets; i++)
    {
        IAttachment*  pIAttachment = GetInterfaceByIndex(i);
        if (pIAttachment->GetType() == CA_BONE)
        {
            int32 id = pIAttachment->GetJointID();
            const char* jname = rDefaultSkeleton.GetJointNameByID(id);
            pIAttachment->PostUpdateSimulationParams(1, jname);
        }
        if (pIAttachment->GetType() == CA_FACE)
        {
            pIAttachment->PostUpdateSimulationParams(1);
        }
        if (pIAttachment->GetType() == CA_PROW)
        {
            pIAttachment->PostUpdateSimulationParams(1);
        }
    }

    m_TypeSortingRequired++;
}








IAttachment* CAttachmentManager::CreateAttachment(const char* szAttName, uint32 type, const char* szFirstJointName, bool bCallProject, const char* szSecondJointName)
{
    string strAttachmentName = szAttName;
    strAttachmentName.MakeLower();

    IAttachment* pIAttachmentName = GetInterfaceByName(strAttachmentName.c_str());
    if (pIAttachmentName)
    {
        CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_ERROR, "CryAnimation: Attachment name '%s' is already in use, attachment will not be created", strAttachmentName.c_str());
        return 0;
    }

    uint32 nameCRC = CCrc32::ComputeLowercase(strAttachmentName.c_str());
    IAttachment* pIAttachmentCRC32 = GetInterfaceByNameCRC(nameCRC);
    if (pIAttachmentCRC32)
    {
        CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_ERROR, "CryAnimation: Attachment CRC32 for '%s' clashes with attachment name '%s' (crc's are created using lower case only), attachment will not be created", strAttachmentName.c_str(), pIAttachmentCRC32->GetName());
        return 0;
    }

    CDefaultSkeleton& rDefaultSkeleton = *m_pSkelInstance->m_pDefaultSkeleton;

    //----------------------------------------------------------------------------------
    if (type == CA_BONE)
    {
        if (szFirstJointName == 0)
        {
            return 0;
        }

        CAttachmentBONE* pAttachment = new CAttachmentBONE();
        pAttachment->m_pAttachmentManager = this;
        pAttachment->m_strSocketName = strAttachmentName.c_str();
        pAttachment->m_nSocketCRC32 = nameCRC;
        pAttachment->m_strJointName = szFirstJointName;
        pAttachment->ProjectAttachment(nullptr, szSecondJointName);
        m_arrAttachments.push_back(pAttachment);
        m_TypeSortingRequired++;
        return pAttachment;
    }

    if (type == CA_FACE)
    {
        CAttachmentFACE* pAttachment = new CAttachmentFACE();
        pAttachment->m_pAttachmentManager = this;
        pAttachment->m_strSocketName = strAttachmentName.c_str();
        pAttachment->m_nSocketCRC32 = nameCRC;
        //  if( bCallProject )
        //      pAttachment->ProjectAttachment();
        m_arrAttachments.push_back(pAttachment);
        m_TypeSortingRequired++;
        return pAttachment;
    }

    if (type == CA_SKIN)
    {
        CAttachmentSKIN* pAttachment = new CAttachmentSKIN();
        pAttachment->m_pAttachmentManager = this;
        pAttachment->m_strSocketName = strAttachmentName.c_str();
        pAttachment->m_nSocketCRC32 = nameCRC;
        m_arrAttachments.push_back(pAttachment);
        m_TypeSortingRequired++;
        return pAttachment;
    }

    if (type == CA_PROW)
    {
        if (szFirstJointName == 0)
        {
            return 0;
        }
        CAttachmentPROW* pAttachment = new CAttachmentPROW();
        pAttachment->m_pAttachmentManager = this;
        pAttachment->m_strSocketName = strAttachmentName.c_str();
        pAttachment->m_nSocketCRC32 = nameCRC;
        pAttachment->m_strRowJointName = szFirstJointName;
        m_arrAttachments.push_back(pAttachment);
        m_TypeSortingRequired++;
        return pAttachment;
    }

    if (type == CA_VCLOTH)
    {
        CAttachmentVCLOTH* pAttachment = new CAttachmentVCLOTH();
        pAttachment->m_pAttachmentManager = this;
        pAttachment->m_strSocketName = strAttachmentName.c_str();
        pAttachment->m_nSocketCRC32 = nameCRC;
        m_arrAttachments.push_back(pAttachment);
        m_TypeSortingRequired++;
        return pAttachment;
    }
    m_TypeSortingRequired++;
    return 0;
};




ICharacterInstance* CAttachmentManager::GetSkelInstance() const
{
    return m_pSkelInstance;
}

float CAttachmentManager::GetExtent(EGeomForm eForm)
{
    if (m_arrAttachments.empty())
    {
        return 0.f;
    }

    CGeomExtent& extent = m_Extents.Make(eForm);
    if (!extent)
    {
        // Add attachments.
        extent.ReserveParts(m_arrAttachments.size());

        // Add attachments.
        for_array (i, m_arrAttachments)
        {
            float fExt = 0.f;
            if (IAttachment* pa = m_arrAttachments[i])
            {
                if (IAttachmentObject* pAttachmentObject = pa->GetIAttachmentObject())
                {
                    if (ICharacterInstance* pSkinInstance = pAttachmentObject->GetICharacterInstance())
                    {
                        fExt = pSkinInstance->GetExtent(eForm);
                    }
                    else if (IStatObj* pStatObj = pAttachmentObject->GetIStatObj())
                    {
                        fExt = pStatObj->GetExtent(eForm);
                    }
                    else if (IAttachmentSkin* pSkin = pAttachmentObject->GetIAttachmentSkin())
                    {
                        fExt = pSkin->GetExtent(eForm);
                    }
                }
            }
            extent.AddPart(fExt);
        }
    }

    return extent.TotalExtent();
}

void CAttachmentManager::GetRandomPos(PosNorm& ran, EGeomForm eForm) const
{
    ran.zero();

    CGeomExtent const& ext = m_Extents[eForm];
    int iPart = ext.RandomPart();
    if (iPart < m_arrAttachments.size())
    {
        // Choose attachment.
        if (IAttachment* pAttachment = m_arrAttachments[iPart])
        {
            if (IAttachmentObject* pAttachmentObject = pAttachment->GetIAttachmentObject())
            {
                if (ICharacterInstance* pCharInstance = pAttachmentObject->GetICharacterInstance())
                {
                    pCharInstance->GetRandomPos(ran, eForm);
                }
                else if (IStatObj* pStatObj = pAttachmentObject->GetIStatObj())
                {
                    pStatObj->GetRandomPos(ran, eForm);
                }
                else if (IAttachmentSkin* pSkin = pAttachmentObject->GetIAttachmentSkin())
                {
                    pSkin->GetRandomPos(ran, eForm);
                }
            }
            ran <<= QuatTS(pAttachment->GetAttModelRelative());
        }
    }
}


void CAttachmentManager::PhysicalizeAttachment(int idx, IPhysicalEntity* pent, int nLod)
{
    if (!pent)
    {
        if (!(pent = m_pSkelInstance->GetISkeletonPose()->GetCharacterPhysics()))
        {
            return;
        }
    }
    PhysicalizeAttachment(idx, nLod, pent, m_pSkelInstance->m_SkeletonPose.m_physics.GetOffset());
}



void CAttachmentManager::PhysicalizeAttachment(int idx, int nLod, IPhysicalEntity* pent, const Vec3& offset)
{
    IStatObj* pStatObj;
    IAttachment* pIAttachment = GetInterfaceByIndex(idx);
    if (pIAttachment == 0)
    {
        return;
    }

    bool bWasPhysicalized = false;
    if ((pIAttachment->GetFlags() & FLAGS_ATTACH_PHYSICALIZED) == 0)
    {
        if (pIAttachment->GetIAttachmentObject())
        {
            bWasPhysicalized = pIAttachment->GetIAttachmentObject()->PhysicalizeAttachment(this, idx, nLod, pent, offset);
        }
    }
    //
    if (bWasPhysicalized)
    {
        return;
    }

    // old path
    if (!(pIAttachment = GetInterfaceByIndex(idx)) || pIAttachment->GetType() != CA_BONE || !(pIAttachment->GetFlags() & FLAGS_ATTACH_PHYSICALIZED) ||
        !pIAttachment->GetIAttachmentObject() || !(pStatObj = pIAttachment->GetIAttachmentObject()->GetIStatObj()) || !pStatObj->GetPhysGeom() ||
        pIAttachment->IsAttachmentHidden())
    {
        return;
    }

    int iJoint = pIAttachment->GetJointID();
    pe_articgeomparams gp;
    const   Skeleton::CPoseData& rPoseData = m_pSkelInstance->m_SkeletonPose.GetPoseData();
    Matrix34 mtx = Matrix34(rPoseData.GetJointAbsolute(iJoint) * ((CAttachmentFACE*)pIAttachment)->m_AttRelativeDefault);
    mtx.AddTranslation(offset);
    gp.pMtx3x4 = &mtx;
    //FIXME:
    gp.idbody = m_pSkelInstance->GetISkeletonPose()->getBonePhysParentOrSelfIndex(iJoint, nLod);
    if (gp.idbody >= 0)
    {
        while ((iJoint = m_pSkelInstance->m_SkeletonPose.m_physics.getBonePhysParentIndex(gp.idbody, nLod)) >= 0 &&
               (m_pSkelInstance->m_SkeletonPose.m_physics.GetModelJointPointer(gp.idbody)->m_PhysInfo.flags & all_angles_locked) == all_angles_locked)
        {
            gp.idbody = iJoint;
        }
    }
    gp.flags = 0;
    if (pIAttachment->GetFlags() & FLAGS_ATTACH_PHYSICALIZED_COLLISIONS)
    {
        gp.flags = geom_colltype_solid | geom_colltype_solid | geom_floats | geom_colltype_explosion;
    }
    if (pIAttachment->GetFlags() & FLAGS_ATTACH_PHYSICALIZED_RAYS)
    {
        gp.flags |= geom_colltype_ray;
    }
    const char* pAName = pIAttachment->GetName();
    uint32 nCRC32 = CCrc32::ComputeLowercase(pAName) & 0x3fffffff;
    pent->AddGeometry(pStatObj->GetPhysGeom(), &gp, nCRC32);
    pIAttachment->SetFlags(pIAttachment->GetFlags() | FLAGS_ATTACH_WAS_PHYSICALIZED);
}


void CAttachmentManager::DephysicalizeAttachment(int idx, IPhysicalEntity* pent)
{
    if (!pent)
    {
        if (!(pent = m_pSkelInstance->GetISkeletonPose()->GetCharacterPhysics()))
        {
            return;
        }
    }
    IAttachment* pIAttachment = GetInterfaceByIndex(idx);
    uint32 nCRC32 = CCrc32::ComputeLowercase(pIAttachment->GetName()) & 0x3fffffff;
    pent->RemoveGeometry(nCRC32);
    if (pIAttachment)
    {
        pIAttachment->SetFlags(pIAttachment->GetFlags() & ~FLAGS_ATTACH_WAS_PHYSICALIZED);
    }
}

int CAttachmentManager::UpdatePhysicalizedAttachment(int idx, IPhysicalEntity* pent, const QuatT& offset)
{
    IStatObj* pStatObj;
    IAttachment* pIAttachment;

    bool WasHandled = false;
    if (!(pIAttachment = GetInterfaceByIndex(idx)) || !(pIAttachment->GetFlags() & FLAGS_ATTACH_PHYSICALIZED) ||
        !pIAttachment->GetIAttachmentObject() /* || !(pStatObj=pIAttachment->GetIAttachmentObject()->GetIStatObj()) || !pStatObj->GetPhysGeom()*/)
    {
        if (pIAttachment && pIAttachment->GetIAttachmentObject())
        {
            WasHandled = pIAttachment->GetIAttachmentObject()->UpdatePhysicalizedAttachment(this, idx, pent, offset);
        }
    }

    if (WasHandled)
    {
        return 0;
    }

    if (!(pIAttachment = GetInterfaceByIndex(idx)) || pIAttachment->GetType() != CA_BONE || !(pIAttachment->GetFlags() & FLAGS_ATTACH_PHYSICALIZED) ||
        !pIAttachment->GetIAttachmentObject() || !(pStatObj = pIAttachment->GetIAttachmentObject()->GetIStatObj()) || !pStatObj->GetPhysGeom())
    {
        return 0;
    }

    int changed = 0;
    if (pIAttachment->IsAttachmentHidden() && pIAttachment->GetFlags() & FLAGS_ATTACH_WAS_PHYSICALIZED)
    {
        DephysicalizeAttachment(idx, pent), changed = 1;
    }
    else if (!pIAttachment->IsAttachmentHidden() && !(pIAttachment->GetFlags() & FLAGS_ATTACH_WAS_PHYSICALIZED))
    {
        PhysicalizeAttachment(idx, 0, pent, offset.t), changed = 1;
    }

    pe_status_awake sa;
    if (!pent->GetStatus(&sa))
    {
        int iJoint = pIAttachment->GetJointID();
        pe_params_part pp;
        Matrix34 mtx = Matrix34(offset * pIAttachment->GetAttModelRelative());
        const char* pAName = pIAttachment->GetName();
        pp.partid = CCrc32::ComputeLowercase(pAName) & 0x3fffffff;
        pp.pMtx3x4 = &mtx;
        pp.bRecalcBBox = !m_pSkelInstance || !m_pSkelInstance->m_SkeletonPose.m_physics.GetModelJointPointer(idx)->m_PhysInfo.pPhysGeom;
        pent->SetParams(&pp);
    }
    return changed;
}

int CAttachmentManager::UpdatePhysAttachmentHideState(int idx, IPhysicalEntity* pent, const Vec3& offset)
{
    IStatObj* pStatObj;
    IAttachment* pIAttachment;

    if (!(pIAttachment = GetInterfaceByIndex(idx)) || pIAttachment->GetType() != CA_BONE || !(pIAttachment->GetFlags() & FLAGS_ATTACH_PHYSICALIZED) ||
        !pIAttachment->GetIAttachmentObject() || !(pStatObj = pIAttachment->GetIAttachmentObject()->GetIStatObj()) || !pStatObj->GetPhysGeom())
    {
        return 0;
    }

    if (pIAttachment->IsAttachmentHidden() && pIAttachment->GetFlags() & FLAGS_ATTACH_WAS_PHYSICALIZED)
    {
        DephysicalizeAttachment(idx, pent);
        return 2;
    }
    if (!pIAttachment->IsAttachmentHidden() && !(pIAttachment->GetFlags() & FLAGS_ATTACH_WAS_PHYSICALIZED))
    {
        PhysicalizeAttachment(idx, 0, pent, offset);
        return 1;
    }
    return 3;
}



int32 CAttachmentManager::RemoveAttachmentByInterface(const IAttachment* pIAttachment)
{
    const char* pName = pIAttachment->GetName();
    return RemoveAttachmentByName(pName);
}

int32 CAttachmentManager::RemoveAttachmentByName(const char* szName)
{
    int32 index = GetIndexByName(szName);
    if (index == -1)
    {
        return 0;
    }
    RemoveAttachmentByIndex(index);
    m_Extents.Clear();
    m_TypeSortingRequired++;
    return 1;
};

int32 CAttachmentManager::RemoveAttachmentByNameCRC(uint32 nameCRC)
{
    int32 index = GetIndexByNameCRC(nameCRC);
    if (index == -1)
    {
        return 0;
    }
    RemoveAttachmentByIndex(index);
    m_TypeSortingRequired++;
    return 1;
};

void CAttachmentManager::RemoveAttachmentByIndex(uint32 n)
{
    IAttachment* pIAttachment = GetInterfaceByIndex(n);
    if (!pIAttachment)
    {
        return;
    }

    CRY_ASSERT(pIAttachment->GetType() == CA_BONE || pIAttachment->GetType() == CA_FACE || pIAttachment->GetType() == CA_SKIN || pIAttachment->GetType() == CA_PROW || pIAttachment->GetType() == CA_VCLOTH);
    //
    if (pIAttachment->GetIAttachmentObject())
    {
        pIAttachment->GetIAttachmentObject()->OnRemoveAttachment(this, n);
    }
    size_t size = m_arrAttachments.size();
    if (size == 0)
    {
        return;
    }
    if (size <= n)
    {
        return;
    }
    m_arrAttachments[n]->ClearBinding();
    m_arrAttachments[n] = m_arrAttachments[size - 1];
    m_arrAttachments.pop_back();
    m_TypeSortingRequired++;
}

uint32 CAttachmentManager::ProjectAllAttachment()
{
    uint32 numAttachments = m_arrAttachments.size();
    for (uint32 i = 0; i < numAttachments; i++)
    {
        uint32 flags = m_arrAttachments[i]->GetFlags();
        flags &= (~FLAGS_ATTACH_PROJECTED);
        m_arrAttachments[i]->SetFlags(flags);
    }
    return 1;
}

IAttachment* CAttachmentManager::GetInterfaceByName(const char* szName) const
{
    int32 idx = GetIndexByName(szName);
    if (idx == -1)
    {
        return 0;
    }
    return GetInterfaceByIndex(idx);
};

IAttachment* CAttachmentManager::GetInterfaceByIndex(uint32 c) const
{
    size_t size = m_arrAttachments.size();
    if (size == 0)
    {
        return 0;
    }
    if (size <= c)
    {
        return 0;
    }
    return m_arrAttachments[c];
};

int32 CAttachmentManager::GetIndexByName(const char* szName) const
{
    const uint32 nameCRC = CCrc32::ComputeLowercase(szName);
    return GetIndexByNameCRC(nameCRC);
}

IAttachment* CAttachmentManager::GetInterfaceByNameCRC(uint32 nameCRC) const
{
    int32 idx = GetIndexByNameCRC(nameCRC);
    if (idx == -1)
    {
        return 0;
    }
    return GetInterfaceByIndex(idx);
};

int32 CAttachmentManager::GetIndexByNameCRC(uint32 nameCRC) const
{
    const int32 num = GetAttachmentCount();
    for (int32 i = 0; i < num; i++)
    {
        const IAttachment* pA = m_arrAttachments[i];
        if (pA->GetNameCRC() == nameCRC)
        {
            return i;
        }
    }
    return -1;
}

bool CAttachmentManager::NeedsHierarchicalUpdate()
{
    const uint32 numAttachments = GetAttachmentCount();
    for (uint32 i = 0; i < numAttachments; i++)
    {
        IAttachmentObject* pIAttachmentObject = m_arrAttachments[i]->GetIAttachmentObject();
        if (pIAttachmentObject == 0)
        {
            continue;
        }

        ICharacterInstance* pICharacterInstance = pIAttachmentObject->GetICharacterInstance();
        if (pICharacterInstance == 0)
        {
            continue;
        }

        IAnimationSet* pIAnimationSet = pICharacterInstance->GetIAnimationSet();
        uint32 numAnimations = pIAnimationSet->GetAnimationCount();
        if (numAnimations == 0)
        {
            continue;
        }

        ISkeletonAnim* pISkeletonAnim = pICharacterInstance->GetISkeletonAnim();
        uint32 numAnims = 0;
        for (uint32 l = 0; l < numVIRTUALLAYERS; l++)
        {
            numAnims += pISkeletonAnim->GetNumAnimsInFIFO(l);
        }
        if (numAnims)
        {
            return 1;
        }
    }

    return 0;
}





void CAttachmentManager::UpdateAllRemapTables()
{
    if (m_TypeSortingRequired && (GetAttachmentCount() != m_sortop[1][sm]))
    {
        SortByType();
    }

    for (uint32 r = 0; r < m_sortop[1][bx]; r++)
    {
        IAttachment* pIAttachment = m_arrAttachments[r];
        CAttachmentBONE* pCAttachmentBone = (CAttachmentBONE*)pIAttachment;
        pCAttachmentBone->m_nJointID = -1; //request re-projection
        pCAttachmentBone->m_Simulation.m_arrChildren.resize(0);
    }
    uint32 x = FLAGS_ATTACH_PROJECTED ^ -1;
    for (uint32 i = m_sortop[0][00]; i < m_sortop[1][sm]; i++)
    {
        m_arrAttachments[i]->SetFlags(m_arrAttachments[i]->GetFlags() & x);
    }
    for (uint32 i = m_sortop[0][sm]; i < m_sortop[1][sm]; i++)
    {
        IAttachment* pIAttachment = m_arrAttachments[i];
        CAttachmentSKIN* pCAttachmentSkin = (CAttachmentSKIN*)pIAttachment;
        pCAttachmentSkin->UpdateRemapTable();
    }

    SortByType();
    #ifdef EDITOR_PCDEBUGCODE
    Verification();
    #endif
}




void CAttachmentManager::VerifyProxyLinks()
{
    //verify and re-adjust proxy links
    uint32 numAttachmnets = GetAttachmentCount();
    for (uint32 a = 0; a < numAttachmnets; a++)
    {
        IAttachment* pIAttachment = GetInterfaceByIndex(a);

        if (pIAttachment->GetType() == CA_BONE)
        {
            CAttachmentBONE* pCAttachmentBone = (CAttachmentBONE*)pIAttachment;
            uint32 numUsedProxies = pCAttachmentBone->m_Simulation.m_arrProxyNames.size();
            if (numUsedProxies == 0)
            {
                continue;
            }

            string arrProxyNames[100];
            for (uint32 p = 0; p < numUsedProxies; p++)
            {
                arrProxyNames[p] = pCAttachmentBone->m_Simulation.m_arrProxyNames[p].c_str();
            }

            pCAttachmentBone->m_Simulation.m_arrProxyNames.resize(0);
            pCAttachmentBone->m_Simulation.m_arrProxyIndex.resize(0);
            uint32 numAllProxies = m_arrProxies.size();
            for (uint32 crc = 0; crc < numUsedProxies; crc++)
            {
                uint32 nCRC32lower = CCrc32::ComputeLowercase(arrProxyNames[crc].c_str());
                for (uint32 p = 0; p < numAllProxies; p++)
                {
                    if (nCRC32lower == m_arrProxies[p].m_nProxyCRC32)
                    {
                        pCAttachmentBone->m_Simulation.m_arrProxyNames.push_back(CCryName(arrProxyNames[crc]));
                        pCAttachmentBone->m_Simulation.m_arrProxyIndex.push_back(p);
                        break;
                    }
                }
            }
        }

        if (pIAttachment->GetType() == CA_FACE)
        {
            CAttachmentFACE* pCAttachmentFace = (CAttachmentFACE*)pIAttachment;
            uint32 numUsedProxies = pCAttachmentFace->m_Simulation.m_arrProxyNames.size();
            if (numUsedProxies == 0)
            {
                continue;
            }

            string arrProxyNames[100];
            for (uint32 p = 0; p < numUsedProxies; p++)
            {
                arrProxyNames[p] = pCAttachmentFace->m_Simulation.m_arrProxyNames[p].c_str();
            }

            pCAttachmentFace->m_Simulation.m_arrProxyNames.resize(0);
            pCAttachmentFace->m_Simulation.m_arrProxyIndex.resize(0);
            uint32 numAllProxies = m_arrProxies.size();
            for (uint32 crc = 0; crc < numUsedProxies; crc++)
            {
                uint32 nCRC32lower = CCrc32::ComputeLowercase(arrProxyNames[crc].c_str());
                for (uint32 p = 0; p < numAllProxies; p++)
                {
                    if (nCRC32lower == m_arrProxies[p].m_nProxyCRC32)
                    {
                        pCAttachmentFace->m_Simulation.m_arrProxyNames.push_back(CCryName(arrProxyNames[crc]));
                        pCAttachmentFace->m_Simulation.m_arrProxyIndex.push_back(p);
                        break;
                    }
                }
            }
        }
    }

    const CDefaultSkeleton& rDefaultSkeleton = *m_pSkelInstance->m_pDefaultSkeleton;
    uint32 numProxies = m_arrProxies.size();
    for (uint32 i = 0; i < numProxies; i++)
    {
        const char* strJointName = m_arrProxies[i].m_strJointName.c_str();
        int16 nJointID = rDefaultSkeleton.GetJointIDByName(strJointName);
        if (nJointID < 0)
        {
            CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_ERROR, "CryAnimation: Proxy '%s' specified wrong joint name '%s'", m_arrProxies[i].m_strProxyName.c_str(), strJointName);
            m_arrProxies.erase(m_arrProxies.begin() + i);
            numProxies = m_arrProxies.size();
            --i;
            continue;
        }
        m_arrProxies[i].m_nJointID = nJointID;
        QuatT jointQT = rDefaultSkeleton.GetDefaultAbsJointByID(nJointID);
        m_arrProxies[i].m_ProxyRelativeDefault = jointQT.GetInverted() * m_arrProxies[i].m_ProxyAbsoluteDefault;
    }
}

//special update for skinned joints
void CAttachmentManager::UpdateAllRedirectedTransformations(Skeleton::CPoseData& rPoseData)
{
    if (m_TypeSortingRequired)
    {
        SortByType();
    }
#ifdef EDITOR_PCDEBUGCODE
    Verification();
#endif

    const f32 fIPlaybackScale    = m_pSkelInstance->GetPlaybackScale();
    const f32 fLPlaybackScale    = m_pSkelInstance->m_SkeletonAnim.GetLayerPlaybackScale(0);
    const f32 fAverageFrameTime  = g_AverageFrameTime * fIPlaybackScale * fLPlaybackScale ? g_AverageFrameTime * fIPlaybackScale * fLPlaybackScale : g_AverageFrameTime;
    m_fTurbulenceGlobal += gf_PI * fAverageFrameTime, m_fTurbulenceLocal = 0;

    DEFINE_PROFILER_FUNCTION();
    const QuatTS& rPhysLocation = m_pSkelInstance->m_location;
    uint32 nproxies = m_arrProxies.size();
    for (uint32 i = 0; i < nproxies; i++)
    {
        int idx = m_arrProxies[i].m_nJointID;
        m_arrProxies[i].m_ProxyModelRelative = rPoseData.GetJointAbsolute(idx) * m_arrProxies[i].m_ProxyRelativeDefault;
#ifndef _RELEASE
        if (m_pSkelInstance->GetCharEditMode() == 3)
        {
            const Vec3 pos = rPoseData.GetJointAbsolute(idx).t;
            g_pAuxGeom->DrawLine(rPhysLocation * pos, RGBA8(0xff, 0x00, 0x00, 0xff), rPhysLocation * m_arrProxies[i].m_ProxyModelRelative.t, RGBA8(0x00, 0xff, 0x00, 0xff));
        }
#endif
    }

#ifdef EDITOR_PCDEBUGCODE
    if (m_nDrawProxies)
    {
        for (uint32 i = 0; i < nproxies; i++)
        {
            QuatTS wlocation = rPhysLocation * m_arrProxies[i].m_ProxyModelRelative;
            if (m_nDrawProxies & 2 && m_arrProxies[i].m_nHideProxy == 0 && m_arrProxies[i].m_nPurpose == 0)
            {
                m_arrProxies[i].Draw(wlocation, RGBA8(0xe7, 0xc0, 0xbc, 0xff), 16, m_pSkelInstance->m_Viewdir);
            }
            if (m_nDrawProxies & 4 && m_arrProxies[i].m_nHideProxy == 0 && m_arrProxies[i].m_nPurpose == 1)
            {
                m_arrProxies[i].Draw(wlocation, RGBA8(0xa0, 0xe7, 0x80, 0xff), 16, m_pSkelInstance->m_Viewdir);
            }
            if (m_nDrawProxies & 8 && m_arrProxies[i].m_nHideProxy == 0 && m_arrProxies[i].m_nPurpose == 2)
            {
                m_arrProxies[i].Draw(wlocation, RGBA8(0xff, 0xa0, 0x80, 0xff), 16, m_pSkelInstance->m_Viewdir);
            }
        }
    }
#endif


#ifdef EDITOR_PCDEBUGCODE
    //Verification();
#endif

    CAttachmentManager& pAttachmentManager = m_pSkelInstance->m_AttachmentManager;
    uint32 n = pAttachmentManager.m_arrProcExec.size();
    for (uint32 i = 0; i < n; i++)
    {
        if (pAttachmentManager.m_arrProcExec[i])
        {
            pAttachmentManager.m_arrProcExec[i]->Update_Redirected(rPoseData), pAttachmentManager.m_arrProcExec[i] = 0;
        }
    }

    for (uint32 i = 0; i < nproxies; i++)
    {
        m_arrProxies[i].m_ProxyModelRelativePrev = rPhysLocation.q * m_arrProxies[i].m_ProxyModelRelative;
        m_arrProxies[i].m_ProxyModelRelativePrev.q.Normalize();
    }

    for (uint32 i = m_sortop[0][vc]; i < m_sortop[1][vc]; i++)
    {
        // If our sorted indices are no longer valid, then bail out so we don't access m_arrAttachments out of bounds. (calling SortByType() here wouldn't make sense)
        if (m_TypeSortingRequired)
        {
            break;
        }

        IAttachment* pIAttachment   = m_arrAttachments[i];
        if (pIAttachment->GetType() != CA_PROW)
        {
            continue;
        }
        CAttachmentPROW* pCAttachmentPROW = (CAttachmentPROW*)pIAttachment;
        pCAttachmentPROW->UpdateRow(rPoseData);
    }
}







void CAttachmentManager::UpdateAllLocations(Skeleton::CPoseData& rPoseData)
{
    if (m_TypeSortingRequired)
    {
        SortByType();
    }
#ifdef EDITOR_PCDEBUGCODE
    Verification();
#endif


    for (uint32 i = m_sortop[0][be]; i < m_sortop[1][be]; i++)
    {
        IAttachment* pIAttachment = m_arrAttachments[i];
        CAttachmentBONE* pCAttachmentBone = (CAttachmentBONE*)pIAttachment;
        pCAttachmentBone->Update_Empty(rPoseData);
    }
    for (uint32 i = m_sortop[0][bs]; i < m_sortop[1][bs]; i++)
    {
        IAttachment* pIAttachment = m_arrAttachments[i];
        CAttachmentBONE* pCAttachmentBone = (CAttachmentBONE*)pIAttachment;
        pCAttachmentBone->Update_Static(rPoseData);
    }
    for (uint32 i = m_sortop[0][bx]; i < m_sortop[1][bx]; i++)
    {
        IAttachment* pIAttachment = m_arrAttachments[i];
        CAttachmentBONE* pCAttachmentBone = (CAttachmentBONE*)pIAttachment;
        pCAttachmentBone->Update_Execute(rPoseData);
    }

    if (m_pSkelInstance->m_nAnimationLOD < 1)
    {
        for (uint32 i = m_sortop[0][fe]; i < m_sortop[1][fe]; i++)
        {
            IAttachment* pIAttachment = m_arrAttachments[i];
            CAttachmentFACE* pCAttachmentFace = (CAttachmentFACE*)pIAttachment;
            pCAttachmentFace->Update_Empty(rPoseData);
        }
    }

    for (uint32 i = m_sortop[0][fs]; i < m_sortop[1][fs]; i++)
    {
        IAttachment* pIAttachment = m_arrAttachments[i];
        CAttachmentFACE* pCAttachmentFace = (CAttachmentFACE*)pIAttachment;
        pCAttachmentFace->Update_Static(rPoseData);
    }
    for (uint32 i = m_sortop[0][fx]; i < m_sortop[1][fx]; i++)
    {
        IAttachment* pIAttachment = m_arrAttachments[i];
        CAttachmentFACE* pCAttachmentFace = (CAttachmentFACE*)pIAttachment;
        pCAttachmentFace->Update_Execute(rPoseData);
    }
}


void CAttachmentManager::UpdateAllLocationsFast(Skeleton::CPoseData& rPoseData)
{
    if (m_TypeSortingRequired)
    {
        SortByType();
    }
#ifdef EDITOR_PCDEBUGCODE
    Verification();
#endif

    //only entities need an update when not on screen
    for (uint32 i = m_sortop[0][bx]; i < m_sortop[1][bx]; i++)
    {
        IAttachment* pIAttachment = m_arrAttachments[i];
        CAttachmentBONE* pCAttachmentBone = (CAttachmentBONE*)pIAttachment;
        pCAttachmentBone->Update_Execute(rPoseData);
    }

    for (uint32 i = m_sortop[0][fx]; i < m_sortop[1][fx]; i++)
    {
        IAttachment* pIAttachment = m_arrAttachments[i];
        CAttachmentFACE* pCAttachmentFace = (CAttachmentFACE*)pIAttachment;
        pCAttachmentFace->Update_Execute(rPoseData);
    }
}



uint32 CAttachmentManager::RemoveAllAttachments()
{
    uint32 counter = GetAttachmentCount();
    for (uint32 i = 0; i < counter; i++)
    {
        m_arrAttachments[i]->ClearBinding(CA_SkipSkelRecreation);
    }
    for (uint32 i = counter; i > 0; i--)
    {
        RemoveAttachmentByIndex(i - 1);
    }
    return 1;
}


void CAttachmentManager::Serialize(TSerialize ser)
{
    if (ser.GetSerializationTarget() != eST_Network)
    {
        ser.BeginGroup("CAttachmentManager");
        int32 numAttachments = GetAttachmentCount();
        for (int32 i = 0; i < numAttachments; i++)
        {
            GetInterfaceByIndex(i)->Serialize(ser);
        }
        ser.EndGroup();
    }
}



#if !defined(_RELEASE)
void VisualizeEmptySocket(const Matrix34& WorldMat34, const QuatT& rAttModelRelative, ColorB col)
{
    g_pAuxGeom->SetRenderFlags(e_Def3DPublicRenderflags);
    Matrix34 FinalMat34 = WorldMat34 * Matrix34(rAttModelRelative);
    Vec3 pos =  FinalMat34.GetTranslation();
    static Ang3 angle1(0, 0, 0);
    static Ang3 angle2(0, 0, 0);
    static Ang3 angle3(0, 0, 0);
    angle1 += Ang3(0.01f, +0.02f, +0.03f);
    angle2 -= Ang3(0.01f, -0.02f, -0.03f);
    angle3 += Ang3(0.03f, -0.02f, +0.01f);

    AABB aabb1 = AABB(Vec3(-0.05f, -0.05f, -0.05f), Vec3(+0.05f, +0.05f, +0.05f));
    AABB aabb2 = AABB(Vec3(-0.005f, -0.005f, -0.005f), Vec3(+0.005f, +0.005f, +0.005f));

    Matrix33 m33;
    OBB obb;
    m33 = Matrix33::CreateRotationXYZ(angle1);
    obb = OBB::CreateOBBfromAABB(m33, aabb1);
    g_pAuxGeom->DrawOBB(obb, pos, 0, col, eBBD_Extremes_Color_Encoded);
    obb = OBB::CreateOBBfromAABB(m33, aabb1);
    g_pAuxGeom->DrawOBB(obb, pos, 0, col, eBBD_Extremes_Color_Encoded);

    m33 = Matrix33::CreateRotationXYZ(angle2);
    obb = OBB::CreateOBBfromAABB(m33, aabb1);
    g_pAuxGeom->DrawOBB(obb, pos, 0, col, eBBD_Extremes_Color_Encoded);
    obb = OBB::CreateOBBfromAABB(m33, aabb2);
    g_pAuxGeom->DrawOBB(obb, pos, 0, col, eBBD_Extremes_Color_Encoded);

    m33 = Matrix33::CreateRotationXYZ(angle3);
    obb = OBB::CreateOBBfromAABB(m33, aabb1);
    g_pAuxGeom->DrawOBB(obb, pos, 0, col, eBBD_Extremes_Color_Encoded);
    obb = OBB::CreateOBBfromAABB(m33, aabb2);
    g_pAuxGeom->DrawOBB(obb, pos, 0, col, eBBD_Extremes_Color_Encoded);

    obb = OBB::CreateOBBfromAABB(rAttModelRelative.q, aabb1);
    g_pAuxGeom->DrawOBB(obb, pos, 0, RGBA8(0xff, 0xff, 0xff, 0xff), eBBD_Extremes_Color_Encoded);
}
#endif


void CAttachmentManager::DrawAttachments(const SRendParams& rRendParams, const Matrix34& rWorldMat34, const SRenderingPassInfo& passInfo)
{
#ifdef DEFINE_PROFILER_FUNCTION
    DEFINE_PROFILER_FUNCTION();
#endif
    const uint32 numAttachments = m_arrAttachments.size();
    if (numAttachments == 0)
    {
        return;
    }

    if (m_TypeSortingRequired)
    {
        SortByType();
    }
#ifdef EDITOR_PCDEBUGCODE
    Verification();
#endif

#if !defined(_RELEASE)
    g_pAuxGeom->SetRenderFlags(e_Def3DPublicRenderflags);
    if (Console::GetInst().ca_DrawAttachments == 0)
    {
        return;
    }
#endif

    f32 fFOV = g_pIRenderer->GetCamera().GetFov();
    f32 fZoomFactor = 0.0f + 1.0f * (RAD2DEG(fFOV) / 60.f);
    f32 attachmentCullingRation = (gEnv->bMultiplayer) ? Console::GetInst().ca_AttachmentCullingRationMP : Console::GetInst().ca_AttachmentCullingRation;
    f32 fZoomDistanceSq = sqr(rRendParams.fDistance * fZoomFactor / attachmentCullingRation);

    uint32 InRecursion = passInfo.IsRecursivePass();
    const bool InShadow = passInfo.IsShadowPass();

    //Only save the zoom sq distance if not called from recursive render call
    //this is because zoom sq distance is used to cull attachments and zoom is modified for recursive calls (see e_RecursionViewDistRatio)
    if (InRecursion == 0)
    {
        m_fZoomDistanceSq = fZoomDistanceSq;
    }

    uint32 uHideFlags = 0;
    if (InRecursion > 1)
    {
        uHideFlags |= FLAGS_ATTACH_HIDE_RECURSION;
    }
    else if (InShadow)
    {
        uHideFlags |= FLAGS_ATTACH_HIDE_SHADOW_PASS;
    }
    else
    {
        uHideFlags |= FLAGS_ATTACH_HIDE_MAIN_PASS;
    }

    const uint32 uLastAttachmentIndex = numAttachments - 1;
    uint32 uAdditionalObjFlags = 0;
    if (m_pSkelInstance->m_rpFlags & CS_FLAG_DRAW_NEAR)
    {
        uAdditionalObjFlags |= FOB_NEAREST;
    }

    SRendParams rParams (rRendParams);
    rParams.pMaterial = NULL;   // this is required to avoid the attachments using the parent character material (this is the material that overrides the default material in the attachment)
    rParams.dwFObjFlags |= uAdditionalObjFlags;
    if (m_numRedirectionWithAttachment)
    {
        for (uint32 i = 0; i < m_sortop[1][br]; i++)
        {
            IAttachment* pIAttachment   = m_arrAttachments[i];
            CAttachmentBONE* pCAttachmentBone = (CAttachmentBONE*)pIAttachment;
            IAttachmentObject* pIAttachmentObject = pCAttachmentBone->m_pIAttachmentObject;
            if (pIAttachmentObject == 0)
            {
                continue; //most likely all of them are 0
            }
            if (pCAttachmentBone->m_AttFlags & uHideFlags)
            {
                continue;
            }
            if (pCAttachmentBone->m_nJointID < 0)
            {
                continue; //No success! Maybe next time
            }
            Matrix34 FinalMat34 = rWorldMat34 * Matrix34(pCAttachmentBone->m_AttModelRelative * pCAttachmentBone->m_addTransformation);
            rParams.pMatrix = &FinalMat34;
            pIAttachmentObject->RenderAttachment(rParams, passInfo);
        }
    }
    for (uint32 i = m_sortop[0][bs]; i < m_sortop[1][bx]; i++)
    {
        IAttachment* pIAttachment   = m_arrAttachments[i];
        CAttachmentBONE* pCAttachmentBone = (CAttachmentBONE*)pIAttachment;
        if (pCAttachmentBone->m_AttFlags & uHideFlags)
        {
            continue;
        }
        if (pCAttachmentBone->m_nJointID < 0)
        {
            continue; //No success! Maybe next time
        }
        if ((pCAttachmentBone->m_AttFlags & FLAGS_ATTACH_VISIBLE) == 0)
        {
            continue;
        }
        Matrix34 FinalMat34 = rWorldMat34 * Matrix34(pCAttachmentBone->m_AttModelRelative * pCAttachmentBone->m_addTransformation);
        rParams.pMatrix = &FinalMat34;
        pCAttachmentBone->m_pIAttachmentObject->RenderAttachment(rParams, passInfo);
    }
    for (uint32 i = m_sortop[0][fs]; i < m_sortop[1][fx]; i++)
    {
        IAttachment* pIAttachment   = m_arrAttachments[i];
        CAttachmentFACE* pCAttachmentFace = (CAttachmentFACE*)pIAttachment;
        if (pCAttachmentFace->m_AttFlags & uHideFlags)
        {
            continue;
        }
        if ((pCAttachmentFace->m_AttFlags & FLAGS_ATTACH_PROJECTED) == 0)
        {
            continue; //no success! maybe next time
        }
        if ((pCAttachmentFace->m_AttFlags & FLAGS_ATTACH_VISIBLE) == 0)
        {
            continue; //Distance culling. Object is too small for rendering
        }
        Matrix34 FinalMat34 = rWorldMat34 * Matrix34(pCAttachmentFace->m_AttModelRelative * pCAttachmentFace->m_addTransformation);
        rParams.pMatrix = &FinalMat34;
        pCAttachmentFace->m_pIAttachmentObject->RenderAttachment(rParams, passInfo);
    }

#if !defined(_RELEASE)
    // for debugdrawing - drawOffset prevents attachments from overdrawing each other, drawScale helps scaling depending on attachment count
    // used for Skin and Cloth Attachments for now
    static ICVar* p_e_debug_draw = gEnv->pConsole->GetCVar("e_DebugDraw");
    float drawOffset = 0.0f;
    float debugDrawScale = 1.0f;

    if (p_e_debug_draw->GetIVal() == 20)
    {
        int attachmentCount = m_sortop[1][sm] - m_sortop[0][sm];
        attachmentCount += (m_sortop[1][vc] - m_sortop[0][vc]);
        debugDrawScale = 0.5f + (10.0f / max(1.0f, (float)attachmentCount));
        debugDrawScale = min(debugDrawScale, 1.5f);
    }
#endif

    for (uint32 i = m_sortop[0][sm]; i < m_sortop[1][sm]; i++)
    {
        IAttachment* pIAttachment   = m_arrAttachments[i];
        CAttachmentSKIN* pCAttachmentSkin = (CAttachmentSKIN*)pIAttachment;
        if (pCAttachmentSkin->m_AttFlags & uHideFlags)
        {
            continue;
        }
        if (pCAttachmentSkin->m_pIAttachmentObject == 0)
        {
            continue;
        }
        uint32 nDrawModel = m_pSkelInstance->m_rpFlags & CS_FLAG_DRAW_MODEL;
        if (nDrawModel == 0)
        {
            continue;
        }
        const f32 fRadiusSqr = m_pSkelInstance->m_SkeletonPose.GetAABB().GetRadiusSqr();
        if (fRadiusSqr == 0.0f)
        {
            continue;           //if radius is zero, then the object is most probably not visible and we can continue
        }
        if (fZoomDistanceSq > fRadiusSqr)
        {
            continue; //too small to render. cancel the update
        }
        pCAttachmentSkin->DrawAttachment(rParams, passInfo, rWorldMat34, fZoomFactor);

#if !defined(_RELEASE)
        // pMaterial is set to NULL above, but restored in DrawAttachment with correct material
        if (p_e_debug_draw->GetIVal() == 20)
        {
            Vec3 drawLoc = rWorldMat34.GetTranslation();
            drawLoc.z += drawOffset;
            drawOffset += DebugDrawAttachment(pCAttachmentSkin, pCAttachmentSkin->GetISkin(), drawLoc, rParams.pMaterial, debugDrawScale);
        }
#endif
    }
    for (uint32 i = m_sortop[0][vc]; i < m_sortop[1][vc]; i++)
    {
        IAttachment* pIAttachment   = m_arrAttachments[i];
        if (pIAttachment->GetType() != CA_VCLOTH)
        {
            continue;
        }
        CAttachmentVCLOTH* pCAttachmentVCloth = (CAttachmentVCLOTH*)pIAttachment;
        if (pCAttachmentVCloth->m_AttFlags & uHideFlags)
        {
            continue;
        }
        if (pCAttachmentVCloth->m_pIAttachmentObject == 0)
        {
            continue;
        }
        uint32 nDrawModel = m_pSkelInstance->m_rpFlags & CS_FLAG_DRAW_MODEL;
        if (nDrawModel == 0)
        {
            continue;
        }
        const f32 fRadiusSqr = m_pSkelInstance->m_SkeletonPose.GetAABB().GetRadiusSqr();
        if (fRadiusSqr == 0.0f)
        {
            continue;           //if radius is zero, then the object is most probably not visible and we can continue
        }
        if (fZoomDistanceSq > fRadiusSqr)
        {
            continue;  //too small to render. cancel the update
        }
        pCAttachmentVCloth->InitializeCloth();
        pCAttachmentVCloth->DrawAttachment(rParams, passInfo, rWorldMat34, fZoomFactor);

#if !defined(_RELEASE)
        if (p_e_debug_draw->GetIVal() == 20)
        {
            Vec3 drawLoc = rWorldMat34.GetTranslation();
            drawLoc.z += drawOffset;
            drawOffset += DebugDrawAttachment(pCAttachmentVCloth, pCAttachmentVCloth->GetISkin(), drawLoc, rParams.pMaterial, debugDrawScale);
        }
#endif
    }



#if !defined(_RELEASE)
    if (Console::GetInst().ca_DrawEmptyAttachments)
    {
        for (uint32 i = m_sortop[0][be]; i < m_sortop[1][be]; i++)
        {
            IAttachment* pIAttachment   = m_arrAttachments[i];
            CAttachmentBONE* pCAttachmentBone = (CAttachmentBONE*)pIAttachment;
            if (pCAttachmentBone->m_AttFlags & uHideFlags)
            {
                continue;
            }
            if (pCAttachmentBone->m_nJointID < 0)
            {
                return; //no success! maybe next time
            }
            VisualizeEmptySocket(rWorldMat34, pCAttachmentBone->m_AttModelRelative, RGBA8(0xff, 0x00, 0x1f, 0xff));
        }
        for (uint32 i = m_sortop[0][fe]; i < m_sortop[1][fe]; i++)
        {
            IAttachment* pIAttachment   = m_arrAttachments[i];
            CAttachmentFACE* pCAttachmentFace = (CAttachmentFACE*)pIAttachment;
            if (pCAttachmentFace->m_AttFlags & uHideFlags)
            {
                continue;
            }
            if ((pCAttachmentFace->m_AttFlags & FLAGS_ATTACH_PROJECTED) == 0)
            {
                return; //no success! maybe next time
            }
            VisualizeEmptySocket(rWorldMat34, pCAttachmentFace->m_AttModelRelative, RGBA8(0x1f, 0x00, 0xff, 0xff));
        }
    }

    if (Console::GetInst().ca_DrawAttachmentOBB)
    {
        g_pAuxGeom->SetRenderFlags(e_Def3DPublicRenderflags);
        for (uint32 i = m_sortop[0][bs]; i < m_sortop[1][bx]; i++)
        {
            IAttachment* pIAttachment   = m_arrAttachments[i];
            CAttachmentBONE* pCAttachmentBone = (CAttachmentBONE*)pIAttachment;
            if (pCAttachmentBone->m_AttFlags & uHideFlags)
            {
                continue;
            }
            if (pCAttachmentBone->m_nJointID < 0)
            {
                continue; //no success! maybe next time
            }
            if ((pCAttachmentBone->m_AttFlags & FLAGS_ATTACH_VISIBLE) == 0)
            {
                continue; //Distance culling. Object is too small for rendering
            }
            Matrix34 FinalMat34 = rWorldMat34 * Matrix34(pCAttachmentBone->m_AttModelRelative);
            Vec3 obbPos = FinalMat34.GetTranslation();
            if (rParams.dwFObjFlags & FOB_NEAREST)
            {
                obbPos += gEnv->pRenderer->GetCamera().GetPosition();   // Convert to world space
            }
            AABB caabb =    pCAttachmentBone->m_pIAttachmentObject->GetAABB();
            OBB obb2 = OBB::CreateOBBfromAABB(Matrix33(FinalMat34), caabb);
            g_pAuxGeom->DrawOBB(obb2, obbPos, 0, RGBA8(0xff, 0x00, 0x1f, 0xff), eBBD_Extremes_Color_Encoded);
        }
        for (uint32 i = m_sortop[0][fs]; i < m_sortop[1][fx]; i++)
        {
            IAttachment* pIAttachment   = m_arrAttachments[i];
            CAttachmentFACE* pCAttachmentFace = (CAttachmentFACE*)pIAttachment;
            if (pCAttachmentFace->m_AttFlags & uHideFlags)
            {
                continue;
            }
            if ((pCAttachmentFace->m_AttFlags & FLAGS_ATTACH_PROJECTED) == 0)
            {
                continue; //no success! maybe next time
            }
            if ((pCAttachmentFace->m_AttFlags & FLAGS_ATTACH_VISIBLE) == 0)
            {
                continue; //Distance culling. Object is too small for rendering
            }
            Matrix34 FinalMat34 = rWorldMat34 * Matrix34(pCAttachmentFace->m_AttModelRelative);
            Vec3 obbPos = FinalMat34.GetTranslation();
            if (rParams.dwFObjFlags & FOB_NEAREST)
            {
                obbPos += gEnv->pRenderer->GetCamera().GetPosition();   // Convert to world space
            }
            AABB caabb =    pCAttachmentFace->m_pIAttachmentObject->GetAABB();
            OBB obb2 = OBB::CreateOBBfromAABB(Matrix33(FinalMat34), caabb);
            g_pAuxGeom->DrawOBB(obb2, obbPos, 0, RGBA8(0x1f, 0x00, 0xff, 0xff), eBBD_Extremes_Color_Encoded);
        }
    }
    #endif
}









//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------

IProxy* CAttachmentManager::CreateProxy(const char* szProxyName, const char* szJointName)
{
    string strProxyName = szProxyName;
    strProxyName.MakeLower();
    IProxy* pIProxyName = GetProxyInterfaceByName(strProxyName.c_str());
    if (pIProxyName)
    {
        CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_ERROR, "CryAnimation: Attachment name '%s' is already in use, attachment will not be created", strProxyName.c_str());
        return 0;
    }

    uint32 nameCRC = CCrc32::ComputeLowercase(strProxyName.c_str());
    IAttachment* pIAttachmentCRC32 = GetInterfaceByNameCRC(nameCRC);
    if (pIAttachmentCRC32)
    {
        CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_ERROR, "CryAnimation: Attachment CRC32 for '%s' clashes with attachment name '%s' (crc's are created using lower case only), attachment will not be created", strProxyName.c_str(), pIAttachmentCRC32->GetName());
        return 0;
    }
    CDefaultSkeleton& rDefaultSkeleton = *m_pSkelInstance->m_pDefaultSkeleton;
    if (szJointName == 0)
    {
        return 0;
    }

    CProxy proxy;
    proxy.m_pAttachmentManager   = this;
    proxy.m_strProxyName         = strProxyName.c_str();
    proxy.m_nProxyCRC32          = CCrc32::ComputeLowercase(proxy.m_strProxyName.c_str());
    proxy.m_strJointName         = szJointName;
    proxy.m_nJointID             = rDefaultSkeleton.GetJointIDByName(proxy.m_strJointName.c_str());
    if (proxy.m_nJointID < 0)
    {
        CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_ERROR, "CryAnimation: Proxy '%s' specified wrong joint name '%s'", strProxyName.c_str(), proxy.m_strJointName.c_str());
        return 0;
    }
    proxy.m_ProxyAbsoluteDefault = rDefaultSkeleton.GetDefaultAbsJointByID(proxy.m_nJointID);
    proxy.m_params               = Vec4(0, 0, 0, 0.25f);
    QuatT jointQT = rDefaultSkeleton.GetDefaultAbsJointByID(proxy.m_nJointID);
    proxy.m_ProxyRelativeDefault = jointQT.GetInverted() * proxy.m_ProxyAbsoluteDefault;
    m_arrProxies.push_back(proxy);
    return GetProxyInterfaceByCRC(proxy.m_nProxyCRC32);
}

IProxy* CAttachmentManager::GetProxyInterfaceByIndex(uint32 c) const
{
    size_t size = m_arrProxies.size();
    if (size == 0)
    {
        return 0;
    }
    if (size <= c)
    {
        return 0;
    }
    IProxy* pIProxy = (IProxy*)&m_arrProxies[c];
    return pIProxy;
};

IProxy* CAttachmentManager::GetProxyInterfaceByName(const char* szName) const
{
    int32 idx = GetProxyIndexByName(szName);
    if (idx == -1)
    {
        return 0;
    }
    return GetProxyInterfaceByIndex(idx);
}

IProxy* CAttachmentManager::GetProxyInterfaceByCRC(uint32 nameCRC) const
{
    int32 idx = GetProxyIndexByCRC(nameCRC);
    if (idx == -1)
    {
        return 0;
    }
    return GetProxyInterfaceByIndex(idx);
};

int32 CAttachmentManager::GetProxyIndexByName(const char* szName) const
{
    int num = GetProxyCount();
    for (int i = 0; i < num; i++)
    {
        IProxy* pA = GetProxyInterfaceByIndex(i);
        if (!_stricmp(pA->GetName(), szName))
        {
            return i;
        }
    }
    return -1;
}

int32 CAttachmentManager::GetProxyIndexByCRC(uint32 nameCRC) const
{
    int num = GetProxyCount();
    for (int i = 0; i < num; i++)
    {
        IProxy* pA = GetProxyInterfaceByIndex(i);
        if (pA->GetNameCRC() == nameCRC)
        {
            return i;
        }
    }
    return -1;
}

int32 CAttachmentManager::RemoveProxyByInterface(const IProxy* pIProxy)
{
    const char* pName = pIProxy->GetName();
    return RemoveProxyByName(pName);
}

int32 CAttachmentManager::RemoveProxyByName(const char* szName)
{
    int32 index = GetProxyIndexByName(szName);
    if (index == -1)
    {
        return 0;
    }
    RemoveProxyByIndex(index);
    return 1;
};

int32 CAttachmentManager::RemoveProxyByNameCRC(uint32 nameCRC)
{
    int32 index = GetIndexByNameCRC(nameCRC);
    if (index == -1)
    {
        return 0;
    }
    RemoveProxyByIndex(index);
    return 1;
};

void CAttachmentManager::RemoveProxyByIndex(uint32 n)
{
    size_t size = m_arrProxies.size();
    if (size == 0)
    {
        return;
    }
    if (size <= n)
    {
        return;
    }
    IProxy* pIProxy = GetProxyInterfaceByIndex(n);
    if (pIProxy == 0)
    {
        return;
    }
    m_arrProxies[n] = m_arrProxies[size - 1];
    m_arrProxies.pop_back();
    VerifyProxyLinks();
}

size_t CAttachmentManager::SizeOfAllAttachments()
{
    size_t nSize = 0;
    nSize += m_arrAttachments.get_alloc_size();
    uint32 numAttachments = m_arrAttachments.size();
    for (uint32 i = 0; i < numAttachments; i++)
    {
        nSize += m_arrAttachments[i]->SizeOfThis();
    }
    return nSize;
}

void CAttachmentManager::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(m_arrAttachments);
}


void CAttachmentManager::SortByType()
{
    m_TypeSortingRequired = 0;
    CDefaultSkeleton& rDefaultSkeleton = *m_pSkelInstance->m_pDefaultSkeleton;
    memset(m_sortop, 0, sizeof(m_sortop));
    uint32 numAttachments = m_arrAttachments.size(), n = 0;
    if (numAttachments == 0)
    {
        return;
    }
    VerifyProxyLinks();
    IAttachment** parr = (IAttachment**)&m_arrAttachments[0];
    STACK_ARRAY(IAttachment*, parr2, numAttachments);
    for (uint32 a = 0; a < numAttachments; a++)
    {
        if (parr[a]->GetType() != CA_BONE)
        {
            continue;
        }
        CAttachmentBONE* pb = (CAttachmentBONE*)parr[a];
        const char* strJointName = pb->m_strJointName.c_str();
        if (pb->m_Simulation.m_useRedirect)
        {
            parr2[n++] = parr[a], pb->m_nJointID = rDefaultSkeleton.GetJointIDByName(strJointName);
        }
    }
    m_sortop[1][br] = n, m_sortop[0][be] = n;
    for (uint32 i = 0; i < numAttachments; i++)
    {
        if (parr[i]->GetType() != CA_BONE)
        {
            continue;
        }
        CAttachmentBONE* pb = (CAttachmentBONE*)parr[i];
        if (pb->m_Simulation.m_useRedirect)
        {
            continue;
        }
        if (parr[i]->GetIAttachmentObject() == 0)
        {
            parr2[n++] = parr[i], pb->m_nJointID = rDefaultSkeleton.GetJointIDByName(pb->m_strJointName.c_str());
        }
    }
    m_sortop[1][be] = n, m_sortop[0][bs] = n;
    for (uint32 i = 0; i < numAttachments; i++)
    {
        if (parr[i]->GetType() != CA_BONE)
        {
            continue;
        }
        CAttachmentBONE* pb = (CAttachmentBONE*)parr[i];
        if (pb->m_Simulation.m_useRedirect)
        {
            continue;
        }
        IAttachmentObject* p = pb->m_pIAttachmentObject;
        if (p && p->GetAttachmentType() == IAttachmentObject::eAttachment_StatObj)
        {
            parr2[n++] = parr[i], pb->m_nJointID = rDefaultSkeleton.GetJointIDByName(pb->m_strJointName.c_str());
        }
    }
    m_sortop[1][bs] = n, m_sortop[0][bx] = n;
    for (uint32 i = 0; i < numAttachments; i++)
    {
        if (parr[i]->GetType() != CA_BONE)
        {
            continue;
        }
        CAttachmentBONE* pb = (CAttachmentBONE*)parr[i];
        if (pb->m_Simulation.m_useRedirect)
        {
            continue;
        }
        IAttachmentObject* p = pb->m_pIAttachmentObject;
        if (p)
        {
            IAttachmentObject::EType t = p->GetAttachmentType();
            uint32 isSkeleton = t == IAttachmentObject::eAttachment_Skeleton;
            uint32 isEntity   = t == IAttachmentObject::eAttachment_Entity;
            uint32 isLight    = t == IAttachmentObject::eAttachment_Light;
            uint32 isEffect   = t == IAttachmentObject::eAttachment_Effect;
            if (isEntity + isSkeleton + isLight + isEffect)
            {
                parr2[n++] = parr[i], pb->m_nJointID = rDefaultSkeleton.GetJointIDByName(pb->m_strJointName.c_str());
            }
        }
    }
    m_sortop[1][bx] = n, m_sortop[0][fe] = n;
    for (uint32 i = 0; i < numAttachments; i++)
    {
        if (parr[i]->GetType() != CA_FACE)
        {
            continue;
        }
        if (parr[i]->GetIAttachmentObject() == 0)
        {
            parr2[n++] = parr[i];
        }
    }
    m_sortop[1][fe] = n, m_sortop[0][fs] = n;
    for (uint32 i = 0; i < numAttachments; i++)
    {
        if (parr[i]->GetType() != CA_FACE)
        {
            continue;
        }
        IAttachmentObject* p = parr[i]->GetIAttachmentObject();
        if (p && p->GetAttachmentType() == IAttachmentObject::eAttachment_StatObj)
        {
            parr2[n++] = parr[i];
        }
    }
    m_sortop[1][fs] = n, m_sortop[0][fx] = n;
    for (uint32 i = 0; i < numAttachments; i++)
    {
        if (parr[i]->GetType() != CA_FACE)
        {
            continue;
        }
        if (parr[i]->GetIAttachmentObject())
        {
            IAttachmentObject::EType t = parr[i]->GetIAttachmentObject()->GetAttachmentType();
            uint32 isSkeleton = t == IAttachmentObject::eAttachment_Skeleton;
            uint32 isEntity   = t == IAttachmentObject::eAttachment_Entity;
            uint32 isLight    = t == IAttachmentObject::eAttachment_Light;
            uint32 isEffect   = t == IAttachmentObject::eAttachment_Effect;
            if (isEntity + isSkeleton + isLight + isEffect)
            {
                parr2[n++] = parr[i];
            }
        }
    }
    m_sortop[1][fx] = n, m_sortop[0][sm] = n;
    for (uint32 i = 0; i < numAttachments; i++)
    {
        if (parr[i]->GetType() == CA_SKIN)
        {
            PREFAST_SUPPRESS_WARNING(6386) parr2[n++] = parr[i];
        }
    }
    m_sortop[1][sm] = n, m_sortop[0][vc] = n;
    for (uint32 i = 0; i < numAttachments; i++)
    {
        if (parr[i]->GetType() == CA_PROW || parr[i]->GetType() == CA_VCLOTH)
        {
            PREFAST_SUPPRESS_WARNING(6386) parr2[n++] = parr[i];
        }
    }
    m_sortop[1][vc] = n;

    if (n != numAttachments)
    {
        CryFatalError("CryAnimation: sorting error: %s", m_pSkelInstance->GetFilePath());
    }
    for (uint32 i = 0; i < numAttachments; i++)
    {
        parr[i] = parr2[i];
    }
    if (m_sortop[1][br])
    {
        STACK_ARRAY(uint8, parrflags, m_sortop[1][br]);
        memset(parrflags, 0, m_sortop[1][br]);
        for (uint32 i = 0; i < m_sortop[1][br]; i++)
        {
            uint32 s = 0x7fff, t = 0;
            for (uint32 r = 0; r < m_sortop[1][br]; r++)
            {
                CAttachmentBONE* pb = (CAttachmentBONE*)parr[r];
                if (s > uint32((pb->m_nJointID + 1) | parrflags[r] << 0x10))
                {
                    t = r, s = pb->m_nJointID + 1;
                }
            }
            parr2[i] = parr[t], parrflags[t] = 1;
        }
        m_numRedirectionWithAttachment = 0;
        for (uint32 r = 0; r < m_sortop[1][br]; r++)
        {
            parr[r] = parr2[r];
            CAttachmentBONE* pb = (CAttachmentBONE*)parr[r];
            if (pb->m_pIAttachmentObject)
            {
                m_numRedirectionWithAttachment++;
            }
            if (pb->m_Simulation.m_arrChildren.size())
            {
                continue;
            }
            pb->m_Simulation.m_nAnimOverrideJoint = rDefaultSkeleton.GetJointIDByName(BoneNames::BlendWeightPendulum);
            int32 parentid = pb->m_nJointID;
            if (parentid < 0)
            {
                continue;
            }
            bool arrChildren[MAX_JOINT_AMOUNT];
            for (uint32 i = 0; i < MAX_JOINT_AMOUNT; i++)
            {
                arrChildren[i] = 0;
            }
            pb->m_Simulation.m_arrChildren.resize(0);
            pb->m_Simulation.m_arrChildren.reserve(16);
            uint32 numJoints = rDefaultSkeleton.GetJointCount();
            for (uint32 i = parentid + 1; i < numJoints; i++)
            {
                if (rDefaultSkeleton.GetJointParentIDByID(i) == parentid)
                {
                    arrChildren[i] = 1;
                }
            }
            for (uint32 pid = 0; pid < MAX_JOINT_AMOUNT; pid++)
            {
                if (arrChildren[pid] == 0)
                {
                    continue;
                }
                for (uint32 i = pid + 1; i < numJoints; i++)
                {
                    if (rDefaultSkeleton.GetJointParentIDByID(i) == pid)
                    {
                        arrChildren[i] = 1;
                    }
                }
            }
            for (uint32 i = 0; i < MAX_JOINT_AMOUNT; i++)
            {
                if (arrChildren[i])
                {
                    pb->m_Simulation.m_arrChildren.push_back(i);
                }
            }
        }
    }
    for (uint32 r = m_sortop[0][vc]; r < m_sortop[1][vc]; r++)
    {
        if (parr[r]->GetType() != CA_PROW)
        {
            continue;
        }
        CAttachmentPROW* pb = (CAttachmentPROW*)parr[r];
        pb->m_nRowJointID = rDefaultSkeleton.GetJointIDByName(pb->m_strRowJointName.c_str());
        if (pb->m_nRowJointID < 0)
        {
            continue;
        }
        const char* psrc = pb->m_strRowJointName.c_str();
        const char* pstr = CryStringUtils::stristr(psrc, "_x00_");
        if (pstr == 0)
        {
            continue; //invalid name
        }
        char pJointName[256];
        azstrcpy(pJointName, 256, psrc);
        JointIdType jindices[MAX_JOINT_AMOUNT];
        uint16& numParticles = pb->m_rowparams.m_nParticlesPerRow;
        numParticles = 0;
        for (int idx = int(pstr - psrc) + 2; numParticles < MAX_JOINTS_PER_ROW; numParticles++)
        {
            if ((jindices[numParticles] = rDefaultSkeleton.GetJointIDByName(pJointName)) == JointIdType(-1))
            {
                break;
            }
            if (++pJointName[idx + 1] == 0x3a)
            {
                pJointName[idx]++, pJointName[idx + 1] = 0x30;
            }
        }
        pb->m_rowparams.m_arrParticles.resize(numParticles);
        uint32 numJoints = rDefaultSkeleton.GetJointCount();
        for (uint32 r = 0; r < numParticles; r++)
        {
            pb->m_rowparams.m_arrParticles[r].m_childID = JointIdType(-1);
            JointIdType parentid = jindices[r];
            if (parentid == JointIdType(-1))
            {
                continue; //invalid
            }
            for (uint32 i = parentid + 1; i < numJoints; i++)
            {
                if (rDefaultSkeleton.GetJointParentIDByID(i) == parentid)
                {
                    pb->m_rowparams.m_arrParticles[r].m_childID = i;
                    break;
                }
            }
        }
        for (uint32 i = 0; i < numParticles; i++)
        {
            pb->m_rowparams.m_arrParticles[i].m_vDistance.y = 0.07f; //use 7cm by default
            if (pb->m_rowparams.m_arrParticles[i].m_childID != JointIdType(-1))
            {
                pb->m_rowparams.m_arrParticles[i].m_vDistance.y = rDefaultSkeleton.GetDefaultRelJointByID(pb->m_rowparams.m_arrParticles[i].m_childID).t.GetLength(); //set the vertical distance
            }
        }
        for (uint32 i = 0; i < numParticles; i++)
        {
            uint32 i0 = i;
            uint32 i1 = (i + 1) % numParticles;
            JointIdType id0 = jindices[i0];
            Vec3 j0 = rDefaultSkeleton.GetDefaultAbsJointByID(id0).q.GetColumn0() * pb->m_rowparams.m_arrParticles[i0].m_vDistance.y + rDefaultSkeleton.GetDefaultAbsJointByID(id0).t;
            JointIdType id1 = jindices[i1];
            Vec3 j1 = rDefaultSkeleton.GetDefaultAbsJointByID(id1).q.GetColumn0() * pb->m_rowparams.m_arrParticles[i1].m_vDistance.y + rDefaultSkeleton.GetDefaultAbsJointByID(id1).t;
            pb->m_rowparams.m_arrParticles[i0].m_vDistance.x = (j0 - j1).GetLength(); //set the horizontal distance
            pb->m_rowparams.m_arrParticles[i].m_jointID = id0;
        }
    }
}


#ifdef EDITOR_PCDEBUGCODE
void CAttachmentManager::Verification()
{
    int32 s = -1;
    for (uint32 i = 0; i < m_sortop[1][br]; i++)
    {
        IAttachment* pIAttachment = m_arrAttachments[i];
        if (pIAttachment->GetType() != CA_BONE)
        {
            CryFatalError("CryAnimation: setup conflict in attachment: %s", pIAttachment->GetName());
        }
        CAttachmentBONE* pCAttachmentBone = (CAttachmentBONE*)pIAttachment;
        if (pCAttachmentBone->m_Simulation.m_nClampType == 0)
        {
            CryFatalError("CryAnimation: setup conflict in attachment: %s", pIAttachment->GetName());
        }
        int32 nJointCount = m_pSkelInstance->m_pDefaultSkeleton->GetJointCount();
        if (pCAttachmentBone->m_nJointID >= nJointCount)
        {
            CryFatalError("CryAnimation: setup conflict in attachment: %s", pIAttachment->GetName());
        }
        if (pCAttachmentBone->m_Simulation.m_useRedirect != 1)
        {
            CryFatalError("CryAnimation: setup conflict in attachment: %s", pIAttachment->GetName());
        }
        if (pCAttachmentBone->m_nJointID < s)
        {
            CryFatalError("CryAnimation: setup conflict in attachment: %s", pIAttachment->GetName());
        }
        s = pCAttachmentBone->m_nJointID;
    }

    for (uint32 i = m_sortop[0][be]; i < m_sortop[1][be]; i++)
    {
        IAttachment* pIAttachment = m_arrAttachments[i];
        if (pIAttachment->GetType() != CA_BONE)
        {
            CryFatalError("CryAnimation: setup conflict in attachment: %s", pIAttachment->GetName());
        }
        CAttachmentBONE* pCAttachmentBone = (CAttachmentBONE*)pIAttachment;
        if (pCAttachmentBone->m_pIAttachmentObject)
        {
            CryFatalError("CryAnimation: setup conflict in attachment: %s", pIAttachment->GetName());
        }
        if (pCAttachmentBone->m_nJointID >= 0)
        {
            int32 nJointCount = m_pSkelInstance->m_pDefaultSkeleton->GetJointCount();
            if (pCAttachmentBone->m_nJointID > nJointCount)
            {
                CryFatalError("CryAnimation: setup conflict in attachment: %s", pIAttachment->GetName());
            }
        }
    }

    for (uint32 i = m_sortop[0][bs]; i < m_sortop[1][bs]; i++)
    {
        IAttachment* pIAttachment = m_arrAttachments[i];
        if (pIAttachment->GetType() != CA_BONE)
        {
            CryFatalError("CryAnimation: setup conflict in attachment: %s", pIAttachment->GetName());
        }
        CAttachmentBONE* pCAttachmentBone = (CAttachmentBONE*)pIAttachment;
        if (pCAttachmentBone->m_Simulation.m_useRedirect && pCAttachmentBone->m_Simulation.m_nClampType)
        {
            CryFatalError("CryAnimation: setup conflict in attachment: %s", pIAttachment->GetName());
        }
        if (pCAttachmentBone->m_nJointID >= 0)
        {
            int32 nJointCount = m_pSkelInstance->m_pDefaultSkeleton->GetJointCount();
            if (pCAttachmentBone->m_nJointID > nJointCount)
            {
                CryFatalError("CryAnimation: setup conflict in attachment: %s", pIAttachment->GetName());
            }
        }
        IAttachmentObject* p = pIAttachment->GetIAttachmentObject();
        if (p == 0)
        {
            CryFatalError("CryAnimation: setup conflict in attachment: %s", pIAttachment->GetName());
        }
        IAttachmentObject::EType eAttachmentType = p->GetAttachmentType();
        uint32 isStatic = eAttachmentType == IAttachmentObject::eAttachment_StatObj;
        if (isStatic == 0)
        {
            CryFatalError("CryAnimation: setup conflict in attachment: %s", pIAttachment->GetName());
        }
    }

    for (uint32 i = m_sortop[0][bx]; i < m_sortop[1][bx]; i++)
    {
        IAttachment* pIAttachment = m_arrAttachments[i];
        if (pIAttachment->GetType() != CA_BONE)
        {
            CryFatalError("CryAnimation: setup conflict in attachment: %s", pIAttachment->GetName());
        }
        CAttachmentBONE* pCAttachmentBone = (CAttachmentBONE*)pIAttachment;
        if (pCAttachmentBone->m_nJointID >= 0)
        {
            uint32 nJointCount = m_pSkelInstance->m_pDefaultSkeleton->GetJointCount();
            if (pCAttachmentBone->m_nJointID > nJointCount)
            {
                CryFatalError("CryAnimation: setup conflict in attachment: %s", pIAttachment->GetName());
            }
        }
        IAttachmentObject* p = pIAttachment->GetIAttachmentObject();
        if (p == 0)
        {
            CryFatalError("CryAnimation: setup conflict in attachment: %s", pIAttachment->GetName());
        }
        IAttachmentObject::EType eAttachmentType = p->GetAttachmentType();
        uint32 isSkeleton = eAttachmentType == IAttachmentObject::eAttachment_Skeleton;
        uint32 isEntity   = eAttachmentType == IAttachmentObject::eAttachment_Entity;
        uint32 isLight    = eAttachmentType == IAttachmentObject::eAttachment_Light;
        uint32 isEffect   = eAttachmentType == IAttachmentObject::eAttachment_Effect;
        if ((isSkeleton | isEntity | isLight | isEffect) == 0)
        {
            CryFatalError("CryAnimation: setup conflict in attachment: %s", pIAttachment->GetName());
        }
        if (p == 0 && pCAttachmentBone->m_Simulation.m_useRedirect && pCAttachmentBone->m_Simulation.m_nClampType)
        {
            CryFatalError("CryAnimation: setup conflict in attachment: %s", pIAttachment->GetName());
        }
    }



    for (uint32 i = m_sortop[0][fe]; i < m_sortop[1][fe]; i++)
    {
        IAttachment* pIAttachment = m_arrAttachments[i];
        if (pIAttachment->GetType() != CA_FACE)
        {
            CryFatalError("CryAnimation: setup conflict in attachment: %s", pIAttachment->GetName());
        }
        IAttachmentObject* p = pIAttachment->GetIAttachmentObject();
        if (p)
        {
            CryFatalError("CryAnimation: setup conflict in attachment: %s", pIAttachment->GetName());
        }
    }

    for (uint32 i = m_sortop[0][fs]; i < m_sortop[1][fs]; i++)
    {
        IAttachment* pIAttachment = m_arrAttachments[i];
        if (pIAttachment->GetType() != CA_FACE)
        {
            CryFatalError("CryAnimation: setup conflict in attachment: %s", pIAttachment->GetName());
        }
        IAttachmentObject* p = pIAttachment->GetIAttachmentObject();
        if (p == 0)
        {
            CryFatalError("CryAnimation: setup conflict in attachment: %s", pIAttachment->GetName());
        }
        IAttachmentObject::EType eAttachmentType = p->GetAttachmentType();
        uint32 isStatic = eAttachmentType == IAttachmentObject::eAttachment_StatObj;
        if (isStatic == 0)
        {
            CryFatalError("CryAnimation: setup conflict in attachment: %s", pIAttachment->GetName());
        }
    }

    for (uint32 i = m_sortop[0][fx]; i < m_sortop[1][fx]; i++)
    {
        IAttachment* pIAttachment = m_arrAttachments[i];
        if (pIAttachment->GetType() != CA_FACE)
        {
            CryFatalError("CryAnimation: setup conflict in attachment: %s", pIAttachment->GetName());
        }
        IAttachmentObject* p = pIAttachment->GetIAttachmentObject();
        if (p == 0)
        {
            CryFatalError("CryAnimation: setup conflict in attachment: %s", pIAttachment->GetName());
        }
        IAttachmentObject::EType eAttachmentType = p->GetAttachmentType();
        uint32 isSkeleton = eAttachmentType == IAttachmentObject::eAttachment_Skeleton;
        uint32 isEntity   = eAttachmentType == IAttachmentObject::eAttachment_Entity;
        uint32 isLight    = eAttachmentType == IAttachmentObject::eAttachment_Light;
        uint32 isEffect   = eAttachmentType == IAttachmentObject::eAttachment_Effect;
        if ((isSkeleton | isEntity | isLight | isEffect) == 0)
        {
            CryFatalError("CryAnimation: setup conflict in attachment: %s", pIAttachment->GetName());
        }
    }



    for (uint32 i = m_sortop[0][sm]; i < m_sortop[1][sm]; i++)
    {
        IAttachment* pIAttachment = m_arrAttachments[i];
        if (pIAttachment->GetType() != CA_SKIN)
        {
            CryFatalError("CryAnimation: setup conflict in attachment: %s", pIAttachment->GetName());
        }
    }
    for (uint32 i = m_sortop[0][vc]; i < m_sortop[1][vc]; i++)
    {
        IAttachment* pIAttachment = m_arrAttachments[i];
        if (pIAttachment->GetType() != CA_PROW && pIAttachment->GetType() != CA_VCLOTH)
        {
            CryFatalError("CryAnimation: setup conflict in attachment: %s", pIAttachment->GetName());
        }
    }

    //---------------------------------------------------------------
    return;

    float fColorYellow[4] = {1, 1, 0, 1};
    float fColorRed[4] = {1, 0, 0, 1};
    float fColorGreen[4] = {0, 1, 0, 1};
    float fColorBlue[4] = {0, 0, 1, 1};

    /*  if (m_sortop[1][br])
            g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.5f, fColorYellow, false,"m_sortop[0][br]: %d  m_sortop[1][br]: %d  name: %s",m_sortop[0][br],m_sortop[1][br],m_pSkelInstance->GetFilePath()),g_YLine+=16.0f;

        if (m_sortop[1][be])
            g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.5f, fColorRed, false,"m_sortop[0][be]: %d  m_sortop[1][be]: %d  name: %s",m_sortop[0][be],m_sortop[1][be],m_pSkelInstance->GetFilePath()),g_YLine+=16.0f;
        if (m_sortop[1][bs])
            g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.5f, fColorRed, false,"m_sortop[0][bs]: %d  m_sortop[1][bs]: %d  name: %s",m_sortop[0][bs],m_sortop[1][bs],m_pSkelInstance->GetFilePath()),g_YLine+=16.0f;
        if (m_sortop[1][bx])
            g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.5f, fColorRed, false,"m_sortop[0][bx]: %d  m_sortop[1][bx]: %d  name: %s",m_sortop[0][bx],m_sortop[1][bx],m_pSkelInstance->GetFilePath()),g_YLine+=16.0f;

        if (m_sortop[1][fe])
            g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.5f, fColorBlue, false,"m_sortop[0][fe]: %d  m_sortop[1][fe]: %d  name: %s",m_sortop[0][fe],m_sortop[1][fe],m_pSkelInstance->GetFilePath()),g_YLine+=16.0f;
        if (m_sortop[1][fs])
            g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.5f, fColorBlue, false,"m_sortop[0][fs]: %d  m_sortop[1][fs]: %d  name: %s",m_sortop[0][fs],m_sortop[1][fs],m_pSkelInstance->GetFilePath()),g_YLine+=16.0f;
        if (m_sortop[1][fx])
            g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.5f, fColorBlue, false,"m_sortop[0][fx]: %d  m_sortop[1][fx]: %d  name: %s",m_sortop[0][fx],m_sortop[1][fx],m_pSkelInstance->GetFilePath()),g_YLine+=16.0f;
        if (m_sortop[1][sm])
            g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.5f, fColorGreen, false,"m_sortop[0][sm]: %d  m_sortop[1][sm]: %d  name: %s",m_sortop[0][sm],m_sortop[1][sm],m_pSkelInstance->GetFilePath()),g_YLine+=16.0f;
        */

    uint32 numAttachments = m_arrAttachments.size();
    for (uint32 i = 0; i < numAttachments; i++)
    {
        IAttachment* pIAttachment = m_arrAttachments[i];
        uint32 nType = pIAttachment->GetType();
        if (nType == CA_BONE)
        {
            CAttachmentBONE* pCAttachmentBone = (CAttachmentBONE*)pIAttachment;
            uint32 IsJiggleJoint = (pIAttachment->GetIAttachmentObject() == 0 && pCAttachmentBone->m_Simulation.m_nClampType && pCAttachmentBone->m_Simulation.m_useRedirect);
            if (IsJiggleJoint)
            {
                g_pIRenderer->Draw2dLabel(1, g_YLine, 1.0f, fColorYellow, false, "CA_JIGL: %s refcount: %d  children: %d", pIAttachment->GetName(), pCAttachmentBone->m_nRefCounter, pCAttachmentBone->m_Simulation.m_arrChildren.size()), g_YLine += 10.0f;
            }
            else
            {
                IAttachmentObject* p = pIAttachment->GetIAttachmentObject();
                if (p)
                {
                    g_pIRenderer->Draw2dLabel(1, g_YLine, 1.0f, fColorRed, false, "CA_BONE: %s refcount: %d", pIAttachment->GetName(), pCAttachmentBone->m_nRefCounter), g_YLine += 10.0f;
                }
                else
                {
                    g_pIRenderer->Draw2dLabel(1, g_YLine, 1.0f, fColorRed, false, "CA_BONE: %s (empty) refcount: %d", pIAttachment->GetName(), pCAttachmentBone->m_nRefCounter), g_YLine += 10.0f;
                }
            }
        }
        if (nType == CA_FACE)
        {
            CAttachmentFACE* pCAttachmentFace = (CAttachmentFACE*)pIAttachment;
            IAttachmentObject* p = pCAttachmentFace->m_pIAttachmentObject;
            if (p)
            {
                g_pIRenderer->Draw2dLabel(1, g_YLine, 1.0f, fColorBlue, false, "CA_FACE: %s refcount: %d", pIAttachment->GetName(), pCAttachmentFace->m_nRefCounter), g_YLine += 10.0f;
            }
            else
            {
                g_pIRenderer->Draw2dLabel(1, g_YLine, 1.0f, fColorBlue, false, "CA_FACE: %s (empty) refcount: %d", pIAttachment->GetName(), pCAttachmentFace->m_nRefCounter), g_YLine += 10.0f;
            }
        }
        if (nType == CA_SKIN)
        {
            CAttachmentSKIN* pCAttachmentSkin = (CAttachmentSKIN*)pIAttachment;
            IAttachmentObject* p = pCAttachmentSkin->m_pIAttachmentObject;
            if (p)
            {
                g_pIRenderer->Draw2dLabel(1, g_YLine, 1.0f, fColorGreen, false, "CA_SKIN: %s  refcount: %d", pIAttachment->GetName(), pCAttachmentSkin->m_nRefCounter), g_YLine += 10.0f;
            }
            else
            {
                g_pIRenderer->Draw2dLabel(1, g_YLine, 1.0f, fColorGreen, false, "CA_SKIN: %s (empty)  refcount: %d", pIAttachment->GetName(), pCAttachmentSkin->m_nRefCounter), g_YLine += 10.0f;
            }
        }
    }
}

#endif

#if !defined(_RELEASE)

float CAttachmentManager::DebugDrawAttachment(IAttachment* pAttachment, ISkin* pSkin, Vec3 drawLoc, _smart_ptr<IMaterial> pMaterial, float drawScale)
{
    if (!pMaterial || !pAttachment || !pSkin ||  !pSkin->GetIRenderMesh(0))
    {
        return 0.0f;
    }

    const int nTexMemUsage = pSkin->GetIRenderMesh(0)->GetTextureMemoryUsage(pMaterial);
    ICVar* p_texBudget = gEnv->pConsole->GetCVar("ca_AttachmentTextureMemoryBudget"); // the closer to this limit, the more red we draw it for clarity
    const float fTextMemBudget = p_texBudget ? p_texBudget->GetIVal() * 1024 * 1024 : 1.0f;
    const float white = max(1.0f - (nTexMemUsage / fTextMemBudget), 0.0f);

    float color[4] = {1, white, white, 1};
    float fDist = (gEnv->pRenderer->GetCamera().GetPosition() - drawLoc).GetLength();

    static float scalar = 60.0f;
    float drawOffset = (drawScale * (fDist / scalar));

    gEnv->pRenderer->DrawLabelEx(drawLoc, drawScale, color, true, true, "%s (%d kb)", pAttachment->GetName(), nTexMemUsage / 1024);

    return drawOffset;
}
#endif

const char* CAttachmentManager::GetProcFunctionName(uint32 idx) const
{
    return 0;
}

void CAttachmentManager::ClearAttachmentData()
{
    //Clear Render Data
    for (IAttachment* pIAttachment : m_arrAttachments)
    {
        if (pIAttachment->GetType() == CA_SKIN)
        {
            CAttachmentSKIN* pCAttachmentSkin = static_cast<CAttachmentSKIN*>(pIAttachment);
            pCAttachmentSkin->ReleaseRemapTablePair();
        }
    }
}

void CAttachmentManager::RebindAttachments(uint32 nLoadingFlags)
{
    for (IAttachment* pIAttachment : m_arrAttachments)
    {
        if (pIAttachment->GetType() == CA_SKIN)
        {
            CAttachmentSKIN* pCAttachmentSkin = static_cast<CAttachmentSKIN*>(pIAttachment);
            pCAttachmentSkin->Rebind(nLoadingFlags);
        }
    }
}

const char* CAttachmentManager::ExecProcFunction(uint32 nCRC32, Skeleton::CPoseData* pPoseData, const char* pstrFunction) const
{
    return 0;
}


