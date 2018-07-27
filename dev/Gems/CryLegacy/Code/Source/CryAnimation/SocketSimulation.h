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

#ifndef _CRY_SOCKETSIM
#define _CRY_SOCKETSIM

#include "Skeleton.h"
class CAttachmentManager;

struct CSimulation
    : public SimulationParams
{
    CSimulation()
    {
        m_crcProcFunction = 0;
        m_idxDirTransJoint = -1;
        m_nAnimOverrideJoint = -1;
        m_fHRotationHCosX = DEG2HCOS(m_vDiskRotation.x);
        m_fHRotationHCosZ = DEG2HCOS(m_vDiskRotation.y);
        m_fMaxAngleCos    = DEG2COS (m_fMaxAngle);
        m_fMaxAngleHCos   = DEG2HCOS(m_fMaxAngle);
        m_aa1c = 1, m_aa1s = 0, m_aa2c = 1, m_aa2s = 0;

        //dynamic members
        m_vBobPosition.zero();
        m_vBobVelocity.zero();
        m_vLocationPrev.SetIdentity();
        m_vAttModelRelativePrev = Vec3(ZERO);
        m_crcProcFunction = 0;
    };

    void PostUpdate(const CAttachmentManager* pAttachmentManager, const char* pJointName);
    void UpdateSimulation(const CAttachmentManager* pAttachmentManager, Skeleton::CPoseData& rPoseData, int32 nRedirectionID, QuatT& rAttModelRelative, QuatT& raddTransformation, const char* pAttName);
    void UpdatePendulumSimulation(const CAttachmentManager* pAttachmentManager, Skeleton::CPoseData& rPoseData, int32 nRedirectionID, const QuatT& rAttModelRelative, QuatT& raddTransformation, const char* pAttName);
    void UpdateSpringSimulation  (const CAttachmentManager* pAttachmentManager, Skeleton::CPoseData& rPoseData, int32 nRedirectionID, const QuatT& rAttModelRelative, Vec3& raddTranslation, const char* pAttName);
    void ProjectJointOutOfLozenge(const CAttachmentManager* pAttachmentManager, Skeleton::CPoseData& rPoseData, int32 nRedirectionID, const QuatT& rAttModelRelative, const char* pAttName);
    void Draw(const QuatTS& qt, const ColorB clr, const uint32 tesselation, const Vec3& vdir);

    //these members store the optimized parameters
    uint32 m_crcProcFunction;
    int16  m_idxDirTransJoint;
    int16  m_nAnimOverrideJoint;
    f32    m_fMaxAngleCos, m_fMaxAngleHCos;      //cosine and half-cosine for the angle
    f32    m_fHRotationHCosX, m_fHRotationHCosZ; //yaw and pitch  half-cosine

    f32    m_aa1c, m_aa1s, m_aa2c, m_aa2s;     //half-cosines of yaw & pitch
    DynArray<int16>  m_arrProxyIndex;      //the indices to the proxies (atm only lozenges)
    DynArray<JointIdType> m_arrChildren;

    //dynamic members
    Vec3 m_vBobPosition; //this is a world-position
    Vec3 m_vBobVelocity;
    Vec3 m_vAttModelRelativePrev;
    QuatT m_vLocationPrev;         //location from last frame
};


#endif
