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

#include <AzCore/Math/Color.h>

namespace AzFramework
{
    /**
     * Various colors used by the editor and shared between objects
     */
    namespace ViewportColors
    {
        /**
         * Color to use for a deselected object
         */
        static const AZ::Color DeselectedColor( 1.0f, 1.0f, 0.78f, 0.4f );

        /**
         * Color to use for a selected object
         */
        static const AZ::Color SelectedColor( 1.0, 1.0f, 0.0f, 0.9f );

        /** 
         * Color to use when hovering
         */
        static const AZ::Color HoverColor( 1.0f, 1.0f, 0.0f, 0.9f );

        /**
        * Color to use for wireframe
        */
        static const AZ::Color WireColor( 1.0f, 1.0f, 0.78f, 0.5f );
    }
} // namespace AzToolsFramework

