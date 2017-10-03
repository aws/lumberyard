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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_MISCENTITIES_H
#define CRYINCLUDE_EDITOR_OBJECTS_MISCENTITIES_H
#pragma once


#include "EntityObject.h"

//////////////////////////////////////////////////////////////////////////
// WindArea entity.
//////////////////////////////////////////////////////////////////////////
class CWindAreaEntity
    : public CEntityObject
{
    Q_OBJECT
public:

    //////////////////////////////////////////////////////////////////////////
    CWindAreaEntity(){ }

    static const GUID& GetClassID()
    {
        // {5F7F60EC-8894-498E-B43F-50CDB6957B63}
        static const GUID guid = {
            0x5f7f60ec, 0x8894, 0x498e, { 0xb4, 0x3f, 0x50, 0xcd, 0xb6, 0x95, 0x7b, 0x63 }
        };
        return guid;
    }

    virtual void Display(DisplayContext& disp);

private:
};

// Constraint entity.
//////////////////////////////////////////////////////////////////////////
class CConstraintEntity
    : public CEntityObject
{
    Q_OBJECT
public:
    //////////////////////////////////////////////////////////////////////////
    CConstraintEntity()
        : m_body0(NULL)
        , m_body1(NULL) { }

    static const GUID& GetClassID()
    {
    // {5F7F60EC-8894-498E-B43F-50CDB6957B63}
        static const GUID guid = {
            0x3b94f399, 0x04a5, 0x432e, { 0x8e, 0x8d, 0xa9, 0xeb, 0xfd, 0x9f, 0x4c, 0x5d }
        };
        return guid;
    }
  

	//////////////////////////////////////////////////////////////////////////
    virtual void Display(DisplayContext& disp);

private:
    // the orientation of the constraint in world, body0 and body1 space
    quaternionf m_qframe, m_qloc0, m_qloc1;

    // the position of the constraint in world space
    Vec3 m_posLoc;

    // pointers to the two joined bodies
    IPhysicalEntity* m_body0, * m_body1;
};

// GeomCache entity.
class SANDBOX_API CGeomCacheEntity
    : public CEntityObject
{
    Q_OBJECT
public:
    CGeomCacheEntity() {}

    static const GUID& GetClassID()
    {
		// {3b94f399-04a5-432e-8e8d-a9ebfd9f4c5d}
        static const GUID guid = {
            0xb176e924, 0xd009, 0x4a71, { 0xa9, 0x47, 0x3, 0xc9, 0x94, 0xa2, 0xb1, 0x6f }
        };
        return guid;
    }

    virtual bool HitTestEntity(HitContext& hc, bool& bHavePhysics) override
    {
#if defined(USE_GEOM_CACHES)
        IGeomCacheRenderNode* pGeomCacheRenderNode = m_pEntity->GetGeomCacheRenderNode(0);
        if (pGeomCacheRenderNode)
        {
            SRayHitInfo hitInfo;
            ZeroStruct(hitInfo);
            hitInfo.inReferencePoint = hc.raySrc;
            hitInfo.inRay = Ray(hitInfo.inReferencePoint, hc.rayDir.GetNormalized());
            hitInfo.bInFirstHit = false;
            hitInfo.bUseCache = false;

            if (pGeomCacheRenderNode->RayIntersection(hitInfo))
            {
                hc.object = this;
                hc.dist = hitInfo.fDistance;
                return true;
            }
        }
#endif
        return false;
    }
};


#endif // CRYINCLUDE_EDITOR_OBJECTS_MISCENTITIES_H
