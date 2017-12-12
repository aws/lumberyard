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

#ifndef CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_FLIGHTNAVREGION_H
#define CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_FLIGHTNAVREGION_H
#pragma once


#include "IAISystem.h"
#include "NavRegion.h"
#include "AutoTypeStructs.h"
#include <vector>
#include <list>
#include <map>

// Enable this to visualise the path radius calculations.
// This will add some extra data to the structures, so it is required to recreate the navigation.
// The extra information will not be saved to the file.
//#define       FLIGHTNAV_DEBUG_SPHERES 1

struct IRenderer;
struct I3DEngine;
struct IPhysicalWorld;
struct IGeometry;
struct SpecialArea;
class CGraph;

class CFlightNavRegion
    : public CNavRegion
{
public:

public:
    CFlightNavRegion(IPhysicalWorld* pPhysWorld, CGraph* pGraph);
    ~CFlightNavRegion(void);
    virtual void Clear();

    // Returns graph node based on the specified location.
    virtual unsigned GetEnclosing(const Vec3& pos, float passRadius = 0.0f, unsigned startIndex = 0,
        float range = -1.0f, Vec3* closestValid = 0, bool returnSuspect = false, const char* requesterName = "");

    // Builds the flight navigation from: terrain and static obstacles, limited by the specified special areas.
    void    Process(I3DEngine* p3DEngine, IPhysicalEntity** pObstacles, int obstacleCount,
        const std::list<SpecialArea*>& flightAreas,
        unsigned childSubDiv = 2, unsigned terrainDownSample = 8);

    // Writes the flight navigation to binary file, returns true on success.
    bool    WriteToFile(const char* pName);

    // Reads the flight navigation from binary file, returns true on success.
    bool    ReadFromFile(const char* pName);

    // Checks a sphere for collision, and adjusts the center to fit inside flight navigation cosntrains.
    bool    CheckAndAdjustSphere(Vec3& center, float rad);

    // check if the space is void inside of the box which is made by these 8 point
    Vec3    IsSpaceVoid(const Vec3& vPos, const Vec3& vForward, const Vec3& vWing, const Vec3& vUp) const;

    // Returns the height directly below and above the certain point in the world. If the point intersects
    void  GetHeightAndSpaceAt(const Vec3& pos, float& height, float& heightAbove) const;

    /// inherited
    virtual void Serialize(TSerialize ser);

    /// inherited
    virtual size_t MemStats();

private:

    const static int    INVALID_IDX = -1;


    // The navigation information is stored in cells, each containing arbitrary number of height spans.
    struct SSpan
    {
        SSpan()
            : m_x(0)
            , m_y(0)
            , m_minz(0)
            , m_maxz(0)
            , m_graphNodeIndex(0)
            , m_maxRadius(0)
            , m_classification(0)
            , m_childIdx(INVALID_IDX)
            , m_nextIdx(INVALID_IDX) {};

        SSpan(float x, float y, float min_, float max_, float rad_, int classification_ = 0)
            : m_x(x)
            , m_y(y)
            , m_minz(min_)
            , m_maxz(max_)
            , m_graphNodeIndex(0)
            , m_maxRadius(rad_)
            , m_classification(classification_)
            , m_childIdx(INVALID_IDX)
            , m_nextIdx(INVALID_IDX) {};

        void    Set(float x, float y, float min_, float max_, float rad_, int classification_ = 0)
        {
            m_x = x;
            m_y = y;
            m_minz = min_;
            m_maxz = max_;
            m_maxRadius = rad_;
            m_classification = classification_;
        };

        void    CopyFrom(const SSpan& other)
        {
            m_minz = other.m_minz;
            m_maxz = other.m_maxz;
            m_x = other.m_x;
            m_y = other.m_y;
            m_graphNodeIndex = other.m_graphNodeIndex;
            m_maxRadius = other.m_maxRadius;
            //          m_spherePos = other.m_spherePos;
            m_classification = other.m_classification;
            m_childIdx = other.m_childIdx;
            m_nextIdx = other.m_nextIdx;
        };

        bool     operator<(const SSpan& other) const
        {
            if (m_minz < other.m_minz)
            {
                return true;
            }
            return false;
        };

        bool     operator>(const SSpan& other) const
        {
            if (m_maxz > other.m_maxz)
            {
                return true;
            }
            return false;
        };

        // Test if two spans overlap, return true if overlapping.
        bool    overlaps(float min_, float max_) const
        {
            if (!((min_ < m_minz && max_ < m_minz) || (min_ > m_maxz && max_ > m_maxz)))
            {
                return true;
            }
            return false;
        }

        // Each span
        enum EClassificationFlags
        {
            HIRES                   = 0x0001,               // The span should be kept in high resolution.
            LANDING_GOOD    = 0x0002,               // Solid ground or surface, good for landing.
            LANDING_OK      = 0x0004,               // Solid ground or surface, suitable for landing.
            LANDING_BAD     = 0x0008,               // Unstable ground, not suitable for landing.
            LINKABLE            = 0x0010,
        };

        float   m_minz, m_maxz;             // Minimum and maximum height of the span.
        float   m_x, m_y;                           // Location of the span.
        unsigned m_graphNodeIndex;      // Pointer to the graph node which lies on top of the span.
        float   m_maxRadius;                    // Max radius of a sphere the span can contain.
#ifdef FLIGHTNAV_DEBUG_SPHERES
        Vec3    m_spherePos;                    // DEBUG: This variable is used to visualise the calculated radius with a sphere, can be omitted.
#endif
        int     m_classification;           // Flag indicating that the span should be stored in hires (used in precalc).
        int     m_childIdx;
        int     m_nextIdx;
    };

    // Cell structure used for building the span height field.

    struct SCell
    {
        SCell()
            : m_used(0)
            , m_hires(false) {};

        int                             m_used;
        bool                            m_hires;
        std::list<SSpan>    m_spans;
        std::list<int>      m_obstacles;
    };

    // Returns the minimum distance (squared) to any span in the cells contained by the specified cube.
    float   GetMinDistanceSqrCellsInCube(const Vec3& center, float rad) const;

    // Return the minimum distance (squared) to any span in the cell. The coordinates are local heightfield space.
    float   GetMinDistanceSqrToCell(int x, int y, const Vec3& center) const;

    // Returns the minimum distance (squared) to the AABB.
    float   GetDistanceSqrToBox(const Vec3& boxMin, const Vec3& boxMax, const Vec3& center) const;

    // Adjusts the sphere so that it does not intersect the AABB, returns true in case of collision.
    bool    BoxSphereAdjust(const Vec3& boxMin, const Vec3& boxMax, Vec3& center, float rad);

    // Builds the navigation graph nodes and links from the cells.
    void    BuildGraph();

    // Creates all possible connections between the two cells, assumes that the graphnodes at spans are already created.
    void        ConnectNodes(int srcSpanIdx, int dstSpanIdx);

    // Returns graph node based on the specified location.
    unsigned GetEnclosing(int x, int y, const Vec3& pos, Vec3* closestValid = 0, Vec3* spanPos = 0);

    void    DumpLinkRadii(const char* filename);

    IPhysicalWorld*             m_pPhysWorld;
    CGraph*                             m_pGraph;
    int                                 m_heightFieldOriginX, m_heightFieldOriginY;
    int                       m_heightFieldDimX, m_heightFieldDimY; // The size of the coarse grid of cells.
    int                                     m_childSubDiv;                                              // If the cell is defined as hires (child idx > 0), the cell is divided based on this variable.
    int                                     m_terrainDownSample;                                    // The downsample ratio from the terrain.
    std::vector<SSpan>      m_spans;                                                            // Array of spans, the first m_heightFieldDimX * m_heightFieldDimY cells are the base cells, the rest are used for childs.
};

#endif // CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_FLIGHTNAVREGION_H
