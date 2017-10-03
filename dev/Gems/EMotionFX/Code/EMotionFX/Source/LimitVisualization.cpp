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
#include "LimitVisualization.h"
#include "EventManager.h"
#include <MCore/Source/Compare.h>
#include <MCore/Source/FastMath.h>
#include <MCore/Source/Color.h>


namespace EMotionFX
{
    // constructor
    LimitVisualization::LimitVisualization(uint32 numHSegments, uint32 numVSegments)
        : BaseObject()
    {
        mNumHSegments   = numHSegments;
        mNumVSegments   = numVSegments;
        mEllipseRadii   = AZ::Vector2(FLT_MAX, FLT_MAX);

        mFlags.Resize(mNumHSegments * mNumVSegments);
    }


    // creation method
    LimitVisualization* LimitVisualization::Create(uint32 numHSegments, uint32 numVSegments)
    {
        return new LimitVisualization(numHSegments, numVSegments);
    }


    void LimitVisualization::SetFlag(uint32 horizontalIndex, uint32 verticalIndex, bool flag)
    {
        mFlags[horizontalIndex + verticalIndex * mNumHSegments] = flag;
    }


    // construct a point on the surface of a unit sphere based on the number of elements horizontally and vertically
    MCore::Vector3 LimitVisualization::GeneratePointOnSphere(uint32 horizontalIndex, uint32 verticalIndex)
    {
        const float z = (1.0f - (horizontalIndex / (float)(mNumHSegments)) * 2.0f);
        const float r = MCore::Math::Sin(MCore::Math::ACos(z));
        const float p = (verticalIndex / (float)mNumVSegments) * MCore::Math::twoPi;
        const float x = r * MCore::Math::Sin(p);
        const float y = r * MCore::Math::Cos(p);

        return MCore::Vector3(x, z, y);
    }


    // initialize the limit visualization
    void LimitVisualization::Init(const AZ::Vector2& ellipseRadii)
    {
        // skip the initialization in case we have already initialized for the the given ellipse radii
        if (MCore::Compare<AZ::Vector2>::CheckIfIsClose(mEllipseRadii, ellipseRadii, MCore::Math::epsilon))
        {
            return;
        }

        // copy over the ellipse radii to avoid reinitialization in the next call
        mEllipseRadii = ellipseRadii;

        // iterate around the sphere vertically
        for (uint32 v = 0; v < mNumVSegments; ++v)
        {
            // iterate around the sphere horizontally
            for (uint32 h = 0; h < mNumHSegments; ++h)
            {
                // convert into standard emfx coordinate system
                MCore::Vector3 pointOnSphere = GeneratePointOnSphere(h, v);
                pointOnSphere.Normalize();

                // construct the matrix pointing towards the pointOnSphere
                MCore::Vector3 axisForward(0.0f, 1.0f, 0.0f);
                MCore::Vector3 axis = axisForward.Cross(pointOnSphere);
                const float angle = MCore::Math::ACos(axisForward.Dot(pointOnSphere));
                MCore::Matrix lookAtMatrix;
                lookAtMatrix.SetRotationMatrixAxisAngle(axis, angle);

                // calculate the swing and twist
                //MCore::SwingAndTwist swingAndTwist(lookAtMatrix);

                // apply swing constraints
                const bool flag = true; //(swingAndTwist.ConstrainSwingWithEllipse(ellipseRadii) == false);

                // set the flag
                SetFlag(h, v, flag);
            }
        }
    }


    // check if the point on the sphere with the given indices is within the limit or not
    bool LimitVisualization::GetFlag(uint32 horizontalIndex, uint32 verticalIndex) const
    {
        // a we're dealing with a sphere wrap indices bigger than the max index back to zero using modulo
        uint32 h = horizontalIndex;
        if (h >= mNumHSegments)
        {
            h = mNumHSegments % horizontalIndex;
        }

        // do the same for the vertical index
        uint32 v = verticalIndex;
        if (v >= mNumVSegments)
        {
            v = mNumVSegments % verticalIndex;
        }

        // construct the final index in the one dimensional array and check for the flag
        return mFlags[h + v * mNumHSegments];
    }


    // render the limit visualization
    void LimitVisualization::Render(const MCore::Matrix& globalTM, uint32 color, float scale)
    {
        // render solid mesh
        MCore::Vector3  origin  = globalTM.GetTranslation();
        MCore::Vector3  a, b, c, d;

        // iterate around the sphere vertically
        for (uint32 v = 0; v < mNumVSegments; ++v)
        {
            // iterate around the sphere horizontally
            for (uint32 h = 0; h < mNumHSegments; ++h)
            {
                uint32 numEnableVerts = 0;
                if (GetFlag(h + 0, v + 0))
                {
                    numEnableVerts++;
                }
                if (GetFlag(h + 1, v + 0))
                {
                    numEnableVerts++;
                }
                if (GetFlag(h + 0, v + 1))
                {
                    numEnableVerts++;
                }
                if (GetFlag(h + 1, v + 1))
                {
                    numEnableVerts++;
                }

                // render the segment in case all four corner points are within the limit
                if (numEnableVerts == 4)
                {
                    a = (GeneratePointOnSphere(h + 0, v + 0) * scale) * globalTM;
                    b = (GeneratePointOnSphere(h + 1, v + 0) * scale) * globalTM;
                    c = (GeneratePointOnSphere(h + 1, v + 1) * scale) * globalTM;
                    d = (GeneratePointOnSphere(h + 0, v + 1) * scale) * globalTM;

                    MCore::Vector3 faceNormal = (b - a).Cross(c - a);

                    GetEventManager().OnDrawTriangle(a, b, c, faceNormal, faceNormal, faceNormal, color);
                    GetEventManager().OnDrawTriangle(a, c, d, faceNormal, faceNormal, faceNormal, color);
                }
                else if (numEnableVerts == 3)
                {
                    // bottom right triangle
                    if (GetFlag(h, v) && GetFlag(h + 1, v) && GetFlag(h + 1, v + 1))
                    {
                        a = (GeneratePointOnSphere(h + 0, v + 0) * scale) * globalTM;
                        b = (GeneratePointOnSphere(h + 1, v + 0) * scale) * globalTM;
                        c = (GeneratePointOnSphere(h + 1, v + 1) * scale) * globalTM;

                        GetEventManager().OnDrawTriangle(a, b, c, a - origin, b - origin, c - origin, color);
                    }
                    // top left triangle
                    else if (GetFlag(h, v) && GetFlag(h + 1, v + 1) && GetFlag(h, v + 1))
                    {
                        a = (GeneratePointOnSphere(h + 0, v + 0) * scale) * globalTM;
                        b = (GeneratePointOnSphere(h + 1, v + 1) * scale) * globalTM;
                        c = (GeneratePointOnSphere(h + 0, v + 1) * scale) * globalTM;

                        GetEventManager().OnDrawTriangle(a, b, c, a - origin, b - origin, c - origin, color);
                    }
                    // bottom left triangle
                    else if (GetFlag(h, v) && GetFlag(h + 1, v) && GetFlag(h, v + 1))
                    {
                        a = (GeneratePointOnSphere(h + 0, v + 0) * scale) * globalTM;
                        b = (GeneratePointOnSphere(h + 1, v + 0) * scale) * globalTM;
                        c = (GeneratePointOnSphere(h + 0, v + 1) * scale) * globalTM;

                        GetEventManager().OnDrawTriangle(a, b, c, a - origin, b - origin, c - origin, color);
                    }
                    // top right triangle
                    else if (GetFlag(h, v + 1) && GetFlag(h + 1, v) && GetFlag(h + 1, v + 1))
                    {
                        a = (GeneratePointOnSphere(h + 0, v + 1) * scale) * globalTM;
                        b = (GeneratePointOnSphere(h + 1, v + 0) * scale) * globalTM;
                        c = (GeneratePointOnSphere(h + 1, v + 1) * scale) * globalTM;

                        GetEventManager().OnDrawTriangle(a, b, c, a - origin, b - origin, c - origin, color);
                    }
                }
            }
        }

        // render all triangles that got added for the limit viz
        //  GetEventManager().OnDrawTriangles();            // commented because of an NVidia OpenGL 2 driver crash in EMStudio
    }
}   // namespace EMotionFX
