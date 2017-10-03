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

#ifndef __EMFX_MOTIONCOMPRESSIONCOMMANDS_H
#define __EMFX_MOTIONCOMPRESSIONCOMMANDS_H

// include the required headers
#include "CommandSystemConfig.h"
#include <MCore/Source/Command.h>
#include <EMotionFX/Source/MotionManager.h>
//#include <EMotionFX/Source/MorphMotion.h>
#include <EMotionFX/Source/SkeletalMotion.h>
#include <EMotionFX/Source/MotionSystem.h>
#include <EMotionFX/Source/WaveletSkeletalMotion.h>
#include <EMotionFX/Source/Importer/Importer.h>


namespace CommandSystem
{
    // wavelet compression command
    MCORE_DEFINECOMMAND_START(CommandKeyframeCompressMotion, "Compress keyframe motion", true)
public:
    // class for storing compression statistics
    class Statistics
    {
    public:
        Statistics()
        {
            mNumOriginalKeyframes   = 0;
            mNumCompressedKeyframes = 0;
            mOriginalSize           = 0;
            mCompressedSize         = 0;
            mCompressionRatio       = 0;
        }

        uint32 mNumOriginalKeyframes;
        uint32 mNumCompressedKeyframes;
        uint32 mOriginalSize;
        uint32 mCompressedSize;
        uint32 mCompressionRatio;
    };

    /**
     * optimizes a skeletal motion.
     * @param motion pointer to the motion to optimize.
     * @return the optimized motion.
     */
    static EMotionFX::Motion* OptimizeSkeletalMotion(EMotionFX::SkeletalMotion* motion, bool optimizePosition, bool optimizeRotation, bool optimizeScale, float maxPositionError, float maxRotationError, float maxScaleError, Statistics* outStatistics, bool removeFromMotionManager);

    /**
     * optimizes a skeletal motion.
     * @param motion pointer to the motion to optimize.
     * @return the optimized motion.
     */
    //static EMotionFX::Motion* OptimizeMorphMotion(EMotionFX::MorphMotion* motion, bool optimize, float maxError, Statistics* outStatistics, bool removeFromMotionManager);
    MCORE_DEFINECOMMAND_END


    // remove motion command
        MCORE_DEFINECOMMAND_START(CommandWaveletCompressMotion, "Compress wavelet motion", true)
public:
    /**
     * optimizes a skeletal motion using wavelet compression.
     * @param motion pointer to the motion to optimize.
     * @return the optimized motion.
     */
    static EMotionFX::Motion* WaveletCompressSkeletalMotion(EMotionFX::SkeletalMotion* motion, uint32 waveletType, float positionQuality, float rotationQuality, float scaleQuality, uint32 samplesPerSecond, uint32 samplesPerChunk, bool removeFromMotionManager);

    MCORE_DEFINECOMMAND_END
} // namespace CommandSystem


#endif
