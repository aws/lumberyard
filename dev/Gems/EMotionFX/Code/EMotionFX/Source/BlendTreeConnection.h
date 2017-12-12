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

#pragma once

// include the required headers
#include "EMotionFXConfig.h"
#include <AzCore/RTTI/RTTI.h>


namespace EMotionFX
{
    // forward declarations
    class AnimGraphNode;
    class AnimGraph;


    /**
     * A connection between two nodes.
     */
    class EMFX_API BlendTreeConnection
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendTreeConnection, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_CONNECTIONS);

    public:
        AZ_RTTI(BlendTreeConnection, "{B48FFEDB-87FB-4085-AE54-0302AC49373A}");

        BlendTreeConnection();
        BlendTreeConnection(AnimGraphNode* sourceNode, uint16 sourcePort, uint16 targetPort);
        virtual ~BlendTreeConnection();

        bool GetIsValid() const;
        BlendTreeConnection* Clone(AnimGraph* animGraph);

        MCORE_INLINE AnimGraphNode* GetSourceNode() const          { return mBlendNode; }
        MCORE_INLINE uint16 GetSourcePort() const                   { return mSourcePort; }
        MCORE_INLINE uint16 GetTargetPort() const                   { return mTargetPort; }

        MCORE_INLINE void SetSourceNode(AnimGraphNode* node)       { mBlendNode = node; }
        MCORE_INLINE void SetSourcePort(uint16 sourcePort)          { mSourcePort = sourcePort; }
        MCORE_INLINE void SetTargetPort(uint16 targetPort)          { mTargetPort = targetPort; }

        MCORE_INLINE uint32 GetID() const                           { return mID; }
        MCORE_INLINE void SetID(uint32 id)                          { mID = id; }

        #ifdef EMFX_EMSTUDIOBUILD
        MCORE_INLINE void SetIsVisited(bool visited)            { mVisited = visited; }
        MCORE_INLINE bool GetIsVisited() const                  { return mVisited; }
        #endif

    private:
        AnimGraphNode*         mBlendNode;         /**< The source node from which the incoming connection comes. */
        uint32                  mID;                /**< The unique ID value of the connection. */
        uint16                  mSourcePort;        /**< The source port number, so the output port number of the node where the connection comes from. */
        uint16                  mTargetPort;        /**< The target port number, which is the input port number of the target node. */

        #ifdef EMFX_EMSTUDIOBUILD
        bool                mVisited;               /**< True when during updates this connection was used. */
        #endif
    };
}   // namespace EMotionFX
