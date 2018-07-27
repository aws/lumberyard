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
#include "CharacterDefinition.h"
#include "Strings.h"
#include "Serialization.h"
#include "Expected.h"
#include <ICryAnimation.h>
#include <CryExtension/ICryFactory.h>
#include <Serialization/CryExtension.h>
#include <Serialization/CryExtensionImpl.h>
#include "Serialization/IArchiveHost.h"
#include "Serialization/CryName.h"
#include <CryExtension/CryCreateClassInstance.h>


#include <IEditor.h>

namespace CharacterTool
{
    SERIALIZATION_ENUM_BEGIN(AttachmentTypes,  "Attachment Type")
    SERIALIZATION_ENUM(CA_BONE,   "bone",   "Joint Attachment")
    SERIALIZATION_ENUM(CA_FACE,   "face",   "Face Attachment")
    SERIALIZATION_ENUM(CA_SKIN,   "skin",   "Skin Attachment")
    SERIALIZATION_ENUM(CA_PROX,   "prox",   "Proxy Attachment")
    SERIALIZATION_ENUM(CA_PROW,   "prow",   "PRow Attachment")
    SERIALIZATION_ENUM_END()

    SERIALIZATION_ENUM_BEGIN_NESTED(SimulationParams, ClampType,   "Clamp Mode")
    SERIALIZATION_ENUM(SimulationParams::DISABLED,                 "disabled",  "Disabled")
    SERIALIZATION_ENUM(SimulationParams::PENDULUM_CONE,            "cone",      "Pendulum Cone")
    SERIALIZATION_ENUM(SimulationParams::PENDULUM_HINGE_PLANE,     "hinge",     "Pendulum Hinge")
    SERIALIZATION_ENUM(SimulationParams::PENDULUM_HALF_CONE,       "halfCone",  "Pendulum Half Cone")
    SERIALIZATION_ENUM(SimulationParams::SPRING_ELLIPSOID,         "ellipsoid", "Spring Ellipsoid")
    SERIALIZATION_ENUM(SimulationParams::TRANSLATIONAL_PROJECTION, "projection", "Translational Projection")
    SERIALIZATION_ENUM_END()

    SERIALIZATION_ENUM_BEGIN_NESTED(RowSimulationParams, ClampMode,   "Clamp Mode")
    SERIALIZATION_ENUM(RowSimulationParams::PENDULUM_CONE,            "cone",         "Pendulum Cone")
    SERIALIZATION_ENUM(RowSimulationParams::PENDULUM_HINGE_PLANE,     "hinge",        "Pendulum Hinge")
    SERIALIZATION_ENUM(RowSimulationParams::PENDULUM_HALF_CONE,       "halfCone",     "Pendulum Half Cone")
    SERIALIZATION_ENUM(RowSimulationParams::TRANSLATIONAL_PROJECTION, "projection",   "Translational Projection")
    SERIALIZATION_ENUM_END()

    SERIALIZATION_ENUM_BEGIN_NESTED(CharacterAttachment, ProxyPurpose, "Proxy Pupose")
    SERIALIZATION_ENUM(CharacterAttachment::AUXILIARY,      "auxiliary", "Auxiliary")
#if INCLUDE_SECONDARY_ANIMATION_RAGDOLL
    SERIALIZATION_ENUM(CharacterAttachment::RAGDOLL,        "ragdoll",   "Rag Doll")
#endif
    SERIALIZATION_ENUM_END()

    enum ProjectionSelection1
    {
        PS1_NoProjection,
        PS1_ShortarcRotation,
    };

    enum ProjectionSelection2
    {
        PS2_NoProjection,
        PS2_ShortarcRotation,
        PS2_DirectedRotation,
    };

    enum ProjectionSelection3
    {
        PS3_NoProjection, // ACCEPTED_USE
        PS3_ShortvecTranslation, // ACCEPTED_USE
    };

    enum ProjectionSelection4
    {
        PS4_NoProjection, // ACCEPTED_USE
        PS4_ShortvecTranslation, // ACCEPTED_USE
        PS4_DirectedTranslation // ACCEPTED_USE
    };

    SERIALIZATION_ENUM_BEGIN(ProjectionSelection1, "projectionType")
    SERIALIZATION_ENUM(PS1_NoProjection,        "no_projection",        "No Projection")
    SERIALIZATION_ENUM(PS1_ShortarcRotation,    "shortarc_rotation",    "Shortarc Rotation")
    SERIALIZATION_ENUM_END()

    SERIALIZATION_ENUM_BEGIN(ProjectionSelection2, "projectionType")
    SERIALIZATION_ENUM(PS2_NoProjection,        "no_projection",        "No Projection")
    SERIALIZATION_ENUM(PS2_ShortarcRotation,    "shortarc_rotation",    "Shortarc Rotation")
    SERIALIZATION_ENUM(PS2_DirectedRotation,    "directed_rotation",    "Directed Rotation")
    SERIALIZATION_ENUM_END()

    SERIALIZATION_ENUM_BEGIN(ProjectionSelection3, "projectionType")
    SERIALIZATION_ENUM(PS3_NoProjection,        "no_projection",        "No Projection") // ACCEPTED_USE
    SERIALIZATION_ENUM(PS3_ShortvecTranslation, "shortvec_translation", "Shortvec Translation") // ACCEPTED_USE
    SERIALIZATION_ENUM_END()

    SERIALIZATION_ENUM_BEGIN(ProjectionSelection4, "projectionType")
    SERIALIZATION_ENUM(PS4_NoProjection,        "no_projection",        "No Projection") // ACCEPTED_USE
    SERIALIZATION_ENUM(PS4_ShortvecTranslation, "shortvec_translation", "Shortvec Translation") // ACCEPTED_USE
    SERIALIZATION_ENUM(PS4_DirectedTranslation, "directed_translation", "Directed Translation") // ACCEPTED_USE
    SERIALIZATION_ENUM_END()

    SERIALIZATION_ENUM_BEGIN_NESTED(CharacterAttachment, TransformSpace, "Position Space")
    SERIALIZATION_ENUM(CharacterAttachment::SPACE_CHARACTER,  "character", "Relative to Character")
    SERIALIZATION_ENUM(CharacterAttachment::SPACE_JOINT,      "joint",     "Relative to Joint")
    SERIALIZATION_ENUM_END()

    SERIALIZATION_ENUM_BEGIN(AttachmentFlags, "Attachment Flags")
    SERIALIZATION_ENUM(FLAGS_ATTACH_HIDE_ATTACHMENT,         "hide",                    "Hidden                                                                     ")
    SERIALIZATION_ENUM(FLAGS_ATTACH_PHYSICALIZED_RAYS,       "physicalized_rays",       "Physicalized Rays                                                          ")
    SERIALIZATION_ENUM(FLAGS_ATTACH_PHYSICALIZED_COLLISIONS, "physicalized_collisions", "Physicalized Collisions                                                    ")
    SERIALIZATION_ENUM(FLAGS_ATTACH_SW_SKINNING,             "software_skinning",       "Software Skinning                                                          ")
    SERIALIZATION_ENUM(FLAGS_ATTACH_LINEAR_SKINNING,         "Linear_skinning",         "Linear Skinning GPU                                                        ")
    SERIALIZATION_ENUM(FLAGS_ATTACH_MATRIX_SKINNING,         "Matrix_skinning",         "Linear Skinning CPU                                                        ")
    SERIALIZATION_ENUM_END()

    // ---------------------------------------------------------------------------

    struct ProxySet
    {
        DynArray<CCryName>& names;
        CharacterDefinition* definition;
        ProxySet(DynArray<CCryName>& names, CharacterDefinition* definition)
            : names(names)
            , definition(definition)
        {}
    };

    bool Serialize(Serialization::IArchive& ar, ProxySet& set, const char* name, const char* label)
    {
        if (ar.IsEdit() && set.definition)
        {
            if (ar.OpenBlock(name, label))
            {
                DynArray<CCryName> allProxyNames;
                uint32 numAttachments = set.definition->attachments.size();
                allProxyNames.reserve(numAttachments);
                for (uint32 i = 0; i < numAttachments; i++)
                {
                    const CharacterAttachment& att = set.definition->attachments[i];
                    const char* strSocketName = att.m_strSocketName.c_str();
                    if (att.m_attachmentType == CA_PROX)
                    {
                        allProxyNames.push_back(CCryName(strSocketName));
                    }
                }

                uint32 numProxyNames = allProxyNames.size();
                DynArray<bool> arrUsedProxies(numProxyNames, false);

                arrUsedProxies.resize(numProxyNames);
                uint32 numNames = set.names.size();
                for (uint32 crcIndex = 0; crcIndex < numNames; crcIndex++)
                {
                    uint32 nCRC32lower = CCrc32::ComputeLowercase(set.names[crcIndex].c_str());
                    for (uint32 proxyIndex = 0; proxyIndex < numProxyNames; ++proxyIndex)
                    {
                        uint32 nCRC32 = CCrc32::ComputeLowercase(allProxyNames[proxyIndex].c_str());
                        if (nCRC32lower == nCRC32)
                        {
                            arrUsedProxies[proxyIndex] = true;
                        }
                    }
                }

                // make sure names are preserved even if proxies are not there at the moment
                ar(set.names, "__allUsedNames", 0);

                static std::set<string> staticNames;
                for (uint32 proxyIndex = 0; proxyIndex < numProxyNames; ++proxyIndex)
                {
                    const char* proxyName = allProxyNames[proxyIndex].c_str();
                    const string& name = *staticNames.insert(proxyName).first;
                    ar(arrUsedProxies[proxyIndex], name.c_str(), name.c_str());
                }

                for (uint32 proxyIndex = 0; proxyIndex < numProxyNames; proxyIndex++)
                {
                    int index = -1;
                    for (int j = 0; j < set.names.size(); ++j)
                    {
                        if (set.names[j] == allProxyNames[proxyIndex])
                        {
                            index = j;
                        }
                    }

                    if (arrUsedProxies[proxyIndex])
                    {
                        if (index < 0)
                        {
                            set.names.push_back(allProxyNames[proxyIndex]);
                        }
                    }
                    else if (index >= 0)
                    {
                        set.names.erase(index);
                    }
                }
                ar.CloseBlock();
                return true;
            }
            return false;
        }
        else
        {
            return ar(set.names, name, label);
        }
    }



    // ---------------------------------------------------------------------------

    struct SimulationParamsSerializer
    {
        SimulationParams& p;
        SimulationParamsSerializer(SimulationParams& p)
            : p(p) {}

        void Serialize(Serialization::IArchive& ar)
        {
            bool inJoint = false;
            bool hasGeometry = false;
            CharacterAttachment* attachment = ar.FindContext<CharacterAttachment>();
            CharacterDefinition* definition = 0;

            if (attachment)
            {
                inJoint = attachment->m_attachmentType == CA_BONE;
                hasGeometry = !attachment->m_strGeometryFilepath.empty();
                definition = attachment->m_definition;
                if (!definition)
                {
                    if (CharacterDefinition* definitionFromContext = ar.FindContext<CharacterDefinition>())
                    {
                        definition = definitionFromContext;
                    }
                }
            }

            SimulationParams::ClampType ct = p.m_nClampType;
            ar(ct, "clampType", "^");
            p.m_nClampType = ct;
            if (ct == SimulationParams::PENDULUM_CONE || ct == SimulationParams::PENDULUM_HINGE_PLANE || ct == SimulationParams::PENDULUM_HALF_CONE)
            {
                ar(p.m_useRedirect,                           "useRedirect", inJoint ?  "Redirect to Joint                                                " : 0);
                if (!inJoint || p.m_useRedirect || hasGeometry)
                {
                    ar(p.m_useDebugSetup,                       "useDebug",        "Debug Setup                                                           ");
                    ar(p.m_useDebugText,                        "useDebugText",    "Debug Text                                                            ");
                    ar(p.m_useSimulation,                       "useSimulation",   "Activate Simulation                                                   ");
                    ar(Range(p.m_nSimFPS, uint8(10), uint8(255)), "simulationFPS",   "Simulation FPS");

                    ar(p.m_vSimulationAxis,                     "simulationAxis",  "Simulation Axis");
                    ar(Range(p.m_fMass, 0.0001f, 10.0f),          "mass",            "Mass");
                    ar(Range(p.m_fGravity, -90.0f, 90.0f),        "gravity",         "Gravity");
                    ar(Range(p.m_fDamping, 0.0f, 10.0f),          "damping",         "Damping");
                    ar(Range(p.m_fStiffness, 0.0f, 999.0f),       "jointSpring",     "Joint Spring");//in the case of a pendulum its a joint-based force
                    Vec2& vStiffnessTarget = (Vec2&)p.m_vStiffnessTarget;
                    ar(vStiffnessTarget,                         "stiffnessTarget", "Spring Target");

                    ar(Range(p.m_fMaxAngle, 0.0f, 179.0f),        "coneAngle",       "Cone Angle");
                    ar(Range(p.m_vDiskRotation.x, 0.0f, 359.0f),  "hRotation",       "Hinge Rotation");
                    ar(p.m_vPivotOffset,                         "pivotOffset",     "Pivot Offset");

                    ar(p.m_vCapsule, "capsule", "Capsule");
                    p.m_vCapsule.x = clamp_tpl(p.m_vCapsule.x, 0.0f, 2.0f);
                    p.m_vCapsule.y = clamp_tpl(p.m_vCapsule.y, 0.0f, 0.5f);

                    if (ct == SimulationParams::PENDULUM_CONE || ct == SimulationParams::PENDULUM_HALF_CONE)
                    {
                        ar((ProjectionSelection1&)p.m_nProjectionType, "projectionType", "Projection Type");
                    }
                    else
                    {
                        ar((ProjectionSelection2&)p.m_nProjectionType, "projectionType", "Projection Type");
                    }

                    if (p.m_nProjectionType)
                    {
                        ar(ProxySet(p.m_arrProxyNames, definition), "proxyNames", "Available Collision Proxies");
                    }
                }
                else
                {
                    ar.Warning(p.m_useRedirect, "For joint attachments, please specify an attachment containing geometry, or using redirection.");
                }
            }

            if (ct == SimulationParams::SPRING_ELLIPSOID)
            {
                ar(p.m_useRedirect,                             "useRedirect",    inJoint ? "Redirect to Joint                                               " : 0);
                bool hasGeomtryOrRedirect = inJoint ? (hasGeometry || p.m_useRedirect) : hasGeometry;
                if (hasGeomtryOrRedirect)
                {
                    ar(p.m_useDebugSetup,                       "useDebug",       "Debug Setup                                                               ");
                    ar(p.m_useDebugText,                        "useDebugText",   "Debug Text                                                                ");
                    ar(p.m_useSimulation,                       "useSimulation",  "Activate Simulation                                                       ");
                    ar(Range(p.m_nSimFPS, uint8(10), uint8(255)), "simulationFPS",  "Simulation FPS");

                    ar(Range(p.m_fMass, 0.0001f, 10.0f),          "mass",            "Mass");
                    ar(Range(p.m_fGravity, 0.0001f, 99.0f),       "gravity",         "Gravity");
                    ar(Range(p.m_fDamping, 0.0f, 10.0f),          "damping",         "Damping");
                    ar(Range(p.m_fStiffness, 0.0f, 999.0f),       "Stiffness",       "Stiffness");//in the case of a spring its a position-based force
                    ar(p.m_vStiffnessTarget,                     "stiffnessTarget", "Spring Target");

                    ar(Range(p.m_fRadius,   0.0f, 10.0f),        "diskRadius",      "Disk Radius");
                    ar(p.m_vSphereScale,                        "sphereScale",     "Sphere Scale");
                    p.m_vSphereScale.x = clamp_tpl(p.m_vSphereScale.x, 0.0f, 99.0f);
                    p.m_vSphereScale.y = clamp_tpl(p.m_vSphereScale.y, 0.0f, 99.0f);
                    ar(p.m_vDiskRotation,                       "diskRotation",    "Disk Rotation");
                    p.m_vDiskRotation.x = clamp_tpl(p.m_vDiskRotation.x, 0.0f, 359.0f);
                    p.m_vDiskRotation.y = clamp_tpl(p.m_vDiskRotation.y, 0.0f, 359.0f);
                    ar(p.m_vPivotOffset,                         "pivotOffset",     "Pivot Offset");

                    ar((ProjectionSelection3&)p.m_nProjectionType, "projectionType", "Projection Type");
                    if (p.m_nProjectionType)
                    {
                        if (p.m_nProjectionType == ProjectionSelection3::PS3_ShortvecTranslation) // ACCEPTED_USE
                        {
                            ar(p.m_vCapsule.y,         "radius",         "Radius");
                            p.m_vCapsule.x = 0;
                            p.m_vCapsule.y = clamp_tpl(p.m_vCapsule.y, 0.0f, 0.5f);
                        }
                        if (p.m_nProjectionType)
                        {
                            ar(ProxySet(p.m_arrProxyNames, definition), "proxyNames", "Available Collision Proxies");
                        }
                    }
                }
                else
                {
                    ar.Warning(p.m_useRedirect, "Either Redirection or Geometry should be specified for simulation to function.");
                }
            }

            if (ct == SimulationParams::TRANSLATIONAL_PROJECTION)
            {
                ar(p.m_useDebugSetup,     "useDebug",        "Debug Setup                                                           ");
                ar(p.m_useDebugText,      "useDebugText",    "Debug Text                                                            ");
                ar(p.m_useSimulation,     "useProjection",   "Activate Projection                                                   ");
                ar(p.m_vPivotOffset,      "pivotOffset",     "Pivot Offset");

                ar((ProjectionSelection4&)p.m_nProjectionType, "projectionType", "Projection Type");
                if (p.m_nProjectionType)
                {
                    if (p.m_nProjectionType == ProjectionSelection4::PS4_ShortvecTranslation) // ACCEPTED_USE
                    {
                        ar(p.m_vCapsule.y,         "radius",         "Radius");
                        p.m_vCapsule.x = clamp_tpl(p.m_vCapsule.x, 0.0f, 2.0f);
                        p.m_vCapsule.y = clamp_tpl(p.m_vCapsule.y, 0.0f, 0.5f);
                    }
                    else
                    {
                        ar(JointName(p.m_strDirTransJoint), "dirTransJoint", "Directional Translation Joint");
                        uint32 hasJointName = p.m_strDirTransJoint.length();
                        if (hasJointName == 0)
                        {
                            ar(p.m_vSimulationAxis,   "translationAxis", "Translation Axis");
                        }
                        ar(p.m_vCapsule,          "capsule",         "Capsule");
                        p.m_vCapsule.x = clamp_tpl(p.m_vCapsule.x, 0.0f, 2.0f);
                        p.m_vCapsule.y = clamp_tpl(p.m_vCapsule.y, 0.0f, 0.5f);
                    }
                    if (p.m_nProjectionType)
                    {
                        ar(ProxySet(p.m_arrProxyNames, definition), "proxyNames", "Available Collision Proxies");
                    }
                }

                if (inJoint)
                {
                    ICharacterInstance* pICharacterInstance = ar.FindContext<ICharacterInstance>();
                    if (inJoint && pICharacterInstance)
                    {
                        IAttachmentManager* pIAttachmentManager = pICharacterInstance->GetIAttachmentManager();
                        Serialization::StringList types;
                        types.push_back("none");
                        uint32 numFunctions = pIAttachmentManager->GetProcFunctionCount();
                        for (uint32 i = 0; i < numFunctions; i++)
                        {
                            const char* pFunctionName = pIAttachmentManager->GetProcFunctionName(i);
                            if (pFunctionName)
                            {
                                types.push_back(pFunctionName);
                            }
                        }

                        if (p.m_strProcFunction.empty())
                        {
                            p.m_strProcFunction = types[0];
                        }

                        Serialization::StringListValue strProcFunction(types, p.m_strProcFunction.c_str());
                        ar(strProcFunction, "procFunction", "Proc Function");
                        if (ar.IsInput())
                        {
                            if (strProcFunction.index() <= 0)
                            {
                                p.m_strProcFunction.reset();
                            }
                            else
                            {
                                p.m_strProcFunction = strProcFunction.c_str();
                            }
                        }
                    }
                    else
                    {
                        ar(p.m_strProcFunction, "procFunction", "!Proc Function");
                    }
                }
                else
                {
                    ar(p.m_strProcFunction, "procFunction", 0);
                }
            }
            //----------------------------------------------
        }
    };
}

bool Serialize(Serialization::IArchive& ar, SimulationParams& params, const char* name, const char* label)
{
    CharacterTool::SimulationParamsSerializer s(params);
    return ar(s, name, label);
}

namespace CharacterTool
{
    void CharacterAttachment::Serialize(Serialization::IArchive& ar)
    {
        using Serialization::Range;
        using Serialization::LocalToJoint;
        using Serialization::LocalToEntity;
        using Serialization::LocalFrame;

        Serialization::SContext<CharacterAttachment> attachmentContext(ar, this);

        if (ar.IsEdit() && ar.IsOutput())
        {
            ar(m_strSocketName, "inlineName", "!^");
            ar(m_attachmentType, "inlineType", "!>100>^");
        }

        stack_string oldName = m_strSocketName;
        ar(m_strSocketName, "name", "Name");
        if (m_strSocketName.empty())
        {
            ar.Warning(m_strSocketName, "Please specify a name for the attachment.");
        }

        ar(m_attachmentType, "type", "Type");

        stack_string oldGeometry = m_strGeometryFilepath;
        if (m_attachmentType == CA_BONE)
        {
            ar(JointName(m_strJointName), "jointName", "Joint");

            ar(ResourceFilePath(m_strGeometryFilepath, "Geometry", true), "geometry", "<Geometry");
            ar(ResourceFilePath(m_strMaterial, "Material"), "material", "<Material");
            ar(m_viewDistanceMultiplier, "viewDistanceMultiplier", "View Distance Multiplier");

            ar(m_positionSpace, "positionSpace", "Store Position");
            ar(m_rotationSpace, "rotationSpace", "Store Rotation");
            if (ar.IsEdit())
            {
                const char* transformNamesBySpaces[] = { "transform_jj", "transform_jc", "transform_cj", "transform_cc" };
                const char* transformName = transformNamesBySpaces[(int)m_rotationSpace * 2 + (int)m_positionSpace];
                QuatT& abs = m_characterSpacePosition;
                QuatT& rel = m_jointSpacePosition;
                bool rotJointSpace = m_rotationSpace == SPACE_JOINT;
                bool posJointSpace = m_positionSpace == SPACE_JOINT;
                ar(LocalFrame(rotJointSpace ? &rel.q : &abs.q, rotJointSpace ? Serialization::SPACE_SOCKET_RELATIVE_TO_JOINT : Serialization::SPACE_SOCKET_RELATIVE_TO_BINDPOSE,
                        posJointSpace ? &rel.t : &abs.t, posJointSpace ? Serialization::SPACE_SOCKET_RELATIVE_TO_JOINT : Serialization::SPACE_SOCKET_RELATIVE_TO_BINDPOSE,
                        m_strSocketName.c_str(), this),
                    transformName,
                    "+Transform"
                    );
                abs.q.Normalize();
                rel.q.Normalize();
            }
            else
            {
                if (m_rotationSpace == SPACE_JOINT)
                {
                    ar(m_jointSpacePosition.q, "jointRotation", 0);
                }
                else
                {
                    ar(m_characterSpacePosition.q, "characterRotation", 0);
                }
                if (m_positionSpace == SPACE_JOINT)
                {
                    ar(m_jointSpacePosition.t, "jointPosition", 0);
                }
                else
                {
                    ar(m_characterSpacePosition.t, "characterPosition", 0);
                }
            }

            ar(m_simulationParams, "simulation", "+Simulation");

            const uint32 availableAttachmentFlags = FLAGS_ATTACH_HIDE_ATTACHMENT |
                                                    FLAGS_ATTACH_PHYSICALIZED_RAYS |
                                                    FLAGS_ATTACH_PHYSICALIZED_COLLISIONS;

            BitFlags<AttachmentFlags>(m_nFlags, availableAttachmentFlags).Serialize(ar);
            if ((m_nFlags & FLAGS_ATTACH_HIDE_ATTACHMENT) != 0)
            {
                ar.Warning(*this, "Hidden by default.");
            }
        }


        if (m_attachmentType == CA_FACE)
        {
            ar(ResourceFilePath(m_strGeometryFilepath, "Geometry", true), "geometry", "<Geometry");
            ar(ResourceFilePath(m_strMaterial, "Material"), "material", "<Material");
            ar(m_viewDistanceMultiplier, "viewDistanceMultiplier", "View Distance Multiplier");

            if (ar.IsInput())
            {
                m_positionSpace = SPACE_CHARACTER;
                m_rotationSpace = SPACE_CHARACTER;
            }
            QuatT& abs = m_characterSpacePosition;
            ar(LocalFrame(&abs.q, Serialization::SPACE_SOCKET_RELATIVE_TO_BINDPOSE, &abs.t, Serialization::SPACE_SOCKET_RELATIVE_TO_BINDPOSE, m_strSocketName.c_str(), this),
                "characterPosition",
                "+Transform"
                );
            abs.q.Normalize();

            if (ar.IsEdit())
            {
                ar(m_jointSpacePosition, "jointPosition", 0);
            }

            ar(m_simulationParams, "simulation", "+Simulation");
            if (m_simulationParams.m_nClampType == SimulationParams::TRANSLATIONAL_PROJECTION)
            {
                m_simulationParams.m_nClampType = SimulationParams::DISABLED; //you can't choose this mode on face attachments
            }

            const uint32 availableAttachmentFlags = FLAGS_ATTACH_HIDE_ATTACHMENT |
                                                    FLAGS_ATTACH_PHYSICALIZED_RAYS |
                                                    FLAGS_ATTACH_PHYSICALIZED_COLLISIONS;

            BitFlags<AttachmentFlags>(m_nFlags, availableAttachmentFlags).Serialize(ar);
            if ((m_nFlags & FLAGS_ATTACH_HIDE_ATTACHMENT) != 0)
            {
                ar.Warning(*this, "Hidden by default.");
            }
        }


        if (m_attachmentType == CA_SKIN)
        {
            ar(ResourceFilePath(m_strGeometryFilepath, "Skinned Mesh"), "geometry", "<Geometry");
            ar(ResourceFilePath(m_strMaterial, "Material"), "material", "<Material");
            ar(m_viewDistanceMultiplier, "viewDistanceMultiplier", "View Distance Multiplier");

            const uint32 availableAttachmentFlags = FLAGS_ATTACH_HIDE_ATTACHMENT |
                                                    FLAGS_ATTACH_SW_SKINNING |
                                                    FLAGS_ATTACH_LINEAR_SKINNING |
                                                    FLAGS_ATTACH_MATRIX_SKINNING;

            BitFlags<AttachmentFlags>(m_nFlags, availableAttachmentFlags).Serialize(ar);
            if ((m_nFlags & FLAGS_ATTACH_HIDE_ATTACHMENT) != 0)
            {
                ar.Warning(*this, "Hidden by default.");
            }

            ar(m_positionSpace, "positionSpace", 0);
            ar(m_rotationSpace, "rotationSpace", 0);
            ar(m_jointSpacePosition, "relativeJointPosition", 0);
            ar(m_characterSpacePosition, "relativeCharacterPosition", 0);
        }

        if (m_attachmentType == CA_PROX)
        {
            ar(JointName(m_strJointName), "jointName", "Joint");

            ar(m_positionSpace, "positionSpace", "Store Position");
            ar(m_rotationSpace, "rotationSpace", "Store Rotation");
            if (ar.IsEdit())
            {
                const char* transformNamesBySpaces[] = { "transform_jj", "transform_jc", "transform_cj", "transform_cc" };
                const char* transformName = transformNamesBySpaces[(int)m_rotationSpace * 2 + (int)m_positionSpace];
                QuatT& abs = m_characterSpacePosition;
                QuatT& rel = m_jointSpacePosition;
                bool rotJointSpace = m_rotationSpace == SPACE_JOINT;
                bool posJointSpace = m_positionSpace == SPACE_JOINT;
                ar(LocalFrame(rotJointSpace ? &rel.q : &abs.q,    rotJointSpace ? Serialization::SPACE_SOCKET_RELATIVE_TO_JOINT : Serialization::SPACE_SOCKET_RELATIVE_TO_BINDPOSE,
                        posJointSpace ? &rel.t : &abs.t,    posJointSpace ? Serialization::SPACE_SOCKET_RELATIVE_TO_JOINT : Serialization::SPACE_SOCKET_RELATIVE_TO_BINDPOSE,
                        m_strSocketName.c_str(), this), transformName, "+Transform");
                abs.q.Normalize();
                rel.q.Normalize();
            }
            else
            {
                if (m_rotationSpace == SPACE_JOINT)
                {
                    ar(m_jointSpacePosition.q, "jointRotation", 0);
                }
                else
                {
                    ar(m_characterSpacePosition.q, "characterRotation", 0);
                }
                if (m_positionSpace == SPACE_JOINT)
                {
                    ar(m_jointSpacePosition.t, "jointPosition", 0);
                }
                else
                {
                    ar(m_characterSpacePosition.t, "characterPosition", 0);
                }
            }

            ar(m_ProxyPurpose, "proxyPurpose", "Purpose");

            ar(Range(m_ProxyParams.w, 0.0f, 1.0f), "radius", "Radius");
            ar(Range(m_ProxyParams.x, 0.0f, 1.0f), "x-axis", "X-axis");
            ar(Range(m_ProxyParams.y, 0.0f, 1.0f), "y-axis", "Y-axis");
            ar(Range(m_ProxyParams.z, 0.0f, 1.0f), "z-axis", "Z-axis");
        }


        if (m_attachmentType == CA_PROW)
        {
            ar(JointName(m_strRowJointName), "rowJointName", "Row Joint Name");

            ar(m_rowSimulationParams.m_nClampMode, "clampMode", "Clamp Mode");
            ar(m_rowSimulationParams.m_useDebugSetup,                       "useDebug",        "Debug Setup                                                           ");
            ar(m_rowSimulationParams.m_useDebugText,                        "useDebugText",    "Debug Text                                                            ");
            ar(m_rowSimulationParams.m_useSimulation,                       "useSimulation",   "Activate Simulation                                                   ");

            RowSimulationParams::ClampMode ct = m_rowSimulationParams.m_nClampMode;
            if (ct == RowSimulationParams::TRANSLATIONAL_PROJECTION)
            {
                ar((ProjectionSelection4&)m_rowSimulationParams.m_nProjectionType, "projectionType", "Projection Type");
                if (m_rowSimulationParams.m_nProjectionType)
                {
                    if (m_rowSimulationParams.m_nProjectionType == ProjectionSelection4::PS4_ShortvecTranslation) // ACCEPTED_USE
                    {
                        ar(m_rowSimulationParams.m_vCapsule.y,         "radius",         "Radius");
                        m_rowSimulationParams.m_vCapsule.x = 0;
                        m_rowSimulationParams.m_vCapsule.y = clamp_tpl(m_rowSimulationParams.m_vCapsule.y, 0.0f, 0.5f);
                    }
                    else
                    {
                        ar(JointName(m_rowSimulationParams.m_strDirTransJoint), "dirTransJoint", "Directional Translation Joint");
                        uint32 hasJointName = m_rowSimulationParams.m_strDirTransJoint.length();
                        if (hasJointName == 0)
                        {
                            ar(m_rowSimulationParams.m_vTranslationAxis,   "translationAxis", "Translation Axis");
                        }
                        ar(m_rowSimulationParams.m_vCapsule,          "capsule",         "Capsule");
                        m_rowSimulationParams.m_vCapsule.x = clamp_tpl(m_rowSimulationParams.m_vCapsule.x, 0.0f, 2.0f);
                        m_rowSimulationParams.m_vCapsule.y = clamp_tpl(m_rowSimulationParams.m_vCapsule.y, 0.0f, 0.5f);
                    }

                    if (m_rowSimulationParams.m_nProjectionType)
                    {
                        ar(ProxySet(m_rowSimulationParams.m_arrProxyNames, m_definition), "proxyNames", "Available Collision Proxies");
                    }
                }
            }
            else
            {
                ar(Range(m_rowSimulationParams.m_nSimFPS, uint8(10), uint8(255)), "simulationFPS",   "Simulation FPS");

                ar(Range(m_rowSimulationParams.m_fMass, 0.0001f, 10.0f),          "mass",            "Mass");
                ar(Range(m_rowSimulationParams.m_fGravity, -90.0f, 90.0f),        "gravity",         "Gravity");
                ar(Range(m_rowSimulationParams.m_fDamping, 0.0f, 10.0f),          "damping",         "Damping");
                ar(Range(m_rowSimulationParams.m_fJointSpring, 0.0f, 999.0f),       "jointSpring",     "Joint Spring");//in the case of a pendulum its a joint-based force

                ar(Range(m_rowSimulationParams.m_fConeAngle, 0.0f, 179.0f),        "coneAngle",       "Cone Angle");
                ar(m_rowSimulationParams.m_vConeRotation,                        "coneRotation",    "Cone Rotation");
                m_rowSimulationParams.m_vConeRotation.x = clamp_tpl(m_rowSimulationParams.m_vConeRotation.x, -179.0f, +179.0f);
                m_rowSimulationParams.m_vConeRotation.y = clamp_tpl(m_rowSimulationParams.m_vConeRotation.y, -179.0f, +179.0f);
                m_rowSimulationParams.m_vConeRotation.z = clamp_tpl(m_rowSimulationParams.m_vConeRotation.z, -179.0f, +179.0f);
                ar(Range(m_rowSimulationParams.m_fRodLength, 0.1f, 5.0f),          "rodLength",       "Rod Length");
                ar(m_rowSimulationParams.m_vStiffnessTarget,                     "stiffnessTarget", "Spring Target");
                m_rowSimulationParams.m_vStiffnessTarget.x = clamp_tpl(m_rowSimulationParams.m_vStiffnessTarget.x, -179.0f, +179.0f);
                m_rowSimulationParams.m_vStiffnessTarget.y = clamp_tpl(m_rowSimulationParams.m_vStiffnessTarget.y, -179.0f, +179.0f);
                ar(m_rowSimulationParams.m_vTurbulence,                          "turbulenceTarget", "Turbelance");
                m_rowSimulationParams.m_vTurbulence.x = clamp_tpl(m_rowSimulationParams.m_vTurbulence.x, 0.0f, 2.0f);
                m_rowSimulationParams.m_vTurbulence.y = clamp_tpl(m_rowSimulationParams.m_vTurbulence.y, 0.0f, 9.0f);
                ar(Range(m_rowSimulationParams.m_fMaxVelocity, 0.1f, 100.0f),      "maxVelocity",       "Max Velocity");
                ar(m_rowSimulationParams.m_worldSpaceDamping,                  "worldSpaceDamping", "World Space Damping");
                m_rowSimulationParams.m_worldSpaceDamping.x = clamp_tpl(m_rowSimulationParams.m_worldSpaceDamping.x, 0.f, 1.f);
                m_rowSimulationParams.m_worldSpaceDamping.y = clamp_tpl(m_rowSimulationParams.m_worldSpaceDamping.y, 0.f, 1.f);
                m_rowSimulationParams.m_worldSpaceDamping.z = clamp_tpl(m_rowSimulationParams.m_worldSpaceDamping.z, 0.f, 1.f);

                ar(m_rowSimulationParams.m_cycle,                                "cycle",           "Cycle");
                ar(Range(m_rowSimulationParams.m_fStretch, 0.00f, 0.9f),           "stretch",         "Stretch");
                ar(Range(m_rowSimulationParams.m_relaxationLoops, 0u, 20u),        "relaxationLoops", "Relax Loops");

                ar(m_rowSimulationParams.m_vCapsule, "capsule", "Capsule");
                m_rowSimulationParams.m_vCapsule.x = clamp_tpl(m_rowSimulationParams.m_vCapsule.x, 0.0f, 2.0f);
                m_rowSimulationParams.m_vCapsule.y = clamp_tpl(m_rowSimulationParams.m_vCapsule.y, 0.0f, 0.5f);

                if (ct == RowSimulationParams::PENDULUM_CONE || ct == RowSimulationParams::PENDULUM_HALF_CONE)
                {
                    ar((ProjectionSelection1&)m_rowSimulationParams.m_nProjectionType, "projectionType", "Projection Type");
                }
                else
                {
                    ar((ProjectionSelection2&)m_rowSimulationParams.m_nProjectionType, "projectionType", "Projection Type");
                }

                if (m_rowSimulationParams.m_nProjectionType)
                {
                    ar(ProxySet(m_rowSimulationParams.m_arrProxyNames, m_definition), "proxyNames", "Available Collision Proxies");
                }
            }
        }

        // Keep character/joint-space positions in sync. Important to run this with
        // ar.isOutput() as this is the only place when conversion occurs before
        // applying definition to character.
        if (ICharacterInstance* characterInstance = ar.FindContext<ICharacterInstance>())
        {
            const IDefaultSkeleton& defaultSkeleton = characterInstance->GetIDefaultSkeleton();
            int id = defaultSkeleton.GetJointIDByName(m_strJointName.c_str());
            QuatT defaultJointTransform = IDENTITY;
            if (id != -1)
            {
                defaultJointTransform = defaultSkeleton.GetDefaultAbsJointByID(id);
            }
            if (m_rotationSpace == SPACE_JOINT)
            {
                m_characterSpacePosition.q = (defaultJointTransform * m_jointSpacePosition).q;
            }
            else
            {
                m_jointSpacePosition.q = (defaultJointTransform.GetInverted() * m_characterSpacePosition).q;
            }

            m_characterSpacePosition.q.Normalize();
            m_jointSpacePosition.q.Normalize();

            if (m_positionSpace == SPACE_JOINT)
            {
                m_characterSpacePosition.t = (defaultJointTransform * m_jointSpacePosition).t;
            }
            else
            {
                m_jointSpacePosition.t = (defaultJointTransform.GetInverted() * m_characterSpacePosition).t;
            }

            if (m_simulationParams.m_nClampType == SimulationParams::TRANSLATIONAL_PROJECTION)
            {
                m_characterSpacePosition = defaultJointTransform;
                m_jointSpacePosition.SetIdentity();
            }
        }

        if (ar.IsInput())
        {
            // assign attachment name based on geometry file path when geometry is changing, by the name is empty
            if (m_strSocketName.empty() && oldName.empty() && oldGeometry.empty() && !m_strGeometryFilepath.empty())
            {
                string filename = PathUtil::GetFileName(m_strGeometryFilepath.c_str());
                m_strSocketName = m_definition ? m_definition->GenerateUniqueAttachmentName(filename.c_str()) : filename;
            }
        }
    }


    // ---------------------------------------------------------------------------


    void CharacterDefinition::Serialize(Serialization::IArchive& ar)
    {
        Serialization::SContext<CharacterDefinition> definitionContext(ar, this);

        ar(SkeletonPath(skeleton), "skeleton", "Skeleton");
        if (skeleton.empty())
        {
            ar.Warning(skeleton, "A skeleton is required for every character.");
        }
        ar(MaterialPath(materialPath), "Material");
        // evgenya: .rig and .phys references are temporarily disabled
        ar(CharacterPhysicsPath(physics), "physics", 0);
        ar(CharacterRigPath(rig), "rig", 0);
        ar(attachments, "attachments", "Attachments");
        m_initialized = ar.FindContext<ICharacterInstance>() != 0;

        if (ar.IsInput())
        {
            for (size_t i = 0; i < attachments.size(); ++i)
            {
                attachments[i].m_definition = this;
            }
        }

        if (attachments.empty())
        {
            ar.Warning(attachments, "No attachments listed. Add an attachment to create character geometry.");
        }
#ifdef ENABLE_RUNTIME_POSE_MODIFIERS
        if (ar.IsInput() && !modifiers)
        {
            CryCreateClassInstance("PoseModifierSetup", modifiers);
        }
        if (modifiers)
        {
            modifiers->Serialize(ar);
        }
#endif
    }

    bool CharacterDefinition::LoadFromXmlFile(const char* filename)
    {
        XmlNodeRef root = GetIEditor()->GetSystem()->LoadXmlFromFile(filename);
        if (root == 0)
        {
            return false;
        }

        return LoadFromXml(root);
    }

    bool CharacterDefinition::LoadFromXml(const XmlNodeRef& root)
    {
        m_initialized = false;
        uint32 numChilds = root->getChildCount();
        if (numChilds == 0)
        {
            return false;
        }

        struct tagname
        {
            uint32 m_crc32;
            AttachmentTypes m_type;
            bool m_dsetup;
            bool m_dtext;
            bool m_active;
            tagname(uint32 crc32, AttachmentTypes type, bool dsetup, bool dtext, bool active)
            {
                m_crc32  = crc32;
                ;
                m_type   = type;
                m_dsetup = dsetup;
                m_dtext  = dtext;
                m_active = active;
            }
        };
        DynArray<tagname> arrDebugSetup;
        uint32 numAttachments = attachments.size();
        for (uint32 i = 0; i < numAttachments; i++)
        {
            bool dsetup = attachments[i].m_simulationParams.m_useDebugSetup;
            bool dtext  = attachments[i].m_simulationParams.m_useDebugText;
            bool dsim   = attachments[i].m_simulationParams.m_useSimulation;
            if (dsetup || dtext || dsim == 0)
            {
                uint32 crc32 = CCrc32::ComputeLowercase(attachments[i].m_strSocketName.c_str());
                AttachmentTypes type = attachments[i].m_attachmentType;
                arrDebugSetup.push_back(tagname(crc32, type, dsetup, dtext, dsim));
            }
        }

        //------------------------------------------------------------------------------------------------
        //------------------------------------------------------------------------------------------------
        //------------------------------------------------------------------------------------------------

        for (uint32 xmlnode = 0; xmlnode < numChilds; xmlnode++)
        {
            XmlNodeRef node = root->getChild(xmlnode);
            const char* tag = node->getTag();

            //-----------------------------------------------------------
            //load base model
            //-----------------------------------------------------------
            if (strcmp(tag, "Model") == 0)
            {
                skeleton = node->getAttr("File");
                materialPath = node->getAttr("Material");
                physics = node->getAttr("Physics");
                rig = node->getAttr("Rig File");
            }

            //-----------------------------------------------------------
            //load attachment-list
            //-----------------------------------------------------------
            if (strcmp(tag, "AttachmentList") == 0)
            {
                XmlNodeRef nodeAttachements = node;
                uint32 num = nodeAttachements->getChildCount();
                attachments.clear();
                attachments.reserve(num);

                for (uint32 i = 0; i < num; i++)
                {
                    CharacterAttachment attach;
                    XmlNodeRef nodeAttach = nodeAttachements->getChild(i);
                    const char* AttachTag = nodeAttach->getTag();
                    if (strcmp(AttachTag, "Attachment"))
                    {
                        continue; //invalid
                    }
                    stack_string Type = nodeAttach->getAttr("Type");
                    attach.m_attachmentType = CA_Invalid;
                    if (Type == "CA_BONE")
                    {
                        attach.m_attachmentType = CA_BONE;
                    }
                    if (Type == "CA_FACE")
                    {
                        attach.m_attachmentType = CA_FACE;
                    }
                    if (Type == "CA_SKIN")
                    {
                        attach.m_attachmentType = CA_SKIN;
                    }
                    if (Type == "CA_PROX")
                    {
                        attach.m_attachmentType = CA_PROX;
                    }
                    if (Type == "CA_PROW")
                    {
                        attach.m_attachmentType = CA_PROW;
                    }
                    if (Type == "CA_VCLOTH")
                    {
                        attach.m_attachmentType = CA_VCLOTH;
                    }

                    attach.m_strSocketName = nodeAttach->getAttr("AName");

                    QuatT& abstransform =  attach.m_characterSpacePosition;
                    nodeAttach->getAttr("Rotation", abstransform.q);
                    nodeAttach->getAttr("Position", abstransform.t);

                    QuatT& reltransform =  attach.m_jointSpacePosition;
                    attach.m_rotationSpace = nodeAttach->getAttr("RelRotation", reltransform.q) ? attach.SPACE_JOINT : attach.SPACE_CHARACTER;
                    attach.m_positionSpace = nodeAttach->getAttr("RelPosition", reltransform.t) ? attach.SPACE_JOINT : attach.SPACE_CHARACTER;

                    attach.m_strJointName = nodeAttach->getAttr("BoneName");
                    attach.m_strGeometryFilepath = nodeAttach->getAttr("Binding");
                    attach.m_strMaterial = nodeAttach->getAttr("Material");

                    nodeAttach->getAttr("ProxyParams", attach.m_ProxyParams);
                    uint32 nProxyPurpose = 0;
                    nodeAttach->getAttr("ProxyPurpose", nProxyPurpose), attach.m_ProxyPurpose = CharacterAttachment::ProxyPurpose(nProxyPurpose);


                    SimulationParams::ClampType ct = SimulationParams::DISABLED;
                    nodeAttach->getAttr("PA_PendulumType", (int&)ct);
                    if (ct == SimulationParams::PENDULUM_CONE || ct == SimulationParams::PENDULUM_HINGE_PLANE || ct == SimulationParams::PENDULUM_HALF_CONE)
                    {
                        attach.m_simulationParams.m_nClampType = ct;
                        nodeAttach->getAttr("PA_FPS",             attach.m_simulationParams.m_nSimFPS);
                        nodeAttach->getAttr("PA_Redirect",        attach.m_simulationParams.m_useRedirect);

                        nodeAttach->getAttr("PA_MaxAngle",        attach.m_simulationParams.m_fMaxAngle);
                        nodeAttach->getAttr("PA_HRotation",       attach.m_simulationParams.m_vDiskRotation.x);

                        nodeAttach->getAttr("PA_Mass",            attach.m_simulationParams.m_fMass);
                        nodeAttach->getAttr("PA_Gravity",         attach.m_simulationParams.m_fGravity);
                        nodeAttach->getAttr("PA_Damping",         attach.m_simulationParams.m_fDamping);
                        nodeAttach->getAttr("PA_Stiffness",       attach.m_simulationParams.m_fStiffness);

                        nodeAttach->getAttr("PA_PivotOffset",     attach.m_simulationParams.m_vPivotOffset);
                        nodeAttach->getAttr("PA_SimulationAxis",  attach.m_simulationParams.m_vSimulationAxis);
                        nodeAttach->getAttr("PA_StiffnessTarget", attach.m_simulationParams.m_vStiffnessTarget);

                        nodeAttach->getAttr("PA_ProjectionType",  attach.m_simulationParams.m_nProjectionType);
                        nodeAttach->getAttr("PA_CapsuleX",        attach.m_simulationParams.m_vCapsule.x);
                        nodeAttach->getAttr("PA_CapsuleY",        attach.m_simulationParams.m_vCapsule.y);
                        attach.m_simulationParams.m_strDirTransJoint = nodeAttach->getAttr("PA_DirTransJointName");
                        uint32 IsIdentical = azstricmp(attach.m_simulationParams.m_strDirTransJoint.c_str(), attach.m_strJointName.c_str()) == 0;
                        if (attach.m_simulationParams.m_strDirTransJoint.length() && IsIdentical)
                        {
                            attach.m_simulationParams.m_strDirTransJoint.reset();
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
                            attach.m_simulationParams.m_arrProxyNames.push_back(strProxyName);
                        }
                    }

                    uint32 isSpring = 0;
                    nodeAttach->getAttr("SA_SpringType", isSpring);
                    if (isSpring)
                    {
                        attach.m_simulationParams.m_nClampType = SimulationParams::SPRING_ELLIPSOID;
                        nodeAttach->getAttr("SA_FPS",             attach.m_simulationParams.m_nSimFPS);

                        nodeAttach->getAttr("SA_Radius",          attach.m_simulationParams.m_fRadius);
                        nodeAttach->getAttr("SA_ScaleZP",         attach.m_simulationParams.m_vSphereScale.x);
                        nodeAttach->getAttr("SA_ScaleZN",         attach.m_simulationParams.m_vSphereScale.y);
                        nodeAttach->getAttr("SA_DiskRotX",        attach.m_simulationParams.m_vDiskRotation.x);
                        nodeAttach->getAttr("SA_DiskRotZ",        attach.m_simulationParams.m_vDiskRotation.y);
                        nodeAttach->getAttr("SA_HRotation",       attach.m_simulationParams.m_vDiskRotation.x);//just for backwards compatibility

                        nodeAttach->getAttr("SA_Redirect",        attach.m_simulationParams.m_useRedirect);

                        nodeAttach->getAttr("SA_Mass",            attach.m_simulationParams.m_fMass);
                        nodeAttach->getAttr("SA_Gravity",         attach.m_simulationParams.m_fGravity);
                        nodeAttach->getAttr("SA_Damping",         attach.m_simulationParams.m_fDamping);
                        nodeAttach->getAttr("SA_Stiffness",       attach.m_simulationParams.m_fStiffness);

                        nodeAttach->getAttr("SA_PivotOffset",     attach.m_simulationParams.m_vPivotOffset);
                        nodeAttach->getAttr("SA_StiffnessTarget", attach.m_simulationParams.m_vStiffnessTarget);

                        nodeAttach->getAttr("SA_ProjectionType",  attach.m_simulationParams.m_nProjectionType);
                        attach.m_simulationParams.m_vCapsule.x = 0;
                        nodeAttach->getAttr("SA_CapsuleY",        attach.m_simulationParams.m_vCapsule.y);
                        char proxytag[] = "SA_Proxy00";
                        for (uint32 i = 0; i < SimulationParams::MaxCollisionProxies; i++)
                        {
                            CCryName strProxyName = CCryName(nodeAttach->getAttr(proxytag));
                            proxytag[9]++;
                            if (strProxyName.empty())
                            {
                                continue;
                            }
                            attach.m_simulationParams.m_arrProxyNames.push_back(strProxyName);
                        }
                    }

                    uint32 isProjection = 0;
                    nodeAttach->getAttr("P_Projection", isProjection);
                    if (isProjection)
                    {
                        attach.m_simulationParams.m_nClampType = SimulationParams::TRANSLATIONAL_PROJECTION;
                        attach.m_simulationParams.m_useRedirect = 1;
                        nodeAttach->getAttr("P_ProjectionType",  attach.m_simulationParams.m_nProjectionType);

                        attach.m_simulationParams.m_strDirTransJoint = nodeAttach->getAttr("P_DirTransJointName");
                        uint32 IsIdentical = azstricmp(attach.m_simulationParams.m_strDirTransJoint.c_str(), attach.m_strJointName.c_str()) == 0;
                        if (attach.m_simulationParams.m_strDirTransJoint.length() && IsIdentical)
                        {
                            attach.m_simulationParams.m_strDirTransJoint.reset();
                        }
                        nodeAttach->getAttr("P_TranslationAxis",  attach.m_simulationParams.m_vSimulationAxis);

                        nodeAttach->getAttr("P_CapsuleX",        attach.m_simulationParams.m_vCapsule.x);
                        nodeAttach->getAttr("P_CapsuleY",        attach.m_simulationParams.m_vCapsule.y);

                        nodeAttach->getAttr("P_PivotOffset",     attach.m_simulationParams.m_vPivotOffset);

                        char proxytag[] = "P_Proxy00";
                        for (uint32 i = 0; i < SimulationParams::MaxCollisionProxies; i++)
                        {
                            CCryName strProxyName = CCryName(nodeAttach->getAttr(proxytag));
                            proxytag[8]++;
                            if (strProxyName.empty())
                            {
                                continue;
                            }
                            attach.m_simulationParams.m_arrProxyNames.push_back(strProxyName);
                        }
                    }

                    const char* strProcFunction = nodeAttach->getAttr("ProcFunction");
                    if (strProcFunction && strProcFunction[0])
                    {
                        attach.m_simulationParams.m_strProcFunction = strProcFunction;
                    }

                    if (Type == "CA_PROW")
                    {
                        attach.m_strRowJointName = nodeAttach->getAttr("RowJointName");

                        uint32 nPendulumClampMode = 0;
                        uint32 isPendulum = nodeAttach->getAttr("ROW_ClampMode", nPendulumClampMode);
                        if (isPendulum)
                        {
                            attach.m_rowSimulationParams.m_nClampMode = RowSimulationParams::ClampMode(nPendulumClampMode);

                            nodeAttach->getAttr("ROW_FPS",                  attach.m_rowSimulationParams.m_nSimFPS);
                            nodeAttach->getAttr("ROW_ConeAngle",            attach.m_rowSimulationParams.m_fConeAngle);
                            nodeAttach->getAttr("ROW_ConeRotation",         attach.m_rowSimulationParams.m_vConeRotation);

                            nodeAttach->getAttr("ROW_Mass",                 attach.m_rowSimulationParams.m_fMass);
                            nodeAttach->getAttr("ROW_Gravity",              attach.m_rowSimulationParams.m_fGravity);
                            nodeAttach->getAttr("ROW_Damping",              attach.m_rowSimulationParams.m_fDamping);
                            nodeAttach->getAttr("ROW_JointSpring",          attach.m_rowSimulationParams.m_fJointSpring);
                            nodeAttach->getAttr("ROW_RodLength",            attach.m_rowSimulationParams.m_fRodLength);
                            nodeAttach->getAttr("ROW_StiffnessTarget",      attach.m_rowSimulationParams.m_vStiffnessTarget);
                            nodeAttach->getAttr("ROW_Turbulence",           attach.m_rowSimulationParams.m_vTurbulence);
                            nodeAttach->getAttr("ROW_MaxVelocity",          attach.m_rowSimulationParams.m_fMaxVelocity);
                            nodeAttach->getAttr("ROW_WorldSpaceDamping",    attach.m_rowSimulationParams.m_worldSpaceDamping);

                            nodeAttach->getAttr("ROW_Cycle",                attach.m_rowSimulationParams.m_cycle);
                            nodeAttach->getAttr("ROW_RelaxLoops",           attach.m_rowSimulationParams.m_relaxationLoops);
                            nodeAttach->getAttr("ROW_Stretch",              attach.m_rowSimulationParams.m_fStretch);

                            nodeAttach->getAttr("ROW_ProjectionType",       attach.m_rowSimulationParams.m_nProjectionType);
                            nodeAttach->getAttr("ROW_CapsuleX",             attach.m_rowSimulationParams.m_vCapsule.x);
                            nodeAttach->getAttr("ROW_CapsuleY",             attach.m_rowSimulationParams.m_vCapsule.y);

                            char proxytag[] = "ROW_Proxy00";
                            for (uint32 i = 0; i < SimulationParams::MaxCollisionProxies; i++)
                            {
                                CCryName strProxyName = CCryName(nodeAttach->getAttr(proxytag));
                                proxytag[10]++;
                                if (strProxyName.empty())
                                {
                                    continue;
                                }
                                attach.m_rowSimulationParams.m_arrProxyNames.push_back(strProxyName);
                            }
                        }
                    }

                    if (Type == "CA_VCLOTH")
                    {
                        nodeAttach->getAttr("hide",                  attach.m_vclothParams.hide);
                        //nodeAttach->getAttr( "debug",               attach.m_vclothParams.debug );
                        nodeAttach->getAttr("thickness",             attach.m_vclothParams.thickness);
                        nodeAttach->getAttr("collsionDamping",       attach.m_vclothParams.collisionDamping);
                        //nodeAttach->getAttr( "SA_Mass",             attach.m_vclothParams.dragDamping );
                        nodeAttach->getAttr("stretchStiffness",      attach.m_vclothParams.stretchStiffness);
                        nodeAttach->getAttr("shearStiffness",        attach.m_vclothParams.shearStiffness);
                        nodeAttach->getAttr("bendStiffness",         attach.m_vclothParams.bendStiffness);
                        nodeAttach->getAttr("numIterations",         attach.m_vclothParams.numIterations);
                        nodeAttach->getAttr("timeStep",              attach.m_vclothParams.timeStep);
                        nodeAttach->getAttr("rigidDamping",          attach.m_vclothParams.rigidDamping);
                        nodeAttach->getAttr("translationBlend",      attach.m_vclothParams.translationBlend);
                        nodeAttach->getAttr("rotationBlend",         attach.m_vclothParams.rotationBlend);
                        nodeAttach->getAttr("friction",              attach.m_vclothParams.friction);
                        nodeAttach->getAttr("pullStiffness",         attach.m_vclothParams.pullStiffness);
                        nodeAttach->getAttr("SA_Mass",               attach.m_vclothParams.tolerance);
                        nodeAttach->getAttr("maxBlendWeight",        attach.m_vclothParams.maxBlendWeight);
                        nodeAttach->getAttr("maxAnimDistance",       attach.m_vclothParams.maxAnimDistance);
                        //nodeAttach->getAttr( "SA_Mass",             attach.m_vclothParams.windBlend );
                        //nodeAttach->getAttr( "SA_Mass",             attach.m_vclothParams.collDampingRange );
                        nodeAttach->getAttr("stiffnessGradient",     attach.m_vclothParams.stiffnessGradient);
                        nodeAttach->getAttr("halfStretchIterations", attach.m_vclothParams.halfStretchIterations);
                        nodeAttach->getAttr("isMainCharacter",       attach.m_vclothParams.isMainCharacter);

                        attach.m_vclothParams.simMeshName                = nodeAttach->getAttr("simMeshName");
                        attach.m_vclothParams.renderMeshName             = nodeAttach->getAttr("renderMeshName");
                        attach.m_vclothParams.simBinding                 = nodeAttach->getAttr("SimBinding");
                        attach.m_vclothParams.renderBinding              = nodeAttach->getAttr("Binding");
                    }

                    uint32 flags;
                    if (nodeAttach->getAttr("Flags", flags))
                    {
                        attach.m_nFlags = flags;
                    }
                    nodeAttach->getAttr("ViewDistRatio", attach.m_viewDistanceMultiplier);

                    attach.m_definition = this;
                    attachments.push_back(attach);
                }
            }
        }

        //------------------------------------------------------------------------------------------------
        //------------------------------------------------------------------------------------------------
        //------------------------------------------------------------------------------------------------
        uint32 numDebugSetup = arrDebugSetup.size();
        for (uint32 d = 0; d < numDebugSetup; d++)
        {
            uint32 _crc32         = arrDebugSetup[d].m_crc32;
            AttachmentTypes _type = arrDebugSetup[d].m_type;
            uint32 numAttachments = attachments.size();
            for (uint32 i = 0; i < numAttachments; i++)
            {
                uint32 crc32 = CCrc32::ComputeLowercase(attachments[i].m_strSocketName.c_str());
                AttachmentTypes type = attachments[i].m_attachmentType;
                if (crc32 == _crc32 && type == _type)
                {
                    attachments[i].m_simulationParams.m_useDebugSetup = arrDebugSetup[d].m_dsetup;
                    attachments[i].m_simulationParams.m_useDebugText  = arrDebugSetup[d].m_dtext;
                    attachments[i].m_simulationParams.m_useSimulation = arrDebugSetup[d].m_active;
                }
            }
        }

#ifdef ENABLE_RUNTIME_POSE_MODIFIERS
        if (CryCreateClassInstance("PoseModifierSetup", modifiers))
        {
            if (!Serialization::LoadXmlNode(*modifiers, root))
            {
                return false;
            }
        }
#endif

        return true;
    }

    XmlNodeRef CharacterDefinition::SaveToXml()
    {
        XmlNodeRef root = GetIEditor()->GetSystem()->CreateXmlNode("CharacterDefinition");

        {
            XmlNodeRef node = root->newChild("Model");
            //-----------------------------------------------------------
            // load base model
            //-----------------------------------------------------------
            node->setAttr("File", skeleton);

            if (!materialPath.empty())
            {
                node->setAttr("Material", materialPath);
            }
            else
            {
                node->delAttr("Material");
            }

            if (!physics.empty())
            {
                node->setAttr("Physics", physics.c_str());
            }
            else
            {
                node->delAttr("Physics");
            }

            if (!rig.empty())
            {
                node->setAttr("Rig", rig.c_str());
            }
            else
            {
                node->delAttr("Rig");
            }

            // We don't currently modify these in the editor...

            // node->getAttr( "KeepModelsInMemory",def.m_nKeepModelsInMemory );
        }

        //-----------------------------------------------------------
        // load attachment-list
        //-----------------------------------------------------------
        XmlNodeRef nodeAttachments = root->newChild("AttachmentList");
        const uint32 numAttachments = nodeAttachments->getChildCount();

        // clear out the existing attachments
        while (XmlNodeRef childNode = nodeAttachments->findChild("Attachment"))
        {
            nodeAttachments->removeChild(childNode);
        }


        //don't save names for not existing proxies
        uint32 numAttachmentsInList = attachments.size();
        for (uint32 x = 0; x < numAttachmentsInList; x++)
        {
            DynArray<uint8> arrProxies;
            arrProxies.resize(numAttachmentsInList);
            memset(&arrProxies[0], 0, numAttachmentsInList);
            for (uint32 pn = 0; pn < attachments[x].m_simulationParams.m_arrProxyNames.size(); pn++)
            {
                const char* strProxyName = attachments[x].m_simulationParams.m_arrProxyNames[pn].c_str();
                for (uint32 a = 0; a < numAttachmentsInList; a++)
                {
                    const char* strSocketName = attachments[a].m_strSocketName.c_str();
                    if (attachments[a].m_attachmentType == CA_PROX && azstricmp(strSocketName, strProxyName) == 0)
                    {
                        arrProxies[a] = 1;
                        break;
                    }
                }
            }
            attachments[x].m_simulationParams.m_arrProxyNames.resize(0);
            for (uint32 n = 0; n < numAttachmentsInList; n++)
            {
                if (arrProxies[n])
                {
                    attachments[x].m_simulationParams.m_arrProxyNames.push_back(CCryName(attachments[n].m_strSocketName.c_str()));
                }
            }

            memset(&arrProxies[0], 0, numAttachmentsInList);
            for (uint32 pn = 0; pn < attachments[x].m_rowSimulationParams.m_arrProxyNames.size(); pn++)
            {
                const char* strProxyName = attachments[x].m_rowSimulationParams.m_arrProxyNames[pn].c_str();
                for (uint32 a = 0; a < numAttachmentsInList; a++)
                {
                    const char* strSocketName = attachments[a].m_strSocketName.c_str();
                    if (attachments[a].m_attachmentType == CA_PROX && azstricmp(strSocketName, strProxyName) == 0)
                    {
                        arrProxies[a] = 1;
                        break;
                    }
                }
            }
            attachments[x].m_rowSimulationParams.m_arrProxyNames.resize(0);
            for (uint32 n = 0; n < numAttachmentsInList; n++)
            {
                if (arrProxies[n])
                {
                    attachments[x].m_rowSimulationParams.m_arrProxyNames.push_back(CCryName(attachments[n].m_strSocketName.c_str()));
                }
            }
        }

        for (vector<CharacterAttachment>::const_iterator iter = attachments.begin(), itEnd = attachments.end(); iter != itEnd; ++iter)
        {
            const CharacterAttachment& attach = (*iter);
            if (attach.m_attachmentType == CA_BONE)
            {
                ExportBoneAttachment(attach, nodeAttachments);
            }
            if (attach.m_attachmentType == CA_FACE)
            {
                ExportFaceAttachment(attach, nodeAttachments);
            }
            if (attach.m_attachmentType == CA_SKIN)
            {
                ExportSkinAttachment(attach, nodeAttachments);
            }
            if (attach.m_attachmentType == CA_PROX)
            {
                ExportProxyAttachment(attach, nodeAttachments);
            }
            if (attach.m_attachmentType == CA_PROW)
            {
                ExportPClothAttachment(attach, nodeAttachments);
            }
            if (attach.m_attachmentType == CA_VCLOTH)
            {
                ExportVClothAttachment(attach, nodeAttachments);
            }
        }


        if (XmlNodeRef node = root->findChild("Modifiers"))
        {
            root->removeChild(node);
        }

#ifdef ENABLE_RUNTIME_POSE_MODIFIERS
        if (modifiers)
        {
            Serialization::SaveXmlNode(root, *modifiers);
        }
#endif
        // make sure we don't leave empty modifiers block
        XmlNodeRef modifiersNode = root->findChild("Modifiers");
        if (modifiersNode && modifiersNode->getChildCount() == 0 && modifiersNode->getNumAttributes() == 0)
        {
            root->removeChild(modifiersNode);
        }

        return root;
    }


    void CharacterDefinition::ExportBoneAttachment(const CharacterAttachment& attach, XmlNodeRef nodeAttachments)
    {
        XmlNodeRef nodeAttach = gEnv->pSystem->CreateXmlNode("Attachment");
        nodeAttachments->addChild(nodeAttach);

        nodeAttach->setAttr("Type", "CA_BONE");

        if (!attach.m_strSocketName.empty())
        {
            nodeAttach->setAttr("AName", attach.m_strSocketName);
        }

        if (!attach.m_strGeometryFilepath.empty())
        {
            nodeAttach->setAttr("Binding", attach.m_strGeometryFilepath);
        }
        else
        {
            nodeAttach->delAttr("Binding");
        }

        if (!attach.m_strMaterial.empty())
        {
            nodeAttach->setAttr("Material", attach.m_strMaterial);
        }

        const QuatT& reltransform = attach.m_jointSpacePosition;
        const QuatT& abstransform = attach.m_characterSpacePosition;

        if (attach.m_rotationSpace == CharacterAttachment::SPACE_JOINT)
        {
            Quat identity(IDENTITY);
            if (fabs_tpl(reltransform.q.w) > 0.99999f)
            {
                nodeAttach->setAttr("RelRotation", identity);
            }
            else
            {
                nodeAttach->setAttr("RelRotation", reltransform.q);
            }
        }
        else
        {
            nodeAttach->setAttr("Rotation", abstransform.q);
        }

        if (attach.m_positionSpace == CharacterAttachment::SPACE_JOINT)
        {
            nodeAttach->setAttr("RelPosition", reltransform.t);
        }
        else
        {
            nodeAttach->setAttr("Position", abstransform.t);
        }

        if (!attach.m_strJointName.empty())
        {
            nodeAttach->setAttr("BoneName", attach.m_strJointName);
        }
        else
        {
            nodeAttach->delAttr("BoneName");
        }

        ExportSimulation(attach, nodeAttach);

        if (attach.m_simulationParams.m_strProcFunction.length())
        {
            nodeAttach->setAttr("ProcFunction", attach.m_simulationParams.m_strProcFunction.c_str());
        }

        nodeAttach->setAttr("Flags", attach.m_nFlags);

        if (attach.m_viewDistanceMultiplier != 1.0f)
        {
            nodeAttach->setAttr("ViewDistRatio", attach.m_viewDistanceMultiplier);
        }
        else
        {
            nodeAttach->delAttr("ViewDistRatio");
        }
    }



    void CharacterDefinition::ExportFaceAttachment(const CharacterAttachment& attach, XmlNodeRef nodeAttachments)
    {
        XmlNodeRef nodeAttach = gEnv->pSystem->CreateXmlNode("Attachment");
        nodeAttachments->addChild(nodeAttach);

        nodeAttach->setAttr("Type", "CA_FACE");

        if (!attach.m_strSocketName.empty())
        {
            nodeAttach->setAttr("AName", attach.m_strSocketName);
        }

        if (!attach.m_strGeometryFilepath.empty())
        {
            nodeAttach->setAttr("Binding", attach.m_strGeometryFilepath);
        }
        else
        {
            nodeAttach->delAttr("Binding");
        }

        if (!attach.m_strMaterial.empty())
        {
            nodeAttach->setAttr("Material", attach.m_strMaterial);
        }


        const QuatT& transform = attach.m_characterSpacePosition;
        nodeAttach->setAttr("Rotation", transform.q);
        nodeAttach->setAttr("Position", transform.t);

        ExportSimulation(attach, nodeAttach);

        nodeAttach->setAttr("Flags", attach.m_nFlags);

        if (attach.m_viewDistanceMultiplier != 1.0f)
        {
            nodeAttach->setAttr("ViewDistRatio", attach.m_viewDistanceMultiplier);
        }
        else
        {
            nodeAttach->delAttr("ViewDistRatio");
        }
    }


    void CharacterDefinition::ExportSkinAttachment(const CharacterAttachment& attach, XmlNodeRef nodeAttachments)
    {
        XmlNodeRef nodeAttach = gEnv->pSystem->CreateXmlNode("Attachment");
        nodeAttachments->addChild(nodeAttach);

        nodeAttach->setAttr("Type", "CA_SKIN");

        if (!attach.m_strSocketName.empty())
        {
            nodeAttach->setAttr("AName", attach.m_strSocketName);
        }

        if (!attach.m_strGeometryFilepath.empty())
        {
            nodeAttach->setAttr("Binding", attach.m_strGeometryFilepath);
        }
        else
        {
            nodeAttach->delAttr("Binding");
        }

        if (!attach.m_strMaterial.empty())
        {
            nodeAttach->setAttr("Material", attach.m_strMaterial);
        }

        nodeAttach->setAttr("Flags", attach.m_nFlags);

        if (attach.m_viewDistanceMultiplier != 1.0f)
        {
            nodeAttach->setAttr("ViewDistRatio", attach.m_viewDistanceMultiplier);
        }
        else
        {
            nodeAttach->delAttr("ViewDistRatio");
        }
    }

    void CharacterDefinition::ExportProxyAttachment(const CharacterAttachment& attach, XmlNodeRef nodeAttachments)
    {
        XmlNodeRef nodeAttach = gEnv->pSystem->CreateXmlNode("Attachment");
        nodeAttachments->addChild(nodeAttach);

        nodeAttach->setAttr("Type", "CA_PROX");

        if (!attach.m_strSocketName.empty())
        {
            nodeAttach->setAttr("AName", attach.m_strSocketName);
        }

        const QuatT& reltransform = attach.m_jointSpacePosition;
        const QuatT& abstransform = attach.m_characterSpacePosition;
        if (attach.m_rotationSpace == CharacterAttachment::SPACE_JOINT)
        {
            nodeAttach->setAttr("RelRotation", reltransform.q);
        }
        else
        {
            nodeAttach->setAttr("Rotation", abstransform.q);
        }
        if (attach.m_positionSpace == CharacterAttachment::SPACE_JOINT)
        {
            nodeAttach->setAttr("RelPosition", reltransform.t);
        }
        else
        {
            nodeAttach->setAttr("Position", abstransform.t);
        }

        if (!attach.m_strJointName.empty())
        {
            nodeAttach->setAttr("BoneName", attach.m_strJointName);
        }
        else
        {
            nodeAttach->delAttr("BoneName");
        }

        nodeAttach->setAttr("ProxyParams", attach.m_ProxyParams);
        nodeAttach->setAttr("ProxyPurpose", attach.m_ProxyPurpose);
    }


    void CharacterDefinition::ExportPClothAttachment(const CharacterAttachment& attach, XmlNodeRef nodeAttachments)
    {
        XmlNodeRef nodeAttach = gEnv->pSystem->CreateXmlNode("Attachment");
        nodeAttachments->addChild(nodeAttach);
        nodeAttach->setAttr("Type", "CA_PROW");
        if (!attach.m_strSocketName.empty())
        {
            nodeAttach->setAttr("AName", attach.m_strSocketName);
        }

        nodeAttach->setAttr("rowJointName", attach.m_strRowJointName);
        nodeAttach->setAttr("ROW_ClampMode", (int&)attach.m_rowSimulationParams.m_nClampMode);

        RowSimulationParams::ClampMode ct = attach.m_rowSimulationParams.m_nClampMode;
        if (ct == RowSimulationParams::TRANSLATIONAL_PROJECTION)
        {
            //only store values in XML if they are not identical with the default-values
            if (attach.m_rowSimulationParams.m_nProjectionType)
            {
                nodeAttach->setAttr("ROW_ProjectionType", attach.m_rowSimulationParams.m_nProjectionType);
            }
            if (attach.m_rowSimulationParams.m_nProjectionType == ProjectionSelection4::PS4_DirectedTranslation) // ACCEPTED_USE
            {
                uint32 IsIdentical = azstricmp(attach.m_rowSimulationParams.m_strDirTransJoint.c_str(), attach.m_strJointName.c_str()) == 0;
                if (attach.m_rowSimulationParams.m_strDirTransJoint.length() && IsIdentical == 0)
                {
                    nodeAttach->setAttr("ROW_DirTransJointName", attach.m_rowSimulationParams.m_strDirTransJoint.c_str());
                }
                else
                {
                    nodeAttach->setAttr("ROW_TranslationAxis", attach.m_rowSimulationParams.m_vTranslationAxis);
                }

                if (attach.m_rowSimulationParams.m_vCapsule.x)
                {
                    nodeAttach->setAttr("ROW_CapsuleX", attach.m_rowSimulationParams.m_vCapsule.x);
                }
                if (attach.m_rowSimulationParams.m_vCapsule.y)
                {
                    nodeAttach->setAttr("ROW_CapsuleY", attach.m_rowSimulationParams.m_vCapsule.y);
                }
            }
            if (attach.m_rowSimulationParams.m_nProjectionType == ProjectionSelection4::PS4_ShortvecTranslation) // ACCEPTED_USE
            {
                if (attach.m_rowSimulationParams.m_vCapsule.y)
                {
                    nodeAttach->setAttr("ROW_CapsuleY", attach.m_rowSimulationParams.m_vCapsule.y);
                }
            }

            uint32 numProxiesName = min(static_cast<int>(SimulationParams::MaxCollisionProxies), attach.m_rowSimulationParams.m_arrProxyNames.size());
            char proxytag[] = "ROW_Proxy00";
            for (uint32 i = 0; i < numProxiesName; i++)
            {
                const char* pname = attach.m_rowSimulationParams.m_arrProxyNames[i].c_str();
                nodeAttach->setAttr(proxytag, pname);
                proxytag[10]++;
            }
        }
        else
        {
            //only store values in XML if they are not identical with the default-values
            if (attach.m_rowSimulationParams.m_nSimFPS  != 10)
            {
                nodeAttach->setAttr("ROW_FPS",            attach.m_rowSimulationParams.m_nSimFPS);
            }
            if (attach.m_rowSimulationParams.m_fConeAngle != 45.0f)
            {
                nodeAttach->setAttr("ROW_ConeAngle",      attach.m_rowSimulationParams.m_fConeAngle);
            }
            if (sqr(attach.m_rowSimulationParams.m_vConeRotation))
            {
                nodeAttach->setAttr("ROW_ConeRotation",   attach.m_rowSimulationParams.m_vConeRotation);
            }

            if (attach.m_rowSimulationParams.m_fMass    != 1.00f)
            {
                nodeAttach->setAttr("ROW_Mass",           attach.m_rowSimulationParams.m_fMass);
            }
            if (attach.m_rowSimulationParams.m_fGravity != 9.81f)
            {
                nodeAttach->setAttr("ROW_Gravity",        attach.m_rowSimulationParams.m_fGravity);
            }
            if (attach.m_rowSimulationParams.m_fDamping != 1.00f)
            {
                nodeAttach->setAttr("ROW_Damping",        attach.m_rowSimulationParams.m_fDamping);
            }
            if (attach.m_rowSimulationParams.m_fJointSpring)
            {
                nodeAttach->setAttr("ROW_JointSpring",    attach.m_rowSimulationParams.m_fJointSpring);
            }
            if (attach.m_rowSimulationParams.m_fRodLength > 0.1)
            {
                nodeAttach->setAttr("ROW_RodLength",      attach.m_rowSimulationParams.m_fRodLength);
            }
            if (sqr(attach.m_rowSimulationParams.m_vStiffnessTarget))
            {
                nodeAttach->setAttr("ROW_StiffnessTarget", attach.m_rowSimulationParams.m_vStiffnessTarget);
            }
            if (sqr(attach.m_rowSimulationParams.m_vTurbulence))
            {
                nodeAttach->setAttr("ROW_Turbulence",     attach.m_rowSimulationParams.m_vTurbulence);
            }
            if (attach.m_rowSimulationParams.m_fMaxVelocity)
            {
                nodeAttach->setAttr("ROW_MaxVelocity",    attach.m_rowSimulationParams.m_fMaxVelocity);
            }
            if (sqr(attach.m_rowSimulationParams.m_worldSpaceDamping))
            {
                nodeAttach->setAttr("ROW_WorldSpaceDamping", attach.m_rowSimulationParams.m_worldSpaceDamping);
            }

            if (attach.m_rowSimulationParams.m_cycle)
            {
                nodeAttach->setAttr("ROW_Cycle",          attach.m_rowSimulationParams.m_cycle);
            }
            if (attach.m_rowSimulationParams.m_relaxationLoops)
            {
                nodeAttach->setAttr("ROW_RelaxLoops",     attach.m_rowSimulationParams.m_relaxationLoops);
            }
            if (attach.m_rowSimulationParams.m_fStretch != 0.10f)
            {
                nodeAttach->setAttr("ROW_Stretch",        attach.m_rowSimulationParams.m_fStretch);
            }

            if (attach.m_rowSimulationParams.m_vCapsule.x)
            {
                nodeAttach->setAttr("ROW_CapsuleX",       attach.m_rowSimulationParams.m_vCapsule.x);
            }
            if (attach.m_rowSimulationParams.m_vCapsule.y)
            {
                nodeAttach->setAttr("ROW_CapsuleY",       attach.m_rowSimulationParams.m_vCapsule.y);
            }
            if (attach.m_rowSimulationParams.m_nProjectionType)
            {
                nodeAttach->setAttr("ROW_ProjectionType", attach.m_rowSimulationParams.m_nProjectionType);
            }

            uint32 numProxiesName = min(static_cast<int>(SimulationParams::MaxCollisionProxies), attach.m_rowSimulationParams.m_arrProxyNames.size());
            char proxytag[] = "ROW_Proxy00";
            for (uint32 i = 0; i < numProxiesName; i++)
            {
                const char* pname = attach.m_rowSimulationParams.m_arrProxyNames[i].c_str();
                nodeAttach->setAttr(proxytag, pname);
                proxytag[10]++;
            }
        }
    }

    void CharacterDefinition::ExportVClothAttachment(const CharacterAttachment& attach, XmlNodeRef nodeAttachments)
    {
        XmlNodeRef nodeAttach = gEnv->pSystem->CreateXmlNode("Attachment");
        nodeAttachments->addChild(nodeAttach);

        nodeAttach->setAttr("Type", "CA_VCLOTH");

        if (!attach.m_strSocketName.empty())
        {
            nodeAttach->setAttr("AName", attach.m_strSocketName);
        }

        nodeAttach->setAttr("hide", attach.m_vclothParams.hide);
        nodeAttach->setAttr("thickness", attach.m_vclothParams.thickness);
        nodeAttach->setAttr("collsionDamping", attach.m_vclothParams.collisionDamping);
        nodeAttach->setAttr("stretchStiffness", attach.m_vclothParams.stretchStiffness);
        nodeAttach->setAttr("shearStiffness", attach.m_vclothParams.shearStiffness);
        nodeAttach->setAttr("bendStiffness", attach.m_vclothParams.bendStiffness);
        nodeAttach->setAttr("numIterations", attach.m_vclothParams.numIterations);
        nodeAttach->setAttr("timeStep", attach.m_vclothParams.timeStep);
        nodeAttach->setAttr("rigidDamping", attach.m_vclothParams.rigidDamping);
        nodeAttach->setAttr("translationBlend", attach.m_vclothParams.translationBlend);
        nodeAttach->setAttr("rotationBlend", attach.m_vclothParams.rotationBlend);
        nodeAttach->setAttr("friction", attach.m_vclothParams.friction);
        nodeAttach->setAttr("pullStiffness", attach.m_vclothParams.pullStiffness);
        nodeAttach->setAttr("SA_Mass", attach.m_vclothParams.tolerance);
        nodeAttach->setAttr("maxBlendWeight", attach.m_vclothParams.maxBlendWeight);
        nodeAttach->setAttr("maxAnimDistance", attach.m_vclothParams.maxAnimDistance);

        nodeAttach->setAttr("stiffnessGradient", attach.m_vclothParams.stiffnessGradient);
        nodeAttach->setAttr("halfStretchIterations", attach.m_vclothParams.halfStretchIterations);
        nodeAttach->setAttr("isMainCharacter", attach.m_vclothParams.isMainCharacter);

        nodeAttach->setAttr("renderMeshName", attach.m_vclothParams.renderMeshName);
        nodeAttach->setAttr("Binding", attach.m_vclothParams.renderBinding);

        nodeAttach->setAttr("simMeshName", attach.m_vclothParams.simMeshName);
        nodeAttach->setAttr("simBinding", attach.m_vclothParams.simBinding);
    }

    void CharacterDefinition::ExportSimulation(const CharacterAttachment& attach, XmlNodeRef nodeAttach)
    {
        if (SimulationParams::DISABLED != attach.m_simulationParams.m_nClampType)
        {
            uint32 ct = attach.m_simulationParams.m_nClampType;
            if (ct == SimulationParams::PENDULUM_CONE || ct == SimulationParams::PENDULUM_HINGE_PLANE || ct == SimulationParams::PENDULUM_HALF_CONE)
            {
                nodeAttach->setAttr("PA_PendulumType", (int&)attach.m_simulationParams.m_nClampType);

                //only store values in XML if they are not identical with the default-values
                if (attach.m_simulationParams.m_nSimFPS  != 10)
                {
                    nodeAttach->setAttr("PA_FPS", attach.m_simulationParams.m_nSimFPS);
                }
                if (attach.m_simulationParams.m_fMaxAngle != 45.0f)
                {
                    nodeAttach->setAttr("PA_MaxAngle", attach.m_simulationParams.m_fMaxAngle);
                }
                if (attach.m_simulationParams.m_vDiskRotation.x)
                {
                    nodeAttach->setAttr("PA_HRotation", attach.m_simulationParams.m_vDiskRotation.x);
                }
                if (attach.m_simulationParams.m_useRedirect)
                {
                    nodeAttach->setAttr("PA_Redirect", attach.m_simulationParams.m_useRedirect);
                }

                if (attach.m_simulationParams.m_fMass    != 1.00f)
                {
                    nodeAttach->setAttr("PA_Mass", attach.m_simulationParams.m_fMass);
                }
                if (attach.m_simulationParams.m_fGravity != 9.81f)
                {
                    nodeAttach->setAttr("PA_Gravity", attach.m_simulationParams.m_fGravity);
                }
                if (attach.m_simulationParams.m_fDamping != 1.00f)
                {
                    nodeAttach->setAttr("PA_Damping", attach.m_simulationParams.m_fDamping);
                }
                if (attach.m_simulationParams.m_fStiffness)
                {
                    nodeAttach->setAttr("PA_Stiffness", attach.m_simulationParams.m_fStiffness);
                }

                if (sqr(attach.m_simulationParams.m_vPivotOffset))
                {
                    nodeAttach->setAttr("PA_PivotOffset", attach.m_simulationParams.m_vPivotOffset);
                }
                if (sqr(attach.m_simulationParams.m_vSimulationAxis))
                {
                    nodeAttach->setAttr("PA_SimulationAxis", attach.m_simulationParams.m_vSimulationAxis);
                }
                if (sqr(attach.m_simulationParams.m_vStiffnessTarget))
                {
                    nodeAttach->setAttr("PA_StiffnessTarget", attach.m_simulationParams.m_vStiffnessTarget);
                }

                if (attach.m_simulationParams.m_vCapsule.x)
                {
                    nodeAttach->setAttr("PA_CapsuleX", attach.m_simulationParams.m_vCapsule.x);
                }
                if (attach.m_simulationParams.m_vCapsule.y)
                {
                    nodeAttach->setAttr("PA_CapsuleY", attach.m_simulationParams.m_vCapsule.y);
                }
                if (attach.m_simulationParams.m_nProjectionType)
                {
                    nodeAttach->setAttr("PA_ProjectionType", attach.m_simulationParams.m_nProjectionType);
                }

                if (attach.m_simulationParams.m_nProjectionType == 3)
                {
                    uint32 IsIdentical = azstricmp(attach.m_simulationParams.m_strDirTransJoint.c_str(), attach.m_strJointName.c_str()) == 0;
                    if (attach.m_simulationParams.m_strDirTransJoint.length() && IsIdentical == 0)
                    {
                        nodeAttach->setAttr("PA_DirTransJointName", attach.m_simulationParams.m_strDirTransJoint.c_str());
                    }
                }

                uint32 numProxiesName = min(static_cast<int>(SimulationParams::MaxCollisionProxies), attach.m_simulationParams.m_arrProxyNames.size());
                char proxytag[] = "PA_Proxy00";
                for (uint32 i = 0; i < numProxiesName; i++)
                {
                    const char* pname = attach.m_simulationParams.m_arrProxyNames[i].c_str();
                    nodeAttach->setAttr(proxytag, pname);
                    proxytag[9]++;
                }
            }

            if (ct == SimulationParams::SPRING_ELLIPSOID)
            {
                nodeAttach->setAttr("SA_SpringType", (int&)attach.m_simulationParams.m_nClampType);

                //only store values in XML if they are not identical with the default-values
                if (attach.m_simulationParams.m_nSimFPS  != 10)
                {
                    nodeAttach->setAttr("SA_FPS",            attach.m_simulationParams.m_nSimFPS);
                }

                if (attach.m_simulationParams.m_fRadius  != 0.5f)
                {
                    nodeAttach->setAttr("SA_Radius",         attach.m_simulationParams.m_fRadius);
                }
                if (attach.m_simulationParams.m_vSphereScale.x != 1)
                {
                    nodeAttach->setAttr("SA_ScaleZP",        attach.m_simulationParams.m_vSphereScale.x);
                }
                if (attach.m_simulationParams.m_vSphereScale.y != 1)
                {
                    nodeAttach->setAttr("SA_ScaleZN",        attach.m_simulationParams.m_vSphereScale.y);
                }
                if (attach.m_simulationParams.m_vDiskRotation.x)
                {
                    nodeAttach->setAttr("SA_DiskRotX",       attach.m_simulationParams.m_vDiskRotation.x);
                }
                if (attach.m_simulationParams.m_vDiskRotation.y)
                {
                    nodeAttach->setAttr("SA_DiskRotZ",       attach.m_simulationParams.m_vDiskRotation.y);
                }
                if (attach.m_simulationParams.m_useRedirect)
                {
                    nodeAttach->setAttr("SA_Redirect",       attach.m_simulationParams.m_useRedirect);
                }

                if (attach.m_simulationParams.m_fMass    != 1.00f)
                {
                    nodeAttach->setAttr("SA_Mass",           attach.m_simulationParams.m_fMass);
                }
                if (attach.m_simulationParams.m_fGravity != 9.81f)
                {
                    nodeAttach->setAttr("SA_Gravity",        attach.m_simulationParams.m_fGravity);
                }
                if (attach.m_simulationParams.m_fDamping != 1.00f)
                {
                    nodeAttach->setAttr("SA_Damping",        attach.m_simulationParams.m_fDamping);
                }
                if (attach.m_simulationParams.m_fStiffness)
                {
                    nodeAttach->setAttr("SA_Stiffness",      attach.m_simulationParams.m_fStiffness);
                }

                if (sqr(attach.m_simulationParams.m_vStiffnessTarget))
                {
                    nodeAttach->setAttr("SA_StiffnessTarget", attach.m_simulationParams.m_vStiffnessTarget);
                }

                //only store values in XML if they are not identical with the default-values
                if (attach.m_simulationParams.m_nProjectionType)
                {
                    nodeAttach->setAttr("SA_ProjectionType", attach.m_simulationParams.m_nProjectionType);
                }
                if (attach.m_simulationParams.m_nProjectionType)
                {
                    if (attach.m_simulationParams.m_vCapsule.y)
                    {
                        nodeAttach->setAttr("SA_CapsuleY",       attach.m_simulationParams.m_vCapsule.y);
                    }
                }
                if (sqr(attach.m_simulationParams.m_vPivotOffset))
                {
                    nodeAttach->setAttr("SA_PivotOffset",    attach.m_simulationParams.m_vPivotOffset);
                }

                uint32 numProxiesName = min(static_cast<int>(SimulationParams::MaxCollisionProxies), attach.m_simulationParams.m_arrProxyNames.size());
                char proxytag[] = "SA_Proxy00";
                for (uint32 i = 0; i < numProxiesName; i++)
                {
                    const char* pname = attach.m_simulationParams.m_arrProxyNames[i].c_str();
                    nodeAttach->setAttr(proxytag, pname);
                    proxytag[9]++;
                }
            }

            if (ct == SimulationParams::TRANSLATIONAL_PROJECTION)
            {
                nodeAttach->setAttr("P_Projection", (int&)attach.m_simulationParams.m_nClampType);

                //only store values in XML if they are not identical with the default-values
                if (attach.m_simulationParams.m_nProjectionType)
                {
                    nodeAttach->setAttr("P_ProjectionType", attach.m_simulationParams.m_nProjectionType);
                }
                if (attach.m_simulationParams.m_nProjectionType == ProjectionSelection4::PS4_DirectedTranslation) // ACCEPTED_USE
                {
                    uint32 IsIdentical = azstricmp(attach.m_simulationParams.m_strDirTransJoint.c_str(), attach.m_strJointName.c_str()) == 0;
                    if (attach.m_simulationParams.m_strDirTransJoint.length() && IsIdentical == 0)
                    {
                        nodeAttach->setAttr("P_DirTransJointName", attach.m_simulationParams.m_strDirTransJoint.c_str());
                    }
                    else
                    {
                        nodeAttach->setAttr("P_TranslationAxis", attach.m_simulationParams.m_vSimulationAxis);
                    }
                }
                if (attach.m_simulationParams.m_nProjectionType == ProjectionSelection4::PS4_DirectedTranslation) // ACCEPTED_USE
                {
                    if (attach.m_simulationParams.m_vCapsule.x)
                    {
                        nodeAttach->setAttr("P_CapsuleX", attach.m_simulationParams.m_vCapsule.x);
                    }
                    if (attach.m_simulationParams.m_vCapsule.y)
                    {
                        nodeAttach->setAttr("P_CapsuleY", attach.m_simulationParams.m_vCapsule.y);
                    }
                }
                else
                {
                    if (attach.m_simulationParams.m_vCapsule.y)
                    {
                        nodeAttach->setAttr("P_CapsuleY", attach.m_simulationParams.m_vCapsule.y);
                    }
                }

                if (sqr(attach.m_simulationParams.m_vPivotOffset))
                {
                    nodeAttach->setAttr("P_PivotOffset", attach.m_simulationParams.m_vPivotOffset);
                }

                uint32 numProxiesName = min(static_cast<int>(SimulationParams::MaxCollisionProxies), attach.m_simulationParams.m_arrProxyNames.size());
                char proxytag[] = "P_Proxy00";
                for (uint32 i = 0; i < numProxiesName; i++)
                {
                    const char* pname = attach.m_simulationParams.m_arrProxyNames[i].c_str();
                    nodeAttach->setAttr(proxytag, pname);
                    proxytag[8]++;
                }
            }
        }
    }


    bool CharacterDefinition::Save(const char* filename)
    {
        XmlNodeRef root = SaveToXml();
        if (!root)
        {
            return false;
        }
        return root->saveToFile(filename);
    }

    bool CharacterDefinition::SaveToMemory(vector<char>* buffer)
    {
        XmlNodeRef root = SaveToXml();
        if (!root)
        {
            return false;
        }
        _smart_ptr<IXmlStringData> str(root->getXMLData());
        buffer->assign(str->GetString(), str->GetString() + str->GetStringLength());
        return true;
    }

    static string NormalizeMaterialName(const char* materialName)
    {
        string result = materialName;
        result.MakeLower();
        PathUtil::RemoveExtension(result);
        return result;
    }













    void CharacterDefinition::ApplyToCharacter(bool* skinSetChanged, ICharacterInstance* pICharacterInstance, ICharacterManager* characterManager, bool showDebug)
    {
        if (m_initialized == 0)
        {
            return; //no serialization, no update
        }
        IAttachmentManager* pIAttachmentManager = pICharacterInstance->GetIAttachmentManager();

        vector<string> names;
        uint32 attachmentCount = pIAttachmentManager->GetAttachmentCount();
        for (size_t i = 0; i < attachmentCount; ++i)
        {
            IAttachment* attachment = pIAttachmentManager->GetInterfaceByIndex(i);
            names.push_back(attachment->GetName());
        }

        vector<string> proxynames;
        uint32 proxyCount = pIAttachmentManager->GetProxyCount();
        for (size_t i = 0; i < proxyCount; ++i)
        {
            IProxy* proxy = pIAttachmentManager->GetProxyInterfaceByIndex(i);
            proxynames.push_back(proxy->GetName());
        }

        uint32 numAttachments = attachments.size();
        for (uint32 i = 0; i < numAttachments; ++i)
        {
            const CharacterAttachment& attachment = attachments[i];
            const char* strSocketName = attachment.m_strSocketName.c_str();
            int index = pIAttachmentManager->GetIndexByName(strSocketName);

            IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByIndex(index);
            if (index < 0)
            {
                if (attachment.m_attachmentType == CA_BONE || attachment.m_attachmentType == CA_FACE || attachment.m_attachmentType == CA_SKIN || attachment.m_attachmentType == CA_PROW)
                {
                    if (attachment.m_strSocketName.length())
                    {
                        pIAttachment = pIAttachmentManager->CreateAttachment(attachment.m_strSocketName.c_str(), attachment.m_attachmentType, attachment.m_strJointName.c_str());
                        if (pIAttachment)
                        {
                            index = pIAttachmentManager->GetIndexByName(pIAttachment->GetName());
                        }
                    }
                }
            }

            if (pIAttachment && index >= 0)
            {
                if (attachment.m_attachmentType != CA_PROX)
                {
                    if (pIAttachment->GetType() != attachment.m_attachmentType)
                    {
                        pIAttachmentManager->RemoveAttachmentByInterface(pIAttachment);
                        pIAttachment = pIAttachmentManager->CreateAttachment(attachment.m_strSocketName.c_str(), attachment.m_attachmentType, attachment.m_strJointName.c_str());
                        if (pIAttachment)
                        {
                            index = pIAttachmentManager->GetIndexByName(pIAttachment->GetName());
                        }
                    }
                }
            }

            if (pIAttachment == 0)
            {
                if (index < 0 && attachment.m_attachmentType == CA_PROX)
                {
                    IProxy* pIProxy = pIAttachmentManager->GetProxyInterfaceByName(strSocketName);
                    if (pIProxy == 0)
                    {
                        uint32 IsEmpty = attachment.m_strJointName.empty();
                        if (IsEmpty == 0)
                        {
                            pIAttachmentManager->CreateProxy(strSocketName, attachment.m_strJointName);
                        }
                    }

                    //physics-proxies are not in the regular attachment-list
                    pIProxy = pIAttachmentManager->GetProxyInterfaceByName(strSocketName);
                    if (pIProxy)
                    {
                        pIProxy->SetProxyParams(attachment.m_ProxyParams);
                        pIProxy->SetProxyPurpose(int8(attachment.m_ProxyPurpose));
                        QuatT engineLocation = pIProxy->GetProxyAbsoluteDefault();
                        QuatT editorLocation = attachment.m_characterSpacePosition;
                        if (engineLocation.t != editorLocation.t || engineLocation.q != editorLocation.q)
                        {
                            pIProxy->SetProxyAbsoluteDefault(editorLocation);
                        }
                        string lowercaseAttachmentName(attachment.m_strSocketName);
                        lowercaseAttachmentName.MakeLower(); // lowercase because the names in the list coming from the engine are lowercase
                        proxynames.erase(std::remove(proxynames.begin(), proxynames.end(), lowercaseAttachmentName.c_str()), proxynames.end());
                    }
                }
                continue;
            }

            if (pIAttachment->GetType() != attachment.m_attachmentType)
            {
                continue;
            }

            switch (attachment.m_attachmentType)
            {
            case CA_BONE:
            {
                ApplyBoneAttachment(pIAttachment, characterManager, attachment, pICharacterInstance, showDebug);
                break;
            }

            case CA_FACE:
            {
                ApplyFaceAttachment(pIAttachment, characterManager, attachment, showDebug);
                break;
            }

            case CA_SKIN:
            {
                ApplySkinAttachment(pIAttachment, characterManager, attachment, pICharacterInstance, skinSetChanged);
                break;
            }

            case CA_VCLOTH:
            {
                break;
            }
            case CA_PROW:
            {
                pIAttachment->SetJointName(attachment.m_strRowJointName.c_str());
                if (showDebug)
                {
                    pIAttachment->GetRowParams() = attachment.m_rowSimulationParams;
                    pIAttachment->PostUpdateSimulationParams(0, 0);
                }
                break;
            }
            }

            //make sure we don't loose the dyanamic flags
            uint32 nMaskForDynamicFlags = 0;
            nMaskForDynamicFlags |= FLAGS_ATTACH_VISIBLE;
            nMaskForDynamicFlags |= FLAGS_ATTACH_PROJECTED;
            nMaskForDynamicFlags |= FLAGS_ATTACH_WAS_PHYSICALIZED;
            nMaskForDynamicFlags |= FLAGS_ATTACH_HIDE_MAIN_PASS;
            nMaskForDynamicFlags |= FLAGS_ATTACH_HIDE_SHADOW_PASS;
            nMaskForDynamicFlags |= FLAGS_ATTACH_HIDE_RECURSION;
            nMaskForDynamicFlags |= FLAGS_ATTACH_NEAREST_NOFOV;
            nMaskForDynamicFlags |= FLAGS_ATTACH_NO_BBOX_INFLUENCE;
            nMaskForDynamicFlags |= FLAGS_ATTACH_COMBINEATTACHMENT;
            uint32 flags = pIAttachment->GetFlags() & nMaskForDynamicFlags;
            pIAttachment->SetFlags(attachment.m_nFlags | flags);
            pIAttachment->HideAttachment((attachment.m_nFlags & FLAGS_ATTACH_HIDE_ATTACHMENT) != 0);

            string lowercaseAttachmentName(attachment.m_strSocketName);
            lowercaseAttachmentName.MakeLower(); // lowercase because the names in the list coming from the engine are lowercase
            names.erase(std::remove(names.begin(), names.end(), lowercaseAttachmentName.c_str()), names.end());
        }


        // remove attachments that weren't updated
        for (size_t i = 0; i < names.size(); ++i)
        {
            pIAttachmentManager->RemoveAttachmentByName(names[i].c_str());
            if (skinSetChanged)
            {
                *skinSetChanged = true;
            }
        }
        for (size_t i = 0; i < proxynames.size(); ++i)
        {
            pIAttachmentManager->RemoveProxyByName(proxynames[i].c_str());
        }

        if (skinSetChanged)
        {
            //if the a skin-attachment was changed on the character, then probaly the whole skeleton changed
            //that means we have to set all bone-attachments again to the correct absolute location
            for (uint32 i = 0; i < numAttachments; ++i)
            {
                CharacterAttachment& attachment = attachments[i];
                if (attachment.m_attachmentType == CA_BONE)
                {
                    const char* strSocketName = attachment.m_strSocketName.c_str();
                    int index = pIAttachmentManager->GetIndexByName(strSocketName);
                    IAttachment* pIAttachment = pIAttachmentManager->GetInterfaceByIndex(index);
                    if (pIAttachment)
                    {
                        const IDefaultSkeleton& rIDefaultSkeleton = pICharacterInstance->GetIDefaultSkeleton();
                        int id = rIDefaultSkeleton.GetJointIDByName(attachment.m_strJointName.c_str());
                        if (id == -1)
                        {
                            continue;
                        }

                        const QuatT& defaultJointTransform = rIDefaultSkeleton.GetDefaultAbsJointByID(id);
                        if (attachment.m_rotationSpace == CharacterAttachment::SPACE_JOINT)
                        {
                            attachment.m_characterSpacePosition.q = (defaultJointTransform * attachment.m_jointSpacePosition).q.GetNormalized();
                        }
                        if (attachment.m_positionSpace == CharacterAttachment::SPACE_JOINT)
                        {
                            attachment.m_characterSpacePosition.t = (defaultJointTransform * attachment.m_jointSpacePosition).t;
                        }

                        if (attachment.m_simulationParams.m_nClampType == SimulationParams::TRANSLATIONAL_PROJECTION)
                        {
                            attachment.m_characterSpacePosition = defaultJointTransform;
                            attachment.m_jointSpacePosition.SetIdentity();
                        }

                        QuatT engineLocation = pIAttachment->GetAttAbsoluteDefault();
                        QuatT editorLocation = attachment.m_characterSpacePosition;
                        if (engineLocation.t != editorLocation.t || engineLocation.q != editorLocation.q)
                        {
                            pIAttachment->SetAttAbsoluteDefault(editorLocation);
                        }
                    }
                }
            }
        }

        SynchModifiers(*pICharacterInstance);
        m_initialized = true; //only the serialization code can set this to true.
    }




    void CharacterDefinition::ApplyBoneAttachment(IAttachment* pIAttachment, ICharacterManager* characterManager, const CharacterAttachment& desc, ICharacterInstance* pICharacterInstance, bool showDebug) const
    {
        if (pIAttachment == 0)
        {
            return;
        }

        uint32 type = pIAttachment->GetType();
        if (type == CA_BONE)
        {
            const char* strJointname = desc.m_strJointName.c_str();
            int32 nJointID1 = pIAttachment->GetJointID();
            int32 nJointID2 = pICharacterInstance->GetIDefaultSkeleton().GetJointIDByName(strJointname);
            if (nJointID1 != nJointID2 && nJointID2 != -1)
            {
                pIAttachment->SetJointName(strJointname);
            }

            IAttachmentObject* pIAttachmentObject = pIAttachment->GetIAttachmentObject();
            string existingBindingFilename;
            if (pIAttachmentObject)
            {
                if (IStatObj* statObj = pIAttachmentObject->GetIStatObj())
                {
                    existingBindingFilename = statObj->GetFilePath();
                }
                else if (ICharacterInstance* attachedCharacter = pIAttachmentObject->GetICharacterInstance())
                {
                    existingBindingFilename = attachedCharacter->GetFilePath();
                }
                else if (IAttachmentSkin* attachmentSkin = pIAttachmentObject->GetIAttachmentSkin())
                {
                    if (ISkin* skin = attachmentSkin->GetISkin())
                    {
                        existingBindingFilename = skin->GetModelFilePath();
                    }
                }
            }

            if (azstricmp(existingBindingFilename.c_str(), desc.m_strGeometryFilepath.c_str()) != 0)
            {
                if (!desc.m_strGeometryFilepath.empty())
                {
                    string fileExt = PathUtil::GetExt(desc.m_strGeometryFilepath.c_str());

                    bool IsCDF = (0 == azstricmp(fileExt, "cdf"));
                    bool IsCHR = (0 == azstricmp(fileExt, "chr"));
                    bool IsCGA = (0 == azstricmp(fileExt, "cga"));
                    bool IsCGF = (0 == azstricmp(fileExt, "cgf"));
                    if (IsCDF || IsCHR || IsCGA)
                    {
                        ICharacterInstance* pIChildCharacter = characterManager->CreateInstance(desc.m_strGeometryFilepath.c_str(), CA_CharEditModel);
                        if (pIChildCharacter)
                        {
                            CSKELAttachment* pCharacterAttachment = new CSKELAttachment();
                            pCharacterAttachment->m_pCharInstance  = pIChildCharacter;
                            pIAttachmentObject = (IAttachmentObject*)pCharacterAttachment;
                        }
                    }
                    if (IsCGF)
                    {
                        _smart_ptr<IStatObj> pIStatObj = gEnv->p3DEngine->LoadStatObjAutoRef(desc.m_strGeometryFilepath.c_str(), 0, 0, false);
                        if (pIStatObj)
                        {
                            CCGFAttachment* pStatAttachment = new CCGFAttachment();
                            pStatAttachment->pObj  = pIStatObj;
                            pIAttachmentObject = (IAttachmentObject*)pStatAttachment;
                        }
                    }
                    if (pIAttachmentObject != pIAttachment->GetIAttachmentObject())
                    {
                        pIAttachment->AddBinding(pIAttachmentObject);
                    }
                }
                else
                {
                    pIAttachment->ClearBinding();
                }
            }

            SimulationParams& ap = pIAttachment->GetSimulationParams();
            bool bAttachmentSortingRequired  = ap.m_nClampType != desc.m_simulationParams.m_nClampType;
            bAttachmentSortingRequired      |= uint8(ap.m_useRedirect) != desc.m_simulationParams.m_useRedirect;

            ap.m_nClampType       = desc.m_simulationParams.m_nClampType;
            if (showDebug && (ap.m_nClampType == SimulationParams::PENDULUM_CONE || ap.m_nClampType == SimulationParams::PENDULUM_HINGE_PLANE || ap.m_nClampType == SimulationParams::PENDULUM_HALF_CONE))
            {
                ap.m_useDebugSetup    = desc.m_simulationParams.m_useDebugSetup;
                ap.m_useDebugText     = desc.m_simulationParams.m_useDebugText;
                ap.m_useSimulation    = desc.m_simulationParams.m_useSimulation;
                ap.m_useRedirect      = desc.m_simulationParams.m_useRedirect;

                ap.m_nSimFPS          = desc.m_simulationParams.m_nSimFPS;
                ap.m_fMaxAngle        = desc.m_simulationParams.m_fMaxAngle;
                ap.m_vDiskRotation.x  = desc.m_simulationParams.m_vDiskRotation.x;

                ap.m_fMass            = desc.m_simulationParams.m_fMass;
                ap.m_fGravity         = desc.m_simulationParams.m_fGravity;
                ap.m_fDamping         = desc.m_simulationParams.m_fDamping;
                ap.m_fStiffness       = desc.m_simulationParams.m_fStiffness;

                ap.m_vPivotOffset     = desc.m_simulationParams.m_vPivotOffset;
                ap.m_vSimulationAxis  = desc.m_simulationParams.m_vSimulationAxis;
                ap.m_vStiffnessTarget = desc.m_simulationParams.m_vStiffnessTarget;
                ap.m_strProcFunction  = desc.m_simulationParams.m_strProcFunction;

                ap.m_vCapsule         = desc.m_simulationParams.m_vCapsule;
                ap.m_nProjectionType  = desc.m_simulationParams.m_nProjectionType;
                ap.m_arrProxyNames    = desc.m_simulationParams.m_arrProxyNames;
            }
            if (showDebug && ap.m_nClampType == SimulationParams::SPRING_ELLIPSOID)
            {
                ap.m_useDebugSetup    = desc.m_simulationParams.m_useDebugSetup;
                ap.m_useDebugText     = desc.m_simulationParams.m_useDebugText;
                ap.m_useSimulation    = desc.m_simulationParams.m_useSimulation;
                ap.m_useRedirect      = desc.m_simulationParams.m_useRedirect;
                ap.m_nSimFPS          = desc.m_simulationParams.m_nSimFPS;

                ap.m_fRadius          = desc.m_simulationParams.m_fRadius;
                ap.m_vSphereScale     = desc.m_simulationParams.m_vSphereScale;
                ap.m_vDiskRotation    = desc.m_simulationParams.m_vDiskRotation;

                ap.m_fMass            = desc.m_simulationParams.m_fMass;
                ap.m_fGravity         = desc.m_simulationParams.m_fGravity;
                ap.m_fDamping         = desc.m_simulationParams.m_fDamping;
                ap.m_fStiffness       = desc.m_simulationParams.m_fStiffness;

                ap.m_vStiffnessTarget = desc.m_simulationParams.m_vStiffnessTarget;
                ap.m_strProcFunction  = desc.m_simulationParams.m_strProcFunction;

                ap.m_nProjectionType  = desc.m_simulationParams.m_nProjectionType;
                ap.m_vCapsule         = desc.m_simulationParams.m_vCapsule;
                ap.m_vCapsule.x = 0;
                ap.m_vPivotOffset     = desc.m_simulationParams.m_vPivotOffset;
                ap.m_arrProxyNames    = desc.m_simulationParams.m_arrProxyNames;
            }
            if (showDebug && ap.m_nClampType == SimulationParams::TRANSLATIONAL_PROJECTION)
            {
                ap.m_useDebugSetup    = desc.m_simulationParams.m_useDebugSetup;
                ap.m_useDebugText     = desc.m_simulationParams.m_useDebugText;
                ap.m_useSimulation    = desc.m_simulationParams.m_useSimulation;
                ap.m_useRedirect      = 1;


                ap.m_nProjectionType  = desc.m_simulationParams.m_nProjectionType;
                ap.m_strDirTransJoint = desc.m_simulationParams.m_strDirTransJoint;
                ap.m_vSimulationAxis  = desc.m_simulationParams.m_vSimulationAxis;

                ap.m_vCapsule         = desc.m_simulationParams.m_vCapsule;
                if (ap.m_nProjectionType == ProjectionSelection4::PS4_ShortvecTranslation) // ACCEPTED_USE
                {
                    ap.m_vCapsule.x = 0;
                }
                ap.m_vPivotOffset     = desc.m_simulationParams.m_vPivotOffset;
                ap.m_arrProxyNames    = desc.m_simulationParams.m_arrProxyNames;
            }
            pIAttachment->PostUpdateSimulationParams(bAttachmentSortingRequired, desc.m_strJointName.c_str());
        }

        QuatT engineLocation = pIAttachment->GetAttAbsoluteDefault();
        QuatT editorLocation = desc.m_characterSpacePosition;
        if (engineLocation.t != editorLocation.t || engineLocation.q != editorLocation.q)
        {
            pIAttachment->SetAttAbsoluteDefault(editorLocation);
        }
    }




    void CharacterDefinition::ApplyFaceAttachment(IAttachment* pIAttachment, ICharacterManager* characterManager, const CharacterAttachment& desc, bool showDebug) const
    {
        if (pIAttachment == 0)
        {
            return;
        }

        uint32 type = pIAttachment->GetType();
        if (type == CA_FACE)
        {
            IAttachmentObject* pIAttachmentObject = pIAttachment->GetIAttachmentObject();
            string existingBindingFilename;
            if (pIAttachmentObject)
            {
                if (IStatObj* statObj = pIAttachmentObject->GetIStatObj())
                {
                    existingBindingFilename = statObj->GetFilePath();
                }
                else if (ICharacterInstance* attachedCharacter = pIAttachmentObject->GetICharacterInstance())
                {
                    existingBindingFilename = attachedCharacter->GetFilePath();
                }
                else if (IAttachmentSkin* attachmentSkin = pIAttachmentObject->GetIAttachmentSkin())
                {
                    if (ISkin* skin = attachmentSkin->GetISkin())
                    {
                        existingBindingFilename = skin->GetModelFilePath();
                    }
                }
            }

            if (azstricmp(existingBindingFilename.c_str(), desc.m_strGeometryFilepath.c_str()) != 0)
            {
                if (!desc.m_strGeometryFilepath.empty())
                {
                    string fileExt = PathUtil::GetExt(desc.m_strGeometryFilepath.c_str());

                    bool IsCDF = (0 == azstricmp(fileExt, "cdf"));
                    bool IsCHR = (0 == azstricmp(fileExt, "chr"));
                    bool IsCGA = (0 == azstricmp(fileExt, "cga"));
                    bool IsCGF = (0 == azstricmp(fileExt, "cgf"));
                    if (IsCDF || IsCHR || IsCGA)
                    {
                        ICharacterInstance* pIChildCharacter = characterManager->CreateInstance(desc.m_strGeometryFilepath.c_str(), CA_CharEditModel);
                        if (pIChildCharacter)
                        {
                            CSKELAttachment* pCharacterAttachment = new CSKELAttachment();
                            pCharacterAttachment->m_pCharInstance  = pIChildCharacter;
                            pIAttachmentObject = (IAttachmentObject*)pCharacterAttachment;
                        }
                    }
                    if (IsCGF)
                    {
                        _smart_ptr<IStatObj> pIStatObj = gEnv->p3DEngine->LoadStatObjAutoRef(desc.m_strGeometryFilepath.c_str(), 0, 0, false);
                        if (pIStatObj)
                        {
                            CCGFAttachment* pStatAttachment = new CCGFAttachment();
                            pStatAttachment->pObj  = pIStatObj;
                            pIAttachmentObject = (IAttachmentObject*)pStatAttachment;
                        }
                    }
                    if (pIAttachmentObject != pIAttachment->GetIAttachmentObject())
                    {
                        pIAttachment->AddBinding(pIAttachmentObject);
                    }
                }
                else
                {
                    pIAttachment->ClearBinding();
                }
            }

            SimulationParams& ap = pIAttachment->GetSimulationParams();
            bool bAttachmentSortingRequired  = ap.m_nClampType != desc.m_simulationParams.m_nClampType;

            ap.m_nClampType       = desc.m_simulationParams.m_nClampType;
            if (showDebug && (ap.m_nClampType == SimulationParams::PENDULUM_CONE || ap.m_nClampType == SimulationParams::PENDULUM_HINGE_PLANE || ap.m_nClampType == SimulationParams::PENDULUM_HALF_CONE))
            {
                ap.m_useDebugSetup    = desc.m_simulationParams.m_useDebugSetup;
                ap.m_useDebugText     = desc.m_simulationParams.m_useDebugText;
                ap.m_useSimulation    = desc.m_simulationParams.m_useSimulation;
                ap.m_useRedirect      = 0;
                ap.m_nSimFPS          = desc.m_simulationParams.m_nSimFPS;

                ap.m_fMaxAngle        = desc.m_simulationParams.m_fMaxAngle;
                ap.m_vDiskRotation.x  = desc.m_simulationParams.m_vDiskRotation.x;

                ap.m_fMass            = desc.m_simulationParams.m_fMass;
                ap.m_fGravity         = desc.m_simulationParams.m_fGravity;
                ap.m_fDamping         = desc.m_simulationParams.m_fDamping;
                ap.m_fStiffness       = desc.m_simulationParams.m_fStiffness;

                ap.m_vPivotOffset     = desc.m_simulationParams.m_vPivotOffset;
                ap.m_vSimulationAxis  = desc.m_simulationParams.m_vSimulationAxis;
                ap.m_vStiffnessTarget = desc.m_simulationParams.m_vStiffnessTarget;
                ap.m_strProcFunction.reset();
                ap.m_vCapsule         = desc.m_simulationParams.m_vCapsule;

                ap.m_nProjectionType  = desc.m_simulationParams.m_nProjectionType;
                ap.m_arrProxyNames    = desc.m_simulationParams.m_arrProxyNames;
            }
            if (showDebug && ap.m_nClampType == SimulationParams::SPRING_ELLIPSOID)
            {
                ap.m_useDebugSetup    = desc.m_simulationParams.m_useDebugSetup;
                ap.m_useDebugText     = desc.m_simulationParams.m_useDebugText;
                ap.m_useSimulation    = desc.m_simulationParams.m_useSimulation;
                ap.m_useRedirect      = 0;
                ap.m_nSimFPS          = desc.m_simulationParams.m_nSimFPS;

                ap.m_fRadius          = desc.m_simulationParams.m_fRadius;
                ap.m_vSphereScale     = desc.m_simulationParams.m_vSphereScale;
                ap.m_vDiskRotation    = desc.m_simulationParams.m_vDiskRotation;

                ap.m_fMass            = desc.m_simulationParams.m_fMass;
                ap.m_fGravity         = desc.m_simulationParams.m_fGravity;
                ap.m_fDamping         = desc.m_simulationParams.m_fDamping;
                ap.m_fStiffness       = desc.m_simulationParams.m_fStiffness;

                ap.m_vStiffnessTarget   = desc.m_simulationParams.m_vStiffnessTarget;
                ap.m_strProcFunction.reset();

                ap.m_nProjectionType  = desc.m_simulationParams.m_nProjectionType;
                ap.m_vCapsule         = desc.m_simulationParams.m_vCapsule;
                ap.m_vCapsule.x = 0;
                ap.m_vPivotOffset     = desc.m_simulationParams.m_vPivotOffset;
                ap.m_arrProxyNames    = desc.m_simulationParams.m_arrProxyNames;
            }

            pIAttachment->PostUpdateSimulationParams(bAttachmentSortingRequired);
        }

        QuatT engineLocation = pIAttachment->GetAttAbsoluteDefault();
        QuatT editorLocation = desc.m_characterSpacePosition;
        if (engineLocation.t != editorLocation.t || engineLocation.q != editorLocation.q)
        {
            pIAttachment->SetAttAbsoluteDefault(desc.m_characterSpacePosition);
        }
    }


    void CharacterDefinition::ApplySkinAttachment(IAttachment* pIAttachment, ICharacterManager* characterManager, const CharacterAttachment& desc, ICharacterInstance* pICharacterInstance, bool* skinChanged) const
    {
        uint32 type = pIAttachment->GetType();
        IAttachmentObject* iattachmentObject = pIAttachment->GetIAttachmentObject();
        IAttachmentManager* attachmentManager =  pICharacterInstance->GetIAttachmentManager();

        string existingMaterialFilename;
        string existingBindingFilename;
        bool hasExistingAttachment = false;

        if (iattachmentObject)
        {
            if (iattachmentObject->GetAttachmentType() == IAttachmentObject::eAttachment_SkinMesh)
            {
                CSKINAttachment* attachmentObject = (CSKINAttachment*)iattachmentObject;
                if (attachmentObject->m_pIAttachmentSkin)
                {
                    if (ISkin* skin = attachmentObject->m_pIAttachmentSkin->GetISkin())
                    {
                        existingBindingFilename = skin->GetModelFilePath();
                        if (_smart_ptr<IMaterial>  material = attachmentObject->GetReplacementMaterial(0))
                        {
                            existingMaterialFilename = material->GetName();
                        }
                        hasExistingAttachment = true;
                    }
                }
            }
        }

        if (!hasExistingAttachment ||  existingBindingFilename != desc.m_strGeometryFilepath || existingMaterialFilename != NormalizeMaterialName(desc.m_strMaterial.c_str()))
        {
            string fileExt = PathUtil::GetExt(desc.m_strGeometryFilepath.c_str());
            _smart_ptr<ISkin> pISkin = 0;
            if (desc.m_strGeometryFilepath.empty())
            {
                pIAttachment->ClearBinding();
            }
            else
            {
                bool isSkin = azstricmp(fileExt, CRY_SKIN_FILE_EXT) == 0;
                if (isSkin)
                {
                    pISkin = characterManager->LoadModelSKINAutoRef(desc.m_strGeometryFilepath.c_str(), CA_CharEditModel);
                }
            }
            if (pISkin == 0)
            {
                return;
            }

            IAttachmentSkin* attachmentSkin = pIAttachment->GetIAttachmentSkin();
            if (attachmentSkin == 0)
            {
                return;
            }

            CSKINAttachment* attachmentObject = new CSKINAttachment();
            attachmentObject->m_pIAttachmentSkin = attachmentSkin;

            _smart_ptr<IMaterial> material;
            if (!desc.m_strMaterial.empty())
            {
                _smart_ptr<IMaterial> newMaterial = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(NormalizeMaterialName(desc.m_strMaterial.c_str()).c_str(), false);
                material.reset(newMaterial);
            }
            attachmentObject->SetReplacementMaterial(material, 0);

            // AddBinding can destroy attachmentObject!
            pIAttachment->AddBinding(attachmentObject, pISkin, CA_CharEditModel);
            if (!attachmentSkin->GetISkin())
            {
                return;
            }

            if (skinChanged)
            {
                *skinChanged = true;
            }
        }

        return;
    }




    void CharacterDefinition::SynchModifiers(ICharacterInstance& character)
    {
#ifdef ENABLE_RUNTIME_POSE_MODIFIERS
        IAnimationSerializablePtr pPoseModifierSetup = character.GetISkeletonAnim()->GetPoseModifierSetup();
        if (pPoseModifierSetup == modifiers)
        {
            return;
        }

        MemoryOArchive bo;
        modifiers->Serialize(bo);
        MemoryIArchive bi;
        if (bi.open(bo.buffer(), bo.length()))
        {
            pPoseModifierSetup->Serialize(bi);
        }
#endif
    }

    string CharacterDefinition::GenerateUniqueAttachmentName(const char* referenceName) const
    {
        string result = referenceName;
        int index = 0;
        while (true)
        {
            bool found = false;
            for (const CharacterAttachment& attachment : attachments)
            {
                if (attachment.m_strSocketName == result)
                {
                    found = true;
                    break;
                }
            }

            if (found)
            {
                index++;
                result = referenceName;
                result += std::to_string(index).c_str();
            }
            else
            {
                break;
            }
        }
        return result;
    }
}
