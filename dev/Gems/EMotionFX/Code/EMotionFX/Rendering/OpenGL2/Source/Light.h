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

#ifndef __RENDERGL_LIGHT__H
#define __RENDERGL_LIGHT__H

#include "RenderGLConfig.h"
#include <MCore/Source/Vector.h>
#include <MCore/Source/Color.h>


namespace RenderGL
{
    struct RENDERGL_API Light
    {
        Light()
        {
            dir = MCore::Vector3(-1.0f, 0.0f, -1.0f);

            diffuse  = MCore::RGBAColor(1.0f, 1.0f, 1.0f, 1.0f);
            specular = MCore::RGBAColor(1.0f, 1.0f, 1.0f, 1.0f);
            ambient  = MCore::RGBAColor(0.0f, 0.0f, 0.0f, 0.0f);
        }

        MCore::Vector3 dir;

        MCore::RGBAColor diffuse;
        MCore::RGBAColor specular;
        MCore::RGBAColor ambient;
    };
}

#endif
