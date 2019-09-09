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

#include "IGPUParticleEngine.h"

class CImpl_GPUParticles;
class CD3DGPUParticleEngine
    : public IGPUParticleEngine
{
public:
    CD3DGPUParticleEngine();
    ~CD3DGPUParticleEngine();

    virtual bool Initialize(CRenderer* renderer) override;
    virtual void Release() override;

    virtual EmitterTypePtr GetEmitterByIndex(int index) override;
    virtual int GetEmitterCount() override;

    virtual SShaderItem* GetRenderShader() override;

    // ----------------------------------------------
    //!< Called from the main thread.
    virtual void SetEmitterResourceParameters(EmitterTypePtr emitter, const ResourceParticleParams* parameters) override;
    virtual void SetEmitterTransform(EmitterTypePtr emitter, const Matrix34& transform) override;
    virtual void SetEmitterLodBlendAlpha(EmitterTypePtr emitter, const float lodBlendAlpha) override;

    virtual EmitterTypePtr AddEmitter(const ResourceParticleParams* parameters, const SpawnParams& spawnParams, const unsigned* emitterFlags, EmitterTypePtr parent, const ParticleTarget* target, const SPhysEnviron* physEnv) override;
    virtual bool RemoveEmitter(EmitterTypePtr emitter) override;
    virtual void ResetEmitter(EmitterTypePtr emitter) override;
    virtual void StartEmitter(EmitterTypePtr pEmitter) override;
    virtual void StopEmitter(EmitterTypePtr pEmitter) override;
    virtual void PrimeEmitter(EmitterTypePtr pEmitter, float equilibriumAge) override;

    virtual void OnEffectChanged(EmitterTypePtr emitter) override;

    virtual void QueueEmitterNextFrame(EmitterTypePtr emitter, bool isAuxWindowUpdate) override;

    void QueueRenderElementToReleaseNextFrame(CREParticleGPU* renderElement) override;

    void OnCubeDepthMapResolutionChanged(ICVar*) override;

    // ----------------------------------------------
    //!< Called from render thread.
    virtual void UpdateFrame() override;

    virtual void Update(EmitterTypePtr emitter) override;
    virtual void Render(EmitterTypePtr emitter, EGPUParticlePass pass, int shadowMode, float fov, float aspectRatio, bool isWireframeEnabled) override;

protected:
    CImpl_GPUParticles* m_impl;
};