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

#ifndef CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_GRAPH_H
#define CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_GRAPH_H
#pragma once


#include "IAISystem.h"
#include "IAgent.h"
#include "GraphStructures.h"
#include "AILog.h"
#include "AllNodesContainer.h"
#include "AutoTypeStructs.h"
#include "AIHash.h"

#include <list>
#include <map>
#include <set>
#include <vector>
#include <CryArray.h>
#include <VectorMap.h>
#include "STLPoolAllocator.h"

struct IRenderer;
class CCryFile;
class CGraphLinkManager;

enum EPathfinderResult
{
    PATHFINDER_STILLFINDING,
    PATHFINDER_BEAUTIFYINGPATH,
    PATHFINDER_POPNEWREQUEST,
    PATHFINDER_PATHFOUND,
    PATHFINDER_NOPATH,
    PATHFINDER_ABORT,
    PATHFINDER_MAXVALUE
};
class CAISystem;
struct IStatObj;
class ICrySizer;
class CAIObject;
class CVolumeNavRegion;
class CFlightNavRegion;
class CGraphNodeManager;

class CSmartObject;
struct CCondition;

struct IVisArea;

inline int TypeIndexFromType(IAISystem::ENavigationType type)
{
    int typeIndex;
    for (typeIndex = IAISystem::NAV_TYPE_COUNT - 1; typeIndex >= 0 && ((1 << typeIndex) & type) == 0; --typeIndex)
    {
        ;
    }
    return typeIndex;
}

inline const char* StringFromTypeIndex(int typeIndex)
{
    static const char* navTypeStrings[IAISystem::NAV_TYPE_COUNT] = {
        "NAV_UNSET",
        "NAV_TRIANGULAR",
        "NAV_WAYPOINT_HUMAN",
        "NAV_WAYPOINT_3DSURFACE",
        "NAV_FLIGHT",
        "NAV_VOLUME",
        "NAV_ROAD",
        "NAV_SMARTOBJECT",
        "NAV_FREE_2D"
    };
    enum
    {
        STRING_COUNT = sizeof(navTypeStrings) / sizeof(navTypeStrings[0])
    };

    if (typeIndex   < 0)
    {
        return "<Invalid Nav Type>";
    }
    else if (typeIndex >= STRING_COUNT)
    {
        return "<Nav Type Added - Update StringFromType()>";
    }
    else
    {
        return navTypeStrings[typeIndex];
    }
}

inline const char* StringFromType(IAISystem::ENavigationType type)
{
    return StringFromTypeIndex(TypeIndexFromType(type));
}

//====================================================================
// CObstacleRef
//====================================================================
class CObstacleRef
{
protected:
    IAIObject*    m_pAnchor;    // pointer to designer defined hiding point
    int           m_vertexIndex;// index of vertex
    unsigned  m_nodeIndex;      // for indoors nodes could be hide points
    GraphNode* m_pNode;
    CSmartObject* m_pSmartObject; // pointer to smart object to be used for hiding
    CCondition*   m_pRule;      // pointer to smart object rule to be used for hiding

public:
    IAIObject* GetAnchor() const { return m_pAnchor; }
    int GetVertex() const { return m_vertexIndex; }
    const unsigned GetNodeIndex() const { return m_nodeIndex; }
    const GraphNode* GetNode() const {return m_pNode; }
    CSmartObject* GetSmartObject() const { return m_pSmartObject; }
    const CCondition* GetRule() const { return m_pRule; }

    CObstacleRef()
        : m_pAnchor(NULL)
        , m_vertexIndex(-1)
        , m_nodeIndex(0)
        , m_pNode(0)
        , m_pSmartObject(NULL)
        , m_pRule(NULL) {}
    CObstacleRef(const CObstacleRef& other)
        : m_pAnchor(other.m_pAnchor)
        , m_vertexIndex(other.m_vertexIndex)
        , m_nodeIndex(other.m_nodeIndex)
        , m_pNode(other.m_pNode)
        , m_pSmartObject(other.m_pSmartObject)
        , m_pRule(other.m_pRule) {}
    CObstacleRef(IAIObject* pAnchor)
        : m_pAnchor(pAnchor)
        , m_vertexIndex(-1)
        , m_nodeIndex(0)
        , m_pNode(0)
        , m_pSmartObject(NULL)
        , m_pRule(NULL) {}
    CObstacleRef(int vertexIndex)
        : m_pAnchor(NULL)
        , m_vertexIndex(vertexIndex)
        , m_nodeIndex(0)
        , m_pSmartObject(NULL)
        , m_pRule(NULL) {}
    CObstacleRef(unsigned nodeIndex, GraphNode* pNode)
        : m_pAnchor(NULL)
        , m_vertexIndex(-1)
        , m_nodeIndex(nodeIndex)
        , m_pNode(pNode)
        , m_pSmartObject(NULL)
        , m_pRule(NULL) {}
    CObstacleRef(CSmartObject* pSmartObject, CCondition* pRule)
        : m_pAnchor(NULL)
        , m_vertexIndex(-1)
        , m_nodeIndex(0)
        , m_pNode(0)
        , m_pSmartObject(pSmartObject)
        , m_pRule(pRule) {}

    Vec3 GetPos() const;
    float GetApproxRadius() const;
    const CObstacleRef& operator = (const CObstacleRef& other)
    {
        m_pAnchor = other.m_pAnchor;
        m_vertexIndex = other.m_vertexIndex;
        m_nodeIndex = other.m_nodeIndex;
        m_pNode = other.m_pNode;
        m_pSmartObject = other.m_pSmartObject;
        m_pRule = other.m_pRule;
        return *this;
    }

    bool operator == (const CObstacleRef& other) const
    {
        return m_pAnchor == other.m_pAnchor && m_vertexIndex == other.m_vertexIndex && m_nodeIndex == other.m_nodeIndex
               && m_pNode == other.m_pNode && m_pSmartObject == other.m_pSmartObject && m_pRule == other.m_pRule;
    }
    bool operator != (const CObstacleRef& other) const
    {
        return m_pAnchor != other.m_pAnchor || m_vertexIndex != other.m_vertexIndex || m_nodeIndex != other.m_nodeIndex
               || m_pNode != other.m_pNode || m_pSmartObject != other.m_pSmartObject || m_pRule != other.m_pRule;
    }
    bool operator < (const CObstacleRef& other) const
    {
        return
            m_nodeIndex < other.m_nodeIndex || m_nodeIndex == other.m_nodeIndex &&
            (m_pAnchor < other.m_pAnchor || m_pAnchor == other.m_pAnchor &&
             (m_vertexIndex < other.m_vertexIndex || m_vertexIndex == other.m_vertexIndex &&
              (m_pSmartObject < other.m_pSmartObject || m_pSmartObject < other.m_pSmartObject &&
               m_pRule < other.m_pRule)));
    }
    operator bool () const
    {
        return m_pAnchor || m_vertexIndex >= 0 || m_nodeIndex || m_pSmartObject && m_pRule;
    }
    bool operator ! () const
    {
        return !m_pAnchor && m_vertexIndex < 0 && !m_nodeIndex && (!m_pSmartObject || !m_pRule);
    }

private:
    operator int () const
    {
        // it is illegal to cast CObstacleRef to an int!!!
        // are you still using old code?
        AIAssert(0);
        return 0;
    }
};

// NOTE: int64 here avoids a tiny performance impact on 32-bit platform
// for the cost of loss of full compatibility: 64-bit generated BAI files
// can't be used on 32-bit platform safely. Change the key to int64 to
// make it fully compatible. The code that uses this map will be recompiled
// to use the full 64-bit key on both 32-bit and 64-bit platforms.
typedef std::multimap<int64, unsigned> EntranceMap;
typedef std::list<Vec3> ListPositions;
typedef std::list<ObstacleData> ListObstacles;
typedef std::multimap<float, ObstacleData> MultimapRangeObstacles;
typedef std::vector<NodeDescriptor> NodeDescBuffer;
typedef std::vector<LinkDescriptor> LinkDescBuffer;
typedef std::list<GraphNode*> ListNodes;
typedef std::list<unsigned> ListNodeIds;
typedef std::set<GraphNode*> SetNodes;
typedef std::set<const GraphNode*> SetConstNodes;
typedef std::list<const GraphNode*> ListConstNodes;
typedef std::vector<const GraphNode*> VectorConstNodes;
typedef std::vector<unsigned> VectorConstNodeIndices;
typedef std::multimap<float, GraphNode*> CandidateMap;
typedef std::multimap<float, unsigned> CandidateIdMap;
typedef std::set< CObstacleRef > SetObstacleRefs;
typedef VectorMap<unsigned, SCachedPassabilityResult> PassabilityCache;

// [Mikko] Note: The Vector map is faster when traversing, and the normal map with pool allocator seems
// to be a little faster in CGraph.GetNodesInRange. Both are faster than normal std::map.
//typedef stl::STLPoolAllocator< std::pair<const GraphNode*, float> > NodeMapAllocator;
//typedef std::map<const GraphNode*, float, std::less<const GraphNode*>, NodeMapAllocator> MapConstNodesDistance;
typedef VectorMap<const GraphNode*, float> MapConstNodesDistance;

class CNavigation;

//====================================================================
// CGraph
//====================================================================
class CGraph
{
public:
    CGraph(CNavigation* pNavigation);
    ~CGraph();

    CGraphLinkManager& GetLinkManager() {return *m_pGraphLinkManager; }
    const CGraphLinkManager& GetLinkManager() const {return *m_pGraphLinkManager; }

    CGraphNodeManager& GetNodeManager() {return *m_pGraphNodeManager; }
    const CGraphNodeManager& GetNodeManager() const {return *m_pGraphNodeManager; }

    /// Restores the graph to the initial state (i.e. restores pass radii etc).
    void Reset();

    /// removes all nodes and stuff associated with navTypes matching the bitmask
    void Clear(unsigned navTypeMask);

    /// Connects (two-way) two nodes, optionally returning pointers to the new links
    void Connect(unsigned oneIndex, unsigned twoIndex, float radiusOneToTwo = 100.0f, float radiusTwoToOne = 100.0f,
        unsigned* pLinkOneTwo = 0, unsigned* pLinkTwoOne = 0);

    /// Disconnects a node from its neighbours. if bDelete then pNode will be deleted. Note that
    /// the previously connected nodes will not be deleted, even if they
    /// end up with no nodes.
    void Disconnect(unsigned nodeIndex, bool bDelete = true);

    /// Removes an individual link from a node (and removes the reciprocal link) -
    /// doesn't delete it.
    void Disconnect(unsigned nodeIndex, unsigned linkId);

    /// Checks the graph is OK (as far as possible). Asserts if not, and then
    /// returns true/false to indicate if it's OK
    /// msg should indicate where this is being called from (for writing error msgs)
    bool Validate(const char* msg, bool checkPassable) const;

    /// Checks that a node exists (should be quick). If fullCheck is true it will do some further
    /// checks which will be slower
    bool ValidateNode(unsigned nodeIndex, bool fullCheck) const;
    bool ValidateHashSpace() { return m_allNodes.ValidateHashSpace(); }

    // fns used by the editor to get info about nodes
    int GetBuildingIDFromWaypointNode(const GraphNode* pNode);
    void SetBuildingIDInWaypointNode(GraphNode* pNode, unsigned nBuildingID);
    void SetVisAreaInWaypointNode(GraphNode* pNode, IVisArea* pArea);
    EWaypointNodeType GetWaypointNodeTypeFromWaypointNode(const GraphNode* pNode);
    void SetWaypointNodeTypeInWaypointNode(GraphNode* pNode, EWaypointNodeType type);
    EWaypointLinkType GetOrigWaypointLinkTypeFromLink(unsigned link);
    float GetRadiusFromLink(unsigned link);
    float GetOrigRadiusFromLink(unsigned link);
    IAISystem::ENavigationType GetNavType(const GraphNode* pNode);
    unsigned GetNumNodeLinks(const GraphNode* pNode);
    unsigned GetGraphLink(const GraphNode* pNode, unsigned iLink);
    Vec3 GetWaypointNodeUpDir(const GraphNode* pNode);
    void SetWaypointNodeUpDir(GraphNode* pNode, const Vec3& up);
    Vec3 GetWaypointNodeDir(const GraphNode* pNode);
    void SetWaypointNodeDir(GraphNode* pNode, const Vec3& dir);
    Vec3 GetNodePos(const GraphNode* pNode);
    const GraphNode* GetNextNode(unsigned ink) const;
    GraphNode* GetNextNode(unsigned ink);
    unsigned int LinkId (unsigned link) const;

    /// General function to get the graph node enclosing a position - the type depends
    /// on the navigation modifiers etc and the navigation capabilities. If you know what
    /// navigation type you want go via the specific NavRegion class. If there are a number of nodes
    /// that could be returned, only nodes that have at least one link with radius > passRadius will
    /// be returned. Range determines the search range for the enclosing node: (range < 0) indicates
    /// a default search radius will be used determined by the navigation type. (range >= 0) will be
    /// taken into account depending on the navigation type.
    unsigned GetEnclosing(const Vec3& pos, IAISystem::tNavCapMask navCapMask, float passRadius = 0.0f,
        unsigned startIndex = 0, float range = -1.0f, Vec3* closestValid = 0, bool returnSuspect = false, const char* requesterName = "");

    /// Restores all node/links
    void RestoreAllNavigation();

    /// adds an entrance for easy traversing later
    void MakeNodeRemovable(int nBuildingID, unsigned nodeIndex, bool bRemovable = true);
    /// adds an entrance for easy traversing later
    void AddIndoorEntrance(int nBuildingID, unsigned nodeIndex, bool bExitOnly = false);
    /// returns false if the entrace can't be found
    bool RemoveEntrance(int nBuildingID, unsigned nodeIndex);

    /// Reads the AI graph from a specified file
    bool ReadFromFile(const char* szName);
    /// Writes the AI graph to file
    void WriteToFile(const char* pname);

    /// Returns all nodes that are in the graph - not all nodes will be
    /// connected (even indirectly) to each other
    CAllNodesContainer& GetAllNodes() {return m_allNodes; }
    const CAllNodesContainer& GetAllNodes() const {return m_allNodes; }

    /// Checks that the graph is empty. Pass in a bitmask of IAISystem::ENavigationType to
    /// specify the types to check
    bool CheckForEmpty(IAISystem::tNavCapMask navTypeMask = IAISystem::NAVMASK_ALL) const;

    //====================================================================
    //  functions for modifying the graph at run-time
    //====================================================================
    struct CRegion
    {
        CRegion(const std::list<Vec3>& outline, const AABB& aabb)
            : m_outline(outline)
            , m_AABB(aabb) {}
        std::list<Vec3> m_outline;
        AABB m_AABB;
    };
    struct CRegions
    {
        std::list<CRegion> m_regions;
        AABB m_AABB;
    };

    // Check whether a position is within a node's triangle
    bool PointInTriangle(const Vec3& pos, GraphNode* pNode);

    /// uses mark for internal graph operation without disturbing the pathfinder
    void MarkNode(unsigned nodeIndex) const;
    /// clears the marked nodes
    void ClearMarks() const;

    /// Tagging of nodes - must be used ONLY by the pathfinder (apart from when triangulation is generated)
    void TagNode(unsigned nodeIndex) const;
    /// Clears the tagged nodes
    void ClearTags() const;
    /// for debug drawing - nodes get tagged during A*
    const VectorConstNodeIndices& GetTaggedNodes() const {return m_taggedNodes; }

    // defines bounding rectangle of this graph
    void SetBBox(const Vec3& min, const Vec3& max);
    // how is that for descriptive naming of functions ??
    bool InsideOfBBox(const Vec3& pos) const; // returns true if pos is inside of bbox (but not on boundaries)

    /// Creates a new node of the specified type (which can't be subsequently changed). Note that
    /// to delete the node use the disconnect function.
    unsigned CreateNewNode(IAISystem::ENavigationType type, const Vec3& pos, unsigned ID = 0);
    GraphNode* GetNode(unsigned index);

    /// Moves a node, updating spatial structures
    void MoveNode(unsigned nodeIndex, const Vec3& newPos);

    SCachedPassabilityResult* GetCachedPassabilityResult(unsigned linkId, bool bCreate);

    //====================================================================
    //  The following functions are used during graph generation/triangulation
    //====================================================================
    void ResolveLinkData(GraphNode* pOne, GraphNode* pTwo);
    void ConnectNodes(ListNodeIds& lstNodes);
    void FillGraphNodeData(unsigned nodeIndex);
    typedef std::list<int>            ObstacleIndexList;
    typedef std::multimap< float, GraphNode* >    NodesList;
    int           Rearrange(const string& areaName, ListNodeIds& nodesList, const Vec3r& cutStart, const Vec3r& cutEnd);
    int           ProcessMegaMerge(const string& areaName, ListNodeIds& nodesList, const Vec3r& cutStart, const Vec3r& cutEnd);
    bool      CreateOutline(const string& areaName, ListNodeIds& insideNodes, ListNodeIds& nodesList, ObstacleIndexList& outline);
    void      TriangulateOutline(const string& areaName, ListNodeIds& nodesList, ObstacleIndexList&  outline, bool orientation);
    bool      ProcessMerge(const string& areaName, GraphNode* curNode, CGraph::NodesList& ndList);

    // Returns memory usage not including nodes
    size_t MemStats();
    // Returns the memory usage for nodes of the type passed in (bitmask)
    size_t NodeMemStats(unsigned navTypeMask);

    struct SBadGraphData
    {
        enum EType
        {
            BAD_PASSABLE, BAD_IMPASSABLE
        };
        EType mType;
        Vec3 mPos1, mPos2;
    };
    /// List of bad stuff we found during the last validation. mutable because it's
    /// debug - Validate(...) should really be a const method, since it wouldn't change
    /// any "real" data
    mutable std::vector<SBadGraphData> mBadGraphData;

    GraphNode* m_pSafeFirst;
    unsigned m_safeFirstIndex;

    CGraphNodeManager* m_pGraphNodeManager;

private:

    bool DbgCheckList(ListNodeIds& nodesList)   const;

public:
    /// deletes (disconnects too) all nodes with a type matching the bitmask
    void DeleteGraph(IAISystem::tNavCapMask navTypeMask);

private:
    GraphNode* GetEntrance(int nBuildingID, const Vec3& pos);
    bool GetEntrances(int nBuildingID, const Vec3& pos, std::vector<unsigned>& nodes);
    void WriteLine(unsigned nodeIndex);
    // reads all the nodes in a map
    bool ReadNodes(CCryFile& file);
    /// Deletes the node, which should have been disconnected first (warning if not)
    void DeleteNode(unsigned nodeIndex);

    // helper called from ValidateNode only (to get ValidateNode to be inlined inside
    // Graph.cpp)
    bool ValidateNodeFullCheck(const GraphNode* pNode) const;

    unsigned m_currentIndex;
    GraphNode* m_pCurrent;
    unsigned m_firstIndex;
    GraphNode* m_pFirst;

    ListNodeIds   m_lstNodesInsideSphere;

    /// All the nodes we've marked
    mutable VectorConstNodeIndices m_markedNodes;
    /// All the nodes we've tagged
    mutable VectorConstNodeIndices m_taggedNodes;

    /// nodes are allocated/deleted via a single interface, so keep track of them
    /// all - for memory tracking and to allow quick iteration
    CAllNodesContainer m_allNodes;

    PassabilityCache m_passabilityCache;

    /// Used to save/load nodes - should be empty otherwise
    NodeDescBuffer m_vNodeDescs;
    /// Used to save/load links - should be empty otherwise
    LinkDescBuffer m_vLinkDescs;

    CGraphLinkManager* m_pGraphLinkManager;

    /// Bounding box of the triangular area
    AABB m_triangularBBox;

    EntranceMap m_mapEntrances;
    EntranceMap m_mapExits;
    EntranceMap m_mapRemovables;

    CNavigation* m_pNavigation;

    static SetObstacleRefs static_AllocatedObstacles;

    friend class CTriangularNavRegion;
    friend class CWaypointHumanNavRegion;
    friend class CFlightNavRegion;
    friend class CVolumeNavRegion;
};

//====================================================================
// SMarkClearer
// Helper - the constructor and destructor clear marks
//====================================================================
struct SMarkClearer
{
    SMarkClearer(const CGraph* pGraph)
        : m_pGraph(pGraph) {m_pGraph->ClearMarks(); }
    ~SMarkClearer() {m_pGraph->ClearMarks(); }
private:
    const CGraph* m_pGraph;
};

//====================================================================
// Inline implementations
//====================================================================

inline unsigned int CGraph::LinkId (unsigned link) const
{
    unsigned int prev = GetLinkManager().GetPrevNode (link);
    unsigned int next = GetLinkManager().GetNextNode (link);
    const GraphNode* prevNode = GetNodeManager().GetNode(prev);
    const GraphNode* nextNode = GetNodeManager().GetNode(next);
    return HashFromVec3 (prevNode->GetPos (), 0.0f) + HashFromVec3 (nextNode->GetPos (), 0.0f);
}

#endif // CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_GRAPH_H
