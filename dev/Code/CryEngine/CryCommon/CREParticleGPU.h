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
#ifndef CRYINCLUDE_CRYCOMMON_CREPARTICLEGPU_H
#define CRYINCLUDE_CRYCOMMON_CREPARTICLEGPU_H
#pragma once

#include "IGPUParticleEngine.h"

class CREParticleGPU
    : public CRendElementBase
{
public:
    CREParticleGPU();

    // CRendElement implementation.
    virtual CRendElementBase* mfCopyConstruct();
    virtual int Size();

    void* GetInstance() const;
    void SetInstance(void* instance) { m_instance = instance; }

    void SetPass(EGPUParticlePass pass) { m_pass = pass;  }
    void SetShadowMode(int shadowMode) { m_shadowMode = shadowMode; }
    void SetCameraFOV(float fov) { m_cameraFOV = fov; }
    void SetAspectRatio(float aspectRatio) { m_aspectRatio = aspectRatio; }
    void SetWireframeEnabled(bool isWireframeEnabled) { m_isWireframeEnabled = isWireframeEnabled; }

    virtual void mfPrepare(bool bCheckOverflow);
    virtual bool mfPreDraw(SShaderPass* sl);
    virtual bool mfDraw(CShader* ef, SShaderPass* sl);
protected:
    void* m_instance;
    int m_shadowMode;

    //needed for camera distance fade (for consistency with CPU version)
    float m_cameraFOV;

    //used in particle motion blur
    float m_aspectRatio;

    EGPUParticlePass m_pass;
    bool m_isWireframeEnabled;
};

#endif // CRYINCLUDE_CRYCOMMON_CREPARTICLEGPU_H
