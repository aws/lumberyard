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

#include <Cry_Math.h>
#include "../EditorCommon/QViewportSettings.h"
#include "Serialization.h"

namespace CharacterTool
{
    enum CharacterMovement
    {
        CHARACTER_MOVEMENT_INPLACE,
        CHARACTER_MOVEMENT_REPEATED,
        CHARACTER_MOVEMENT_CONTINUOUS
    };

    enum CompressionPreview
    {
        COMPRESSION_PREVIEW_COMPRESSED,
        COMPRESSION_PREVIEW_BOTH
    };

    struct DisplayAnimationOptions
    {
        CharacterMovement movement;
        CompressionPreview compressionPreview;
        bool showTrail;
        bool showLocator;
        bool animationEventGizmos;
        bool showDccToolOrigin;
        // state variables
        bool resetCharacter;
        bool resetGrid;
        bool updateCameraTarget;

        DisplayAnimationOptions()
            : movement(CHARACTER_MOVEMENT_INPLACE)
            , compressionPreview(COMPRESSION_PREVIEW_COMPRESSED)
            , showTrail(true)
            , showLocator(false)
            , animationEventGizmos(true)
            , showDccToolOrigin(false)
            , resetCharacter(false)
            , resetGrid(false)
            , updateCameraTarget(true)
        {
        }

        void Serialize(IArchive& ar);
    };

    struct DisplayRenderingOptions
    {
        bool showEdges;

        DisplayRenderingOptions()
            : showEdges(false)
        {
        }

        void Serialize(Serialization::IArchive& ar);
    };

    struct DisplaySkeletonOptions
    {
        string jointFilter;
        bool showJoints;
        bool showJointNames;
        bool showSkeletonBoundingBox;

        DisplaySkeletonOptions()
            : showJoints(false)
            , showJointNames(false)
            , showSkeletonBoundingBox(false)
        {
        }

        void Serialize(Serialization::IArchive& ar);
    };

    struct DisplaySecondaryAnimationOptions
    {
        bool showDynamicProxies;
        bool showAuxiliaryProxies;
        bool showClothProxies;
        bool showRagdollProxies;

        DisplaySecondaryAnimationOptions()
            : showDynamicProxies(false)
            , showAuxiliaryProxies(false)
            , showClothProxies(false)
            , showRagdollProxies(false)
        {
        }

        void Serialize(Serialization::IArchive& ar);
    };

    struct DisplayPhysicsOptions
    {
        bool showPhysicalProxies;
        bool showRagdollJointLimits;

        DisplayPhysicsOptions()
            : showPhysicalProxies(false)
            , showRagdollJointLimits(false)
        {
        }

        void Serialize(Serialization::IArchive& ar);
    };

    struct DisplayFollowJointOptions
    {
        string jointName;
        bool lockAlign;
        bool usePosition;
        bool useRotation;

        DisplayFollowJointOptions()
            : lockAlign(false)
            , usePosition(true)
            , useRotation(false)
        {
        }

        void Serialize(Serialization::IArchive& ar);
    };


    struct DisplayOptions
    {
        bool attachmentAndPoseModifierGizmos;
        DisplayAnimationOptions animation;
        DisplayRenderingOptions rendering;
        DisplaySkeletonOptions skeleton;
        DisplaySecondaryAnimationOptions secondaryAnimation;
        DisplayPhysicsOptions physics;
        DisplayFollowJointOptions followJoint;
        SViewportSettings viewport;

        DisplayOptions()
            : attachmentAndPoseModifierGizmos(true)
        {
        }

        void Serialize(Serialization::IArchive& ar);
    };
}
