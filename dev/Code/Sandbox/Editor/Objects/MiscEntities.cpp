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
#include "MiscEntities.h"
#include "GameEngine.h"

//////////////////////////////////////////////////////////////////////////
// CConstraintEntity
//////////////////////////////////////////////////////////////////////////
static inline void DrawHingeQuad(DisplayContext& dc, float angle)
{
    const float len = 1.0f;
    Vec3 zero(0, 0, 0);
    const float halfLen = len * 0.5f;
    Vec3 vDest(0, len * cos_tpl(angle), len * sin_tpl(angle));
    Vec3 p1 = zero;
    p1.x += halfLen;
    Vec3 p2 = vDest;
    p2.x += halfLen;
    Vec3 p3 = vDest;
    p3.x -= halfLen;
    Vec3 p4 = zero;
    p4.x -= halfLen;
    dc.DrawQuad(p1, p2, p3, p4);
    dc.DrawQuad(p4, p3, p2, p1);
}

void CConstraintEntity::Display(DisplayContext& dc)
{
    // CRYIII-1928: disabled drawing while in AI/Physics mode so it doesn't crash anymore
    if (GetIEditor()->GetGameEngine()->GetSimulationMode())
    {
        return;
    }

    // The pre-allocated array is used as an optimization, trying to avoid the physics system from allocating an entity list.
    const int               nPreAllocatedListSize(1024);                                            //Magic number, probably big enough for the list.
    IPhysicalEntity* pPreAllocatedEntityList[nPreAllocatedListSize];    //Pre-allocated list.
    IPhysicalEntity** pEnts = pPreAllocatedEntityList;                                  // Container for the list of entities.

    CEntityObject::Display(dc);

    // get the entities within the proximity radius of the constraint (only when not simulated)
    Vec3 center(GetWorldPos());
    Vec3 radVec(m_proximityRadius);
    IPhysicalWorld* pWorld = GetIEditor()->GetSystem()->GetIPhysicalWorld();
    // ent_allocate_list is needed to avoid possible crashes, as it could return an internal list (modifiable by other threads).
    // Notice that in case of refactoring, there is the need to call the DeletePointer method before returning from this function, or memory will leak.
    int objtypes = ent_static | ent_rigid | ent_sleeping_rigid | ent_sort_by_mass | ent_allocate_list;
    int nEnts = pWorld->GetEntitiesInBox(center - radVec, center + radVec, pEnts, objtypes, nPreAllocatedListSize);

    m_body0 = NULL;
    m_body1 = NULL;

    if (
        (pEnts)
        &&
        (pEnts != (IPhysicalEntity**)(-1))
        )
    {
        m_body0 = nEnts > 0 ? pEnts[0] : NULL;
        m_body1 = nEnts > 1 ? pEnts[1] : NULL;

        if (m_body0 == (IPhysicalEntity*)-1)
        {
            m_body0 = NULL;
        }

        if (m_body1 == (IPhysicalEntity*)-1)
        {
            m_body1 = NULL;
        }
    }

    // determine the reference frame of the constraint and push it onto the display context stack
    bool useEntityFrame = GetEntityPropertyBool("bUseEntityFrame");
    quaternionf qbody0;
    qbody0.SetIdentity();
    Vec3 posBody0;
    if (m_body0)
    {
        pe_status_pos pos;
        m_body0->GetStatus(&pos);
        qbody0 = pos.q;
        posBody0 = pos.pos;
    }
    quaternionf qbody1;
    qbody1.SetIdentity();
    Vec3 posBody1;
    if (m_body1)
    {
        pe_status_pos pos;
        m_body1->GetStatus(&pos);
        qbody1 = pos.q;
        posBody1 = pos.pos;
    }

    if (!GetIEditor()->GetGameEngine()->GetSimulationMode())
    {
        if (useEntityFrame)
        {
            m_qframe = qbody0;
        }
        else
        {
            m_qframe = quaternionf(GetWorldTM());
        }
        m_qloc0 = !qbody0 * m_qframe;
        m_qloc1 = !qbody1 * m_qframe;
        m_posLoc = !qbody0 * (GetWorldPos() - posBody0);
    }
    quaternionf qframe0 = qbody0 * m_qloc0;
    quaternionf qframe1 = qbody1 * m_qloc1;
    // construct 3 orthonormal vectors using the constraint X axis and the constraint position in body0 space (the link vector)
    Matrix33 rot;
    Vec3 u = qframe0 * Vec3(1, 0, 0);
    Vec3 posFrame = posBody0 + qbody0 * m_posLoc;
    Vec3 l = posFrame - posBody0;
    Vec3 v = l - (l * u) * u;
    v.Normalize();
    Vec3 w = v ^ u;
    rot.SetFromVectors(u, v, w);
    Matrix34 transform(rot);
    transform.SetTranslation(posFrame);
    dc.PushMatrix(transform);

    // X limits (hinge limits)
    float minLimit = GetEntityPropertyFloat("x_min");
    float maxLimit = GetEntityPropertyFloat("x_max");
    if (minLimit < maxLimit)
    {
        // the limits
        dc.SetColor(0.5f, 0, 0.5f);
        DrawHingeQuad(dc, DEG2RAD(minLimit));
        DrawHingeQuad(dc, DEG2RAD(maxLimit));
        // the current X angle
        quaternionf qrel = !qframe1 * qframe0;
        Ang3 angles(qrel);
        dc.SetColor(0, 0, 1);
        DrawHingeQuad(dc, angles.x);
    }
    // YZ limits
    minLimit = GetEntityPropertyFloat("yz_min");
    maxLimit = GetEntityPropertyFloat("yz_max");
    if (maxLimit > 0 &&  minLimit < maxLimit)
    {
        // draw the cone
        float height, radius;
        if (maxLimit == 90)
        {
            height = 0;
            radius = 1;
        }
        else
        {
            const float tanAngle = fabs(tan_tpl(DEG2RAD(maxLimit)));
            if (tanAngle < 1)
            {
                height = 1;
                radius = height * tanAngle;
            }
            else
            {
                radius = 1;
                height = radius / tanAngle;
            }
            if (sin(DEG2RAD(maxLimit)) < 0)
            {
                height = -height;
            }
        }
        const int divs = 20;
        const float delta = 2.0f * gf_PI / divs;
        Vec3 apex(0, 0, 0);
        Vec3 p0(height, radius, 0);
        float angle = delta;
        dc.SetColor(0, 0.5f, 0.5f);
        dc.SetFillMode(e_FillModeWireframe);
        for (int i = 0; i < divs; i++, angle += delta)
        {
            Vec3 p(height, radius * cos_tpl(angle), radius * sin_tpl(angle));
            dc.DrawTri(apex, p0, p);
            dc.DrawTri(apex, p, p0);
            p0 = p;
        }
        dc.SetFillMode(e_FillModeSolid);
    }
    dc.PopMatrix();

    // draw the body 1 constraint X axis
    if (minLimit < maxLimit)
    {
        dc.SetColor(1.0f, 0.0f, 0.0f);
        Vec3 x1 = qbody1 * m_qloc1 * Vec3(1, 0, 0);
        dc.DrawLine(center, center + 2.0f * x1);
    }

    // If we have an entity list pointer, and this list was allocated in the physics system (not our default list)...
    if (
        (pEnts)
        &&
        (pEnts != pPreAllocatedEntityList)
        &&
        (pEnts != (IPhysicalEntity**)(-1))
        )
    {
        // Delete it from the physics system.
        gEnv->pPhysicalWorld->GetPhysUtils()->DeletePointer(pEnts);
    }
}

//////////////////////////////////////////////////////////////////////////
void CWindAreaEntity::Display(DisplayContext& dc)
{
    CEntityObject::Display(dc);

    IEntity* pEntity = GetIEntity();
    if (pEntity == NULL)
    {
        return;
    }

    IPhysicalEntity* pPhysEnt = pEntity->GetPhysics();
    if (pPhysEnt == NULL || pPhysEnt->GetType() != PE_AREA)
    {
        return;
    }

    pe_status_pos pos;
    if (pPhysEnt->GetStatus(&pos) == 0)
    {
        return;
    }

    Vec3 samples[8][8][8];
    QuatT transform(pos.pos, pos.q);
    AABB bbox = AABB(pos.BBox[0], pos.BBox[1]);
    float frameTime = gEnv->pTimer->GetCurrTime();
    float theta = frameTime - floor(frameTime);

    float len[3] =
    {
        fabs_tpl(bbox.min.x - bbox.max.x),
        fabs_tpl(bbox.min.y - bbox.max.y),
        fabs_tpl(bbox.min.z - bbox.max.z)
    };

    pe_status_area area;
    for (size_t i = 0; i < 8; ++i)
    {
        for (size_t j = 0; j < 8; ++j)
        {
            for (size_t  k = 0; k < 8; ++k)
            {
                area.ctr = transform * Vec3(
                        bbox.min.x + i * (len[0] / 8.f)
                        , bbox.min.y + j * (len[1] / 8.f)
                        , bbox.min.z + k * (len[2] / 8.f));
                samples[i][j][k] = (pPhysEnt->GetStatus(&area)) ? area.pb.waterFlow : Vec3(0, 0, 0);
            }
        }
    }

    for (size_t i = 0; i < 8; ++i)
    {
        for (size_t j = 0; j < 8; ++j)
        {
            for (size_t  k = 0; k < 8; ++k)
            {
                Vec3 ctr = transform * Vec3(
                        bbox.min.x + i * (len[0] / 8.f),
                        bbox.min.y + j * (len[1] / 8.f),
                        bbox.min.z + k * (len[2] / 8.f));
                Vec3 dir = samples[i][j][k].GetNormalized();
                dc.SetColor(Col_Red);
                dc.DrawArrow(ctr, ctr + dir, 1.f);
            }
        }
    }
}

#include <Objects/MiscEntities.moc>