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
#ifndef CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTCLIPVOLUME_H
#define CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTCLIPVOLUME_H
#pragma once

#include <IComponent.h>
#include <ComponentType.h>

//! Interface for the entity's clip-volume component.
//! A clip-volume can be used to define limits for things like
//! a light's illumination.
struct IComponentClipVolume
    : public IComponent
{
    DECLARE_COMPONENT_TYPE("ComponentClipVolume", 0x41B6CD60A3524F5F, 0x861099C97DF3E49C)

    IComponentClipVolume() {}
    virtual void UpdateRenderMesh(IRenderMesh* pRenderMesh, const DynArray<Vec3>& meshFaces) = 0;
    virtual IClipVolume* GetClipVolume() const = 0;
    virtual IBSPTree3D* GetBspTree() const = 0;
};

DECLARE_SMART_POINTERS(IComponentClipVolume);

#endif // CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTCLIPVOLUME_H