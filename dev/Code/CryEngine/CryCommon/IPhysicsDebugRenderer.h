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

#ifndef CRYINCLUDE_CRYCOMMON_IPHYSICSDEBUGRENDERER_H
#define CRYINCLUDE_CRYCOMMON_IPHYSICSDEBUGRENDERER_H
#pragma once

struct IPhyicalWorld;
class CCamera;

// this object implements rendering of debug information
// through IPhysRenderer (CryPhysics) with IRenderer (XRender)
struct IPhysicsDebugRenderer
{
    // <interfuscator:shuffle>
    virtual ~IPhysicsDebugRenderer(){}
    virtual void UpdateCamera(const CCamera& camera) = 0;
    virtual void DrawAllHelpers(IPhysicalWorld* world) = 0;
    virtual void DrawEntityHelpers(IPhysicalEntity* entity, int helperFlags) = 0;
    virtual void Flush(float dt) = 0;
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_IPHYSICSDEBUGRENDERER_H
