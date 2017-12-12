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
#include "VertexList.h"
#include "AILog.h"
#include <ISystem.h>
#include <CryFile.h>

#define BAI_VERTEX_FILE_VERSION 1


CVertexList::CVertexList()
{
    m_hashSpace = new CHashSpace<SVertexRecord, VertexHashSpaceTraits>(Vec3(7, 7, 7), 8192, VertexHashSpaceTraits(*this));
    m_obstacles.clear();
}

CVertexList::~CVertexList()
{
    delete m_hashSpace;
}

int CVertexList::FindVertex(const ObstacleData& od) const
{
    const Obstacles::const_iterator oiend = m_obstacles.end();
    int index = 0;

    for (Obstacles::const_iterator oi = m_obstacles.begin(); oi != oiend; ++oi, ++index)
    {
        if ((*oi) == od)
        {
            return index;
        }
    }

    return -1;
}

int CVertexList::AddVertex(const ObstacleData& od)
{
    int index = FindVertex(od);
    if (index < 0)
    {
        m_obstacles.push_back(od);
        index = (int)m_obstacles.size() - 1;
        m_hashSpace->AddObject(SVertexRecord(index));
    }

    return index;
}


const ObstacleData& CVertexList::GetVertex(int index) const
{
    if (!IsIndexValid(index))
    {
        AIError("CVertexList::GetVertex Tried to retrieve a non existing vertex (%d) from vertex list (size %ul).Please regenerate the triangulation [Design bug]",
            index, m_obstacles.size());
        return m_obstacles[0];
    }
    return m_obstacles[index];
}

ObstacleData& CVertexList::ModifyVertex(int index)
{
    if (!IsIndexValid(index))
    {
        AIError("CVertexList::ModifyVertex Tried to retrieve a non existing vertex (%d) from vertex list (size %ul).Please regenerate the triangulation [Design bug]",
            index, m_obstacles.size());
        static ObstacleData od;
        return od;
    }
    return m_obstacles[index];
}


void CVertexList::WriteToFile(const char* fileName)
{
    CCryFile file;
    if (false != file.Open(fileName, "wb"))
    {
        int nFileVersion = BAI_VERTEX_FILE_VERSION;
        file.Write(&nFileVersion, sizeof(int));

        int iNumber = (int)m_obstacles.size();
        std::vector<ObstacleDataDesc> obDescs;
        obDescs.resize(iNumber);
        for (int i = 0; i < iNumber; ++i)
        {
            obDescs[i].vPos = m_obstacles[i].vPos;
            obDescs[i].vDir = m_obstacles[i].vDir;
            obDescs[i].fApproxRadius = m_obstacles[i].fApproxRadius;
            obDescs[i].flags = m_obstacles[i].flags;
            obDescs[i].approxHeight = m_obstacles[i].approxHeight;
        }
        file.Write(&iNumber, sizeof(int));
        if (!iNumber)
        {
            return;
        }
        file.Write(&obDescs[ 0 ], iNumber * sizeof(ObstacleDataDesc));
        file.Close();
    }
}

bool CVertexList::ReadFromFile(const char* fileName)
{
    m_obstacles.clear();
    m_hashSpace->Clear(true);


    CCryFile file;
    if (!file.Open(fileName, "rb"))
    {
        AIError("CVertexList::ReadFromFile could not open vertex file: Regenerate triangulation in the editor [Design bug]");
        return false;
    }

    int iNumber;

    AILogLoading("Verifying BAI file version");
    file.ReadType(&iNumber);
    if (iNumber != BAI_VERTEX_FILE_VERSION)
    {
        AIError("CVertexList::ReadFromFile Wrong vertex list BAI file version - found %d expected %d: Regenerate triangulation in the editor [Design bug]", iNumber, BAI_VERTEX_FILE_VERSION);
        file.Close();
        return false;
    }

    // Read number of descriptors.
    file.ReadType(&iNumber);

    if (iNumber > 0)
    {
        std::vector<ObstacleDataDesc> obDescs(iNumber);
        file.ReadType(&obDescs[0], iNumber);
        m_obstacles.resize(iNumber);
        for (int i = 0; i < iNumber; ++i)
        {
            m_obstacles[i].vPos = obDescs[i].vPos;
            m_obstacles[i].vDir = obDescs[i].vDir;
            m_obstacles[i].fApproxRadius = obDescs[i].fApproxRadius;
            m_obstacles[i].flags = obDescs[i].flags;
            m_obstacles[i].approxHeight = obDescs[i].approxHeight;
            m_hashSpace->AddObject(SVertexRecord(i));
        }
    }

    file.Close();

    return true;
}

//===================================================================
// GetVerticesInRange
//===================================================================
void CVertexList::GetVerticesInRange(std::vector<std::pair<float, unsigned> >& vertsOut, const Vec3& pos, float range, unsigned char flags)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
    vertsOut.clear();
    SVertCollector collector(*this, vertsOut, flags);
    m_hashSpace->ProcessObjectsWithinRadius(pos, range, collector);
}


//===================================================================
// Reset
//===================================================================
void CVertexList::Reset()
{
    const Obstacles::iterator oiend = m_obstacles.end();
    for (Obstacles::iterator oi = m_obstacles.begin(); oi != oiend; ++oi)
    {
        ObstacleData& od = *oi;
        od.ClearNavNodes();
    }
}
