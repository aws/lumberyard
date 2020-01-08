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
#ifndef CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTCAMERA_H
#define CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTCAMERA_H
#pragma once

#include <IComponent.h>
#include <ComponentType.h>

class CCamera;

//! This component is used to render from the camera's perspective
//! to the entity's material.
struct IComponentCamera
    : public IComponent
{
    // <interfuscator:shuffle>
    DECLARE_COMPONENT_TYPE("ComponentCamera", 0x417D5BD7B2F84AED, 0xADC4DD6A1DD14C2F)

    IComponentCamera() {}

    ComponentEventPriority GetEventPriority(const int eventID) const override { return EntityEventPriority::Camera; }

    virtual void SetCamera(CCamera& cam) = 0;
    virtual CCamera& GetCamera() = 0;
    // </interfuscator:shuffle>
};

DECLARE_SMART_POINTERS(IComponentCamera);

#endif // CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTCAMERA_H