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
#ifndef CRYINCLUDE_CRYCOMMON_PARAMNODETYPES_H
#define CRYINCLUDE_CRYCOMMON_PARAMNODETYPES_H
#pragma once

#include "CryArray.h"

// Summary:
//      This enumerates the different node types that can be contained in the chrparams file
enum EChrParamNodeType
{
    e_chrParamsNodeFirst,
    e_chrParamsNodeBoneLod = e_chrParamsNodeFirst,
    e_chrParamsNodeBBoxExcludeList,
    e_chrParamsNodeBBoxIncludeList,
    e_chrParamsNodeBBoxExtensionList,
    e_chrParamsNodeUsePhysProxyBBox,
    e_chrParamsNodeIKDefinition,
    e_chrParamsNodeAnimationList,
    e_chrParamsNodeCount,
    e_chrParamsNodeInvalid = e_chrParamsNodeCount,
};

// Summary:
//      This enumerates the different IK subnode types that can be contained in the IK Definition node
enum EChrParamsIKNodeSubType
{
    e_chrParamsIKNodeSubTypeFirst,
    e_chrParamsIKNodeLimb = e_chrParamsIKNodeSubTypeFirst,
    e_chrParamsIKNodeAnimDriven,
    e_chrParamsIKNodeFootLock,
    e_chrParamsIKNodeRecoil,
    e_chrParamsIKNodeLook,
    e_chrParamsIKNodeAim,
    e_chrParamsIKNodeSubTypeCount,
    e_chrParamsIKNodeSubTypeInvalid = e_chrParamsIKNodeSubTypeCount,
};

// Summary:
//      This enumerates the different types of animation nodes supported by the animation list node
enum EChrParamsAnimationNodeType
{
    e_chrParamsAnimationNodeTypeFirst,
    e_chrParamsAnimationNodeSingleAsset = e_chrParamsAnimationNodeTypeFirst,
    e_chrParamsAnimationNodeWildcardAsset,
    e_chrParamsAnimationNodeSingleDBA,
    e_chrParamsAnimationNodeWildcardDBA,
    e_chrParamsAnimationNodeFilepath,
    e_chrParamsAnimationNodeParseSubfolders,
    e_chrParamsAnimationNodeInclude,
    e_chrParamsAnimationNodeFaceLib,
    e_chrParamsAnimationNodeAnimEventsDB,
    e_chrParamsAnimationNodeTypeCount,
    e_chrParamsAnimationNodeTypeInvalid = e_chrParamsAnimationNodeTypeCount,
};

// Summary:
//      This contains the default handle for each of the animation node types
static const char* g_defaultAnimationNodeNames[] =
{
    "",                     // e_chrParamsAnimationNodeSingleAsset
    "*",                    // e_chrParamsAnimationNodeWildcardAsset,
    "$TracksDatabase",      // e_chrParamsAnimationNodeSingleDBA,
    "$TracksDatabase",      // e_chrParamsAnimationNodeWildcardDBA,
    "#filepath",            // e_chrParamsAnimationNodeFilepath,
    "#ParseSubfolders",     // e_chrParamsAnimationNodeParseSubfolders,
    "$Include",             // e_chrParamsAnimationNodeInclude,
    "$FaceLib",             // e_chrParamsAnimationNodeFaceLib,
    "$AnimEventsDatabase",  // e_chrParamsAnimationNodeAnimEventsDB,
};

// Description:
//      This contains the required metadata for all limb IK configurations. Note that all data elements
//      may not be used depending on the solver specified by m_solver.
// Summary:
//      Limb IK code data structure
struct SLimbIKDef
{
    string m_endEffector;               // End effector joint, which will be the joint being "placed"
    string m_handle;                    // Handle, used by specialized IK solutions (e.g. AnimDriven)
    string m_rootJoint;                 // Root joint of the IK chain
    string m_solver;                    // IK solver to be used for this IK chain (e.g. 2BIK, CCDX)
    float m_stepSize;                   // Step size used during IK solving (only used by CCD)
    float m_threshold;                  // Threshold used during IK solving (only used by CCD)
    int m_maxIterations;                // Maximum iterations to used during IK solving (only used by CCDX)
    DynArray<string>m_ccdFiveJoints;    // Joints to use (only used by CCD5)

    SLimbIKDef()
        : m_endEffector("")
        , m_handle("")
        , m_rootJoint("")
        , m_solver("")
        , m_stepSize(0.f)
        , m_threshold(0.f)
        , m_maxIterations(0)
    {}
};

// Description:
//      This contains the required metadata for an animation driven IK definition.
// Summary:
//      Animation driven IK code data structure
struct SAnimDrivenIKDef
{
    string m_handle;        // A limb IK handle indicating which limb IK solver will be used
    string m_targetJoint;   // Joint which will be used as a target point for this solver
    string m_weightJoint;   // Joint which will be used to determine degree of influence for this solver
};

// Description:
//      This contains the required metadata for a recoil impact joint used by the recoil IK definition
// Summary:
//      Recoil impact joint data structure
struct SRecoilImpactJoint
{
    int m_arm;          // Arm id
    float m_delay;      // Delay value
    float m_weight;     // Weight value
    string m_jointName; // Name of the joint
};

// Description:
//      This contains the required metadata for the directional blend used by aim and look IK definitions.
// Summary:
//      Directional blend data structure
struct SAimLookDirectionalBlend
{
    string m_animToken;         // Identifier used by the game
    string m_parameterJoint;    // Joint indicating aiming/looking direction
    string m_startJoint;        // Joint used as relative offset for aiming/looking
    string m_referenceJoint;    // Joint used to indicate entity facing
};

// Description:
//      This contains the required metadata for a position or roation bone for a look or aim IK
//      definition.
// Summary:
//      Position or rotation bone data for an aim or look IK definition
struct SAimLookPosRot
{
    bool m_additive;    // Indicates whether this joint is to be applied additively. (only used for rotation)
    bool m_primary;     // Indicates whether this joint is in the heirarchy between root and the joint
    string m_jointName; // Joint name for this position or rotation joint
    SAimLookPosRot()
        : m_jointName("")
        , m_primary(false)
        , m_additive(false)
    {}
};

// Description:
//      This contains the required metadata for the eye limits for a look IK definition
// Summary:
//      Eye limit data for look IK definition
struct SEyeLimits
{
    float m_halfYaw;    // Indication of yaw from center supported by the eyes
    float m_pitchUp;    // Indication of the pitch up from center supported by the eyes
    float m_pitchDown;  // Indication of the pitch down from center supported by the eyes
    SEyeLimits()
        : m_halfYaw(45.f)
        , m_pitchDown(45.f)
        , m_pitchUp(45.f){}
};

// Description:
//      This contains the required metadata for an animation element for an animation list node
// Summary:
//      Animation node data for animation list
struct SAnimNode
{
    string m_name;                          // The handle that will be used to refer to this animation
                                            // by the engine.
    string m_flags;                         // Flags for this animation (only used by single DBA type)
    string m_path;                          // Path used for this animation node.
    EChrParamsAnimationNodeType m_type;     // Type of this animation node
    SAnimNode(const char* name, const char* path, const char* flags, EChrParamsAnimationNodeType type)
        : m_name(name)
        , m_path(path)
        , m_flags(flags)
        , m_type(type)
    {
    }
};

#endif // CRYINCLUDE_CRYCOMMON_PARAMNODETYPES_H
