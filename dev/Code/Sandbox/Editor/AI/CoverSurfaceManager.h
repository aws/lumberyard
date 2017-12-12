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

#ifndef CRYINCLUDE_EDITOR_AI_COVERSURFACEMANAGER_H
#define CRYINCLUDE_EDITOR_AI_COVERSURFACEMANAGER_H
#pragma once


#include <ICoverSystem.h>


class CAICoverSurface;
class CCoverSurfaceManager
{
public:
    CCoverSurfaceManager();
    virtual ~CCoverSurfaceManager();

    bool WriteToFile(const char* fileName);
    bool ReadFromFile(const char* fileName);

    void ClearGameSurfaces();
    void AddSurfaceObject(CAICoverSurface* surface);
    void RemoveSurfaceObject(CAICoverSurface* surface);

    typedef std::set<CAICoverSurface*> SurfaceObjects;
    const SurfaceObjects& GetSurfaceObjects() const;

private:
    SurfaceObjects m_surfaceObjects;
};

#endif // CRYINCLUDE_EDITOR_AI_COVERSURFACEMANAGER_H
