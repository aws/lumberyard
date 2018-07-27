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

// Description : The AreaSolid has most general functions for an area object


#ifndef CRYINCLUDE_CRYENTITYSYSTEM_AREASOLID_H
#define CRYINCLUDE_CRYENTITYSYSTEM_AREASOLID_H
#pragma once


#include "AreaUtil.h"

class CBSPTree3D;
class CSegmentSet;

class CAreaSolid
{
public:

    enum ESegmentType
    {
        eSegmentType_Open,
        eSegmentType_Close,
    };

    enum ESegmentQueryFlag
    {
        eSegmentQueryFlag_Obstruction = 0x0001,
        eSegmentQueryFlag_Open = 0x0002,
        eSegmentQueryFlag_All = eSegmentQueryFlag_Obstruction | eSegmentQueryFlag_Open,

        eSegmentQueryFlag_UsingReverseSegment = 0x0010
    };

public:

    CAreaSolid();
    ~CAreaSolid()
    {
        Clear();
    }

    void AddRef()
    {
        CryInterlockedIncrement(&m_nRefCount);
    }
    void Release()
    {
        if (CryInterlockedDecrement(&m_nRefCount) <= 0)
        {
            delete this;
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////
    bool QueryNearest(const Vec3& vPos, int queryFlag, Vec3& outNearestPos, float& outNearestDistance) const;
    bool IsInside(const Vec3& vPos) const;
    void Draw(const Matrix34& worldTM, const ColorB& color0, const ColorB& color1) const;

    /////////////////////////////////////////////////////////////////////////////////////////////
    void AddSegment(const Vec3* verticesOfConvexhull, bool bObstruction, int numberOfPoints);
    void BuildBSP();

    ////////////////////////////////////////////////////////////////////////////////////////////
    const AABB& GetBoundBox()
    {
        return m_BoundBox;
    }

    void GetMemoryUsage(ICrySizer* pSizer) const;

private:

    void Clear();

    static bool IsQueryTypeIdenticalToSegmentType(int queryFlag, ESegmentType segmentType)
    {
        if ((queryFlag & eSegmentQueryFlag_All) != eSegmentQueryFlag_All)
        {
            if ((queryFlag & eSegmentQueryFlag_Obstruction) && segmentType != eSegmentType_Close)
            {
                return false;
            }
            if ((queryFlag & eSegmentQueryFlag_Open) && segmentType != eSegmentType_Open)
            {
                return false;
            }
        }
        return true;
    }

private:

    CBSPTree3D* m_BSPTree;
    std::vector<CSegmentSet*> m_SegmentSets;
    AABB m_BoundBox;
    int m_nRefCount;
};

#endif // CRYINCLUDE_CRYENTITYSYSTEM_AREASOLID_H
