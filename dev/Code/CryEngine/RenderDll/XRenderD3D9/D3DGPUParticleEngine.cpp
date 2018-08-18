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

#include "StdAfx.h"
#include "../Common/Renderer.h"
#include "../../CryCommon/IShader.h"
#include "DriverD3D.h"
#include "../../Cry3DEngine/ParticleEffect.h"
#include "D3DPostProcess.h"
#include "../../Cry3DEngine/ParticleEnviron.h"
#include "../../CryEngine/CryCommon/ParticleParams.h"

#include "D3DGPUParticleEngine.h"
#include "D3DGPUParticleProfiler.h"
#include "GPUTimerFactory.h"

#include <AzCore/Casting/numeric_cast.h>
#include <vector>
#include "Common/RenderView.h"

// - Constants
#define D3DGPUParticleEngine_MAX_SUBEMITTER_NUM 10
#define D3DGPUParticleEngine_MIN_SUBEMITTER_NUM 1
#define D3DGPUParticleEngine_GROUP_FACTORY 1.5f

static const char* s_GPUParticles_Material_RenderTechniqueName = "General";
static const char* s_GPUParticles_Material_RenderShadowTechniqueName = "ShadowGen";
static const char* s_GPUParticles_Material_UpdateTechniqueName = "GPUParticleUpdate";

static const char* s_GPUParticles_Material_TechniqueName = "Default";

static const char* s_GPUParticles_Material_GPUBegin = "GPUParticleBegin";
static const char* s_GPUParticles_Material_GPUEmit = "GPUParticleEmit";
static const char* s_GPUParticles_Material_GPUUpdate = "GPUParticleUpdate";
static const char* s_GPUParticles_Material_GPURenderNoGS = "GPUParticleRenderNoGS";
static const char* s_GPUParticles_Material_GPUBitonicSort = "GPUParticleBitonicSort";
static const char* s_GPUParticles_Material_GPUBitonicSortGlobal2048 = "GPUParticleBitonicSortGlobal2048";
static const char* s_GPUParticles_Material_GPUBitonicSortLocal = "GPUParticleBitonicSortLocal";
static const char* s_GPUParticles_Material_GPUOddEvenSort = "GPUParticleOddEvenSort";
static const char* s_GPUParticles_Material_GPUGatherSortDistance = "GPUParticleGatherSortDistance";

static const int s_GPUParticles_CurveNumSamples = 32;
static const int s_GPUParticles_CurveNumSlots = 26;
static const int s_GPUParticles_CurveTextureSlot = 9;
static const int s_GPUParticles_CurveTextureHeight = 32;    // must match GPUParticleCurves.cfi (MAX_NUM_PARTICLE_CURVE_SAMPLES)

// Note: Thread group values must correspond with X value ([numthreads(X,1,1)]) in matching shader!
// These values are chosen in the optimization phase. The occupancy was greater and performance was better when we changed the thread group size to 256.
static const int s_GPUParticles_ThreadGroupUpdateX = 256;
static const int s_GPUParticles_ThreadGroupBeginX = 256;
static const int s_GPUParticles_ThreadGroupEmitX = 128;
static const int s_GPUParticles_ThreadGroupSortBitonicXLocal = 1024;
static const int s_GPUParticles_ThreadGroupSortBitonicX = 256;
static const int s_GPUParticles_ThreadGroupSortOddEvenX = 256;
static const int s_GPUParticles_ThreadGroupSortGatherX = 256;
static const int s_GPUParticles_ThreadGroupMaximalX = MAX(MAX(MAX(MAX(s_GPUParticles_ThreadGroupUpdateX, s_GPUParticles_ThreadGroupEmitX), s_GPUParticles_ThreadGroupSortBitonicX), s_GPUParticles_ThreadGroupSortOddEvenX), s_GPUParticles_ThreadGroupSortGatherX);

static const float nearPlane = 0.2f;


#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define D3DGPUPARTICLEENGINE_CPP_SECTION_1 1
#define D3DGPUPARTICLEENGINE_CPP_SECTION_2 2
#define D3DGPUPARTICLEENGINE_CPP_SECTION_3 3
#define D3DGPUPARTICLEENGINE_CPP_SECTION_4 4
#define D3DGPUPARTICLEENGINE_CPP_SECTION_5 5
#define D3DGPUPARTICLEENGINE_CPP_SECTION_6 6
#define D3DGPUPARTICLEENGINE_CPP_SECTION_7 7
#define D3DGPUPARTICLEENGINE_CPP_SECTION_8 8
#define D3DGPUPARTICLEENGINE_CPP_SECTION_9 9
#define D3DGPUPARTICLEENGINE_CPP_SECTION_10 10
#define D3DGPUPARTICLEENGINE_CPP_SECTION_11 11
#define D3DGPUPARTICLEENGINE_CPP_SECTION_12 12
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION D3DGPUPARTICLEENGINE_CPP_SECTION_1
#include AZ_RESTRICTED_FILE(D3DGPUParticleEngine_cpp, AZ_RESTRICTED_PLATFORM)
#endif
static const Vec3 s_GPUParticles_RotationOffset = Vec3(90, 0, 0);

struct GPUEmitterResources
{
    GPUEmitterResources()
        : numMaxParticlesInBuffer(0)
        , texSampledCurves(0)
        , depthCubemap(nullptr)
    {
    }

    ~GPUEmitterResources()
    {
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION D3DGPUPARTICLEENGINE_CPP_SECTION_2
#include AZ_RESTRICTED_FILE(D3DGPUParticleEngine_cpp, AZ_RESTRICTED_PLATFORM)
#endif
    }


    WrappedDX11Buffer bufEmitterState;                  // bind as u0 or t0
    WrappedDX11Buffer bufParticleData;                  // bind as u1 or t1
    WrappedDX11Buffer bufParticleDeadlist;              // bind as u2 or t2
    WrappedDX11Buffer bufParticleDistance;
    WrappedDX11Buffer bufParticleIndices;
    WrappedDX11Buffer bufDeadParticleLocations;
    WrappedDX11Buffer bufDeadParticleLocationsStaging;
    WrappedDX11Buffer bufWindAreas;

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION D3DGPUPARTICLEENGINE_CPP_SECTION_3
#include AZ_RESTRICTED_FILE(D3DGPUParticleEngine_cpp, AZ_RESTRICTED_PLATFORM)
#endif

    unsigned int texSampledCurves;                      // bind as t9
    CTexture* depthCubemap;

    int numMaxParticlesInBuffer;
    int numDeathLocations;

#if PROFILE_GPU_PARTICLES
    CGPUParticleProfiler gpuParticleProfiler;
#endif // PROFILE_GPU_PARTICLES
};

struct GPUSubemitterData
{
    GPUSubemitterData()
        : accumParticleSpawn(1.0f) // This starts at 1 to ensure that the first particle gets spawned on the first frame rather than waiting for 1*SpawnRate
        , accumParticlePulse(1.0f)
        , paramCurrentCount(1.0f)
        , paramCurrentPulsePeriod(1.0f)
        , renderTimeInMilliseconds(0)
        , particleSpawnPerformanceScale(1.0f)
        , chaosKey(0u)
        , transform(IDENTITY)
        , strength(0)
    {
    }

    float accumParticleSpawn;
    float accumParticlePulse;
    float paramCurrentCount;
    float paramCurrentPulsePeriod;
    UFloat renderTimeInMilliseconds;
    UnitFloat particleSpawnPerformanceScale;

    ParticleTarget target; //need a separate target per subemitter to allow for indirect spawning targetting own position

    //used for focus computation
    CChaosKey chaosKey;

    Matrix34 transform;

    float repeatAge;
    float startAge;
    float stopAge;
    float deathAge;
    float strength;

    //the offset to get to our portion of the data buffers
    int bufferOffset;
};

//! Collects data required for a single render pass.
struct RenderPassData
{
    RenderPassData()
        : cameraPos(0, 0, 0, 0)
        , cameraRightAxis(1, 0, 0, 0)
        , cameraUpAxis(0, 0, 1, 0)
        , cameraLookDir(0, -1, 0, 0)
        , projection(IDENTITY)
        , modelview(IDENTITY)
        , pass(EGPUParticlePass::Main)
        , shadowMode(0)
        , fov(0)
        , aspectRatio(0)
        , isWireframeEnabled(false)
    {}

    Matrix44 modelview;
    Matrix44 projection;
    Vec4 cameraPos;
    Vec4 cameraRightAxis;
    Vec4 cameraUpAxis;
    Vec4 cameraLookDir;
    EGPUParticlePass pass;
    int shadowMode;
    float fov;
    float aspectRatio;
    bool isWireframeEnabled;
};

//! Collects data to track the state of a single emitter.
struct GPUEmitterBaseData
{
    GPUEmitterBaseData()
        : isFirstUpdatePass(true)
        , numLastMaxParticles(0)
        , stagedMaxParticlesInBuffer(0)
        , time(0.0f)
        , deltaTime(0.0f)
        , transform(IDENTITY)
        , previousTransform(IDENTITY)
        , emitterIncrementXYZ(0, 0, 0)
        , emitterIncrementXYZprev(0, 0, 0)
        , lodBlendAlpha(0.5f)
        , emitterFlags(0)
        , isIndirect(false)
        , isIndirectParent(false)
        , isPrimed(false)
        , numWindAreas(0)
        , windAreaBufferSize(0)
    {}

    uint64_t frameQueued;
    bool isFirstUpdatePass;

    const unsigned* emitterFlags;
    const ParticleTarget* target;
    const SPhysEnviron* physEnv;
    Matrix34 transform;
    Matrix34 previousTransform;

    Vec3 emitterIncrementXYZ;
    Vec3 emitterIncrementXYZprev;

    // actual counts allocated & in use
    int numLastMaxParticles;
    int numParticlesPerChunk;

    float time;
    float deltaTime;

    // counts that we want for the current emitter, and need to be actualized
    int stagedMaxParticlesInBuffer;

    float lodBlendAlpha;

    bool isAuxWindowUpdate;
    bool isIndirect;
    bool isIndirectParent;
    bool isPrimed;

    int numWindAreas;
    int windAreaBufferSize;
};

struct SGPUParticleEmitterData
{
    SGPUParticleEmitterData(const SpawnParams& a_spawnParams)
        : baseData()
        , spawnParams(a_spawnParams)
        , resourceParameters(0)
    {}

    GPUEmitterResources resources;
    GPUEmitterBaseData baseData;
    const ResourceParticleParams* resourceParameters;
    const SpawnParams& spawnParams;

    int numMaxSubemitters; //this is how many we have space for, not a hard limit
    std::stack<int> availableOffsets;
    std::vector<GPUSubemitterData> subEmitters;

    std::vector<SGPUParticleEmitterData*> children;
    SGPUParticleEmitterData* parent;
};

struct SGPUParticleCurve
{
    float samples[s_GPUParticles_CurveNumSamples];
};

struct SGPUParticleData
{
    enum EParticleDataFlags
    {
        PARTICLE_FLAG_ORIENTTOVELOCITY = 0,
        // TODO: when there are more facing modes we can reserve some bits in the flag particle parameter to support multiple values
        PARTICLE_FLAG_FREE_FACING = 1,
        PARTICLE_FLAG_COLLIDE_DEPTH_BUFFER = 2,
        PARTICLE_FLAG_ON_COLLISION_DIE = 3,
        PARTICLE_FLAG_ON_COLLISION_BOUNCE = 4,
        PARTICLE_FLAG_ON_COLLISION_STOP     = 5,
        PARTICLE_FLAG_TARGET_ATTRACTION_SHRINK = 6,
        PARTICLE_FLAG_TARGET_ATTRACTION_ORBIT = 7,
        PARTICLE_FLAG_TARGET_ATTRACITON_EXTEND_SPEED = 8,
        PARTICLE_FLAG_FACING_SHAPE = 9,
    };

    // mirrored type definitions from HLSL
    typedef float float2[2];
    typedef float float3[3];
    typedef float float4[4];
    typedef unsigned int uint;

    float4 position;
    // - float4 boundary (16 bytes)

    float4 rgba;
    // - float4 boundary (32 bytes)

    float4 velocity;
    // - float4 boundary (48 bytes)

    float4 rotation;
    // - float4 boundary (64 bytes)

    float4 previous_rotation_pos_z;
    // - float4 boundary ( 80 bytes)

    float2 scale;
    float2 pivotXY;
    // - float4 boundary ( 96 bytes)

    float life;
    float lifeMax;
    float stretch;
    uint flags; // bit 0 is set as facing mode flag
    // - float4 boundary (112 bytes)

    float2 previous_position_xy;
    uint random0; // 4 random values - 8 bit per channel, intended for low quality random values
    uint padding;
    // - float4 boundary (128 bytes)

};

struct SGPUWindArea
{
    enum AreaShape
    {
        SHAPE_BOX = 0, SHAPE_SPHERE
    };

    Vec3 wind_direction;
    int is_radial;

    Vec3 position;
    float falloffScale;

    Vec3 aabb_min;
    int shape;

    Vec3 aabb_max;
    float pad;

    Matrix34 matToLocal;
};

class CImpl_GPUParticles
{
public:
    // Renderer
    CRenderer* renderer;
    CD3D9Renderer* dxRenderer;

    // GPU Shaders
    CShader* shrGpuBegin;
    CShader* shrGpuEmit;
    CShader* shrGpuUpdate;
    CShader* shrGpuGatherSortDistance;
    CShader* shrGpuBitonicSort;
    CShader* shrGpuBitonicSortGlobal2048;
    CShader* shrGpuBitonicSortLocal;
    CShader* shrGpuOddEvenSort;
    SShaderItem shrGpuRender;

    //Flag to mark when shaders need to be reloaded
    bool requireShaderReload = false;

    // GPU Particle buffer
    std::vector<SGPUParticleEmitterData*> emitters;
    std::vector<SGPUParticleEmitterData*> emittersQueuedForUpdate;
    
    // Used in RenderEmiter() to measure render-time for throttling emission rates.
    IGPUTimer* gpuTimer;

    // Sort odd even incremental
    uint32 oddEvenLValue;
    uint32 oddEvenPValue;
    uint32 oddEvenDValue;
    uint32 oddEvenTotalPasses;
    uint32 oddEvenConvergedPasses;

    // These track the camera state data that was used to render the most recent frame (the main pass, not shadows/reflections/etc passes)
    // Needed mostly for update calculations based on the depth buffer, like depth-based collision detection.
    Matrix44 latestModelview;
    Matrix44 latestProjection;
    Vec4 latestCameraPos;

    bool initialized;

    CImpl_GPUParticles()
        : latestModelview(IDENTITY)
        , latestProjection(IDENTITY)
        , latestCameraPos(0)
    {
        gpuTimer = GPUTimerFactory::Create();
    }

    ~CImpl_GPUParticles()
    {
        delete gpuTimer;
    }

    void InitializeEmitter(SGPUParticleEmitterData* emitter);
    void InitializeBuffers(GPUEmitterResources& resources, int numParticlesRequested, const ResourceParticleParams* particleParams, const SpawnParams& spawnParams, bool isIndirectParent, int numSubemitters, int numWindAreas);
    void GrowBuffers(SGPUParticleEmitterData& emitter);
    float RenderEmitter(const RenderPassData& renderPassData, const GPUEmitterBaseData& baseData, const GPUEmitterResources& resources, const GPUSubemitterData& emitData, const ResourceParticleParams* particleParams, const SpawnParams& spawnParams);
    void EmitParticles(SGPUParticleEmitterData* emitter);
    void EmitParticlesGPU(GPUEmitterBaseData& baseData, const GPUEmitterResources& resources, const GPUSubemitterData& emitData, const ResourceParticleParams* particleParams, const SpawnParams& spawnParams, int numSpawnParticles);
    void BeginUpdate(GPUEmitterResources& resources, int numSubEmitters);
    void UpdateGPU(GPUEmitterBaseData& baseData, GPUEmitterResources& resources, GPUSubemitterData& emitData, const ResourceParticleParams* particleParams, const SpawnParams& spawnParams);
    void EndUpdate(GPUEmitterBaseData& baseData);
    void UpdateEmitterCounts(SGPUParticleEmitterData* pEmitter);
    void UpdateParameterCount(GPUSubemitterData& emitData, const ResourceParticleParams* particleParams, const SpawnParams& spawnParams);
    void UpdateParameterPulsePeriod(GPUSubemitterData& emitData, const ResourceParticleParams* particleParams, const SpawnParams& spawnParams);
    void UpdateShaders();
    void GatherSortScore(GPUEmitterResources& pEmitter);
    void SortParticlesBitonicLocal(GPUEmitterResources& emitter, GPUEmitterBaseData& baseData, GPUSubemitterData& emitData);
    void SortParticlesBitonic(GPUEmitterResources& emitter, GPUEmitterBaseData& baseData, GPUSubemitterData& emitData);
    void SortParticlesOddEvenIncremental(GPUEmitterResources& resources, GPUEmitterBaseData& baseData, GPUSubemitterData& emitData, const ResourceParticleParams* particleParams);
    void UpdateGPUCurveData(GPUEmitterResources& emitter, const ResourceParticleParams* particleParams);
    void RenderEmitterCubeDepthmap(SGPUParticleEmitterData* emitter);
    void OnEffectChanged(GPUEmitterResources& resources, GPUEmitterBaseData& baseData, const ResourceParticleParams* particleParams, int emitterFlags);
    void LoadShaderItems();
    void AddSubemitter(SGPUParticleEmitterData* emitter, int offset);
    void SpawnSubemitter(SGPUParticleEmitterData* emitter, Vec3 position);
    void ReadbackDeathLocationsAndSpawnChildren(SGPUParticleEmitterData& emitter);
    void SetEmitterShapeFlag(ParticleParams::EEmitterShapeType type);
};



struct ParticleSystemEventListener
    : public ISystemEventListener
{
public:
    virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
    {
        switch (event)
        {
        case ESYSTEM_EVENT_LEVEL_UNLOAD:
        {
            if (renderer)
            {
                renderer->GetGPUParticleEngine()->Release();
            }
        }
        break;
        }
    }
    CRenderer* renderer = nullptr;
};
static ParticleSystemEventListener g_system_event_listener;

static CImpl_GPUParticles* gpuParticleEngine_impl = nullptr;

// *** Helpers
static inline int RoundUpAndDivide(int Value, int RoundTo)
{
    return ((Value + RoundTo - 1) / RoundTo);
}

static inline int RoundUp(int Value, int RoundTo)
{
    // rounds value up to multiple of RoundTo
    // Example: RoundTo: 4
    // Value:  0 1 2 3 4 5 6 7 8
    // Result: 0 4 4 4 4 8 8 8 8
    return RoundUpAndDivide(Value, RoundTo) * RoundTo;
}

static inline bool BindTextureByID(CRenderer* renderer, int textureID, int slotID, int nState = -1, int sUnit = -1, EHWShaderClass type = EHWShaderClass::eHWSC_Pixel)
{
    ITexture* texture = renderer->EF_GetTextureByID(textureID);

    if (texture && texture->IsTextureLoaded())
    {
        reinterpret_cast<CTexture*>(texture)->Apply(slotID, nState, EFTT_UNKNOWN, sUnit, SResourceView::DefaultView, type);
        return true;
    }
    return false;
}

static inline ID3D11ShaderResourceView* GetTextureSRVByID(CRenderer* renderer, int textureID)
{
    ITexture* texture = renderer->EF_GetTextureByID(textureID);

    if (texture && texture->IsTextureLoaded())
    {
        return reinterpret_cast<ID3D11ShaderResourceView*>(reinterpret_cast<CTexture*>(texture)->GetShaderResourceView());
    }
    return nullptr;
}

template <class T>
static void SampleCurveToBuffer(const TCurve<T>& floatCurve, SGPUParticleCurve& sampledCurve)
{
    // sample the curve and store in sampledCurve object. samples from 0 to 1 (inclusively).
    const int curveSamples = s_GPUParticles_CurveNumSamples - 1;
    CRY_ASSERT(curveSamples);

    float stepSize = 1.0f / static_cast<float>(curveSamples);
    for (int step = 0; step < s_GPUParticles_CurveNumSamples; step++)
    {
        float x = static_cast<float>(step) * stepSize;
        sampledCurve.samples[step] = floatCurve.GetValue(x);
    }
}

template <class T>
static void SampleCurveToBufferXYZ(const TCurve<T>& floatCurve, SGPUParticleCurve* sampledCurve)
{
    // sample the curve and store in sampledCurve object. samples from 0 to 1 (inclusively).
    const int curveSamples = s_GPUParticles_CurveNumSamples - 1;
    CRY_ASSERT(curveSamples);

    float stepSize = 1.0f / static_cast<float>(curveSamples);
    for (int step = 0; step < s_GPUParticles_CurveNumSamples; step++)
    {
        float x = static_cast<float>(step) * stepSize;
        sampledCurve[0].samples[step] = floatCurve.GetValue(x).x;
        sampledCurve[1].samples[step] = floatCurve.GetValue(x).y;
        sampledCurve[2].samples[step] = floatCurve.GetValue(x).z;
    }
}

static void ConvertCurveBufferToTextureData(const SGPUParticleCurve* curve, unsigned char* outTextureData)
{
    for (int i = 0; i < s_GPUParticles_CurveNumSamples; i++)
    {
        outTextureData[i] = static_cast<unsigned char>(CLAMP(curve->samples[i] * 255.0f, 0.0f, 255.0f));
    }
}

static Vec3 GetFocusDirection(const GPUSubemitterData& emitter, const ResourceParticleParams* particleParams)
{
    Quat q;

    float angle = DEG2RAD(particleParams->fFocusAngle(emitter.chaosKey, emitter.strength));
    float azimuth = DEG2RAD(particleParams->fFocusAzimuth(emitter.chaosKey, emitter.strength));

    q = Quat::CreateRotationXYZ(Ang3(angle, 0, azimuth));

    Vec3 focus = q.GetColumn2();

    return focus;
}

static void BindEmitterDataToCS(const GPUSubemitterData& emitData, const GPUEmitterBaseData& baseData, const ResourceParticleParams* particleParams, const SpawnParams& spawnParams, CShader* shader, IRenderer* renderer)
{
    CRY_ASSERT(shader);
    CRY_ASSERT(particleParams);

    // set global constants
    static const CCryNameR paramFrameInfo0("ParticleFrameInfo0");
    Vec4 vecFrameInfo0(baseData.deltaTime, baseData.time, 0/*unused*/, reinterpret_cast<const float&>(emitData.bufferOffset));
    shader->FXSetCSFloat(paramFrameInfo0, &vecFrameInfo0, 1);

    // calculate strength influences
    float emtStrengthParticleLifetime =     particleParams->fParticleLifeTime.GetEmitterStrength().GetValue(emitData.strength);
    float emtStrengthSpeed =                particleParams->fSpeed.GetEmitterStrength().GetValue(emitData.strength);
    float emtStrengthSizeX =                particleParams->fSizeX.GetEmitterStrength().GetValue(emitData.strength);
    float emtStrengthSizeY =                particleParams->fSizeY.GetEmitterStrength().GetValue(emitData.strength);
    float emtStrengthDrag =                 particleParams->fAirResistance.GetEmitterStrength().GetValue(emitData.strength);
    float emtStrengthGravity =              particleParams->fGravityScale.GetEmitterStrength().GetValue(emitData.strength);
    float emtStrengthTurbulence3DSpeed =    particleParams->fTurbulence3DSpeed.GetEmitterStrength().GetValue(emitData.strength);
    float emtStrengthTurbulenceSize =       particleParams->fTurbulenceSize.GetEmitterStrength().GetValue(emitData.strength);
    float emtStrengthTurbulenceSpeed =      particleParams->fTurbulenceSpeed.GetEmitterStrength().GetValue(emitData.strength);
    float emtStrengthRotationRateX =        particleParams->vRotationRateX.GetEmitterStrength().GetValue(emitData.strength);
    float emtStrengthRotationRateY =        particleParams->vRotationRateY.GetEmitterStrength().GetValue(emitData.strength);
    float emtStrengthRotationRateZ =        particleParams->vRotationRateZ.GetEmitterStrength().GetValue(emitData.strength);
    Color3F emtStrengthColor0 =             particleParams->cColor.GetEmitterStrength().GetValue(emitData.strength);
    Color3F emtStrengthColor1 =             particleParams->cColor.GetSecondaryEmitterStrength().GetValue(emitData.strength);
    float emtStrengthAlpha0 =               particleParams->fAlpha.GetEmitterStrength().GetValue(emitData.strength);
    float emtStrengthAlpha1 =               particleParams->fAlpha.GetSecondaryEmitterStrength().GetValue(emitData.strength);
    float emtStrengthPivotX =               particleParams->fPivotX.GetEmitterStrength().GetValue(emitData.strength);
    float emtStrengthPivotY =               particleParams->fPivotY.GetEmitterStrength().GetValue(emitData.strength);
    float emtStrengthStretch =              particleParams->fStretch.GetEmitterStrength().GetValue(emitData.strength);
    float emtStrengthAttractionRadius =     particleParams->TargetAttraction.fRadius.GetEmitterStrength().GetValue(emitData.strength);

    // bind fixed base values
    static const CCryNameR paramEmitterDataWorld("ed_world");
    static const CCryNameR paramEmitterDataWorldRotation("ed_world_rotation");
    static const CCryNameR paramEmitterDataInvWorld("ed_inv_world");
    static const CCryNameR paramEmitterDataLife("ed_life_random");
    static const CCryNameR paramEmitterDataScale("ed_scalexy_random");
    static const CCryNameR paramEmitterDataDragRandom("ed_drag_random");
    static const CCryNameR paramEmitterDataColor("ed_color");
    static const CCryNameR paramEmitterDataColorRandom("ed_color_random");
    static const CCryNameR paramEmitterDataColorEmtStrength("ed_colorEmtStrength");
    static const CCryNameR paramEmitterDataColorEmtStrengthRandom("ed_colorEmtStrength_random");
    static const CCryNameR paramEmitterDataPosition("ed_positionOffset");
    static const CCryNameR paramEmitterDataPositionRandom("ed_positionOffset_random");
    static const CCryNameR paramEmitterDataAccelGravity("ed_acceleration_gravity");
    static const CCryNameR paramEmitterDataPositionDelta("ed_world_position_delta");
    static const CCryNameR paramEmitterDataWorldPosition("ed_world_position");
    static const CCryNameR paramEmitterDataOffsetRoundnessFractionAngleRandom("ed_offset_roundness_fraction_emitangle_random");
    static const CCryNameR paramEmitterDataTurbulence3dSpeedSize("ed_turbulence_3dspeed_size_random");
    static const CCryNameR paramEmitterDataTurbulenceSpeed("ed_turbulence_speed_random");
    static const CCryNameR paramEmitterDataRotationInit("ed_rotationInit_rotationRateXrandom");
    static const CCryNameR paramEmitterDataRotationRandom("ed_rotationRandom_rotationRateYrandom");
    static const CCryNameR paramEmitterDataRotationRate("ed_rotationRate_rotationRateZrandom");
    static const CCryNameR parameterEmitterDataPivotXY("ed_pivotXY");
    static const CCryNameR parameterEmitterDataStretch("ed_stretch");
    static const CCryNameR parameterEmitterDataFlagsAndCollision("ed_flags_collisionRadius");
    static const CCryNameR parameterEmitterDataFocusDirection("ed_focus_direction");
    static const CCryNameR parameterTargetPositionRadius("ed_target_position_radius");
    static const CCryNameR parameterTargetVelocity("ed_target_velocity_maxObjSize");
    static const CCryNameR parameterTargetAttractionParams("ed_target_radiusRandom");
    
    Vec4 vecEmitterLifeRandomSpread(
        particleParams->fParticleLifeTime * emtStrengthParticleLifetime,
        particleParams->fParticleLifeTime * emtStrengthParticleLifetime * -particleParams->fParticleLifeTime.GetRandomRange(),
        particleParams->fSpeed * emtStrengthSpeed,
        particleParams->fSpeed * emtStrengthSpeed * -particleParams->fSpeed.GetRandomRange());
    Vec4 vecEmitterScaleRandom(
        particleParams->fSizeX * emtStrengthSizeX,
        particleParams->fSizeX * emtStrengthSizeX * -particleParams->fSizeX.GetRandomRange(),
        particleParams->fSizeY * emtStrengthSizeY,
        particleParams->fSizeY * emtStrengthSizeY * -particleParams->fSizeY.GetRandomRange());
    Vec4 vecEmitterDragRandom(
        particleParams->fAirResistance * emtStrengthDrag,
        particleParams->fAirResistance * emtStrengthDrag * -particleParams->fAirResistance.GetRandomRange(),
        particleParams->fGravityScale * emtStrengthGravity,
        particleParams->fGravityScale * emtStrengthGravity * -particleParams->fGravityScale.GetRandomRange());
    Vec4 vecEmitterPosition = Vec4(
            particleParams->vSpawnPosOffset.x,
            particleParams->vSpawnPosOffset.y,
            particleParams->vSpawnPosOffset.z,
            0.0f);
    Vec4 vecEmitterPositionRandomness = Vec4(
            particleParams->vSpawnPosRandomOffset.x,
            particleParams->vSpawnPosRandomOffset.y,
            particleParams->vSpawnPosRandomOffset.z,
            particleParams->fInheritVelocity);
    Vec4 vecEmitterAccelGravity = Vec4(
            particleParams->vAcceleration.x,
            particleParams->vAcceleration.y,
            particleParams->vAcceleration.z,
            1.0f);
    Vec4 vecEmitterPositionDelta = Vec4(
            baseData.transform.GetTranslation() - baseData.previousTransform.GetTranslation(),
            0.0f);
    Vec4 vecEmitterWorldPosition = Vec4(
        baseData.transform.GetTranslation(),
        0.0f);
    Vec4 vecEmitterOffsetRoundnessFractionAngleRandom = Vec4(
            particleParams->fOffsetRoundness,
            particleParams->fSpawnIncrement,
            DEG2RAD(particleParams->fEmitAngle(VMAX, emitData.strength)),
            particleParams->fEmitAngle.GetRandomRange());
    Vec4 vecEmitterTurbulence3DSpeedSize = Vec4(
            particleParams->fTurbulence3DSpeed * emtStrengthTurbulence3DSpeed,
            particleParams->fTurbulence3DSpeed * emtStrengthTurbulence3DSpeed * -particleParams->fTurbulence3DSpeed.GetRandomRange(),
            particleParams->fTurbulenceSize * emtStrengthTurbulenceSize,
            particleParams->fTurbulenceSize * emtStrengthTurbulenceSize * -particleParams->fTurbulenceSize.GetRandomRange());
    Vec4 vecEmitterTurbulenceSpeed = Vec4(
            particleParams->fTurbulenceSpeed * emtStrengthTurbulenceSpeed,
            particleParams->fTurbulenceSpeed * emtStrengthTurbulenceSpeed * -particleParams->fTurbulenceSpeed.GetRandomRange(),
            0.0f,
            0.0f);
    Vec4 vecRotationInitXRand = Vec4(
            particleParams->vInitAngles,
            particleParams->vRotationRateX * emtStrengthRotationRateX * -particleParams->vRotationRateX.GetRandomRange());
    Vec4 vecRotationRandomYRand = Vec4(
            particleParams->vRandomAngles,
            particleParams->vRotationRateY * emtStrengthRotationRateY * -particleParams->vRotationRateY.GetRandomRange());
    Vec4 vecRotationRateZRand = Vec4(
            particleParams->vRotationRateX * emtStrengthRotationRateX,
            particleParams->vRotationRateY * emtStrengthRotationRateY,
            particleParams->vRotationRateZ * emtStrengthRotationRateZ,
            particleParams->vRotationRateZ * emtStrengthRotationRateZ * -particleParams->vRotationRateZ.GetRandomRange());
    Vec4 vecPivotsXY = Vec4(
            particleParams->fPivotX * emtStrengthPivotX,
            particleParams->fPivotX * emtStrengthPivotX * -particleParams->fPivotX.GetRandomRange(),
            particleParams->fPivotY * emtStrengthPivotY,
            particleParams->fPivotY * emtStrengthPivotY * -particleParams->fPivotY.GetRandomRange());
    Vec4 vecStretch = Vec4(
            particleParams->fStretch * emtStrengthStretch,
            particleParams->fStretch * emtStrengthStretch * -particleParams->fStretch.GetRandomRange(),
            0, //use random hue is packed into this to save memory
            0);

    // * Color logic
    // final_color = lerp(colorMin, colorMax, 1.0-rnd0) * lerp(emtStrengthMin, emtStrengthMax, 1.0-rnd0) * lerp(particleAgeColorMin, particleAgeColorMax, 1.0-rnd0)
    // final_color = (colorMax + colorRandom * rnd0) * (colorEmtStrengthMax + colorEmtStrengthRandom * rnd0) * (particleAgeColorMax + particleAgeColorRandom * rnd0)
    Vec4 vecEmitterColor = Vec4(
            particleParams->cColor.x,
            particleParams->cColor.y,
            particleParams->cColor.z,
            particleParams->fAlpha);
    Color3F randomrange = particleParams->cColor.GetRandomRange().GetRandom();
    Vec4 vecEmitterColorRandom = Vec4(
            particleParams->cColor.x * -randomrange[0],
            particleParams->cColor.y * -randomrange[1],
            particleParams->cColor.z * -randomrange[2],
            particleParams->fAlpha * -particleParams->fAlpha.GetRandomRange()
            );
    Vec4 vecEmitterStrengthColor;
    Vec4 vecEmitterStrengthColorRandom;
    Vec4 vecEmitterFocusDirection = Vec4(GetFocusDirection(emitData, particleParams), 0.f);

    if (particleParams->cColor.GetRandomLerpStrength())
    {
        vecEmitterStrengthColor = Vec4(
                emtStrengthColor1.x,
                emtStrengthColor1.y,
                emtStrengthColor1.z,
                emtStrengthAlpha1
                );
        // we calculate the differences and on the gpu we generate a random number between 0 and 1 and do a multiply and add to the base value
        vecEmitterStrengthColorRandom = Vec4(
                emtStrengthColor0.x - emtStrengthColor1.x,
                emtStrengthColor0.y - emtStrengthColor1.y,
                emtStrengthColor0.z - emtStrengthColor1.y,
                emtStrengthAlpha0 - emtStrengthAlpha1
                );
    }
    else
    {
        vecEmitterStrengthColor = Vec4(
                emtStrengthColor0.x,
                emtStrengthColor0.y,
                emtStrengthColor0.z,
                emtStrengthAlpha0
                );
        vecEmitterStrengthColorRandom = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    // set the per flags, note that we save these per particle so we can use these flags with global particle sorting in the future
    ParticleParams::ECollisionResponse collisionResponse = particleParams->eFinalCollision;
    unsigned int particleFlags = 0;
    particleFlags |= static_cast<unsigned int>(particleParams->bOrientToVelocity) << SGPUParticleData::EParticleDataFlags::PARTICLE_FLAG_ORIENTTOVELOCITY;        // bit 0 == orient to velocity false (0); orient to velocity true (1)
    particleFlags |= static_cast<unsigned int>(particleParams->eFacingGpu == ParticleParams::EFacingGpu::Free) << SGPUParticleData::EParticleDataFlags::PARTICLE_FLAG_FREE_FACING;                    // bit 1 == facing mode camera (0); facing mode free (1)
    particleFlags |= static_cast<unsigned int>((particleParams->eDepthCollision == ParticleParams::EDepthCollision::FrameBuffer) && !baseData.isAuxWindowUpdate) << SGPUParticleData::EParticleDataFlags::PARTICLE_FLAG_COLLIDE_DEPTH_BUFFER;// bit 0 == collide with depth buffer is false (0); collide with depth buffer is true (1)
    particleFlags |= static_cast<unsigned int>(collisionResponse == ParticleParams::ECollisionResponse::Die) << SGPUParticleData::EParticleDataFlags::PARTICLE_FLAG_ON_COLLISION_DIE;// bit 0 == die on collision is false (0); die on collision is true (1)
    particleFlags |= static_cast<unsigned int>(collisionResponse == ParticleParams::ECollisionResponse::Bounce) << SGPUParticleData::EParticleDataFlags::PARTICLE_FLAG_ON_COLLISION_BOUNCE;// bit 0 == ignore collision except for bouncyness? false (0); ignore collision except for bouncyness? true (1)
    particleFlags |= static_cast<unsigned int>(collisionResponse == ParticleParams::ECollisionResponse::Stop) << SGPUParticleData::EParticleDataFlags::PARTICLE_FLAG_ON_COLLISION_STOP;// bit 0 == collide with depth buffer is false (0); collide with depth buffer is true (1)
    particleFlags |= static_cast<unsigned int>(particleParams->TargetAttraction.bShrink) << SGPUParticleData::EParticleDataFlags::PARTICLE_FLAG_TARGET_ATTRACTION_SHRINK;
    particleFlags |= static_cast<unsigned int>(particleParams->TargetAttraction.bOrbit) << SGPUParticleData::EParticleDataFlags::PARTICLE_FLAG_TARGET_ATTRACTION_ORBIT;
    particleFlags |= static_cast<unsigned int>(particleParams->TargetAttraction.bExtendSpeed) << SGPUParticleData::EParticleDataFlags::PARTICLE_FLAG_TARGET_ATTRACITON_EXTEND_SPEED;
    particleFlags |= static_cast<unsigned int>(particleParams->eFacingGpu == ParticleParams::EFacingGpu::Shape) << SGPUParticleData::EParticleDataFlags::PARTICLE_FLAG_FACING_SHAPE;


    unsigned int collisionResolveFlags = static_cast<unsigned int>(particleParams->eFinalCollision);
    Vec4 vecFlagsCollisionRadiusAndFlags = Vec4(reinterpret_cast<float&>(particleFlags), // encode integer as an invalid float (will be decoded on the GPU)
            particleParams->fThickness,
            particleParams->fElasticity,
            0);

    Vec4 vecTargetPositionRadius = Vec4(emitData.target.vTarget, emitData.target.fRadius);
    Vec4 vecTargetVelocity = Vec4(emitData.target.vVelocity, particleParams->GetMaxObjectSize());
    Vec4 vecTargetAttractionParams =
        Vec4(particleParams->TargetAttraction.fRadius * emtStrengthAttractionRadius,
            particleParams->TargetAttraction.fRadius * emtStrengthAttractionRadius * -particleParams->TargetAttraction.fRadius.GetRandomRange(), 0, 0);

    Matrix44 mtxWorld(emitData.transform);
    Matrix44 mtxWorldRotation(baseData.transform);
    mtxWorldRotation.SetTranslation(Vec3(0.0f, 0.0f, 0.0f));
    Matrix44 mtxInvWorld = mtxWorld.GetInverted();
    
    shader->FXSetCSFloat(paramEmitterDataWorld, reinterpret_cast<Vec4*>(mtxWorld.GetData()), 4);
    shader->FXSetCSFloat(paramEmitterDataWorldRotation, reinterpret_cast<Vec4*>(mtxWorldRotation.GetData()), 4);
    shader->FXSetCSFloat(paramEmitterDataInvWorld, reinterpret_cast<Vec4*>(mtxInvWorld.GetData()), 4);
    shader->FXSetCSFloat(paramEmitterDataLife, &vecEmitterLifeRandomSpread, 1);
    shader->FXSetCSFloat(paramEmitterDataScale, &vecEmitterScaleRandom, 1);
    shader->FXSetCSFloat(paramEmitterDataColor, &vecEmitterColor, 1);
    shader->FXSetCSFloat(paramEmitterDataColorRandom, &vecEmitterColorRandom, 1);
    shader->FXSetCSFloat(paramEmitterDataColorEmtStrength, &vecEmitterStrengthColor, 1);
    shader->FXSetCSFloat(paramEmitterDataColorEmtStrengthRandom, &vecEmitterStrengthColorRandom, 1);
    shader->FXSetCSFloat(paramEmitterDataPosition, &vecEmitterPosition, 1);
    shader->FXSetCSFloat(paramEmitterDataPositionRandom, &vecEmitterPositionRandomness, 1);
    shader->FXSetCSFloat(paramEmitterDataDragRandom, &vecEmitterDragRandom, 1);
    shader->FXSetCSFloat(paramEmitterDataAccelGravity, &vecEmitterAccelGravity, 1);
    shader->FXSetCSFloat(paramEmitterDataPositionDelta, &vecEmitterPositionDelta, 1);
    shader->FXSetCSFloat(paramEmitterDataWorldPosition, &vecEmitterWorldPosition, 1);
    shader->FXSetCSFloat(paramEmitterDataOffsetRoundnessFractionAngleRandom, &vecEmitterOffsetRoundnessFractionAngleRandom, 1);
    shader->FXSetCSFloat(paramEmitterDataTurbulence3dSpeedSize, &vecEmitterTurbulence3DSpeedSize, 1);
    shader->FXSetCSFloat(paramEmitterDataTurbulenceSpeed, &vecEmitterTurbulenceSpeed, 1);
    shader->FXSetCSFloat(paramEmitterDataRotationInit, &vecRotationInitXRand, 1);
    shader->FXSetCSFloat(paramEmitterDataRotationRandom, &vecRotationRandomYRand, 1);
    shader->FXSetCSFloat(paramEmitterDataRotationRate, &vecRotationRateZRand, 1);
    shader->FXSetCSFloat(parameterEmitterDataPivotXY, &vecPivotsXY, 1);
    shader->FXSetCSFloat(parameterEmitterDataStretch, &vecStretch, 1);
    shader->FXSetCSFloat(parameterEmitterDataFlagsAndCollision, &vecFlagsCollisionRadiusAndFlags, 1);
    shader->FXSetCSFloat(parameterEmitterDataFocusDirection, &vecEmitterFocusDirection, 1);
    shader->FXSetCSFloat(parameterTargetPositionRadius, &vecTargetPositionRadius, 1);
    shader->FXSetCSFloat(parameterTargetVelocity, &vecTargetVelocity, 1);
    shader->FXSetCSFloat(parameterTargetAttractionParams, &vecTargetAttractionParams, 1);

    // Spawn parameters
    static const CCryNameR spawnParamScales("sp_particleScaleXY_globalScale_speedScale");
    const Vec4 vecSpawnParamScales(spawnParams.particleSizeScale.x, spawnParams.particleSizeScale.y, spawnParams.fSizeScale, spawnParams.fSpeedScale);
    shader->FXSetCSFloat(spawnParamScales, &vecSpawnParamScales, 1);

    static const CCryNameR paramVelocityXYZ("ed_paramVelocityXYZ");
    static const CCryNameR paramVelocityXYZRandRange("ed_paramVelocityXYZRandRange");
    Vec4 vecVelocityXYZ = Vec4(particleParams->vVelocity.fX.GetValue(emitData.strength, 1.f),
                                particleParams->vVelocity.fY.GetValue(emitData.strength, 1.f),
                                particleParams->vVelocity.fZ.GetValue(emitData.strength, 1.f),
                                particleParams->fVelocitySpread.GetValue(1.f, emitData.chaosKey) * 0.5f);
    Vec4 vecVelocityXYZRandRange = vecVelocityXYZ * Vec4(  -particleParams->vVelocity.fX.GetRandomRange(),
                                                    -particleParams->vVelocity.fY.GetRandomRange(),
                                                    -particleParams->vVelocity.fZ.GetRandomRange(), 0);
    shader->FXSetCSFloat(paramVelocityXYZ, &vecVelocityXYZ, 1);
    shader->FXSetCSFloat(paramVelocityXYZRandRange, &vecVelocityXYZRandRange, 1);

    static const CCryNameR paramVelocityRandom("ed_paramVelocityXYZRandom");
    static const CCryNameR paramVelocityRandomRandRange("ed_paramVelocityXYZRandomRandRange");
    Vec4 vecVelocityRandom = Vec4(  particleParams->vVelocityXYZRandom.fX.GetValue(emitData.strength, 1.f),
                                    particleParams->vVelocityXYZRandom.fY.GetValue(emitData.strength, 1.f),
                                    particleParams->vVelocityXYZRandom.fZ.GetValue(emitData.strength, 1.f),
                                    0);
    Vec4 vecVelocityRandomRandRange = vecVelocityRandom * Vec4(-particleParams->vVelocityXYZRandom.fX.GetRandomRange(),
                                                               -particleParams->vVelocityXYZRandom.fY.GetRandomRange(),
                                                               -particleParams->vVelocityXYZRandom.fZ.GetRandomRange(), 0);
    shader->FXSetCSFloat(paramVelocityRandom, &vecVelocityRandom, 1);
    shader->FXSetCSFloat(paramVelocityRandomRandRange, &vecVelocityRandomRandRange, 1);

    if (particleParams->GetEmitterShape() == ParticleParams::EEmitterShapeType::SPHERE ||
        particleParams->GetEmitterShape() == ParticleParams::EEmitterShapeType::CIRCLE ||
        particleParams->GetEmitterShape() == ParticleParams::EEmitterShapeType::POINT ||
        particleParams->GetEmitterShape() == ParticleParams::EEmitterShapeType::BOX)
    {
        if (particleParams->GetEmitterShape() == ParticleParams::EEmitterShapeType::POINT)
        {
            static const CCryNameR paramVelocitySpread("ed_velSpreadSpawnOffset");
            float curVelSpread = particleParams->fVelocitySpread(VRANDOM, emitData.strength);
            float curOffset = particleParams->fSpawnOffset(VRANDOM, emitData.strength);
            Vec4 vecVelocitySpread = Vec4( curVelSpread,
                                           curVelSpread * -particleParams->fVelocitySpread.GetRandomRange(),
                                           curOffset,
                                           curOffset * -particleParams->fEmitterSizeDiameter.GetRandomRange());
            shader->FXSetCSFloat(paramVelocitySpread, &vecVelocitySpread, 1);
        }
        else if (particleParams->GetEmitterShape() == ParticleParams::EEmitterShapeType::BOX)
        {
            static const CCryNameR paramEmitterSizeXYZ("ed_emitterSizeXYZ");
            static const CCryNameR paramEmitterSizeXYZRand("ed_emitterSizeXYZRand");
            Vec4 vecEmitterSizeXYZ =
                Vec4(particleParams->vEmitterSizeXYZ.fX.GetValue(emitData.strength, 1.f),
                     particleParams->vEmitterSizeXYZ.fY.GetValue(emitData.strength, 1.f),
                     particleParams->vEmitterSizeXYZ.fZ.GetValue(emitData.strength, 1.f),
                     0.0f);
            Vec4 vecEmitterSizeXYZRand = vecEmitterSizeXYZ *
                Vec4(-particleParams->vEmitterSizeXYZ.fX.GetRandomRange(),
                     -particleParams->vEmitterSizeXYZ.fY.GetRandomRange(),
                     -particleParams->vEmitterSizeXYZ.fZ.GetRandomRange(),
                     0.0f);
            shader->FXSetCSFloat(paramEmitterSizeXYZ, &vecEmitterSizeXYZ, 1);
            shader->FXSetCSFloat(paramEmitterSizeXYZRand, &vecEmitterSizeXYZRand, 1);
        }
        else if (particleParams->GetEmitterShape() == ParticleParams::EEmitterShapeType::SPHERE ||
                 particleParams->GetEmitterShape() == ParticleParams::EEmitterShapeType::CIRCLE)
        {
            // TODO: Move emitter increment to the GPU. Then this variable can be deleted.
            static const CCryNameR paramEmitterIncrementXYZprev("ed_emitterIncrementXYZprev");
            Vec4 vecEmitterIncrementXYZprev(baseData.emitterIncrementXYZprev, 0);
            shader->FXSetCSFloat(paramEmitterIncrementXYZprev, &vecEmitterIncrementXYZprev, 1);

            // spawn pos increment XYZ
            static const CCryNameR paramSpawnPosIncrementXYZ("ed_spawnPosIncrementXYZ");
            static const CCryNameR paramSpawnPosIncrementXYZRand("ed_spawnPosIncrementXYZRandRange");
            Vec4 vecSpawnPosInc = Vec4( particleParams->vSpawnPosIncrementXYZ.fX.GetValue(emitData.strength, 1.f),
                                        particleParams->vSpawnPosIncrementXYZ.fY.GetValue(emitData.strength, 1.f),
                                        particleParams->vSpawnPosIncrementXYZ.fZ.GetValue(emitData.strength, 1.f),
                                        0);
            vecSpawnPosInc.w = baseData.emitterIncrementXYZ.x;
            Vec4 vecSpawnPosIncRand = vecSpawnPosInc *
                                    Vec4(-particleParams->vSpawnPosIncrementXYZ.fX.GetRandomRange(),
                                        -particleParams->vSpawnPosIncrementXYZ.fY.GetRandomRange(),
                                        -particleParams->vSpawnPosIncrementXYZ.fZ.GetRandomRange(),
                                        0);
            vecSpawnPosIncRand.w = baseData.emitterIncrementXYZ.y;
            shader->FXSetCSFloat(paramSpawnPosIncrementXYZ, &vecSpawnPosInc, 1);
            shader->FXSetCSFloat(paramSpawnPosIncrementXYZRand, &vecSpawnPosIncRand, 1);

            // spawn pos increment random XYZ
            // TODO: Move emitter increment to the GPU. These spawnPosIncrementXYZRandom variables aren't used right now, but will be needed once calculations are moved to the GPU.
            static const CCryNameR paramSpawnPosIncrementXYZRandom("ed_spawnPosIncrementXYZRandom");
            static const CCryNameR paramSpawnPosIncrementXYZRandomRandRange("ed_spawnPosIncrementXYZRandomRandRange");
            Vec4 vecSpawnPosIncrementXYZRandom = Vec4( particleParams->vSpawnPosIncrementXYZRandom.fX.GetValue(emitData.strength, 1.f),
                                        particleParams->vSpawnPosIncrementXYZRandom.fY.GetValue(emitData.strength, 1.f),
                                        particleParams->vSpawnPosIncrementXYZRandom.fZ.GetValue(emitData.strength, 1.f),
                                        0);
            vecSpawnPosIncrementXYZRandom.w = baseData.emitterIncrementXYZ.z;
            Vec4 vecSpawnPosIncrementXYZRandomRand = vecSpawnPosIncrementXYZRandom *
                                                    Vec4(-particleParams->vSpawnPosXYZ.fX.GetRandomRange(),
                                                         -particleParams->vSpawnPosXYZ.fY.GetRandomRange(),
                                                         -particleParams->vSpawnPosXYZ.fZ.GetRandomRange(),
                                                         0.0f);
            shader->FXSetCSFloat(paramSpawnPosIncrementXYZRandom, &vecSpawnPosIncrementXYZRandom, 1);
            shader->FXSetCSFloat(paramSpawnPosIncrementXYZRandomRandRange, &vecSpawnPosIncrementXYZRandomRand, 1);

            // emitter size X
            static const CCryNameR paramEmitterSizeX("ed_emitterSizeX");
            float emitterSizeX = particleParams->fEmitterSizeDiameter.GetValue(emitData.strength, 1.f);
            Vec4 vecParamEmitterSizeX = Vec4(   emitterSizeX,
                                                emitterSizeX * -particleParams->fEmitterSizeDiameter.GetRandomRange(),
                                                0,
                                                0);
            shader->FXSetCSFloat(paramEmitterSizeX, &vecParamEmitterSizeX, 1);

        }

        // spawn pos XYZ
        static const CCryNameR paramSpawnPosXYZ("ed_spawnPosXYZ");
        static const CCryNameR paramSpawnPosXYZRandRange("ed_spawnPosXYZRandRange");
        Vec4 vecSpawnPosXYZ = Vec4( particleParams->vSpawnPosXYZ.fX.GetValue(emitData.strength, 1.f),
                                    particleParams->vSpawnPosXYZ.fY.GetValue(emitData.strength, 1.f),
                                    particleParams->vSpawnPosXYZ.fZ.GetValue(emitData.strength, 1.f),
                                    0.0f);
        Vec4 vecSpawnPosXYZRand = vecSpawnPosXYZ *
                                Vec4(   -particleParams->vSpawnPosXYZ.fX.GetRandomRange(),
                                        -particleParams->vSpawnPosXYZ.fY.GetRandomRange(),
                                        -particleParams->vSpawnPosXYZ.fZ.GetRandomRange(),
                                        0.0f);
        shader->FXSetCSFloat(paramSpawnPosXYZ, &vecSpawnPosXYZ, 1);
        shader->FXSetCSFloat(paramSpawnPosXYZRandRange, &vecSpawnPosXYZRand, 1);

        // spawn pos XYZ Random
        static const CCryNameR paramSpawnPosXYZRandom("ed_spawnPosXYZRandom");
        static const CCryNameR paramSpawnPosXYZRandomRandRange("ed_spawnPosXYZRandomRandRange");
        Vec4 vecSpawnPosXYZRandom =
            Vec4(particleParams->vSpawnPosXYZRandom.fX.GetValue(emitData.strength, 1.f),
                particleParams->vSpawnPosXYZRandom.fY.GetValue(emitData.strength, 1.f),
                particleParams->vSpawnPosXYZRandom.fZ.GetValue(emitData.strength, 1.f),
                0.0f);
        Vec4 vecSpawnPosXYZRandomRand = vecSpawnPosXYZRandom *
            Vec4(-particleParams->vSpawnPosXYZRandom.fX.GetRandomRange(),
                -particleParams->vSpawnPosXYZRandom.fY.GetRandomRange(),
                -particleParams->vSpawnPosXYZRandom.fZ.GetRandomRange(),
                0.0f);
        shader->FXSetCSFloat(paramSpawnPosXYZRandom, &vecSpawnPosXYZRandom, 1);
        shader->FXSetCSFloat(paramSpawnPosXYZRandomRandRange, &vecSpawnPosXYZRandomRand, 1);
    }
}

static bool BindCurveData(CD3D9Renderer* renderer, const GPUEmitterResources& emitter, EHWShaderClass shaderType)
{
    return BindTextureByID(renderer, emitter.texSampledCurves, s_GPUParticles_CurveTextureSlot, CTexture::GetTexState(STexState(FILTER_LINEAR, true)), 10, shaderType);
}

static bool UnBindCurveData(CD3D9Renderer* renderer, EHWShaderClass shaderType)
{
    return BindTextureByID(renderer, 0, s_GPUParticles_CurveTextureSlot, CTexture::GetTexState(STexState(FILTER_LINEAR, true)), 10, shaderType);
}

static float ComputeMaxParticleCount(const ResourceParticleParams* particleParams, const SpawnParams& spawnParams)
{
    return MIN(particleParams->fCount(VMAX) * spawnParams.fCountScale, PARTICLE_PARAMS_MAX_COUNT_GPU);
}

static float ComputePulsePeriod(const ResourceParticleParams* particleParams, const SpawnParams& spawnParams, bool allowRandom)
{
    if (spawnParams.fPulsePeriod > 0)
    {
        return spawnParams.fPulsePeriod;
    }
    else if(allowRandom)
    {
        return particleParams->fPulsePeriod(VRANDOM);
    }
    else
    {
        return particleParams->fPulsePeriod;
    }
}

static void UpdateEmitterLifetimeParams(GPUSubemitterData& emitter, const ResourceParticleParams* particleParams, const SpawnParams& spawnParams, float time)
{
    // time buffer is a time offset to make sure that floating point inaccuracies don't give any problem with
    // particle/emitter lifetimes. 0.5 is a reasonable buffer time wouldn't keep things alive for too long.
    const float timeBuffer = 0.5;

    emitter.repeatAge = fHUGE;
    if (particleParams->fPulsePeriod(VMAX) > 0.f || spawnParams.fPulsePeriod > 0.f)
    {
        float repeat = ComputePulsePeriod(particleParams, spawnParams, true);
        repeat = max(repeat, 0.1f);

        emitter.repeatAge = time + repeat;
    }
    emitter.startAge = emitter.stopAge = time;
    emitter.deathAge = emitter.stopAge + particleParams->fParticleLifeTime(VMAX) + timeBuffer;

    if (particleParams->bContinuous || !particleParams->fParticleLifeTime)
    {
        if (particleParams->fEmitterLifeTime)
        {
            emitter.stopAge += particleParams->fEmitterLifeTime(VRANDOM);
            emitter.deathAge = emitter.stopAge + particleParams->fParticleLifeTime(VMAX) + timeBuffer;
        }
        else
        {
            emitter.stopAge = fHUGE;
            emitter.deathAge = fHUGE;
        }
    }
}

static float ComputeEmitterStrength(GPUSubemitterData& emitter, const ResourceParticleParams* particleParams, float time, const SpawnParams& spawnParams)
{
    float strength = spawnParams.fStrength;
    if (strength < 0.f)
    {
        if (particleParams->bContinuous)
        {
            const float stopAge = MIN(emitter.stopAge, emitter.repeatAge);
            return div_min(time - emitter.startAge, stopAge - emitter.startAge, 1.f);
        }
        else
        {
            return 0.f;
        }
    }
    else
    {
        return min(strength, 1.f);
    }
}

static bool UpdateSubemitter(GPUSubemitterData& emitData, const ResourceParticleParams* resourceParameters, const SpawnParams& spawnParams, float time, float deltaTime, const ParticleTarget* base_target)
{
    if (time > emitData.repeatAge)
    {
        UpdateEmitterLifetimeParams(emitData, resourceParameters, spawnParams, time);
        emitData.chaosKey = CChaosKey(cry_random_uint32());
    }
    emitData.strength = ComputeEmitterStrength(emitData, resourceParameters, time, spawnParams);

    emitData.target = *base_target;
    auto targetting = resourceParameters->TargetAttraction.eTarget;
    if (!emitData.target.bPriority && targetting == targetting.OwnEmitter)
    {
        emitData.target.bTarget = true;
        emitData.target.vTarget = emitData.transform.GetTranslation();
        emitData.target.vVelocity = Vec3(0);
        emitData.target.fRadius = 0.f;
    }

    if (time <= emitData.stopAge)
    {
        // update spawn rates
        if (resourceParameters->bContinuous)
        {
            // spawnRate is calculated by dividing the current particle count (set in the editor) by the emitter or particle lifetime
            // this yields in a rate at which the particle count is the maximum particle count in the scene, similar to CPU particles.
            
            float spawnRate = 0.0f;
            const float particleLifetime = resourceParameters->fParticleLifeTime.GetMaxValue();
            const float particleCount = emitData.paramCurrentCount;
            const float emitterLifetime = resourceParameters->fEmitterLifeTime;

            // Continuous emission rate which maintains fCount particles, while checking for possible Div by Zero.
            if (emitterLifetime <= 0.f)
            {
                spawnRate = particleCount / particleLifetime;
            }
            else
            {
                spawnRate = max(particleCount / particleLifetime, particleCount / emitterLifetime);
            }

            // If the GPU took too long during last emitter render, we will surely take just as long next time.
            // Repeating emissions under this condition further stalls the GPU (and will eventually hang), thus we reduce emitting, but continue to update.
            // In order to render with better performance, we allow the emitter to gradually resume emitting at original rate when rendering performance threshold is not exceeded.
            const float renderTimeThresholdInMilliseconds = 100.0f; // <-Arbitrary value selected based on particle effect look. 
            
            emitData.particleSpawnPerformanceScale = (emitData.renderTimeInMilliseconds >= renderTimeThresholdInMilliseconds)
                ? emitData.particleSpawnPerformanceScale - deltaTime  // Scale down emissions (Automatically caps at value 0).
                : emitData.particleSpawnPerformanceScale + deltaTime; // Restore the emission scale over-time. (Automatically caps at value 1.0f)

            // Update the particle spawn accumulation amount, but ultimately scale it by the 'particleSpawnPerformanceScale'.
            // This way, we scale the emission rate according to how much load the GPU can take before hanging.
            // The scale ranges from [0-1], and changes based on how long the GPU proessed during last emitter render.
            const float incrementAccumulation = spawnRate * deltaTime * emitData.particleSpawnPerformanceScale;
            static const ICVar* particlesMaxEmitCount = gEnv->pConsole->GetCVar("r_ParticlesGpuMaxEmitCount");
            emitData.accumParticleSpawn = AZ::GetMin<float>(emitData.accumParticleSpawn + incrementAccumulation, particlesMaxEmitCount->GetFVal());
        }
        else     // if the spawn rate is not continuous
        {
            emitData.accumParticlePulse = 1.f;
        }
    }

    if (time > emitData.deathAge && emitData.repeatAge == fHUGE)
    {
        return false;
    }

    return true;
}

void CD3DGPUParticleEngine::OnCubeDepthMapResolutionChanged(ICVar*)
{
    if (gpuParticleEngine_impl == nullptr)
    {
        return;
    }

    for (auto emit : gpuParticleEngine_impl->emitters)
    {
        if (emit->resources.depthCubemap)
        {
            SAFE_RELEASE(emit->resources.depthCubemap);
        }
        gpuParticleEngine_impl->OnEffectChanged(emit->resources, emit->baseData, emit->resourceParameters, *emit->baseData.emitterFlags);
    }
}

static void ReleaseBuffers(GPUEmitterResources& resources)
{
    resources.bufEmitterState.Release();
    resources.bufParticleData.Release();
    resources.bufParticleDeadlist.Release();
    resources.bufParticleDistance.Release();
    resources.bufParticleIndices.Release();
    resources.bufDeadParticleLocations.Release();
    resources.bufDeadParticleLocationsStaging.Release();
}

static void SetupDeathLocationsBuffer(GPUEmitterResources& resources, const ResourceParticleParams* particleParams, const SpawnParams& spawnParams, int numSubemitters)
{
    float spawnRate = ComputeMaxParticleCount(particleParams, spawnParams) / particleParams->fParticleLifeTime(VMIN);
    //assume 60FPS. We would like to over estimate the spanwRate since it is a heuristic.
    spawnRate *= 1.f / 60.f;

    // We scale the spawn rate to get an overestimate result so that we could get a better render result.
    resources.numDeathLocations = static_cast<int>(std::ceil(spawnRate * 1.5f)) * numSubemitters;
    // Allocate space for death locations, which contians 4 float. Also the staging buffer needs space for all death locations + extra 4 floats in order to preserve
    // alignment to float4 boundaries.
    resources.bufDeadParticleLocations.Create(resources.numDeathLocations, sizeof(float) * 4, DXGI_FORMAT_UNKNOWN, DX11BUF_STRUCTURED | DX11BUF_BIND_UAV, nullptr);
    resources.bufDeadParticleLocationsStaging.Create(resources.numDeathLocations * 4 + 4, sizeof(float), DXGI_FORMAT_UNKNOWN, DX11BUF_STAGING, nullptr);
}

static void AddChild(SGPUParticleEmitterData* parent, SGPUParticleEmitterData* child)
{
    child->parent = parent;
    if (parent)
    {
        parent->children.push_back(child);
        parent->baseData.isIndirectParent = true;

        //allocate the buffer for the dead particle locations if we have children
        if (!parent->resources.bufDeadParticleLocations.m_pBuffer)
        {
            SetupDeathLocationsBuffer(parent->resources, parent->resourceParameters, parent->spawnParams, parent->numMaxSubemitters);
        }
    }
}

static void InitializeSubemitter(GPUSubemitterData& emitter, int offset)
{
    emitter.bufferOffset = offset;
    emitter.chaosKey = CChaosKey(cry_random_uint32());
}

static int SendWindDataToGPU(GPUEmitterResources& resources, const SPhysEnviron* physEnv, int& bufferSize)
{
    const auto& windAreas = physEnv->GetNonUniformAreas();

    if (windAreas.size() > 0)
    {
        char* dataBuf = new char[windAreas.size() * sizeof(SGPUWindArea)];

        SGPUWindArea* gpuData = reinterpret_cast<SGPUWindArea*>(dataBuf);
        int j = 0; //index into gpu data

        for (int i = 0; i < windAreas.size(); ++i)
        {
            if ((windAreas[i].m_nFlags & ENV_WIND) == 0)
            {
                continue;
            }

            //copy data
            gpuData[j].wind_direction = windAreas[i].m_Forces.vWind;
            gpuData[j].is_radial = static_cast<int>(windAreas[i].m_bRadial);
            gpuData[j].position = windAreas[i].m_vCenter;
            gpuData[j].falloffScale = windAreas[i].m_fFalloffScale;
            gpuData[j].aabb_min = windAreas[i].m_bbArea.min;
            gpuData[j].aabb_max = windAreas[i].m_bbArea.max;
            gpuData[j].shape = windAreas[i].m_nGeomShape == GEOM_BOX ? SGPUWindArea::SHAPE_BOX : SGPUWindArea::SHAPE_SPHERE;
            gpuData[j].matToLocal = Matrix34(windAreas[i].m_matToLocal);

            j++;
        }

        if (j > bufferSize)
        {
            bufferSize = j;
            resources.bufWindAreas.Release();
            resources.bufWindAreas.Create(bufferSize, sizeof(SGPUWindArea), DXGI_FORMAT_UNKNOWN, DX11BUF_STRUCTURED | DX11BUF_BIND_SRV | DX11BUF_DYNAMIC, nullptr);
        }

        resources.bufWindAreas.UpdateBufferContent(dataBuf, j * sizeof(SGPUWindArea));

        delete[] dataBuf;

        return j;
    }

    return 0;
}



// *** CImpl_GPUParticles Implementation ------------------------------------------------------------------------------------

void CImpl_GPUParticles::InitializeEmitter(SGPUParticleEmitterData* pEmitter)
{
    CRY_ASSERT(pEmitter);
    CRY_ASSERT(pEmitter->resourceParameters);

    auto * m_impl = this;


    ReleaseBuffers(pEmitter->resources);
    pEmitter->subEmitters.clear();

    // set counts
    if (pEmitter->baseData.stagedMaxParticlesInBuffer == 0)
    {
        UpdateEmitterCounts(pEmitter);
    }

    pEmitter->baseData.numParticlesPerChunk = pEmitter->baseData.stagedMaxParticlesInBuffer;

    if (pEmitter->baseData.isIndirect)
    {
        pEmitter->numMaxSubemitters = D3DGPUParticleEngine_MAX_SUBEMITTER_NUM;
        for (int off = pEmitter->numMaxSubemitters - 1; off >= 0; off--)
        {
            pEmitter->availableOffsets.push(off);
        }
    }
    else
    {
        pEmitter->numMaxSubemitters = D3DGPUParticleEngine_MIN_SUBEMITTER_NUM;
    }

    pEmitter->baseData.windAreaBufferSize = 0;
    //determine number of non-uniform areas in the physEnviron that have wind
    for (auto area : pEmitter->baseData.physEnv->GetNonUniformAreas())
    {
        if (area->m_nFlags & ENV_WIND)
        {
            pEmitter->baseData.windAreaBufferSize++;
        }
    }
    pEmitter->baseData.windAreaBufferSize = (pEmitter->resourceParameters->fAirResistance != 0) ?
        pEmitter->baseData.windAreaBufferSize : 0;

    InitializeBuffers(pEmitter->resources, pEmitter->baseData.stagedMaxParticlesInBuffer * pEmitter->numMaxSubemitters, pEmitter->resourceParameters, pEmitter->spawnParams, pEmitter->baseData.isIndirectParent, pEmitter->numMaxSubemitters,
        pEmitter->baseData.windAreaBufferSize);

    // set initial parameters
    if (!pEmitter->baseData.isIndirect)
    {
        AddSubemitter(pEmitter, 0);
    }

    // sort initialize values
    oddEvenDValue = 0;
    oddEvenLValue = 0;
    oddEvenPValue = 0;
    oddEvenTotalPasses = 0;
    oddEvenConvergedPasses = 0;


    OnEffectChanged(pEmitter->resources, pEmitter->baseData, pEmitter->resourceParameters, *pEmitter->baseData.emitterFlags);
}

void CImpl_GPUParticles::InitializeBuffers(GPUEmitterResources& resources, int numParticlesRequested, const ResourceParticleParams* particleParams, const SpawnParams& spawnParams, bool isIndirectParent, int numSubemitters, int numWindAreas)
{
    resources.numMaxParticlesInBuffer = numParticlesRequested;

    // prepare initial cpu buffer with a sequential index array that is going to be shuffled by the sorting algorithm(s)
    int* cpuSortIndices = new int[2 * (int)resources.numMaxParticlesInBuffer];
    for (int i = 0; i < (int)resources.numMaxParticlesInBuffer; i++)
    {
        // we only initialize the first value of a buffer that is a key value pair for the depth sort on the gpu
        // index x = draw index
        // index x + 1 = distance value (gets initialized on the gpu)
        cpuSortIndices[i * 2] = i;
    }

    // create buffers
    resources.bufEmitterState.Create(1 + numSubemitters, sizeof(int), DXGI_FORMAT_UNKNOWN, DX11BUF_STRUCTURED | DX11BUF_BIND_UAV | DX11BUF_BIND_SRV, nullptr);
    resources.bufParticleData.Create(resources.numMaxParticlesInBuffer, sizeof(SGPUParticleData), DXGI_FORMAT_UNKNOWN, DX11BUF_STRUCTURED | DX11BUF_BIND_UAV | DX11BUF_BIND_SRV, nullptr);
    resources.bufParticleDeadlist.Create(resources.numMaxParticlesInBuffer, sizeof(int), DXGI_FORMAT_UNKNOWN, DX11BUF_STRUCTURED | DX11BUF_BIND_UAV | DX11BUF_BIND_SRV, nullptr);

    resources.bufParticleDistance.Create(resources.numMaxParticlesInBuffer, sizeof(float), DXGI_FORMAT_UNKNOWN, DX11BUF_STRUCTURED | DX11BUF_BIND_UAV | DX11BUF_BIND_SRV, nullptr);

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION D3DGPUPARTICLEENGINE_CPP_SECTION_4
#include AZ_RESTRICTED_FILE(D3DGPUParticleEngine_cpp, AZ_RESTRICTED_PLATFORM)
#endif

    resources.bufParticleIndices.Create(resources.numMaxParticlesInBuffer, sizeof(int) * 2, DXGI_FORMAT_UNKNOWN, DX11BUF_STRUCTURED | DX11BUF_BIND_UAV | DX11BUF_BIND_SRV, cpuSortIndices);
    if (numWindAreas > 0)
    {
        resources.bufWindAreas.Create(numWindAreas, sizeof(SGPUWindArea), DXGI_FORMAT_UNKNOWN, DX11BUF_STRUCTURED | DX11BUF_BIND_SRV | DX11BUF_DYNAMIC, nullptr);
    }

    if (isIndirectParent)
    {
        SetupDeathLocationsBuffer(resources, particleParams, spawnParams, numSubemitters);
    }

    // cleanup temporary CPU buffer
    delete[] cpuSortIndices;
}

void CImpl_GPUParticles::GrowBuffers(SGPUParticleEmitterData& emitter)
{
    assert(emitter.baseData.isIndirect);

    WrappedDX11Buffer oldParticleData;
    oldParticleData = emitter.resources.bufParticleData;
    emitter.resources.bufParticleData.m_pBuffer = nullptr;
    for (uint32_t i = 0; i < WrappedDX11Buffer::MAX_VIEW_COUNT; ++i)
    {
        emitter.resources.bufParticleData.m_pSRV[i] = nullptr;
        emitter.resources.bufParticleData.m_pUAV[i] = nullptr;
    }

    ReleaseBuffers(emitter.resources);

    //compute a likely required size based on the emission properties of the parent, as well as directly growing and then use whatever is larger -> less likely to cause issues
    int estimatedRequiredSpace = static_cast<int>((ComputeMaxParticleCount(emitter.parent->resourceParameters, emitter.parent->spawnParams) / emitter.parent->resourceParameters->fParticleLifeTime.GetMinValue()) * (emitter.resourceParameters->fEmitterLifeTime.GetMaxValue()
        + emitter.resourceParameters->fParticleLifeTime.GetMaxValue()));
    int newMaxSubemitters = std::max(estimatedRequiredSpace, static_cast<int>(std::ceil(D3DGPUParticleEngine_GROUP_FACTORY * emitter.numMaxSubemitters)));

    InitializeBuffers(emitter.resources, emitter.baseData.numParticlesPerChunk * newMaxSubemitters, emitter.resourceParameters, emitter.spawnParams, emitter.baseData.isIndirectParent, newMaxSubemitters, emitter.baseData.windAreaBufferSize);

    gcpRendD3D->GetDeviceContext().CopySubresourceRegion(emitter.resources.bufParticleData.m_pBuffer, 0, 0, 0, 0, oldParticleData.m_pBuffer, 0, nullptr);

    oldParticleData.Release();

    for (int off = newMaxSubemitters - 1; off >= emitter.numMaxSubemitters; --off)
    {
        emitter.availableOffsets.push(off);
    }

    emitter.numMaxSubemitters = newMaxSubemitters;
}

float CImpl_GPUParticles::RenderEmitter(const RenderPassData& renderPassData, const GPUEmitterBaseData& baseData, const GPUEmitterResources& resources, const GPUSubemitterData& emitData, const ResourceParticleParams* particleParams, const SpawnParams& spawnParams)
{
    CRY_ASSERT(particleParams);

    CImpl_GPUParticles* m_impl = this;

    const char* drawGPUParticles = "DRAW_GPU_PARTICLES";
    const char* drawHalfResGPUParticles = "HALF_RES_DRAW_GPU_PARTICLES";

    // when 1/2 res is enabled for GPU particles, render in the 1/2 res render target
    int iTempX, iTempY, iWidth, iHeight;
    const uint32 nOldForceStateAnd = m_impl->dxRenderer->m_RP.m_ForceStateAnd;
    const uint32 nOldForceStateOr = m_impl->dxRenderer->m_RP.m_ForceStateOr;
    CTexture* pHalfResTarget = CTexture::s_ptexHDRTargetScaled[m_impl->dxRenderer->CV_r_ParticlesHalfResAmount];
    assert(CTexture::IsTextureExist(pHalfResTarget));
    if (CTexture::IsTextureExist(pHalfResTarget) && particleParams->bHalfRes)
    {
        const int nHalfWidth = pHalfResTarget->GetWidth();
        const int nHalfHeight = pHalfResTarget->GetHeight();

        // Get current viewport
        m_impl->dxRenderer->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

        if (particleParams->eBlendType == ParticleParams::EBlend::AlphaBased)
        {
            m_impl->dxRenderer->FX_PushRenderTarget(0, pHalfResTarget, NULL);
        }
        else
        {
            m_impl->dxRenderer->FX_PushRenderTarget(0, pHalfResTarget, m_impl->dxRenderer->FX_GetDepthSurface(pHalfResTarget->GetWidth(), pHalfResTarget->GetHeight(), false));
        }

        m_impl->dxRenderer->RT_SetViewport(0, 0, nHalfWidth, nHalfHeight);
        ColorF clClear(0, 0, 0, 0);
        m_impl->dxRenderer->EF_ClearTargetsLater(FRT_CLEAR_COLOR | FRT_CLEAR_IMMEDIATE | FRT_CLEAR_DEPTH, clClear);

        m_impl->dxRenderer->m_RP.m_PersFlags2 |= RBPF2_HALFRES_PARTICLES;
        if (particleParams->eBlendType == ParticleParams::EBlend::AlphaBased)
        {
            m_impl->dxRenderer->m_RP.m_ForceStateOr = GS_NODEPTHTEST;
            m_impl->dxRenderer->m_RP.m_ForceStateAnd = GS_BLSRC_SRCALPHA;
            m_impl->dxRenderer->m_RP.m_ForceStateOr |= GS_BLSRC_SRCALPHA_A_ZERO;
        }
        else
        {
            m_impl->dxRenderer->m_RP.m_ForceStateOr &= (~GS_NODEPTHTEST);
        }
    }

    PROFILE_LABEL_SCOPE(particleParams->bHalfRes ? drawHalfResGPUParticles : drawGPUParticles);

    CShader* shader = static_cast<CShader*>(m_impl->shrGpuRender.m_pShader);

    const bool isShadowPass = (renderPassData.pass == EGPUParticlePass::Shadow);

    const char* technique = isShadowPass ? s_GPUParticles_Material_RenderShadowTechniqueName : s_GPUParticles_Material_RenderTechniqueName;

    // check for shader validity
    CRY_ASSERT(shader);
    CRY_ASSERT(shader->mfFindTechnique(technique));
    CRY_ASSERT(shader->mfFindTechnique(technique)->m_Passes.Num() > 0);
    uint32 nPasses = 0;

    if (isShadowPass == false)
    {
        // modify render state of particle draw shader primary technique
        uint32& renderState = shader->mfFindTechnique(technique)->m_Passes[0].m_RenderState;
        renderState &= ~(GS_BLSRC_MASK | GS_BLDST_MASK | GS_DEPTHWRITE);

        switch (particleParams->eBlendType)
        {
        case ParticleParams::EBlend::AlphaBased:
        {
            //color = src.rgba * 1.0 + dst.rgba * (1.0-src.a)
            renderState |= GS_BLSRC_ONE;
            renderState |= GS_BLDST_ONEMINUSSRCALPHA;
        }
        break;
        case ParticleParams::EBlend::Additive:
            // color = src.rgba * 1.0 + dst.rgba * 1.0              = src.rgba + dst.rgba;
            renderState |= GS_BLSRC_ONE;
            renderState |= GS_BLDST_ONE;
            break;
        case ParticleParams::EBlend::Multiplicative:
            // color = src.rgba * 0.0 + dst.rgba * src.rgba         = dst.rgba * src.rgba;
            renderState |= GS_BLSRC_ZERO;
            renderState |= GS_BLDST_SRCCOL;
            break;
        case ParticleParams::EBlend::Opaque:
            // color = src.rgba * 1.0 + dst.rgba * 0.0              = src.rgba;
            renderState |= GS_BLSRC_ONE;
            renderState |= GS_BLDST_ZERO;
            renderState |= GS_DEPTHWRITE; // enable depth write for opaque
            break;
        }
    }

    // set shader flags
    uint64 originalShaderRT = m_impl->dxRenderer->m_RP.m_FlagsShader_RT;
    if (isShadowPass  && !baseData.isAuxWindowUpdate)
    {
        m_impl->dxRenderer->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_GPU_PARTICLE_SHADOW_PASS];
    }
    if ((particleParams->bSoftParticle || particleParams->bHalfRes) && !baseData.isAuxWindowUpdate)
    {
        m_impl->dxRenderer->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SOFT_PARTICLE];
    }
    if (static_cast<SRenderingPassInfo::EShadowMapType>(renderPassData.shadowMode) == SRenderingPassInfo::EShadowMapType::SHADOW_MAP_LOCAL  && !baseData.isAuxWindowUpdate)
    {
        m_impl->dxRenderer->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_CUBEMAP0];
    }
    if (particleParams->TextureTiling.nTilesX > 1 ||
        particleParams->TextureTiling.nTilesY > 1)
    {
        m_impl->dxRenderer->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_GPU_PARTICLE_UV_ANIMATION];

        if (particleParams->TextureTiling.bAnimBlend)
        {
            m_impl->dxRenderer->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_ANIM_BLEND];
        }
    }
    if (particleParams->nNormalTexId)
    {
        m_impl->dxRenderer->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_GPU_PARTICLE_NORMAL_MAP];
    }
    if (particleParams->nGlowTexId)
    {
        m_impl->dxRenderer->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_GPU_PARTICLE_GLOW_MAP];
    }
    if (particleParams->bMotionBlur)
    {
        m_impl->dxRenderer->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_MOTION_BLUR];
    }
    if (particleParams->bReceiveShadows && isShadowPass == false)
    {
        shader->m_eShaderType = eST_Particle;
        m_impl->dxRenderer->FX_SetupShadowsForTransp();
    }

    SetEmitterShapeFlag(particleParams->GetEmitterShape());

    // start fx pass
    shader->FXSetTechnique(technique);
    shader->FXBegin(&nPasses, 0);
    shader->FXBeginPass(0);

    // set constants
    static const CCryNameR paramProjection("Projection");
    static const CCryNameR paramModelview("Modelview");
    static const CCryNameR paramCameraXAxis("CameraXAxis");
    static const CCryNameR paramCameraYAxis("CameraYAxis");
    static const CCryNameR paramCameraLookDir("CameraLookDir");
    static const CCryNameR paramEmitterStretchOffsetAndFlipChanceY("ed_stretch_pivotOffset_flipChanceY");
    static const CCryNameR paramUVAnimation("ed_tilesXY_firstTile_variantCount");
    static const CCryNameR paramUVAnimation2("ed_numFrames_frameRate_blend_flipChanceX");
    static const CCryNameR paramSoftParticles("ed_softParticlesAndBackLighting");
    static const CCryNameR paramEmissiveAndFadeCoefficients("ed_Emissive_FadeCoefficients");
    static const CCryNameR paramFadeOffsetStrengthsThreshhold("ed_DistOffset_FadeStrengths_FadeThreshold");
    static const CCryNameR paramCamPos("CameraPosition");
    static const CCryNameR paramLODAlphaPixelDiscardMotionBlurStrength("ed_lodAlpha_PixelDiscard_motionBlurStrength");
    static const CCryNameR paramAspectRatio("ed_aspectRatio");
    static const CCryNameR paramBufferOffset("BufferOffset");
    static const CCryNameR paramEmitterBounds("ed_emitterBounds");
    static const CCryNameR paramEmitterConfineBounds("ed_emitterConfineBounds");
    static const CCryNameR paramEmitterPosition("EmitterPosition");
    static const CCryNameR paramEmitterAxis("EmitterAxis");
    static const CCryNameR paramEmitterRight("EmitterRight");
    static const CCryNameR spawnColorTint("Spawn_ColorTint");

    // holds the offset for the pivot over the velocity axis, cst stands for constant
    Vec4 cstParticleStretchPivotOffsetAndFlipChanceY(particleParams->fStretch.fOffsetRatio, particleParams->TextureTiling.fVerticalFlipChance, 0.0f, 0.0f);

    // store blend bool and cycle enum together
    ParticleParams::STextureTiling::EAnimationCycle animCycle = particleParams->TextureTiling.eAnimCycle;
    uint bitsOffsetFromBlendBoolFlag = 1;
    uint animCycleEnumAndBlendFlags = (1 << (bitsOffsetFromBlendBoolFlag + static_cast<unsigned int>(animCycle))) | static_cast<unsigned int>(particleParams->TextureTiling.bAnimBlend);

    // animCycleEnumAndBlendFlags is casted using reinterpret_cast<float&> (basically asfloat()) so that the bits of the value stay the same while transporting the value to the gpu; we can then read it asuint() on the gpu to retrieve the flags
    // Unary+ is used to avoid having to describe TSmall<> to numeric_limits.
    Vec4 uvAnimationParams2(
        aznumeric_caster(+particleParams->TextureTiling.nAnimFramesCount),
        particleParams->TextureTiling.fAnimFramerate,
        reinterpret_cast<float&>(animCycleEnumAndBlendFlags),
        particleParams->TextureTiling.fHorizontalFlipChance);

    float particleSoftness = powf(32.0f, -particleParams->bSoftParticle.fSoftness + 1.0f);

    // x = boolean if soft particle is enabled, y = softness, z unused,
    Vec4 softParticleAndBackLightingParams(particleParams->bSoftParticle || particleParams->bHalfRes,
        particleSoftness,
        0, //unused
        particleParams->fDiffuseBacklighting);

    bool hdrEnabled = false;
    renderer->EF_Query(EFQ_HDRModeEnabled, hdrEnabled);
    Vec4 emissivePropsParams;
    // Grab the following values from CPU particle engine;
    emissivePropsParams.x = particleParams->fEmissiveLighting; 
    emissivePropsParams.y = 1.f;
    emissivePropsParams.z = 0.f;
    emissivePropsParams.w = 0.f;

    Vec4 vecAspectRatio;
    vecAspectRatio.x = renderPassData.aspectRatio;

    Vec4 LODAlphaParamsPixelDiscardMotionBlurStrength;
    LODAlphaParamsPixelDiscardMotionBlurStrength.x = baseData.lodBlendAlpha;
    LODAlphaParamsPixelDiscardMotionBlurStrength.y = particleParams->nParticleSizeDiscard;
    LODAlphaParamsPixelDiscardMotionBlurStrength.z = particleParams->bMotionBlur.fBlurStrength;

    Vec4 fadeOffsetStrengthThreshholdParams;
    fadeOffsetStrengthThreshholdParams.x = particleParams->fCameraDistanceOffset;
    fadeOffsetStrengthThreshholdParams.y = particleParams->fCameraFadeNearStrength;
    fadeOffsetStrengthThreshholdParams.z = particleParams->fCameraFadeFarStrength;
    fadeOffsetStrengthThreshholdParams.w = 0.f;

    if (particleParams->GetEmitterShape() == ParticleParams::EEmitterShapeType::BOX)
    {
        Vec3 confinedAxis = Vec3(particleParams->ConfinedParticle());
        Vec3 freeAxis = (Vec3(1.0f, 1.0f, 1.0f) - confinedAxis) * FLT_MAX;

        Vec4 vecEmitterConfineBounds = Vec4((particleParams->vEmitterSizeXYZ.GetVector(emitData.strength).CompMul(confinedAxis) + freeAxis) * 0.5f, 0);

        shader->FXSetVSFloat(paramEmitterConfineBounds, &vecEmitterConfineBounds, 1);

        Vec4 vecEmitterBounds = Vec4(particleParams->vEmitterSizeXYZ.GetVector(1) * 0.5f, 0);
        shader->FXSetVSFloat(paramEmitterBounds, &vecEmitterBounds, 1);
    }

    Vec4 vecEmitterPos = Vec4(emitData.transform.GetTranslation(), 0);

    //compute coefficients for distance fade equation
    //"zoom" value used for consistency with CPU version
    float fZoom = 1.f / renderPassData.fov;
    if (particleParams->fCameraMaxDistance > EPSILON)
    {
        float farVal = sqr(particleParams->fCameraMaxDistance * fZoom);
        float nearVal = sqr(particleParams->fCameraMinDistance * fZoom);
        float border = (farVal - nearVal) * 0.1f;       //Grab the border formular from cpu version

        fadeOffsetStrengthThreshholdParams.w = (farVal + nearVal) * 0.5f; //Get the average of the far and near ends.

        //if there is not supposed to be near fade, set the near fade strength to 0 -> this will disable near fade
        if (particleParams->fCameraMinDistance < EPSILON)
        {
            fadeOffsetStrengthThreshholdParams.y = 0.f;
        }

        //This quotient is what results from the required conditions that
        // f(F) = 0
        // f(F - B) = 1
        // f(N + B) = 1
        // f(N) = 0
        //where f is our fade function and is a quadratic function over the square distance
        //see also the CPU implementation of the camera distance fade for reference
        float quotient = border * (border - farVal + nearVal);
        emissivePropsParams.y = (farVal * nearVal) / quotient;
        emissivePropsParams.z = (-farVal - nearVal) / quotient;
        emissivePropsParams.w = 1.f / quotient;
    }
    else if (particleParams->fCameraMinDistance > EPSILON)
    {
        //here, we have only a near fade, not a far one
        //instead of picking the fade border relative to the distance between near and far, just set it at 1/4th
        //of the near distance (compare CPU implementation)
        float border = particleParams->fCameraMinDistance * fZoom * 0.25f;

        //set the threshold arbitrarily higher than the fade border
        fadeOffsetStrengthThreshholdParams.w = border * 6.f;
        // This value is get from quadratic equation. Also grab the value from CPU Particle Engine.
        emissivePropsParams.y = -4.f;
        emissivePropsParams.z = 1.f / border;
        emissivePropsParams.w = 0.f;
    }

    // The unary + is used to move the TSmall<> type through integer promotion so it can then be safely converted into
    // the Vec4 as there are no entries in numeric_limits for TSmall and the caster function fails to compile otherwise.
    Vec4 uvAnimationParams(
        aznumeric_caster(+particleParams->TextureTiling.nTilesX),
        aznumeric_caster(+particleParams->TextureTiling.nTilesY),
        aznumeric_caster(particleParams->TextureTiling.nFirstTile),
        aznumeric_caster(+particleParams->TextureTiling.nVariantCount));

    const Vec4 bufferOffsetParams(reinterpret_cast<const float&>(emitData.bufferOffset), 0, 0, 0);

    const Vec4 vecEmitterAxis = Vec4(emitData.transform.GetColumn2(), 0);
    const Vec4 vecEmitterRight = Vec4(emitData.transform.GetColumn0(), 0);
    const Vec4 vecSpawnColorTint = Vec4(spawnParams.colorTint, 1);

    // bind for vertex shader
    shader->FXSetVSFloat(paramEmitterAxis, &vecEmitterAxis, 1);
    shader->FXSetVSFloat(paramEmitterRight, &vecEmitterRight, 1);
    shader->FXSetVSFloat(paramModelview, reinterpret_cast<const Vec4*>(renderPassData.modelview.GetData()), 4);
    shader->FXSetVSFloat(paramEmitterStretchOffsetAndFlipChanceY, &cstParticleStretchPivotOffsetAndFlipChanceY, 1);
    shader->FXSetVSFloat(paramUVAnimation, &uvAnimationParams, 1);
    shader->FXSetVSFloat(paramUVAnimation2, &uvAnimationParams2, 1);
    shader->FXSetVSFloat(paramSoftParticles, &softParticleAndBackLightingParams, 1);
    shader->FXSetVSFloat(paramBufferOffset, &bufferOffsetParams, 1);
    shader->FXSetVSFloat(paramEmitterPosition, &vecEmitterPos, 1);
    shader->FXSetVSFloat(spawnColorTint, &vecSpawnColorTint, 1);

    shader->FXSetVSFloat(paramProjection, reinterpret_cast<const Vec4*>(renderPassData.projection.GetData()), 4);
    shader->FXSetVSFloat(paramModelview, reinterpret_cast<const Vec4*>(renderPassData.modelview.GetData()), 4);
    shader->FXSetVSFloat(paramAspectRatio, &vecAspectRatio, 1);
    shader->FXSetVSFloat(paramCameraXAxis, &renderPassData.cameraRightAxis, 1);
    shader->FXSetVSFloat(paramCameraYAxis, &renderPassData.cameraUpAxis, 1);
    shader->FXSetVSFloat(paramCameraLookDir, &renderPassData.cameraLookDir, 1);
    shader->FXSetVSFloat(paramUVAnimation, &uvAnimationParams, 1);
    shader->FXSetVSFloat(paramUVAnimation2, &uvAnimationParams2, 1);
    shader->FXSetVSFloat(paramSoftParticles, &softParticleAndBackLightingParams, 1);
    shader->FXSetVSFloat(paramCamPos, &renderPassData.cameraPos, 1);
    shader->FXSetVSFloat(paramFadeOffsetStrengthsThreshhold, &fadeOffsetStrengthThreshholdParams, 1);
    shader->FXSetVSFloat(paramEmissiveAndFadeCoefficients, &emissivePropsParams, 1);
    shader->FXSetVSFloat(paramLODAlphaPixelDiscardMotionBlurStrength, &LODAlphaParamsPixelDiscardMotionBlurStrength, 1);

    // bind for pixel shader
    shader->FXSetPSFloat(paramUVAnimation2, &uvAnimationParams2, 1);
    shader->FXSetPSFloat(paramSoftParticles, &softParticleAndBackLightingParams, 1);
    shader->FXSetPSFloat(paramEmissiveAndFadeCoefficients, &emissivePropsParams, 1);
    shader->FXSetPSFloat(paramLODAlphaPixelDiscardMotionBlurStrength, &LODAlphaParamsPixelDiscardMotionBlurStrength, 1);

    // apply fx state

    // Because of how shader state can be shared between different rendering elements the GPU particles
    // can get material state for a different render element, like the terrain. When the material constant
    // buffers are applied with terrain information it can cause rendering artifacts after GPU particles
    // are rendered, such as a flickering screen. For now disable material updates for the GPU particles
    // since they don't use materials until more time can be spent to figure out why the terrain material
    // constants cause such issues.
    uint64 materialBit = dxRenderer->m_RP.m_nCommitFlags & FC_MATERIAL_PARAMS;
    dxRenderer->m_RP.m_nCommitFlags &= ~FC_MATERIAL_PARAMS;

    m_impl->dxRenderer->FX_Commit(false);

    dxRenderer->m_RP.m_nCommitFlags |= materialBit;

    // bind SRVs (textures/structuredBuffers)
    D3DShaderResourceView* pSRV[] = { resources.bufParticleData.GetShaderResourceView(), resources.bufParticleIndices.GetShaderResourceView() };
    m_impl->dxRenderer->m_DevMan.BindSRV(eHWSC_Vertex, pSRV, 6, sizeof(pSRV) / sizeof(pSRV[0]));
    if (!BindCurveData(m_impl->dxRenderer, resources, eHWSC_Vertex) || 
        !BindCurveData(m_impl->dxRenderer, resources, eHWSC_Pixel))
        return 0;

    // bind textures
    if (renderPassData.isWireframeEnabled)
    {
        // diffuse texture is set to black to cancel out alpha and make wireframe easy to see
        BindTextureByID(m_impl->dxRenderer, gRenDev->GetBlackTextureId(), 0);
    }
    else
    {
        BindTextureByID(m_impl->dxRenderer, particleParams->nTexId, 0);   // diffuse texture
    }
    
    if (particleParams->nNormalTexId)
    {
        BindTextureByID(m_impl->dxRenderer, particleParams->nNormalTexId, 3);
    }
    if (particleParams->nGlowTexId)
    {
        BindTextureByID(m_impl->dxRenderer, particleParams->nGlowTexId, 4);
    }

    if (!baseData.isAuxWindowUpdate)
    {
        uint32 depthTextureBindLocation = 5;
        STexState texStatePoint = STexState(FILTER_POINT, true);
        CTexture::s_ptexZTarget->Apply(depthTextureBindLocation, CTexture::GetTexState(texStatePoint), EFTT_UNKNOWN, -1, SResourceView::DefaultView, EHWShaderClass::eHWSC_Pixel);
    }

    // draw (internally actuates GPU state changes)
    gpuTimer->Start("Emit Throttle Timer");
    GPU_PARTICLE_PROFILER_START(const_cast<GPUEmitterResources*>(&resources), CGPUParticleProfiler::RENDER);
    m_impl->dxRenderer->FX_DrawPrimitive(eRenderPrimitiveType::eptTriangleStrip, 0, baseData.numParticlesPerChunk, 4);
    GPU_PARTICLE_PROFILER_END(const_cast<GPUEmitterResources*>(&resources), CGPUParticleProfiler::RENDER);
    gpuTimer->Stop();

    // unbind SRVs
    ID3D11ShaderResourceView* pSRVNull[10] = { NULL, NULL };
    m_impl->dxRenderer->m_DevMan.BindSRV(eHWSC_Vertex, pSRVNull, 0, sizeof(pSRVNull) / sizeof(pSRVNull[0]));

    if (!baseData.isAuxWindowUpdate)
    {
        CTexture::s_ptexZTarget->Unbind();
    }

    // actuate GPU state changes (to unbind)
    m_impl->dxRenderer->m_DevMan.CommitDeviceStates();

    // end fx pass
    shader->FXEndPass();
    shader->FXEnd();

    // restore render flags
    m_impl->dxRenderer->m_RP.m_FlagsShader_RT = originalShaderRT;

    // when 1/2 res is enabled for GPU particles, directly composite these particles since they are alpha blended
    if (CTexture::IsTextureExist(pHalfResTarget) && particleParams->bHalfRes)
    {
        m_impl->dxRenderer->m_RP.m_ForceStateAnd = nOldForceStateAnd;
        m_impl->dxRenderer->m_RP.m_ForceStateOr = nOldForceStateOr;
        m_impl->dxRenderer->m_RP.m_PersFlags2 &= ~RBPF2_HALFRES_PARTICLES;

        m_impl->dxRenderer->FX_PopRenderTarget(0);

        {
            CShader* pSH = CShaderMan::s_shPostEffects;
            CTexture* pHalfResSrc = pHalfResTarget;
            CTexture* pZTarget = CTexture::s_ptexZTarget;
            CTexture* pZTargetScaled = m_impl->dxRenderer->CV_r_ParticlesHalfResAmount > 0 ? CTexture::s_ptexZTargetScaled2 : CTexture::s_ptexZTargetScaled;

            uint32 nStates = GS_NODEPTHTEST | GS_COLMASK_RGB;
            if (particleParams->eBlendType == ParticleParams::EBlend::AlphaBased)
            {
                nStates |= GS_BLSRC_ONE | GS_BLDST_ONEMINUSSRCALPHA;
            }
            else
            {
                nStates |= GS_BLSRC_ONE | GS_BLDST_ONE;
            }

            m_impl->dxRenderer->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);
            static const CCryNameTSCRC pTechNameNearestDepth("NearestDepthUpsample");
            PostProcessUtils().ShBeginPass(CShaderMan::s_shPostEffects, pTechNameNearestDepth, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

            static const CCryNameR pParam0Name("texToTexParams0");
            Vec4 vParam0(static_cast<float>(pZTarget->GetWidth()), static_cast<float>(pZTarget->GetHeight()), static_cast<float>(pZTargetScaled->GetWidth()), static_cast<float>(pZTargetScaled->GetHeight()));
            CShaderMan::s_shPostEffects->FXSetPSFloat(pParam0Name, &vParam0, 1);

            PostProcessUtils().SetTexture(pHalfResSrc, 1, FILTER_LINEAR);
            PostProcessUtils().SetTexture(pZTarget, 2, FILTER_POINT);
            PostProcessUtils().SetTexture(pZTargetScaled, 3, FILTER_POINT);

            m_impl->dxRenderer->FX_SetState(nStates);
            PostProcessUtils().DrawFullScreenTri(m_impl->dxRenderer->GetWidth(), m_impl->dxRenderer->GetHeight());

            PostProcessUtils().ShEndPass();
        }
    }

    gpuTimer->UpdateTime();
    return gpuTimer->GetTime();
}

void CImpl_GPUParticles::EmitParticles(SGPUParticleEmitterData* emitter)
{
    CRY_ASSERT(emitter);
    CRY_ASSERT(emitter->resourceParameters);

    for (GPUSubemitterData& emitData : emitter->subEmitters)
    {
        bool spawnedParticles = false;
        if (emitter->resourceParameters->bContinuous == true && emitData.accumParticleSpawn >= 1.0f)
        {
            // if continuous, and the accumulator determined that we should spawn one (or more) particles, do so.
            EmitParticlesGPU(emitter->baseData, emitter->resources, emitData, emitter->resourceParameters, emitter->spawnParams, (int)(emitData.accumParticleSpawn));
            emitData.accumParticleSpawn -= (int)(emitData.accumParticleSpawn);
            spawnedParticles = true;
        }
        else if (emitter->resourceParameters->bContinuous == false && emitData.accumParticlePulse >= 1.0f)
        {
            // if not continuous, and the pulse accumulator determined that we should (re)spawn all particles again in a single burst
                        
            const float particleCount = ComputeMaxParticleCount(emitter->resourceParameters, emitter->spawnParams);
            EmitParticlesGPU(emitter->baseData, emitter->resources, emitData, emitter->resourceParameters, emitter->spawnParams, (int)(particleCount));
            emitData.accumParticlePulse -= (int)(emitData.accumParticlePulse);
            spawnedParticles = true;
        }

        if (spawnedParticles)
        {
            // retrieve new count/pulse period value for the next spawn iteration.
            UpdateParameterCount(emitData, emitter->resourceParameters, emitter->spawnParams);
            UpdateParameterPulsePeriod(emitData, emitter->resourceParameters, emitter->spawnParams);
        }
    }
}

void CImpl_GPUParticles::EmitParticlesGPU(GPUEmitterBaseData& baseData, const GPUEmitterResources& resources, const GPUSubemitterData& emitData, const ResourceParticleParams* particleParams, const SpawnParams& spawnParams, int numSpawnParticles)
{
    PROFILE_LABEL_SCOPE("EMIT_GPU_PARTICLES");


    // * spawn particles:   spawn a number of particles, limited to "numSpawnParticles". only will spawn desired amount if there are "dead" particles available,
    //                      determined by the "Update" step before.
    if (numSpawnParticles <= 0)
    {
        return;
    }

    //Particles are invisible at this point, so we can stop emitting them.
    if (baseData.lodBlendAlpha < EPSILON)
    {
        return;
    }

    CShader* shader = shrGpuEmit;

    CRY_ASSERT(shader);


    uint64 originalShaderRT = dxRenderer->m_RP.m_FlagsShader_RT;

    if (particleParams->bMotionBlur)
    {
        dxRenderer->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_MOTION_BLUR];
    }

    SetEmitterShapeFlag(particleParams->GetEmitterShape());

    uint32 nPasses = 0;

    // start fx pass
    shader->FXSetTechnique(s_GPUParticles_Material_TechniqueName);
    shader->FXBegin(&nPasses, 0);
    shader->FXBeginPass(0);

    // bind emitter data
    BindEmitterDataToCS(emitData, baseData, particleParams, spawnParams, shader, dxRenderer);

    // set spawn data
    static const CCryNameR paramSpawnInfo("SpawnInfo");
    Vec4 vecSpawnInfo(aznumeric_caster(numSpawnParticles), 0.0f, 0.0f, 0.0f);
    shader->FXSetCSFloat(paramSpawnInfo, &vecSpawnInfo, 1);

    static const CCryNameR paramCamPos("CameraPosition");
    shader->FXSetCSFloat(paramCamPos, &latestCameraPos, 1);

    // apply fx state
    dxRenderer->FX_Commit(false);

    // bind UAVs (rwStructuredBuffers) & SRVs (textures/structuredBuffers)
    ID3D11ShaderResourceView* pSRV[] = { resources.bufParticleDeadlist.GetShaderResourceView() };
    ID3D11UnorderedAccessView* pUAV[] = { nullptr, resources.bufParticleData.GetUnorderedAccessView(), resources.bufParticleDistance.GetUnorderedAccessView() };
    uint32 pUAVCounters[] = { 0, 0, 0 }; // initial UAV counter values
    dxRenderer->m_DevMan.BindUAV(eHWSC_Compute, pUAV, pUAVCounters, 0, sizeof(pUAV) / sizeof(pUAV[0]));
    dxRenderer->m_DevMan.BindSRV(eHWSC_Compute, pSRV, 7, sizeof(pSRV) / sizeof(pSRV[0]));

    // Emitter increment update
    // TODO: Move emitter increment to the GPU. Doing it on the CPU for a whole group of spawned particles makes it impossible
    //       to make the random increment feature behave consistently between equivalent CPU and GPU particle systems. We are going to 
    //       have to do this later because of an upcoming deadline and the need to mitigate risk, as this will require improving the Emitter 
    //       shader to keep a buffer of memory to store the accumulator.
    {
        // Since the GPU may spawn multiple particles for a single CPU update of emitterIncrementXYZ, we include the previous
        // emitterIncrementXYZ. Then the GPU can interpolate between them to reduce the appearance of particle position aliasing.
        baseData.emitterIncrementXYZprev = baseData.emitterIncrementXYZ;

        // We need to use a larger random range when spawning multiple particles because the idea is for *each* particle to
        // randomly increment the emitter position, but we are generating a new random number a whole *group* of particles.

        // We can't just multiply by numSpawnParticles because that would cause the accumulated increment to drift too quickly.
        // Using an attenuation factor of 0.15 seemed to work pretty well when compared to the behavior of an equivalent CPU particle system.
        // This isn't mathematically correct but seemed to produce convincing results.
        static const float rangeAttenuateFactor = 0.15f;
        float randomRange = 1.0f + static_cast<float>(numSpawnParticles - 1) * rangeAttenuateFactor;
        float groupRandom = cry_random(-1.0f, 1.0f) * randomRange;

        baseData.emitterIncrementXYZ.x += particleParams->vSpawnPosIncrementXYZRandom.fX(VRANDOM, emitData.strength) * groupRandom;
        baseData.emitterIncrementXYZ.y += particleParams->vSpawnPosIncrementXYZRandom.fY(VRANDOM, emitData.strength) * groupRandom;
        baseData.emitterIncrementXYZ.z += particleParams->vSpawnPosIncrementXYZRandom.fZ(VRANDOM, emitData.strength) * groupRandom;

        // Unlike the random calculation above, it's okay to just multiply numSpawnParticles here because the function is linear.
        baseData.emitterIncrementXYZ += static_cast<float>(numSpawnParticles) * particleParams->vSpawnPosIncrementXYZ.GetVector(emitData.strength);
    }


    // dispatch compute (internally actuates GPU state changes)
    GPU_PARTICLE_PROFILER_START(const_cast<GPUEmitterResources*>(&resources), CGPUParticleProfiler::EMIT);
    dxRenderer->m_DevMan.Dispatch(RoundUpAndDivide(numSpawnParticles, s_GPUParticles_ThreadGroupEmitX), 1, 1);
    GPU_PARTICLE_PROFILER_END(const_cast<GPUEmitterResources*>(&resources), CGPUParticleProfiler::EMIT);

    // unbind UAVs & SRVs
    ID3D11UnorderedAccessView* pUAVNull[10] = { NULL };
    ID3D11ShaderResourceView* pSRVNull[10] = { NULL };
    dxRenderer->m_DevMan.BindUAV(eHWSC_Compute, pUAVNull, NULL, 0, sizeof(pUAV) / sizeof(pUAV[0]));
    dxRenderer->m_DevMan.BindSRV(eHWSC_Compute, pSRVNull, 0, sizeof(pSRV) / sizeof(pSRV[0]));

    // actuate GPU state changes (to actually unbind)
    dxRenderer->m_DevMan.CommitDeviceStates();

    // end fx pass
    shader->FXEndPass();
    shader->FXEnd();

    // restore render flags
    dxRenderer->m_RP.m_FlagsShader_RT = originalShaderRT;
}

void CImpl_GPUParticles::UpdateEmitterCounts(SGPUParticleEmitterData* pEmitter)
{
    CRY_ASSERT(pEmitter);
    CRY_ASSERT(pEmitter->resourceParameters);

    // enforce that there is at least one particle, and no more than the absolute max
    const float particleCount = pEmitter->resourceParameters->fCount * pEmitter->spawnParams.fCountScale;
    const int maxParticles = aznumeric_caster(
            MIN(
                MAX(ceil(particleCount) + 1, 1), 
                PARTICLE_PARAMS_MAX_COUNT_GPU
            )
        );
    int maxParticlesRounded = maxParticles;

    const float pulsePeriod = ComputePulsePeriod(pEmitter->resourceParameters, pEmitter->spawnParams, false);

    if (pEmitter->resourceParameters->bContinuous == false &&
        pulsePeriod > 0.0f &&
        pulsePeriod < pEmitter->resourceParameters->fParticleLifeTime)
    {
        // only valid when pulse period < particle lifetime
        const float pulseOverParticleLifeTime = (pEmitter->resourceParameters->fParticleLifeTime / pulsePeriod);
        maxParticlesRounded = aznumeric_caster(maxParticlesRounded * ceil(pulseOverParticleLifeTime));
    }

    // if sorting is enabled, buffer particle count needs to be a power of two
    if (pEmitter->resourceParameters->eSortMethod != ParticleParams::ESortMethod::None && pEmitter->resourceParameters->eBlendType == ParticleParams::EBlend::AlphaBased)
    {
        maxParticlesRounded = max(2048u, NextPower2(maxParticlesRounded));
    }

    // update staged emitter counts (buffers are not resized yet, reinitializing the emitter will actuate the values)
    pEmitter->baseData.stagedMaxParticlesInBuffer = RoundUp(maxParticlesRounded, s_GPUParticles_ThreadGroupMaximalX);

    if (pEmitter->baseData.numLastMaxParticles != maxParticles)
    {
        for (GPUSubemitterData& emitData : pEmitter->subEmitters)
        {
            UpdateParameterCount(emitData, pEmitter->resourceParameters, pEmitter->spawnParams);
            UpdateParameterPulsePeriod(emitData, pEmitter->resourceParameters, pEmitter->spawnParams);
        }
    }

    pEmitter->baseData.numLastMaxParticles = maxParticles;
}

void CImpl_GPUParticles::UpdateParameterCount(GPUSubemitterData& emitData, const ResourceParticleParams* particleParams, const SpawnParams& spawnParams)
{
    emitData.paramCurrentCount =
        MIN(
            MAX(0.01f, particleParams->fCount(VRANDOM, emitData.strength) * spawnParams.fCountScale),
            PARTICLE_PARAMS_MAX_COUNT_GPU
        );
}

void CImpl_GPUParticles::UpdateParameterPulsePeriod(GPUSubemitterData& emitData, const ResourceParticleParams* particleParams, const SpawnParams& spawnParams)
{
    emitData.paramCurrentPulsePeriod = ComputePulsePeriod(particleParams, spawnParams, true);
}

void CImpl_GPUParticles::UpdateGPUCurveData(GPUEmitterResources& emitter, const ResourceParticleParams* particleParams)
{
    // generate curve data (w.i.p. - preferably only upload when curves are actually changed..)
    SGPUParticleCurve curves[s_GPUParticles_CurveNumSlots];
    CRY_ASSERT(particleParams);
    SampleCurveToBuffer(particleParams->fAirResistance.GetParticleAge(), curves[0]);
    SampleCurveToBuffer(particleParams->fGravityScale.GetParticleAge(), curves[1]);
    SampleCurveToBuffer(particleParams->fSizeX.GetParticleAge(), curves[2]);
    SampleCurveToBuffer(particleParams->fSizeY.GetParticleAge(), curves[3]);
    SampleCurveToBuffer(particleParams->fTurbulence3DSpeed.GetParticleAge(), curves[4]);
    SampleCurveToBuffer(particleParams->fTurbulenceSize.GetParticleAge(), curves[5]);
    SampleCurveToBuffer(particleParams->fTurbulenceSpeed.GetParticleAge(), curves[6]);
    SampleCurveToBuffer(particleParams->vRotationRateX.GetParticleAge(), curves[7]);
    SampleCurveToBuffer(particleParams->vRotationRateY.GetParticleAge(), curves[8]);
    SampleCurveToBuffer(particleParams->vRotationRateZ.GetParticleAge(), curves[9]);
    SampleCurveToBufferXYZ(particleParams->cColor.GetParticleAge(), &curves[10]);
    SampleCurveToBuffer(particleParams->fAlpha.GetParticleAge(), curves[13]);
    if (particleParams->cColor.GetRandomLerpAge())
    {
        SampleCurveToBufferXYZ(particleParams->cColor.GetSecondaryParticleAge(), &curves[14]);
        SampleCurveToBuffer(particleParams->fAlpha.GetSecondaryParticleAge(), curves[17]);
    }
    else
    {
        SampleCurveToBufferXYZ(particleParams->cColor.GetParticleAge(), &curves[14]);
        SampleCurveToBuffer(particleParams->fAlpha.GetParticleAge(), curves[17]);
    }
    SampleCurveToBuffer(particleParams->fPivotX.GetParticleAge(), curves[18]);
    SampleCurveToBuffer(particleParams->fPivotY.GetParticleAge(), curves[19]);
    SampleCurveToBuffer(particleParams->fStretch.GetParticleAge(), curves[20]);
    //Curve 21 for softness of soft particel is removed since we don't need softness during lifetime per-particle
    SampleCurveToBuffer(particleParams->TargetAttraction.fRadius.GetParticleAge(), curves[22]);
    SampleCurveToBuffer(particleParams->vVelocity.fX.GetParticleAge(), curves[23]);
    SampleCurveToBuffer(particleParams->vVelocity.fY.GetParticleAge(), curves[24]);
    SampleCurveToBuffer(particleParams->vVelocity.fZ.GetParticleAge(), curves[25]);

    // convert curves to texture data
    unsigned char curveTextureData[s_GPUParticles_CurveNumSamples * s_GPUParticles_CurveTextureHeight];
    for (int i = 0; i < s_GPUParticles_CurveNumSlots; i++)
    {
        ConvertCurveBufferToTextureData(&curves[i], curveTextureData + i * s_GPUParticles_CurveNumSamples);
    }

    // upload to GPU (if not exists create new texture, otherwise update existing texture)
    emitter.texSampledCurves = renderer->DownLoadToVideoMemory(curveTextureData, s_GPUParticles_CurveNumSamples, s_GPUParticles_CurveTextureHeight, eTF_R8, eTF_R8, 1, false, FILTER_BILINEAR, emitter.texSampledCurves);
}

void CImpl_GPUParticles::UpdateShaders()
{
    if (requireShaderReload)
    {
        LoadShaderItems();
    }

    if (shrGpuRender.m_nPreprocessFlags == -1)
    {
        shrGpuRender.Update();
    }
}

void CImpl_GPUParticles::BeginUpdate(GPUEmitterResources& resources, int numSubEmitters)
{
    PROFILE_LABEL_SCOPE("RESET_COUNTERS_GPU_PARTICLES");

#if D3DGPUPARTICLEENGINE_CPP_TRAIT_BEGINUPDATE_INIT_COMPUTE || defined(OPENGL)

    // * GPU Compute
    CImpl_GPUParticles* m_impl = this;
    CShader* shader = m_impl->shrGpuBegin;

    // check for shader validity
    CRY_ASSERT(shader);
    CRY_ASSERT(shader->mfFindTechnique(s_GPUParticles_Material_TechniqueName));
    CRY_ASSERT(shader->mfFindTechnique(s_GPUParticles_Material_TechniqueName)->m_Passes.Num() > 0);

    uint32 nPasses = 0;

    // start fx pass
    shader->FXSetTechnique(s_GPUParticles_Material_TechniqueName);
    shader->FXBegin(&nPasses, 0);
    shader->FXBeginPass(0);

    // we add 1 to the numSubEmitters to account for the dead particles
    int numSubEmittersAndDeathCount = numSubEmitters + 1;

    static const CCryNameR paramClearInfo("ClearInfo");
    Vec4 clearInfo = Vec4(reinterpret_cast<float&>(numSubEmittersAndDeathCount), 0, 0, 0);
    shader->FXSetCSFloat(paramClearInfo, &clearInfo, 1);

    // apply fx state
    m_impl->dxRenderer->FX_Commit(false);

    // bind UAVs (rwStructuredBuffers)
    ID3D11UnorderedAccessView* pUAV[] = { resources.bufEmitterState.GetUnorderedAccessView() };
    uint32 pUAVCounters[] = { 0 };     // initial UAV counter values
    m_impl->dxRenderer->m_DevMan.BindUAV(eHWSC_Compute, pUAV, pUAVCounters, 0, sizeof(pUAV) / sizeof(pUAV[0]));

    // dispatch compute (internally actuates GPU state changes)
    GPU_PARTICLE_PROFILER_START((&resources), CGPUParticleProfiler::BEGIN);
    m_impl->dxRenderer->m_DevMan.Dispatch(RoundUpAndDivide(numSubEmittersAndDeathCount, s_GPUParticles_ThreadGroupBeginX), 1, 1);
    GPU_PARTICLE_PROFILER_END((&resources), CGPUParticleProfiler::BEGIN);

    // unbind UAVs & SRVs
    ID3D11UnorderedAccessView* pUAVNull[10] = { NULL };
    m_impl->dxRenderer->m_DevMan.BindUAV(eHWSC_Compute, pUAVNull, NULL, 0, sizeof(pUAV) / sizeof(pUAV[0]));

    // actuate GPU state changes (to actually unbind)
    m_impl->dxRenderer->m_DevMan.CommitDeviceStates();

    // end fx pass
    shader->FXEndPass();
    shader->FXEnd();
#else
    unsigned clearVal[4] = { 0 };
    gcpRendD3D->GetDeviceContext().ClearUnorderedAccessViewUint(resources.bufEmitterState.GetUnorderedAccessView(), clearVal);
#endif
}

void CImpl_GPUParticles::UpdateGPU(GPUEmitterBaseData& baseData, GPUEmitterResources& resources, GPUSubemitterData& emitData, const ResourceParticleParams* particleParams, const SpawnParams& spawnParams)
{
    PROFILE_LABEL_SCOPE("UPDATE_GPU_PARTICLES");

    // * GPU Compute
    CImpl_GPUParticles* m_impl = this;
    CShader* shader = m_impl->shrGpuUpdate;

    // check for shader validity
    CRY_ASSERT(shader);
    CRY_ASSERT(shader->mfFindTechnique(s_GPUParticles_Material_UpdateTechniqueName));
    CRY_ASSERT(shader->mfFindTechnique(s_GPUParticles_Material_UpdateTechniqueName)->m_Passes.Num() > 0);

    uint64 originalShaderRT = m_impl->dxRenderer->m_RP.m_FlagsShader_RT;
    if ((particleParams->eDepthCollision == ParticleParams::EDepthCollision::FrameBuffer) && !baseData.isAuxWindowUpdate)
    {
        m_impl->dxRenderer->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_GPU_PARTICLE_DEPTH_COLLISION];
    }
    else if ((particleParams->eDepthCollision == ParticleParams::EDepthCollision::Cubemap) && !baseData.isAuxWindowUpdate)
    {
        m_impl->dxRenderer->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_GPU_PARTICLE_CUBEMAP_DEPTH_COLLISION];
    }
    if (particleParams->fTurbulenceSpeed != 0.0f ||
        particleParams->fTurbulence3DSpeed != 0.0f ||
        particleParams->fTurbulenceSize != 0.0f)
    {
        m_impl->dxRenderer->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_GPU_PARTICLE_TURBULENCE];
    }
    if (particleParams->bMotionBlur)
    {
        m_impl->dxRenderer->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_MOTION_BLUR];
    }
    if (baseData.isIndirectParent)
    {
        m_impl->dxRenderer->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_GPU_PARTICLE_WRITEBACK_DEATH_LOCATIONS];
    }
    if (baseData.target->bTarget)
    {
        m_impl->dxRenderer->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_GPU_PARTICLE_TARGET_ATTRACTION];
    }
    if (baseData.numWindAreas != 0 || particleParams->fAirResistance != 0.0f)
    {
        m_impl->dxRenderer->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_GPU_PARTICLE_WIND];
    }

    SetEmitterShapeFlag(particleParams->GetEmitterShape());

    if (particleParams->fAirResistance != 0)
    {
        baseData.numWindAreas = SendWindDataToGPU(resources, baseData.physEnv, baseData.windAreaBufferSize);
    }
    else
    {
        baseData.numWindAreas = 0;
    }

    uint32 nPasses = 0;
    // start fx pass
    shader->FXSetTechnique(s_GPUParticles_Material_UpdateTechniqueName);
    shader->FXBegin(&nPasses, 0);
    shader->FXBeginPass(0);

    static const CCryNameR paramProjectionMat("ProjectionMatrix");
    shader->FXSetCSFloat(paramProjectionMat, reinterpret_cast<const Vec4*>(latestProjection.GetData()), 4);

    static const CCryNameR paramViewMat("ViewMatrix");
    shader->FXSetCSFloat(paramViewMat, reinterpret_cast<const Vec4*>(latestModelview.GetData()), 4);

    static const CCryNameR paramCamPos("CameraPosition");
    shader->FXSetCSFloat(paramCamPos, &latestCameraPos, 1);

    static const CCryNameR paramEmitterPos("EmitterPosition");
    Vec4 emitterPos = Vec4(baseData.transform.GetTranslation(), 1);
    shader->FXSetCSFloat(paramEmitterPos, &emitterPos, 1);

    static const CCryNameR paramNearFarPlane("depthCubeNearFar");
    Vec4 nearFarPlanes = Vec4(nearPlane, particleParams->fCubemapFarDistance, 0, 0);
    shader->FXSetCSFloat(paramNearFarPlane, &nearFarPlanes, 1);

    static const CCryNameR paramDeadCountOffset("DeadCountOffset");
    int offset = emitData.bufferOffset / baseData.numParticlesPerChunk;
    Vec4 deadCountOffset = Vec4(reinterpret_cast<float&>(offset), 0, 0, 0);
    shader->FXSetCSFloat(paramDeadCountOffset, &deadCountOffset, 1);

    // bind data
    BindEmitterDataToCS(emitData, baseData, particleParams, spawnParams, shader, renderer);

    // update curve data
    UpdateGPUCurveData(resources, particleParams);

    static const CCryNameR paramNumWindAreas("ed_numWindAreas_windScale");
    Vec4 vecNumWindAreas(reinterpret_cast<float&>(baseData.numWindAreas), particleParams->fAirResistance.fWindScale, 0, 0);
    shader->FXSetCSFloat(paramNumWindAreas, &vecNumWindAreas, 1);

    Vec3 globalWind = gEnv->p3DEngine->GetGlobalWind(false);
    static const CCryNameR paramGlobalWind("ed_globalWind");
    Vec4 vecGlobalWind(globalWind, 0.0f);
    shader->FXSetCSFloat(paramGlobalWind, &vecGlobalWind, 1);

    // apply fx state
    m_impl->dxRenderer->FX_Commit(false);

    // bind UAVs (rwStructuredBuffers) & SRVs (textures/structuredBuffers/curve data)
    D3DUnorderedAccessView* pUAV[] = { resources.bufEmitterState.GetUnorderedAccessView(), resources.bufParticleData.GetUnorderedAccessView(), resources.bufParticleDeadlist.GetUnorderedAccessView(), resources.bufParticleDistance.GetUnorderedAccessView() };
    uint32 pUAVCounters[] = { 0, 0, 0, 0 }; // initial UAV counter values
    m_impl->dxRenderer->m_DevMan.BindUAV(eHWSC_Compute, pUAV, pUAVCounters, 0, sizeof(pUAV) / sizeof(pUAV[0]));
    if (!BindCurveData(m_impl->dxRenderer, resources, eHWSC_Compute))
        return;

    if (!baseData.isAuxWindowUpdate)
    {
        if (particleParams->eDepthCollision == ParticleParams::EDepthCollision::FrameBuffer)
        {
            uint32 depthTextureBindLocation = 5;
            STexState texStatePoint = STexState(FILTER_POINT, true);
            CTexture::s_ptexZTarget->Apply(depthTextureBindLocation, CTexture::GetTexState(texStatePoint), EFTT_UNKNOWN, -1, SResourceView::DefaultView, EHWShaderClass::eHWSC_Compute);

            uint32 normalTextureBindLocation = 6;
            CTexture::s_ptexSceneNormalsMap->Apply(normalTextureBindLocation, CTexture::GetTexState(texStatePoint), EFTT_UNKNOWN, -1, SResourceView::DefaultView, EHWShaderClass::eHWSC_Compute);
        }
        else if (particleParams->eDepthCollision == ParticleParams::EDepthCollision::Cubemap)
        {
            uint32 depthCubemapBindLocation = 8;
            STexState texStateLinear = STexState(FILTER_LINEAR, true);
            resources.depthCubemap->Apply(depthCubemapBindLocation, CTexture::GetTexState(texStateLinear), EFTT_UNKNOWN, -1, SResourceView::DefaultView, EHWShaderClass::eHWSC_Compute);
        }
    }
    if (baseData.isIndirectParent)
    {
        m_impl->dxRenderer->m_DevMan.BindUAV(eHWSC_Compute, resources.bufDeadParticleLocations.GetUnorderedAccessView(), 0, 4);
    }

    const uint32 windAreasBindLocation = 7;
    if (baseData.numWindAreas != 0)
    {
        m_impl->dxRenderer->m_DevMan.BindSRV(eHWSC_Compute, resources.bufWindAreas.GetShaderResourceView(), windAreasBindLocation);
    }

    // dispatch compute (internally actuates GPU state changes)
    GPU_PARTICLE_PROFILER_START((&resources), CGPUParticleProfiler::UPDATE);
    m_impl->dxRenderer->m_DevMan.Dispatch(RoundUpAndDivide(baseData.numParticlesPerChunk, s_GPUParticles_ThreadGroupUpdateX), 1, 1);
    GPU_PARTICLE_PROFILER_END((&resources), CGPUParticleProfiler::UPDATE);

    if (baseData.numWindAreas != 0)
    {
        m_impl->dxRenderer->m_DevMan.BindSRV(eHWSC_Compute, nullptr, windAreasBindLocation);
    }

    // unbind UAVs & SRVs
    ID3D11UnorderedAccessView* pUAVNull[10] = { NULL };
    m_impl->dxRenderer->m_DevMan.BindUAV(eHWSC_Compute, pUAVNull, NULL, 0, sizeof(pUAV) / sizeof(pUAV[0]));
    UnBindCurveData(m_impl->dxRenderer, EHWShaderClass::eHWSC_Compute);
    if (baseData.isIndirectParent)
    {
        m_impl->dxRenderer->m_DevMan.BindUAV(eHWSC_Compute, nullptr, 0, 4);
    }

    if (!baseData.isAuxWindowUpdate)
    {
        if (particleParams->eDepthCollision == ParticleParams::EDepthCollision::FrameBuffer)
        {
            CTexture::s_ptexSceneNormalsMap->Unbind();
            CTexture::s_ptexZTarget->Unbind();
        }
        else if (particleParams->eDepthCollision == ParticleParams::EDepthCollision::Cubemap)
        {
            resources.depthCubemap->Unbind();
        }
    }

    // actuate GPU state changes (to actually unbind)
    m_impl->dxRenderer->m_DevMan.CommitDeviceStates();

    // end fx pass
    shader->FXEndPass();
    shader->FXEnd();

    if (baseData.isIndirectParent)
    {
        D3D11_BOX to_copy;
        to_copy.left = 0;
        to_copy.right = sizeof(Vec4);
        to_copy.front = to_copy.top = 0;
        to_copy.back = to_copy.bottom = 1;
        gcpRendD3D->GetDeviceContext().CopySubresourceRegion(resources.bufDeadParticleLocationsStaging.m_pBuffer, 0, 0, 0, 0, resources.bufEmitterState.m_pBuffer, 0, &to_copy);
        gcpRendD3D->GetDeviceContext().CopySubresourceRegion(resources.bufDeadParticleLocationsStaging.m_pBuffer, 0, sizeof(Vec4), 0, 0, resources.bufDeadParticleLocations.m_pBuffer, 0, nullptr);
    }

    // restore render flags
    m_impl->dxRenderer->m_RP.m_FlagsShader_RT = originalShaderRT;
}

void CImpl_GPUParticles::EndUpdate(GPUEmitterBaseData& baseData)
{
    // * end update: handle post update (CPU specific)
     baseData.previousTransform = baseData.transform;
}

void CImpl_GPUParticles::SortParticlesOddEvenIncremental(GPUEmitterResources& resources, GPUEmitterBaseData& baseData, GPUSubemitterData& emitData, const ResourceParticleParams* particleParams)
{
    PROFILE_LABEL_SCOPE("SORT_ODD_EVEN_GPU_PARTICLES");

    // * begin update: reset state of GPU emitter counters
    CShader* shader = shrGpuOddEvenSort;
    uint32 nPasses = 0;

    CRY_ASSERT(shader);

    // start fx pass
    shader->FXSetTechnique(s_GPUParticles_Material_TechniqueName);
    shader->FXBegin(&nPasses, 0);
    shader->FXBeginPass(0);

    // bind UAVs (rwStructuredBuffers)
    ID3D11UnorderedAccessView* pUAV[] = { resources.bufParticleIndices.GetUnorderedAccessView() };
    uint32 pUAVCounters[] = { 0 }; // initial UAV counter values
    dxRenderer->m_DevMan.BindUAV(eHWSC_Compute, pUAV, pUAVCounters, 0, sizeof(pUAV) / sizeof(pUAV[0]));

    // calculate dispatch size & amount of particles to traverse
    uint32 maxParticles = baseData.numParticlesPerChunk;
    int32 threads_x = maxParticles / s_GPUParticles_ThreadGroupSortOddEvenX;

    // start a new pass when we are done with converging or when we have a new max particle count
    uint32 newDValue = IntegerLog2(maxParticles);
    if (oddEvenConvergedPasses == 0 || newDValue != oddEvenDValue)
    {
        oddEvenDValue = newDValue;
        oddEvenLValue = 1;
        oddEvenPValue = 1;
        uint32 halfD = (oddEvenDValue + 1) / 2;
        oddEvenTotalPasses = oddEvenDValue * halfD + halfD;
        oddEvenConvergedPasses = 0;
    }

    // update state for the number of passes
    uint32 maxPassesThisFrame = MAX(1, (uint32)((float)oddEvenTotalPasses * particleParams->fSortConvergancePerFrame));
    maxPassesThisFrame = oddEvenConvergedPasses + maxPassesThisFrame > oddEvenTotalPasses ? oddEvenTotalPasses - oddEvenConvergedPasses : maxPassesThisFrame;
    oddEvenConvergedPasses += maxPassesThisFrame;

    // run odd even sort iterations
    uint32 numPassesThisFrame = 0;
    GPU_PARTICLE_PROFILER_START((&resources), CGPUParticleProfiler::SORT_TOTAL);
    while (oddEvenLValue <= (int)oddEvenDValue)
    {
        // we only run maxPassesThisFrame
        if (numPassesThisFrame >= maxPassesThisFrame)
        {
            break;
        }

        while (oddEvenPValue <= oddEvenLValue)
        {
            // we only run maxPassesThisFrame
            if (numPassesThisFrame >= maxPassesThisFrame)
            {
                oddEvenLValue--;
                break;
            }

            // profiler uses last pass
            GPU_PARTICLE_PROFILER_START((&resources), CGPUParticleProfiler::SORT_PASS);

            // set all parameters
            static const CCryNameR paramParameters("Parameters");
            Vec4 vecParameters(
                aznumeric_caster(oddEvenLValue),
                aznumeric_caster(oddEvenPValue),
                aznumeric_caster(maxParticles),
                reinterpret_cast<float&>(emitData.bufferOffset));
            shader->FXSetCSFloat(paramParameters, &vecParameters, 1);

            // apply fx state
            dxRenderer->FX_Commit(false);

            // dispatch compute (internally actuates GPU state changes & applies constants)
            dxRenderer->m_DevMan.Dispatch(threads_x, 1, 1);
            GPU_PARTICLE_PROFILER_END((&resources), CGPUParticleProfiler::SORT_PASS);

            numPassesThisFrame++;
            oddEvenPValue++;
        }

        oddEvenPValue = 1;
        oddEvenLValue++;
    }
    GPU_PARTICLE_PROFILER_END((&resources), CGPUParticleProfiler::SORT_TOTAL);

    // when we have converged, restart sorting again from 0
    if (oddEvenConvergedPasses == oddEvenTotalPasses)
    {
        oddEvenConvergedPasses = 0;
    }

    // unbind UAVs
    ID3D11UnorderedAccessView* pUAVNull[10] = { NULL };
    dxRenderer->m_DevMan.BindUAV(eHWSC_Compute, pUAVNull, NULL, 0, sizeof(pUAV) / sizeof(pUAV[0]));

    // actuate GPU state changes (to actually unbind)
    dxRenderer->m_DevMan.CommitDeviceStates();

    // end fx pass
    shader->FXEndPass();
    shader->FXEnd();
}

void CImpl_GPUParticles::GatherSortScore(GPUEmitterResources& emitter)
{
    PROFILE_LABEL_SCOPE("GATHER_SORT_SCORE_GPU_PARTICLES");
    // * GPU Compute
    CImpl_GPUParticles* m_impl = this;
    CShader* shader = m_impl->shrGpuGatherSortDistance;

    // check for shader validity
    CRY_ASSERT(shader);
    CRY_ASSERT(shader->mfGetStartTechnique(0));
    CRY_ASSERT(shader->mfGetStartTechnique(0)->m_Passes.Num() > 0);

    uint32 nPasses = 0;

    // start fx pass
    shader->FXSetTechnique(s_GPUParticles_Material_TechniqueName);
    shader->FXBegin(&nPasses, 0);
    shader->FXBeginPass(0);

    // apply fx state
    m_impl->dxRenderer->FX_Commit(false);

    // bind UAVs (rwStructuredBuffers) & SRVs (textures/structuredBuffers/curve data)
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION D3DGPUPARTICLEENGINE_CPP_SECTION_5
#include AZ_RESTRICTED_FILE(D3DGPUParticleEngine_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    ID3D11UnorderedAccessView* pUAV[] = { emitter.bufParticleIndices.GetUnorderedAccessView() };
#endif
    uint32 pUAVCounters[] = { 0 }; // initial UAV counter values
    m_impl->dxRenderer->m_DevMan.BindUAV(eHWSC_Compute, pUAV, pUAVCounters, 0, sizeof(pUAV) / sizeof(pUAV[0]));

    ID3D11ShaderResourceView* pSRV[] = { emitter.bufParticleDistance.GetShaderResourceView() };
    renderer->m_DevMan.BindSRV(eHWSC_Compute, pSRV, 7, sizeof(pSRV) / sizeof(pSRV[0]));

    // dispatch compute (internally actuates GPU state changes)
    GPU_PARTICLE_PROFILER_START((&emitter), CGPUParticleProfiler::GATHER_SORT);
    m_impl->dxRenderer->m_DevMan.Dispatch(RoundUpAndDivide(emitter.numMaxParticlesInBuffer, s_GPUParticles_ThreadGroupSortGatherX), 1, 1);
    GPU_PARTICLE_PROFILER_END((&emitter), CGPUParticleProfiler::GATHER_SORT);

    // unbind UAVs & SRVs
    ID3D11UnorderedAccessView* pUAVNull[10] = { NULL };
    m_impl->dxRenderer->m_DevMan.BindUAV(eHWSC_Compute, pUAVNull, NULL, 0, sizeof(pUAV) / sizeof(pUAV[0]));

    ID3D11ShaderResourceView* pSRVNull[10] = { NULL };
    m_impl->dxRenderer->m_DevMan.BindSRV(eHWSC_Compute, pSRVNull, 0, sizeof(pSRVNull) / sizeof(pSRVNull[0]));

    // actuate GPU state changes (to actually unbind)
    m_impl->dxRenderer->m_DevMan.CommitDeviceStates();

    // end fx pass
    shader->FXEndPass();
    shader->FXEnd();
}

void CImpl_GPUParticles::SortParticlesBitonicLocal(GPUEmitterResources& emitter, GPUEmitterBaseData& baseData, GPUSubemitterData& emitData)
{
    PROFILE_LABEL_SCOPE("SORT_BITONIC_LOCAL_GPU_PARTICLES");
    CRY_ASSERT(&emitter);

    // * begin update: reset state of GPU emitter counters
    CShader* shader = shrGpuBitonicSortLocal;
    uint32 nPasses = 0;

    CRY_ASSERT(shader);

    // start fx pass
    shader->FXSetTechnique("Default");
    shader->FXBegin(&nPasses, 0);
    shader->FXBeginPass(0);

    // bind UAVs (rwStructuredBuffers)
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION D3DGPUPARTICLEENGINE_CPP_SECTION_6
#include AZ_RESTRICTED_FILE(D3DGPUParticleEngine_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    ID3D11UnorderedAccessView* pUAV[] = { emitter.bufParticleIndices.GetUnorderedAccessView() };
#endif
    uint32 pUAVCounters[] = { 0 }; // initial UAV counter values
    dxRenderer->m_DevMan.BindUAV(eHWSC_Compute, pUAV, pUAVCounters, 0, sizeof(pUAV) / sizeof(pUAV[0]));

    // calculate buffer threshold
    uint32 maxParticles = baseData.numParticlesPerChunk;
    int32 threads_x = ((maxParticles / s_GPUParticles_ThreadGroupSortBitonicXLocal) + 1) / 2;    // bitonic sort only needs half of the threads
                                                                                                 // (each thread reads a pair of two unique indices and swaps them if needed
    static const CCryNameR paramParameters("Parameters");
    Vec4 vecParameters(
        0,
        0,
        reinterpret_cast<float&>(maxParticles),
        reinterpret_cast<float&>(emitData.bufferOffset));
    shader->FXSetCSFloat(paramParameters, &vecParameters, 1);

    // run bitonic sort iterations
    GPU_PARTICLE_PROFILER_START((&emitter), CGPUParticleProfiler::SORT_TOTAL);

    // apply fx state
    dxRenderer->FX_Commit(false);

    // dispatch compute (internally actuates GPU state changes & applies constants)
    dxRenderer->m_DevMan.Dispatch(threads_x, 1, 1);

    GPU_PARTICLE_PROFILER_END((&emitter), CGPUParticleProfiler::SORT_TOTAL);

    // unbind UAVs
    ID3D11UnorderedAccessView* pUAVNull[10] = { NULL };
    dxRenderer->m_DevMan.BindUAV(eHWSC_Compute, pUAVNull, NULL, 0, sizeof(pUAV) / sizeof(pUAV[0]));

    // actuate GPU state changes (to actually unbind)
    dxRenderer->m_DevMan.CommitDeviceStates();

    // end fx pass
    shader->FXEndPass();
    shader->FXEnd();
}

void CImpl_GPUParticles::SortParticlesBitonic(GPUEmitterResources& emitter, GPUEmitterBaseData& baseData, GPUSubemitterData& emitData)
{
    PROFILE_LABEL_SCOPE("SORT_BITONIC_GPU_PARTICLES");
    CRY_ASSERT(&emitter);

    // * begin update: reset state of GPU emitter counters
    CShader* globalShader = shrGpuBitonicSort;
    CShader* localShader = shrGpuBitonicSortGlobal2048;

    CRY_ASSERT(globalShader);
    CRY_ASSERT(localShader);

    uint32 maxParticles = baseData.numParticlesPerChunk;

    // run bitonic sort iterations
    GPU_PARTICLE_PROFILER_START((&emitter), CGPUParticleProfiler::SORT_TOTAL);
    int k, j;
    for (k = 4096; k <= maxParticles; k = 2 * k)
    {
        // start fx pass
        uint32 nPasses = 0;
        globalShader->FXSetTechnique("Default");
        globalShader->FXBegin(&nPasses, 0);
        globalShader->FXBeginPass(0);

        // bind UAVs (rwStructuredBuffers)
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION D3DGPUPARTICLEENGINE_CPP_SECTION_7
#include AZ_RESTRICTED_FILE(D3DGPUParticleEngine_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
        ID3D11UnorderedAccessView* pUAV[] = { emitter.bufParticleIndices.GetUnorderedAccessView() };
#endif
        uint32 pUAVCounters[] = { 0 }; // initial UAV counter values
        dxRenderer->m_DevMan.BindUAV(eHWSC_Compute, pUAV, pUAVCounters, 0, sizeof(pUAV) / sizeof(pUAV[0]));

        for (j = k >> 1; j >= 2048; j = j >> 1)
        {
            // profiler uses last pass
            GPU_PARTICLE_PROFILER_START((&emitter), CGPUParticleProfiler::SORT_PASS);

            // set all parameters
            static const CCryNameR paramParameters("Parameters");
            Vec4 vecParameters(
                aznumeric_caster(j),
                aznumeric_caster(k),
                aznumeric_caster(maxParticles),
                reinterpret_cast<float&>(emitData.bufferOffset));
            globalShader->FXSetCSFloat(paramParameters, &vecParameters, 1);

            // apply fx state
            dxRenderer->FX_Commit(false);

            // dispatch compute (internally actuates GPU state changes & applies constants)

            // calculate buffer threshold
            int32 threads_x = ((maxParticles / s_GPUParticles_ThreadGroupSortBitonicX) + 1) / 2;    // bitonic sort only needs half of the threads
                                                                                                    // (each thread reads a pair of two unique indices and swaps them if needed)
            dxRenderer->m_DevMan.Dispatch(threads_x, 1, 1);
            GPU_PARTICLE_PROFILER_END((&emitter), CGPUParticleProfiler::SORT_PASS);
        }

        // unbind UAVs
        ID3D11UnorderedAccessView* pUAVNull[10] = { NULL };
        dxRenderer->m_DevMan.BindUAV(eHWSC_Compute, pUAVNull, NULL, 0, sizeof(pUAV) / sizeof(pUAV[0]));

        // actuate GPU state changes (to actually unbind)
        dxRenderer->m_DevMan.CommitDeviceStates();

        // end fx pass
        globalShader->FXEndPass();
        globalShader->FXEnd();

        if (j < 2048)
        {
            // start fx pass
            uint32 passes = 0;
            localShader->FXSetTechnique("Default");
            localShader->FXBegin(&passes, 0);
            localShader->FXBeginPass(0);

            // bind UAVs (rwStructuredBuffers)
            dxRenderer->m_DevMan.BindUAV(eHWSC_Compute, pUAV, pUAVCounters, 0, sizeof(pUAV) / sizeof(pUAV[0]));

            // profiler uses last pass
            GPU_PARTICLE_PROFILER_START((&emitter), CGPUParticleProfiler::SORT_PASS);
            // set all parameters
            static const CCryNameR paramParameters("Parameters");
            Vec4 vecParameters(
                aznumeric_caster(j),
                aznumeric_caster(k),
                aznumeric_caster(maxParticles),
                reinterpret_cast<float&>(emitData.bufferOffset));
            localShader->FXSetCSFloat(paramParameters, &vecParameters, 1);

            // apply fx state
            dxRenderer->FX_Commit(false);

            // calculate buffer threshold
            int32 threads_x = ((maxParticles / s_GPUParticles_ThreadGroupSortBitonicXLocal) + 1) / 2;    // bitonic sort only needs half of the threads
                                                                                                         // (each thread reads a pair of two unique indices and swaps them if needed)
            // dispatch compute (internally actuates GPU state changes & applies constants)
            dxRenderer->m_DevMan.Dispatch(threads_x, 1, 1);

            // unbind UAVs
            dxRenderer->m_DevMan.BindUAV(eHWSC_Compute, pUAVNull, NULL, 0, sizeof(pUAV) / sizeof(pUAV[0]));

            // actuate GPU state changes (to actually unbind)
            dxRenderer->m_DevMan.CommitDeviceStates();

            // end fx pass
            localShader->FXEndPass();
            localShader->FXEnd();

            GPU_PARTICLE_PROFILER_END((&emitter), CGPUParticleProfiler::SORT_PASS);
        }
    }
    GPU_PARTICLE_PROFILER_END((&emitter), CGPUParticleProfiler::SORT_TOTAL);
}

void CImpl_GPUParticles::LoadShaderItems()
{
    CShaderMan& shaderManager = renderer->m_cEF;
    shrGpuBegin = shaderManager.mfForName(s_GPUParticles_Material_GPUBegin, EF_RELOAD);
    shrGpuUpdate = shaderManager.mfForName(s_GPUParticles_Material_GPUUpdate, EF_RELOAD);
    shrGpuEmit = shaderManager.mfForName(s_GPUParticles_Material_GPUEmit, EF_RELOAD);
    shrGpuBitonicSort = shaderManager.mfForName(s_GPUParticles_Material_GPUBitonicSort, EF_RELOAD);
    shrGpuBitonicSortGlobal2048 = shaderManager.mfForName(s_GPUParticles_Material_GPUBitonicSortGlobal2048, EF_RELOAD);
    shrGpuBitonicSortLocal = shaderManager.mfForName(s_GPUParticles_Material_GPUBitonicSortLocal, EF_RELOAD);
    shrGpuOddEvenSort = shaderManager.mfForName(s_GPUParticles_Material_GPUOddEvenSort, EF_RELOAD);
    shrGpuGatherSortDistance = shaderManager.mfForName(s_GPUParticles_Material_GPUGatherSortDistance, EF_RELOAD);

    SInputShaderResources commonShaderResources;
    commonShaderResources.m_LMaterial.m_Opacity = 1;
    commonShaderResources.m_LMaterial.m_Diffuse.set(1, 1, 1, 1);
    shrGpuRender = renderer->EF_LoadShaderItem(s_GPUParticles_Material_GPURenderNoGS, true, EF_RELOAD, &commonShaderResources);

    //Mark shaders as loaded
    requireShaderReload = false;
}

void CImpl_GPUParticles::RenderEmitterCubeDepthmap(SGPUParticleEmitterData* emitter)
{
    const float sCubeVector[6][7] =
    {
        {-1, 0, 0, 0, 0, 1, 90}, //posx
        { 1, 0, 0, 0, 0, 1, -90}, //negx
        { 0, -1, 0, 0, 0, -1,  0}, //posy
        { 0, 1, 0, 0, 0, 1,  0}, //negy
        { 0, 0, -1, 0, 1, 0,  0}, //posz
        { 0, 0, 1, 0, 1, 0,  0}, //negz
    };

    assert(emitter->resources.depthCubemap);
    SRenderPipeline& pipeline = renderer->m_RP;

    int threadID = pipeline.m_nProcessThreadID;

    //save previous view data
    int vX, vY, vWidth, vHeight;
    renderer->GetViewport(&vX, &vY, &vWidth, &vHeight);

    renderer->SetViewport(0, 0, dxRenderer->CV_r_CubeDepthMapResolution, dxRenderer->CV_r_CubeDepthMapResolution);

    Vec3 emitterPosition = emitter->baseData.transform.GetTranslation();

    //int prevReverseDepth = gcpRendD3D.CV_r_ReverseDepth;
    //gcpRendD3D.CV_r_ReverseDepth = 0;

    unsigned prevPersFlags = pipeline.m_TI[pipeline.m_nProcessThreadID].m_PersFlags;
    pipeline.m_TI[pipeline.m_nProcessThreadID].m_PersFlags &= ~(RBPF_OBLIQUE_FRUSTUM_CLIPPING);

    for (int side = 0; side < 6; ++side)
    {
        SDepthTexture renderTarget;

        renderTarget.nWidth = dxRenderer->CV_r_CubeDepthMapResolution;
        renderTarget.nHeight = dxRenderer->CV_r_CubeDepthMapResolution;
        renderTarget.nFrameAccess = -1;
        renderTarget.bBusy = false;
        renderTarget.pTex = emitter->resources.depthCubemap;
        renderTarget.pSurf = emitter->resources.depthCubemap->GetDeviceDepthStencilSurf(side, 1);

        //set projection+view data
        CCamera prevCamera = renderer->GetCamera();
        CCamera tmpCamera = prevCamera;

        Vec3 forward = Vec3(sCubeVector[side][0], sCubeVector[side][1], sCubeVector[side][2]);
        Vec3 up = Vec3(sCubeVector[side][3], sCubeVector[side][4], sCubeVector[side][5]);

        Matrix33 rotationMatrix = Matrix33::CreateOrientation(forward, up, DEG2RAD(sCubeVector[side][6]));
        Matrix34 finalTransform = Matrix34(rotationMatrix, emitterPosition);
        tmpCamera.SetMatrix(finalTransform);
        tmpCamera.SetFrustum(dxRenderer->CV_r_CubeDepthMapResolution, dxRenderer->CV_r_CubeDepthMapResolution,
            DEG2RAD(90.f), nearPlane, emitter->resourceParameters->fCubemapFarDistance);

        renderer->SetCamera(tmpCamera);

        gcpRendD3D->FX_PushRenderTarget(0, nullptr, &renderTarget, side);
        gcpRendD3D->FX_SetColorDontCareActions(0, true, true);

        //clear buffer
        gcpRendD3D->EF_ClearTargetsLater(FRT_CLEAR_DEPTH, 1.0f, 0);
        gcpRendD3D->FX_Commit();

        //disable color write
        gcpRendD3D->FX_SetState(GS_COLMASK_NONE, -1);
        pipeline.m_PersFlags2 |= RBPF2_DISABLECOLORWRITES;
        pipeline.m_StateOr |= GS_COLMASK_NONE;
        unsigned batchFilterPrev = pipeline.m_nBatchFilter;
        pipeline.m_nBatchFilter |= FB_Z;
        pipeline.m_pRenderFunc = CD3D9Renderer::FX_FlushShader_ZPass;

        gcpRendD3D->FX_ProcessZPassRender_List(EFSLIST_GPU_PARTICLE_CUBEMAP_COLLISION, FB_Z);

        gcpRendD3D->FX_PopRenderTarget(0);

        renderer->SetCamera(prevCamera);
        //reenable color write
        pipeline.m_PersFlags2 &= ~RBPF2_DISABLECOLORWRITES;
        pipeline.m_StateOr &= ~GS_COLMASK_NONE;
        pipeline.m_nBatchFilter = batchFilterPrev;
    }

    pipeline.m_TI[pipeline.m_nProcessThreadID].m_PersFlags = prevPersFlags;
    renderer->SetViewport(vX, vY, vWidth, vHeight);
}

void CImpl_GPUParticles::OnEffectChanged(GPUEmitterResources& resources, GPUEmitterBaseData& baseData, const ResourceParticleParams* particleParams, int emitterFlags)
{
    //only do depth cubemap if we are in the level
    if ((emitterFlags & ePEF_Nowhere) == 0)
    {
        //if we don't have a cubemap, and we need one, allocate it
        if ((particleParams->eDepthCollision == ParticleParams::EDepthCollision::Cubemap) && !resources.depthCubemap)
        {
            static unsigned short mapID = 0;
            char name_buffer[27]; //20 chars for the name, 1 space, max 5 digits, 1 NULL
            azsprintf(name_buffer, "ParticleDepthCubemap %05hu", mapID++);
            resources.depthCubemap = CTexture::CreateTextureObject(name_buffer, dxRenderer->CV_r_CubeDepthMapResolution, dxRenderer->CV_r_CubeDepthMapResolution,
                    1, eTT_Cube, FT_USAGE_DEPTHSTENCIL | FT_USAGE_UNORDERED_ACCESS, eTF_D16);
            const byte* pData[6] = { NULL };
            resources.depthCubemap->CreateDeviceTexture(pData);
        }
        //if we have one and don't need it, get rid of it
        else if ((particleParams->eDepthCollision != ParticleParams::EDepthCollision::Cubemap) && resources.depthCubemap)
        {
            SAFE_RELEASE(resources.depthCubemap);
        }
    }


    baseData.emitterIncrementXYZ.x = particleParams->vSpawnPosIncrementXYZ.fX.GetMaxValue() == 0 ? 0 : baseData.emitterIncrementXYZ.x;
    baseData.emitterIncrementXYZ.y = particleParams->vSpawnPosIncrementXYZ.fY.GetMaxValue() == 0 ? 0 : baseData.emitterIncrementXYZ.y;
    baseData.emitterIncrementXYZ.z = particleParams->vSpawnPosIncrementXYZ.fZ.GetMaxValue() == 0 ? 0 : baseData.emitterIncrementXYZ.z;
}

void CImpl_GPUParticles::AddSubemitter(SGPUParticleEmitterData* emitter, int offset)
{
    GPUSubemitterData emitData;

    InitializeSubemitter(emitData, offset);
    UpdateParameterCount(emitData, emitter->resourceParameters, emitter->spawnParams);
    UpdateParameterPulsePeriod(emitData, emitter->resourceParameters, emitter->spawnParams);
    UpdateEmitterLifetimeParams(emitData, emitter->resourceParameters, emitter->spawnParams, emitter->baseData.time);

    emitter->subEmitters.push_back(emitData);
}

void CImpl_GPUParticles::SpawnSubemitter(SGPUParticleEmitterData* emitter, Vec3 position)
{
    if (emitter->availableOffsets.empty())
    {
        GrowBuffers(*emitter);
    }

    int offset = emitter->availableOffsets.top();
    emitter->availableOffsets.pop();

    AddSubemitter(emitter, offset * emitter->baseData.numParticlesPerChunk);
    emitter->subEmitters.back().transform = Matrix34::CreateTranslationMat(position);
}

void CImpl_GPUParticles::ReadbackDeathLocationsAndSpawnChildren(SGPUParticleEmitterData& emitter)
{
    D3D11_MAPPED_SUBRESOURCE mappedRes;
    gcpRendD3D->GetDeviceContext().Map(emitter.resources.bufDeadParticleLocationsStaging.m_pBuffer, 0, D3D11_MAP_READ, 0, &mappedRes);
    char* tmpMem = new char[emitter.resources.bufDeadParticleLocationsStaging.m_numElements * sizeof(float)];
    std::memcpy(tmpMem, mappedRes.pData, emitter.resources.bufDeadParticleLocationsStaging.m_numElements * sizeof(float));
    gcpRendD3D->GetDeviceContext().Unmap(emitter.resources.bufDeadParticleLocationsStaging.m_pBuffer, 0);

    int num_positions = *reinterpret_cast<int*>(tmpMem);
    Vec4* positions = reinterpret_cast<Vec4*>(tmpMem + sizeof(Vec4));

    //avoid buffer overruns
    if (num_positions > emitter.resources.numDeathLocations)
    {
        delete[] tmpMem;
        return;
    }

    for (int i = 0; i < num_positions; ++i)
    {
        Vec3 pos(positions[i].x, positions[i].y, positions[i].z);
        for (auto child : emitter.children)
        {
            SpawnSubemitter(child, pos);
        }
    }

    delete[] tmpMem;
}

void CImpl_GPUParticles::SetEmitterShapeFlag(ParticleParams::EEmitterShapeType type)
{
    switch (type)
    {
    case ParticleParams::EEmitterShapeType::BOX:
        dxRenderer->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_GPU_PARTICLE_SHAPE_BOX];
        break;
    case ParticleParams::EEmitterShapeType::POINT:
        dxRenderer->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_GPU_PARTICLE_SHAPE_POINT];
        break;
    case ParticleParams::EEmitterShapeType::CIRCLE:
        dxRenderer->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_GPU_PARTICLE_SHAPE_CIRCLE];
        break;
    case ParticleParams::EEmitterShapeType::SPHERE:
        dxRenderer->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_GPU_PARTICLE_SHAPE_SPHERE];
        break;
    default:
        dxRenderer->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_GPU_PARTICLE_SHAPE_ANGLE];
        break;
    }
}

// *** CD3DGPUParticleEngine Implementation ---------------------------------------------------------------------------------
CD3DGPUParticleEngine::CD3DGPUParticleEngine()
{
    m_impl = new CImpl_GPUParticles();
    m_impl->initialized = false;

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION D3DGPUPARTICLEENGINE_CPP_SECTION_8
#include AZ_RESTRICTED_FILE(D3DGPUParticleEngine_cpp, AZ_RESTRICTED_PLATFORM)
#endif
}

CD3DGPUParticleEngine::~CD3DGPUParticleEngine()
{
    delete m_impl;
}

bool CD3DGPUParticleEngine::Initialize(CRenderer* renderer)
{
    if (m_impl->initialized)
    {
        return true;
    }
    m_impl->initialized = true;

    CRY_ASSERT(renderer);

    // store renderer internally
    m_impl->renderer = renderer;
    g_system_event_listener.renderer = renderer;
    GetISystem()->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener);
    m_impl->dxRenderer = static_cast<CD3D9Renderer*>(m_impl->renderer);

    // initialize shared resources
    m_impl->LoadShaderItems();

    gpuParticleEngine_impl = m_impl;

    m_impl->UpdateShaders();

    return true;
}

void CD3DGPUParticleEngine::Update(EmitterTypePtr pEmitter)
{
    SGPUParticleEmitterData& emitter = *reinterpret_cast<SGPUParticleEmitterData*>(pEmitter);

    // check if there were changes in the emitter data & reinitialize emitter if this is the case
    m_impl->UpdateEmitterCounts(&emitter);
    if (emitter.baseData.stagedMaxParticlesInBuffer != emitter.baseData.numParticlesPerChunk)
    {
        m_impl->InitializeEmitter(&emitter);
    }

    const float frameTime = gEnv->pTimer->GetFrameTime() * emitter.spawnParams.fTimeScale + emitter.spawnParams.fStepValue;

    if (emitter.baseData.isPrimed)
    {
        emitter.baseData.isPrimed = false;
        emitter.baseData.time = MAX(frameTime, emitter.baseData.time);
        emitter.baseData.deltaTime = emitter.baseData.time;
    }
    else
    {
        emitter.baseData.time += frameTime;
        emitter.baseData.deltaTime = frameTime;
    }

    for (auto emitData = emitter.subEmitters.begin(); emitData != emitter.subEmitters.end(); )
    {
        if (UpdateSubemitter(*emitData, emitter.resourceParameters, emitter.spawnParams, emitter.baseData.time, emitter.baseData.deltaTime, emitter.baseData.target))
        {
            if (!emitter.baseData.isIndirect)
            {
                emitData->transform = emitter.baseData.transform;
            }

            // start compute shader.
            m_impl->UpdateGPU(emitter.baseData, emitter.resources, *emitData, emitter.resourceParameters, emitter.spawnParams);
        }
        else if (emitter.baseData.isIndirect)
        {
            //remove emitter
            int offset = emitData->bufferOffset / emitter.baseData.numParticlesPerChunk;
            emitter.availableOffsets.push(offset);
            std::swap(*emitData, *emitter.subEmitters.rbegin());
            if (emitData == (emitter.subEmitters.end() - 1))
            {
                emitter.subEmitters.pop_back();
                break;
            }
            emitter.subEmitters.pop_back();
            continue;
        }

        emitData++;
    }
}

void CD3DGPUParticleEngine::Render(EmitterTypePtr emitter, EGPUParticlePass pass, int shadowMode, float fov, float aspectRatio, bool isWireframeEnabled)
{
    SGPUParticleEmitterData* pEmitter = reinterpret_cast<SGPUParticleEmitterData*>(emitter);

    RenderPassData renderPassData;
    {
        renderPassData.pass = pass;
        renderPassData.shadowMode = shadowMode;
        renderPassData.fov = fov;
        renderPassData.aspectRatio = aspectRatio;
        renderPassData.isWireframeEnabled = isWireframeEnabled;

        // grab the projection and model view
        m_impl->dxRenderer->GetProjectionMatrix(renderPassData.projection.GetData());
        m_impl->dxRenderer->GetModelViewMatrix(renderPassData.modelview.GetData());

        // before transpose grab the left and up vector from the view matrix
        renderPassData.cameraRightAxis = renderPassData.modelview.GetColumn4(0);
        renderPassData.cameraUpAxis = renderPassData.modelview.GetColumn4(1);
        renderPassData.cameraLookDir = renderPassData.modelview.GetColumn4(2) * -1;
        renderPassData.cameraRightAxis.w = 0.0f;
        renderPassData.cameraUpAxis.w = 0.0f;
        renderPassData.cameraLookDir.w = 0.0f;
        renderPassData.cameraRightAxis.Normalize();
        renderPassData.cameraUpAxis.Normalize();
        renderPassData.cameraLookDir.Normalize();

        // transpose to use projection and modelview matrix in column major order m * v
        renderPassData.projection.Transpose();
        renderPassData.modelview.Transpose();

        renderPassData.cameraPos = Vec4(renderPassData.modelview.GetInverted().GetTranslation(), 0.0f);
    }

    for (GPUSubemitterData& emitData : pEmitter->subEmitters)
    {
        emitData.renderTimeInMilliseconds = m_impl->RenderEmitter(renderPassData, pEmitter->baseData, pEmitter->resources, emitData, pEmitter->resourceParameters, pEmitter->spawnParams);
    }

    if (pass == EGPUParticlePass::Main)
    {
        m_impl->latestModelview = renderPassData.modelview;
        m_impl->latestProjection = renderPassData.projection;
        m_impl->latestCameraPos = renderPassData.cameraPos;
    }
    

    if (pass != EGPUParticlePass::Shadow)
    {
#if PROFILE_GPU_PARTICLES
        const ColorF color = Col_SteelBlue;
        const float xpos = 10, ypos = 10;
        const float verticalIndent = 20.0f;
        for (unsigned int i = 0; i < CGPUParticleProfiler::COUNT; ++i)
        {
            const float yOffset = i * verticalIndent;

            float timeOUT = GPU_PARTICLE_PROFILER_GET_TIME((&(pEmitter->resources)), i);
            m_impl->dxRenderer->Draw2dLabel(xpos, ypos + yOffset, 1.5f, &color.r, false, "%s", GPU_PARTICLE_PROFILER_GET_INDEX_NAME((&(pEmitter->resources)), i));
            m_impl->dxRenderer->Draw2dLabel(xpos + 130, ypos + yOffset, 1.5f, &color.r, false, "%.8fms", timeOUT);
        }
        {// Throttle stats. Will only the report the most-significant sub-emitter.
            const float yOffset = verticalIndent * CGPUParticleProfiler::COUNT;
            bool isThrottled = false;
            m_impl->dxRenderer->Draw2dLabel(xpos, ypos + yOffset, 1.5f, &color.r, false, "EMIT THROTTLE");

            for (const auto& subEmitter : pEmitter->subEmitters)
            {
                if (subEmitter.particleSpawnPerformanceScale < 1.0f)
                {
                    const float throttlePercentage = (1.0f - subEmitter.particleSpawnPerformanceScale) * 100.0f;
                    m_impl->dxRenderer->Draw2dLabel(xpos + 130, ypos + yOffset, 1.5f, &color.r, false, "%.2f%%", throttlePercentage);
                    isThrottled = true;
                    break;
                }
            }

            if (!isThrottled)
            {
                m_impl->dxRenderer->Draw2dLabel(xpos + 130, ypos + yOffset, 1.5f, &color.r, false, "0.00%%");
            }
        }
        m_impl->dxRenderer->RT_RenderTextMessages();
#endif
    }
}

void CD3DGPUParticleEngine::UpdateFrame()
{
    if (m_impl->emittersQueuedForUpdate.size() == 0)
    {
        return;
    }

    PROFILE_LABEL_SCOPE("UPDATE_GPU_PARTICLES");

    // sort particles that need sorting
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION D3DGPUPARTICLEENGINE_CPP_SECTION_9
#include AZ_RESTRICTED_FILE(D3DGPUParticleEngine_cpp, AZ_RESTRICTED_PLATFORM)
#endif

    //render cubemaps for collision, if necesary
    for (int i = 0; i < m_impl->emittersQueuedForUpdate.size(); ++i)
    {
        if (m_impl->emittersQueuedForUpdate[i]->resources.depthCubemap)
        {
            m_impl->RenderEmitterCubeDepthmap(m_impl->emittersQueuedForUpdate[i]);
        }
    }

    // Update shaders in case they weren't loaded in Initialize() if it was called on a non-render thread
    m_impl->UpdateShaders();

    // init emitter update (reset frame state)
    for (int i = 0; i < m_impl->emittersQueuedForUpdate.size(); ++i)
    {
        //spawn children before counts are cleared for the next frame
        if (m_impl->emittersQueuedForUpdate[i]->baseData.isIndirectParent)
        {
            m_impl->ReadbackDeathLocationsAndSpawnChildren(*m_impl->emittersQueuedForUpdate[i]);
        }
        m_impl->BeginUpdate(m_impl->emittersQueuedForUpdate[i]->resources, m_impl->emittersQueuedForUpdate[i]->numMaxSubemitters);
    }

    // update emitters
    for (int i = 0; i < m_impl->emittersQueuedForUpdate.size(); ++i)
    {
        Update(m_impl->emittersQueuedForUpdate[i]);
    }

    // spawn new particles from dead list, if needed
    for (int i = 0; i < m_impl->emittersQueuedForUpdate.size(); ++i)
    {
        m_impl->EmitParticles(m_impl->emittersQueuedForUpdate[i]);
    }

    // sort particles that need sorting
    for (int i = 0; i < m_impl->emittersQueuedForUpdate.size(); ++i)
    {
        auto& emitter = m_impl->emittersQueuedForUpdate[i];
        GPUEmitterResources& emitResources = emitter->resources;

        if (emitter->resourceParameters->eBlendType == ParticleParams::EBlend::AlphaBased)
        {
            if (emitter->resourceParameters->eSortMethod != ParticleParams::ESortMethod::None)
            {
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION D3DGPUPARTICLEENGINE_CPP_SECTION_10
#include AZ_RESTRICTED_FILE(D3DGPUParticleEngine_cpp, AZ_RESTRICTED_PLATFORM)
#endif
                m_impl->GatherSortScore(emitter->resources);
            }

            if (emitter->resourceParameters->eSortMethod == ParticleParams::ESortMethod::Bitonic)
            {
                PROFILE_LABEL_SCOPE("SORT_BITONIC_GPU_PARTICLES");
                for (GPUSubemitterData& emitData : emitter->subEmitters)
                {
                    m_impl->SortParticlesBitonicLocal(emitter->resources, emitter->baseData, emitData);
                    m_impl->SortParticlesBitonic(emitter->resources, emitter->baseData, emitData);
                }
            }
            else if (emitter->resourceParameters->eSortMethod == ParticleParams::ESortMethod::OddEven)
            {
                for (GPUSubemitterData& emitData : emitter->subEmitters)
                {
                    m_impl->SortParticlesOddEvenIncremental(emitter->resources, emitter->baseData, emitData, emitter->resourceParameters);
                }
            }

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION D3DGPUPARTICLEENGINE_CPP_SECTION_11
#include AZ_RESTRICTED_FILE(D3DGPUParticleEngine_cpp, AZ_RESTRICTED_PLATFORM)
#endif
        }
    }

    // finalize emitter update
    for (int i = 0; i < m_impl->emittersQueuedForUpdate.size(); ++i)
    {
        auto& emitter = m_impl->emittersQueuedForUpdate[i];
        GPUEmitterResources& emitResources = emitter->resources;

        m_impl->EndUpdate(emitter->baseData);

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION D3DGPUPARTICLEENGINE_CPP_SECTION_12
#include AZ_RESTRICTED_FILE(D3DGPUParticleEngine_cpp, AZ_RESTRICTED_PLATFORM)
#endif
    }

    // clear queue
    m_impl->emittersQueuedForUpdate.resize(0);
}

SShaderItem* CD3DGPUParticleEngine::GetRenderShader()
{
    SShaderItem* renderShader = &m_impl->shrGpuRender;

    return renderShader;
}

void CD3DGPUParticleEngine::SetEmitterTransform(EmitterTypePtr pEmitter, const Matrix34& transform)
{
    SGPUParticleEmitterData* emitter = reinterpret_cast<SGPUParticleEmitterData*>(pEmitter);
    emitter->baseData.transform = transform;

    if (emitter->baseData.isFirstUpdatePass)
    {
        emitter->baseData.previousTransform = transform;
        emitter->baseData.isFirstUpdatePass = false;
    }
}

void CD3DGPUParticleEngine::SetEmitterResourceParameters(EmitterTypePtr pEmitter, const ResourceParticleParams* parameters)
{
    SGPUParticleEmitterData* emitter = reinterpret_cast<SGPUParticleEmitterData*>(pEmitter);
    emitter->resourceParameters = parameters;
}

void CD3DGPUParticleEngine::SetEmitterLodBlendAlpha(EmitterTypePtr pEmitter, const float lodBlendAlpha)
{
    SGPUParticleEmitterData* emitter = reinterpret_cast<SGPUParticleEmitterData*>(pEmitter);
    emitter->baseData.lodBlendAlpha = lodBlendAlpha;
}

void CD3DGPUParticleEngine::QueueEmitterNextFrame(EmitterTypePtr pEmitter, bool isAuxWindowUpdate)
{
    SGPUParticleEmitterData* emitterData = reinterpret_cast<SGPUParticleEmitterData*>(pEmitter);
    if (emitterData->baseData.frameQueued == m_impl->renderer->GetFrameID())
    {
        return; // already queued
    }
    emitterData->baseData.isAuxWindowUpdate = isAuxWindowUpdate;
    emitterData->baseData.frameQueued = m_impl->renderer->GetFrameID();
    m_impl->emittersQueuedForUpdate.push_back(emitterData);
}

void CD3DGPUParticleEngine::Release()
{
    m_impl->initialized = false;

    SAFE_RELEASE(m_impl->shrGpuBegin);
    SAFE_RELEASE(m_impl->shrGpuUpdate);
    SAFE_RELEASE(m_impl->shrGpuEmit);
    SAFE_RELEASE(m_impl->shrGpuBitonicSort);
    SAFE_RELEASE(m_impl->shrGpuBitonicSortGlobal2048);
    SAFE_RELEASE(m_impl->shrGpuBitonicSortLocal);
    SAFE_RELEASE(m_impl->shrGpuOddEvenSort);
    SAFE_RELEASE(m_impl->shrGpuGatherSortDistance);

    SAFE_RELEASE(m_impl->shrGpuRender.m_pShader);
    SAFE_RELEASE(m_impl->shrGpuRender.m_pShaderResources);

    m_impl->requireShaderReload = true;
}

IGPUParticleEngine::EmitterTypePtr CD3DGPUParticleEngine::AddEmitter(const ResourceParticleParams* parameters, const SpawnParams& spawnParams, const unsigned* emitterFlags, EmitterTypePtr parent, const ParticleTarget* target, const SPhysEnviron* physEnv)
{
    auto emitter = new SGPUParticleEmitterData(spawnParams);
    emitter->resourceParameters = parameters;
    emitter->baseData.emitterFlags = emitterFlags;
    emitter->baseData.target = target;
    emitter->baseData.physEnv = physEnv;
    AddChild(reinterpret_cast<SGPUParticleEmitterData*>(parent), emitter);
    if (parent)
    {
        emitter->baseData.isIndirect = true;
    }
    m_impl->InitializeEmitter(emitter);
    m_impl->emitters.push_back(emitter);
    return emitter;
}

bool CD3DGPUParticleEngine::RemoveEmitter(EmitterTypePtr emitter)
{
    SGPUParticleEmitterData* emitterData = reinterpret_cast<SGPUParticleEmitterData*>(emitter);

    // remove from queued emitters
    auto itQueued = std::find(m_impl->emittersQueuedForUpdate.begin(), m_impl->emittersQueuedForUpdate.end(), emitter);
    if (itQueued != m_impl->emittersQueuedForUpdate.end())
    {
        m_impl->emittersQueuedForUpdate.erase(itQueued);
    }

    //go through children and reset parent pointer
    for (auto child : emitterData->children)
    {
        child->parent = nullptr;
    }

    //remove from parent's child array
    if (emitterData->parent)
    {
        auto itChild = std::find(emitterData->parent->children.begin(), emitterData->parent->children.end(), emitterData);
        if (itChild != emitterData->parent->children.end())
        {
            emitterData->parent->children.erase(itChild);
        }
    }

    // remove from emitter list
    auto itEmitter = std::find(m_impl->emitters.begin(), m_impl->emitters.end(), emitter);
    if (itEmitter != m_impl->emitters.end())
    {
        auto emitterToDelete = *itEmitter;
        m_impl->emitters.erase(itEmitter);
        delete emitterToDelete;
        return true;
    }
    return false;
}

void CD3DGPUParticleEngine::ResetEmitter(EmitterTypePtr pEmitter)
{
    SGPUParticleEmitterData* emitter = reinterpret_cast<SGPUParticleEmitterData*>(pEmitter);
    emitter->subEmitters.clear();

    // Need to reset emitter->baseData.time *before* calling AddSubemitter because AddSubemitter uses it.
    // Before, the pulse "repeatAge" was getting delayed by whatever value was left over in baseData.time.
    emitter->baseData.time = 0.0f;

    if (!emitter->baseData.isIndirect)
    {
        m_impl->AddSubemitter(emitter, 0);
    }
}


void CD3DGPUParticleEngine::StartEmitter(EmitterTypePtr pEmitter)
{
    SGPUParticleEmitterData* emitter = reinterpret_cast<SGPUParticleEmitterData*>(pEmitter);
    for (GPUSubemitterData& emitData : emitter->subEmitters)
    {
        UpdateEmitterLifetimeParams(emitData, emitter->resourceParameters, emitter->spawnParams, emitter->baseData.time);
    }
}

void CD3DGPUParticleEngine::StopEmitter(EmitterTypePtr pEmitter)
{
    SGPUParticleEmitterData* emitter = reinterpret_cast<SGPUParticleEmitterData*>(pEmitter);
    for (GPUSubemitterData& emitData : emitter->subEmitters)
    {
        emitData.stopAge = emitter->baseData.time;
    }
}

void CD3DGPUParticleEngine::PrimeEmitter(EmitterTypePtr pEmitter, float equilibriumAge)
{
    SGPUParticleEmitterData* emitter = reinterpret_cast<SGPUParticleEmitterData*>(pEmitter);
    emitter->baseData.time = equilibriumAge;
    emitter->baseData.isPrimed = true;
}

IGPUParticleEngine::EmitterTypePtr CD3DGPUParticleEngine::GetEmitterByIndex(int index)
{
    CRY_ASSERT(index < m_impl->emitters.size());
    return m_impl->emitters[index];
}

int CD3DGPUParticleEngine::GetEmitterCount()
{
    return m_impl->emitters.size();
}

void CD3DGPUParticleEngine::OnEffectChanged(EmitterTypePtr pEmitter)
{
    SGPUParticleEmitterData* emitter = reinterpret_cast<SGPUParticleEmitterData*>(pEmitter);

    m_impl->OnEffectChanged(emitter->resources, emitter->baseData, emitter->resourceParameters, *emitter->baseData.emitterFlags);
}
