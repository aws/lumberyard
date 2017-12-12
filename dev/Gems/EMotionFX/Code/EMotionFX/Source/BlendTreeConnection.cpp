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

// include required headers
#include "EMotionFXConfig.h"
#include "BlendTreeConnection.h"
#include <MCore/Source/IDGenerator.h>
#include "AnimGraphNode.h"
#include "AnimGraph.h"


namespace EMotionFX
{
    // default constructor
    BlendTreeConnection::BlendTreeConnection()
    {
        mBlendNode      = nullptr;
        mSourcePort     = MCORE_INVALIDINDEX16;
        mTargetPort     = MCORE_INVALIDINDEX16;
        mID             = MCore::GetIDGenerator().GenerateID();

    #ifdef EMFX_EMSTUDIOBUILD
        mVisited        = false;
    #endif
    }


    // extended constructor
    BlendTreeConnection::BlendTreeConnection(AnimGraphNode* sourceNode, uint16 sourcePort, uint16 targetPort)
    {
        mBlendNode      = sourceNode;
        mSourcePort     = sourcePort;
        mTargetPort     = targetPort;
        mID             = MCore::GetIDGenerator().GenerateID();

    #ifdef EMFX_EMSTUDIOBUILD
        mVisited        = false;
    #endif
    }


    // destructor
    BlendTreeConnection::~BlendTreeConnection()
    {
    }


    // check if this connection is valid
    bool BlendTreeConnection::GetIsValid() const
    {
        // make sure the node and input numbers are valid
        if (mBlendNode == nullptr || mSourcePort == MCORE_INVALIDINDEX16 || mTargetPort == MCORE_INVALIDINDEX16)
        {
            return false;
        }

        // the connection is valid
        return true;
    }


    // clone the blend tree connection
    BlendTreeConnection* BlendTreeConnection::Clone(AnimGraph* animGraph)
    {
        BlendTreeConnection* result = new BlendTreeConnection();

        if (mBlendNode)
        {
            result->mBlendNode  = animGraph->RecursiveFindNodeByID(mBlendNode->GetID());
            MCORE_ASSERT(result->mBlendNode);
        }

        result->mSourcePort = mSourcePort;
        result->mTargetPort = mTargetPort;

        // mVisited doesn't need to be cloned
        return result;
    }
}   // namespace EMotionFX

