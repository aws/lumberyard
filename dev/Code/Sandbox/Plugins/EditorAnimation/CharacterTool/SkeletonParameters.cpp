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

#include "pch.h"
#include "SkeletonParameters.h"
#include <CryFile.h> // just for PathUtil::!
#include <CryPath.h>
#include <ICryAnimation.h>
#include "Serialization.h"
#include "IEditor.h"
#include <algorithm> // for std::max

namespace CharacterTool
{
    // XML tags and attributes

    static const char* g_paramsTag = "Params";
    static const char* g_animationListTag = "AnimationList";
    static const char* g_animationTag = "Animation";
    static const char* g_bboxIncludeListTag = "BBoxIncludeList";
    static const char* g_bboxExtensionListTag = "BBoxExtensionList";
    static const char* g_axisTag = "Axis";
    static const char* g_lodTag = "Lod";
    static const char* g_jointListTag = "JointList";
    static const char* g_jointTag = "Joint";
    static const char* g_ikDefinitionTag = "IK_Definition";
    static const char* g_limbIKDefinitionTag = "LimbIK_Definition";
    static const char* g_ikTag = "IK";
    static const char* g_aimIKDefinitionTag = "AimIK_Definition";
    static const char* g_directionalBlendsTag = "DirectionalBlends";
    static const char* g_rotationListTag = "RotationList";
    static const char* g_rotationTag = "Rotation";
    static const char* g_positionListTag = "PositionList";
    static const char* g_positionTag = "Position";
    static const char* g_lookIKDefinitionTag = "LookIK_Definition";
    static const char* g_recoilDefinitionTag = "Recoil_Definition";
    static const char* g_rIKHandleTag = "RIKHandle";
    static const char* g_lIKHandleTag = "LIKHandle";
    static const char* g_rWeaponJointTag = "RWeaponJoint";
    static const char* g_lWeaponJointTag = "LWeaponJoint";
    static const char* g_impactJointsTag = "ImpactJoints";
    static const char* g_impactJointTag = "ImpactJoint";
    static const char* g_feetLockDefinitionTag = "FeetLock_Definition";
    static const char* g_lEyeAttachmentTag = "LEyeAttachment";
    static const char* g_rEyeAttachmentTag = "REyeAttachment";
    static const char* g_animationDrivenIKTargetsTag = "Animation_Driven_IK_Targets";
    static const char* g_ADIKTargetTag = "ADIKTarget";

    static const char* g_additiveAttr = "Additive";
    static const char* g_animTokenAttr = "AnimToken";
    static const char* g_endEffectorAttr = "EndEffector";
    static const char* g_flagsAttr = "flags";
    static const char* g_fStepSizeAttr = "fStepSize";
    static const char* g_fThresholdAttr = "fThreshold";
    static const char* g_handleAttr = "Handle";
    static const char* g_jointNameAttr = "JointName";
    static const char* g_levelAttr = "level";
    static const char* g_nameAttr = "name";
    static const char* g_negXAttr = "negX";
    static const char* g_negYAttr = "negY";
    static const char* g_negZAttr = "negZ";
    static const char* g_nMaxInterationAttr = "nMaxInteration";
    static const char* g_parameterJointAttr = "ParameterJoint";
    static const char* g_pathAttr = "path";
    static const char* g_posXAttr = "posX";
    static const char* g_posYAttr = "posY";
    static const char* g_posZAttr = "posZ";
    static const char* g_primaryAttr = "Primary";
    static const char* g_referenceJointAttr = "ReferenceJoint";
    static const char* g_rootAttr = "Root";
    static const char* g_solverAttr = "Solver";
    static const char* g_startJointAttr = "StartJoint";
    static const char* g_armAttr = "Arm";
    static const char* g_weightAttr = "Weight";
    static const char* g_delayAttr = "Delay";
    static const char* g_NameAttr = "Name";
    static const char* g_targetAttr = "Target";

    struct Substring
    {
        const char* start;
        const char* end;

        bool operator==(const char* str) const { return strncmp(str, start, end - start) == 0; }
        bool Empty() const{ return start == end; }
    };

    static Substring GetLevel(const char** outP)
    {
        const char* p = *outP;
        while (*p != '/' && *p != '\0')
        {
            ++p;
        }
        Substring result;
        result.start = *outP;
        result.end = p;
        if (*p == '/')
        {
            ++p;
        }
        *outP = p;
        return result;
    }

    const bool MatchesWildcard(const Substring& str, const Substring& wildcard)
    {
        const char* savedStr;
        const char* savedWild;

        const char* p = str.start;
        const char* w = wildcard.start;

        while (p != str.end && (*w != '*'))
        {
            if ((*w != *p) && (*w != '?'))
            {
                return false;
            }
            ++w;
            ++p;
        }

        while (p != str.end)
        {
            if (*w == '*')
            {
                ++w;
                if (w == wildcard.end)
                {
                    return true;
                }
                savedWild = w;
                savedStr = p + 1;
            }
            else if ((*w == *p) || (*w == '?'))
            {
                ++w;
                ++p;
            }
            else
            {
                w = savedWild;
                p = savedStr++;
            }
        }

        while (*w == '*')
        {
            ++w;
        }

        return w == wildcard.end;
    }

    static bool MatchesPathWildcardNormalized(const char* path, const char* wildcard)
    {
        std::vector<std::pair<const char*, const char*> > border;
        border.push_back(std::make_pair(path, wildcard));

        while (!border.empty())
        {
            const char* p = border.front().first;
            const char* w = border.front().second;
            border.erase(border.begin());

            while (true)
            {
                Substring ws = GetLevel(&w);

                Substring ps = GetLevel(&p);

                bool containsWildCardsOnly = !ws.Empty();
                for (const char* c = ws.start; c != ws.end; ++c)
                {
                    if (*c != '*')
                    {
                        containsWildCardsOnly = false;
                        break;
                    }
                }

                if (containsWildCardsOnly)
                {
                    if (*p != '\0')
                    {
                        border.push_back(std::make_pair(p, ws.start));
                    }
                    border.push_back(std::make_pair(ps.start, w));
                }

                if (ws.Empty())
                {
                    if (ps.Empty())
                    {
                        return true;
                    }
                    else
                    {
                        break;
                    }
                }
                if (!MatchesWildcard(ps, ws))
                {
                    ps = GetLevel(&p);
                    bool matched = false;
                    while (!ps.Empty())
                    {
                        if (MatchesWildcard(ps, ws))
                        {
                            matched = true;
                            break;
                        }
                        ps = GetLevel(&p);
                    }
                    if (!matched)
                    {
                        break;
                    }
                }
                if (ps.Empty() && ws.Empty())
                {
                    break;
                }
            }
        }
        return false;
    }

    static bool MatchesPathWildcard(const char* path, const char* wildcard)
    {
        stack_string normalizedPath = path;
        normalizedPath.replace('\\', '/');
        normalizedPath.MakeLower();

        stack_string normalizedWildcard = wildcard;
        normalizedWildcard.replace('\\', '/');
        normalizedWildcard.MakeLower();

        return MatchesPathWildcardNormalized(normalizedPath.c_str(), normalizedWildcard.c_str());
    }

    bool AnimationSetFilter::Matches(const char* cafPath) const
    {
        for (size_t i = 0; i < folders.size(); ++i)
        {
            const AnimationFilterFolder& folder = folders[i];
            for (size_t j = 0; j < folder.wildcards.size(); ++j)
            {
                string combinedWildcard = folder.path + "/" + folder.wildcards[j].fileWildcard;
                if (MatchesPathWildcard(cafPath, combinedWildcard.c_str()))
                {
                    return true;
                }
            }
        }
        return false;
    }

    // ---------------------------------------------------------------------------

    BBoxExtension::BBoxExtension()
        : pos(ZERO)
        , neg(ZERO)
    {
    }

    void BBoxExtension::Serialize(IArchive& ar)
    {
        ar(Serialization::LocalToEntity(pos), "pos", "Pos");
        ar(Serialization::LocalToEntity(neg), "neg", "Neg");
    }

    void BBoxExtension::Clear()
    {
        pos = Vec3(ZERO);
        neg = Vec3(ZERO);
    }

    // ---------------------------------------------------------------------------

    static const string& PersistentString(const string& s)
    {
        static std::set<string> stringSet;
        return *stringSet.insert(string(s)).first;
    }

    struct JointTreeSerializer
    {
        typedef AZStd::function<void(IArchive&, const char*)> JointSerializer;

        struct Node
        {
            const char* name;
            const char* label;
            std::vector<const Node*> children;

            Node(const char* name, const char* label)
                : name(name)
                , label(label)
            {
            }

            void Serialize(IArchive& ar)
            {
                JointSerializer* serializer = ar.FindContext<JointSerializer>();
                (*serializer)(ar, name);

                for (auto child : children)
                {
                    ar(*child, child->name, child->label);
                }
            }
        };

        std::vector<std::unique_ptr<Node>> m_joints;
        JointSerializer m_jointSerializer;

        JointTreeSerializer(IDefaultSkeleton* skeleton, const JointSerializer& jointSerializer)
            : m_jointSerializer(jointSerializer)
        {
            int numJoints = skeleton->GetJointCount();
            m_joints.reserve(numJoints);

            for (int i = 0; i < numJoints; i++)
            {
                const char* name = skeleton->GetJointNameByID(i);

                // control codes:
                //   +   expand children
                //   @R  force regular (non-boldface) font
                //   :   disable copy-paste
                const char* label = PersistentString("+@R:" + string(name)).c_str();

                m_joints.emplace_back(new Node(name, label));
            }

            for (int i = 0; i < numJoints; i++)
            {
                int parentId = skeleton->GetJointParentIDByID(i);
                if (parentId >= 0)
                {
                    m_joints[parentId]->children.push_back(m_joints[i].get());
                }
            }
        }

        void Serialize(IArchive& ar)
        {
            if (!m_joints.empty())
            {
                Serialization::SContext<JointSerializer> context(ar, &m_jointSerializer);
                ar(*m_joints[0], m_joints[0]->name, m_joints[0]->label);
            }
        }
    };

    struct JointSetSerializingFunctor
    {
        std::vector<string>& joints;
        IDefaultSkeleton* m_skeleton;
        bool m_enableAncestors;

        JointSetSerializingFunctor(std::vector<string>& joints, IDefaultSkeleton* skeleton, bool enableAncestors = false)
            : joints(joints)
            , m_skeleton(skeleton)
            , m_enableAncestors(enableAncestors)
        {
        }

        void operator()(IArchive& ar, const char* jointName)
        {
            bool enableJoint = (std::find(joints.begin(), joints.end(), jointName) != joints.end());
            bool previousEnableJoint = enableJoint;

            ar(enableJoint, "enable", "^^");

            if (ar.IsInput() && previousEnableJoint != enableJoint)
            {
                if (enableJoint)
                {
                    joints.emplace_back(jointName);

                    if (m_enableAncestors) {
                        // enable ancestor joints

                        int parentId = m_skeleton->GetJointParentIDByID(m_skeleton->GetJointIDByName(jointName));

                        while (parentId >= 0)
                        {
                            const char* parentName = m_skeleton->GetJointNameByID(parentId);

                            if (std::find(joints.begin(), joints.end(), parentName) == joints.end())
                            {
                                joints.emplace_back(parentName);
                            }

                            parentId = m_skeleton->GetJointParentIDByID(parentId);
                        }
                    }
                }
                else
                {
                    joints.erase(
                            std::remove(joints.begin(), joints.end(), jointName),
                            joints.end());
                }
            }
        };
    };

    // ---------------------------------------------------------------------------

    BBoxIncludes::BBoxIncludes()
        : m_initialized(false)
    {
    }

    void BBoxIncludes::Serialize(IArchive& ar)
    {
        if (ar.IsEdit())
        {
            IDefaultSkeleton* skeleton = ar.FindContext<IDefaultSkeleton>();
            if (skeleton)
            {
                if (!m_initialized)
                {
                    // if no joints were defined in the file, enable all
                    // joints by default
                    if (joints.empty())
                    {
                        int numJoints = skeleton->GetJointCount();
                        for (int i = 0; i < numJoints; i++)
                        {
                            joints.emplace_back(skeleton->GetJointNameByID(i));
                        }
                    }

                    m_initialized = true;
                }

                // remove joints with invalid names
                for (auto it = joints.begin(); it != joints.end(); )
                {
                    if (skeleton->GetJointIDByName(it->c_str()) < 0)
                    {
                        it = joints.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }

                JointTreeSerializer(skeleton, JointSetSerializingFunctor(joints, skeleton)).Serialize(ar);
            }
        }
        else
        {
            ar(joints, "joints", "^^");
        }
    }

    void BBoxIncludes::Clear()
    {
        joints.clear();
        m_initialized = false;
    }

    // ---------------------------------------------------------------------------

    SERIALIZATION_ENUM_BEGIN_NESTED2(IKDefinition, LimbIKEntry, Solver, "Solver")
    SERIALIZATION_ENUM(IKDefinition::LimbIKEntry::Solver::TwoBone, "solver_2b", "2BIK")
    SERIALIZATION_ENUM(IKDefinition::LimbIKEntry::Solver::ThreeBone, "solver_3b", "3BIK")
    SERIALIZATION_ENUM(IKDefinition::LimbIKEntry::Solver::CCDX, "solver_ccdx", "CCDX")
    SERIALIZATION_ENUM_END()

    IKDefinition::LimbIKEntry::LimbIKEntry()
        : solver(Solver::TwoBone)
        , stepSize(0)
        , threshold(0)
        , maxIteration(0)
    {
    }

    void IKDefinition::LimbIKEntry::Serialize(IArchive& ar)
    {
        ar(solver, "solver", "Solver");

        ar(handle, "handle", "Handle");
        if (handle.empty())
        {
            ar.Error(handle, "Please specify a name for the handle.");
        }
        else if (handle.size() > s_maxHandleLength)
        {
            ar.Error(handle, "Handle name must be no longer than %d characters.", s_maxHandleLength);
        }
        else
        {
            // check if there's another entry with the same handle
            if (auto parent = ar.FindContext<IKDefinition::LimbIK>())
            {
                const auto& entries = parent->entries;
                auto it = std::find_if(entries.begin(), entries.end(),
                                [&](const IKDefinition::LimbIKEntry& entry)
                                {
                                    return &entry != this && entry.handle == handle;
                                });
                if (it != entries.end())
                {
                    ar.Error(handle, "Duplicated handle name.");
                }
            }
        }

        ar(JointName(root), "root", "Root");
        ar(JointName(endEffector), "endEffector", "End Effector");

        if (!ar.IsEdit() || solver == Solver::CCDX)
        {
            ar(stepSize, "stepSize", "Step Size");
            ar(threshold, "threshold", "Threshold");
            ar(maxIteration, "maxIteration", "Max Iteration");
        }
    }

    IKDefinition::LimbIK::LimbIK()
        : enabled(false)
    {
    }

    void IKDefinition::LimbIK::LoadFromXML(const XmlNodeRef& node)
    {
        if (XmlNodeRef limbIKNode = node->findChild(g_limbIKDefinitionTag))
        {
            enabled = true;

            for (int i = 0, count = limbIKNode->getChildCount(); i < count; ++i)
            {
                XmlNodeRef ikNode = limbIKNode->getChild(i);
                if (azstricmp(ikNode->getTag(), g_ikTag))
                {
                    continue;
                }

                IKDefinition::LimbIKEntry entry;

                const char* solver = ikNode->getAttr(g_solverAttr);
                if (azstricmp(solver, "2BIK") == 0)
                {
                    entry.solver = IKDefinition::LimbIKEntry::Solver::TwoBone;
                }
                else if (azstricmp(solver, "3BIK") == 0)
                {
                    entry.solver = IKDefinition::LimbIKEntry::Solver::ThreeBone;
                }
                else if (azstricmp(solver, "CCDX") == 0)
                {
                    entry.solver = IKDefinition::LimbIKEntry::Solver::CCDX;
                }

                entry.handle = ikNode->getAttr(g_handleAttr);
                entry.root = ikNode->getAttr(g_rootAttr);
                entry.endEffector = ikNode->getAttr(g_endEffectorAttr);

                ikNode->getAttr(g_fStepSizeAttr, entry.stepSize);
                ikNode->getAttr(g_fThresholdAttr, entry.threshold);
                ikNode->getAttr(g_nMaxInterationAttr, entry.maxIteration);

                entries.push_back(entry);
            }
        }
    }

    void IKDefinition::LimbIK::SaveToXML(const XmlNodeRef& node) const
    {
        if (enabled)
        {
            XmlNodeRef limbIKNode = node->createNode(g_limbIKDefinitionTag);
            node->addChild(limbIKNode);

            for (auto& entry : entries)
            {
                XmlNodeRef ikNode = limbIKNode->createNode(g_ikTag);
                limbIKNode->addChild(ikNode);

                switch (entry.solver)
                {
                    case IKDefinition::LimbIKEntry::Solver::TwoBone:
                        ikNode->setAttr(g_solverAttr, "2BIK");
                        break;

                    case IKDefinition::LimbIKEntry::Solver::ThreeBone:
                        ikNode->setAttr(g_solverAttr, "3BIK");
                        break;

                    case IKDefinition::LimbIKEntry::Solver::CCDX:
                        ikNode->setAttr(g_solverAttr, "CCDX");
                        break;
                }

                ikNode->setAttr(g_handleAttr, entry.handle);
                ikNode->setAttr(g_rootAttr, entry.root);
                ikNode->setAttr(g_endEffectorAttr, entry.endEffector);

                if (entry.solver == IKDefinition::LimbIKEntry::Solver::CCDX)
                {
                    ikNode->setAttr(g_fStepSizeAttr, entry.stepSize);
                    ikNode->setAttr(g_fThresholdAttr, entry.threshold);
                    ikNode->setAttr(g_nMaxInterationAttr, entry.maxIteration);
                }
            }
        }
    }

    void IKDefinition::LimbIK::Serialize(IArchive& ar)
    {
        ar(enabled, "enabled", "^^");
        if (!ar.IsEdit() || enabled)
        {
            Serialization::SContext<IKDefinition::LimbIK> context(ar, this);
            ar(entries, "entries", "Limb IK Definitions");
        }
    }

    void IKDefinition::LimbIK::Clear()
    {
        enabled = false;
        entries.clear();
    }

    // ---------------------------------------------------------------------------

    IKDefinition::Rotation::Rotation(const string& joint)
        : joint(joint)
        , additive(false)
        , primary(false)
    {
    }

    void IKDefinition::Rotation::Serialize(IArchive& ar)
    {
        ar(joint, "joint");
        ar(additive, "additive");
        ar(primary, "primary");
    }

    void IKDefinition::RotationList::LoadFromXML(const XmlNodeRef& node)
    {
        const int count = node->getChildCount();
        for (int i = 0; i < count; i++)
        {
            XmlNodeRef rotationNode = node->getChild(i);
            if (azstricmp(rotationNode->getTag(), g_rotationTag) != 0)
            {
                continue;
            }

            IKDefinition::Rotation rotation;
            rotationNode->getAttr(g_additiveAttr, rotation.additive);
            rotationNode->getAttr(g_primaryAttr, rotation.primary);
            rotation.joint = rotationNode->getAttr(g_jointNameAttr);

            rotations.push_back(rotation);
        }
    }

    void IKDefinition::RotationList::SaveToXML(const XmlNodeRef& node) const
    {
        XmlNodeRef rotationListNode = node->createNode(g_rotationListTag);
        node->addChild(rotationListNode);

        for (auto& rotation : rotations)
        {
            XmlNodeRef rotationNode = rotationListNode->createNode(g_rotationTag);
            rotationListNode->addChild(rotationNode);

            rotationNode->setAttr(g_additiveAttr, static_cast<int>(rotation.additive));
            rotationNode->setAttr(g_primaryAttr, static_cast<int>(rotation.primary));
            rotationNode->setAttr(g_jointNameAttr, rotation.joint);
        }
    }

    void IKDefinition::RotationList::Serialize(IArchive& ar)
    {
        if (ar.IsEdit())
        {
            if (IDefaultSkeleton* skeleton = ar.FindContext<IDefaultSkeleton>())
            {
                auto serializer = [&](IArchive& ar, const char* jointName)
                {
                    auto it = std::find_if(rotations.begin(), rotations.end(),
                                    [&](const Rotation& rotation)
                                    {
                                        return rotation.joint == jointName;
                                    });
                    bool enable = (it != rotations.end());
                    bool previousEnable = enable;

                    ar(enable, "enable", "^^");
                    if (previousEnable)
                    {
                        ar(it->additive, "additive", "^add");
                        ar(it->primary, "primary", "^prim");
                    }

                    if (ar.IsInput() && previousEnable != enable)
                    {
                        if (enable)
                        {
                            rotations.emplace_back(jointName);
                        }
                        else
                        {
                            rotations.erase(
                                    std::remove_if(rotations.begin(), rotations.end(),
                                        [&](const Rotation& rotation)
                                        {
                                            return rotation.joint == jointName;
                                        }),
                                    rotations.end());
                        }
                    }
                };

                ar(JointTreeSerializer(skeleton, serializer), "rotations", "^^");

                if (ar.IsInput())
                {
                    // sort rotations so that:
                    //   * primary rotations precede non-primary rotations
                    //   * rotations with parent joints precede rotations with children joints
                    std::sort(rotations.begin(), rotations.end(),
                                    [&](const Rotation& a, const Rotation& b)
                                    {
                                        if (a.primary != b.primary)
                                        {
                                            return a.primary == true;
                                        }
                                        else
                                        {
                                            // joints with lower IDs are always ancestors of joints with higher IDs
                                            return skeleton->GetJointIDByName(a.joint) < skeleton->GetJointIDByName(b.joint);
                                        }
                                    });
                }
            }
        }
        else
        {
            ar(rotations, "rotations", "Rotation List");
        }
    }

    void IKDefinition::RotationList::Clear()
    {
        rotations.clear();
    }

    // ---------------------------------------------------------------------------

    IKDefinition::Position::Position(const string& joint)
        : joint(joint)
        , additive(false)
    {
    }

    void IKDefinition::Position::Serialize(IArchive& ar)
    {
        ar(joint, "joint");
        ar(additive, "additive");
    }

    void IKDefinition::PositionList::LoadFromXML(const XmlNodeRef& node)
    {
        const int count = node->getChildCount();
        for (int i = 0; i < count; i++)
        {
            XmlNodeRef positionNode = node->getChild(i);
            if (azstricmp(positionNode->getTag(), g_positionTag) != 0)
            {
                continue;
            }

            IKDefinition::Position position;
            positionNode->getAttr(g_additiveAttr, position.additive);
            position.joint = positionNode->getAttr(g_jointNameAttr);

            positions.push_back(position);
        }
    }

    void IKDefinition::PositionList::SaveToXML(const XmlNodeRef& node) const
    {
        XmlNodeRef positionListNode = node->createNode(g_positionListTag);
        node->addChild(positionListNode);

        for (auto& position : positions)
        {
            XmlNodeRef positionNode = positionListNode->createNode(g_positionTag);
            positionListNode->addChild(positionNode);

            positionNode->setAttr(g_additiveAttr, static_cast<int>(position.additive));
            positionNode->setAttr(g_jointNameAttr, position.joint);
        }
    }

    void IKDefinition::PositionList::Serialize(IArchive& ar)
    {
        if (ar.IsEdit())
        {
            if (IDefaultSkeleton* skeleton = ar.FindContext<IDefaultSkeleton>())
            {
                auto serializer = [&](IArchive& ar, const char* jointName)
                {
                    auto it = std::find_if(positions.begin(), positions.end(),
                                    [&](const Position& position)
                                    {
                                        return position.joint == jointName;
                                    });

                    bool enable = (it != positions.end() && jointName == it->joint);
                    bool previousEnable = enable;

                    ar(enable, "enable", "^^");
                    if (previousEnable)
                    {
                        ar(it->additive, "additive", "^add");
                    }

                    if (ar.IsInput() && previousEnable != enable)
                    {
                        if (enable)
                        {
                            positions.emplace_back(jointName);
                        }
                        else
                        {
                            positions.erase(
                                    std::remove_if(positions.begin(), positions.end(),
                                        [&](const Position& position)
                                        {
                                            return position.joint == jointName;
                                        }),
                                    positions.end());

                        }
                    }
                };

                ar(JointTreeSerializer(skeleton, serializer), "positions", "^^");
            }
        }
        else
        {
            ar(positions, "positions", "Position List");
        }
    }

    void IKDefinition::PositionList::Clear()
    {
        positions.clear();
    }

    // ---------------------------------------------------------------------------

    void IKDefinition::DirectionalBlend::Serialize(IArchive& ar)
    {
        bool enforceStartEqualsParameterJoint = false;
        if (auto parent = ar.FindContext<IKDefinition::DirectionalBlendList>())
        {
            enforceStartEqualsParameterJoint = parent->m_enforceStartEqualsParameterJoint;
        }

        ar(animToken, "animToken", "AnimToken");
        if (animToken.empty())
        {
            ar.Error(animToken, "AnimToken must not be empty.");
        }

        ar(JointName(parameterJoint), "parameterJoint", "Parameter Joint");

        if (!enforceStartEqualsParameterJoint)
        {
            ar(JointName(startJoint), "startJoint", "Start Joint");
        }
        else
        {
            if (ar.IsInput())
            {
                // skip copying value from property tree, copy parameterJoint
                startJoint = parameterJoint;
            }

            if (ar.IsOutput() && ar.IsEdit())
            {
                ar(startJoint, "startJoint", "!!Start Joint");
            }
        }

        ar(JointName(referenceJoint), "referenceJont", "Reference Joint");
    }

    IKDefinition::DirectionalBlendList::DirectionalBlendList(bool enforceStartEqualsParameterJoint)
        : m_enforceStartEqualsParameterJoint(enforceStartEqualsParameterJoint)
    {
    }

    void IKDefinition::DirectionalBlendList::LoadFromXML(const XmlNodeRef& node)
    {
        const int count = node->getChildCount();
        for (int i = 0; i < count; i++)
        {
            XmlNodeRef jointNode = node->getChild(i);
            if (azstricmp(jointNode->getTag(), g_jointTag) != 0)
            {
                continue;
            }

            DirectionalBlend blend;

            blend.animToken = jointNode->getAttr(g_animTokenAttr);
            blend.parameterJoint = jointNode->getAttr(g_parameterJointAttr);
            blend.startJoint = jointNode->getAttr(g_startJointAttr);
            blend.referenceJoint = jointNode->getAttr(g_referenceJointAttr);

            directionalBlends.push_back(blend);
        }
    }

    void IKDefinition::DirectionalBlendList::SaveToXML(const XmlNodeRef& node) const
    {
        XmlNodeRef directionalBlendsNode = node->createNode(g_directionalBlendsTag);
        node->addChild(directionalBlendsNode);

        for (auto& blend : directionalBlends)
        {
            XmlNodeRef jointNode = directionalBlendsNode->createNode(g_jointTag);
            directionalBlendsNode->addChild(jointNode);

            jointNode->setAttr(g_animTokenAttr, blend.animToken);
            jointNode->setAttr(g_parameterJointAttr, blend.parameterJoint);
            jointNode->setAttr(g_startJointAttr, blend.startJoint);
            jointNode->setAttr(g_referenceJointAttr, blend.referenceJoint);
        }
    }

    void IKDefinition::DirectionalBlendList::Serialize(IArchive& ar)
    {
        Serialization::SContext<IKDefinition::DirectionalBlendList> context(ar, this);
        ar(directionalBlends, "directionalBlends", "^^");
    }

    void IKDefinition::DirectionalBlendList::Clear()
    {
        directionalBlends.clear();
    }

    // ---------------------------------------------------------------------------

    IKDefinition::AimIK::AimIK()
        : enabled(false)
        , directionalBlends(false)
    {
    }

    void IKDefinition::AimIK::LoadFromXML(const XmlNodeRef& node)
    {
        if (XmlNodeRef aimIKNode = node->findChild(g_aimIKDefinitionTag))
        {
            enabled = true;

            if (XmlNodeRef directionalBlendsNode = aimIKNode->findChild(g_directionalBlendsTag))
            {
                directionalBlends.LoadFromXML(directionalBlendsNode);
            }

            if (XmlNodeRef rotationListNode = aimIKNode->findChild(g_rotationListTag))
            {
                rotations.LoadFromXML(rotationListNode);
            }

            if (XmlNodeRef positionListNode = aimIKNode->findChild(g_positionListTag))
            {
                positions.LoadFromXML(positionListNode);
            }
        }
    }

    void IKDefinition::AimIK::SaveToXML(const XmlNodeRef& node) const
    {
        if (enabled)
        {
            XmlNodeRef aimIKNode = node->createNode(g_aimIKDefinitionTag);
            node->addChild(aimIKNode);

            directionalBlends.SaveToXML(aimIKNode);
            rotations.SaveToXML(aimIKNode);
            positions.SaveToXML(aimIKNode);
        }
    }

    void IKDefinition::AimIK::Serialize(IArchive& ar)
    {
        ar(enabled, "enabled", "^^");
        if (!ar.IsEdit() || enabled)
        {
            ar(directionalBlends, "directionalBlends", "Directional Blends");
            ar(rotations, "rotations", "Rotation List");
            ar(positions, "positions", "Position List");
        }
    }

    void IKDefinition::AimIK::Clear()
    {
        enabled = false;
        directionalBlends.Clear();
        rotations.Clear();
        positions.Clear();
    }

    // ---------------------------------------------------------------------------

    IKDefinition::LookIK::LookIK()
        : enabled(false)
        , directionalBlends(true)
        , leftEyeAttachmentEnabled(false)
        , rightEyeAttachmentEnabled(false)
    {
    }

    void IKDefinition::LookIK::LoadFromXML(const XmlNodeRef& node)
    {
        if (XmlNodeRef lookIKNode = node->findChild(g_lookIKDefinitionTag))
        {
            enabled = true;

            if (XmlNodeRef directionalBlendsNode = lookIKNode->findChild(g_directionalBlendsTag))
            {
                directionalBlends.LoadFromXML(directionalBlendsNode);
            }

            if (XmlNodeRef rotationListNode = lookIKNode->findChild(g_rotationListTag))
            {
                rotations.LoadFromXML(rotationListNode);
            }

            if (XmlNodeRef positionListNode = lookIKNode->findChild(g_positionListTag))
            {
                positions.LoadFromXML(positionListNode);
            }

            if (XmlNodeRef leftEyeAttachmentNode = lookIKNode->findChild(g_lEyeAttachmentTag))
            {
                leftEyeAttachmentEnabled = true;
                leftEyeAttachment = leftEyeAttachmentNode->getAttr(g_NameAttr);
            }

            if (XmlNodeRef rightEyeAttachmentNode = lookIKNode->findChild(g_rEyeAttachmentTag))
            {
                rightEyeAttachmentEnabled = true;
                rightEyeAttachment = rightEyeAttachmentNode->getAttr(g_NameAttr);
            }
        }
    }

    void IKDefinition::LookIK::SaveToXML(const XmlNodeRef& node) const
    {
        if (enabled)
        {
            XmlNodeRef lookIKNode = node->createNode(g_lookIKDefinitionTag);
            node->addChild(lookIKNode);

            directionalBlends.SaveToXML(lookIKNode);
            rotations.SaveToXML(lookIKNode);
            positions.SaveToXML(lookIKNode);

            if (leftEyeAttachmentEnabled)
            {
                XmlNodeRef leftEyeAttachmentNode = lookIKNode->createNode(g_lEyeAttachmentTag);
                lookIKNode->addChild(leftEyeAttachmentNode);

                leftEyeAttachmentNode->setAttr(g_NameAttr, leftEyeAttachment);
            }

            if (rightEyeAttachmentEnabled)
            {
                XmlNodeRef rightEyeAttachmentNode = lookIKNode->createNode(g_rEyeAttachmentTag);
                lookIKNode->addChild(rightEyeAttachmentNode);

                rightEyeAttachmentNode->setAttr(g_NameAttr, rightEyeAttachment);
            }
        }
    }

    void IKDefinition::LookIK::Serialize(IArchive& ar)
    {
        ar(enabled, "enabled", "^^");
        if (!ar.IsEdit() || enabled)
        {
            ar(directionalBlends, "directionalBlends", "Directional Blends");
            ar(rotations, "rotations", "Rotation List");
            ar(positions, "positions", "Position List");

            if (ar.OpenBlock("leftEyeAttachment", "Left Eye Attachment"))
            {
                ar(leftEyeAttachmentEnabled, "enabled", "^^");
                if (leftEyeAttachmentEnabled)
                {
                    ar(leftEyeAttachment, "jointName", "^");
                }

                ar.CloseBlock();
            }

            if (ar.OpenBlock("rightEyeAttachment", "Right Eye Attachment"))
            {
                ar(rightEyeAttachmentEnabled, "enabled", "^^");
                if (rightEyeAttachmentEnabled)
                {
                    ar(rightEyeAttachment, "jointName", "^");
                }

                ar.CloseBlock();
            }
        }
    }

    void IKDefinition::LookIK::Clear()
    {
        enabled = false;
        directionalBlends.Clear();
        rotations.Clear();
        positions.Clear();
        leftEyeAttachmentEnabled = rightEyeAttachmentEnabled = false;
        leftEyeAttachment.clear();
        rightEyeAttachment.clear();
    }

    // ---------------------------------------------------------------------------

    IKDefinition::ImpactJoint::ImpactJoint(const string& joint)
        : joint(joint)
        , arm(0)
        , delay(0)
        , weight(0)
    {
    }

    void IKDefinition::ImpactJoint::Serialize(IArchive& ar)
    {
        ar(joint, "joint");
        ar(arm, "arm");
        ar(delay, "delay");
        ar(weight, "weight");
    }

    // ---------------------------------------------------------------------------

    IKDefinition::RecoilIK::RecoilIK()
        : enabled(false)
    {
    }

    void IKDefinition::RecoilIK::LoadFromXML(const XmlNodeRef& node)
    {
        if (XmlNodeRef recoilNode = node->findChild(g_recoilDefinitionTag))
        {
            enabled = true;

            if (XmlNodeRef rIKHandleNode = recoilNode->findChild(g_rIKHandleTag))
            {
                rightHandle = rIKHandleNode->getAttr(g_handleAttr);
            }

            if (XmlNodeRef lIKHandleNode = recoilNode->findChild(g_lIKHandleTag))
            {
                leftHandle = lIKHandleNode->getAttr(g_handleAttr);
            }

            if (XmlNodeRef rWeaponJointNode = recoilNode->findChild(g_rWeaponJointTag))
            {
                rightWeaponJoint = rWeaponJointNode->getAttr(g_jointNameAttr);
            }

            if (XmlNodeRef lWeaponJointNode = recoilNode->findChild(g_lWeaponJointTag))
            {
                leftWeaponJoint = lWeaponJointNode->getAttr(g_jointNameAttr);
            }

            if (XmlNodeRef impactJointsNode = recoilNode->findChild(g_impactJointsTag))
            {
                for (int i = 0, count = impactJointsNode->getChildCount(); i < count; ++i)
                {
                    XmlNodeRef impactJointNode = impactJointsNode->getChild(i);
                    if (azstricmp(impactJointNode->getTag(), g_impactJointTag) != 0)
                    {
                        continue;
                    }

                    ImpactJoint impactJoint;
                    impactJointNode->getAttr(g_armAttr, impactJoint.arm);
                    impactJointNode->getAttr(g_delayAttr, impactJoint.delay);
                    impactJointNode->getAttr(g_weightAttr, impactJoint.weight);
                    impactJoint.joint = impactJointNode->getAttr(g_jointNameAttr);
                    impactJoints.push_back(impactJoint);
                }
            }
        }
    }

    void IKDefinition::RecoilIK::SaveToXML(const XmlNodeRef& node) const
    {
        if (enabled)
        {
            XmlNodeRef recoilNode = node->createNode(g_recoilDefinitionTag);
            node->addChild(recoilNode);

            XmlNodeRef rIKHandleNode = recoilNode->createNode(g_rIKHandleTag);
            recoilNode->addChild(rIKHandleNode);
            rIKHandleNode->setAttr(g_handleAttr, rightHandle);

            XmlNodeRef lIKHandleNode = recoilNode->createNode(g_lIKHandleTag);
            recoilNode->addChild(lIKHandleNode);
            lIKHandleNode->setAttr(g_handleAttr, leftHandle);

            XmlNodeRef rWeaponJointNode = recoilNode->createNode(g_rWeaponJointTag);
            recoilNode->addChild(rWeaponJointNode);
            rWeaponJointNode->setAttr(g_jointNameAttr, leftWeaponJoint);

            XmlNodeRef lWeaponJointNode = recoilNode->createNode(g_lWeaponJointTag);
            recoilNode->addChild(lWeaponJointNode);
            lWeaponJointNode->setAttr(g_jointNameAttr, rightWeaponJoint);

            XmlNodeRef impactJointsNode = recoilNode->createNode(g_impactJointsTag);
            recoilNode->addChild(impactJointsNode);

            for (auto& impactJoint : impactJoints)
            {
                XmlNodeRef impactJointNode = impactJointsNode->createNode(g_impactJointTag);
                impactJointsNode->addChild(impactJointNode);

                impactJointNode->setAttr(g_armAttr, impactJoint.arm);
                impactJointNode->setAttr(g_delayAttr, impactJoint.delay);
                impactJointNode->setAttr(g_weightAttr, impactJoint.weight);
                impactJointNode->setAttr(g_jointNameAttr, impactJoint.joint);
            }
        }
    }

    struct IKHandleSerializer
    {
        string& handle;

        IKHandleSerializer(string& handle)
            : handle(handle)
        {
        }

        void Serialize(IArchive& ar)
        {
            Serialization::StringList handles;

            handles.push_back("");

            if (auto parent = ar.FindContext<SkeletonParameters>())
            {
                const auto& limbIK = parent->ikDefinition.limbIK;
                if (limbIK.enabled)
                {
                    for (auto& entry : limbIK.entries)
                    {
                        handles.push_back(entry.handle);
                    }
                }
            }

            int index = handles.find(handle);
            Serialization::StringListValue value(handles, index == -1 ? 0 : index);

            ar(value, "joints", "^");

            if (ar.IsInput())
            {
                handle = value.c_str();
            }
        }
    };

    void IKDefinition::RecoilIK::Serialize(IArchive& ar)
    {
        ar(enabled, "enabled", "^^");
        if (!ar.IsEdit() || enabled)
        {
            ar(IKHandleSerializer(leftHandle), "leftHandle", "IK Handle Left");
            ar(IKHandleSerializer(rightHandle), "rightHandle", "IK Handle Right");
            ar(JointName(leftWeaponJoint), "leftWeaponJoint", "Weapon Joint L");
            ar(JointName(rightWeaponJoint), "rightWeaponJoint", "Weapon Joint R");

            if (ar.IsEdit())
            {
                if (IDefaultSkeleton *skeleton = ar.FindContext<IDefaultSkeleton>())
                {
                    auto serializer = [&](IArchive& ar, const char* jointName)
                    {
                        auto it = std::find_if(impactJoints.begin(), impactJoints.end(),
                                    [&](const ImpactJoint& impactJoint)
                                    {
                                        return impactJoint.joint == jointName;
                                    });

                        bool enable = (it != impactJoints.end() && jointName == it->joint);
                        bool previousEnable = enable;

                        ar(enable, "enable", "^^");
                        if (previousEnable)
                        {
                            ar(it->arm, "arm", "^arm");
                            ar(it->delay, "delay", "^delay");
                            ar(it->weight, "weight", "^weight");
                        }

                        if (ar.IsInput() && previousEnable != enable)
                        {
                            if (enable)
                            {
                                impactJoints.emplace_back(jointName);
                            }
                            else
                            {
                                impactJoints.erase(
                                        std::remove_if(impactJoints.begin(), impactJoints.end(),
                                            [&](const ImpactJoint& impactJoint)
                                            {
                                                return impactJoint.joint == jointName;
                                            }),
                                        impactJoints.end());
                            }
                        }
                    };

                    ar(JointTreeSerializer(skeleton, serializer), "impactJoints", "Impact Joints");
                }
            }
            else
            {
                ar(impactJoints, "impactJoints", "Impact Joints");
            }
        }
    }

    void IKDefinition::RecoilIK::Clear()
    {
        enabled = false;
        impactJoints.clear();
    }

    // ---------------------------------------------------------------------------

    IKDefinition::FeetLockIK::FeetLockIK()
        : enabled(false)
    {
    }

    void IKDefinition::FeetLockIK::LoadFromXML(const XmlNodeRef& node)
    {
        if (XmlNodeRef feetLockNode = node->findChild(g_feetLockDefinitionTag))
        {
            enabled = true;

            if (XmlNodeRef rIKHandleNode = feetLockNode->findChild(g_rIKHandleTag))
            {
                rightHandle = rIKHandleNode->getAttr(g_handleAttr);
            }

            if (XmlNodeRef lIKHandleNode = feetLockNode->findChild(g_lIKHandleTag))
            {
                leftHandle = lIKHandleNode->getAttr(g_handleAttr);
            }
        }
    }

    void IKDefinition::FeetLockIK::SaveToXML(const XmlNodeRef& node) const
    {
        if (enabled)
        {
            XmlNodeRef feetLockNode = node->createNode(g_feetLockDefinitionTag);
            node->addChild(feetLockNode);

            XmlNodeRef rIKHandleNode = feetLockNode->createNode(g_rIKHandleTag);
            feetLockNode->addChild(rIKHandleNode);
            rIKHandleNode->setAttr(g_handleAttr, rightHandle);

            XmlNodeRef lIKHandleNode = feetLockNode->createNode(g_lIKHandleTag);
            feetLockNode->addChild(lIKHandleNode);
            lIKHandleNode->setAttr(g_handleAttr, leftHandle);
        }
    }

    void IKDefinition::FeetLockIK::Serialize(IArchive& ar)
    {
        ar(enabled, "enabled", "^^");
        if (!ar.IsEdit() || enabled)
        {
            ar(IKHandleSerializer(leftHandle), "leftHandle", "IK Handle Left");
            ar(IKHandleSerializer(rightHandle), "rightHandle", "IK Handle Right");
        }
    }

    void IKDefinition::FeetLockIK::Clear()
    {
        enabled = false;
    }

    // ---------------------------------------------------------------------------

    void IKDefinition::AnimationDrivenIKTarget::Serialize(IArchive& ar)
    {
        ar(IKHandleSerializer(handle), "handle", "Handle");
        ar(JointName(target), "targetJoint", "Target Joint");
        ar(JointName(weight), "weight", "Weight");
    }

    IKDefinition::AnimationDrivenIKTargetList::AnimationDrivenIKTargetList()
        : enabled(false)
    {
    }

    void IKDefinition::AnimationDrivenIKTargetList::LoadFromXML(const XmlNodeRef& node)
    {
        if (XmlNodeRef targetsNode = node->findChild(g_animationDrivenIKTargetsTag))
        {
            enabled = true;

            for (int i = 0, count = targetsNode->getChildCount(); i < count; ++i)
            {
                XmlNodeRef targetNode = targetsNode->getChild(i);
                if (azstricmp(targetNode->getTag(), g_ADIKTargetTag))
                {
                    continue;
                }

                AnimationDrivenIKTarget target;

                target.handle = targetNode->getAttr(g_handleAttr);
                target.target = targetNode->getAttr(g_targetAttr);
                target.weight = targetNode->getAttr(g_weightAttr);

                targets.push_back(target);
            }
        }
    }

    void IKDefinition::AnimationDrivenIKTargetList::SaveToXML(const XmlNodeRef& node) const
    {
        if (enabled)
        {
            XmlNodeRef targetsNode = node->createNode(g_animationDrivenIKTargetsTag);
            node->addChild(targetsNode);

            for (auto& target : targets)
            {
                XmlNodeRef targetNode = targetsNode->createNode(g_ADIKTargetTag);
                targetsNode->addChild(targetNode);

                targetNode->setAttr(g_handleAttr, target.handle);
                targetNode->setAttr(g_targetAttr, target.target);
                targetNode->setAttr(g_weightAttr, target.weight);
            }
        }
    }

    void IKDefinition::AnimationDrivenIKTargetList::Serialize(IArchive& ar)
    {
        ar(enabled, "enabled", "^^");
        if (!ar.IsEdit() || enabled)
        {
            ar(targets, "targets", "^^");
        }
    }

    void IKDefinition::AnimationDrivenIKTargetList::Clear()
    {
        enabled = false;
        targets.clear();
    }

    // ---------------------------------------------------------------------------

    void IKDefinition::LoadFromXML(const XmlNodeRef& node)
    {
        limbIK.LoadFromXML(node);
        aimIK.LoadFromXML(node);
        lookIK.LoadFromXML(node);
        recoilIK.LoadFromXML(node);
        feetLockIK.LoadFromXML(node);
        animationDrivenIKTargets.LoadFromXML(node);
    }

    void IKDefinition::SaveToXML(const XmlNodeRef& node) const
    {
        XmlNodeRef ikDefinitionNode = node->createNode(g_ikDefinitionTag);
        node->addChild(ikDefinitionNode);

        limbIK.SaveToXML(ikDefinitionNode);
        aimIK.SaveToXML(ikDefinitionNode);
        lookIK.SaveToXML(ikDefinitionNode);
        recoilIK.SaveToXML(ikDefinitionNode);
        feetLockIK.SaveToXML(ikDefinitionNode);
        animationDrivenIKTargets.SaveToXML(ikDefinitionNode);
    }

    void IKDefinition::Serialize(IArchive& ar)
    {
        ar(limbIK, "limbIK", "+Limb IK");
        ar(aimIK, "aimIK", "+Aim IK");
        ar(lookIK, "lookIK", "+Look IK");
        ar(recoilIK, "recoilIK", "+Recoil IK");
        ar(feetLockIK, "feetLockIK", "+Feet Lock IK");
        ar(animationDrivenIKTargets, "animationDrivenIKTargets", "+Animation Driven IK Targets");
    }

    void IKDefinition::Clear()
    {
        limbIK.Clear();
        aimIK.Clear();
        lookIK.Clear();
        recoilIK.Clear();
        feetLockIK.Clear();
        animationDrivenIKTargets.Clear();
    }

    bool IKDefinition::HasEnabledDefinitions() const
    {
        return limbIK.enabled
                || aimIK.enabled
                || lookIK.enabled
                || recoilIK.enabled
                || feetLockIK.enabled
                || animationDrivenIKTargets.enabled;
    }

    // ---------------------------------------------------------------------------

    LODJoints::LODJoints()
    {
    }

    void LODJoints::Serialize(IArchive& ar)
    {
        if (ar.IsEdit())
        {
            if (ar.IsOutput())
            {
                if (auto parent = ar.FindContext<SkeletonParameters>())
                {
                    auto& jointLods = parent->jointLods.levels;
                    auto it = std::find_if(jointLods.begin(), jointLods.end(),
                                [this](const LODJoints& jointList)
                                {
                                    return &jointList == this;
                                });
                    if (it != jointLods.end())
                    {
                        auto index = std::distance(jointLods.begin(), it);

                        string label;
                        label.Format("LOD %d", index + 1);

                        ar(PersistentString(label), "lodLabel", "!^");
                    }
                }
            }

            if (IDefaultSkeleton* skeleton = ar.FindContext<IDefaultSkeleton>())
            {
                JointTreeSerializer(skeleton, JointSetSerializingFunctor(joints, skeleton, true)).Serialize(ar);

                if (ar.IsInput())
                {
                    // disable all dangling joints

                    auto it = joints.begin();

                    while (it != joints.end())
                    {
                        auto& jointName = *it;

                        int parentId = skeleton->GetJointParentIDByID(skeleton->GetJointIDByName(jointName));

                        bool isDangling = false;

                        while (parentId >= 0)
                        {
                            const char* parentName = skeleton->GetJointNameByID(parentId);

                            if (std::find(joints.begin(), joints.end(), parentName) == joints.end())
                            {
                                isDangling = true;
                                break;
                            }

                            parentId = skeleton->GetJointParentIDByID(parentId);
                        }

                        if (isDangling)
                        {
                            it = joints.erase(it);
                        }
                        else
                        {
                            ++it;
                        }
                    }

                    // sort joints so that parents joints precede children joints
                    std::sort(joints.begin(), joints.end(),
                                    [&](const string& jointA, const string& jointB)
                                    {
                                        // joints with lower IDs are always ancestors of joints with higher IDs
                                        return skeleton->GetJointIDByName(jointA) < skeleton->GetJointIDByName(jointB);
                                    });
                }
            }
        }
        else
        {
            ar(joints, "joints", "Joints");
        }
    }

    // ---------------------------------------------------------------------------

    void LODJointsList::LoadFromXML(const XmlNodeRef& node)
    {
        int lodCount = 0;
        levels.resize(s_maxLevels);

        for (int i = 0, count = node->getChildCount(); i < count; ++i)
        {
            XmlNodeRef jointListNode = node->getChild(i);
            if (azstricmp(jointListNode->getTag(), g_jointListTag) != 0)
            {
                continue;
            }

            int level;
            if (jointListNode->getAttr(g_levelAttr, level) && level >= 1 && level <= s_maxLevels)
            {
                lodCount = std::max(lodCount, level);

                auto& lod = levels[level - 1];

                for (int j = 0, count = jointListNode->getChildCount(); j < count; ++j)
                {
                    XmlNodeRef jointNode = jointListNode->getChild(j);
                    if (azstricmp(jointNode->getTag(), g_jointTag) != 0)
                    {
                        continue;
                    }

                    lod.joints.emplace_back(jointNode->getAttr(g_nameAttr));
                }
            }
        }

        levels.resize(lodCount);
    }

    void LODJointsList::SaveToXML(const XmlNodeRef& node) const
    {
        if (levels.size())
        {
            XmlNodeRef lodNode = node->createNode(g_lodTag);
            node->addChild(lodNode);

            int level = 1;

            for (auto& lod : levels)
            {
                if (!lod.joints.empty())
                {
                    XmlNodeRef jointListNode = lodNode->createNode(g_jointListTag);
                    jointListNode->setAttr(g_levelAttr, level);
                    lodNode->addChild(jointListNode);

                    for (auto& joint : lod.joints)
                    {
                        XmlNodeRef jointNode = jointListNode->createNode(g_jointTag);
                        jointListNode->addChild(jointNode);

                        jointNode->setAttr(g_nameAttr, joint.c_str());
                    }

                    ++level;
                }
            }
        }
    }

    void LODJointsList::Serialize(IArchive& ar)
    {
        ar(levels, "lods", "^^");

        if (levels.size() > s_maxLevels)
        {
            ar.Error(levels, "Number of joint LODs should not exceed %d.", s_maxLevels);
        }

        if (ar.IsInput())
        {
            // initialize empty LOD levels

            for (size_t i = 0; i < levels.size(); ++i)
            {
                auto& lod = levels[i];

                if (lod.joints.empty())
                {
                    if (i == 0)
                    {
                        // first level, initialize it with all skeleton joints

                        if (IDefaultSkeleton* skeleton = ar.FindContext<IDefaultSkeleton>())
                        {
                            int numJoints = skeleton->GetJointCount();
                            lod.joints.reserve(numJoints);

                            for (int j = 0; j < numJoints; ++j)
                            {
                                lod.joints.emplace_back(skeleton->GetJointNameByID(j));
                            }

                            std::sort(lod.joints.begin(), lod.joints.end(),
                                        [&](const string& jointA, const string& jointB)
                                        {
                                            return skeleton->GetJointIDByName(jointA) < skeleton->GetJointIDByName(jointB);
                                        });
                        }
                    }
                    else if (i < levels.size() - 1)
                    {
                        // LOD level created with "Insert Before", copy next level

                        lod.joints = levels[i + 1].joints;
                    }
                    else
                    {
                        // last level, created with "Add", copy previous last level

                        lod.joints = levels[i - 1].joints;
                    }
                }
            }
        }
    }

    void LODJointsList::Clear()
    {
        levels.clear();
    }

    // ---------------------------------------------------------------------------

    void SkeletonParameters::Serialize(Serialization::IArchive& ar)
    {
        Serialization::SContext<SkeletonParameters> context(ar, this);
        IDefaultSkeleton* skeleton = gEnv->pCharacterManager->LoadModelSKELUnsafeManualRef(skeletonFileName, CA_CharEditModel);
        Serialization::SContext<IDefaultSkeleton> contextSkeleton(ar, skeleton);

        ar(includes, "includes");
        ar(animationSetFilter, "animationSetFilter");
        ar(animationEventDatabase, "animationEventDatabase");
        ar(faceLibFile, "faceLibFile");

        ar(dbaPath, "dbaPath");
        ar(individualDBAs, "dbas");

        ar(bboxExtension, "bboxExtension");
        ar(bboxIncludes, "bboxIncludes");
        ar(jointLods, "jointLods");
        ar(ikDefinition, "ikDefinition");
    }

    struct XmlUsageTracker
    {
        vector<XmlNodeRef> unusedNodes;

        XmlUsageTracker(const XmlNodeRef& root)
        {
            int numChildren = root->getChildCount();
            for (int i = 0; i < numChildren; ++i)
            {
                unusedNodes.push_back(root->getChild(i));
            }
        }

        XmlNodeRef UseNode(const char* name)
        {
            for (size_t i = 0; i < unusedNodes.size(); ++i)
            {
                if (unusedNodes[i]->isTag(name))
                {
                    XmlNodeRef result = unusedNodes[i];
                    unusedNodes.erase(unusedNodes.begin() + i);
                    return result;
                }
            }
            return XmlNodeRef();
        }
    };

    bool SkeletonParameters::LoadFromXML(XmlNodeRef topRoot, bool* dataLost)
    {
        if (!topRoot)
        {
            return false;
        }

        AnimationFilterFolder* filterFolder = 0;
        animationSetFilter.folders.clear();
        includes.clear();
        individualDBAs.clear();
        bboxIncludes.Clear();
        bboxExtension.Clear();
        jointLods.Clear();
        ikDefinition.Clear();

        XmlUsageTracker xml(topRoot);

        if (XmlNodeRef calNode = xml.UseNode(g_animationListTag))
        {
            uint32 count = calNode->getChildCount();
            for (uint32 i = 0; i < count; ++i)
            {
                XmlNodeRef assignmentNode = calNode->getChild(i);

                if (azstricmp(assignmentNode->getTag(), g_animationTag) != 0)
                {
                    continue;
                }

                const int BITE = 512;
                CryFixedStringT<BITE> line;

                CryFixedStringT<BITE> key;
                CryFixedStringT<BITE> value;

                key.append(assignmentNode->getAttr(g_nameAttr));
                line.append(assignmentNode->getAttr(g_pathAttr));

                int32 pos = 0;
                value = line.Tokenize("\t\n\r=", pos).TrimRight(' ');


                // now only needed for aim poses
                char buffer[BITE] = "";
                azstrcpy(buffer, AZ_ARRAY_SIZE(buffer), line.c_str());

                if (value.empty() || value.at(0) == '?')
                {
                    continue;
                }

                if (0 == azstricmp(key, "#filepath"))
                {
                    animationSetFilter.folders.push_back(AnimationFilterFolder());
                    filterFolder = &animationSetFilter.folders.back();
                    filterFolder->wildcards.clear();
                    filterFolder->path = PathUtil::ToUnixPath(value.c_str());
                    filterFolder->path.TrimRight('/'); // delete the trailing slashes
                    continue;
                }

                if (0 == azstricmp(key, "#ParseSubFolders"))
                {
                    bool parseSubFolders = stricmp (value, "true") == 0 ? true : false; // if it's false, stricmp return 0
                    if (parseSubFolders != true)
                    {
                        if (dataLost)
                        {
                            *dataLost = true;
                        }
                    }
                    continue;
                }

                // remove first '\' and replace '\' with '/'
                value.replace('\\', '/');
                value.TrimLeft('/');

                // process the possible directives
                if (key.c_str()[0] == '$')
                {
                    if (!azstricmp(key, "$AnimationDir") || !azstricmp(key, "$AnimDir") || !azstricmp(key, "$AnimationDirectory") || !azstricmp(key, "$AnimDirectory"))
                    {
                        if (dataLost)
                        {
                            *dataLost = true;
                        }
                    }
                    else if (!azstricmp(key, "$AnimEventDatabase"))
                    {
                        if (animationEventDatabase.empty())
                        {
                            animationEventDatabase = value.c_str();
                        }
                        else
                        {
                            *dataLost = true;
                        }
                    }
                    else if (!azstricmp(key, "$TracksDatabase"))
                    {
                        int wildcardPos = value.find('*');
                        if (wildcardPos != string::npos)
                        {
                            //--- Wildcard include

                            dbaPath = PathUtil::ToUnixPath(value.Left(wildcardPos)).c_str();
                        }
                        else
                        {
                            CryFixedStringT<BITE> flags;
                            flags.append(assignmentNode->getAttr(g_flagsAttr));

                            SkeletonParametersDBA dba;

                            // flag handling
                            if (!flags.empty())
                            {
                                if (strstr(flags.c_str(), "persistent"))
                                {
                                    dba.persistent = true;
                                }
                            }

                            dba.filename = PathUtil::ToUnixPath(value.c_str()).c_str();
                            individualDBAs.push_back(dba);
                        }
                    }
                    else if (!azstricmp(key, "$Include"))
                    {
                        SkeletonParametersInclude include;
                        include.filename = value.c_str();
                        includes.push_back(include);
                    }
                    else if (!azstricmp(key, "$FaceLib"))
                    {
                        faceLibFile = value.c_str();
                    }
                    else
                    {
                        if (dataLost)
                        {
                            *dataLost = true;
                        }
                        //Warning(paramFileName, VALIDATOR_ERROR, "Unknown directive in '%s'", key.c_str());
                    }
                }
                else
                {
                    AnimationFilterWildcard wildcard;
                    wildcard.renameMask = key.c_str();
                    wildcard.fileWildcard = value.c_str();
                    if (!filterFolder)
                    {
                        animationSetFilter.folders.push_back(AnimationFilterFolder());
                        filterFolder = &animationSetFilter.folders.back();
                        filterFolder->wildcards.clear();
                    }
                    filterFolder->wildcards.push_back(wildcard);
                }
            }
        }

        if (XmlNodeRef bboxIncludeListNode = xml.UseNode(g_bboxIncludeListTag))
        {
            auto& joints = bboxIncludes.joints;

            int count = bboxIncludeListNode->getChildCount();
            for (int i = 0; i < count; ++i)
            {
                XmlNodeRef jointNode = bboxIncludeListNode->getChild(i);

                if (azstricmp(jointNode->getTag(), g_jointTag) != 0)
                {
                    continue;
                }

                joints.emplace_back(jointNode->getAttr(g_nameAttr));
            }

            bboxIncludes.m_initialized = true;
        }

        if (XmlNodeRef bboxExtensionNode = xml.UseNode(g_bboxExtensionListTag))
        {
            if (XmlNodeRef axisNode = bboxExtensionNode->findChild(g_axisTag))
            {
                axisNode->getAttr(g_negXAttr, bboxExtension.neg.x);
                axisNode->getAttr(g_negYAttr, bboxExtension.neg.y);
                axisNode->getAttr(g_negZAttr, bboxExtension.neg.z);

                axisNode->getAttr(g_posXAttr, bboxExtension.pos.x);
                axisNode->getAttr(g_posYAttr, bboxExtension.pos.y);
                axisNode->getAttr(g_posZAttr, bboxExtension.pos.z);
            }
        }

        if (XmlNodeRef lodNode = xml.UseNode(g_lodTag))
        {
            jointLods.LoadFromXML(lodNode);
        }

        if (XmlNodeRef ikDefinitionNode = xml.UseNode(g_ikDefinitionTag))
        {
            ikDefinition.LoadFromXML(ikDefinitionNode);
        }

        unknownNodes = xml.unusedNodes;

        return true;
    }

    bool SkeletonParameters::LoadFromXMLFile(const char* filename, bool* dataLost)
    {
        XmlNodeRef root = gEnv->pSystem->LoadXmlFromFile(filename);
        if (!root)
        {
            return false;
        }

        if (!LoadFromXML(root, dataLost))
        {
            return false;
        }

        return true;
    }

    static void AddAnimation(const XmlNodeRef& dest, const char* name, const char* path, const char* flags = 0)
    {
        XmlNodeRef child = dest->createNode(g_animationTag);
        child->setAttr(g_nameAttr, name);
        child->setAttr(g_pathAttr, path);
        if (flags)
        {
            child->setAttr(g_flagsAttr, flags);
        }
        dest->addChild(child);
    }

    XmlNodeRef SkeletonParameters::SaveToXML()
    {
        XmlNodeRef root = gEnv->pSystem->CreateXmlNode(g_paramsTag);

        XmlNodeRef animationList = root->findChild(g_animationListTag);
        if (animationList)
        {
            animationList->removeAllChilds();
        }
        else
        {
            animationList = root->createNode(g_animationListTag);
            root->addChild(animationList);
        }

        for (size_t i = 0; i < includes.size(); ++i)
        {
            const string& filename = includes[i].filename;
            AddAnimation(animationList, "$Include", filename.c_str());
        }

        if (!animationEventDatabase.empty())
        {
            AddAnimation(animationList, "$AnimEventDatabase", animationEventDatabase.c_str());
        }

        if (!dbaPath.empty())
        {
            string path = dbaPath;
            if (!path.empty() && path[path.size() - 1] != '/' && path[path.size() - 1] != '\\')
            {
                path += "/";
            }
            path += "*.dba";
            AddAnimation(animationList, "$TracksDatabase", path.c_str());
        }

        for (size_t i = 0; i < individualDBAs.size(); ++i)
        {
            AddAnimation(animationList, "$TracksDatabase", individualDBAs[i].filename.c_str(), individualDBAs[i].persistent ? "persistent" : 0);
        }

        for (size_t i = 0; i < animationSetFilter.folders.size(); ++i)
        {
            AnimationFilterFolder& folder = animationSetFilter.folders[i];

            AddAnimation(animationList, "#filepath", folder.path);
            for (size_t j = 0; j < folder.wildcards.size(); ++j)
            {
                AnimationFilterWildcard& w = folder.wildcards[j];
                AddAnimation(animationList, w.renameMask.c_str(), w.fileWildcard.c_str());
            }
        }

        if (bboxIncludes.m_initialized)
        {
            XmlNodeRef bboxIncludeListNode = root->createNode(g_bboxIncludeListTag);
            root->addChild(bboxIncludeListNode);

            for (auto& joint : bboxIncludes.joints)
            {
                XmlNodeRef jointNode = bboxIncludeListNode->createNode(g_jointTag);
                bboxIncludeListNode->addChild(jointNode);

                jointNode->setAttr(g_nameAttr, joint.c_str());
            }
        }

        XmlNodeRef bboxExtensionNode = root->createNode(g_bboxExtensionListTag);
        root->addChild(bboxExtensionNode);

        XmlNodeRef axisNode = bboxExtensionNode->createNode(g_axisTag);
        bboxExtensionNode->addChild(axisNode);

        axisNode->setAttr(g_negXAttr, bboxExtension.neg.x);
        axisNode->setAttr(g_negYAttr, bboxExtension.neg.y);
        axisNode->setAttr(g_negZAttr, bboxExtension.neg.z);

        axisNode->setAttr(g_posXAttr, bboxExtension.pos.x);
        axisNode->setAttr(g_posYAttr, bboxExtension.pos.y);
        axisNode->setAttr(g_posZAttr, bboxExtension.pos.z);

        jointLods.SaveToXML(root);

        ikDefinition.SaveToXML(root);

        for (size_t i = 0; i < unknownNodes.size(); ++i)
        {
            root->addChild(unknownNodes[i]);
        }

        return root;
    }

    bool SkeletonParameters::SaveToMemory(vector<char>* buffer)
    {
        XmlNodeRef root = SaveToXML();
        if (!root)
        {
            return false;
        }

        _smart_ptr<IXmlStringData> str(root->getXMLData());
        buffer->assign(str->GetString(), str->GetString() + str->GetStringLength());
        return true;
    }
}
