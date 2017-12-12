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

#pragma once
#include <vector>
#include "Strings.h"
#include "Serialization.h"
#include <NameCRCHelper.h>

struct IAnimationSet;

namespace CharacterTool
{
    using std::vector;

    struct BlendSpaceDimension
    {
        string parameterName;
        int32 parameterId;
        float minimal;
        float maximal;
        uint8 cellCount;
        float debugVisualScale;
        float startKey;
        float endKey;
        string jointName;
        bool locked;

        BlendSpaceDimension()
            : parameterId(0)
            , minimal(0)
            , maximal(1)
            , cellCount(8)
            , debugVisualScale(1.0f)
            , startKey(0.0f)
            , endKey(1.0f)
            , locked(false)
        {
        }

        void Serialize(IArchive& ar);
    };
    typedef vector<BlendSpaceDimension> BlendSpaceDimensions;

    struct BlendSpaceAnnotation
    {
        vector<int> indices;

        void Serialize(IArchive& ar)
        {
            ar(indices, "indices", "^");
        }
    };
    typedef vector<BlendSpaceAnnotation> BlendSpaceAnnotations;

    struct BlendSpaceExample
    {
        string animation;
        Vec4 parameters;
        bool specified[4];
        bool useDirectlyForDeltaMotion[4];
        float playbackScale;

        BlendSpaceExample()
            : playbackScale(1.0f)
            , parameters(0.0f, 0.0f, 0.0f, 0.0f)
        {
            memset(specified, 0, sizeof(specified));
            memset(useDirectlyForDeltaMotion, 0, sizeof(useDirectlyForDeltaMotion));
        }

        void Serialize(IArchive& ar);
    };
    typedef vector<BlendSpaceExample> BlendSpaceExamples;

    struct BlendSpaceAdditionalExtraction
    {
        string parameterName;
        int32 parameterId;

        BlendSpaceAdditionalExtraction()
            : parameterId(0)
        {
        }
    };
    typedef vector<BlendSpaceAdditionalExtraction> BlendSpaceAdditionalExtractions;

    bool Serialize(IArchive& ar, BlendSpaceAdditionalExtraction& value, const char* name, const char* label);

    struct BlendSpacePseudoExample
    {
        int i0, i1;
        float w0;
        float w1;

        BlendSpacePseudoExample()
            : i0(0)
            , i1(0)
            , w0(0.5f)
            , w1(0.5f)
        {
        }

        void Serialize(IArchive& ar);
    };
    typedef vector<BlendSpacePseudoExample> BlendSpacePseudoExamples;

    struct BlendSpaceMotionCombination
    {
        string animation;
    };
    typedef vector<BlendSpaceMotionCombination> BlendSpaceMotionCombinations;

    inline bool Serialize(IArchive& ar, BlendSpaceMotionCombination& anim, const char* name, const char* label)
    {
        return ar(AnimationAlias(anim.animation), name, label);
    }

    struct BlendSpaceJoint
    {
        string name;

        bool operator<(const BlendSpaceJoint& rhs) const { return name < rhs.name; }
    };

    inline bool Serialize(IArchive& ar, BlendSpaceJoint& joint, const char* name, const char* label)
    {
        return ar(JointName(joint.name), name, label);
    }

    struct BlendSpaceVirtualExample1D
    {
        uint8 i0, i1;
        float w0, w1;

        void Serialize(IArchive& ar)
        {
            ar(i0, "i0", "^i0");
            ar(i1, "i1", "^i1");
            ar(w0, "w0", "^w0");
            ar(w1, "w1", "^w1");
        }
    };

    struct BlendSpaceVirtualExample2D
    {
        uint8 i0, i1, i2, i3;
        float w0, w1, w2, w3;

        void Serialize(IArchive& ar)
        {
            ar(i0, "i0", "^i0");
            ar(i1, "i1", "^i1");
            ar(i2, "i2", "^i2");
            ar(i3, "i3", "^i3");
            ar(w0, "w0", "^w0");
            ar(w1, "w1", "^w1");
            ar(w2, "w2", "^w2");
            ar(w3, "w3", "^w3");
        }
    };

    struct BlendSpaceVirtualExample3D
    {
        uint8 i0, i1, i2, i3, i4, i5, i6, i7;
        float w0, w1, w2, w3, w4, w5, w6, w7;

        void Serialize(IArchive& ar)
        {
            ar(i0, "i0", "^i0");
            ar(i1, "i1", "^i1");
            ar(i2, "i2", "^i2");
            ar(i3, "i3", "^i3");
            ar(i4, "i4", "^i4");
            ar(i5, "i5", "^i5");
            ar(i6, "i6", "^i6");
            ar(i7, "i7", "^i7");
            ar(w0, "w0", "^w0");
            ar(w1, "w1", "^w1");
            ar(w2, "w2", "^w2");
            ar(w3, "w3", "^w3");
            ar(w4, "w4", "^w4");
            ar(w5, "w5", "^w5");
            ar(w6, "w6", "^w6");
            ar(w7, "w7", "^w7");
        }
    };

    struct BlendSpace
    {
        float m_threshold;
        bool m_idleToMove;
        BlendSpaceDimensions m_dimensions;
        BlendSpaceExamples m_examples;
        BlendSpacePseudoExamples m_pseudoExamples;
        BlendSpaceAdditionalExtractions m_additionalExtraction;
        BlendSpaceAnnotations m_annotations;
        vector<BlendSpaceMotionCombination> m_motionCombinations;
        vector<BlendSpaceJoint> m_joints;
        vector<BlendSpaceVirtualExample1D> m_virtualExamples1d;
        vector<BlendSpaceVirtualExample2D> m_virtualExamples2d;
        vector<BlendSpaceVirtualExample3D> m_virtualExamples3d;

        bool LoadFromXml(string& errorMessage, XmlNodeRef root, IAnimationSet* pAnimationSet);
        XmlNodeRef SaveToXml() const;
        bool HasVGridData() const;

        BlendSpace()
            : m_idleToMove(false)
            , m_threshold(-99999.f)
        {
        }

        void Serialize(IArchive& ar);
    };

    struct CombinedBlendSpaceDimension
    {
        string parameterName;
        int parameterId;
        bool locked;
        float parameterScale;
        bool chooseBlendSpace;

        CombinedBlendSpaceDimension()
            : locked(false)
            , parameterScale(1.0f)
            , chooseBlendSpace(false)
        {
        }

        void Serialize(IArchive& ar);
    };
    typedef vector<CombinedBlendSpaceDimension> CombinedBlendSpaceDimensions;

    struct BlendSpaceReference
    {
        string path;
    };
    typedef vector<BlendSpaceReference> BlendSpaceReferences;

    bool Serialize(IArchive& ar, BlendSpaceReference& ref, const char* name, const char* label);


    struct CombinedBlendSpace
    {
        bool m_idleToMove;
        CombinedBlendSpaceDimensions m_dimensions;
        BlendSpaceAdditionalExtractions m_additionalExtraction;
        BlendSpaceReferences m_blendSpaces;
        BlendSpaceMotionCombinations m_motionCombinations;
        vector<BlendSpaceJoint> m_joints;

        CombinedBlendSpace()
            : m_idleToMove(false)
        {
        }

        void Serialize(IArchive& ar)
        {
            ar(m_idleToMove, "idleToMove", "Idle To Move");
            ar(m_dimensions, "dimensions", "Dimensions");
            ar(m_additionalExtraction, "m_additionalExtraction", "Additional Extraction");
            ar(m_blendSpaces, "blendSpaces", "Blend Spaces");
            ar(m_motionCombinations, "motionCombinations", "Motion Combinations");
            ar(m_joints, "joints", "Joints");
        }

        bool LoadFromXml(string& errorMessage, XmlNodeRef root, IAnimationSet* pAnimationSet);
        XmlNodeRef SaveToXml() const;
    };
}
