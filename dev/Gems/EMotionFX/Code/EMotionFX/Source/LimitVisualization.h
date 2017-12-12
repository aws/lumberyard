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

// include required headers
#include "EMotionFXConfig.h"
#include "BaseObject.h"

#include <MCore/Source/Array.h>
#include <MCore/Source/UnicodeString.h>
#include <MCore/Source/Vector.h>


namespace EMotionFX
{
    class EMFX_API LimitVisualization
        : public BaseObject
    {
        MCORE_MEMORYOBJECTCATEGORY(LimitVisualization, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_VISUALIZATION_LIMITS);

    public:
        static LimitVisualization* Create(uint32 numHSegments, uint32 numVSegments);

        MCore::Vector3 GeneratePointOnSphere(uint32 horizontalIndex, uint32 verticalIndex);

        void SetFlag(uint32 horizontalIndex, uint32 verticalIndex, bool flag);
        bool GetFlag(uint32 horizontalIndex, uint32 verticalIndex) const;

        void Init(const AZ::Vector2& ellipseRadii);
        void Render(const MCore::Matrix& globalTM, uint32 color, float scale);

    private:
        MCore::Array<bool>      mFlags;
        AZ::Vector2             mEllipseRadii;
        uint32                  mNumHSegments;
        uint32                  mNumVSegments;

        LimitVisualization(uint32 numHSegments, uint32 numVSegments);
    };
}   // namespace EMotionFX
