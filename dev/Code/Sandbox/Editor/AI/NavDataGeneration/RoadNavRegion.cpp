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
#include "RoadNavRegion.h"
#include "TriangularNavRegion.h"
#include "ISerialize.h"
#include "CryFile.h"

#include <limits>

#define BAI_ROADS_FILE_VERSION 2

static IAISystem::tNavCapMask roadNavTypes = IAISystem::NAV_TRIANGULAR | IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_VOLUME;

//====================================================================
// CRoadNavRegion
//====================================================================
CRoadNavRegion::CRoadNavRegion(CGraph* pGraph)
{
    AIAssert(pGraph);
    m_pGraph = pGraph;
}

//====================================================================
// CRoadNavRegion
//====================================================================
CRoadNavRegion::~CRoadNavRegion()
{
    Clear();
}

//====================================================================
// GetEnclosing
//====================================================================
unsigned CRoadNavRegion::GetEnclosing(const Vec3& pos, float passRadius, unsigned startIndex,
    float /*range*/, Vec3* closestValid, bool returnSuspect, const char* requesterName)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

    if (!m_pGraph || m_roads.empty())
    {
        return 0;
    }

    for (tRoads::iterator roadIt = m_roads.begin(); roadIt != m_roads.end(); ++roadIt)
    {
        tNodes& nodes = roadIt->second;
        unsigned nNodes = nodes.size();
        for (unsigned i = 0; i < nNodes - 1; ++i)
        {
            SRoadNode& node = nodes[i];
            SRoadNode& nodeNext = nodes[i + 1];
            if (node.nodeIndex && nodeNext.nodeIndex)
            {
                float t;
                float d = Distance::Point_Lineseg(pos, Lineseg(node.pos, nodeNext.pos), t);
                if (d < 0.5f * node.width && t > 0.0f && t < 1.0f)
                {
                    if (t < 0.5f)
                    {
                        return node.nodeIndex;
                    }
                    else
                    {
                        return nodeNext.nodeIndex;
                    }
                }
            }
        }
    }
    return 0;
}

//====================================================================
// Serialize
//====================================================================
void CRoadNavRegion::Serialize(TSerialize ser)
{
    ser.BeginGroup("RoadNavRegion");

    ser.EndGroup();
}

//====================================================================
// Reset
//====================================================================
void CRoadNavRegion::Clear()
{
    while (!m_originalRoads.empty())
    {
        DeleteRoad(m_originalRoads.begin()->first);
    }
}

//====================================================================
// GetSements
//====================================================================
void CRoadNavRegion::GetSegments(const Vec3& p0, const Vec3& p1, std::vector<Vec3>& pts) const
{
    pts.resize(0);

    static float minSegDist = 4.0f;
    static int maxPts = 100;

    Vec3 delta = p1 - p0;
    Vec3 deltaH(delta.x, delta.y, 0.0f);
    float dist = deltaH.GetLength();
    int nPts = 2 + (int) (dist / minSegDist);
    if (nPts > maxPts)
    {
        nPts = maxPts;
    }
    for (int i = 0; i < nPts; ++i)
    {
        float frac = i / (nPts - 1.0f);
        pts.push_back((1.0f - frac) * p0 + frac * p1);
    }
}

//====================================================================
// RemoveRoadNodes
//====================================================================
CRoadNavRegion::tRoads::iterator CRoadNavRegion::RemoveRoadNodes(const string& roadName)
{
    AIAssert(m_pGraph);
    tRoads::iterator roadIt = m_roads.find(roadName);
    if (roadIt == m_roads.end())
    {
        return m_roads.end();
    }

    tNodes& nodes = roadIt->second;
    unsigned nNodes = nodes.size();
    for (unsigned i = 0; i < nNodes; ++i)
    {
        SRoadNode& node = nodes[i];
        if (node.nodeIndex)
        {
            m_pGraph->Disconnect(node.nodeIndex, true);
        }
        node.nodeIndex = 0;
    }
    tRoads::iterator nextIt = roadIt;
    ++nextIt;
    m_roads.erase(roadIt);
    return nextIt;
}

//====================================================================
// SetLinkParams
//====================================================================
static void SetLinkParams(CGraphLinkManager& linkManager, unsigned l1, unsigned l2)
{
    if (l1)
    {
        linkManager.SetExposure(l1, 1.0f);
        linkManager.SetMaxWaterDepth(l1, -9999.0f);
        linkManager.SetMinWaterDepth(l1, 9999.0f);
    }
    if (l2)
    {
        linkManager.SetExposure(l2, 1.0f);
        linkManager.SetMaxWaterDepth(l2, -9999.0f);
        linkManager.SetMinWaterDepth(l2, 9999.0f);
    }
}

//====================================================================
// CreateRoadNodes
//====================================================================
void CRoadNavRegion::CreateRoadNodes(const string& roadName)
{
    tRoads::iterator origRoadIt = m_originalRoads.find(roadName);
    if (origRoadIt == m_originalRoads.end())
    {
        return;
    }

    // cut m_originalRoads against the triangulation to generate m_roads
    tNodes& nodes = m_roads[roadName]; // inserts a new one
    if (!nodes.empty())
    {
        RemoveRoadNodes(roadName);
    }

    tNodes& origNodes = origRoadIt->second;
    unsigned nOrigNodes = origNodes.size();

    std::vector<Vec3> pts;
    /// Each segment we calculate we don't add the last point - we add that right at the end
    for (unsigned i = 1; i < nOrigNodes; ++i)
    {
        GetSegments(origNodes[i - 1].pos, origNodes[i].pos, pts);
        // now add these
        const SRoadNode& origNode = origNodes[i];
        for (unsigned j = 0; j < pts.size() - 1; ++j)
        {
            nodes.push_back(SRoadNode(pts[j], origNode.width, origNode.offset));
        }
    }
    nodes.push_back(origNodes[nOrigNodes - 1]);

    unsigned nNodes = nodes.size();
    unsigned lastNodeIndex = 0;
    unsigned lastTriNodeIndex = 0;
    for (unsigned i = 0; i < nNodes; ++i)
    {
        SRoadNode& node = nodes[i];
        if (node.nodeIndex == 0)
        {
            node.nodeIndex = m_pGraph->CreateNewNode(IAISystem::NAV_ROAD, node.pos);
            AIAssert(node.nodeIndex);
            m_pGraph->GetNodeManager().GetNode(node.nodeIndex)->GetRoadNavData()->fRoadWidth = node.width;
            //node.pNode->GetRoadNavData()->fRoadOffset = 0.0f; /*node.offset*/

            if (lastNodeIndex)
            {
                unsigned linkOneTwo = 0;
                unsigned linkTwoOne = 0;
                // connect em up
                float width = min(node.width, m_pGraph->GetNodeManager().GetNode(lastNodeIndex)->GetRoadNavData()->fRoadWidth);
                m_pGraph->Connect(lastNodeIndex, node.nodeIndex, width, width, &linkOneTwo, &linkTwoOne);
                SetLinkParams(m_pGraph->GetLinkManager(), linkOneTwo, linkTwoOne);
            }

            // and link to the triangular/waypoint (so we can go over bridges)
            unsigned groundNode = m_pGraph->GetEnclosing(node.pos, roadNavTypes, 0.0f, lastTriNodeIndex);
            if (groundNode && (m_pGraph->GetNodeManager().GetNode(groundNode)->navType & roadNavTypes))
            {
                unsigned linkOneTwo = 0;
                unsigned linkTwoOne = 0;
                m_pGraph->Connect(node.nodeIndex, groundNode, node.width, node.width, &linkOneTwo, &linkTwoOne);
                SetLinkParams(m_pGraph->GetLinkManager(), linkOneTwo, linkTwoOne);
                lastTriNodeIndex = groundNode;
            }

            // also link to any other nearby roads
        }
        lastNodeIndex = node.nodeIndex;
    }

    tNodes& nodes1 = nodes;
    for (tRoads::iterator it2 = m_roads.begin(); it2 != m_roads.end(); ++it2)
    {
        tNodes& nodes2 = it2->second;
        if (&nodes1 == &nodes2)
        {
            continue;
        }

        for (tNodes::iterator itNode1 = nodes1.begin(); itNode1 != nodes1.end(); ++itNode1)
        {
            SRoadNode& node1 = *itNode1;
            for (tNodes::iterator itNode2 = nodes2.begin(); itNode2 != nodes2.end(); ++itNode2)
            {
                SRoadNode& node2 = *itNode2;
                float width = min(node1.width, node2.width);
                float deltaLenSq = Distance::Point_PointSq(node1.pos, node2.pos);
                if (deltaLenSq < square(width))
                {
                    unsigned linkOneTwo = 0;
                    unsigned linkTwoOne = 0;
                    m_pGraph->Connect(node1.nodeIndex, node2.nodeIndex, width, width, &linkOneTwo, &linkTwoOne);
                    SetLinkParams(m_pGraph->GetLinkManager(), linkOneTwo, linkTwoOne);
                }
            }
        }
    }
}

//====================================================================
// ReinsertIntoGraph
//====================================================================
void CRoadNavRegion::ReinsertIntoGraph()
{
    AIAssert(m_pGraph);

    // The graph can be empty if triangulation was bad
    if (!m_pGraph->m_pSafeFirst || !m_pGraph->m_pSafeFirst->firstLinkIndex)
    {
        return;
    }

    for (tRoads::iterator roadIt = m_roads.begin(); roadIt != m_roads.end(); )
    {
        roadIt = RemoveRoadNodes(roadIt->first);
    }

    // cut m_originalRoads against the triangulation to generate m_roads
    for (tRoads::iterator origRoadIt = m_originalRoads.begin(); origRoadIt != m_originalRoads.end(); ++origRoadIt)
    {
        CreateRoadNodes(origRoadIt->first);
    }
}

//====================================================================
// CreateRoad
//====================================================================
bool CRoadNavRegion::CreateRoad(const char* roadName, const std::vector<Vec3>& points, float roadWidth, float roadOffset)
{
    AIAssert(m_pGraph);
    if (!m_pGraph)
    {
        return false;
    }

    // Make sure all is safe
    tRoads::iterator it = m_roads.find(roadName);
    if (it != m_roads.end())
    {
        AIError("CRoadNavRegion::CreateRoad: Road '%s' already exists, please rename the path.", roadName);
        return false;
    }

    //  DeleteRoad(roadName);

    unsigned nPts = points.size();
    if (nPts < 2)
    {
        return true;
    }

    GraphNode* lastNode = 0;
    for (unsigned iPt = 0; iPt < nPts; ++iPt)
    {
        m_originalRoads[roadName].push_back(SRoadNode(points[iPt], roadWidth, roadOffset));
    }

    if (m_pGraph->m_pSafeFirst && m_pGraph->m_pSafeFirst->firstLinkIndex)
    {
        RemoveRoadNodes(roadName);
        CreateRoadNodes(roadName);
    }

    return true;
}

//====================================================================
// DeleteRoad
//====================================================================
bool CRoadNavRegion::DeleteRoad(const char* roadName)
{
    AIAssert(m_pGraph);
    if (!m_pGraph)
    {
        return false;
    }

    m_originalRoads.erase(roadName);

    tRoads::iterator it = m_roads.find(roadName);
    if (it == m_roads.end())
    {
        return false;
    }

    tNodes& nodes = it->second;
    for (tNodes::iterator nodeIt = nodes.begin(); nodeIt != nodes.end(); ++nodeIt)
    {
        unsigned nodeIndex = nodeIt->nodeIndex;
        if (nodeIndex)
        {
            m_pGraph->Disconnect(nodeIndex, true);
        }
    }
    m_roads.erase(it);

    return true;
}

//====================================================================
// DoesRoadExists
//====================================================================
bool CRoadNavRegion::DoesRoadExists(const char* roadName)
{
    return m_roads.find(roadName) != m_roads.end();
}

//====================================================================
// WriteRoad
//====================================================================
void CRoadNavRegion::WriteRoad(CCryFile& file, const string& name, const tNodes& nodes) const
{
    unsigned nameLen = name.length();
    file.Write(&nameLen, sizeof(nameLen));
    file.Write((char*) name.c_str(), nameLen * sizeof(char));

    unsigned nPts = nodes.size();
    file.Write(&nPts, sizeof(nPts));
    for (unsigned i = 0; i < nPts; ++i)
    {
        file.Write(&nodes[i].pos, sizeof(Vec3));
        file.Write(&nodes[i].width, sizeof(float));
        file.Write(&nodes[i].offset, sizeof(float));
    }
}

//====================================================================
// ReadRoad
//====================================================================
void CRoadNavRegion::ReadRoad(CCryFile& file, string& name, tNodes& nodes)
{
    unsigned nameLen;
    file.ReadType(&nameLen);
    char tmpName[1024];
    file.ReadRaw(&tmpName[0], nameLen);
    tmpName[nameLen] = '\0';
    name = tmpName;

    unsigned nPts;
    file.ReadType(&nPts);
    nodes.reserve(nPts);
    for (unsigned i = 0; i < nPts; ++i)
    {
        nodes.push_back(SRoadNode());
        file.ReadType(&nodes.back().pos);
        file.ReadType(&nodes.back().width);
        file.ReadType(&nodes.back().offset);
        nodes.back().nodeIndex = 0;
    }
}


//====================================================================
// WriteToFile
//====================================================================
bool CRoadNavRegion::WriteToFile(const char* roadsFileName)
{
    CCryFile file;
    if (false != file.Open(roadsFileName, "wb"))
    {
        int fileVersion = BAI_ROADS_FILE_VERSION;
        file.Write(&fileVersion, sizeof(fileVersion));

        unsigned numRoads = m_originalRoads.size();
        file.Write(&numRoads, sizeof(numRoads));
        tRoads::const_iterator it;
        for (it = m_originalRoads.begin(); it != m_originalRoads.end(); ++it)
        {
            WriteRoad(file, it->first, it->second);
        }
        return true;
    }
    else
    {
        AIWarning("Unable to open roads file %s", roadsFileName);
        return false;
    }
}

//====================================================================
// ReadFromFile
//====================================================================
bool CRoadNavRegion::ReadFromFile(const char* roadsFileName)
{
    if (!gEnv->IsEditor())
    {
        Clear();
    }

    CCryFile file;
    if (false != file.Open(roadsFileName, "rb"))
    {
        int iNumber;
        file.ReadType(&iNumber);
        if (iNumber != BAI_ROADS_FILE_VERSION)
        {
            AIError("CRoadNavRegion::ReadFromFile Wrong AI roads file version - found %d expected %d - regenerate triangulation [Design bug]", iNumber, BAI_ROADS_FILE_VERSION);
            return false;
        }

        if (gEnv->IsEditor())
        {
            // insertion into graph happens with OnMissionLoaded
            return true;
        }

        unsigned nRoads;
        file.ReadType(&nRoads);
        for (unsigned iRoad = 0; iRoad < nRoads; ++iRoad)
        {
            string name;
            tNodes nodes;
            ReadRoad(file, name, nodes);

            if (m_originalRoads.find(name) != m_originalRoads.end())
            {
                AIError("CRoadNavRegion::ReadFromFile: Road '%s' already exists, please rename the path and reexport.", name.c_str());
                continue;
            }
            m_originalRoads[name] = nodes;
        }
        ReinsertIntoGraph();
        return true;
    }
    else
    {
        AIWarning("Unable to open roads file %s", roadsFileName);
        Clear();
        return false;
    }
}

//===================================================================
// OnMissionLoaded
//===================================================================
void CRoadNavRegion::OnMissionLoaded()
{
    if (gEnv->IsEditor())
    {
        ReinsertIntoGraph();
    }
}
