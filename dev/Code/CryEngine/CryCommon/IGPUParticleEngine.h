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

struct SpawnParams;
struct ResourceParticleParams;
class CRenderer;
class CREParticleGPU;
class CParticleContainerGPU;
struct ParticleTarget;
struct SPhysEnviron;

enum class EGPUParticlePass
{
    Main,
    Shadow,
    Reflection
};

class IGPUParticleEngine
{
public:
    virtual ~IGPUParticleEngine(){}
    typedef void* EmitterTypePtr;

    virtual bool Initialize(CRenderer* renderer) = 0;
    virtual void Release() = 0;

    virtual EmitterTypePtr GetEmitterByIndex(int index) = 0;
    virtual int GetEmitterCount() = 0;

    virtual SShaderItem* GetRenderShader() = 0;

    //!< Interface designed to be non thread-safe.

    // ----------------------------------------------
    //!< Expected to be called from the main thread.
    virtual void SetEmitterTransform(EmitterTypePtr emitter, const Matrix34& transform) = 0;
    virtual void SetEmitterResourceParameters(EmitterTypePtr emitter, const ResourceParticleParams* parameters) = 0;
    virtual void SetEmitterLodBlendAlpha(EmitterTypePtr emitter, const float lodBlendAlpha) = 0;

    virtual EmitterTypePtr AddEmitter(const ResourceParticleParams* parameters, const SpawnParams& spawnParams, const unsigned* emitterFlags, EmitterTypePtr parent, const ParticleTarget* target, const SPhysEnviron* phys_env) = 0;
    virtual bool RemoveEmitter(EmitterTypePtr emitter) = 0;
    virtual void ResetEmitter(EmitterTypePtr emitter) = 0;
    virtual void StartEmitter(EmitterTypePtr pEmitter) = 0;
    virtual void StopEmitter(EmitterTypePtr pEmitter) = 0;
    virtual void PrimeEmitter(EmitterTypePtr pEmitter, float equilibriumAge) = 0;

    virtual void QueueEmitterNextFrame(EmitterTypePtr emitter, bool isAuxWindowUpdate) = 0;            //!< Expects emitter to be only added once - does not check for duplicate updates!

    virtual void QueueRenderElementToReleaseNextFrame(CREParticleGPU* renderElement) = 0;

    virtual void OnEffectChanged(EmitterTypePtr emitter) = 0;

    virtual void OnCubeDepthMapResolutionChanged(ICVar*) {};

    // ----------------------------------------------
    //!< Expected to be called from the render thread.
    virtual void UpdateFrame() = 0;

    virtual void Update(EmitterTypePtr emitter) = 0;
    virtual void Render(EmitterTypePtr emitter, EGPUParticlePass pass, int shadowMode, float fov, float aspectRatio, bool isWireframeEnabled) = 0;
};

