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

namespace RoadsAndRivers
{
    class SplineGeometry;

    namespace EditorUtils
    {
        /**
         * Erases vegetation around spline geometry, using width and other properties like erase radius and randomized variance
         */
        void EraseVegetation(const SplineGeometry& splineGeometry, float eraseVegetationWidth, float eraseVegetationVariance);

        struct AlignHeightmapParams
        {
            float terrainBorderWidth = 5.0f;
            float riverBedDepth = 0.0f;
            float riverBedWidth = 0.0f;
            float embankment = 0.0f;
            bool dontExceedSplineLength = false;
        };

        /**
         * Aligns heightmap so it maches spline geometry elevation
         */
        void AlignHeightMap(const SplineGeometry& splineGeometry, AlignHeightmapParams params);
    }
}
