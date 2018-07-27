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

#ifndef CRYINCLUDE_CRYAISYSTEM_FLYHELPERS_DEBUG_H
#define CRYINCLUDE_CRYAISYSTEM_FLYHELPERS_DEBUG_H
#pragma once


namespace FlyHelpers
{
    void DrawDebugLocation(const Vec3& position, const ColorB& color)
    {
        gEnv->pRenderer->GetIRenderAuxGeom()->DrawCone(position + Vec3(0, 0, 2), Vec3(0, 0, -1), 1, 2, color);
    }

    void DrawDebugPath(const Path& path, const ColorB& color)
    {
        char buffer[ 16 ];
        const size_t segmentCount = path.GetSegmentCount();
        for (size_t i = 0; i < segmentCount; ++i)
        {
            const Lineseg segment = path.GetSegment(i);
            gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(segment.start, color, segment.end, color);

            sprintf_s(buffer, "%d", uint32(i));
            gEnv->pRenderer->DrawLabel(segment.start, 1.0f, buffer);
        }
    }
}


#endif // CRYINCLUDE_CRYAISYSTEM_FLYHELPERS_DEBUG_H
