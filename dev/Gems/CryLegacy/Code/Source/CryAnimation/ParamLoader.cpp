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
#include "CharacterManager.h"
#include "Model.h"
#include "ParamLoader.h"
#include "StringUtils.h"
#include <AzCore/Casting/numeric_cast.h>

#include "FacialAnimation/FacialModel.h"
#include "FacialAnimation/FaceEffectorLibrary.h"

#include <IPlatformOS.h>

// gcc only allows function attributes at the definition not the declaration
void Warning(const char* calFileName, EValidatorSeverity severity, const char* format, ...) PRINTF_PARAMS(3, 4);

void Warning(const char* calFileName, EValidatorSeverity severity, const char* format, ...)
{
    if (!g_pISystem || !format)
    {
        return;
    }

    va_list ArgList;
    va_start(ArgList, format);
    g_pISystem->WarningV(VALIDATOR_MODULE_ANIMATION, severity, VALIDATOR_FLAG_FILE, calFileName, format, ArgList);
    va_end(ArgList);
}

SAnimFile::SAnimFile(const stack_string& szFilePath, const stack_string& szAnimName)
{
    stack_string tmp = szFilePath;
    CryStringUtils::UnifyFilePath(tmp);
    tmp.MakeLower();

    memset(m_FilePathQ, 0, sizeof(m_FilePathQ));
    uint32 len1 = tmp.length();
    for (uint32 i = 0; i < len1; i++)
    {
        m_FilePathQ[i] = tmp[i];
    }

    memset(m_AnimNameQ, 0, sizeof(m_AnimNameQ));
    uint32 len2 = szAnimName.length();
    for (uint32 i = 0; i < len2; i++)
    {
        m_AnimNameQ[i] = szAnimName[i];
    }

    m_crcAnim = CCrc32::ComputeLowercase(szAnimName);
}

namespace
{
    struct FindByAliasName
    {
        uint32 fileCrc;

        bool operator () (const SAnimFile& animFile) const
        {
            return fileCrc == animFile.m_crcAnim;
        }

        bool operator () (const CAnimationSet::FacialAnimationEntry& facialAnimEntry) const
        {
            return fileCrc == facialAnimEntry.crc;
        }
    };

    void InitalizeBlendRotJointIndices(DynArray<DirectionalBlends>& directionalBlendArray, const DynArray<SJointsAimIK_Rot>& rotationsArray)
    {
        const uint32 directionalBlendsCount = directionalBlendArray.size();
        for (uint32 i = 0; i < directionalBlendsCount; ++i)
        {
            DirectionalBlends& directionalBlend = directionalBlendArray[i];
            const uint32 rotationJointsCount = rotationsArray.size();
            for (uint32 r = 0; r < rotationJointsCount; ++r)
            {
                const SJointsAimIK_Rot& jointAimIkRot = rotationsArray[r];
                const int16 jointIndex = jointAimIkRot.m_nJointIdx;
                if (jointIndex == directionalBlend.m_nParaJointIdx)
                {
                    directionalBlend.m_nRotParaJointIdx = r;
                }
                if (jointIndex == directionalBlend.m_nStartJointIdx)
                {
                    directionalBlend.m_nRotStartJointIdx = r;
                }
            }
        }
    }


    bool FindJointInParentHierarchy(const CDefaultSkeleton* const pDefaultSkeleton, const int16 jointIdToSearch, const int16 jointIdToStartSearchFrom)
    {
        CRY_ASSERT(pDefaultSkeleton);
        AZ_Warning("Animation", 0 <= jointIdToSearch, "Joint index %d is out of range.", jointIdToSearch);
        AZ_Warning("Animation", 0 <= jointIdToStartSearchFrom, "Joint index %d is out of range.", jointIdToStartSearchFrom);

        int16 currentJoint = jointIdToStartSearchFrom;
        while (currentJoint != -1)
        {
            if (currentJoint == jointIdToSearch)
            {
                return true;
            }
            currentJoint = pDefaultSkeleton->GetJointParentIDByID(currentJoint);
        }

        return false;
    }
}

// TODO: Add compiler assert to check for alignment between these arrays and the enums in ParamLoader.h
static const string g_nodeTypeCategoryNames[] =
{
    "Lod",                  // e_chrParamsNodeBoneLod
    "BBoxExcludeList",      // e_chrParamsNodeBBoxExclude
    "BBoxIncludeList",      // e_chrParamsNodeBBoxInclude
    "BBoxExtensionList",    // e_chrParamsNodeBBoxExtension
    "UsePhysProxyBBox",     // e_chrParamsNodeUsePhysProxyBBox
    "IK_Definition",        // e_chrParamsNodeIKDefinition
    "AnimationList",        // e_chrParamsNodeAnimation
};

static const string g_ikSubNodeTypeNames[] =
{
    "LimbIK_Definition",            // e_chrParamsIKNodeSubTypeLimb
    "Animation_Driven_IK_Targets",  // e_chrParamsIKNodeSubTypeAnimDriven
    "FeetLock_Definition",          // e_chrParamsIKNodeSubTypeFootLock
    "Recoil_Definition",            // e_chrParamsIKNodeSubTypeRecoil
    "LookIK_Definition",            // e_chrParamsIKNodeSubTypeLook
    "AimIK_Definition",             // e_chrParamsIKNodeSubTypeAim
};

CBoneLodNode::CBoneLodNode(const CChrParams* owner)
    : m_owner(owner)
{
}

CBoneLodNode::~CBoneLodNode()
{
    stl::free_container(m_boneLods);
}

void CBoneLodNode::SerializeToXml(XmlNodeRef parentNode) const
{
    uint lodCount = m_boneLods.size();
    for (uint i = 0; i < lodCount; ++i)
    {
        XmlNodeRef jointList = g_pISystem->CreateXmlNode("JointList");
        parentNode->addChild(jointList);
        jointList->setAttr("level", m_boneLods[i].m_level);
        uint jointCount = GetJointCount(i);
        for (uint j = 0; j < jointCount; ++j)
        {
            XmlNodeRef joint = g_pISystem->CreateXmlNode("Joint");
            joint->setAttr("name", GetJointName(i, j));
            jointList->addChild(joint);
        }
    }
}

bool CBoneLodNode::SerializeFromXml(XmlNodeRef inputNode)
{
    uint lodCount = inputNode->getChildCount();
    m_boneLods.reserve(lodCount);
    for (uint i = 0; i < lodCount; ++i)
    {
        XmlNodeRef lodNode = inputNode->getChild(i);
        int level;
        if (!lodNode->isTag("JointList"))
        {
            continue;
        }

        if (!lodNode->getAttr("level", level))
        {
            continue;
        }

        SBoneLod lod;
        lod.m_level = level;
        uint jointCount = lodNode->getChildCount();
        lod.m_jointNames.reserve(jointCount);
        for (uint j = 0; j < jointCount; j++)
        {
            XmlNodeRef jointNode = lodNode->getChild(j);
            if (!jointNode->isTag("Joint"))
            {
                continue;
            }

            const char* name = jointNode->getAttr("name");
            if (!name || !name[0])
            {
                continue;
            }

            lod.m_jointNames.push_back(name);
        }

        m_boneLods.push_back(lod);
    }

    return true;
}

void CBoneLodNode::AddBoneLod(uint level, const DynArray<string>& jointList)
{
    SBoneLod lod;
    lod.m_level = level;
    lod.m_jointNames.push_back(jointList);
    m_boneLods.push_back(lod);
}

CBBoxExcludeNode::CBBoxExcludeNode(const CChrParams* owner)
    : m_owner(owner)
{
}

CBBoxExcludeNode::~CBBoxExcludeNode()
{
    stl::free_container(m_jointNames);
}

void CBBoxExcludeNode::SerializeToXml(XmlNodeRef parentNode) const
{
    uint jointCount = m_jointNames.size();
    for (uint i = 0; i < jointCount; ++i)
    {
        XmlNodeRef excludeNode = g_pISystem->CreateXmlNode("Joint");
        excludeNode->setAttr("name", m_jointNames[i].c_str());
        parentNode->addChild(excludeNode);
    }
}

bool CBBoxExcludeNode::SerializeFromXml(XmlNodeRef inputNode)
{
    if (inputNode == 0)
    {
        return false;
    }

    if (!inputNode->isTag("BBoxExcludeList"))
    {
        return false;
    }

    uint jointCount = inputNode->getChildCount();
    for (uint i = 0; i < jointCount; ++i)
    {
        XmlNodeRef jointNode = inputNode->getChild(i);

        if (!jointNode->isTag("Joint"))
        {
            return false;
        }

        const char* name = jointNode->getAttr("name");
        if (!name)
        {
            return false;
        }

        m_jointNames.push_back(name);
    }
    return true;
}

void CBBoxExcludeNode::AddExcludeJoints(const DynArray<string>& jointList)
{
    m_jointNames.push_back(jointList);
}

CBBoxIncludeListNode::CBBoxIncludeListNode(const CChrParams* owner)
    : m_owner(owner)
{
}

CBBoxIncludeListNode::~CBBoxIncludeListNode()
{
    stl::free_container(m_jointNames);
}

void CBBoxIncludeListNode::SerializeToXml(XmlNodeRef parentNode) const
{
    for (uint i = 0; i < m_jointNames.size(); ++i)
    {
        XmlNodeRef includeNode = g_pISystem->CreateXmlNode("Joint");
        includeNode->setAttr("name", m_jointNames[i].c_str());
        parentNode->addChild(includeNode);
    }
}

bool CBBoxIncludeListNode::SerializeFromXml(XmlNodeRef inputNode)
{
    if (inputNode == 0)
    {
        return false;
    }

    if (!inputNode->isTag("BBoxIncludeList"))
    {
        return false;
    }

    uint jointCount = inputNode->getChildCount();
    for (uint i = 0; i < jointCount; ++i)
    {
        XmlNodeRef jointNode = inputNode->getChild(i);

        if (!jointNode->isTag("Joint"))
        {
            return false;
        }

        const char* name = jointNode->getAttr("name");
        if (!name)
        {
            return false;
        }

        m_jointNames.push_back(name);
    }
    return true;
}

void CBBoxIncludeListNode::AddIncludeJoints(const DynArray<string>& jointList)
{
    m_jointNames.push_back(jointList);
}

CBBoxExtensionNode::CBBoxExtensionNode(const CChrParams* owner)
    : m_owner(owner)
{
}

CBBoxExtensionNode::~CBBoxExtensionNode()
{
}

void CBBoxExtensionNode::SerializeToXml(XmlNodeRef parentNode) const
{
    XmlNodeRef axisNode = g_pISystem->CreateXmlNode("Axis");
    axisNode->setAttr("negX", m_bbMin.x);
    axisNode->setAttr("negY", m_bbMin.y);
    axisNode->setAttr("negZ", m_bbMin.z);
    axisNode->setAttr("posX", m_bbMax.x);
    axisNode->setAttr("posY", m_bbMax.y);
    axisNode->setAttr("posZ", m_bbMax.z);
    parentNode->addChild(axisNode);
}

bool CBBoxExtensionNode::SerializeFromXml(XmlNodeRef inputNode)
{
    if (inputNode == 0 || inputNode->getChildCount() < 1)
    {
        return false;
    }

    XmlNodeRef axisNode = inputNode->getChild(0);

    if (!axisNode->isTag("Axis"))
    {
        return false;
    }

    Vec3 bbMin;
    Vec3 bbMax;

    if (!axisNode->getAttr("negX", bbMin.x))
    {
        return false;
    }

    if (!axisNode->getAttr("negY", bbMin.y))
    {
        return false;
    }

    if (!axisNode->getAttr("negZ", bbMin.z))
    {
        return false;
    }

    if (!axisNode->getAttr("posX", bbMax.x))
    {
        return false;
    }

    if (!axisNode->getAttr("posY", bbMax.y))
    {
        return false;
    }

    if (!axisNode->getAttr("posZ", bbMax.z))
    {
        return false;
    }

    m_bbMin = bbMin;
    m_bbMax = bbMax;

    return true;
}

void CBBoxExtensionNode::GetBBoxMaxMin(Vec3& bboxMin, Vec3& bboxMax) const
{
    bboxMin = m_bbMin;
    bboxMax = m_bbMax;
}

void CBBoxExtensionNode::SetBBoxExpansion(Vec3 bbMin, Vec3 bbMax)
{
    m_bbMin = bbMin;
    m_bbMax = bbMax;
}

CLimbIKDefs::CLimbIKDefs(const CChrParams* owner)
    : m_owner(owner)
{
}

CLimbIKDefs::~CLimbIKDefs()
{
    stl::free_container(m_limbIKDefs);
}

void CLimbIKDefs::SerializeToXml(XmlNodeRef parentNode) const
{
    uint limbIKCount = GetLimbIKDefCount();
    for (uint i = 0; i < limbIKCount; ++i)
    {
        const SLimbIKDef* limbDef = GetLimbIKDef(i);
        XmlNodeRef newNode = g_pISystem->CreateXmlNode("IK");
        newNode->setAttr("Handle", limbDef->m_handle.c_str());
        newNode->setAttr("Solver", limbDef->m_solver.c_str());
        if (limbDef->m_solver == "CCD5")
        {
            assert(limbDef->m_ccdFiveJoints.size() == 5);
            for (uint i = 0; i < 5; ++i)
            {
                char jointHandle[3];
                sprintf_s(jointHandle, "J%i", i);
                newNode->setAttr(jointHandle, limbDef->m_ccdFiveJoints[i].c_str());
            }
        }
        else
        {
            newNode->setAttr("EndEffector", limbDef->m_endEffector.c_str());
            newNode->setAttr("Root", limbDef->m_rootJoint.c_str());
            if (limbDef->m_solver == "CCDX")
            {
                newNode->setAttr("fStepSize", limbDef->m_stepSize);
                newNode->setAttr("fThreshold", limbDef->m_threshold);
                newNode->setAttr("nMaxInterations", limbDef->m_maxIterations);
            }
        }
        parentNode->addChild(newNode);
    }
}

bool CLimbIKDefs::SerializeFromXml(XmlNodeRef inputNode)
{
    uint childCount = inputNode->getChildCount();
    m_limbIKDefs.clear();
    m_limbIKDefs.reserve(childCount);
    for (uint i = 0; i < childCount; ++i)
    {
        SLimbIKDef parsedLimbIK;
        XmlNodeRef limbIKNode = inputNode->getChild(i);
        if (!limbIKNode->isTag("IK"))
        {
            Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found unexpected node type %s in Limb IK block, ignoring", limbIKNode->getTag());
            continue;
        }
        parsedLimbIK.m_handle = limbIKNode->getAttr("Handle");
        if (parsedLimbIK.m_handle.empty())
        {
            Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found limb IK Node without mandatory field Handle");
            continue;
        }
        parsedLimbIK.m_solver = limbIKNode->getAttr("Solver");
        if (parsedLimbIK.m_solver.empty())
        {
            Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found limb IK Node %s without mandatory field Solver", parsedLimbIK.m_handle.c_str());
            continue;
        }
        if (parsedLimbIK.m_solver == "CCD5")
        {
            parsedLimbIK.m_ccdFiveJoints.reserve(5);
            for (uint j = 0; j < 5; ++j)
            {
                char jointHandle[3];
                sprintf_s(jointHandle, "J%i", j);
                string jointName = limbIKNode->getAttr(jointHandle);
                if (jointName.empty())
                {
                    Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found malformed limb IK Node %s using CC5 solver without mandatory field %s", parsedLimbIK.m_handle.c_str(), jointHandle);
                    continue;
                }
                else
                {
                    parsedLimbIK.m_ccdFiveJoints.push_back(jointName);
                }
            }
        }
        else
        {
            if (parsedLimbIK.m_solver == "CCDX")
            {
                if (!limbIKNode->getAttr("fStepSize", parsedLimbIK.m_stepSize))
                {
                    Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found malformed limb IK Node %s using solver %s without mandatory field fStepSize", parsedLimbIK.m_handle.c_str(), parsedLimbIK.m_solver.c_str());
                    continue;
                }
                if (!limbIKNode->getAttr("fThreshold", parsedLimbIK.m_threshold))
                {
                    Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found malformed limb IK Node %s using solver %s without mandatory field fThreshold", parsedLimbIK.m_handle.c_str(), parsedLimbIK.m_solver.c_str());
                    continue;
                }
                if (!limbIKNode->getAttr("nMaxInterations", parsedLimbIK.m_maxIterations))
                {
                    Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found malformed limb IK Node %s using solver %s without mandatory field nMaxInterations", parsedLimbIK.m_handle.c_str(), parsedLimbIK.m_solver.c_str());
                    continue;
                }
            }

            parsedLimbIK.m_rootJoint = limbIKNode->getAttr("Root");
            if (parsedLimbIK.m_rootJoint.empty())
            {
                Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found malformed limb IK Node %s using solver %s without mandatory field Root", parsedLimbIK.m_handle.c_str(), parsedLimbIK.m_solver.c_str());
                continue;
            }

            parsedLimbIK.m_endEffector = limbIKNode->getAttr("EndEffector");
            if (parsedLimbIK.m_endEffector.empty())
            {
                Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found malformed limb IK Node %s using solver %s without mandatory field EndEffector", parsedLimbIK.m_handle.c_str(), parsedLimbIK.m_solver.c_str());
                continue;
            }
        }
        m_limbIKDefs.push_back(parsedLimbIK);
    }

    return (m_limbIKDefs.size() > 0);
}

CAnimDrivenIKDefs::CAnimDrivenIKDefs(const CChrParams* owner)
    : m_owner(owner)
{
}

CAnimDrivenIKDefs::~CAnimDrivenIKDefs()
{
    stl::free_container(m_animDrivenIKDefs);
}

void CAnimDrivenIKDefs::SerializeToXml(XmlNodeRef parentNode) const
{
    uint animDrivenIKCount = GetAnimDrivenIKDefCount();
    for (uint i = 0; i < animDrivenIKCount; ++i)
    {
        const SAnimDrivenIKDef* adIKDef = GetAnimDrivenIKDef(i);
        XmlNodeRef newNode = g_pISystem->CreateXmlNode("ADIKTarget");
        newNode->setAttr("Handle", adIKDef->m_handle);
        newNode->setAttr("Target", adIKDef->m_targetJoint);
        newNode->setAttr("Weight", adIKDef->m_weightJoint);
        parentNode->addChild(newNode);
    }
}

bool CAnimDrivenIKDefs::SerializeFromXml(XmlNodeRef inputNode)
{
    uint childCount = inputNode->getChildCount();
    m_animDrivenIKDefs.reserve(childCount);

    for (uint i = 0; i < childCount; ++i)
    {
        XmlNodeRef adIKNode = inputNode->getChild(i);
        if (!adIKNode->isTag("ADIKTarget"))
        {
            Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found unexpected node type %s in Anim Driven IK block, ignoring", adIKNode->getTag());
            continue;
        }

        SAnimDrivenIKDef adIKDef;
        adIKDef.m_handle = adIKNode->getAttr("Handle");
        if (adIKDef.m_handle.empty())
        {
            Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found malformed Anim Driven IK Node without mandatory field Handle");
            continue;
        }

        adIKDef.m_targetJoint = adIKNode->getAttr("Target");
        if (adIKDef.m_targetJoint.empty())
        {
            Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found malformed Anim Driven IK Node %s without mandatory field Target", adIKDef.m_handle.c_str());
            continue;
        }

        adIKDef.m_weightJoint = adIKNode->getAttr("Weight");
        if (adIKDef.m_weightJoint.empty())
        {
            Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found malformed Anim Driven IK Node %s without mandatory field Weight", adIKDef.m_handle.c_str());
            continue;
        }

        m_animDrivenIKDefs.push_back(adIKDef);
    }
    return (m_animDrivenIKDefs.size() > 0);
}

CFootLockIKDefs::CFootLockIKDefs(const CChrParams* owner)
    : m_owner(owner)
{
}

CFootLockIKDefs::~CFootLockIKDefs()
{
    stl::free_container(m_footLockDefs);
}

void CFootLockIKDefs::SerializeToXml(XmlNodeRef parentNode) const
{
    uint footLockCount = GetFootLockIKDefCount();
    for (uint i = 0; i < footLockCount; ++i)
    {
        XmlNodeRef newNode = g_pISystem->CreateXmlNode("IKHandle");
        newNode->setAttr("Handle", GetFootLockIKDef(i));
        parentNode->addChild(newNode);
    }
}

bool CFootLockIKDefs::SerializeFromXml(XmlNodeRef inputNode)
{
    uint footLockCount = inputNode->getChildCount();
    m_footLockDefs.reserve(footLockCount);

    for (uint i = 0; i < footLockCount; ++i)
    {
        XmlNodeRef footLockNode = inputNode->getChild(i);
        const char* strTag = footLockNode->getTag();

        if (_stricmp(strTag, "IKHandle") == 0 ||
            _stricmp(strTag, "RIKHandle") == 0 ||
            _stricmp(strTag, "LIKHandle") == 0)
        {
            string footLockHandle = footLockNode->getAttr("Handle");
            if (footLockHandle.empty())
            {
                Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found malformed Foot Lock IK Node without mandatory field Handle");
                continue;
            }
            if (m_footLockDefs.size() == MAX_FEET_AMOUNT)
            {
                Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found too many Foot Lock nodes (maximum is %i), ignoring %s", MAX_FEET_AMOUNT, footLockHandle.c_str());
                continue;
            }
            m_footLockDefs.push_back(footLockHandle);
        }
        else
        {
            Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found unexpected node type %s in Foot Lock IK block, ignoring", strTag);
        }
    }

    return (m_footLockDefs.size() > 0);
}

CRecoilIKDef::CRecoilIKDef(const CChrParams* owner)
    : m_owner(owner)
{
}

CRecoilIKDef::~CRecoilIKDef()
{
    stl::free_container(m_impactJoints);
}

void CRecoilIKDef::SerializeToXml(XmlNodeRef parentNode) const
{
    // Right Handle
    if (m_rightIKHandle.length() > 0)
    {
        XmlNodeRef rightHandle = g_pISystem->CreateXmlNode("RIKHandle");
        rightHandle->setAttr("Handle", m_rightIKHandle.c_str());
        parentNode->addChild(rightHandle);
    }

    // Left Handle
    if (m_leftIKHandle.length() > 0)
    {
        XmlNodeRef leftHandle = g_pISystem->CreateXmlNode("LIKHandle");
        leftHandle->setAttr("Handle", m_leftIKHandle.c_str());
        parentNode->addChild(leftHandle);
    }

    // Right Joint
    if (m_rightWeaponJoint.length() > 0)
    {
        XmlNodeRef rightJoint = g_pISystem->CreateXmlNode("RWeaponJoint");
        rightJoint->setAttr("JointName", m_rightWeaponJoint.c_str());
        parentNode->addChild(rightJoint);
    }

    // Left Joint
    if (m_leftWeaponJoint.length() > 0)
    {
        XmlNodeRef leftJoint = g_pISystem->CreateXmlNode("LWeaponJoint");
        leftJoint->setAttr("JointName", m_leftWeaponJoint.c_str());
        parentNode->addChild(leftJoint);
    }

    // Impact Joints
    uint impactJointCount = GetImpactJointCount();
    if (impactJointCount > 0)
    {
        XmlNodeRef impactJointList = g_pISystem->CreateXmlNode("ImpactJoints");
        for (uint i = 0; i < impactJointCount; ++i)
        {
            const SRecoilImpactJoint* impactJoint = GetImpactJoint(i);
            XmlNodeRef impactJointNode = g_pISystem->CreateXmlNode("ImpactJoint");
            impactJointNode->setAttr("Arm", impactJoint->m_arm);
            impactJointNode->setAttr("Delay", impactJoint->m_delay);
            impactJointNode->setAttr("Weight", impactJoint->m_weight);
            impactJointNode->setAttr("JointName", impactJoint->m_jointName);
            impactJointList->addChild(impactJointNode);
        }
        parentNode->addChild(impactJointList);
    }
}

bool CRecoilIKDef::SerializeFromXml(XmlNodeRef inputNode)
{
    uint childCount = inputNode->getChildCount();
    for (uint i = 0; i < childCount; ++i)
    {
        XmlNodeRef childNode = inputNode->getChild(i);
        if (childNode->isTag("RIKHandle"))
        {
            m_rightIKHandle = childNode->getAttr("Handle");
            if (m_rightIKHandle.empty())
            {
                Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found malformed Recoil IK Node with RIKHandle subnode without mandatory field Handle");
                return false;
            }
        }
        else if (childNode->isTag("LIKHandle"))
        {
            m_leftIKHandle = childNode->getAttr("Handle");
            if (m_leftIKHandle.empty())
            {
                Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found malformed Recoil IK Node with LIKHandle subnode without mandatory field Handle");
                return false;
            }
        }
        else if (childNode->isTag("RWeaponJoint"))
        {
            m_rightWeaponJoint = childNode->getAttr("JointName");
            if (m_rightWeaponJoint.empty())
            {
                Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found malformed Recoil IK Node with RWeaponJoint subnode without mandatory field JointName");
                return false;
            }
        }
        else if (childNode->isTag("LWeaponJoint"))
        {
            m_leftWeaponJoint = childNode->getAttr("JointName");
            if (m_leftWeaponJoint.empty())
            {
                Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found malformed Recoil IK Node with LWeaponJoint subnode without mandatory field JointName");
                return false;
            }
        }
        else if (childNode->isTag("ImpactJoints"))
        {
            uint impactJointCount = childNode->getChildCount();
            for (uint j = 0; j < impactJointCount; ++j)
            {
                XmlNodeRef impactJoint = childNode->getChild(j);
                SRecoilImpactJoint jointData;
                if (!impactJoint->getAttr("Arm", jointData.m_arm))
                {
                    Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found malformed ImpactJoint subnode in Recoil IK Node without mandatory field Arm");
                    return false;
                }
                if (!impactJoint->getAttr("Delay", jointData.m_delay))
                {
                    Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found malformed ImpactJoint subnode in Recoil IK Node without mandatory field Delay");
                    return false;
                }
                if (!impactJoint->getAttr("Weight", jointData.m_weight))
                {
                    Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found malformed ImpactJoint subnode in Recoil IK Node without mandatory field Weight");
                    return false;
                }
                jointData.m_jointName = impactJoint->getAttr("JointName");
                if (jointData.m_jointName.empty())
                {
                    Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found malformed ImpactJoint subnode in Recoil IK Node without mandatory field JointName");
                    return false;
                }

                m_impactJoints.push_back(jointData);
            }
        }
        else
        {
            Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found unexpected node %s in Recoil IK block, discarding", childNode->getTag());
            continue;
        }
    }

    return true;
}

template<typename T>
CAimLookBase<T>::~CAimLookBase()
{
    stl::free_container(m_positionList);
    stl::free_container(m_rotationList);
    stl::free_container(m_directionalBlends);
}

template<typename T>
void CAimLookBase<T>::SerializeToXml(XmlNodeRef parentNode) const
{
    // Directional Blend
    XmlNodeRef directionalBlendNode = g_pISystem->CreateXmlNode("DirectionalBlends");
    uint directionalBlendCount = m_directionalBlends.size();
    for (uint i = 0; i < directionalBlendCount; ++i)
    {
        XmlNodeRef jointNode = g_pISystem->CreateXmlNode("Joint");
        jointNode->setAttr("AnimToken", m_directionalBlends[i].m_animToken);
        jointNode->setAttr("ParameterJoint", m_directionalBlends[i].m_parameterJoint);
        jointNode->setAttr("StartJoint", m_directionalBlends[i].m_startJoint);
        jointNode->setAttr("ReferenceJoint", m_directionalBlends[i].m_referenceJoint);
        directionalBlendNode->addChild(jointNode);
    }
    parentNode->addChild(directionalBlendNode);

    // RotationList
    XmlNodeRef rotationListNode = g_pISystem->CreateXmlNode("RotationList");
    uint rotationCount = m_rotationList.size();
    for (uint i = 0; i < rotationCount; ++i)
    {
        XmlNodeRef rotationNode = g_pISystem->CreateXmlNode("Rotation");
        rotationNode->setAttr("JointName", m_rotationList[i].m_jointName);
        rotationNode->setAttr("Primary", m_rotationList[i].m_primary);
        rotationNode->setAttr("Additive", m_rotationList[i].m_additive);
        rotationListNode->addChild(rotationNode);
    }
    parentNode->addChild(rotationListNode);

    // PositionList
    XmlNodeRef positionListNode = g_pISystem->CreateXmlNode("PositionList");
    uint positionCount = m_positionList.size();
    for (uint i = 0; i < positionCount; ++i)
    {
        XmlNodeRef positionNode = g_pISystem->CreateXmlNode("Position");
        positionNode->setAttr("JointName", m_positionList[i].m_jointName);
        positionNode->setAttr("Additive", m_positionList[i].m_additive);
        positionListNode->addChild(positionNode);
    }
    parentNode->addChild(positionListNode);
}

template<typename T>
bool CAimLookBase<T>::HandleUnknownNode(XmlNodeRef inputNode)
{
    if (inputNode->isTag("DirectionalBlends"))
    {
        if (!SerializeDirectionalBlendsFromXml(inputNode))
        {
            Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Failed to parse DirectionalBlends subnode in Aim IK Node");
        }
        return true;
    }
    else if (inputNode->isTag("RotationList"))
    {
        if (!SerializeRotationListFromXml(inputNode))
        {
            Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Failed to parse RotationList subnode in Aim IK Node");
        }
        return true;
    }
    else if (inputNode->isTag("PositionList"))
    {
        if (!SerializePositionListFromXml(inputNode))
        {
            Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Failed to parse PositionList subnode in Aim IK Node");
        }
        return true;
    }
    else
    {
        return false;
    }
}

template<typename T>
bool CAimLookBase<T>::SerializeDirectionalBlendsFromXml(XmlNodeRef dirBlendsNode)
{
    uint dirBlendCount = dirBlendsNode->getChildCount();
    m_directionalBlends.reserve(dirBlendCount);
    for (uint i = 0; i < dirBlendCount; ++i)
    {
        XmlNodeRef dirBlendNode = dirBlendsNode->getChild(i);
        if (!dirBlendNode->isTag("Joint"))
        {
            Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found unknown subnode %s in DirectionalBlends, ignoring", dirBlendNode->getTag());
            continue;
        }

        SAimLookDirectionalBlend dirBlendData;
        dirBlendData.m_animToken = dirBlendNode->getAttr("AnimToken");
        if (dirBlendData.m_animToken.empty())
        {
            Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found malformed Joint subnode in DirectionalBlends without mandatory field AnimToken");
            continue;
        }
        dirBlendData.m_parameterJoint = dirBlendNode->getAttr("ParameterJoint");
        if (dirBlendData.m_parameterJoint.empty())
        {
            Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found malformed Joint subnode in DirectionalBlends without mandatory field ParameterJoint");
            continue;
        }
        dirBlendData.m_startJoint = dirBlendNode->getAttr("StartJoint");
        if (dirBlendData.m_startJoint.empty())
        {
            Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found malformed Joint subnode in DirectionalBlends without mandatory field StartJoint");
            continue;
        }
        dirBlendData.m_referenceJoint = dirBlendNode->getAttr("ReferenceJoint");
        if (dirBlendData.m_referenceJoint.empty())
        {
            Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found malformed Joint subnode in DirectionalBlends without mandatory field ReferenceJoint");
            continue;
        }
        m_directionalBlends.push_back(dirBlendData);
    }

    return (m_directionalBlends.size() > 0);
}

template<typename T>
bool CAimLookBase<T>::SerializeRotationListFromXml(XmlNodeRef rotationListNode)
{
    uint rotCount = rotationListNode->getChildCount();
    m_rotationList.reserve(rotCount);
    for (uint i = 0; i < rotCount; ++i)
    {
        XmlNodeRef rotationNode = rotationListNode->getChild(i);
        if (!rotationNode->isTag("Rotation"))
        {
            Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found unknown subnode %s in RotationList, ignoring", rotationNode->getTag());
            continue;
        }

        SAimLookPosRot rotData;
        rotationNode->getAttr("Primary", rotData.m_primary);
        rotationNode->getAttr("Additive", rotData.m_additive);
        rotData.m_jointName = rotationNode->getAttr("JointName");
        if (rotData.m_jointName.empty())
        {
            Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found found malformed Rotation subnode in RotationList node without mandatory field JointName");
            continue;
        }
        m_rotationList.push_back(rotData);
    }

    return (m_rotationList.size() > 0);
}

template<typename T>
bool CAimLookBase<T>::SerializePositionListFromXml(XmlNodeRef posListNode)
{
    uint posCount = posListNode->getChildCount();
    m_positionList.reserve(posCount);
    for (uint i = 0; i < posCount; ++i)
    {
        XmlNodeRef posNode = posListNode->getChild(i);
        if (!posNode->isTag("Position"))
        {
            Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found unknown subnode %s in PositionList, ignoring", posNode->getTag());
            continue;
        }

        SAimLookPosRot posData;
        posNode->getAttr("Additive", posData.m_additive);
        posData.m_jointName = posNode->getAttr("JointName");
        if (posData.m_jointName.empty())
        {
            Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found found malformed Position subnode in PositionList without mandatory field JointName");
            continue;
        }
        m_positionList.push_back(posData);
    }

    return (m_positionList.size() > 0);
}

CLookIKDef::CLookIKDef(const CChrParams* owner)
    : CAimLookBase(owner)
{
}

CLookIKDef::~CLookIKDef()
{
}

void CLookIKDef::SerializeToXml(XmlNodeRef parentNode) const
{
    CAimLookBase::SerializeToXml(parentNode);
    // EyeLimits
    XmlNodeRef eyeLimitsNode = g_pISystem->CreateXmlNode("EyeLimits");
    eyeLimitsNode->setAttr("halfYawDegrees", m_eyeLimits.m_halfYaw);
    eyeLimitsNode->setAttr("pitchDegreesUp", m_eyeLimits.m_pitchUp);
    eyeLimitsNode->setAttr("pitchDegreesDown", m_eyeLimits.m_pitchDown);
    parentNode->addChild(eyeLimitsNode);

    // Eye Attachments
    if (!m_leftEyeAttachment.empty())
    {
        XmlNodeRef lEyeAttachNode = g_pISystem->CreateXmlNode("LEyeAttachment");
        lEyeAttachNode->setAttr("Name", m_leftEyeAttachment.c_str());
        parentNode->addChild(lEyeAttachNode);
    }

    if (!m_rightEyeAttachment.empty())
    {
        XmlNodeRef rEyeAttachNode = g_pISystem->CreateXmlNode("REyeAttachment");
        rEyeAttachNode->setAttr("Name", m_rightEyeAttachment.c_str());
        parentNode->addChild(rEyeAttachNode);
    }
}

bool CLookIKDef::SerializeFromXml(XmlNodeRef inputNode)
{
    uint childCount = inputNode->getChildCount();
    for (uint i = 0; i < childCount; ++i)
    {
        XmlNodeRef childNode = inputNode->getChild(i);
        if (childNode->isTag("EyeLimits"))
        {
            childNode->getAttr("halfYawDegrees", m_eyeLimits.m_halfYaw);
            childNode->getAttr("pitchDegreesUp", m_eyeLimits.m_pitchUp);
            childNode->getAttr("pitchDegreesDown", m_eyeLimits.m_pitchDown);
        }
        else if (childNode->isTag("LEyeAttachment"))
        {
            m_leftEyeAttachment = childNode->getAttr("Name");
            if (m_leftEyeAttachment.empty())
            {
                Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found malformed LEyeAttachment subnode in Look IK Node without mandatory field Name");
            }
        }
        else if (childNode->isTag("REyeAttachment"))
        {
            m_rightEyeAttachment = childNode->getAttr("Name");
            if (m_rightEyeAttachment.empty())
            {
                Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found malformed REyeAttachment subnode in Look IK Node without mandatory field Name");
            }
        }
        else if (!CAimLookBase::HandleUnknownNode(childNode))
        {
            Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found unknown subnode %s in Look IK node, ignoring", childNode->getTag());
        }
    }

    return true;
}

CAimIKDef::CAimIKDef(const CChrParams* owner)
    : CAimLookBase(owner)
{
}

CAimIKDef::~CAimIKDef()
{
    stl::free_container(m_procAdjustmentJoints);
}

void CAimIKDef::SerializeToXml(XmlNodeRef parentNode) const
{
    CAimLookBase::SerializeToXml(parentNode);
    // ProcAdjustments
    uint spineCount = m_procAdjustmentJoints.size();
    if (spineCount > 0)
    {
        XmlNodeRef procAdjustmentsNode = g_pISystem->CreateXmlNode("ProcAdjustments");
        for (uint i = 0; i < spineCount; ++i)
        {
            XmlNodeRef spineNode = g_pISystem->CreateXmlNode("Spine");
            spineNode->setAttr("JointName", m_procAdjustmentJoints[i].c_str());
            procAdjustmentsNode->addChild(spineNode);
        }
        parentNode->addChild(procAdjustmentsNode);
    }
}

bool CAimIKDef::SerializeFromXml(XmlNodeRef inputNode)
{
    uint childCount = inputNode->getChildCount();
    for (uint i = 0; i < childCount; ++i)
    {
        XmlNodeRef childNode = inputNode->getChild(i);

        if (childNode->isTag("ProcAdjustments"))
        {
            uint procAdjCount = childNode->getChildCount();
            m_procAdjustmentJoints.reserve(procAdjCount);
            for (uint i = 0; i < procAdjCount; ++i)
            {
                XmlNodeRef procAdjNode = childNode->getChild(i);
                if (!procAdjNode->isTag("Spine"))
                {
                    Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found unknown subnode %s in ProcAdjustments, ignoring", procAdjNode->getTag());
                    continue;
                }
                string jointName = procAdjNode->getAttr("JointName");
                if (jointName.empty())
                {
                    Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found malformed Joint subnode in ProcAdjustments without mandatory field JointName");
                    continue;
                }
                m_procAdjustmentJoints.push_back(jointName);
            }
        }
        else if (!CAimLookBase::HandleUnknownNode(childNode))
        {
            Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found unknown subnode %s in Aim IK node, ignoring", childNode->getTag());
        }
    }

    return true;
}

CPhysProxyBBoxOptionNode::CPhysProxyBBoxOptionNode(const CChrParams* owner)
    : m_owner(owner)
    , m_usePhysProxyBBoxNode(true)
{
    // Note: The existence of this node will imply that the flag is true
}

CPhysProxyBBoxOptionNode::~CPhysProxyBBoxOptionNode()
{
}

bool CPhysProxyBBoxOptionNode::GetUsePhysProxyBBoxNode() const
{
    return m_usePhysProxyBBoxNode;
}

void CPhysProxyBBoxOptionNode::SetUsePhysProxyBBoxNode(bool value)
{
    // Note: The existence of this node will imply that the flag is true.  This node was added by crytek as part of the 1/15/2015 integration.  If
    // in the future the node actually has a value, then we can actually set the value to something other than true
    m_usePhysProxyBBoxNode = value;
}

void CPhysProxyBBoxOptionNode::SerializeToXml(XmlNodeRef parentNode) const
{
    // So far, this node doesnt appear to have children, nothing to serialize
}


bool CPhysProxyBBoxOptionNode::SerializeFromXml(XmlNodeRef inputNode)
{
    // So far, this node doesnt appear to have children, so nothing to read
    return true;
}

EChrParamNodeType CPhysProxyBBoxOptionNode::GetType() const
{
    return e_chrParamsNodeUsePhysProxyBBox;
}

CIKDefinitionNode::CIKDefinitionNode(const CChrParams* owner)
    : m_owner(owner)
{
    for (int type = e_chrParamsIKNodeSubTypeFirst; type < e_chrParamsIKNodeSubTypeCount; ++type)
    {
        m_ikSubTypeNodes[type] = nullptr;
    }
}

CIKDefinitionNode::~CIKDefinitionNode()
{
    for (int type = e_chrParamsIKNodeSubTypeFirst; type < e_chrParamsIKNodeSubTypeCount; ++type)
    {
        if (m_ikSubTypeNodes[type] != nullptr)
        {
            delete m_ikSubTypeNodes[type];
            m_ikSubTypeNodes[type] = nullptr;
        }
    }
}

bool CIKDefinitionNode::HasIKSubNodeType(EChrParamsIKNodeSubType type) const
{
    return (m_ikSubTypeNodes[type] != nullptr);
}

IChrParamsNode* CIKDefinitionNode::GetEditableIKSubNode(EChrParamsIKNodeSubType type)
{
    if (m_ikSubTypeNodes[type] != nullptr)
    {
        delete m_ikSubTypeNodes[type];
        m_ikSubTypeNodes[type] = nullptr;
    }
    CreateNewIKSubNode(type);

    return m_ikSubTypeNodes[type];
}

void CIKDefinitionNode::SerializeToXml(XmlNodeRef parentNode) const
{
    for (int type = e_chrParamsIKNodeSubTypeFirst; type < e_chrParamsIKNodeSubTypeCount; ++type)
    {
        if (HasIKSubNodeType((EChrParamsIKNodeSubType)type))
        {
            XmlNodeRef categoryNode = g_pISystem->CreateXmlNode(g_ikSubNodeTypeNames[type]);
            parentNode->addChild(categoryNode);
            m_ikSubTypeNodes[type]->SerializeToXml(categoryNode);
        }
    }
}

bool CIKDefinitionNode::SerializeFromXml(XmlNodeRef inputNode)
{
    uint childCount = inputNode->getChildCount();

    for (uint i = 0; i < childCount; ++i)
    {
        XmlNodeRef childNode = inputNode->getChild(i);

        EChrParamsIKNodeSubType nodeType = e_chrParamsIKNodeSubTypeInvalid;
        for (int type = e_chrParamsIKNodeSubTypeFirst; type < e_chrParamsIKNodeSubTypeCount; ++type)
        {
            if (childNode->isTag(g_ikSubNodeTypeNames[type]))
            {
                nodeType = (EChrParamsIKNodeSubType)type;
                break;
            }
        }

        if (nodeType != e_chrParamsIKNodeSubTypeInvalid)
        {
            if (m_ikSubTypeNodes[nodeType] != nullptr)
            {
                Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found duplicate IK subnode of type %s", g_ikSubNodeTypeNames[nodeType].c_str());
                continue;
            }
            CreateNewIKSubNode(nodeType);
            m_ikSubTypeNodes[nodeType]->SerializeFromXml(childNode);
        }
        else
        {
            Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found unknown IK block of type %s", childNode->getTag());
        }
    }

    return true;
}

void CIKDefinitionNode::CreateNewIKSubNode(EChrParamsIKNodeSubType type)
{
    IChrParamsNode* newNode;
    switch (type)
    {
    case e_chrParamsIKNodeLimb:
        newNode = new CLimbIKDefs(m_owner);
        break;
    case e_chrParamsIKNodeAnimDriven:
        newNode = new CAnimDrivenIKDefs(m_owner);
        break;
    case e_chrParamsIKNodeFootLock:
        newNode = new CFootLockIKDefs(m_owner);
        break;
    case e_chrParamsIKNodeRecoil:
        newNode = new CRecoilIKDef(m_owner);
        break;
    case e_chrParamsIKNodeLook:
        newNode = new CLookIKDef(m_owner);
        break;
    case e_chrParamsIKNodeAim:
        newNode = new CAimIKDef(m_owner);
        break;
    default:
        Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Requested new IK subnode of invalid type");
        return;
    }

    if (newNode)
    {
        m_ikSubTypeNodes[type] = newNode;
    }
}

CAnimationListNode::CAnimationListNode(CChrParams* owner)
    : m_owner(owner)
{
}

CAnimationListNode::~CAnimationListNode()
{
    stl::free_container(m_parsedAnimations);
    stl::free_container(m_includedChrParams);
}

void CAnimationListNode::SerializeToXml(XmlNodeRef parentNode) const
{
    uint animationCount = m_parsedAnimations.size();
    for (uint i = 0; i < animationCount; ++i)
    {
        XmlNodeRef animNode = g_pISystem->CreateXmlNode("Animation");
        animNode->setAttr("name", m_parsedAnimations[i].m_name.c_str());
        animNode->setAttr("path", m_parsedAnimations[i].m_path.c_str());
        if (m_parsedAnimations[i].m_flags.length() > 0)
        {
            animNode->setAttr("flags", m_parsedAnimations[i].m_flags.c_str());
        }
        parentNode->addChild(animNode);
    }

    uint subChrParamsCount = m_includedChrParams.size();
    for (uint i = 0; i < subChrParamsCount; ++i)
    {
        m_includedChrParams[i]->SerializeToFile();
    }
}

bool CAnimationListNode::SerializeFromXml(XmlNodeRef inputNode)
{
    if (!inputNode->isTag("AnimationList"))
    {
        return false;
    }

    uint animationCount = inputNode->getChildCount();
    m_parsedAnimations.reserve(animationCount);
    for (uint i = 0; i < animationCount; ++i)
    {
        // Note: We are currently stripping comment nodes by this criteria
        XmlNodeRef animNode = inputNode->getChild(i);
        if (animNode->isTag("Comment"))
        {
            continue;
        }

        if (!animNode->isTag("Animation"))
        {
            Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found invalid node of type %s in AnimationList", animNode->getTag());
            continue;
        }

        const char* name = animNode->getAttr("name");
        if (!name || !name[0])
        {
            continue;
        }

        const char* path = animNode->getAttr("path");
        if (!path || !path[0])
        {
            continue;
        }

        const char* flags = animNode->getAttr("flags");

        EChrParamsAnimationNodeType type = GetAnimationNodeType(name, path);

        if (type != e_chrParamsAnimationNodeTypeInvalid)
        {
            SAnimNode node(name, path, flags, type);

            if (_stricmp(name, "$Include") == 0)
            {
                // We need to load another chrparams
                const CChrParams* depParams = static_cast<const CChrParams*>(m_owner->GetParamLoader()->GetChrParams(path));
                if (depParams)
                {
                    m_includedChrParams.push_back(depParams);
                }
            }

            m_parsedAnimations.push_back(node);
        }
        else
        {
            Warning(m_owner->GetName(), VALIDATOR_WARNING, "Warning: Found invalid animation node name = %s   path = %s", name, path);
        }
    }

    return true;
}

void CAnimationListNode::AddAnimations(const DynArray<SAnimNode>& animations)
{
    uint animCount = animations.size();
    string rootPath = "@assets@";
    for (uint i = 0; i < animCount; ++i)
    {
        m_parsedAnimations.push_back(animations[i]);
        if (animations[i].m_type == e_chrParamsAnimationNodeFilepath)
        {
            rootPath = animations[i].m_path;
        }
        if (animations[i].m_type == e_chrParamsAnimationNodeInclude)
        {
            string fullPath = rootPath + "\\" + animations[i].m_path;
            const CChrParams* subChrParams = static_cast<const CChrParams*>(m_owner->GetParamLoader()->GetChrParams(animations[i].m_path));
            if (subChrParams)
            {
                m_includedChrParams.push_back(subChrParams);
            }
        }
    }
}

bool CAnimationListNode::IsAnimationWildcard(const char* const path)
{
    if (!path || !path[0])
    {
        return false;
    }

    if (strchr(path, '*') != NULL || strchr(path, '?') != NULL)
    {
        return true;
    }

    return false;
}

EChrParamsAnimationNodeType CAnimationListNode::GetAnimationNodeType(const char* const name, const char* const path)
{
    if (!name || !name[0] || !path || !path[0])
    {
        return e_chrParamsAnimationNodeTypeInvalid;
    }


    if (name[0] == '#')
    {
        if (_stricmp(name, "#filepath") == 0)
        {
            return e_chrParamsAnimationNodeFilepath;
        }
        else if (_stricmp(name, "#ParseSubFolders") == 0)
        {
            return e_chrParamsAnimationNodeParseSubfolders;
        }
    }
    else if (name[0] == '$')
    {
        if (_stricmp(name, "$TracksDatabase") == 0)
        {
            if (IsAnimationWildcard(path))
            {
                return e_chrParamsAnimationNodeWildcardDBA;
            }
            else
            {
                return e_chrParamsAnimationNodeSingleDBA;
            }
        }
        else if (_stricmp(name, "$Include") == 0)
        {
            return e_chrParamsAnimationNodeInclude;
        }
        else if (_stricmp(name, "$AnimEventDatabase") == 0)
        {
            return e_chrParamsAnimationNodeAnimEventsDB;
        }
        else if (_stricmp(name, "$FaceLib") == 0)
        {
            return e_chrParamsAnimationNodeFaceLib;
        }
    }
    else
    {
        if (IsAnimationWildcard(path))
        {
            return e_chrParamsAnimationNodeWildcardAsset;
        }
        return e_chrParamsAnimationNodeSingleAsset;
    }

    // Did not match correctness criteria (e.g. started with a # or $ but did not match a keyword)
    return e_chrParamsAnimationNodeTypeInvalid;
}

CChrParams::CChrParams(string name, CParamLoader* paramLoader)
    : m_name(name)
    , m_paramLoader(paramLoader)
{
    for (int type = e_chrParamsNodeFirst; type < e_chrParamsNodeCount; ++type)
    {
        m_categoryNodes[type] = nullptr;
    }
}

CChrParams::~CChrParams()
{
    ClearLists();
}

bool CChrParams::AddNode(XmlNodeRef newNode, EChrParamNodeType type)
{
    // This means we have already found a block of this type, discard the new one
    if (m_categoryNodes[type])
    {
        Warning(GetName(), VALIDATOR_WARNING, "Warning: Found a second block of type %s, it will be discarded", g_nodeTypeCategoryNames[type].c_str());
        return false;
    }

    IChrParamsNode* newChrParamsNode = CreateCategoryNodeIfNeeded(type);

    if (newChrParamsNode && newChrParamsNode->SerializeFromXml(newNode))
    {
        return true;
    }
    else
    {
        m_categoryNodes[type] = nullptr;
        delete newChrParamsNode;
        Warning(GetName(), VALIDATOR_ERROR, "Error parsing %s: %s", g_nodeTypeCategoryNames[type].c_str(), static_cast<const char*>(newNode->getXML()));
        return false;
    }
}

void CChrParams::ClearLists()
{
    for (int type = e_chrParamsNodeFirst; type < e_chrParamsNodeCount; ++type)
    {
        if (m_categoryNodes[type])
        {
            delete m_categoryNodes[type];
            m_categoryNodes[type] = NULL;
        }
    }
}

void CChrParams::AddBoneLod(uint level, const DynArray<string>& jointNames)
{
    CBoneLodNode* boneLodNode = static_cast<CBoneLodNode*>(CreateCategoryNodeIfNeeded(e_chrParamsNodeBoneLod));
    if (boneLodNode)
    {
        boneLodNode->AddBoneLod(level, jointNames);
    }
}

void CChrParams::AddBBoxExcludeJoints(const DynArray<string>& jointNames)
{
    CBBoxExcludeNode* bboxExcludeNode = static_cast<CBBoxExcludeNode*>(CreateCategoryNodeIfNeeded(e_chrParamsNodeBBoxExcludeList));
    if (bboxExcludeNode)
    {
        bboxExcludeNode->AddExcludeJoints(jointNames);
    }
}

void CChrParams::AddBBoxIncludeJoints(const DynArray<string>& jointNames)
{
    CBBoxIncludeListNode* bboxIncludeNode = static_cast<CBBoxIncludeListNode*>(CreateCategoryNodeIfNeeded(e_chrParamsNodeBBoxIncludeList));
    if (bboxIncludeNode)
    {
        bboxIncludeNode->AddIncludeJoints(jointNames);
    }
}

void CChrParams::SetBBoxExpansion(Vec3 bbMin, Vec3 bbMax)
{
    CBBoxExtensionNode* bboxExpansionNode = static_cast<CBBoxExtensionNode*>(CreateCategoryNodeIfNeeded(e_chrParamsNodeBBoxExtensionList));
    if (bboxExpansionNode)
    {
        bboxExpansionNode->SetBBoxExpansion(bbMin, bbMax);
    }
}

IChrParamsNode* CChrParams::GetEditableCategoryNode(EChrParamNodeType type)
{
    if (m_categoryNodes[type])
    {
        delete m_categoryNodes[type];
        m_categoryNodes[type] = nullptr;
    }

    return CreateCategoryNodeIfNeeded(type);
}

void CChrParams::AddAnimations(const DynArray<SAnimNode>& animations)
{
    CAnimationListNode* animListNode = static_cast<CAnimationListNode*>(CreateCategoryNodeIfNeeded(e_chrParamsNodeAnimationList));
    if (animListNode)
    {
        animListNode->AddAnimations(animations);
    }
}

void CChrParams::SetUsePhysProxyBBox(bool value)
{
    if (value)
    {
        // If true, create the category node
        CPhysProxyBBoxOptionNode* usePhysProxyBboxNode = static_cast<CPhysProxyBBoxOptionNode*>(CreateCategoryNodeIfNeeded(e_chrParamsNodeUsePhysProxyBBox));
        usePhysProxyBboxNode->SetUsePhysProxyBBoxNode(true);
    }
    else
    {
        // If false, remove the node for now, if/until crytek expands on the definition of this node
        if (m_categoryNodes[e_chrParamsNodeUsePhysProxyBBox])
        {
            delete m_categoryNodes[e_chrParamsNodeUsePhysProxyBBox];
            m_categoryNodes[e_chrParamsNodeUsePhysProxyBBox] = nullptr;
        }
    }
}


bool CChrParams::SerializeToFile() const
{
    XmlNodeRef topRoot = g_pISystem->CreateXmlNode("Params");
    EChrParamNodeType currType = e_chrParamsNodeInvalid;
    XmlNodeRef categoryRoot;

    for (int type = e_chrParamsNodeFirst; type < e_chrParamsNodeCount; type++)
    {
        if (m_categoryNodes[type])
        {
            categoryRoot = g_pISystem->CreateXmlNode(g_nodeTypeCategoryNames[type]);
            topRoot->addChild(categoryRoot);
            m_categoryNodes[type]->SerializeToXml(categoryRoot);
        }
    }

    return topRoot->saveToFile(m_name);
}

IChrParamsNode* CChrParams::CreateCategoryNodeIfNeeded(EChrParamNodeType type)
{
    if (m_categoryNodes[type])
    {
        return m_categoryNodes[type];
    }

    IChrParamsNode* newChrParamsNode = NULL;
    switch (type)
    {
    case e_chrParamsNodeBoneLod:
        newChrParamsNode = new CBoneLodNode(this);
        break;
    case e_chrParamsNodeBBoxExcludeList:
        newChrParamsNode = new CBBoxExcludeNode(this);
        break;
    case e_chrParamsNodeBBoxIncludeList:
        newChrParamsNode = new CBBoxIncludeListNode(this);
        break;
    case e_chrParamsNodeBBoxExtensionList:
        newChrParamsNode = new CBBoxExtensionNode(this);
        break;
    case e_chrParamsNodeUsePhysProxyBBox:
        newChrParamsNode = new CPhysProxyBBoxOptionNode(this);
        break;
    case e_chrParamsNodeIKDefinition:
        newChrParamsNode = new CIKDefinitionNode(this);
        break;
    case e_chrParamsNodeAnimationList:
        newChrParamsNode = new CAnimationListNode(this);
        break;
    default:
        Warning(GetName(), VALIDATOR_ERROR, "Error: Unknown node type requested");
        break;
    }
    ;

    m_categoryNodes[type] = newChrParamsNode;
    return newChrParamsNode;
}

void CChrParams::ClearCategoryNode(EChrParamNodeType type)
{
    if (m_categoryNodes[type])
    {
        delete m_categoryNodes[type];
        m_categoryNodes[type] = nullptr;
    }
}

CParamLoader::CParamLoader()
{
    m_parsedLists.reserve(256);
}

CParamLoader::~CParamLoader(void)
{
    ClearLists();
}

// for a given list id (of an already loaded list) return all list i
bool CParamLoader::BuildDependencyList(int32 rootListID, DynArray<uint32>& listIDs)
{
    SAnimListInfo& animList = m_parsedLists[rootListID];

    int32 amountDep = animList.dependencies.size();
    for (int32 i = 0; i < amountDep; i++)
    {
        BuildDependencyList(animList.dependencies[i], listIDs);
    }
    listIDs.push_back(rootListID);
    return true;
}

int32 CParamLoader::ListProcessed(const char* paramFileName)
{
    // check if List has already been processed
    int32 nListIDs = m_parsedLists.size();
    for (int32 listID = 0; listID < nListIDs; listID++)
    {
        if (strcmp(m_parsedLists[listID].fileName, paramFileName) == 0)
        {
            return listID;
        }
    }
    return -1;
}

#define MAX_STRING_LENGTH (256)




// finds the first occurence of
CAF_ID CParamLoader::MemFindFirst(const char** ppAnimPath, const char* szMask, uint32 crcFolder, CAF_ID nCafID)
{
    LOADING_TIME_PROFILE_SECTION(g_pISystem);

    (*ppAnimPath) = NULL;

    AnimSearchHelper::TIndexVector* vect = g_AnimationManager.m_animSearchHelper.GetAnimationsVector(crcFolder);
    if (vect && nCafID.m_nType == 0)
    {
        for (size_t i = nCafID.m_nCafID, end = vect->size(); i < end; ++i)
        {
            GlobalAnimationHeaderCAF& rCAF = g_AnimationManager.m_arrGlobalCAF[(*vect)[i]];
            stack_string strCAFName = PathUtil::GetFile(rCAF.GetFilePath());

            if (PathUtil::MatchWildcard(strCAFName.c_str(), szMask))
            {
                (*ppAnimPath) = rCAF.GetFilePath();

                return CAF_ID(i, 0);
            }
        }
    }

    return CAF_ID(-1, -1);
}

void CParamLoader::ExpandWildcardsForPath(SAnimListInfo& animList, const char* szMask, uint32 crcFolder, const char* szAnimNamePre, const char* szAnimNamePost)
{
    const char* szAnimPath;
    CAF_ID nCafID =  MemFindFirst(&szAnimPath, szMask, crcFolder, CAF_ID(0, 0));
    if (nCafID.m_nCafID != -1)
    {
        do
        {
            stack_string animName = szAnimNamePre;
            animName.append(PathUtil::GetFileName(szAnimPath).c_str());
            if (szAnimNamePost)
            {
                animName.append(szAnimNamePost);
            }

            AddIfNewAnimationAlias(animList, animName.c_str(), szAnimPath);

            nCafID.m_nCafID++;
            nCafID = MemFindFirst(&szAnimPath, szMask, crcFolder, nCafID);
        } while (nCafID.m_nCafID >= 0);
    }
}

bool CParamLoader::ExpandWildcards(uint32 listID)
{
    LOADING_TIME_PROFILE_SECTION(g_pISystem);

    CDefaultSkeleton* pDefaultSkeleton = m_pDefaultSkeleton;
    SAnimListInfo& animList = m_parsedLists[listID];

    uint32 numAnims = animList.arrAnimFiles.size();
    if (numAnims == 0)
    {
        return 0;
    }

    // insert all animations found in the DBA files
    uint32 numDBALoaders = g_AnimationManager.m_arrGlobalHeaderDBA.size();
    for (uint32 j = 0; j < numDBALoaders; j++)
    {
        CGlobalHeaderDBA& rGlobalHeaderDBA = g_AnimationManager.m_arrGlobalHeaderDBA[j];
        if (rGlobalHeaderDBA.m_pDatabaseInfo == 0)
        {
            const char* pName  = rGlobalHeaderDBA.m_strFilePathDBA;
            uint32 nDBACRC32 = CCrc32::ComputeLowercase(pName);
            uint32 numCAF = g_AnimationManager.m_arrGlobalCAF.size();
            uint32 nFoundAssetsInAIF = 0;
            for (uint32 c = 0; c < numCAF; c++)
            {
                if (g_AnimationManager.m_arrGlobalCAF[c].m_FilePathDBACRC32 == nDBACRC32)
                {
                    nFoundAssetsInAIF = 1;
                    break;
                }
            }
            if (nFoundAssetsInAIF == 0)
            {
                rGlobalHeaderDBA.LoadDatabaseDBA(pDefaultSkeleton->GetModelFilePath());
                if (rGlobalHeaderDBA.m_pDatabaseInfo == 0)
                {
                    continue;
                }
            }
        }
    }

    //----------------------------------------------------------------------------------------
    // expand wildcards
    uint32 numWildcard = animList.arrWildcardAnimFiles.size();
    for (uint32 w = 0; w < numWildcard; ++w)
    {
        const char* szFolder = animList.arrWildcardAnimFiles[w].m_FilePathQ;
        const char* szFile = PathUtil::GetFile(szFolder);
        const char* szFilename = PathUtil::GetFileName(szFolder);
        const char* szExt = PathUtil::GetExt(szFolder);

        uint32 IsLMG = (strcmp(szExt, "lmg") == 0) || (strcmp(szExt, "bspace") == 0) || (strcmp(szExt, "comb") == 0);
        uint32 IsFSQ = strcmp(szExt, "fsq") == 0;

        char szAnimName[MAX_STRING_LENGTH];
        azstrcpy(szAnimName, MAX_STRING_LENGTH, animList.arrWildcardAnimFiles[w].m_AnimNameQ);

        const char* firstWildcard = strchr(szFolder, '*');
        if (const char* qm = strchr(szFolder, '?'))
        {
            if (firstWildcard == NULL || firstWildcard > qm)
            {
                firstWildcard = qm;
            }
        }
        int32 iFirstWildcard = (int32)(firstWildcard - szFolder);
        int32 iPathLength = min((int32)(szFile - szFolder), iFirstWildcard);

        bool parseSubfolders = (iFirstWildcard != (int32)(szFile - szFolder));

        // conversion to offset from beginning of name
        const char* fileWildcard = strchr(szFile, '*');
        int32 offset = (int32)(fileWildcard - szFile);

        stack_string filepath = PathUtil::GetParentDirectoryStackString(stack_string(szFolder));
        char* starPos = strchr(szAnimName, '*');
        if (starPos)
        {
            *starPos++ = 0;
        }

        char strAnimName[MAX_STRING_LENGTH];
        uint32 img = g_pCharacterManager->IsInitializedByIMG();
        if (img & 2  && IsLMG == 0 && IsFSQ == 0)
        {
            uint32 crcFolder = CCrc32::ComputeLowercase(szFolder, iPathLength);

            memset(strAnimName, 0, sizeof(strAnimName));

            AnimSearchHelper::TSubFolderCrCVector* subFolders = parseSubfolders ? g_AnimationManager.m_animSearchHelper.GetSubFoldersVector(crcFolder) : NULL;

            if (subFolders)
            {
                for (uint32 i = 0; i < subFolders->size(); i++)
                {
                    ExpandWildcardsForPath(animList, szFile, (*subFolders)[i], szAnimName, starPos);
                }
            }
            else
            {
                ExpandWildcardsForPath(animList, szFile, crcFolder, szAnimName, starPos);
            }

            const uint32 numAIM = g_AnimationManager.m_arrGlobalAIM.size();
            for (uint32 nCafID = 0; nCafID < numAIM; nCafID++)
            {
                GlobalAnimationHeaderAIM& rAIM = g_AnimationManager.m_arrGlobalAIM[nCafID];
                stack_string strFilename = PathUtil::GetFile(rAIM.GetFilePath());
                stack_string strFilePath = PathUtil::GetPath(rAIM.GetFilePath());
                const int32 filePathLen = strFilePath.length();

                if (parseSubfolders)
                {
                    if ((iPathLength > filePathLen) || (_strnicmp(szFolder, rAIM.GetFilePath(), iPathLength) != 0))
                    {
                        continue;
                    }
                }
                else
                {
                    if ((iPathLength != filePathLen) || (_strnicmp(szFolder, rAIM.GetFilePath(), iPathLength) != 0))
                    {
                        continue;
                    }
                }
                if (PathUtil::MatchWildcard(strFilename.c_str(), szFile))
                {
                    stack_string animName = szAnimName;
                    animName.append(PathUtil::GetFileName(strFilename).c_str());
                    if (starPos)
                    {
                        animName.append(starPos);
                    }

                    AddIfNewAnimationAlias(animList, animName.c_str(), rAIM.GetFilePath());
                }
            }
        }
        else if (parseSubfolders)
        {
            stack_string path(szFolder, 0, iPathLength);
            PathUtil::ToUnixPath(path);

            std::vector<string> cafFiles;
            SDirectoryEnumeratorHelper dirParser;
            dirParser.ScanDirectoryRecursive("", path, szFile, cafFiles);

            const uint32 numCAFs = cafFiles.size();
            for (uint32 i = 0; i < numCAFs; i++)
            {
                stack_string animName = szAnimName;
                animName.append(PathUtil::GetFileName(cafFiles[i].c_str()).c_str());
                if (starPos)
                {
                    animName.append(starPos);
                }

                if (IsFSQ)
                {
                    AddIfNewFacialAnimationAlias(animList, animName.c_str(), cafFiles[i].c_str());
                }
                else
                {
                    AddIfNewAnimationAlias(animList, animName.c_str(), cafFiles[i].c_str());
                }
            }
        }
        else
        {
            //extend the files from disk
            _finddata_t fd;
            intptr_t handle =  g_pIPak->FindFirst(szFolder, &fd, ICryPak::FLAGS_NO_LOWCASE);
            if (handle != -1)
            {
                do
                {
                    stack_string animName = szAnimName;
                    animName.append(PathUtil::GetFileName(fd.name).c_str());
                    if (starPos)
                    {
                        animName.append(starPos);
                    }

                    // Check whether the filename is a facial animation, by checking the extension.
                    if (IsFSQ)
                    {
                        // insert unique
                        AddIfNewFacialAnimationAlias(animList, animName.c_str(), filepath + "/" + fd.name);
                    }
                    else
                    {
                        stack_string fpath = filepath + stack_string("/") + fd.name;
                        AddIfNewAnimationAlias(animList, animName.c_str(), fpath);
                    }
                } while (g_pIPak->FindNext(handle, &fd) >= 0);

                g_pIPak->FindClose(handle);
            }
        }




        //-----------------------------------------------------------------



        if (Console::GetInst().ca_UseIMG_CAF == 0)
        {
            // insert all animations found in the DBA files
            for (uint32 j = 0; j < numDBALoaders; j++)
            {
                CGlobalHeaderDBA& rGlobalHeaderDBA = g_AnimationManager.m_arrGlobalHeaderDBA[j];
                if (rGlobalHeaderDBA.m_pDatabaseInfo == 0)
                {
                    const char* pName  = rGlobalHeaderDBA.m_strFilePathDBA;
                    uint32 nDBACRC32 = CCrc32::ComputeLowercase(pName);
                    uint32 numCAF = g_AnimationManager.m_arrGlobalCAF.size();
                    uint32 nFoundAssetsInAIF = 0;
                    for (uint32 c = 0; c < numCAF; c++)
                    {
                        if (g_AnimationManager.m_arrGlobalCAF[c].m_FilePathDBACRC32 == nDBACRC32)
                        {
                            nFoundAssetsInAIF++;

                            const char* currentFile = g_AnimationManager.m_arrGlobalCAF[c].GetFilePath();
                            const char* file = PathUtil::GetFile(currentFile);
                            const char* ext  = PathUtil::GetExt(currentFile);

                            bool match1 = _strnicmp(szFolder, currentFile, iPathLength) == 0;
                            bool match2 = PathUtil::MatchWildcard(file, szFile);
                            if (match1 && match2)
                            {
                                stack_string folderPathCurrentFile = PathUtil::GetParentDirectoryStackString(stack_string(currentFile));
                                stack_string folderPathFileName    = PathUtil::GetParentDirectoryStackString(stack_string(szFolder));

                                if (parseSubfolders || (!parseSubfolders && folderPathCurrentFile == folderPathFileName))
                                {
                                    stack_string name = "";
                                    if (starPos == NULL)
                                    {
                                        name = szAnimName;
                                    }
                                    else
                                    {
                                        uint32 folderPos = folderPathCurrentFile.length() + 1;
                                        stack_string sa =   stack_string(szAnimName) + stack_string(currentFile + folderPos, ext - currentFile - 1 - folderPos) + starPos;
                                        name = sa;
                                    }

                                    AddIfNewAnimationAlias(animList, name.c_str(), currentFile);
                                }
                            }
                        }
                    }

                    if (nFoundAssetsInAIF == 0)
                    {
                        rGlobalHeaderDBA.LoadDatabaseDBA(pDefaultSkeleton->GetModelFilePath());
                        if (rGlobalHeaderDBA.m_pDatabaseInfo == 0)
                        {
                            continue;
                        }
                    }
                }

                DynArray<string> arrDBAPathNames;
                if (rGlobalHeaderDBA.m_pDatabaseInfo)
                {
                    uint32 numCAF = g_AnimationManager.m_arrGlobalCAF.size();
                    for (uint32 c = 0; c < numCAF; c++)
                    {
                        if (g_AnimationManager.m_arrGlobalCAF[c].m_FilePathDBACRC32 == rGlobalHeaderDBA.m_FilePathDBACRC32)
                        {
                            arrDBAPathNames.push_back(g_AnimationManager.m_arrGlobalCAF[c].GetFilePath());
                        }
                    }
                }

                uint32 numCafFiles = arrDBAPathNames.size();
                for (uint32 f = 0; f < numCafFiles; f++)
                {
                    stack_string currentFile = arrDBAPathNames[f].c_str();
                    CryStringUtils::UnifyFilePath(currentFile);

                    const char* file = PathUtil::GetFile(currentFile.c_str());
                    const char* ext  = PathUtil::GetExt(currentFile.c_str());
                    if (_strnicmp(szFolder, currentFile, iPathLength) == 0 && PathUtil::MatchWildcard(file, szFile))
                    {
                        stack_string folderPathCurrentFile = PathUtil::GetParentDirectory(currentFile);
                        stack_string folderPathFileName = PathUtil::GetParentDirectory(szFolder);

                        if (parseSubfolders || (!parseSubfolders && folderPathCurrentFile == folderPathFileName))
                        {
                            stack_string name = "";
                            if (starPos == NULL)
                            {
                                name = szAnimName;
                            }
                            else
                            {
                                uint32 folderPos = folderPathCurrentFile.length() + 1;
                                stack_string sa =   stack_string(szAnimName) + stack_string(currentFile.c_str() + folderPos, ext - currentFile - 1 - folderPos) + starPos;
                                name = sa;
                            }

                            AddIfNewAnimationAlias(animList, name.c_str(), currentFile);
                        }
                    }
                }
            }
        }
    }

    //  return 0;
    animList.PrepareHeaderList();
    return true;
}

IChrParams* CParamLoader::ClearChrParams(const char* const paramFileName)
{
    uint chrParamsCount = m_parsedChrParams.size();
    for (uint i = 0; i < chrParamsCount; ++i)
    {
        if (_stricmp(paramFileName, m_parsedChrParams[i]->GetName()) == 0)
        {
            m_parsedChrParams[i]->ClearLists();
            return m_parsedChrParams[i];
        }
    }

    CChrParams* newChrParams = new CChrParams(paramFileName, this);
    m_parsedChrParams.push_back(newChrParams);
    return newChrParams;
}

IChrParams* CParamLoader::ClearChrParams(const char* const paramFileName, EChrParamNodeType type)
{
    uint chrParamsCount = m_parsedChrParams.size();
    for (uint i = 0; i < chrParamsCount; ++i)
    {
        if (_stricmp(paramFileName, m_parsedChrParams[i]->GetName()) == 0)
        {
            m_parsedChrParams[i]->ClearCategoryNode(type);
            return m_parsedChrParams[i];
        }
    }

    CChrParams* newChrParams = static_cast<CChrParams*>(GetEditableChrParams(paramFileName));
    newChrParams->ClearCategoryNode(type);
    return newChrParams;
}

void CParamLoader::DeleteParsedChrParams(const char* const paramFileName)
{
    for (auto it = m_parsedChrParams.begin(); it < m_parsedChrParams.end(); ++it)
    {
        CChrParams* params = (*it);
        if (_stricmp(paramFileName, params->GetName()) == 0)
        {
            m_parsedChrParams.erase(it);
            delete params;
        }
    }
}

bool CParamLoader::IsChrParamsParsed(const char* const paramFileName)
{
    uint chrParamsCount = m_parsedChrParams.size();
    for (uint i = 0; i < chrParamsCount; ++i)
    {
        if (_stricmp(paramFileName, m_parsedChrParams[i]->GetName()) == 0)
        {
            return true;
        }
    }

    return false;
}


bool CParamLoader::IsAnimationWithinFilters(const char* animationPath, uint32 listID)
{
    LOADING_TIME_PROFILE_SECTION(g_pISystem);

    string animationPathString = PathUtil::GetPath(animationPath);
    string animationPathExtention = PathUtil::GetExt(animationPath);

    CDefaultSkeleton* pDefaultSkeleton = m_pDefaultSkeleton;
    SAnimListInfo& animList = m_parsedLists[listID];

    uint32 numWildcard = animList.arrWildcardAnimFiles.size();
    for (uint32 w = 0; w < numWildcard; ++w)
    {
        const char* szFolder = animList.arrWildcardAnimFiles[w].m_FilePathQ;
        const char* szFile = PathUtil::GetFile(szFolder);
        const char* szExt = PathUtil::GetExt(szFolder);

        if (animationPathExtention == szExt)
        {
            char szAnimName[MAX_STRING_LENGTH];
            azstrcpy(szAnimName, MAX_STRING_LENGTH, animList.arrWildcardAnimFiles[w].m_AnimNameQ);

            const char* firstWildcard = strchr(szFolder, '*');
            const char* qm = strchr(szFolder, '?');
            if (qm)
            {
                if (firstWildcard == NULL || firstWildcard > qm)
                {
                    firstWildcard = qm;
                }
            }

            int32 iFirstWildcard = aznumeric_caster(firstWildcard - szFolder);
            int32 iPathLength = aznumeric_caster(szFile - szFolder);
            iPathLength = min(iPathLength, iFirstWildcard);

            bool parseSubfolders = (iFirstWildcard != (int32)(szFile - szFolder));

            stack_string path(szFolder, 0, iPathLength);
            PathUtil::ToUnixPath(path);
            string pathString = path;

            if (parseSubfolders)
            {
                if (pathString.compare(0, pathString.length(), animationPathString, pathString.length()) == 0)
                {
                    return true;
                }
            }
            else if (animationPathString == pathString)
            {
                return true;
            }
        }
    }

    return false;
}

IChrParams* CParamLoader::GetEditableChrParams(const char* const paramFileName)
{
    uint chrParamsCount = m_parsedChrParams.size();
    for (uint i = 0; i < chrParamsCount; ++i)
    {
        if (_stricmp(paramFileName, m_parsedChrParams[i]->GetName()) == 0)
        {
            return m_parsedChrParams[i];
        }
    }

    CChrParams* newChrParams = new CChrParams(paramFileName, this);
    m_parsedChrParams.push_back(newChrParams);
    return newChrParams;
}

// check if a list has already been parsed and is in memory, if not, parse it and load it into m_parsedLists
int32 CParamLoader::LoadAnimList(const CAnimationListNode* const animationNode, const char* const paramFileName, string& strAnimDirName)
{
    LOADING_TIME_PROFILE_SECTION(g_pISystem);

    if (!animationNode)
    {
        return -1;
    }

    // check if List has already been processed
    int32 nListIDs = m_parsedLists.size();

    int32 newID = ListProcessed(paramFileName);
    if (newID != -1)
    {
        return newID;
    }

    SAnimListInfo animList(paramFileName);
    const char* pFilePath = m_pDefaultSkeleton->GetModelFilePath();
    const char* pFileName = CryStringUtils::FindFileNameInPath(pFilePath);
    animList.arrAnimFiles.push_back(SAnimFile(stack_string(NULL_ANIM_FILE "/") + pFileName, "null"));

    const int BITE = 512;

    // [Alexey} - TODO! FIXME!!! FIXME!!!
    // We have 50 from 100 identical file parsing!!!
    //total++;

    //stl::hash_map<string, int, stl::hash_stricmp<string>>::iterator it = m_checkMap.find(string(calFileName));
    //if (it != m_checkMap.end())
    //{
    //  it->second = it->second + 1;
    //  int a = 0;
    //  identical++;
    //  Warning(calFileName, VALIDATOR_ERROR, "Identical cal readings %i from %i", identical, total);
    //} else
    //  m_checkMap[string(calFileName)] = 0;

    uint32 count = animationNode->GetAnimationCount();
    for (uint32 i = 0; i < count; ++i)
    {
        const SAnimNode* assignmentNode = animationNode->GetAnimation(i);

        CryFixedStringT<BITE> line;

        CryFixedStringT<BITE> key;
        CryFixedStringT<BITE> value;

        key.append(assignmentNode->m_name.c_str());
        line.append(assignmentNode->m_path.c_str());

        int32 pos = 0;
        value = line.Tokenize("\t\n\r=", pos).TrimRight(' ');

        // now only needed for aim poses
        char buffer[BITE] = "";
        azstrcpy(buffer, BITE, line.c_str());


        if (value.empty() || value.at(0) == '?')
        {
            continue;
        }

        if (assignmentNode->m_type == e_chrParamsAnimationNodeFilepath)
        {
            strAnimDirName = PathUtil::ToUnixPath(value.c_str());
            strAnimDirName.TrimRight('/'); // delete the trailing slashes
            continue;
        }

        if (assignmentNode->m_type == e_chrParamsAnimationNodeParseSubfolders)
        {
            Warning(paramFileName, VALIDATOR_WARNING, "Ignoring deprecated #ParseSubFolders directive");
            continue;
        }

        // remove first '\' and replace '\' with '/'
        value.replace('\\', '/');
        value.TrimLeft('/');


        // process the possible directives
        if (assignmentNode->m_type == e_chrParamsAnimationNodeAnimEventsDB)
        {
            if (animList.animEventDatabase.empty())
            {
                animList.animEventDatabase = value.c_str();
            }

            continue;
            //              else
            //                  Warning(calFileName, VALIDATOR_WARNING, "Failed to set animation event database \"%s\". Animation event database is already set to \"%s\"", value.c_str(), m_animEventDatabase.c_str());
        }
        else if (assignmentNode->m_type == e_chrParamsAnimationNodeWildcardDBA)
        {
            int wildcardPos = value.find('*');
            assert (wildcardPos >= 0);
            //--- Wildcard include

            stack_string path = value.Left(wildcardPos);
            PathUtil::ToUnixPath(path);
            stack_string filename = PathUtil::GetFile(value.c_str());

            std::vector<string> dbaFiles;
            SDirectoryEnumeratorHelper dirParser;
            dirParser.ScanDirectoryRecursive("", path, filename, dbaFiles);

            const uint32 numDBAs = dbaFiles.size();
            for (uint32 d = 0; d < numDBAs; d++)
            {
                if (!AddIfNewModelTracksDatabase(animList, dbaFiles[d]))
                {
                    Warning(paramFileName, VALIDATOR_WARNING, "Duplicate model tracks database declared \"%s\"", dbaFiles[d].c_str());
                }
            }

            continue;
        }
        else if (assignmentNode->m_type == e_chrParamsAnimationNodeSingleDBA)
        {
            if (!AddIfNewModelTracksDatabase(animList, value.c_str()))
            {
                Warning(paramFileName, VALIDATOR_WARNING, "Duplicate model tracks database declared \"%s\"", value.c_str());
            }

            CryFixedStringT<BITE> flags;
            flags.append(assignmentNode->m_flags.c_str());

            // flag handling
            if (!flags.empty())
            {
                if (strstr(flags.c_str(), "persistent"))
                {
                    animList.lockedDatabases.push_back(value.c_str());
                }
            }

            continue;
        }
        else if (assignmentNode->m_type == e_chrParamsAnimationNodeInclude)
        {
            int32 listID = ListProcessed(value.c_str());

            if (listID == -1)
            {
                const IChrParams* subChrParams = GetChrParams(value.c_str());

                if (subChrParams)
                {
                    const CAnimationListNode* subAnimListNode = static_cast<const CAnimationListNode*>(subChrParams->GetCategoryNode(e_chrParamsNodeAnimationList));
                    listID = LoadAnimList(subAnimListNode, subChrParams->GetName(), m_defaultAnimDir);
                }
                else
                {
                    Warning(value.c_str(), VALIDATOR_WARNING, "Warning: Could not load included chrparams file %s", value.c_str());
                }
            }

            // we found a new dependency, add it if it is not already in
            if (listID >= 0)
            {
                if (std::find(animList.dependencies.begin(), animList.dependencies.end(), listID) == animList.dependencies.end())
                {
                    animList.dependencies.push_back(listID);
                }
            }

            continue;
        }
        else if (assignmentNode->m_type == e_chrParamsAnimationNodeFaceLib)
        {
            if (animList.faceLibFile.empty())
            {
                animList.faceLibFile = value.c_str();
                animList.faceLibDir = strAnimDirName;
            }
            else
            {
                Warning(paramFileName, VALIDATOR_WARNING, "Failed to set face lib \"%s\". Face lib is already set to \"%s\"", value.c_str(), animList.faceLibFile.c_str());
            }

            continue;
        }

        // Check whether the filename is a facial animation, by checking the extension.
        const char* szExtension = PathUtil::GetExt(value);
        stack_string szFileName;
        szFileName.Format("%s/%s", strAnimDirName.c_str(), value.c_str());

        // is there any wildcard in the file name?
        if (assignmentNode->m_type == e_chrParamsAnimationNodeWildcardAsset)
        {
            animList.arrWildcardAnimFiles.push_back(SAnimFile(szFileName, key));
        }
        else if (assignmentNode->m_type == e_chrParamsAnimationNodeSingleAsset)
        {
            const char* failedToCreateAlias = "Failed to create animation alias \"%s\" for file \"%s\". Such alias already exists.\"";
            if (szExtension != 0 && _stricmp("fsq", szExtension) == 0)
            {
                bool added = AddIfNewFacialAnimationAlias(animList, key, szFileName.c_str());
                if (!added)
                {
                    Warning(paramFileName, VALIDATOR_WARNING, failedToCreateAlias, key.c_str(), szFileName.c_str());
                }
            }
            else
            {
                bool added = AddIfNewAnimationAlias(animList, key, szFileName.c_str());
                if (!added)
                {
                    Warning(paramFileName, VALIDATOR_WARNING, failedToCreateAlias, key.c_str(), szFileName.c_str());
                }
            }
        }
        else
        {
            Warning(paramFileName, VALIDATOR_WARNING, "Warning: Encountered unexpected node in animation block, name = %s, path = %s", assignmentNode->m_name.c_str(), assignmentNode->m_path.c_str());
        }

#if 0
        // LDS: This appears to be deprecated, removing for now pending consult with Cry
        //check if CAF or LMG has an Aim-Pose
        uint32 numAnims = animList.arrAnimFiles.size();
        if (szExtension != 0 && numAnims)
        {
            if ((_stricmp("lmg", szExtension) == 0) || (_stricmp("bspace", szExtension) == 0) || (_stricmp("comb", szExtension) == 0) || (_stricmp("caf", szExtension) == 0))
            {
                //PREFAST_SUPPRESS_WARNING(6031) strtok (buffer, " \t\n\r=");
                const char* fname = strtok(buffer, " \t\n\r=(");
                while ((fname = strtok(NULL, " \t\n\r,()")) != NULL && fname[0] != 0)
                {
                    if (fname[0] == '/')
                    {
                        break;
                    }
                }
            }
        }
#endif // #if 0
    }

    //end of loop
    m_parsedLists.push_back(animList);

    return m_parsedLists.size() - 1;
}

bool CParamLoader::AddIfNewAnimationAlias(SAnimListInfo& animList, const char* animName, const char* szFileName)
{
#if defined(_RELEASE)
    // We assume there are no duplicates in _RELEASE
    const bool addAnimationAlias = true;
#else
    const SAnimFile* pFoundAnimFile = FindAnimationAliasInDependencies(animList, animName);
    bool bDuplicate = (pFoundAnimFile != NULL);

    bool addAnimationAlias = true;
    if (bDuplicate)
    {
        // When using local files the duplicate check is used to prevent the file
        // from the PAK to be added too - so do not warn and do not add duplicates
        // when using local files and the filename is exactly the same
        const bool allowLocalFiles = (Console::GetInst().ca_UseIMG_CAF == 0);
        const bool sameFileName = (_stricmp(pFoundAnimFile->m_FilePathQ, szFileName) == 0);
        const bool isValidDuplicate = (allowLocalFiles && sameFileName);
        if (isValidDuplicate)
        {
            addAnimationAlias = false;
        }
        else
        {
            Warning(animList.fileName.c_str(), VALIDATOR_WARNING, "Duplicate animation reference not supported: %s:%s", szFileName, pFoundAnimFile->m_FilePathQ);
#if 0
            IPlatformOS::EMsgBoxResult result;
            IPlatformOS* pOS = gEnv->pSystem->GetPlatformOS();
            if (pOS)
            {
                char errorBuf[512];
                sprintf_s(errorBuf, 512, "Duplicate animation reference not supported: %s %s:%s", animList.fileName.c_str(), szFileName, pFoundAnimFile->m_FilePathQ);
                result = pOS->DebugMessageBox(errorBuf, "Animation");

                if (result == IPlatformOS::eMsgBox_Cancel)
                {
                    __debugbreak();
                }
            }
#endif
        }
    }
#endif

    if (addAnimationAlias)
    {
        animList.arrAnimFiles.push_back(SAnimFile(szFileName, animName));
    }

    return true;
}

const SAnimFile* CParamLoader::FindAnimationAliasInDependencies(SAnimListInfo& animList, const char* animName)
{
#if !defined(RELEASE)
    FindByAliasName pd;
    pd.fileCrc =  CCrc32::ComputeLowercase(animName);
    DynArray<SAnimFile>::const_iterator found_it = std::find_if(animList.arrAnimFiles.begin(), animList.arrAnimFiles.end(), pd);
    if (found_it != animList.arrAnimFiles.end())
    {
        return &(*found_it);
    }

    uint32 numDeps = animList.dependencies.size();
    for (uint32 i = 0; i < numDeps; i++)
    {
        SAnimListInfo& childList = m_parsedLists[animList.dependencies[i]];

        const SAnimFile* pFoundAnimFile = FindAnimationAliasInDependencies(childList, animName);
        if (pFoundAnimFile)
        {
            return pFoundAnimFile;
        }
    }
#endif
    return NULL;
}


bool CParamLoader::AddIfNewFacialAnimationAlias(SAnimListInfo& animList,  const char* animName, const char* szFileName)
{
    if (NoFacialAnimationAliasInDependencies(animList, animName))
    {
        animList.facialAnimations.push_back(CAnimationSet::FacialAnimationEntry(animName, szFileName));
        return true;
    }

    return false;
};

bool CParamLoader::NoModelTracksDatabaseInDependencies(SAnimListInfo& animList, const char* dataBase)
{
    DynArray<string>& mTB = animList.modelTracksDatabases;

    uint32 numDBA = mTB.size();
    if (std::find(mTB.begin(), mTB.end(), dataBase) != mTB.end())
    {
        return false;
    }

    uint32 numDeps = animList.dependencies.size();
    for (uint32 i = 0; i < numDeps; i++)
    {
        SAnimListInfo& childList = m_parsedLists[animList.dependencies[i]];
        if (!NoModelTracksDatabaseInDependencies(childList, dataBase))
        {
            return false;
        }
    }

    return true;
}

bool CParamLoader::AddIfNewModelTracksDatabase(SAnimListInfo& animList, const char* dataBase)
{
    stack_string tmp = dataBase;
    CryStringUtils::UnifyFilePath(tmp);
    if (NoModelTracksDatabaseInDependencies(animList, tmp.c_str()))
    {
        animList.modelTracksDatabases.push_back(tmp);
        return true;
    }
    else
    {
        return false;
    }
};

bool CParamLoader::NoFacialAnimationAliasInDependencies(SAnimListInfo& animList, const char* animName)
{
    for (uint32 i = 0; i < animList.facialAnimations.size(); ++i)
    {
        if (0 == _stricmp(animList.facialAnimations[i], animName))
        {
            return false;
        }
    }

    uint32 numDeps = animList.dependencies.size();
    for (uint32 i = 0; i < numDeps; i++)
    {
        SAnimListInfo& childList = m_parsedLists[animList.dependencies[i]];
        if (!NoFacialAnimationAliasInDependencies(childList, animName))
        {
            return false;
        }
    }
    return true;
}

const IChrParams* CParamLoader::GetChrParams(const char* const paramFileName, const std::string* optionalBuffer)
{
    if (!optionalBuffer)
    {
        uint chrParamsCount = m_parsedChrParams.size();
        for (int i = 0; i < chrParamsCount; i++)
        {
            if (_stricmp(m_parsedChrParams[i]->GetName(), paramFileName) == 0)
            {
                return m_parsedChrParams[i];
            }
        }
    }

    // Not parsed yet
    return ParseXML(paramFileName, optionalBuffer);
}


bool CParamLoader::LoadXML(CDefaultSkeleton* pDefaultSkeleton, string defaultAnimDir, const char* const paramFileName, DynArray<uint32>& listIDs, const std::string* optionalBuffer)
{
    LOADING_TIME_PROFILE_SECTION(g_pISystem);

    const CChrParams* rootChrParams = static_cast<const CChrParams*>(GetChrParams(paramFileName, optionalBuffer));

    if (!rootChrParams)
    {
        return false;
    }

    m_pDefaultSkeleton = pDefaultSkeleton;
    m_defaultAnimDir = defaultAnimDir;

    //in case we reload the CHRPARAMS again, we have to make sure we do a full initializations
    pDefaultSkeleton->m_IKLimbTypes.clear();
    pDefaultSkeleton->m_ADIKTargets.clear();

    pDefaultSkeleton->m_poseBlenderLookDesc.m_error = -1;  //not initialized by default
    pDefaultSkeleton->m_poseBlenderLookDesc.m_blends.clear();
    pDefaultSkeleton->m_poseBlenderLookDesc.m_rotations.clear();
    pDefaultSkeleton->m_poseBlenderLookDesc.m_positions.clear();

    pDefaultSkeleton->m_poseBlenderAimDesc.m_error = -1;   //not initialized by default
    pDefaultSkeleton->m_poseBlenderAimDesc.m_blends.clear();
    pDefaultSkeleton->m_poseBlenderAimDesc.m_rotations.clear();
    pDefaultSkeleton->m_poseBlenderAimDesc.m_positions.clear();
    pDefaultSkeleton->m_poseBlenderAimDesc.m_procAdjustments.clear();

    if (LoadRuntimeData(rootChrParams, listIDs))
    {
        uint animListCount = m_parsedLists.size();
        for (uint i = 0; i < animListCount; ++i)
        {
            if (_stricmp(paramFileName, m_parsedLists[i].fileName) == 0)
            {
                BuildDependencyList(i, listIDs);
                return true;
            }
        }
    }

    return false;
}

bool CParamLoader::WriteXML(const char* const paramFileName)
{
    // TODO: Write code to prevent circular dependencies
    uint chrParamsCount = m_parsedChrParams.size();
    for (int i = 0; i < chrParamsCount; i++)
    {
        if (_stricmp(paramFileName, m_parsedChrParams[i]->GetName()) == 0)
        {
            m_parsedChrParams[i]->SerializeToFile();
            return true;
        }
    }

    return false;
}

IChrParams* CParamLoader::ParseXML(const char* const paramFileName, const std::string* optionalBuffer)
{
    XmlNodeRef topRoot;
    
    if (optionalBuffer)
    {
        topRoot = g_pISystem->LoadXmlFromBuffer(optionalBuffer->c_str(), optionalBuffer->length());
    }
    else
    {
        topRoot = g_pISystem->LoadXmlFromFile(paramFileName);
    }

    CChrParams* newChrParams = new CChrParams(paramFileName, this);

    if (!topRoot)
    {
        return newChrParams;
    }

    const char* nodeTag = topRoot->getTag();
    if (_stricmp(nodeTag, "Params"))
    {
        return NULL;
    }

    uint32 numChildren = topRoot->getChildCount();
    for (uint32 i = 0; i < numChildren; ++i)
    {
        XmlNodeRef node = topRoot->getChild(i);
        EChrParamNodeType nodeType = e_chrParamsNodeInvalid;
        const char* nodeTag = node->getTag();

        for (int type = e_chrParamsNodeFirst; type < e_chrParamsNodeInvalid; ++type)
        {
            if (_stricmp(nodeTag, g_nodeTypeCategoryNames[type]) == 0)
            {
                nodeType = (EChrParamNodeType)type;
                break;
            }
        }

        if (nodeType == e_chrParamsNodeInvalid || !newChrParams->AddNode(node, nodeType))
        {
            delete newChrParams;
            return NULL;
        }
    }

   DeleteParsedChrParams(paramFileName);
    m_parsedChrParams.push_back(newChrParams);
    return newChrParams;
}

bool CParamLoader::LoadCategoryType(const CChrParams* const chrParams, EChrParamNodeType type)
{
    const IChrParamsNode* const categoryNode = chrParams->GetCategoryNode(type);

    if (!categoryNode)
    {
        return true;
    }

    // TODO: Not clear if this is used by LoadAnimList or deprecated. Need to investigate and ideally
    //       remove
    bool bParseSubFolders = false;

    switch (type)
    {
    case e_chrParamsNodeBoneLod:
        return LoadBoneLod(static_cast<const CBoneLodNode* const>(categoryNode));
#if defined(ENABLE_BBOX_EXCLUDE_LIST)
    case e_chrParamsNodeBBoxExcludeList:
        return LoadBBoxExclusionList(static_cast<const CBBoxExcludeNode* const>(categoryNode));
#endif // defined(ENABLE_BBOX_EXCLUDE_LIST)
    case e_chrParamsNodeBBoxIncludeList:
        return LoadBBoxInclusionList(static_cast<const CBBoxIncludeListNode* const>(categoryNode));
    case e_chrParamsNodeBBoxExtensionList:
        return LoadBBoxExtension(static_cast<const CBBoxExtensionNode* const>(categoryNode));
    case e_chrParamsNodeUsePhysProxyBBox:
        return LoadUsePhysProxyBBox(static_cast<const CPhysProxyBBoxOptionNode* const>(categoryNode));
    case e_chrParamsNodeIKDefinition:
        return LoadIKDefs(static_cast<const CIKDefinitionNode* const>(categoryNode));
    case e_chrParamsNodeAnimationList:
        return (LoadAnimList(static_cast<const CAnimationListNode* const>(categoryNode), chrParams->GetName(), m_defaultAnimDir) != -1);
    default:
        Warning(chrParams->GetName(), VALIDATOR_WARNING, "Warning: Unknown chrparams block requested for loading");
        return false;
    }
    ;
}

bool CParamLoader::LoadRuntimeData(const CChrParams* rootChrParams, DynArray<uint32>& listIDs)
{
    if (!rootChrParams)
    {
        return false;
    }

    for (int type = e_chrParamsNodeFirst; type < e_chrParamsNodeCount; ++type)
    {
        bool success = LoadCategoryType(rootChrParams, (EChrParamNodeType)type);
        if (!success)
        {
            Warning(rootChrParams->GetName(), VALIDATOR_WARNING, "Warning: Error when trying to load chrparams block %s\n", g_nodeTypeCategoryNames[type].c_str());
        }
    }

    return true;
}

bool CParamLoader::LoadBoneLod(const CBoneLodNode* const boneLodNode)
{
    if (!boneLodNode)
    {
        return false;
    }

    uint lodCount = boneLodNode->GetLodCount();
    for (uint i = 0; i < lodCount; ++i)
    {
        DynArray<uint32> jointMask;
        uint jointCount = boneLodNode->GetJointCount(i);
        for (uint j = 0; j < jointCount; ++j)
        {
            uint32 nameCrc32 = CCrc32::Compute(boneLodNode->GetJointName(i, j));
            jointMask.push_back(nameCrc32);
        }

        if (!jointMask.empty())
        {
            std::sort(jointMask.begin(), jointMask.end());
            m_pDefaultSkeleton->m_arrAnimationLOD.push_back()->swap(jointMask);
        }
    }

    return true;
}

#if defined(ENABLE_BBOX_EXCLUDE_LIST)
bool CParamLoader::LoadBBoxExclusionList(const CBBoxExcludeNode* const bboxExcludeNode)
{
    if (!m_pDefaultSkeleton)
    {
        return false;
    }
    if (!bboxExcludeNode)
    {
        return false;
    }
    CDefaultSkeleton& modelSkeleton = *m_pDefaultSkeleton;
    uint count = bboxExcludeNode->GetJointCount();
    for (uint i = 0; i < count; ++i)
    {
        const char* name = bboxExcludeNode->GetJointName(i).c_str();
        if (!name)
        {
            continue;
        }

        int jointIndex = modelSkeleton.GetJointIDByName(name);
        if (jointIndex < 0)
        {
            continue;
        }

        modelSkeleton.m_arrModelJoints[jointIndex].m_flags |= eJointFlag_ExcludeFromBBox;
    }
    return true;
}
#endif // defined(ENABLE_BBOX_EXCLUDE_LIST)

bool CParamLoader::LoadBBoxInclusionList(const CBBoxIncludeListNode* const bboxIncludeNode)
{
    if (m_pDefaultSkeleton == 0)
    {
        return false;
    }
    if (!bboxIncludeNode)
    {
        return false;
    }

    uint jointCount = bboxIncludeNode->GetJointCount();
    CDefaultSkeleton& rDefaultSkeleton = *m_pDefaultSkeleton;
    rDefaultSkeleton.m_BBoxIncludeList.clear();
    rDefaultSkeleton.m_BBoxIncludeList.reserve(jointCount);
    for (uint i = 0; i < jointCount; ++i)
    {
        const char* jointName = bboxIncludeNode->GetJointName(i).c_str();
        int jointIndex = rDefaultSkeleton.GetJointIDByName(jointName);

        if (jointIndex < 0)
        {
            g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, m_pDefaultSkeleton->GetModelFilePath(), "BBoxInclusionList Error: JointName '%s' for Inclusion not found in model", jointName);
            continue;
        }
        rDefaultSkeleton.m_BBoxIncludeList.push_back(jointIndex);
    }

    return true;
}

bool CParamLoader::LoadBBoxExtension(const CBBoxExtensionNode* const bboxExtensionNode)
{
    if (m_pDefaultSkeleton == 0)
    {
        return false;
    }
    if (!bboxExtensionNode)
    {
        return false;
    }

    CDefaultSkeleton& rDefaultSkeleton = *m_pDefaultSkeleton;

    Vec3 bboxMin;
    Vec3 bboxMax;
    bboxExtensionNode->GetBBoxMaxMin(bboxMin, bboxMax);
    rDefaultSkeleton.m_AABBExtension.min.x = fabsf(bboxMin.x);
    rDefaultSkeleton.m_AABBExtension.min.y = fabsf(bboxMin.y);
    rDefaultSkeleton.m_AABBExtension.min.z = fabsf(bboxMin.z);
    rDefaultSkeleton.m_AABBExtension.max.x = fabsf(bboxMax.x);
    rDefaultSkeleton.m_AABBExtension.max.y = fabsf(bboxMax.y);
    rDefaultSkeleton.m_AABBExtension.max.z = fabsf(bboxMax.z);

    return true;
}

bool CParamLoader::LoadUsePhysProxyBBox(const CPhysProxyBBoxOptionNode* const usePhysProxyBBoxNode)
{
    this->m_pDefaultSkeleton->m_usePhysProxyBBox = usePhysProxyBBoxNode->GetUsePhysProxyBBoxNode() ? 1 : 0;
    return true;
}

bool CParamLoader::LoadIKDefs(const CIKDefinitionNode* const ikDefinitionNode)
{
    for (int type = e_chrParamsIKNodeSubTypeFirst; type < e_chrParamsIKNodeSubTypeCount; ++type)
    {
        if (!ikDefinitionNode->HasIKSubNodeType((EChrParamsIKNodeSubType)type))
        {
            continue;
        }

        const IChrParamsNode* ikSubTypeNode = ikDefinitionNode->GetIKSubNode((EChrParamsIKNodeSubType)type);

        switch (type)
        {
        case e_chrParamsIKNodeLimb:
            LoadIKDefLimbIK(static_cast<const CLimbIKDefs*>(ikSubTypeNode));
            break;
        case e_chrParamsIKNodeAnimDriven:
            LoadIKDefAnimDrivenIKTargets(static_cast<const CAnimDrivenIKDefs*>(ikSubTypeNode));
            break;
        case e_chrParamsIKNodeFootLock:
            LoadIKDefFeetLock(static_cast<const CFootLockIKDefs*>(ikSubTypeNode));
            break;
        case e_chrParamsIKNodeRecoil:
            LoadIKDefRecoil(static_cast<const CRecoilIKDef*>(ikSubTypeNode));
            break;
        case e_chrParamsIKNodeLook:
            LoadIKDefLookIK(static_cast<const CLookIKDef*>(ikSubTypeNode));
            break;
        case e_chrParamsIKNodeAim:
            LoadIKDefAimIK(static_cast<const CAimIKDef*>(ikSubTypeNode));
            break;
        default:
            return false;
        }
    }

    return true;
}

void CParamLoader::ClearLists()
{
    int32 n = m_parsedLists.size();
    for (int32 i = 0; i < n; i++)
    {
        stl::free_container(m_parsedLists[i].arrWildcardAnimFiles);
        stl::free_container(m_parsedLists[i].arrAnimFiles);
        stl::free_container(m_parsedLists[i].modelTracksDatabases);
        stl::free_container(m_parsedLists[i].facialAnimations);
    }
    stl::free_container(m_parsedLists);

    n = m_parsedChrParams.size();
    for (int32 i = 0; i < n; i++)
    {
        delete m_parsedChrParams[i];
    }
    stl::free_container(m_parsedChrParams);
}

PREFAST_SUPPRESS_WARNING(6262)
bool CParamLoader::LoadIKDefLimbIK(const CLimbIKDefs* limbIKDefs)
{
    uint limbIKCount = limbIKDefs->GetLimbIKDefCount();
    CDefaultSkeleton* pDefaultSkeleton = m_pDefaultSkeleton;

    for (uint i = 0; i < limbIKCount; ++i)
    {
        const SLimbIKDef* limbIKDef = limbIKDefs->GetLimbIKDef(i);
        const char* pHandle = limbIKDef->m_handle;
        const char* pSolver = limbIKDef->m_solver;
        const char* pRoot = limbIKDef->m_rootJoint;
        const char* pEndEffector = limbIKDef->m_endEffector;

        IKLimbType IKLimb;
        if (pHandle == 0)
        {
            continue;
        }
        IKLimb.m_nHandle = CCrc32::ComputeLowercase(pHandle);

        IKLimb.m_nSolver = *(uint32*)pSolver;

        //-----------------------------------------------------------------------------------
        if (IKLimb.m_nSolver == *(uint32*)"CCD5")  //check the Solver
        {
            IKLimb.m_arrJointChain.resize(5);
            if (limbIKDef->m_ccdFiveJoints.size() != 5)
            {
                return false;
            }

            for (uint i = 0; i < 5; ++i)
            {
                int32 jointIdx = pDefaultSkeleton->GetJointIDByName(limbIKDef->m_ccdFiveJoints[i]);
                ANIM_ASSET_CHECK_TRACE(jointIdx > 0, ("For J%i %s", i, limbIKDef->m_ccdFiveJoints[i].c_str()));
                if (jointIdx < 0)
                {
                    return false;
                }
                IKLimb.m_arrJointChain[i].m_idxJoint = jointIdx;
                IKLimb.m_arrJointChain[i].m_strJoint = pDefaultSkeleton->GetJointNameByID(jointIdx);
            }

            pDefaultSkeleton->m_IKLimbTypes.push_back(IKLimb);
        }
        else
        {
            int32 nRootIdx = pDefaultSkeleton->GetJointIDByName(pRoot);
            if (nRootIdx < 0)
            {
                return false;
            }

            int32 nEndEffectorIdx = pDefaultSkeleton->GetJointIDByName(pEndEffector);
            if (nEndEffectorIdx < 0)
            {
                return false;
            }

            assert(nRootIdx < nEndEffectorIdx);
            if (nRootIdx >= nEndEffectorIdx)
            {
                return false;
            }

            int32 nBaseRootIdx = pDefaultSkeleton->GetJointParentIDByID(nRootIdx);
            if (nBaseRootIdx < 0)
            {
                return false;
            }

            if (IKLimb.m_nSolver == *(uint32*)"2BIK")  //check the Solver
            {
                int32 nMiddleIdx = pDefaultSkeleton->GetJointParentIDByID(nEndEffectorIdx);
                if (nMiddleIdx < 0)
                {
                    return false;
                }
                //do a simple verification
                int32 idx  = pDefaultSkeleton->GetJointParentIDByID(nMiddleIdx);
                const char* strRoot  = pDefaultSkeleton->GetJointNameByID(idx);
                if (idx != nRootIdx)
                {
                    return false;
                }

                IKLimb.m_arrJointChain.resize(4);
                IKLimb.m_arrJointChain[0].m_idxJoint = nBaseRootIdx;
                IKLimb.m_arrJointChain[0].m_strJoint = pDefaultSkeleton->GetJointNameByID(nBaseRootIdx);
                IKLimb.m_arrJointChain[1].m_idxJoint = nRootIdx;
                IKLimb.m_arrJointChain[1].m_strJoint = pDefaultSkeleton->GetJointNameByID(nRootIdx);
                IKLimb.m_arrJointChain[2].m_idxJoint = nMiddleIdx;
                IKLimb.m_arrJointChain[2].m_strJoint = pDefaultSkeleton->GetJointNameByID(nMiddleIdx);
                IKLimb.m_arrJointChain[3].m_idxJoint = nEndEffectorIdx;
                IKLimb.m_arrJointChain[3].m_strJoint = pDefaultSkeleton->GetJointNameByID(nEndEffectorIdx);

                uint32 numJoints = pDefaultSkeleton->GetJointCount();
                int16 arrIndices[MAX_JOINT_AMOUNT];
                int32 icounter = 0;
                int32 start = nRootIdx;
                for (uint32 i = start; i < numJoints; i++)
                {
                    int32 c = i;
                    while (c > 0)
                    {
                        if (nRootIdx == c)
                        {
                            arrIndices[icounter++] = i;
                            break;
                        }
                        else
                        {
                            c = pDefaultSkeleton->m_arrModelJoints[c].m_idxParent;
                        }
                    }
                }
                if (icounter)
                {
                    IKLimb.m_arrLimbChildren.resize(icounter);
                    for (int32 i = 0; i < icounter; i++)
                    {
                        IKLimb.m_arrLimbChildren[i] = arrIndices[i];
                    }
                }


                int32 arrEndEff2Root[MAX_JOINT_AMOUNT];
                arrEndEff2Root[0] = nEndEffectorIdx;

                int32 error = -1;
                uint32 links;
                for (links = 0; links < (MAX_JOINT_AMOUNT - 1); links++)
                {
                    arrEndEff2Root[links + 1] = pDefaultSkeleton->GetJointParentIDByID(arrEndEff2Root[links]);
                    error = arrEndEff2Root[links + 1];
                    if (error < 0)
                    {
                        break;
                    }
                    if (arrEndEff2Root[links + 1] == 0)
                    {
                        break;
                    }
                }
                if (error < 0)
                {
                    return false;
                }
                links += 2;
                IKLimb.m_arrRootToEndEffector.resize(links);
                int32 s = links - 1;
                for (uint32 j = 0; j < links; j++)
                {
                    IKLimb.m_arrRootToEndEffector[s--] = arrEndEff2Root[j];
                }

                pDefaultSkeleton->m_IKLimbTypes.push_back(IKLimb);
            } //if "2BIK"

            //------------------------------------------------------------------------------------

            else if (IKLimb.m_nSolver == *(uint32*)"3BIK")  //check the Solver
            {
                int32 nLeg03Idx = pDefaultSkeleton->GetJointParentIDByID(nEndEffectorIdx);
                if (nLeg03Idx < 0)
                {
                    return false;
                }
                int32 nLeg02Idx = pDefaultSkeleton->GetJointParentIDByID(nLeg03Idx);
                if (nLeg02Idx < 0)
                {
                    return false;
                }
                //do a simple verification
                int32 idx  = pDefaultSkeleton->GetJointParentIDByID(nLeg02Idx);
                const char* strRoot  = pDefaultSkeleton->GetJointNameByID(idx);
                if (idx != nRootIdx)
                {
                    return false;
                }


                IKLimb.m_arrJointChain.resize(5);
                IKLimb.m_arrJointChain[0].m_idxJoint = nBaseRootIdx;
                IKLimb.m_arrJointChain[0].m_strJoint = pDefaultSkeleton->GetJointNameByID(nBaseRootIdx);

                IKLimb.m_arrJointChain[1].m_idxJoint = nRootIdx;
                IKLimb.m_arrJointChain[1].m_strJoint = pDefaultSkeleton->GetJointNameByID(nRootIdx);

                IKLimb.m_arrJointChain[2].m_idxJoint = nLeg02Idx;
                IKLimb.m_arrJointChain[2].m_strJoint = pDefaultSkeleton->GetJointNameByID(nLeg02Idx);

                IKLimb.m_arrJointChain[3].m_idxJoint = nLeg03Idx;
                IKLimb.m_arrJointChain[3].m_strJoint = pDefaultSkeleton->GetJointNameByID(nLeg03Idx);

                IKLimb.m_arrJointChain[4].m_idxJoint = nEndEffectorIdx;
                IKLimb.m_arrJointChain[4].m_strJoint = pDefaultSkeleton->GetJointNameByID(nEndEffectorIdx);

                uint32 numJoints = pDefaultSkeleton->GetJointCount();
                int16 arrIndices[MAX_JOINT_AMOUNT];
                int32 icounter = 0;
                int32 start = nRootIdx;
                for (uint32 i = start; i < numJoints; i++)
                {
                    int32 c = i;
                    while (c > 0)
                    {
                        if (nRootIdx == c)
                        {
                            arrIndices[icounter++] = i;
                            break;
                        }
                        else
                        {
                            c = pDefaultSkeleton->m_arrModelJoints[c].m_idxParent;
                        }
                    }
                }
                if (icounter)
                {
                    IKLimb.m_arrLimbChildren.resize(icounter);
                    for (int32 i = 0; i < icounter; i++)
                    {
                        IKLimb.m_arrLimbChildren[i] = arrIndices[i];
                    }
                }


                int32 arrEndEff2Root[MAX_JOINT_AMOUNT];
                arrEndEff2Root[0] = nEndEffectorIdx;

                int32 error = -1;
                uint32 links;
                for (links = 0; links < (MAX_JOINT_AMOUNT - 1); links++)
                {
                    arrEndEff2Root[links + 1] = pDefaultSkeleton->GetJointParentIDByID(arrEndEff2Root[links]);
                    error = arrEndEff2Root[links + 1];
                    if (error < 0)
                    {
                        break;
                    }
                    if (arrEndEff2Root[links + 1] == 0)
                    {
                        break;
                    }
                }
                if (error < 0)
                {
                    return false;
                }
                links += 2;
                IKLimb.m_arrRootToEndEffector.resize(links);
                int32 s = links - 1;
                for (uint32 j = 0; j < links; j++)
                {
                    IKLimb.m_arrRootToEndEffector[s--] = arrEndEff2Root[j];
                }

                pDefaultSkeleton->m_IKLimbTypes.push_back(IKLimb);
            } //if "3BIK"


            //--------------------------------------------------------------------------

            else if (IKLimb.m_nSolver == *(uint32*)"CCDX")  //check the Solver
            {
                IKLimb.m_fStepSize = limbIKDef->m_stepSize;
                IKLimb.m_fThreshold = limbIKDef->m_threshold;
                IKLimb.m_nInterations = limbIKDef->m_maxIterations;

                int32 arrCCDChainIdx[MAX_JOINT_AMOUNT];
                const char* arrCCDChainStr[MAX_JOINT_AMOUNT];

                arrCCDChainIdx[0] = nEndEffectorIdx;
                arrCCDChainStr[0] = pDefaultSkeleton->GetJointNameByID(nEndEffectorIdx);

                int32 error = -1;
                uint32 links;
                for (links = 0; links < (MAX_JOINT_AMOUNT - 1); links++)
                {
                    arrCCDChainIdx[links + 1] = pDefaultSkeleton->GetJointParentIDByID(arrCCDChainIdx[links]);
                    error = arrCCDChainIdx[links + 1];
                    if (error < 0)
                    {
                        break;
                    }
                    arrCCDChainStr[links + 1] = pDefaultSkeleton->GetJointNameByID(arrCCDChainIdx[links + 1]);
                    if (arrCCDChainIdx[links + 1] == nRootIdx)
                    {
                        break;
                    }
                }
                if (error < 0)
                {
                    return false;
                }

                links++;
                arrCCDChainIdx[links + 1] = nBaseRootIdx;
                arrCCDChainStr[links + 1] = pDefaultSkeleton->GetJointNameByID(arrCCDChainIdx[links + 1]);

                links += 2;
                IKLimb.m_arrJointChain.resize(links);
                int32 s = links - 1;
                for (uint32 j = 0; j < links; j++)
                {
                    IKLimb.m_arrJointChain[s].m_idxJoint = arrCCDChainIdx[j];
                    IKLimb.m_arrJointChain[s].m_strJoint = arrCCDChainStr[j];
                    s--;
                }


                uint32 numJoints = pDefaultSkeleton->GetJointCount();
                int16 arrIndices[MAX_JOINT_AMOUNT];
                int32 icounter = 0;
                for (uint32 i = nRootIdx; i < numJoints; i++)
                {
                    int32 c = i;
                    while (c > 0)
                    {
                        if (nRootIdx == c)
                        {
                            arrIndices[icounter++] = i;
                            break;
                        }
                        else
                        {
                            c = pDefaultSkeleton->m_arrModelJoints[c].m_idxParent;
                        }
                    }
                }
                if (icounter)
                {
                    IKLimb.m_arrLimbChildren.resize(icounter);
                    for (int32 i = 0; i < icounter; i++)
                    {
                        IKLimb.m_arrLimbChildren[i] = arrIndices[i];
                    }
                }


                int32 arrEndEff2Root[MAX_JOINT_AMOUNT];
                arrEndEff2Root[0] = nEndEffectorIdx;
                error = -1;
                for (links = 0; links < (MAX_JOINT_AMOUNT - 1); links++)
                {
                    arrEndEff2Root[links + 1] = pDefaultSkeleton->GetJointParentIDByID(arrEndEff2Root[links]);
                    error = arrEndEff2Root[links + 1];
                    if (error < 0)
                    {
                        break;
                    }
                    if (arrEndEff2Root[links + 1] == 0)
                    {
                        break;
                    }
                }
                if (error < 0)
                {
                    return false;
                }
                links += 2;
                IKLimb.m_arrRootToEndEffector.resize(links);
                s = links - 1;
                for (uint32 j = 0; j < links; j++)
                {
                    IKLimb.m_arrRootToEndEffector[s--] = arrEndEff2Root[j];
                }

                pDefaultSkeleton->m_IKLimbTypes.push_back(IKLimb);
            } //if "CCDX"
        }
    }


    //--------------------------------------------------------------------------

    return true;
}

bool CParamLoader::LoadIKDefAnimDrivenIKTargets(const CAnimDrivenIKDefs* animDrivenIKDefs)
{
    uint animDrivenIKCount = animDrivenIKDefs->GetAnimDrivenIKDefCount();
    CDefaultSkeleton* pDefaultSkeleton = m_pDefaultSkeleton;

    for (uint i = 0; i < animDrivenIKCount; ++i)
    {
        const SAnimDrivenIKDef* animDrivenIKDef = animDrivenIKDefs->GetAnimDrivenIKDef(i);
        ADIKTarget IKTarget;

        const char* pHandle = animDrivenIKDef->m_handle.c_str();
        ;
        const char* pTarget = animDrivenIKDef->m_targetJoint.c_str();
        const char* pWeight = animDrivenIKDef->m_weightJoint.c_str();

        if (!pHandle)
        {
            continue;
        }

        IKTarget.m_nHandle = CCrc32::ComputeLowercase(pHandle);

        int32 nTargetIdx = pDefaultSkeleton->GetJointIDByName(pTarget);
        ANIM_ASSET_CHECK_TRACE(nTargetIdx > 0, ("Failure for target %s", pTarget));
        if (nTargetIdx < 0)
        {
            return false;
        }
        IKTarget.m_idxTarget = nTargetIdx;
        IKTarget.m_strTarget = pDefaultSkeleton->GetJointNameByID(nTargetIdx);

        int32 nWeightIdx = pDefaultSkeleton->GetJointIDByName(pWeight);
        ANIM_ASSET_CHECK_TRACE(nWeightIdx > 0, ("Failure for weight %s", pWeight));
        if (nWeightIdx < 0)
        {
            return false;
        }
        IKTarget.m_idxWeight = nWeightIdx;
        IKTarget.m_strWeight = pDefaultSkeleton->GetJointNameByID(nWeightIdx);

        pDefaultSkeleton->m_ADIKTargets.push_back(IKTarget);
    }

    return true;
}

bool CParamLoader::LoadIKDefFeetLock(const CFootLockIKDefs* footLockIKDefs)
{
    uint footLockIKCount = footLockIKDefs->GetFootLockIKDefCount();
    CDefaultSkeleton* pDefaultSkeleton = m_pDefaultSkeleton;
    assert(footLockIKCount <= MAX_FEET_AMOUNT);

    for (uint i = 0; i < footLockIKCount; ++i)
    {
        string footLockIKHandle = footLockIKDefs->GetFootLockIKDef(i);
        pDefaultSkeleton->m_strFeetLockIKHandle[i] = footLockIKHandle.c_str();
    }

    return true;
}



bool CParamLoader::LoadIKDefRecoil(const CRecoilIKDef* recoilIKDef)
{
    CDefaultSkeleton* pDefaultSkeleton = m_pDefaultSkeleton;

    const char* pJointName = recoilIKDef->GetRightIKHandle().c_str();
    pDefaultSkeleton->m_recoilDesc.m_ikHandleRight = pJointName;

    pJointName = recoilIKDef->GetLeftIKHandle().c_str();
    pDefaultSkeleton->m_recoilDesc.m_ikHandleLeft = pJointName;

    pJointName = recoilIKDef->GetRightWeaponJoint().c_str();
    int32 jidx = pDefaultSkeleton->GetJointIDByName(pJointName);
    if (jidx > 0)
    {
        pDefaultSkeleton->m_recoilDesc.m_weaponRightJointIndex = jidx;
    }

    pJointName = recoilIKDef->GetLeftWeaponJoint().c_str();
    jidx = pDefaultSkeleton->GetJointIDByName(pJointName);
    if (jidx > 0)
    {
        pDefaultSkeleton->m_recoilDesc.m_weaponLeftJointIndex = jidx;
    }

    uint impactJointCount = recoilIKDef->GetImpactJointCount();
    for (uint i = 0; i < impactJointCount; ++i)
    {
        SRecoilJoints Recoil;
        const SRecoilImpactJoint* parsedJoint = recoilIKDef->GetImpactJoint(i);
        const char* pJointName = parsedJoint->m_jointName.c_str();
        int32 jidx = pDefaultSkeleton->GetJointIDByName(pJointName);
        if (jidx > 0)
        {
            Recoil.m_nIdx = jidx;
            Recoil.m_strJointName = pDefaultSkeleton->GetJointNameByID(jidx);
        }
        AZ_Warning("CHRParams", jidx >= 0, "Failed to locate joint \"%s\".", pJointName);
        Recoil.m_nArm = parsedJoint->m_arm;
        Recoil.m_fDelay = parsedJoint->m_delay;
        Recoil.m_fWeight = parsedJoint->m_weight;
        //  Recoil.m_nAdditive=0;
        pDefaultSkeleton->m_recoilDesc.m_joints.push_back(Recoil);
    }

    return true;
}

uint32 CParamLoader::LoadAimLookIKRotationList(const IChrParamsAimLookBase* aimLookIKDef, DynArray<SJointsAimIK_Rot>& runtimeData)
{
    CDefaultSkeleton* pDefaultSkeleton = m_pDefaultSkeleton;
    uint32 errorCount = 0;
    uint rotCount = aimLookIKDef->GetRotationCount();
    runtimeData.reserve(rotCount);

    for (uint i = 0; i < rotCount; ++i)
    {
        SJointsAimIK_Rot LookIK_Rot;
        const SAimLookPosRot* parsedRot = aimLookIKDef->GetRotation(i);
        const char* pJointName = parsedRot->m_jointName.c_str();
        const int32 jidx = pDefaultSkeleton->GetJointIDByName(pJointName);
        if (jidx > 0)
        {
            LookIK_Rot.m_nJointIdx = jidx;
            LookIK_Rot.m_strJointName = pDefaultSkeleton->GetJointNameByID(jidx);
        }
        else
        {
            errorCount++;
            g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, m_pDefaultSkeleton->GetModelFilePath(), "JointName '%s' for Look-Pose not found in model", pJointName);
        }
        AZ_Warning("CHRParams", jidx >= 0, "Failed to locate joint \"%s\".", pJointName);

        LookIK_Rot.m_nPreEvaluate = parsedRot->m_primary;
        LookIK_Rot.m_nAdditive = parsedRot->m_additive;

        const uint32 numRotations = runtimeData.size();
        const int32 parentJointIndex = pDefaultSkeleton->GetJointParentIDByID(jidx);
        for (uint32 p = 0; p < numRotations; ++p)
        {
            const SJointsAimIK_Rot& otherLookIkRot = runtimeData[p];
            if (LookIK_Rot.m_nPreEvaluate && otherLookIkRot.m_nPreEvaluate)
            {
                const bool isCurrentJointParentOfPreviousJoint = FindJointInParentHierarchy(pDefaultSkeleton, jidx, otherLookIkRot.m_nJointIdx);
                if (isCurrentJointParentOfPreviousJoint)
                {
                    errorCount++;
                    g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, m_pDefaultSkeleton->GetModelFilePath(), "Primary joint with name '%s' for Look-Pose should not be declared after joint '%s'.", pJointName, otherLookIkRot.m_strJointName);
                }
            }
            if (otherLookIkRot.m_nJointIdx == parentJointIndex)
            {
                LookIK_Rot.m_nRotJointParentIdx = p;
                break;
            }
        }
        runtimeData.push_back(LookIK_Rot);
    }

    return errorCount;
}

uint32 CParamLoader::LoadAimLookIKPositionList(const IChrParamsAimLookBase* aimLookIKDef, DynArray<SJointsAimIK_Pos>& runtimeList)
{
    CDefaultSkeleton* pDefaultSkeleton = m_pDefaultSkeleton;
    uint32 errorCount = 0;
    uint posCount = aimLookIKDef->GetPositionCount();
    runtimeList.reserve(posCount);

    for (uint i = 0; i < posCount; ++i)
    {
        SJointsAimIK_Pos LookIK_Pos;
        const SAimLookPosRot* parsedPos = aimLookIKDef->GetPosition(i);
        const char* pJointName = parsedPos->m_jointName;
        int32 jidx = pDefaultSkeleton->GetJointIDByName(pJointName);
        if (jidx > 0)
        {
            LookIK_Pos.m_nJointIdx = jidx;
            LookIK_Pos.m_strJointName = pDefaultSkeleton->GetJointNameByID(jidx);
        }
        else
        {
            errorCount++;
            g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, m_pDefaultSkeleton->GetModelFilePath(), "JointName '%s' for Look-Pose not found in model", pJointName);
        }

        AZ_Warning("CHRParams", jidx >= 0, "Failed to locate joint \"%s\".", pJointName);

        LookIK_Pos.m_nAdditive = parsedPos->m_additive;
        runtimeList.push_back(LookIK_Pos);
    }

    return errorCount;
}

uint32 CParamLoader::LoadAimLookIKDirectionalBlends(const IChrParamsAimLookBase* aimLookIKDef, DynArray<DirectionalBlends>& runtimeList)
{
    CDefaultSkeleton* pDefaultSkeleton = m_pDefaultSkeleton;
    uint32 errorCount = 0;
    uint dirBlendCount = aimLookIKDef->GetDirectionalBlendCount();
    runtimeList.reserve(dirBlendCount);

    for (uint i = 0; i < dirBlendCount; ++i)
    {
        DirectionalBlends DirBlend;
        const SAimLookDirectionalBlend* parsedDirBlend = aimLookIKDef->GetDirectionalBlend(i);
        const char* pAnimToken = parsedDirBlend->m_animToken.c_str();
        DirBlend.m_AnimToken = pAnimToken;
        DirBlend.m_AnimTokenCRC32 = CCrc32::ComputeLowercase(pAnimToken);
        const char* pParameterJointName = parsedDirBlend->m_parameterJoint.c_str();
        int jidx1 = pDefaultSkeleton->GetJointIDByName(pParameterJointName);
        if (jidx1 >= 0)
        {
            DirBlend.m_nParaJointIdx = jidx1;
            DirBlend.m_strParaJointName = pDefaultSkeleton->GetJointNameByID(jidx1);
        }
        else
        {
            errorCount++;
            g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, m_pDefaultSkeleton->GetModelFilePath(), "Look-Pose Error: JointName '%s' for Look-Pose not found in model", pParameterJointName);
        }
        AZ_Warning("CHRParams", jidx1 >= 0, "Failed to locate joint \"%s\".", pParameterJointName);

        const char* pStartJointName = parsedDirBlend->m_startJoint.c_str();
        int jidx2 = pDefaultSkeleton->GetJointIDByName(pStartJointName);
        if (jidx2 >= 0)
        {
            DirBlend.m_nStartJointIdx = jidx2;
            DirBlend.m_strStartJointName = pDefaultSkeleton->GetJointNameByID(jidx2);
        }
        else
        {
            errorCount++;
            g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, m_pDefaultSkeleton->GetModelFilePath(), "Look-Pose Error:  Start JointName '%s' for Look-Pose not found in model", pStartJointName);
        }
        AZ_Warning("CHRParams", jidx2 >= 0, "Failed to locate joint \"%s\".", pStartJointName);

        const char* pReferenceJointName = parsedDirBlend->m_referenceJoint.c_str();
        int jidx3 = pDefaultSkeleton->GetJointIDByName(pReferenceJointName);
        if (jidx3 >= 0)
        {
            DirBlend.m_nReferenceJointIdx = jidx3;
            DirBlend.m_strReferenceJointName = pDefaultSkeleton->GetJointNameByID(jidx3);
        }
        else
        {
            errorCount++;
            g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, m_pDefaultSkeleton->GetModelFilePath(), "Look-Pose Error: Reference JointName '%s' for Look-Pose not found in model", pReferenceJointName);
        }
        AZ_Warning("CHRParams", jidx3 >= 0, "Failed to locate joint \"%s\".", pReferenceJointName);

        runtimeList.push_back(DirBlend);
    }

    return errorCount;
}

void CParamLoader::UpdateSharedPosRotJoints(const DynArray<SJointsAimIK_Pos>& positionList, DynArray<SJointsAimIK_Rot>& rotationList)
{
    uint32 numRot = rotationList.size();
    uint32 numPos = positionList.size();
    for (uint32 r = 0; r < numRot; r++)
    {
        for (uint32 p = 0; p < numPos; p++)
        {
            const char* pRotName = rotationList[r].m_strJointName;
            if (pRotName == 0)
            {
                continue;
            }
            const char* pPosName = positionList[p].m_strJointName;
            if (pPosName == 0)
            {
                continue;
            }
            uint32 SameJoint = strcmp(pRotName, pPosName) == 0;
            if (SameJoint)
            {
                rotationList[r].m_nPosIndex = p;
                break;
            }
        }
    }
}


bool CParamLoader::LoadIKDefLookIK(const CLookIKDef* lookIKDef)
{
    CDefaultSkeleton* pDefaultSkeleton = m_pDefaultSkeleton;
    PoseBlenderLookDesc& poseBlenderLookDesc = pDefaultSkeleton->m_poseBlenderLookDesc;

    pDefaultSkeleton->m_poseBlenderLookDesc.m_error = 0;
    const char* pName = lookIKDef->GetLeftEyeAttachment().c_str();
    poseBlenderLookDesc.m_eyeAttachmentLeftName = pName;              //left eyeball attachment

    pName = lookIKDef->GetRightEyeAttachment().c_str();
    poseBlenderLookDesc.m_eyeAttachmentRightName = pName;             //right eyeball attachment

    const SEyeLimits* eyeLimits = lookIKDef->GetEyeLimits();
    poseBlenderLookDesc.m_eyeLimitHalfYawRadians = DEG2RAD(eyeLimits->m_halfYaw);
    poseBlenderLookDesc.m_eyeLimitPitchRadiansUp = DEG2RAD(eyeLimits->m_pitchUp);
    poseBlenderLookDesc.m_eyeLimitPitchRadiansDown = DEG2RAD(eyeLimits->m_pitchDown);

    poseBlenderLookDesc.m_error += LoadAimLookIKRotationList(lookIKDef, poseBlenderLookDesc.m_rotations);
    poseBlenderLookDesc.m_error += LoadAimLookIKPositionList(lookIKDef, poseBlenderLookDesc.m_positions);
    poseBlenderLookDesc.m_error += LoadAimLookIKDirectionalBlends(lookIKDef, poseBlenderLookDesc.m_blends);

    {
        InitalizeBlendRotJointIndices(poseBlenderLookDesc.m_blends, poseBlenderLookDesc.m_rotations);
    }

    //----------------------------------------------------------------------------------------
    if (poseBlenderLookDesc.m_error)
    {
        poseBlenderLookDesc.m_rotations.clear();
        poseBlenderLookDesc.m_positions.clear();
        g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, m_pDefaultSkeleton->GetModelFilePath(), "CryAnimation Error: Look-Poses disabled");
        return false;
    }

    UpdateSharedPosRotJoints(poseBlenderLookDesc.m_positions, poseBlenderLookDesc.m_rotations);

    return true;
}





bool CParamLoader::LoadIKDefAimIK(const CAimIKDef* aimIKDef)
{
    CDefaultSkeleton* pDefaultSkeleton = m_pDefaultSkeleton;
    PoseBlenderAimDesc& poseBlenderAimDesc = pDefaultSkeleton->m_poseBlenderAimDesc;


    //---------------------------------------------------------------------
    poseBlenderAimDesc.m_error = 0;
    poseBlenderAimDesc.m_error += LoadAimLookIKDirectionalBlends(aimIKDef, poseBlenderAimDesc.m_blends);
    poseBlenderAimDesc.m_error += LoadAimLookIKRotationList(aimIKDef, poseBlenderAimDesc.m_rotations);
    poseBlenderAimDesc.m_error += LoadAimLookIKPositionList(aimIKDef, poseBlenderAimDesc.m_positions);

    uint32 procAdjustmentSize = aimIKDef->GetProcAdjustmentCount();
    pDefaultSkeleton->m_recoilDesc.m_joints.reserve(procAdjustmentSize);
    for (uint32 i = 0; i < procAdjustmentSize; i++)
    {
        ProcAdjust Spine;
        const char* pJointName = aimIKDef->GetProcAdjustmentJoint(i).c_str();
        int32 jidx = pDefaultSkeleton->GetJointIDByName(pJointName);
        if (jidx > 0)
        {
            Spine.m_nIdx = jidx;
            Spine.m_strJointName = pDefaultSkeleton->GetJointNameByID(jidx);
        }
        else
        {
            poseBlenderAimDesc.m_error++;
            g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, m_pDefaultSkeleton->GetModelFilePath(), "Aim-Pose Error: ProcAdjustment JointName '%s' for Aim-Pose not found in model", pJointName);
        }
        poseBlenderAimDesc.m_procAdjustments.push_back(Spine);
    }

    {
        InitalizeBlendRotJointIndices(poseBlenderAimDesc.m_blends, poseBlenderAimDesc.m_rotations);
    }

    //----------------------------------------------------------------------------------------
    if (poseBlenderAimDesc.m_error)
    {
        poseBlenderAimDesc.m_rotations.clear();
        poseBlenderAimDesc.m_positions.clear();
        poseBlenderAimDesc.m_procAdjustments.clear();
        g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, m_pDefaultSkeleton->GetModelFilePath(), "CryAnimation Error: Aim-Poses disabled");
        return false;
    }

    UpdateSharedPosRotJoints(poseBlenderAimDesc.m_positions, poseBlenderAimDesc.m_rotations);

    return true;
}




