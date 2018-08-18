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

#ifndef CRYINCLUDE_CRYANIMATION_COMMAND_COMMANDS_H
#define CRYINCLUDE_CRYANIMATION_COMMAND_COMMANDS_H
#pragma once

namespace Command {
    class CState;

    enum
    {
        eClearPoseBuffer = 0,
        eAddPoseBuffer,     //reads content from m_SourceBuffer, multiplies the pose by a blend weight, and adds the result to the m_TargetBuffer

        eSampleAddAnimFull,
        eScaleUniformFull,
        eNormalizeFull,

        eSampleReplaceAnimPart, //sample a partial-body animation and store the Poses to a destination buffer. This is directly replacing the joints in the destination buffers

        eSampleAddAnimPart, //Layer-Sampling for Overrride and Additve. This command adds a sample partial-body animation and its per-joint Blend-Weights to a destination buffer.
        ePerJointBlending,  //Layer-Blending for Overrride and Additve. This command is using Blend-Weigths per joint, which can be different for positions and orientations

        eJointMask,
        ePoseModifier,

        //just for debugging
        eSampleAddPoseFull, //used to playback the frames in a CAF-file which stored is in GlobalAnimationHeaderAIM
        eSamplePosePart,    //used to playback the frames in a CAF-file which stored is in GlobalAnimationHeaderAIM
        eVerifyFull,
    };




    //this command deletes all previous entries in a pose-buffer (no matter if Temp or Target-Buffer)
    struct ClearPoseBuffer
    {
        enum
        {
            ID = eClearPoseBuffer
        };

        uint8 m_nCommand;
        uint8 m_TargetBuffer;
        uint8 m_nJointStatus;
        uint8 m_nPoseInit;

        void Execute(const CState& state, void* buffers[]) const;
    };

    struct AddPoseBuffer
    {
        enum
        {
            ID = eAddPoseBuffer
        };

        uint8 m_nCommand;
        uint8 m_SourceBuffer;
        uint8 m_TargetBuffer;
        uint8 m_IsEmpty; //temporary
        f32 m_fWeight;

        void Execute(const CState& state, void* buffers[]) const;
    };


    struct SampleAddAnimFull
    {
        enum
        {
            ID = eSampleAddAnimFull
        };
        enum
        {
            Flag_ADMotion     = 1,
            Flag_TmpBuffer    = 8,
        };

        uint8 m_nCommand;
        uint8 m_flags;
        int16 m_nEAnimID;
        f32 m_fETimeNew; //this is a percentage value between 0-1
        f32 m_fWeight;

        void Execute(const CState& state, void* buffers[]) const;
    };



    struct ScaleUniformFull
    {
        enum
        {
            ID = eScaleUniformFull
        };

        uint8 m_nCommand;
        uint8 m_TargetBuffer;
        uint8 _PADDING0;
        uint8 _PADDING1;
        f32 m_fScale;

        void Execute(const CState& state, void* buffers[]) const;
    };

    struct NormalizeFull
    {
        enum
        {
            ID = eNormalizeFull
        };

        uint8 m_nCommand;
        uint8 m_TargetBuffer;
        uint8 _PADDING0;
        uint8 _PADDING1;

        void Execute(const CState& state, void* buffers[]) const;
    };


    struct SampleAddAnimPart
    {
        enum
        {
            ID = eSampleAddAnimPart
        };

        uint8 m_nCommand;
        uint8 m_TargetBuffer;
        uint8 m_SourceBuffer;
        uint8 _PADDING1;

        int32 m_nEAnimID;

        f32 m_fAnimTime; //this is a percentage value between 0-1
        f32 m_fWeight;

#if defined(USE_PROTOTYPE_ABS_BLENDING)
        strided_pointer<const int>   m_maskJointIDs;
        strided_pointer<const float> m_maskJointWeights;
        int                                      m_maskNumJoints;
#endif //!defined(USE_PROTOTYPE_ABS_BLENDING)

        void Execute(const CState& state, void* buffers[]) const;
    };


    struct PerJointBlending
    {
        enum
        {
            ID = ePerJointBlending
        };

        uint8 m_nCommand;
        uint8 m_SourceBuffer;
        uint8 m_TargetBuffer;
        uint8 m_BlendMode; //0=Override / 1=Additive

        void Execute(const CState& state, void* buffers[]) const;
    };


    struct SampleReplaceAnimPart
    {
        enum
        {
            ID = eSampleReplaceAnimPart
        };

        uint8 m_nCommand;
        uint8 m_TargetBuffer;
        uint8 _PADDING0;
        uint8 _PADDING1;

        int32 m_nEAnimID;
        f32 m_fAnimTime; //this is a percentage value between 0-1
        f32 m_fWeight;
        void Execute(const CState& state, void* buffers[]) const;
    };




    //this is only for debugging of uncompiled aim-pose CAFs
    struct SampleAddPoseFull
    {
        enum
        {
            ID = eSampleAddPoseFull
        };
        uint8 m_nCommand;
        uint8 m_flags;
        int16 m_nEAnimID;
        f32 m_fETimeNew; //this is a percentage value between 0-1
        f32 m_fWeight;
        void Execute(const CState& state, void* buffers[]) const;
    };

    struct SamplePosePart
    {
        enum
        {
            ID = eSamplePosePart
        };
        uint8 m_nCommand;
        uint8 m_TargetBuffer;
        uint8 _PADDING0;
        uint8 _PADDING1;
        int32 m_nEAnimID;
        f32 m_fAnimTime; //this is a percentage value between 0-1
        f32 m_fWeight;
        void Execute(const CState& state, void* buffers[]) const;
    };



    struct PoseModifier
    {
        enum
        {
            ID = ePoseModifier
        };

        uint8 m_nCommand;
        uint8 m_TargetBuffer;
        uint8 _PADDING0;
        uint8 _PADDING1;
        IAnimationPoseModifier* m_pPoseModifier;

        void Execute(const CState& state, void* buffers[]) const;
    };

    struct JointMask
    {
        enum
        {
            ID = eJointMask
        };

        uint8 m_nCommand;
        uint8 m_count;
        uint8 _PADDING0;
        uint8 _PADDING1;
        const uint32* m_pMask;

        void Execute(const CState& state, void* buffers[]) const;
    };

    struct VerifyFull
    {
        enum
        {
            ID = eVerifyFull
        };

        uint8 m_nCommand;
        uint8 m_TargetBuffer;
        uint8 _PADDING0;
        uint8 _PADDING1;

        void Execute(const CState& state, void* buffers[]) const;
    };
} // namespace Command

#endif // CRYINCLUDE_CRYANIMATION_COMMAND_COMMANDS_H
