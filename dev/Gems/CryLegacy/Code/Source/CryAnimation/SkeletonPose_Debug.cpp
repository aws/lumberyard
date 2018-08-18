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

#include "CharacterManager.h"
#include "CharacterInstance.h"
#include "Model.h"
#include "DrawHelper.h"


static const char* strstri(const char* s1, const char* s2)
{
    int i, j, k;
    for (i = 0; s1[i]; i++)
    {
        for (j = i, k = 0; tolower(s1[j]) == tolower(s2[k]); j++, k++)
        {
            if (!s2[k + 1])
            {
                return (s1 + i);
            }
        }
    }

    return NULL;
}

void CSkeletonPose::DrawBBox(const Matrix34& rRenderMat34)
{
    OBB obb = OBB::CreateOBBfromAABB(Matrix33(rRenderMat34), m_AABB);
    Vec3 wpos = rRenderMat34.GetTranslation();
    g_pAuxGeom->DrawOBB(obb, wpos, 0, RGBA8(0xff, 0x00, 0x1f, 0xff), eBBD_Extremes_Color_Encoded);
}


void CSkeletonPose::DrawPose(const Skeleton::CPoseData& pose, const Matrix34& rRenderMat34)
{
    g_pAuxGeom->SetRenderFlags(e_Def3DPublicRenderflags | e_DepthTestOff);

    Vec3 vdir = m_pInstance->m_Viewdir;
    CDefaultSkeleton* pDefaultSkeleton = m_pInstance->m_pDefaultSkeleton;

    static Ang3 ang_root(0, 0, 0);
    ang_root += Ang3(0.01f, 0.02f, 0.03f);
    static Ang3 ang_rot(0, 0, 0);
    ang_rot += Ang3(-0.01f, +0.02f, -0.03f);
    static Ang3 ang_pos(0, 0, 0);
    ang_pos += Ang3(-0.03f, -0.02f, -0.01f);

    f32 fUnitScale = m_pInstance->GetUniformScale();
    const QuatT* pJointsAbsolute = pose.GetJointsAbsolute();
    const JointState* pJointState = pose.GetJointsStatus();
    uint32 jointCount = pose.GetJointCount();


    string filterText;

    ICVar* pVar = gEnv->pSystem->GetIConsole()->GetCVar("ca_filterJoints");
    if (pVar)
    {
        filterText = pVar->GetString();
    }

    bool filtered = filterText != "";
    const char* jointName;

    for (uint32 i = 0; i < jointCount; ++i)
    {
        if (filtered)
        {
            jointName = m_pInstance->m_pDefaultSkeleton->m_arrModelJoints[i].m_strJointName.c_str();

            if (strstri(jointName, filterText) == 0)
            {
                continue;
            }
        }


        int32 parentIndex = m_pInstance->m_pDefaultSkeleton->m_arrModelJoints[i].m_idxParent;
        if (parentIndex > -1)
        {
            g_pAuxGeom->DrawBone(rRenderMat34 * pJointsAbsolute[parentIndex].t, rRenderMat34 * pJointsAbsolute[i].t, RGBA8(0xff, 0xff, 0xef, 0xc0));
        }

        f32 scale = 0.7f;
        AABB aabb_rot       = AABB(Vec3(-0.011f, -0.011f, -0.011f) * scale, Vec3(+0.011f, +0.011f, +0.011f) * scale);
        OBB obb_rot         =   OBB::CreateOBBfromAABB(Matrix33::CreateRotationXYZ(ang_rot), aabb_rot);
        if (pJointState[i] & eJS_Orientation)
        {
            g_pAuxGeom->DrawOBB(obb_rot, rRenderMat34 * pJointsAbsolute[i].t, 0, RGBA8(0xff, 0x00, 0x00, 0xff), eBBD_Extremes_Color_Encoded);
        }

        AABB aabb_pos       = AABB(Vec3(-0.010f, -0.010f, -0.010f) * scale, Vec3(+0.010f, +0.010f, +0.010f) * scale);
        OBB obb_pos         =   OBB::CreateOBBfromAABB(Matrix33::CreateRotationXYZ(ang_pos), aabb_pos);
        if (pJointState[i] & eJS_Position)
        {
            g_pAuxGeom->DrawOBB(obb_pos, rRenderMat34 * pJointsAbsolute[i].t, 0, RGBA8(0x00, 0xff, 0x00, 0xff), eBBD_Extremes_Color_Encoded);
        }

        DrawHelper::Frame(rRenderMat34 * Matrix34(pJointsAbsolute[i]), Vec3(0.15f * fUnitScale));
    }
}

ILINE ColorB shader(Vec3 n, Vec3 d0, Vec3 d1, ColorB c)
{
    f32 a = 0.5f * n | d0, b = 0.1f * n | d1, l = min(1.0f, fabs_tpl(a) - a + fabs_tpl(b) - b + 0.05f);
    return RGBA8(uint8(l * c.r), uint8(l * c.g), uint8(l * c.b), c.a);
}


void CSkeletonPose::DrawBone(const Vec3& p, const Vec3& c, ColorB clr, const Vec3& vdir)
{
    const f32 fAxisLen = (p - c).GetLength();
    if (fAxisLen < 0.001f)
    {
        return;
    }

    const Vec3 vAxisNormalized = (p - c).GetNormalized();
    QuatTS qt(Quat::CreateRotationV0V1(Vec3(1, 0, 0), vAxisNormalized), p, 1.0f);

    ColorB clr2 = clr;

    f32 cr = 0.01f + fAxisLen / 50.0f;
    uint32 subdiv = 4 * 4;
    f32 rad = gf_PI * 2.0f / f32(subdiv);
    const float cos15 = cos_tpl(rad), sin15 = sin_tpl(rad);

    Vec3 pt0, pt1, pt2, pt3;
    ColorB pclrlit0, pclrlit1, pclrlit2, pclrlit3;
    ColorB nclrlit0, nclrlit1, nclrlit2, nclrlit3;

    Matrix34 m34 = Matrix34(qt);
    Matrix33 m33 = Matrix33(m34);

    //  Matrix34 t34p; t34p.SetTranslationMat(Vec3( 0,0,0));

    Matrix34 t34m;
    t34m.SetTranslationMat(Vec3(-fAxisLen, 0, 0));
    Matrix34 m34p = m34;
    Matrix34 m34m = m34 * t34m;
    Matrix34 r34m = m34 * t34m;
    r34m.m00 = -r34m.m00;
    r34m.m10 = -r34m.m10;
    r34m.m20 = -r34m.m20;
    Matrix33 r33 = Matrix33(r34m);

    Vec3 ldir0 = vdir;
    ldir0.z = 0;
    (ldir0 = ldir0.normalized() * 0.5f).z = f32(sqrt3 * -0.5f);
    Vec3 ldir1 = Vec3(0, 0, 1);

    f32 x = cos15, y = sin15;
    pt0 = Vec3(0, 0, cr);
    pclrlit0 = shader(m33 * Vec3(0, 0, 1), ldir0, ldir1, clr2);
    f32 s = 0.20f;
    Vec3 _p[0x4000];
    ColorB _c[0x4000];
    uint32 m_dwNumFaceVertices = 0;
    for (uint32 i = 0; i < subdiv; i++)
    {
        pt1 = Vec3(0, -y * cr, x * cr);
        pclrlit1 = shader(m33 * Vec3(0, -y, x), ldir0, ldir1, clr2);
        Vec3 x0 = m34m * Vec3(pt0.x * s, pt0.y * s, pt0.z * s);
        Vec3 x1 = m34p * pt0;
        Vec3 x3 = m34m * Vec3(pt1.x * s, pt1.y * s, pt1.z * s);
        Vec3 x2 = m34p * pt1;
        _p[m_dwNumFaceVertices] = x1;
        _c[m_dwNumFaceVertices++] = pclrlit0;
        _p[m_dwNumFaceVertices] = x2;
        _c[m_dwNumFaceVertices++] = pclrlit1;
        _p[m_dwNumFaceVertices] = x3;
        _c[m_dwNumFaceVertices++] = pclrlit1;

        _p[m_dwNumFaceVertices] = x3;
        _c[m_dwNumFaceVertices++] = pclrlit1;
        _p[m_dwNumFaceVertices] = x0;
        _c[m_dwNumFaceVertices++] = pclrlit0;
        _p[m_dwNumFaceVertices] = x1;
        _c[m_dwNumFaceVertices++] = pclrlit0;
        f32 t = x;
        x = x * cos15 - y * sin15;
        y = y * cos15 + t * sin15;
        pt0 = pt1, pclrlit0 = pclrlit1;
    }

    f32 cost = 1, sint = 0, costup = cos15, sintup = sin15;
    for (uint32 j = 0; j < subdiv / 4; j++)
    {
        Vec3 n = Vec3(sint, 0, cost);
        pt0 = n * cr;
        pclrlit0 = shader(m33 * n, ldir0, ldir1, clr);
        nclrlit0 = shader(r33 * n, ldir0, ldir1, clr);
        n = Vec3(sintup, 0, costup);
        pt2 = n * cr;
        pclrlit2 = shader(m33 * n, ldir0, ldir1, clr);
        nclrlit2 = shader(r33 * n, ldir0, ldir1, clr);

        x = cos15, y = sin15;
        for (uint32 i = 0; i < subdiv; i++)
        {
            n = Vec3(0, -y, x) * costup, n.x += sintup;
            pt3 = n * cr;
            pclrlit3 = shader(m33 * n, ldir0, ldir1, clr);
            nclrlit3 = shader(r33 * n, ldir0, ldir1, clr);
            n = Vec3(0, -y, x) * cost, n.x += sint;
            pt1 = n * cr;
            pclrlit1 = shader(m33 * n, ldir0, ldir1, clr);
            nclrlit1 = shader(r33 * n, ldir0, ldir1, clr);

            Vec3 v0 = m34p * pt0;
            Vec3 v1 = m34p * pt1;
            Vec3 v2 = m34p * pt2;
            Vec3 v3 = m34p * pt3;
            _p[m_dwNumFaceVertices] = v1;
            _c[m_dwNumFaceVertices++] = pclrlit1;
            _p[m_dwNumFaceVertices] = v0;
            _c[m_dwNumFaceVertices++] = pclrlit0;
            _p[m_dwNumFaceVertices] = v2;
            _c[m_dwNumFaceVertices++] = pclrlit2;
            _p[m_dwNumFaceVertices] = v3;
            _c[m_dwNumFaceVertices++] = pclrlit3;
            _p[m_dwNumFaceVertices] = v1;
            _c[m_dwNumFaceVertices++] = pclrlit1;
            _p[m_dwNumFaceVertices] = v2;
            _c[m_dwNumFaceVertices++] = pclrlit2;

            Vec3 w0 = r34m * (pt0 * s);
            Vec3 w1 = r34m * (pt1 * s);
            Vec3 w3 = r34m * (pt3 * s);
            Vec3 w2 = r34m * (pt2 * s);
            _p[m_dwNumFaceVertices] = w0;
            _c[m_dwNumFaceVertices++] = pclrlit0;
            _p[m_dwNumFaceVertices] = w1;
            _c[m_dwNumFaceVertices++] = pclrlit1;
            _p[m_dwNumFaceVertices] = w2;
            _c[m_dwNumFaceVertices++] = pclrlit2;
            _p[m_dwNumFaceVertices] = w1;
            _c[m_dwNumFaceVertices++] = pclrlit1;
            _p[m_dwNumFaceVertices] = w3;
            _c[m_dwNumFaceVertices++] = pclrlit3;
            _p[m_dwNumFaceVertices] = w2;
            _c[m_dwNumFaceVertices++] = pclrlit2;
            f32 t = x;
            x = x * cos15 - y * sin15;
            y = y * cos15 + t * sin15;
            pt0 = pt1, pt2 = pt3;
            pclrlit0 = pclrlit1, pclrlit2 = pclrlit3;
            nclrlit0 = nclrlit1, nclrlit2 = nclrlit3;
        }
        cost = costup;
        sint = sintup;
        costup = cost * cos15 - sint * sin15;
        sintup = sint * cos15 + cost * sin15;
    }


    uint32 rflag = e_Mode3D | e_AlphaNone | e_DrawInFrontOff | e_FillModeSolid | e_CullModeNone | e_DepthWriteOn | e_DepthTestOn;
    SAuxGeomRenderFlags renderFlags(rflag);
    g_pAuxGeom->SetRenderFlags(renderFlags);

    g_pAuxGeom->DrawTriangles(&_p[0], m_dwNumFaceVertices, &_c[0]);
    g_pAuxGeom->DrawLine(p, clr,   c, clr);
}


void CSkeletonPose::DrawSkeleton(const Matrix34& rRenderMat34, uint32 shift)
{
    DrawPose(GetPoseData(), rRenderMat34);
}



f32 CSkeletonPose::SecurityCheck()
{
    f32 fRadius = 0.0f;
    uint32 numJoints = GetPoseData().GetJointCount();
    for (uint32 i = 0; i < numJoints; i++)
    {
        f32 t = GetPoseData().GetJointAbsolute(i).t.GetLength();
        if (fRadius < t)
        {
            fRadius = t;
        }
    }
    return fRadius;
}

uint32 CSkeletonPose::IsSkeletonValid()
{
    uint32 numJoints = GetPoseData().GetJointCount();
    for (uint32 i = 0; i < numJoints; i++)
    {
        uint32 valid = GetPoseData().GetJointAbsolute(i).t.IsValid();
        if (valid == 0)
        {
            return 0;
        }
        if (fabsf(GetPoseData().GetJointAbsolute(i).t.x) > 20000.0f)
        {
            return 0;
        }
        if (fabsf(GetPoseData().GetJointAbsolute(i).t.y) > 20000.0f)
        {
            return 0;
        }
        if (fabsf(GetPoseData().GetJointAbsolute(i).t.z) > 20000.0f)
        {
            return 0;
        }
    }
    return 1;
}

size_t CSkeletonPose::SizeOfThis()
{
    size_t TotalSize = 0;

    TotalSize += GetPoseData().GetAllocationLength();

    TotalSize += m_FaceAnimPosSmooth.get_alloc_size();
    TotalSize += m_FaceAnimPosSmoothRate.get_alloc_size();

    TotalSize += m_arrCGAJoints.get_alloc_size();

    TotalSize += m_physics.SizeOfThis();

    return TotalSize;
}

void CSkeletonPose::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(m_poseData);

    pSizer->AddObject(m_FaceAnimPosSmooth);
    pSizer->AddObject(m_FaceAnimPosSmoothRate);

    pSizer->AddObject(m_arrCGAJoints);
    pSizer->AddObject(m_limbIk);

    pSizer->AddObject(m_PoseBlenderAim);
    pSizer->AddObject(m_PoseBlenderLook);

    pSizer->AddObject(m_physics);
}
