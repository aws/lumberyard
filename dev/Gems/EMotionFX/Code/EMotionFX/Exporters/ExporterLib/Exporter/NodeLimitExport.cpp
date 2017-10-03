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

#include "Exporter.h"
#include <EMotionFX/Source/NodeLimitAttribute.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Importer/ActorFileFormat.h>


namespace ExporterLib
{
    void SaveLimit(MCore::Stream* file, EMotionFX::Actor* actor, EMotionFX::NodeLimitAttribute* limit, const uint32 nodeIndex, MCore::Endian::EEndianType targetEndianType)
    {
        MCORE_ASSERT(file);
        MCORE_ASSERT(limit);

        // chunk information
        EMotionFX::FileFormat::FileChunk chunkHeader;
        chunkHeader.mChunkID        = EMotionFX::FileFormat::ACTOR_CHUNK_LIMIT;
        chunkHeader.mSizeInBytes    = sizeof(EMotionFX::FileFormat::Actor_Limit);
        chunkHeader.mVersion        = 1;

        EMotionFX::FileFormat::Actor_Limit limitChunk;
        limitChunk.mNodeNumber = nodeIndex;

        MCore::Vector3 translationMin = limit->GetTranslationMin();
        MCore::Vector3 translationMax = limit->GetTranslationMax();

        CopyVector(limitChunk.mTranslationMin, translationMin);
        CopyVector(limitChunk.mTranslationMax, translationMax);

        limitChunk.mTranslationMin.mX = MCore::Min<float>(translationMin.x, translationMax.x);
        limitChunk.mTranslationMin.mY = MCore::Min<float>(translationMin.y, translationMax.y);
        limitChunk.mTranslationMin.mZ = MCore::Min<float>(translationMin.z, translationMax.z);

        limitChunk.mTranslationMax.mX = MCore::Max<float>(translationMin.x, translationMax.x);
        limitChunk.mTranslationMax.mY = MCore::Max<float>(translationMin.y, translationMax.y);
        limitChunk.mTranslationMax.mZ = MCore::Max<float>(translationMin.z, translationMax.z);

        CopyVector(limitChunk.mRotationMin, limit->GetRotationMin());
        CopyVector(limitChunk.mRotationMax, limit->GetRotationMax());

        // negate rotational limits
        MCore::Vector3 rotationMin = limit->GetRotationMin();
        MCore::Vector3 rotationMax = limit->GetRotationMax();

        limit->SetRotationMin(rotationMin);
        limit->SetRotationMax(rotationMax);

        limitChunk.mRotationMin.mX = MCore::Min<float>(rotationMin.x, rotationMax.x);
        limitChunk.mRotationMin.mY = MCore::Min<float>(rotationMin.y, rotationMax.y);
        limitChunk.mRotationMin.mZ = MCore::Min<float>(rotationMin.z, rotationMax.z);

        limitChunk.mRotationMax.mX = MCore::Max<float>(rotationMin.x, rotationMax.x);
        limitChunk.mRotationMax.mY = MCore::Max<float>(rotationMin.y, rotationMax.y);
        limitChunk.mRotationMax.mZ = MCore::Max<float>(rotationMin.z, rotationMax.z);


        // scale
        MCore::Vector3 scaleMin = limit->GetScaleMin();
        MCore::Vector3 scaleMax = limit->GetScaleMax();

        CopyVector(limitChunk.mScaleMin, scaleMin);
        CopyVector(limitChunk.mScaleMax, scaleMax);

        limitChunk.mScaleMin.mX = MCore::Min<float>(scaleMin.x, scaleMax.x);
        limitChunk.mScaleMin.mY = MCore::Min<float>(scaleMin.y, scaleMax.y);
        limitChunk.mScaleMin.mZ = MCore::Min<float>(scaleMin.z, scaleMax.z);

        limitChunk.mScaleMax.mX = MCore::Max<float>(scaleMin.x, scaleMax.x);
        limitChunk.mScaleMax.mY = MCore::Max<float>(scaleMin.y, scaleMax.y);
        limitChunk.mScaleMax.mZ = MCore::Max<float>(scaleMin.z, scaleMax.z);

        for (uint32 i = 0; i < 9; ++i)
        {
            limitChunk.mLimitFlags[i] = (uint8)limit->GetIsLimited(static_cast<EMotionFX::NodeLimitAttribute::ELimitType>(1 << i));
        }


        // print limit information
        if (actor)
        {
            MCore::LogDetailedInfo(" - Node Limit: nodeName='%s' nodeNr=%i", actor->GetSkeleton()->GetNode(nodeIndex)->GetName(), limitChunk.mNodeNumber);
        }
        else
        {
            MCore::LogDetailedInfo(" - Node Limit: nodeNr=%i", limitChunk.mNodeNumber);
        }
        MCore::LogDetailedInfo("    + TranslateMin = %f, %f, %f", limitChunk.mTranslationMin.mX, limitChunk.mTranslationMin.mY, limitChunk.mTranslationMin.mZ);
        MCore::LogDetailedInfo("    + TranslateMax = %f, %f, %f", limitChunk.mTranslationMax.mX, limitChunk.mTranslationMax.mY, limitChunk.mTranslationMax.mZ);
        MCore::LogDetailedInfo("    + RotateMin    = %f, %f, %f (Degrees: %f, %f, %f)", limitChunk.mRotationMin.mX, limitChunk.mRotationMin.mY, limitChunk.mRotationMin.mZ, MCore::Math::RadiansToDegrees(limitChunk.mRotationMin.mX), MCore::Math::RadiansToDegrees(limitChunk.mRotationMin.mY), MCore::Math::RadiansToDegrees(limitChunk.mRotationMin.mZ));
        MCore::LogDetailedInfo("    + RotateMax    = %f, %f, %f (Degrees: %f, %f, %f)", limitChunk.mRotationMax.mX, limitChunk.mRotationMax.mY, limitChunk.mRotationMax.mZ, MCore::Math::RadiansToDegrees(limitChunk.mRotationMax.mX), MCore::Math::RadiansToDegrees(limitChunk.mRotationMax.mY), MCore::Math::RadiansToDegrees(limitChunk.mRotationMax.mZ));
        MCore::LogDetailedInfo("    + ScaleMin     = %f, %f, %f", limitChunk.mScaleMin.mX, limitChunk.mScaleMin.mY, limitChunk.mScaleMin.mZ);
        MCore::LogDetailedInfo("    + ScaleMax     = %f, %f, %f", limitChunk.mScaleMax.mX, limitChunk.mScaleMax.mY, limitChunk.mScaleMax.mZ);
        MCore::LogDetailedInfo("    + LimitFlags   = POS[%d, %d, %d], ROT[%d, %d, %d], SCALE[%d, %d, %d]", limitChunk.mLimitFlags[0], limitChunk.mLimitFlags[1], limitChunk.mLimitFlags[2], limitChunk.mLimitFlags[3], limitChunk.mLimitFlags[4], limitChunk.mLimitFlags[5], limitChunk.mLimitFlags[6], limitChunk.mLimitFlags[7], limitChunk.mLimitFlags[8]);

        // endian conversion
        ConvertFileChunk(&chunkHeader, targetEndianType);
        ConvertFileVector3(&limitChunk.mTranslationMin, targetEndianType);
        ConvertFileVector3(&limitChunk.mTranslationMax, targetEndianType);
        ConvertFileVector3(&limitChunk.mRotationMin, targetEndianType);
        ConvertFileVector3(&limitChunk.mRotationMax, targetEndianType);
        ConvertFileVector3(&limitChunk.mScaleMin, targetEndianType);
        ConvertFileVector3(&limitChunk.mScaleMax, targetEndianType);
        ConvertUnsignedInt(&limitChunk.mNodeNumber, targetEndianType);

        // write it
        file->Write(&chunkHeader, sizeof(EMotionFX::FileFormat::FileChunk));
        file->Write(&limitChunk, sizeof(EMotionFX::FileFormat::Actor_Limit));
    }


    void SaveLimits(MCore::Stream* file, EMotionFX::Actor* actor, MCore::Endian::EEndianType targetEndianType)
    {
        MCORE_ASSERT(file);
        MCORE_ASSERT(actor);

        MCore::LogDetailedInfo("============================================================");
        MCore::LogInfo("Node Limits");
        MCore::LogDetailedInfo("============================================================");

        // get the number of nodes
        const uint32 numNodes = actor->GetNumNodes();

        // iterate through all nodes
        for (uint32 i = 0; i < numNodes; i++)
        {
            // get the node from the actor
            EMotionFX::Node* node = actor->GetSkeleton()->GetNode(i);

            // get the limit attribute from the node and continue if the node has no limit attribute
            EMotionFX::NodeAttribute* attribute = node->GetAttributeByType(EMotionFX::NodeLimitAttribute::TYPE_ID);
            if (!attribute)
            {
                continue;
            }

            // type cast to a node limit attribute
            EMotionFX::NodeLimitAttribute* limitAttribute = static_cast<EMotionFX::NodeLimitAttribute*>(attribute);

            // save the limit
            SaveLimit(file, actor, limitAttribute, node->GetNodeIndex(), targetEndianType);
        }
    }
} // namespace ExporterLib
