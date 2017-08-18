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

#ifndef CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_GRAPHNODEMANAGER_H
#define CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_GRAPHNODEMANAGER_H
#pragma once


//#include "GraphStructures.h"

class CGraphNodeManager
{
    enum
    {
        BUCKET_SIZE_SHIFT = 7
    };
    enum
    {
        BUCKET_SIZE = 128
    };

public:
    CGraphNodeManager();
    ~CGraphNodeManager();

    void Clear(unsigned typeMask);

    unsigned CreateNode(IAISystem::ENavigationType type, const Vec3& pos, unsigned ID);
    void DestroyNode(unsigned index);

    GraphNode* GetNode(unsigned index)
    {
        if (!index)
        {
            return 0;
        }
        int bucket = (index - 1) >> BUCKET_SIZE_SHIFT;
        if (!m_buckets[bucket])
        {
            return GetDummyNode();
        }
        return reinterpret_cast<GraphNode*>(static_cast<char*>(m_buckets[bucket]->nodes) +
                                            (((index - 1) & (BUCKET_SIZE - 1)) * m_buckets[bucket]->nodeSize));
    }

    const GraphNode* GetNode(unsigned index) const
    {
        if (!index)
        {
            return 0;
        }
        int bucket = (index - 1) >> BUCKET_SIZE_SHIFT;
        if (!m_buckets[bucket])
        {
            return GetDummyNode();
        }
        return reinterpret_cast<GraphNode*>(static_cast<char*>(m_buckets[bucket]->nodes) +
                                            (((index - 1) & (BUCKET_SIZE - 1)) * m_buckets[bucket]->nodeSize));
    }

private:
    class BucketHeader
    {
    public:
        unsigned type;
        unsigned nodeSize;
        void* nodes;
    };

    // This function exists only to hide some fundamental problems in the path-finding system. Basically
    // things aren't cleaned up properly as of writing (pre-alpha Crysis 5/5/2007). Therefore we return
    // an object that looks like a graph node to stop client code from barfing.
    GraphNode* GetDummyNode() const;

    std::vector<BucketHeader*> m_buckets;
    int m_lastBucket[IAISystem::NAV_TYPE_COUNT];
    int m_nextNode[IAISystem::NAV_TYPE_COUNT];
    int m_typeSizes[IAISystem::NAV_TYPE_COUNT];
};

#endif // CRYINCLUDE_EDITOR_AI_NAVDATAGENERATION_GRAPHNODEMANAGER_H
