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

#include "StdAfx.h"
#include "Statoscope.h"
#include "FrameProfileSystem.h"
#include <IGame.h>
#include <IGameFramework.h>
#include <IRenderer.h>
#include <ICryAnimation.h>
#include "SimpleStringPool.h"
#include "System.h"
#include "ThreadProfiler.h"

#include "StatoscopeStreamingIntervalGroup.h"
#include "StatoscopeTextureStreamingIntervalGroup.h"

#include <AzCore/Socket/AzSocket.h>


#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define STATOSCOPE_CPP_SECTION_1 1
#define STATOSCOPE_CPP_SECTION_2 2
#endif

#if ENABLE_STATOSCOPE

namespace
{
    const int default_statoscope_port = 29527;

    class CCompareFrameProfilersSelfTime
    {
    public:
        bool operator() (const std::pair<CFrameProfiler*, int64>& p1, const std::pair<CFrameProfiler*, int64>& p2)
        {
            return p1.second > p2.second;
        }
    };
}

static string GetFrameProfilerPath(CFrameProfiler* pProfiler)
{
    if (pProfiler)
    {
        const char* sThreadName = CryThreadGetName(pProfiler->m_threadId);
        char sThreadNameBuf[11]; // 0x 12345678 \0 => 2+8+1=11
        if (!sThreadName || !sThreadName[0])
        {
            azsnprintf(sThreadNameBuf, sizeof(sThreadNameBuf), "%" PRI_THREADID "", (pProfiler->m_threadId));
        }

        char sNameBuffer[256];
        sprintf_s(sNameBuffer, sizeof(sNameBuffer), pProfiler->m_name);

#if !defined(_MSC_VER) // __FUNCTION__ only contains classname on MSVC, for other function we have __PRETTY_FUNCTION__, so we need to strip return / argument types
        {
            char* pEnd = (char*)strchr(sNameBuffer, '(');
            if (pEnd)
            {
                *pEnd = 0;
                while (*(pEnd) != ' ' && *(pEnd) != '*' && pEnd != (sNameBuffer - 1))
                {
                    --pEnd;
                }
                memmove(sNameBuffer, pEnd + 1, &sNameBuffer[sizeof(sNameBuffer)] - (pEnd + 1));
            }
        }
#endif

        string path = sThreadName ? sThreadName : sThreadNameBuf;
        path += "/";
        path += gEnv->pFrameProfileSystem->GetModuleName(pProfiler);
        path += "/";
        path += sNameBuffer;
        path += "/";

        return path;
    }
    else
    {
        return string("SmallFunctions/SmallFunction/");
    }
}

CStatoscopeDataClass::CStatoscopeDataClass(const char* format)
    : m_format(format)
    , m_numDataElements(0)
{
    uint32 numOpening = 0;
    uint32 numClosing = 0;
    uint32 numDollars = 0;
    const char* pNameStart = NULL;

    string formatString(format);
    int pathStart = formatString.find_first_of('\'');
    int pathEnd = formatString.find_first_of('\'', pathStart + 1);
    m_path = formatString.substr(pathStart + 1, pathEnd - 2);

    for (const char* c = format; c && *c != '\0'; ++c)
    {
        if (*c == '(')
        {
            ++numOpening;
            pNameStart = c + 1;
        }
        else if (*c == ')')
        {
            ++numClosing;

            if (pNameStart)
            {
                ProcessNewBinDataElem(pNameStart, c);
                pNameStart = NULL;
            }
        }
        else if (*c == '$')
        {
            ++numDollars;
        }
    }
    if (numClosing == numOpening)
    {
        m_numDataElements = numOpening + numDollars;
    }
    else
    {
        m_numDataElements = 0;
        CryFatalError("Mismatched opening/closing braces in Statoscope format description.");
    }
}

void CStatoscopeDataClass::ProcessNewBinDataElem(const char* pStart, const char* pEnd)
{
    if (pStart)
    {
        BinDataElement newElem;

        //determine data type
        string s(pStart);
        int spaceIndex = s.find_first_of(' ');

        if (spaceIndex != -1)
        {
            string typeString = s.substr(0, spaceIndex);
            bool bBroken = false;

            if (typeString.compareNoCase("float") == 0)
            {
                newElem.type = DataWriter::Float;
            }
            else if (typeString.compareNoCase("int") == 0)
            {
                newElem.type = DataWriter::Int;
            }
            else if (typeString.compareNoCase("int64") == 0)
            {
                newElem.type = DataWriter::Int64;
            }
            else if (typeString.compareNoCase("string") == 0)
            {
                newElem.type = DataWriter::String;
            }
            else
            {
                bBroken = true;
                CryLogAlways("Broken!");
            }

            if (!bBroken)
            {
                int bracketIndex = s.find_first_of(')');
                newElem.name = s.substr(spaceIndex + 1, bracketIndex - spaceIndex - 1);

                m_binElements.push_back(newElem);
            }
        }
    }
}

void CStatoscopeDataGroup::WriteHeader(CDataWriter* pDataWriter)
{
    pDataWriter->WriteDataStr(m_dataClass.GetPath());
    int nDataElems = (int)m_dataClass.GetNumBinElements();

    pDataWriter->WriteData(nDataElems);

    for (int i = 0; i < nDataElems; i++)
    {
        const CStatoscopeDataClass::BinDataElement& elem = m_dataClass.GetBinElement(i);
        pDataWriter->WriteData(elem.type);
        pDataWriter->WriteDataStr(elem.name.c_str());
    }
}

CStatoscopeIntervalGroup::CStatoscopeIntervalGroup(uint32 id, const char* name, const char* format)
    : m_id(id)
    , m_name(name)
    , m_dataClass(format)
    , m_pWriter(NULL)
{
}

void CStatoscopeIntervalGroup::Enable(CStatoscopeEventWriter* pWriter)
{
    m_pWriter = pWriter;
    Enable_Impl();
}

void CStatoscopeIntervalGroup::Disable()
{
    Disable_Impl();
    m_pWriter = NULL;
}

size_t CStatoscopeIntervalGroup::GetDescEventLength() const
{
    size_t numElements = m_dataClass.GetNumElements();
    size_t length = numElements * sizeof(uint8);

    for (size_t i = 0; i < m_dataClass.GetNumBinElements(); ++i)
    {
        const CStatoscopeDataClass::BinDataElement& elem = m_dataClass.GetBinElement(i);
        length += elem.name.length() + 1;
    }

    return length;
}

void CStatoscopeIntervalGroup::WriteDescEvent(void* p) const
{
    size_t numElements = m_dataClass.GetNumElements();

    char* pc = (char*)p;
    for (size_t i = 0; i < m_dataClass.GetNumBinElements(); ++i)
    {
        const CStatoscopeDataClass::BinDataElement& elem = m_dataClass.GetBinElement(i);
        *pc++ = (char)elem.type;
        azstrcpy(pc, elem.name.length() + 1, elem.name.c_str());
        pc += elem.name.length() + 1;
    }
}

void CStatoscopeFrameRecordWriter::AddValue(float f)
{
    m_pDataWriter->WriteData(f);
    ++m_nWrittenElements;
}

void CStatoscopeFrameRecordWriter::AddValue(const char* s)
{
    m_pDataWriter->WriteDataStr(s);
    ++m_nWrittenElements;
}

void CStatoscopeFrameRecordWriter::AddValue(int i)
{
    m_pDataWriter->WriteData(i);
    ++m_nWrittenElements;
}

struct SFrameLengthDG
    : public IStatoscopeDataGroup
{
    virtual SDescription GetDescription() const
    {
        return SDescription('f', "frame lengths", "['/' (float frameLengthInMS) (float lostProfilerTimeInMS) ]");
    }

    virtual void Write(IStatoscopeFrameRecord& fr)
    {
        IFrameProfileSystem* pFrameProfileSystem = gEnv->pSystem->GetIProfileSystem();

        fr.AddValue(gEnv->pTimer->GetRealFrameTime() * 1000.0f);
        fr.AddValue(pFrameProfileSystem ? pFrameProfileSystem->GetLostFrameTimeMS() : -1.f);
    }
};

struct SEF_ListDG
    : public IStatoscopeDataGroup
{
    virtual SDescription GetDescription() const
    {
        //These MUST match the EFSLIST_ lists from IShader.h
        return SDescription('e', "effects", "['/Effects/' "
            "(float timeInvalid) "
            "(float timePreProcess) "
            "(float timeGeneral) "
            "(float timeTerrainLayer) "
            "(float timeShadowGen) "
            "(float timeDecal) "
            "(float timeWaterVolumes) "
            "(float timeTransparent) "
            "(float timeWater) "
            "(float timeHDRPostProcess) "
            "(float timeAfterHDRPostProcess) "
            "(float timePostProcess) "
            "(float timeAfterPostProcess) "
            "(float timeShadowPass) "
            "(float timeDeferredPreProcess) "
            "(float timeSkin) "
            "(float timeHalfResParticles) "
            "(float timeParticlesThickness) "
            "(float timeLensOptic) "
            "(float timeEyeOverlay) "
            "(float sumTimeEffects) "
            "]");
    }

    virtual void Write(IStatoscopeFrameRecord& fr)
    {
        float fPassTime, fTotalTime = 0.0f;
        for (int i = 0; i < EFSLIST_NUM; i++)
        {
            fPassTime = gEnv->pRenderer->GetCurrentDrawCallRTTimes(1 << i);
            fr.AddValue(fPassTime * 1000.0f);
            fTotalTime += fPassTime;
        }
        fr.AddValue(fTotalTime * 1000.0f);
    }
};

struct SMemoryDG
    : public IStatoscopeDataGroup
{
    virtual SDescription GetDescription() const
    {
        return SDescription('m', "memory", "['/Memory/' (float mainMemUsageInMB) (int vidMemUsageInMB)]");
    }

    virtual void Write(IStatoscopeFrameRecord& fr)
    {
        IMemoryManager::SProcessMemInfo processMemInfo;
        GetISystem()->GetIMemoryManager()->GetProcessMemInfo(processMemInfo);
        fr.AddValue(processMemInfo.PagefileUsage / (1024.f * 1024.f));

        size_t vidMem, lastVidMem;
        gEnv->pRenderer->GetVideoMemoryUsageStats(vidMem, lastVidMem);
        fr.AddValue((int)vidMem);
    }
};

struct SStreamingDG
    : public IStatoscopeDataGroup
{
    virtual SDescription GetDescription() const
    {
        return SDescription('s', "streaming", "['/Streaming/' (float cgfStreamingMemUsedInMB) (float cgfStreamingMemRequiredInMB) (int cgfStreamingPoolSize) (float tempMemInKB)]");
    }

    virtual void Write(IStatoscopeFrameRecord& fr)
    {
        I3DEngine::SObjectsStreamingStatus objectsStreamingStatus;
        gEnv->p3DEngine->GetObjectsStreamingStatus(objectsStreamingStatus);
        fr.AddValue(objectsStreamingStatus.nAllocatedBytes / (1024.f * 1024.f));
        fr.AddValue(objectsStreamingStatus.nMemRequired / (1024.f * 1024.f));
        fr.AddValue(objectsStreamingStatus.nMeshPoolSize);
        fr.AddValue(gEnv->pSystem->GetStreamEngine()->GetStreamingStatistics().nTempMemory / 1024.f);
    }
};

struct SStreamingAudioDG
    : public IStatoscopeDataGroup
{
    virtual SDescription GetDescription() const
    {
        return SDescription('a', "streaming audio", "['/StreamingAudio/' (float bandwidthActualKBsecond) (float bandwidthRequestedKBsecond)]");
    }

    virtual void Write(IStatoscopeFrameRecord& fr)
    {
        I3DEngine::SStremaingBandwidthData subsystemStreamingData;
        memset(&subsystemStreamingData, 0, sizeof(I3DEngine::SStremaingBandwidthData));
        gEnv->p3DEngine->GetStreamingSubsystemData(eStreamTaskTypeSound, subsystemStreamingData);
        fr.AddValue(subsystemStreamingData.fBandwidthActual);
        fr.AddValue(subsystemStreamingData.fBandwidthRequested);
    }
};

struct SStreamingObjectsDG
    : public IStatoscopeDataGroup
{
    virtual SDescription GetDescription() const
    {
        return SDescription('o', "streaming objects", "['/StreamingObjects/' (float bandwidthActualKBsecond) (float bandwidthRequestedKBsecond)]");
    }

    virtual void Write(IStatoscopeFrameRecord& fr)
    {
        I3DEngine::SStremaingBandwidthData subsystemStreamingData;
        memset(&subsystemStreamingData, 0, sizeof(I3DEngine::SStremaingBandwidthData));
        gEnv->p3DEngine->GetStreamingSubsystemData(eStreamTaskTypeGeometry, subsystemStreamingData);
        fr.AddValue(subsystemStreamingData.fBandwidthActual);
        fr.AddValue(subsystemStreamingData.fBandwidthRequested);
    }
};

struct SThreadsDG
    : public IStatoscopeDataGroup
{
    virtual SDescription GetDescription() const
    {
        return SDescription('t', "threading", "['/Threading/' (float MTLoadInMS) (float MTWaitingForRTInMS) "
            "(float RTLoadInMS) (float RTWaitingForMTInMS) (float RTWaitingForGPUInMS) "
            "(float RTFrameLengthInMS) (float RTSceneDrawningLengthInMS) (float NetThreadTimeInMS)]");
    }

    virtual void Write(IStatoscopeFrameRecord& fr)
    {
        IRenderer::SRenderTimes renderTimes;
        gEnv->pRenderer->GetRenderTimes(renderTimes);

        SNetworkPerformance netPerformance;
        if (gEnv->pNetwork)
        {
            gEnv->pNetwork->GetPerformanceStatistics(&netPerformance);
        }
        else
        {
            netPerformance.m_threadTime = 0.0f;
        }

        float RTWaitingForMTInMS = renderTimes.fWaitForMain * 1000.f;
        float MTWaitingForRTInMS = renderTimes.fWaitForRender * 1000.f;
        float RTWaitingForGPUInMS = renderTimes.fWaitForGPU * 1000.f;
        float RTLoadInMS = renderTimes.fTimeProcessedRT * 1000.f;

        float MTLoadInMS = (gEnv->pTimer->GetRealFrameTime() * 1000.0f) - MTWaitingForRTInMS;

        //Load represents pure RT work, so compensate for GPU sync
        RTLoadInMS = RTLoadInMS - RTWaitingForGPUInMS;

        fr.AddValue(MTLoadInMS);
        fr.AddValue(MTWaitingForRTInMS);
        fr.AddValue(RTLoadInMS);
        fr.AddValue(RTWaitingForMTInMS);
        fr.AddValue(RTWaitingForGPUInMS);
        fr.AddValue(renderTimes.fTimeProcessedRT * 1000);
        fr.AddValue(renderTimes.fTimeProcessedRTScene * 1000);
        fr.AddValue(netPerformance.m_threadTime * 1000.f);
    }
};

struct SSystemThreadsDG
    : public IStatoscopeDataGroup
{
    virtual SDescription GetDescription() const
    {
        return SDescription('T', "system threading", "['/SystemThreading/' "
            "(float MTLoadInMS) (float RTLoadInMS) (float otherLoadInMS) "
            "(float sysIdle0InMS) (float sysIdle1InMS) (float sysIdleTotalInMS) "
            "(float totalLoadInMS) (float timeFrameInMS)]");
    }

    virtual void Enable()
    {
        IStatoscopeDataGroup::Enable();

        SSystemThreadsDG::StartThreadProf();
    }

    virtual void Disable()
    {
        IStatoscopeDataGroup::Disable();

        SSystemThreadsDG::StopThreadProf();
    }

    virtual void Write(IStatoscopeFrameRecord& fr)
    {
#if defined(THREAD_SAMPLER)
        CSystem* pSystem = static_cast<CSystem*>(gEnv->pSystem);
        CThreadProfiler* pThreadProf = pSystem->GetThreadProfiler();
        IThreadSampler* pThreadSampler = pThreadProf ? pThreadProf->GetThreadSampler() : NULL;

        if (pThreadSampler)
        {
            pThreadSampler->Tick();

            fr.AddValue(pThreadSampler->GetExecutionTime(TT_MAIN));
            fr.AddValue(pThreadSampler->GetExecutionTime(TT_RENDER));
            fr.AddValue(pThreadSampler->GetExecutionTime(TT_OTHER));
            fr.AddValue(pThreadSampler->GetExecutionTime(TT_SYSTEM_IDLE_0));
            fr.AddValue(pThreadSampler->GetExecutionTime(TT_SYSTEM_IDLE_1));
            fr.AddValue(pThreadSampler->GetExecutionTime(TT_SYSTEM_IDLE_0) + pThreadSampler->GetExecutionTime(TT_SYSTEM_IDLE_1));
            fr.AddValue(pThreadSampler->GetExecutionTime(TT_TOTAL) / pThreadSampler->GetNumHWThreads());
            fr.AddValue(pThreadSampler->GetExecutionTimeFrame());
        }
        else
#endif  // defined(THREAD_SAMPLER)
        {
            for (uint32 i = 0; i < 8; i++)
            {
                fr.AddValue(0.0f);
            }
        }
    }

    static void StartThreadProf()
    {
        CSystem* pSystem = static_cast<CSystem*>(gEnv->pSystem);
        CThreadProfiler* pThreadProf = pSystem->GetThreadProfiler();
        pThreadProf->Start();
    }

    static void StopThreadProf()
    {
        CSystem* pSystem = static_cast<CSystem*>(gEnv->pSystem);
        CThreadProfiler* pThreadProf = pSystem->GetThreadProfiler();
        pThreadProf->Stop();
    }
};

struct SCPUTimesDG
    : public IStatoscopeDataGroup
{
    virtual SDescription GetDescription() const
    {
        return SDescription('j', "CPU Times", "['/CPUTimes/' (float physTime) "
            "(float particleTime) (float particleSyncTime) (int particleNumEmitters) "
            "(float animTime) (int animNumCharacters) "
            "(float aiTime)]");
    }

    virtual void Write(IStatoscopeFrameRecord& fr)
    {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        double frequency = 1.0 / static_cast<double>(freq.QuadPart);
#define TICKS_TO_MS(t) ((float)((t) * 1000.0 * frequency))

        uint32 gUpdateTimeIdx = 0, gUpdateTimesNum = 0;
        const sUpdateTimes* gUpdateTimes = gEnv->pSystem->GetUpdateTimeStats(gUpdateTimeIdx, gUpdateTimesNum);
        float curPhysTime = TICKS_TO_MS(gUpdateTimes[gUpdateTimeIdx].PhysStepTime);
        fr.AddValue(curPhysTime);

        IParticleManager* pPartMan = gEnv->p3DEngine->GetParticleManager();
        if (pPartMan != NULL)
        {
            int nNumEmitter = (int)pPartMan->NumEmitter();
            float fTimeMS = TICKS_TO_MS(pPartMan->NumFrameTicks());
            float fTimeSyncMS = TICKS_TO_MS(pPartMan->NumFrameSyncTicks());

            fr.AddValue(fTimeMS);
            fr.AddValue(fTimeSyncMS);
            fr.AddValue(nNumEmitter);
        }
        else
        {
            fr.AddValue(0.0f);
            fr.AddValue(0.0f);
            fr.AddValue(0);
        }

        ICharacterManager* pCharManager = gEnv->pCharacterManager;
        if (pCharManager != NULL)
        {
            int nNumCharacters = (int)pCharManager->NumCharacters();
            float fTimeMS = TICKS_TO_MS(pCharManager->NumFrameTicks());

            fr.AddValue(fTimeMS);
            fr.AddValue(nNumCharacters);
        }
        else
        {
            fr.AddValue(0.0f);
            fr.AddValue(0);
        }

        IAISystem* pAISystem = gEnv->pAISystem;
        if (pAISystem != NULL)
        {
            float fTimeMS = TICKS_TO_MS(pAISystem->NumFrameTicks());
            fr.AddValue(fTimeMS);
        }
        else
        {
            fr.AddValue(0.0f);
        }
#undef TICKS_TO_MS
    }
};

struct SVertexCostDG
    : public IStatoscopeDataGroup
{
    virtual SDescription GetDescription() const
    {
        return SDescription('v', "Vertex data", "['/VertexData/' (int StaticPolyCountZ) (int SkinnedPolyCountZ) (int VegetationPolyCountZ)]");
    }

    virtual void Write(IStatoscopeFrameRecord& fr)
    {
        IRenderer* pRenderer = gEnv->pRenderer;

        int32 nPolyCountZ =  pRenderer->GetPolygonCountByType(EFSLIST_GENERAL, EVCT_STATIC, 1);
        nPolyCountZ +=  pRenderer->GetPolygonCountByType(EFSLIST_SHADOW_GEN, EVCT_STATIC, 1);
        nPolyCountZ +=  pRenderer->GetPolygonCountByType(EFSLIST_TRANSP, EVCT_STATIC, 1);
        nPolyCountZ +=  pRenderer->GetPolygonCountByType(EFSLIST_DECAL, EVCT_STATIC, 1);
        fr.AddValue(nPolyCountZ);

        nPolyCountZ = pRenderer->GetPolygonCountByType(EFSLIST_GENERAL, EVCT_SKINNED, 1);
        nPolyCountZ +=  pRenderer->GetPolygonCountByType(EFSLIST_SHADOW_GEN, EVCT_SKINNED, 1);
        nPolyCountZ +=  pRenderer->GetPolygonCountByType(EFSLIST_TRANSP, EVCT_SKINNED, 1);
        nPolyCountZ +=  pRenderer->GetPolygonCountByType(EFSLIST_DECAL, EVCT_SKINNED, 1);
        fr.AddValue(nPolyCountZ);

        nPolyCountZ = pRenderer->GetPolygonCountByType(EFSLIST_GENERAL, EVCT_VEGETATION, 1);
        nPolyCountZ +=  pRenderer->GetPolygonCountByType(EFSLIST_SHADOW_GEN, EVCT_VEGETATION, 1);
        nPolyCountZ +=  pRenderer->GetPolygonCountByType(EFSLIST_TRANSP, EVCT_VEGETATION, 1);
        nPolyCountZ +=  pRenderer->GetPolygonCountByType(EFSLIST_DECAL, EVCT_VEGETATION, 1);
        fr.AddValue(nPolyCountZ);
    }
};

struct SParticlesDG
    : public IStatoscopeDataGroup
{
    virtual SDescription GetDescription() const
    {
        return SDescription('p', "particles", "['/Particles/' (float numParticlesRendered) (float numParticlesActive) (float numParticlesAllocated) "
            "(float particleScreenFractionRendered) (float particleScreenFractionProcessed) "
            "(float numEmittersRendered) (float numEmittersActive) (float numEmittersAllocated) "
            "(float numParticlesReiterated) (float numParticlesRejected) "
            "(float numParticlesCollideTest) (float numParticlesCollideHit) (float numParticlesClipped)]");
    }

    virtual void Write(IStatoscopeFrameRecord& fr)
    {
        SParticleCounts particleCounts;
        gEnv->pParticleManager->GetCounts(particleCounts);

        float fScreenPix = (float)(gEnv->pRenderer->GetWidth() * gEnv->pRenderer->GetHeight());
        fr.AddValue(particleCounts.ParticlesRendered);
        fr.AddValue(particleCounts.ParticlesActive);
        fr.AddValue(particleCounts.ParticlesAlloc);
        fr.AddValue(particleCounts.PixelsRendered / fScreenPix);
        fr.AddValue(particleCounts.PixelsProcessed / fScreenPix);
        fr.AddValue(particleCounts.EmittersRendered);
        fr.AddValue(particleCounts.EmittersActive);
        fr.AddValue(particleCounts.EmittersAlloc);
        fr.AddValue(particleCounts.ParticlesReiterate);
        fr.AddValue(particleCounts.ParticlesReject);
        fr.AddValue(particleCounts.ParticlesCollideTest);
        fr.AddValue(particleCounts.ParticlesCollideHit);
        fr.AddValue(particleCounts.ParticlesClip);
    }
};

struct SLocationDG
    : public IStatoscopeDataGroup
{
    virtual SDescription GetDescription() const
    {
        return SDescription('l', "location", "['/' (float posx) (float posy) (float posz) (float rotx) (float roty) (float rotz)]");
    }

    virtual void Write(IStatoscopeFrameRecord& fr)
    {
        Matrix33 m = Matrix33(gEnv->pRenderer->GetCamera().GetMatrix());
        Vec3 pos = gEnv->pRenderer->GetCamera().GetPosition();
        Ang3 rot = RAD2DEG(Ang3::GetAnglesXYZ(m));

        fr.AddValue(pos.x);
        fr.AddValue(pos.y);
        fr.AddValue(pos.z);

        fr.AddValue(rot.x);
        fr.AddValue(rot.y);
        fr.AddValue(rot.z);
    }
};

struct SPerCGFGPUProfilersDG
    : public IStatoscopeDataGroup
{
    CSimpleStringPool m_cattedCGFNames;

    SPerCGFGPUProfilersDG()
    // Frame IDs are currently problematic and will be ommitted
    //: SDataGroup('c', "per-cgf gpu profilers", "['/DrawCalls/$' (int frameID) (int totalDrawCallCount) (int numInstances)]", 4)
        : m_cattedCGFNames(true)
    {}

    virtual SDescription GetDescription() const
    {
        // Frame IDs are currently problematic and will be ommitted
        //: SDataGroup('c', "per-cgf gpu profilers", "['/DrawCalls/$' (int frameID) (int totalDrawCallCount) (int numInstances)]", 4)
        return SDescription('c', "per-cgf gpu profilers", "['/DrawCalls/$' (int totalDrawCallCount) (int numInstances)]");
    }

    virtual void Write(IStatoscopeFrameRecord& fr)
    {
#if defined(_RELEASE)
        CryFatalError("Per-CGF GPU profilers not enabled in release");
#else
        IRenderer* pRenderer = gEnv->pRenderer;

        pRenderer->CollectDrawCallsInfo(true);

        IRenderer::RNDrawcallsMapMesh& drawCallsInfo = gEnv->pRenderer->GetDrawCallsInfoPerMesh();

        IRenderer::RNDrawcallsMapMeshItor pEnd = drawCallsInfo.end();
        IRenderer::RNDrawcallsMapMeshItor pItor = drawCallsInfo.begin();

        string sPathName;
        sPathName.reserve(64);

        //Per RenderNode Stats
        for (; pItor != pEnd; ++pItor)
        {
            IRenderer::SDrawCallCountInfo& drawInfo = pItor->second;

            const char* pRenderMeshName = drawInfo.meshName;
            const char* pNameShort = strrchr(pRenderMeshName, '/');

            if (pNameShort)
            {
                pRenderMeshName = pNameShort + 1;
            }

            if (drawInfo.nShadows > 0)
            {
                sPathName = "Shadow/";
                sPathName += drawInfo.typeName;
                sPathName += "/";
                sPathName += pRenderMeshName;
                sPathName += "/";
                fr.AddValue(m_cattedCGFNames.Append(sPathName.c_str(), sPathName.length()));
                //fr.AddValue( batchStat->nFrameID );
                fr.AddValue(drawInfo.nShadows);
                fr.AddValue(1);
            }
            if (drawInfo.nGeneral > 0)
            {
                sPathName = "Opaque/";
                sPathName += drawInfo.typeName;
                sPathName += "/";
                sPathName += pRenderMeshName;
                sPathName += "/";
                fr.AddValue(m_cattedCGFNames.Append(sPathName.c_str(), sPathName.length()));
                //fr.AddValue( batchStat->nFrameID );
                fr.AddValue(drawInfo.nGeneral);
                fr.AddValue(1);
            }
            if (drawInfo.nTransparent > 0)
            {
                sPathName = "Transparent/";
                sPathName += drawInfo.typeName;
                sPathName += "/";
                sPathName += pRenderMeshName;
                sPathName += "/";
                fr.AddValue(m_cattedCGFNames.Append(sPathName.c_str(), sPathName.length()));
                //fr.AddValue( batchStat->nFrameID );
                fr.AddValue(drawInfo.nTransparent);
                fr.AddValue(1);
            }
            if (drawInfo.nZpass > 0)
            {
                sPathName = "ZPass/";
                sPathName += drawInfo.typeName;
                sPathName += "/";
                sPathName += pRenderMeshName;
                sPathName += "/";
                fr.AddValue(m_cattedCGFNames.Append(sPathName.c_str(), sPathName.length()));
                //fr.AddValue( batchStat->nFrameID );
                fr.AddValue(drawInfo.nZpass);
                fr.AddValue(1);
            }
            if (drawInfo.nMisc > 0)
            {
                sPathName = "Misc/";
                sPathName += drawInfo.typeName;
                sPathName += "/";
                sPathName += pRenderMeshName;
                sPathName += "/";
                fr.AddValue(m_cattedCGFNames.Append(sPathName.c_str(), sPathName.length()));
                //fr.AddValue( batchStat->nFrameID );
                fr.AddValue(drawInfo.nMisc);
                fr.AddValue(1);
            }
        }
#endif
    }

    virtual uint32 PrepareToWrite()
    {
        uint32 drawProfilerCount = 0;
#if !defined(_RELEASE)
        IRenderer* pRenderer = gEnv->pRenderer;
        pRenderer->CollectDrawCallsInfo(true);
        IRenderer::RNDrawcallsMapMesh& drawCallsInfo = gEnv->pRenderer->GetDrawCallsInfoPerMesh();
        IRenderer::RNDrawcallsMapMeshItor pEnd = drawCallsInfo.end();
        IRenderer::RNDrawcallsMapMeshItor pItor = drawCallsInfo.begin();

        //Per RenderNode Stats
        for (; pItor != pEnd; ++pItor)
        {
            IRenderer::SDrawCallCountInfo& pInfo = pItor->second;

            if (pInfo.nShadows > 0)
            {
                drawProfilerCount++;
            }
            if (pInfo.nGeneral > 0)
            {
                drawProfilerCount++;
            }
            if (pInfo.nTransparent > 0)
            {
                drawProfilerCount++;
            }
            if (pInfo.nZpass > 0)
            {
                drawProfilerCount++;
            }
            if (pInfo.nMisc > 0)
            {
                drawProfilerCount++;
            }
        }
#endif
        return drawProfilerCount;
    }
};

struct SParticleProfilersDG
    : public IStatoscopeDataGroup
{
    SParticleProfilersDG()
        : m_particleInfos()
    {
    }

    void AddParticleInfo(const SParticleInfo& pi)
    {
        if (IsEnabled())
        {
            m_particleInfos.push_back(pi);
        }
    }

    virtual SDescription GetDescription() const
    {
        return SDescription('y', "ParticlesColliding", "['/ParticlesColliding/$/' (int count)]");
    }

    virtual void Disable()
    {
        IStatoscopeDataGroup::Disable();

        m_particleInfos.clear();
    }

    virtual void Write(IStatoscopeFrameRecord& fr)
    {
        for (uint32 i = 0; i < m_particleInfos.size(); i++)
        {
            SParticleInfo& particleInfo = m_particleInfos[i];

            const char* pEffectName = particleInfo.name.c_str();

            fr.AddValue(pEffectName);
            fr.AddValue(particleInfo.numParticles);
        }

        m_particleInfos.clear();
    }

    virtual uint32 PrepareToWrite()
    {
        return m_particleInfos.size();
    }

private:
    std::vector<SParticleInfo> m_particleInfos;
};

struct SPhysEntityProfilersDG
    : public IStatoscopeDataGroup
{
    virtual SDescription GetDescription() const
    {
        return SDescription('w', "PhysEntities", "['/PhysEntities/$/' (float time) (int nCalls) (float x) (float y) (float z)]");
    }

    virtual void Disable()
    {
        IStatoscopeDataGroup::Disable();

        m_physInfos.clear();
    }

    virtual uint32 PrepareToWrite()
    {
        return m_physInfos.size();
    }

    virtual void Write(IStatoscopeFrameRecord& fr)
    {
        //poke CVar in physics
        gEnv->pPhysicalWorld->GetPhysVars()->bProfileEntities = 2;

        for (uint32 i = 0; i < m_physInfos.size(); i++)
        {
            SPhysInfo& physInfo = m_physInfos[i];

            const char* pEntityName = physInfo.name.c_str();

            fr.AddValue(pEntityName);
            fr.AddValue(physInfo.time);
            fr.AddValue(physInfo.nCalls);
            fr.AddValue(physInfo.pos.x);
            fr.AddValue(physInfo.pos.y);
            fr.AddValue(physInfo.pos.z);
        }

        m_physInfos.clear();
    }

    std::vector<SPhysInfo> m_physInfos;
};

struct SFrameProfilersDG
    : public IStatoscopeDataGroup
{
    SFrameProfilersDG()
        : m_frameProfilerRecords()
    {
    }

    virtual SDescription GetDescription() const
    {
        return SDescription('r', "frame profilers", "['/Threads/$' (int count) (float selfTimeInMS) (float peak)]");
    }

    virtual void Enable()
    {
        IStatoscopeDataGroup::Enable();
        ICVar* pCV_profile = gEnv->pConsole->GetCVar("profile");
        if (pCV_profile)
        {
            pCV_profile->Set(-1);
        }
    }

    virtual void Disable()
    {
        IStatoscopeDataGroup::Disable();
        ICVar* pCV_profile = gEnv->pConsole->GetCVar("profile");
        if (pCV_profile)
        {
            pCV_profile->Set(0);
        }
    }

    virtual void Write(IStatoscopeFrameRecord& fr)
    {
        for (uint32 i = 0; i < m_frameProfilerRecords.size(); i++)
        {
            SPerfStatFrameProfilerRecord& fpr = m_frameProfilerRecords[i];
            string fpPath = GetFrameProfilerPath(fpr.m_pProfiler);
            fr.AddValue(fpPath.c_str());
            fr.AddValue(fpr.m_count);
            fr.AddValue(fpr.m_selfTime);
            fr.AddValue(fpr.m_peak);
        }

        m_frameProfilerRecords.clear();
    }

    virtual uint32 PrepareToWrite()
    {
        return m_frameProfilerRecords.size();
    }

    std::vector<SPerfStatFrameProfilerRecord> m_frameProfilerRecords;   // the most recent frame's profiler data
};

struct SPerfCountersDG
    : public IStatoscopeDataGroup
{
    virtual SDescription GetDescription() const
    {
        return SDescription('q', "performance counters", "['/PerfCounters/' (int lhsCount) (int iCacheMissCount)]");
    }

    virtual void Enable()
    {
        IStatoscopeDataGroup::Enable();
    }

    virtual void Disable()
    {
        IStatoscopeDataGroup::Disable();
    }

    virtual void Write(IStatoscopeFrameRecord& fr)
    {
        fr.AddValue(0);
        fr.AddValue(0);
    }
};

struct SUserMarkerDG
    : public IStatoscopeDataGroup
{
    virtual SDescription GetDescription() const
    {
        return SDescription('u', "user markers", "['/UserMarkers/$' (string name)]");
    }

    virtual void Disable()
    {
        IStatoscopeDataGroup::Disable();

        m_userMarkers.clear();
        m_tmpUserMarkers.clear();
    }

    virtual uint32 PrepareToWrite()
    {
        m_tmpUserMarkers.clear();
        m_userMarkers.swap(m_tmpUserMarkers);
        return m_tmpUserMarkers.size();
    }

    virtual void Write(IStatoscopeFrameRecord& fr)
    {
        for (uint32 i = 0; i < m_tmpUserMarkers.size(); i++)
        {
            fr.AddValue(m_tmpUserMarkers[i].m_path.c_str());
            fr.AddValue(m_tmpUserMarkers[i].m_name.c_str());
        }
    }

    std::vector<SUserMarker> m_tmpUserMarkers;
    CryMT::vector<SUserMarker> m_userMarkers;
};

struct SCallstacksDG
    : public IStatoscopeDataGroup
{
    CSimpleStringPool m_callstackAddressStrings;

    SCallstacksDG()
        : m_callstackAddressStrings(true)
    {}

    virtual SDescription GetDescription() const
    {
        return SDescription('k', "callstacks", "['/Callstacks/$' (string callstack)]");
    }

    virtual uint32 PrepareToWrite()
    {
        m_tmpCallstacks.clear();
        m_callstacks.swap(m_tmpCallstacks);
        return m_tmpCallstacks.size();
    }

    virtual void Write(IStatoscopeFrameRecord& fr)
    {
        for (uint32 i = 0; i < m_tmpCallstacks.size(); i++)
        {
            const SCallstack& callstack = m_tmpCallstacks[i];
            uint32 numAddresses = callstack.m_addresses.size();
            string callstackString;
            callstackString.reserve((numAddresses * 11) + 1); // see ptrStr

            for (uint32 j = 0; j < numAddresses; j++)
            {
                char ptrStr[12]; // 0x + 12345678 + " " + '\0'
                sprintf_s(ptrStr, sizeof(ptrStr), "0x%p ", callstack.m_addresses[j]);
                callstackString += ptrStr;
            }

            fr.AddValue(callstack.m_tag.c_str());
            fr.AddValue(m_callstackAddressStrings.Append(callstackString.c_str(), callstackString.length()));
        }
    }

    CryMT::vector<SCallstack> m_callstacks;
    std::vector<SCallstack> m_tmpCallstacks;
};

struct SNetworkDG
    : public IStatoscopeDataGroup
{
    virtual SDescription GetDescription() const
    {
        return SDescription('n', "network", "['/Network/' (int TotalBitsSent) (int RmiBitsSent) (int AspectBitsSent) (int LobbyBitsSent) (int FragmentedBitsSent) (int TotalBitsRecvd) (int TotalPacketsSent) (int LobbyPacketsSent) (int FragmentedPacketsSent) (int TotalPacketsDropped)  (float PercentPacketsDropped)]");
    }

    //this PREFAST_SUPPRESS_WARNING needs better investigation
    virtual void Write(IStatoscopeFrameRecord& fr) PREFAST_SUPPRESS_WARNING(6262)
    {
        if (gEnv->pNetwork)
        {
            SBandwidthStats bandwidthStats;
            gEnv->pNetwork->GetBandwidthStatistics(&bandwidthStats);

            memcpy(&bandwidthStats.m_prev, &m_prev, sizeof(SBandwidthStatsSubset));
            SBandwidthStatsSubset delta = bandwidthStats.TickDelta();
            memcpy(&m_prev, &bandwidthStats.m_total, sizeof(SBandwidthStatsSubset));

            fr.AddValue((int)delta.m_totalBandwidthSent);
            fr.AddValue((int)delta.m_rmiPayloadBitsSent);
            fr.AddValue((int)delta.m_aspectPayloadBitsSent);
            fr.AddValue((int)delta.m_lobbyBandwidthSent);
            fr.AddValue((int)delta.m_fragmentBandwidthSent);

            fr.AddValue((int)delta.m_totalBandwidthRecvd);

            fr.AddValue(delta.m_totalPacketsSent);
            fr.AddValue(delta.m_lobbyPacketsSent);
            fr.AddValue(delta.m_fragmentPacketsSent);

            fr.AddValue((int)delta.m_totalPacketsDropped);
            fr.AddValue((float)bandwidthStats.m_total.m_totalPacketsDropped / (float)bandwidthStats.m_total.m_totalPacketsSent);
        }
        else
        {
            fr.AddValue(0);
            fr.AddValue(0);
            fr.AddValue(0);
            fr.AddValue(0);
            fr.AddValue(0);

            fr.AddValue(0);

            fr.AddValue(0);
            fr.AddValue(0);
            fr.AddValue(0);

            fr.AddValue(0);
            fr.AddValue(0.0f);
        }
    }

    SBandwidthStatsSubset   m_prev;
};

struct SChannelDG
    : public IStatoscopeDataGroup
{
    virtual SDescription GetDescription() const
    {
        return SDescription('z', "channel", "['/Channel/$/' (float BytesSent) (float BytesRecvd) (float PacketRate) (int MaxPacketSize) (int IdealPacketSize) (int SparePacketSize) (int UsedPacketSize) (int SentMessages) (int UnsentMessages) (int Ping)]");
    }

    virtual void Write(IStatoscopeFrameRecord& fr)
    {
        if (gEnv->pNetwork)
        {
            for (uint32 index = 0; index < m_count; ++index)
            {
                fr.AddValue(m_bandwidthStats.m_channel[index].m_name);
                fr.AddValue(m_bandwidthStats.m_channel[index].m_bandwidthOutbound);
                fr.AddValue(m_bandwidthStats.m_channel[index].m_bandwidthInbound);
                fr.AddValue(m_bandwidthStats.m_channel[index].m_currentPacketRate);
                fr.AddValue((int)m_bandwidthStats.m_channel[index].m_maxPacketSize);
                fr.AddValue((int)m_bandwidthStats.m_channel[index].m_idealPacketSize);
                fr.AddValue((int)m_bandwidthStats.m_channel[index].m_sparePacketSize);
                fr.AddValue((int)m_bandwidthStats.m_channel[index].m_messageQueue.m_usedPacketSize);
                fr.AddValue(m_bandwidthStats.m_channel[index].m_messageQueue.m_sentMessages);
                fr.AddValue(m_bandwidthStats.m_channel[index].m_messageQueue.m_unsentMessages);
                fr.AddValue((int)m_bandwidthStats.m_channel[index].m_ping);
            }
        }
    }

    virtual uint32 PrepareToWrite()
    {
        m_count = 0;
        if (gEnv->pNetwork)
        {
            gEnv->pNetwork->GetBandwidthStatistics(&m_bandwidthStats);


            for (uint32 index = 0; index < STATS_MAX_NUMBER_OF_CHANNELS; ++index)
            {
                if (m_bandwidthStats.m_channel[index].m_inUse)
                {
                    ++m_count;
                }
            }
        }

        return m_count;
    }

protected:
    SBandwidthStats m_bandwidthStats;
    uint32 m_count;
};

struct SNetworkProfileDG
    : public IStatoscopeDataGroup
{
    virtual SDescription GetDescription() const
    {
        return SDescription('d', "network profile", "['/NetworkProfile/$' (int totalBits) (int seqBits) (int rmiBits) (int calls)]");
    }

    virtual void Write(IStatoscopeFrameRecord& fr)
    {
        for (uint32 i = 0; i < m_statsCache.size(); i++)
        {
            SProfileInfoStat& nps = m_statsCache[i];
            fr.AddValue(nps.m_name.c_str());
            fr.AddValue((int)nps.m_totalBits); //-- as bits, not Kbits, so we can graph against the bandwidth stats on a comparable scale.
            fr.AddValue((int)(nps.m_rmi ? 0 : nps.m_totalBits));
            fr.AddValue((int)(nps.m_rmi ? nps.m_totalBits : 0));
            fr.AddValue((int)nps.m_calls);
        }
    }

    virtual uint32 PrepareToWrite()
    {
        m_statsCache.clear();

        if (gEnv->pNetwork)
        {
            SNetworkProfilingStats profileStats;
            gEnv->pNetwork->GetProfilingStatistics(&profileStats);

            for (int32 i = 0; i < profileStats.m_ProfileInfoStats.size(); i++)
            {
                SProfileInfoStat& nps = profileStats.m_ProfileInfoStats[i];
                if (nps.m_totalBits)
                {
                    m_statsCache.push_back(nps);
                }
            }
        }

        return m_statsCache.size();
    }

    std::vector<SProfileInfoStat> m_statsCache;
};

class CStreamingObjectIntervalGroup
    : public CStatoscopeIntervalGroup
    , public IStreamedObjectListener
{
public:
    enum
    {
        Stage_Unloaded,
        Stage_Requested,
        Stage_LoadedUnused,
        Stage_LoadedUsed,
    };

public:
    CStreamingObjectIntervalGroup()
        : CStatoscopeIntervalGroup('o', "streaming objects",
            "['/Objects/' "
            "(string filename) "
            "(int stage) "
            "]")
    {
    }

    void Enable_Impl()
    {
        gEnv->p3DEngine->SetStreamableListener(this);
    }

    void Disable_Impl()
    {
        gEnv->p3DEngine->SetStreamableListener(NULL);
    }

    void WriteChangeStageBlockEvent(CStatoscopeEventWriter* pWriter, const void* pHandle, int stage)
    {
        size_t payloadLen = GetValueLength(stage);
        DataWriter::EventModifyInterval* pEv = pWriter->BeginBlockEvent<DataWriter::EventModifyInterval>(payloadLen);
        pEv->id = reinterpret_cast<UINT_PTR>(pHandle);
        pEv->classId = GetId();
        pEv->field = DataWriter::EventModifyInterval::FieldSplitIntervalMask | 1;

        char* pPayload = (char*)(pEv + 1);
        WriteValue(pPayload, stage);

        pWriter->EndBlockEvent();
    }

    void WriteChangeStageEvent(const void* pHandle, int stage)
    {
        CStatoscopeEventWriter* pWriter = GetWriter();

        if (pWriter)
        {
            pWriter->BeginBlock();
            WriteChangeStageBlockEvent(pWriter, pHandle, stage);
            pWriter->EndBlock();
        }
    }

public: // IStreamedObjectListener Members
    virtual void OnCreatedStreamedObject(const char* filename, void* pHandle)
    {
        CStatoscopeEventWriter* pWriter = GetWriter();

        if (pWriter)
        {
            size_t payloadLen =
                GetValueLength(filename) +
                GetValueLength(0)
            ;

            DataWriter::EventBeginInterval* pEv = pWriter->BeginEvent<DataWriter::EventBeginInterval>(payloadLen);
            pEv->id = reinterpret_cast<UINT_PTR>(pHandle);
            pEv->classId = GetId();

            char* pPayload = (char*)(pEv + 1);
            WriteValue(pPayload, filename);
            WriteValue(pPayload, Stage_Unloaded);

            pWriter->EndEvent();
        }
    }

    virtual void OnRequestedStreamedObject(void* pHandle)
    {
        WriteChangeStageEvent(pHandle, Stage_Requested);
    }

    virtual void OnReceivedStreamedObject(void* pHandle)
    {
        WriteChangeStageEvent(pHandle, Stage_LoadedUnused);
    }

    virtual void OnUnloadedStreamedObject(void* pHandle)
    {
        WriteChangeStageEvent(pHandle, Stage_Unloaded);
    }

    void OnBegunUsingStreamedObjects(void** pHandles, size_t numHandles)
    {
        CStatoscopeEventWriter* pWriter = GetWriter();

        if (pWriter)
        {
            pWriter->BeginBlock();

            for (size_t i = 0; i < numHandles; ++i)
            {
                WriteChangeStageBlockEvent(pWriter, pHandles[i], Stage_LoadedUsed);
            }

            pWriter->EndBlock();
        }
    }

    void OnEndedUsingStreamedObjects(void** pHandles, size_t numHandles)
    {
        CStatoscopeEventWriter* pWriter = GetWriter();

        if (pWriter)
        {
            pWriter->BeginBlock();

            for (size_t i = 0; i < numHandles; ++i)
            {
                WriteChangeStageBlockEvent(pWriter, pHandles[i], Stage_LoadedUnused);
            }

            pWriter->EndBlock();
        }
    }

    virtual void OnDestroyedStreamedObject(void* pHandle)
    {
        CStatoscopeEventWriter* pWriter = GetWriter();

        if (pWriter)
        {
            DataWriter::EventEndInterval* pEv = pWriter->BeginEvent<DataWriter::EventEndInterval>();
            pEv->id = reinterpret_cast<UINT_PTR>(pHandle);
            pWriter->EndEvent();
        }
    }
};

CStatoscopeEventWriter::CStatoscopeEventWriter()
    : m_eventNextSequence(0)
    , m_lastTimestampUs(0)
{
}

void CStatoscopeEventWriter::Flush(CDataWriter* pWriter)
{
    {
        using std::swap;
        CryAutoLock<CryCriticalSectionNonRecursive> lock(m_eventStreamLock);
        swap(m_eventStream, m_eventStreamTmp);
    }

    uint32 size = (uint32)m_eventStreamTmp.size();
    pWriter->WriteData(size);
    if (size)
    {
        pWriter->WriteData(&m_eventStreamTmp[0], size);
    }

#ifndef _RELEASE
    if (!m_eventStreamTmp.empty())
    {
        using namespace DataWriter;

        char* pBuff = (char*)&m_eventStreamTmp[0];
        char* pBuffEnd = pBuff + size;
        EventHeader* pHdr = (EventHeader*)pBuff;
        uint64 firstTimestampUs = pHdr->timeStampUs;

        pBuff += pHdr->eventLengthInWords * 4;
        while (pBuff < pBuffEnd)
        {
            pHdr = (EventHeader*)pBuff;

            if (pHdr->timeStampUs < firstTimestampUs)
            {
                __debugbreak();
            }
            firstTimestampUs = pHdr->timeStampUs;

            pBuff += pHdr->eventLengthInWords * 4;
        }
    }
#endif

    m_eventStreamTmp.clear();
}

void CStatoscopeEventWriter::Reset()
{
    CryAutoLock<CryCriticalSectionNonRecursive> lock(m_eventStreamLock);
    stl::free_container(m_eventStream);
    stl::free_container(m_eventStreamTmp);
    m_eventNextSequence = 0;
}

CStatoscope::CStatoscope()
{
    m_lastDumpTime = 0.f;
    m_screenshotLastCaptureTime = 0.f;
    m_activeDataGroupMask = 0;
    m_activeIvDataGroupMask = 0;
    m_groupMaskInitialized = false;

    m_pScreenShotBuffer = NULL;
    m_ScreenShotState = eSSCS_Idle;

    m_bLevelStarted = false;

    RegisterBuiltInDataGroups();
    RegisterBuiltInIvDataGroups();

    m_pStatoscopeEnabledCVar = REGISTER_INT("e_StatoscopeEnabled", 0, VF_NULL, "Controls whether all statoscope is enabled.");
    m_pStatoscopePortCVar = REGISTER_INT("e_StatoscopePort", default_statoscope_port, VF_NULL, "Controls which port the statoscope uses to communicate.");
    m_pStatoscopeDumpAllCVar = REGISTER_INT("e_StatoscopeDumpAll", 0, VF_NULL, "Controls whether all functions are dumped in a profile log.");
    m_pStatoscopeDataGroupsCVar = REGISTER_INT64("e_StatoscopeDataGroups", AlphaBits64("fgmrtuO"), VF_BITFIELD, GetDataGroupsCVarHelpString(m_allDataGroups));
    m_pStatoscopeIvDataGroupsCVar = REGISTER_INT64("e_StatoscopeIvDataGroups", m_activeIvDataGroupMask, VF_BITFIELD, GetDataGroupsCVarHelpString(m_intervalGroups));
    m_pStatoscopeLogDestinationCVar = REGISTER_INT_CB("e_StatoscopeLogDestination", eLD_Socket, VF_NULL, "Where the Statoscope log gets written to:\n  0 - file\n  1 - socket\n  2 - telemetry server (default)", OnLogDestinationCVarChange);  // see ELogDestination
    m_pStatoscopeScreenshotCapturePeriodCVar = REGISTER_FLOAT("e_StatoscopeScreenshotCapturePeriod", -1.0f, VF_NULL, "How many seconds between Statoscope screenshot captures (-1 to disable).");
    m_pStatoscopeFilenameUseBuildInfoCVar = REGISTER_INT("e_StatoscopeFilenameUseBuildInfo", 1, VF_NULL, "Set to include the platform and build number in the log filename.");
    m_pStatoscopeFilenameUseMapCVar = REGISTER_INT("e_StatoscopeFilenameUseMap", 0, VF_NULL, "Set to include the map name in the log filename.");
    m_pStatoscopeFilenameUseTagCvar = REGISTER_STRING("e_StatoscopeFilenameUseTag", "", VF_NULL, "Set to include tag in the log file name.");
    m_pStatoscopeFilenameUseTimeCVar = REGISTER_INT("e_StatoscopeFilenameUseTime", 0, VF_NULL, "Set to include the time and date in the log filename.");
    m_pStatoscopeFilenameUseDatagroupsCVar = REGISTER_INT("e_StatoscopeFilenameUseDatagroups", 0, VF_NULL, "Set to include the datagroup and date in the log filename.");
    m_pStatoscopeMinFuncLengthMsCVar = REGISTER_FLOAT("e_StatoscopeMinFuncLengthMs", 0.01f, VF_NULL, "Min func duration (ms) to be logged by statoscope.");
    m_pStatoscopeMaxNumFuncsPerFrameCVar = REGISTER_INT("e_StatoscopeMaxNumFuncsPerFrame", 150, VF_NULL, "Max number of funcs to log per frame.");
    m_pStatoscopeCreateLogFilePerLevelCVar = REGISTER_INT("e_StatoscopeCreateLogFilePerLevel", 0, VF_NULL, "Create a new perflog file per level.");
    m_pStatoscopeWriteTimeout = REGISTER_FLOAT("e_StatoscopeWriteTimeout", 1.0f, VF_NULL, "The number of seconds the data writer will stall before it gives up trying to write data (currently only applies to the telemetry data writer).");
    m_pStatoscopeConnectTimeout = REGISTER_FLOAT("e_StatoscopeConnectTimeout", 5.0f, VF_NULL, "The number of seconds the data writer will stall while trying connect to the telemetry server.");
    m_pStatoscopeAllowFPSOverrideCVar = REGISTER_INT("e_StatoscopeAllowFpsOverride", 1, VF_NULL, "Allow overriding of cvars in release for fps captures (MP only).");
    m_pGameRulesCVar = NULL;

    m_logNum = 1;

    REGISTER_COMMAND("e_StatoscopeAddUserMarker", &ConsoleAddUserMarker, 0, "Add a user marker to the perf stat logging for this frame");

    gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this);

    m_pServer = new CStatoscopeServer(this);

    m_pDataWriter = NULL;
}

CStatoscope::~CStatoscope()
{
    gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
    gEnv->pRenderer->UnRegisterCaptureFrame(this);
    delete[] m_pScreenShotBuffer;
    m_pScreenShotBuffer = NULL;

    SAFE_DELETE(m_pDataWriter);
    SAFE_DELETE(m_pServer);
}

static char* Base64Encode(const uint8* buffer, int len)
{
    FRAME_PROFILER("CStatoscope::Base64Encode", gEnv->pSystem, PROFILE_SYSTEM);

    static const char base64Dict[64] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
        'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
        'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
    };
    int b64Len = ((len + 2) / 3) * 4;
    char* b64Buf = new char[b64Len + 1];
    int byteCount = 0;
    int cycle = 0;

    for (int i = 0; i < b64Len; i++)
    {
        uint8 val = 0;

        switch (cycle)
        {
        case 0:
            val = buffer[byteCount] >> 2;
            break;
        case 1:
            val = (buffer[byteCount++] & 0x3) << 4;
            val |= buffer[byteCount] >> 4;
            break;
        case 2:
            val = (buffer[byteCount++] & 0xf) << 2;
            val |= buffer[byteCount] >> 6;
            break;
        case 3:
            val = buffer[byteCount++] & 0x3f;
            break;
        }

        cycle = (cycle + 1) & 3;

        b64Buf[i] = base64Dict[val];
    }

    b64Buf[b64Len] = '\0';

    return b64Buf;
}

bool CStatoscope::RegisterDataGroup(IStatoscopeDataGroup* pDG)
{
    IStatoscopeDataGroup::SDescription desc = pDG->GetDescription();

    for (std::vector<CStatoscopeDataGroup*>::iterator it = m_allDataGroups.begin(), itEnd = m_allDataGroups.end(); it != itEnd; ++it)
    {
        CStatoscopeDataGroup* pGroup = *it;

        if (pGroup->GetId() == desc.key)
        {
            return false;
        }
    }

    CStatoscopeDataGroup* pNewGroup = new CStatoscopeDataGroup(desc, pDG);
    m_allDataGroups.push_back(pNewGroup);

    return true;
}

void CStatoscope::UnregisterDataGroup(IStatoscopeDataGroup* pDG)
{
    for (std::vector<CStatoscopeDataGroup*>::iterator it = m_activeDataGroups.begin(), itEnd = m_activeDataGroups.end(); it != itEnd; ++it)
    {
        CStatoscopeDataGroup* pGroup = *it;
        IStatoscopeDataGroup* pCallback = pGroup->GetCallback();

        if (pCallback == pDG)
        {
            pDG->Disable();
            m_activeDataGroups.erase(it);
            break;
        }
    }

    for (std::vector<CStatoscopeDataGroup*>::iterator it = m_allDataGroups.begin(), itEnd = m_allDataGroups.end(); it != itEnd; ++it)
    {
        CStatoscopeDataGroup* pGroup = *it;
        IStatoscopeDataGroup* pCallback = pGroup->GetCallback();

        if (pCallback == pDG)
        {
            delete *it;
            m_allDataGroups.erase(it);
            break;
        }
    }
}

void CStatoscope::Tick()
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_SYSTEM);

    if (m_pStatoscopeEnabledCVar->GetIVal() != 0)
    {
        CreateDataWriter();

        if (m_pDataWriter && m_pDataWriter->Open())
        {
            SetIsRunning(true);

            uint64 currentActiveDataGroupMask = (uint64)m_pStatoscopeDataGroupsCVar->GetI64Val();
            uint64 differentDGs = currentActiveDataGroupMask ^ m_activeDataGroupMask;

            if (differentDGs)
            {
                uint64 enabledDGs = differentDGs & currentActiveDataGroupMask;
                uint64 disabledDGs = differentDGs & ~currentActiveDataGroupMask;
                SetDataGroups(enabledDGs, disabledDGs);

                m_activeDataGroupMask = currentActiveDataGroupMask;
                m_groupMaskInitialized = true;
            }

            AddFrameRecord(differentDGs != 0);
        }
    }
    else if (IsRunning())
    {
        CryLogAlways("Flushing Statoscope log\n");
        m_pDataWriter->Close();

        for (IntervalGroupVec::const_iterator it = m_intervalGroups.begin(), itEnd = m_intervalGroups.end(); it != itEnd; ++it)
        {
            (*it)->Disable();
        }

        for (std::vector<CStatoscopeDataGroup*>::iterator it = m_activeDataGroups.begin(), itEnd = m_activeDataGroups.end(); it != itEnd; ++it)
        {
            (*it)->GetCallback()->Disable();
        }

        m_activeDataGroups.clear();
        m_activeDataGroupMask = 0;

        m_eventWriter.Reset();

        SetIsRunning(false);
    }
}

void CStatoscope::SetCurrentProfilerRecords(const std::vector<CFrameProfiler*>* profilers)
{
    if (m_pFrameProfilers)
    {
        // we want to avoid reallocation of m_perfStatDumpProfilers
        // even if numProfilers is quite large (in the thousands), it'll only be tens of KB
        uint32 numProfilers = profilers->size();
        m_perfStatDumpProfilers.reserve(MAX(numProfilers, m_perfStatDumpProfilers.size()));
        m_perfStatDumpProfilers.resize(0);

        float minFuncTime = m_pStatoscopeMinFuncLengthMsCVar->GetFVal();

        int64 smallFuncs = 0;
        uint32 smallFuncsCount = 0;

        for (uint32 i = 0; i < numProfilers; i++)
        {
            CFrameProfiler* pProfiler = (*profilers)[i];

            // ignore really quick functions or ones what weren't called
            if (1000.f * gEnv->pTimer->TicksToSeconds(pProfiler->m_selfTime) > minFuncTime)
            {
                m_perfStatDumpProfilers.push_back(std::make_pair(pProfiler, pProfiler->m_selfTime));
            }
            else
            {
                smallFuncs += pProfiler->m_selfTime;
                smallFuncsCount++;
            }
        }

        std::sort(m_perfStatDumpProfilers.begin(), m_perfStatDumpProfilers.end(), CCompareFrameProfilersSelfTime());

        bool bDumpAll = false;

        if (m_pStatoscopeDumpAllCVar->GetIVal())
        {
            bDumpAll = true;
        }

        if (!bDumpAll)
        {
            uint32 maxNumFuncs = (uint32)m_pStatoscopeMaxNumFuncsPerFrameCVar->GetIVal();
            // limit the number being recorded
            m_perfStatDumpProfilers.resize(MIN(m_perfStatDumpProfilers.size(), maxNumFuncs));
        }

        uint32 numDumpProfilers = m_perfStatDumpProfilers.size();
        std::vector<SPerfStatFrameProfilerRecord>& records = m_pFrameProfilers->m_frameProfilerRecords;

        records.reserve(numDumpProfilers);
        records.resize(0);

        for (uint32 i = 0; i < numDumpProfilers; i++)
        {
            CFrameProfiler* pProfiler = m_perfStatDumpProfilers[i].first;
            int64 selfTime = m_perfStatDumpProfilers[i].second;
            SPerfStatFrameProfilerRecord profilerRecord;

            profilerRecord.m_pProfiler = pProfiler;
            profilerRecord.m_count = pProfiler->m_count;
            profilerRecord.m_selfTime = 1000.f * gEnv->pTimer->TicksToSeconds(selfTime);
            profilerRecord.m_variance = pProfiler->m_variance;
            profilerRecord.m_peak = 1000.f * gEnv->pTimer->TicksToSeconds(pProfiler->m_peak);

            records.push_back(profilerRecord);
        }

        if (bDumpAll)
        {
            SPerfStatFrameProfilerRecord profilerRecord;

            profilerRecord.m_pProfiler = NULL;
            profilerRecord.m_count = smallFuncsCount;
            profilerRecord.m_selfTime = 1000.f * gEnv->pTimer->TicksToSeconds(smallFuncs);
            profilerRecord.m_variance = 0;

            records.push_back(profilerRecord);
        }
    }
}

void CStatoscope::AddParticleEffect(const char* pEffectName, int count)
{
    if (m_pParticleProfilers)
    {
        SParticleInfo p = {pEffectName, count};
        m_pParticleProfilers->AddParticleInfo(p);
    }
}

void CStatoscope::AddPhysEntity(const phys_profile_info* pInfo)
{
    if (m_pPhysEntityProfilers && m_pPhysEntityProfilers->IsEnabled())
    {
        float dt = 1000.f * gEnv->pTimer->TicksToSeconds(pInfo->nTicksLast * 1024);

        if (dt > 0.f)
        {
            static const char* peTypes[] =
            {
                "None",                 //PE_NONE=0,
                "Static",               //PE_STATIC=1,
                "Rigid",                //PE_RIGID=2,
                "Wheeled",          //PE_WHEELEDVEHICLE=3,
                "Living",               //PE_LIVING=4,
                "Particle",         //PE_PARTICLE=5,
                "Articulated",  //PE_ARTICULATED=6,
                "Rope",                 //PE_ROPE=7,
                "Soft",                 //PE_SOFT=8,
                "Area"                  //PE_AREA=9
            };

            pe_type type = pInfo->pEntity->GetType();

            int numElems = sizeof(peTypes) / sizeof(char*);

            if (type >= numElems)
            {
                CryFatalError("peTypes enum has changed, please update statoscope CStatoscope::AddPhysEntity");
            }

            const char* pEntName = pInfo->pName;

            IRenderNode* pRenderNode = (IRenderNode*)pInfo->pEntity->GetForeignData(PHYS_FOREIGN_ID_STATIC);

            if (pRenderNode)
            {
                pEntName = pRenderNode->GetName();
            }

            //extra '/'s will break the log output
            const char* pLastSlash = strrchr(pEntName, '/');
            pEntName = pLastSlash ? pLastSlash + 1 : pEntName;

            string name;
            assert(type < 10);
            PREFAST_ASSUME(type < 10);
            name.Format("%s/%s", peTypes[type], pEntName);

            pe_status_pos status_pos;
            status_pos.pos = Vec3(0.f, 0.f, 0.f);

            pInfo->pEntity->GetStatus(&status_pos);

            SPhysInfo p = {name.c_str(), dt, pInfo->nCalls, status_pos.pos};
            m_pPhysEntityProfilers->m_physInfos.push_back(p);
        }
    }
}

void CStatoscope::CreateTelemetryStream(const char* postHeader, const char* hostname, int port)
{
    SAFE_DELETE(m_pDataWriter);
    if (postHeader)
    {
        m_pDataWriter = new CTelemetryDataWriter(postHeader, hostname, port, m_pStatoscopeWriteTimeout->GetFVal(), m_pStatoscopeConnectTimeout->GetFVal());
    }
}

void CStatoscope::CloseTelemetryStream()
{
    SAFE_DELETE(m_pDataWriter);
}

void CStatoscope::PrepareScreenShot()
{
    static int screenWidth = gEnv->pRenderer->GetWidth();
    static int screenHeight = gEnv->pRenderer->GetHeight();

    const int widthBefore = screenWidth;
    const int heightBefore = screenHeight;
    screenWidth = gEnv->pRenderer->GetWidth();
    screenHeight = gEnv->pRenderer->GetHeight();
    const int widthDelta = widthBefore - screenWidth;
    const int heightDelta = heightBefore - screenHeight;

    const int shrunkenWidthNotAligned = screenWidth / SCREENSHOT_SCALING_FACTOR;
    const int shrunkenWidth = shrunkenWidthNotAligned - shrunkenWidthNotAligned % 4;
    const int shrunkenHeight = screenHeight / SCREENSHOT_SCALING_FACTOR;

    if (!m_pScreenShotBuffer || widthDelta != 0 || heightDelta != 0)
    {
        const int SCREENSHOT_BIT_DEPTH = 4;
        SAFE_DELETE_ARRAY(m_pScreenShotBuffer);

        m_pScreenShotBuffer = new uint8[ shrunkenWidth * shrunkenHeight * SCREENSHOT_BIT_DEPTH ];
        memset((void*)m_pScreenShotBuffer, 0, (sizeof(uint8) * shrunkenWidth * shrunkenHeight * SCREENSHOT_BIT_DEPTH));
        gEnv->pRenderer->RegisterCaptureFrame(this);
    }
}

uint8* CStatoscope::ProcessScreenShot()
{
    //Reserved bytes in buffer indicate size and scale
    enum
    {
        SCREENSHOT_SCALED_WIDTH, SCREENSHOT_SCALED_HEIGHT, SCREENSHOT_MULTIPLIER
    };
    uint8* pScreenshotBuf = NULL;

    if (m_ScreenShotState == eSSCS_DataReceived)
    {
        const int SCREENSHOT_BIT_DEPTH = 4;
        const int SCREENSHOT_TARGET_BIT_DEPTH = 3;
        const int shrunkenWidthNotAligned = gEnv->pRenderer->GetWidth() / SCREENSHOT_SCALING_FACTOR;
        const int shrunkenWidth = shrunkenWidthNotAligned - shrunkenWidthNotAligned % 4;
        const int shrunkenHeight = gEnv->pRenderer->GetHeight() / SCREENSHOT_SCALING_FACTOR;

        pScreenshotBuf = new uint8[ 3 + (shrunkenWidth * shrunkenHeight * SCREENSHOT_TARGET_BIT_DEPTH) ];

        if (pScreenshotBuf)
        {
            pScreenshotBuf[ SCREENSHOT_MULTIPLIER ] = ( uint8 )((max(shrunkenWidth, shrunkenHeight) + UCHAR_MAX) / UCHAR_MAX);          //Scaling factor
            pScreenshotBuf[ SCREENSHOT_SCALED_WIDTH ] = ( uint8 )(shrunkenWidth / pScreenshotBuf[ SCREENSHOT_MULTIPLIER ]);
            pScreenshotBuf[ SCREENSHOT_SCALED_HEIGHT ] = ( uint8 )(shrunkenHeight / pScreenshotBuf[ SCREENSHOT_MULTIPLIER ]);
            int iSrcPixel = 0;
            int iDstPixel = 3;

            while (iSrcPixel < shrunkenWidth * shrunkenHeight * SCREENSHOT_BIT_DEPTH)
            {
                pScreenshotBuf[ iDstPixel + 0 ] = m_pScreenShotBuffer[ iSrcPixel++ ];
                pScreenshotBuf[ iDstPixel + 1 ] = m_pScreenShotBuffer[ iSrcPixel++ ];
                pScreenshotBuf[ iDstPixel + 2 ] = m_pScreenShotBuffer[ iSrcPixel++ ];

                iSrcPixel++;
                iDstPixel += SCREENSHOT_TARGET_BIT_DEPTH;
            }

            m_ScreenShotState = eSSCS_Idle;
        }
    }

    return pScreenshotBuf;
}

void CStatoscope::AddFrameRecord(bool bOutputHeader)
{
    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_SYSTEM);

    float currentTime = gEnv->pTimer->GetAsyncTime().GetSeconds();

    CStatoscopeFrameRecordWriter fr(m_pDataWriter);

    uint8* pScreenshot = NULL;

    //Screen shot in progress, attempt to process
    if (m_ScreenShotState == eSSCS_DataReceived)
    {
        pScreenshot = ProcessScreenShot();
    }

    //auto screen shot logic
    float screenshotCapturePeriod = m_pStatoscopeScreenshotCapturePeriodCVar->GetFVal();

    if ((m_ScreenShotState == eSSCS_Idle) && (screenshotCapturePeriod >= 0.0f))
    {
        if (currentTime >= m_screenshotLastCaptureTime + screenshotCapturePeriod)
        {
            //Tell the Render thread to dump the mini screenshot to main memory
            //Then wait for the callback to tell us the data is ready
            RequestScreenShot();
            m_screenshotLastCaptureTime = currentTime;
        }
    }

    if (m_pDataWriter->m_bShouldOutputLogTopHeader)
    {
#if defined(NEED_ENDIAN_SWAP)
        m_pDataWriter->WriteData((char)DataWriter::EE_BigEndian);
#else
        m_pDataWriter->WriteData((char)DataWriter::EE_LittleEndian);
#endif

        m_pDataWriter->WriteData(STATOSCOPE_BINARY_VERSION);

        //Using string pool?
        m_pDataWriter->WriteData(m_pDataWriter->IsUsingStringPool());

        for (IntervalGroupVec::const_iterator it = m_intervalGroups.begin(), itEnd = m_intervalGroups.end(); it != itEnd; ++it)
        {
            (*it)->Disable();
        }

        m_eventWriter.Reset();
        WriteIntervalClassEvents();

        uint64 ivGroups = m_pStatoscopeIvDataGroupsCVar->GetI64Val();

        for (IntervalGroupVec::const_iterator it = m_intervalGroups.begin(), itEnd = m_intervalGroups.end(); it != itEnd; ++it)
        {
            if (AlphaBit64((*it)->GetId()) & ivGroups)
            {
                (*it)->Enable(&m_eventWriter);
            }
        }

        bOutputHeader = true;
        m_pDataWriter->m_bShouldOutputLogTopHeader = false;
    }

    if (bOutputHeader)
    {
        m_pDataWriter->WriteData(true);

        //Module info
        if (m_activeDataGroupMask & AlphaBit64('k') && !m_pDataWriter->m_bHaveOutputModuleInformation)
        {
            m_pDataWriter->WriteData(true);
            OutputLoadedModuleInformation(m_pDataWriter);
        }
        else
        {
            m_pDataWriter->WriteData(false);
        }

        m_pDataWriter->WriteData((int)m_activeDataGroups.size());

        for (uint32 i = 0; i < m_activeDataGroups.size(); i++)
        {
            CStatoscopeDataGroup* dataGroup = m_activeDataGroups[i];
            dataGroup->WriteHeader(m_pDataWriter);
        }
    }
    else
    {
        m_pDataWriter->WriteData(false);
    }

    //
    // 1. Frame time
    //
    m_pDataWriter->WriteData(currentTime);

    //
    // 2. Screen shot
    //
    if (pScreenshot)
    {
        m_pDataWriter->WriteData(DataWriter::B64Texture);
        int screenShotSize = 3 + ((pScreenshot[0] * pScreenshot[2]) * (pScreenshot[1] * pScreenshot[2]) * 3);   // width,height,scale + (width*scale * height*scale * 3bpp)
        m_pDataWriter->WriteData(screenShotSize);
        m_pDataWriter->WriteData(pScreenshot, screenShotSize);
        SAFE_DELETE_ARRAY(pScreenshot);
    }
    else
    {
        m_pDataWriter->WriteData(DataWriter::None);
    }

    //
    // 3. Data groups
    //
    for (uint32 i = 0; i < m_activeDataGroups.size(); i++)
    {
        CStatoscopeDataGroup& dataGroup = *m_activeDataGroups[i];
        IStatoscopeDataGroup* pCallback = dataGroup.GetCallback();

        //output how many data sets to expect
        int nDataSets = pCallback->PrepareToWrite();
        m_pDataWriter->WriteData(nDataSets);

        fr.ResetWrittenElementCount();
        pCallback->Write(fr);

        int nElementsWritten = fr.GetWrittenElementCount();
        int nExpectedElems = dataGroup.GetNumElements() * nDataSets;

        if (nExpectedElems != nElementsWritten)
        {
            CryFatalError("Statoscope data group: %s is broken. Check data group declaration", dataGroup.GetName());
        }
    }

    //
    // 4. Events
    //
    m_eventWriter.Flush(m_pDataWriter);

    // 5. Magic Number, indicate end of frame record
    m_pDataWriter->WriteData(0xdeadbeef);
}

void CStatoscope::Flush()
{
    if (m_pDataWriter)
    {
        m_pDataWriter->Flush();
    }
}

bool CStatoscope::RequiresParticleStats(bool& bEffectStats)
{
    bool bGlobalStats = (m_activeDataGroupMask & AlphaBit64('p')) != 0;
    bool bCollisionStats = (m_activeDataGroupMask & AlphaBit64('y')) != 0;

    bool bRequiresParticleStats = false;
    bEffectStats = false;

    if (bCollisionStats)
    {
        bEffectStats = true;
        bRequiresParticleStats = true;
    }
    else if (bGlobalStats)
    {
        bRequiresParticleStats = true;
    }

    return bRequiresParticleStats;
}

void CStatoscope::SetLogFilename()
{
    m_logFilename = "@log@/statoscope/perf";

    if (m_pStatoscopeFilenameUseBuildInfoCVar->GetIVal() > 0)
    {
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION STATOSCOPE_CPP_SECTION_1
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/Statoscope_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/Statoscope_cpp_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(WIN64)
        m_logFilename += "_WIN64";
#elif defined(WIN32)    // make sure this stays below the WIN64 test as WIN32 is also defined for 64-bit compiles
        m_logFilename += "_WIN32";
#elif defined(MAC)
        m_logFilename += "_MAC";
#elif defined(ANDROID)
        m_logFilename += "_ANDROID";
#elif defined(LINUX)
    #if defined(PLATFORM_64BIT)
        m_logFilename += "_LINUX64";
    #else
        m_logFilename += "_LINUX32";
    #endif
#elif defined(IOS)
    #if defined(PLATFORM_64BIT)
        m_logFilename += "_IOS64";
    #else
        m_logFilename += "_IOS32";
    #endif
#elif defined(APPLETV)
    #if defined(PLATFORM_64BIT)
        m_logFilename += "_APPLETV64";
    #else
        m_logFilename += "_APPLETV32";
    #endif
#else
        m_logFilename += "_UNKNOWNPLAT";
#endif

        const SFileVersion& ver = gEnv->pSystem->GetFileVersion();
        char versionString[64];
        sprintf_s(versionString, sizeof(versionString), "_%d_%d_%d_%d", ver.v[3], ver.v[2], ver.v[1], ver.v[0]);

        m_logFilename += versionString;
    }

    if (strcmp(m_pStatoscopeFilenameUseTagCvar->GetString(), "") != 0)
    {
        m_logFilename += "_";
        m_logFilename += m_pStatoscopeFilenameUseTagCvar->GetString();
    }

    if (m_pStatoscopeFilenameUseMapCVar->GetIVal() > 0)
    {
        const char* mapName = NULL;

        //If we don't have a last map loaded, try to look up the current one now
        if (m_currentMap.empty())
        {
            if (gEnv->pGame)
            {
                mapName = gEnv->pGame->GetIGameFramework()->GetLevelName();
            }
        }
        //If we tracked the last map loaded then use it here
        else
        {
            mapName = m_currentMap.c_str();
        }

        if (mapName)
        {
            const char* nameNoDir = strrchr(mapName, '/');

            // strip directory for now
            if (nameNoDir)
            {
                mapName = nameNoDir + 1;
            }

            string mapNameTrucated(mapName);

            m_logFilename += "_";
            int mapNameLenLimit = 32;
            if (m_pStatoscopeFilenameUseBuildInfoCVar->GetIVal() > 0)
            {
                mapNameLenLimit = 12;
            }
            m_logFilename += mapNameTrucated.Left(mapNameLenLimit);
        }
    }

    if (m_pStatoscopeFilenameUseDatagroupsCVar->GetIVal() > 0)
    {
        string datagroups = "_";
        uint64 currentActiveDataGroupMask = (uint64)m_pStatoscopeDataGroupsCVar->GetI64Val();
        for (uint32 i = 0, ic = m_allDataGroups.size(); i < ic; i++)
        {
            CStatoscopeDataGroup& dataGroup = *m_allDataGroups[i];

            if (AlphaBit64(dataGroup.GetId()) & currentActiveDataGroupMask)
            {
                datagroups += dataGroup.GetId();
            }
        }

        m_logFilename += datagroups;
    }

    if (m_pStatoscopeFilenameUseTimeCVar->GetIVal() > 0)
    {
        time_t curTime;
        time(&curTime);
#ifdef AZ_COMPILER_MSVC
        struct tm lt;
        localtime_s(&lt, &curTime);

        char name[MAX_PATH];
        strftime(name, sizeof(name) / sizeof(name[0]), "_%Y%m%d_%H%M%S", &lt);
#else
        auto lt = localtime(&curTime);

        char name[MAX_PATH];
        strftime(name, sizeof(name) / sizeof(name[0]), "_%Y%m%d_%H%M%S", lt);
#endif

        m_logFilename += name;
    }

    //ensure unique log name
    if (m_pStatoscopeCreateLogFilePerLevelCVar->GetIVal())
    {
        char logNumBuf[10];
        sprintf_s(logNumBuf, 10, "_%d", m_logNum++);
        m_logFilename += logNumBuf;
    }

    if (m_pStatoscopeLogDestinationCVar->GetIVal() == eLD_File)
    {
        SAFE_DELETE(m_pDataWriter);
    }

    m_logFilename += ".bin";
}

void CStatoscope::SetDataGroups(uint64 enabledDGs, uint64 disabledDGs)
{
    for (uint32 i = 0, ic = m_allDataGroups.size(); i < ic; i++)
    {
        CStatoscopeDataGroup& dataGroup = *m_allDataGroups[i];

        if (AlphaBit64(dataGroup.GetId()) & disabledDGs)
        {
            dataGroup.GetCallback()->Disable();
            m_activeDataGroups.erase(std::find(m_activeDataGroups.begin(), m_activeDataGroups.end(), &dataGroup));
        }

        if (AlphaBit64(dataGroup.GetId()) & enabledDGs)
        {
            m_activeDataGroups.push_back(&dataGroup);
            dataGroup.GetCallback()->Enable();
        }
    }
}

void CStatoscope::OutputLoadedModuleInformation(CDataWriter* pDataWriter)
{
    pDataWriter->WriteData(0);
    pDataWriter->m_bHaveOutputModuleInformation = true;
}

void CStatoscope::StoreCallstack(const char* tag, void** callstackAddresses, uint32 callstackLength)
{
    if (m_pCallstacks && m_pCallstacks->IsEnabled())
    {
        CryMT::vector<SCallstack>::AutoLock lock(m_pCallstacks->m_callstacks.get_lock());
        SCallstack callstack(callstackAddresses, callstackLength, tag);
        m_pCallstacks->m_callstacks.push_back(SCallstack());
        m_pCallstacks->m_callstacks.back().swap(callstack);
    }
}

void CStatoscope::AddUserMarker(const char* path, const char* name)
{
    if (!IsRunning())
    {
        return;
    }

    if (m_pUserMarkers && m_pUserMarkers->IsEnabled())
    {
        m_pUserMarkers->m_userMarkers.push_back(SUserMarker(path, name));
    }
}

void CStatoscope::AddUserMarkerFmt(const char* path, const char* fmt, ...)
{
    if (!IsRunning())
    {
        return;
    }

    if (m_pUserMarkers && m_pUserMarkers->IsEnabled())
    {
        char msg[1024];
        va_list args;
        va_start(args, fmt);
        azvsnprintf(msg, sizeof(msg), fmt, args);
        msg[sizeof(msg) - 1] = '\0';
        va_end(args);

        m_pUserMarkers->m_userMarkers.push_back(SUserMarker(path, msg));
    }
}

//LogCallstack("USER MARKER");
void CStatoscope::LogCallstack(const char* tag)
{
    if (!IsRunning())
    {
        return;
    }

    uint32 callstackLength = 128;
    void* callstackAddresses[128];

    CSystem::debug_GetCallStackRaw(callstackAddresses, callstackLength);
    StoreCallstack(tag, callstackAddresses, callstackLength);
}

void CStatoscope::LogCallstackFormat(const char* tagFormat, ...)
{
    if (!IsRunning())
    {
        return;
    }

    va_list args;
    va_start(args, tagFormat);
    char tag[256];
    vsprintf_s(tag, sizeof(tag), tagFormat, args);
    va_end(args);

    uint32 callstackLength = 128;
    void* callstackAddresses[128];

    CSystem::debug_GetCallStackRaw(callstackAddresses, callstackLength);
    StoreCallstack(tag, callstackAddresses, callstackLength);
}

void CStatoscope::ConsoleAddUserMarker(IConsoleCmdArgs* pParams)
{
    if (pParams->GetArgCount() == 3)
    {
        gEnv->pStatoscope->AddUserMarker(pParams->GetArg(1), pParams->GetArg(2));
    }
    else
    {
        CryLogAlways("Invalid use of e_StatoscopeAddUserMarker. Expecting 2 arguments, not %d.\n", pParams->GetArgCount() - 1);
    }
}

void CStatoscope::OnLogDestinationCVarChange(ICVar* pVar)
{
    CStatoscope* pStatoscope = (CStatoscope*)gEnv->pStatoscope;
    SAFE_DELETE(pStatoscope->m_pDataWriter);
    pStatoscope->m_pServer->CloseConnection();
}

bool CStatoscope::IsLoggingForTelemetry()
{
    return m_pStatoscopeLogDestinationCVar->GetIVal() == eLD_Telemetry;
}

void CStatoscope::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
    switch (event)
    {
    case ESYSTEM_EVENT_LEVEL_PRECACHE_END:
    {
        if (!m_bLevelStarted)
        {
            if (!m_pGameRulesCVar)
            {
                m_pGameRulesCVar = gEnv->pConsole->GetCVar("sv_gamerules");
            }

            const char* mapName = NULL;

            if (gEnv->pGame && gEnv->pGame->GetIGameFramework())
            {
                mapName = gEnv->pGame->GetIGameFramework()->GetLevelName();
            }

            if (!mapName)
            {
                mapName = "unknown_map";
            }
            else
            {                
                m_currentMap = mapName;
            }

            string userMarker = "Start ";
            userMarker += mapName;

            if (m_pGameRulesCVar)
            {
                userMarker += " ";
                userMarker += m_pGameRulesCVar->GetString();
            }

            AddUserMarker("Level", userMarker.c_str());

            if (m_pStatoscopeCreateLogFilePerLevelCVar->GetIVal())
            {
                //force new log file
                SetLogFilename();
            }

            m_bLevelStarted = true;
        }
    }
    break;
    case ESYSTEM_EVENT_LEVEL_UNLOAD:
    {
        AddUserMarker("Level", "End");
        m_bLevelStarted = false;

        if (m_pDataWriter)
        {
            AddFrameRecord(false);
        }
    }
    break;
    }
}

//Screenshot capturing
bool CStatoscope::OnNeedFrameData(unsigned char*& pConvertedTextureBuf)
{
    //The renderer will only perform the screen grab if we supply a pointer from this callback. Since we only want one intermittently return null most of the time
    if (m_ScreenShotState == eSSCS_AwaitingBufferRequest || m_ScreenShotState == eSSCS_AwaitingCapture)
    {
        m_ScreenShotState = eSSCS_AwaitingCapture;
        pConvertedTextureBuf = m_pScreenShotBuffer;
        return true;
    }
    return false;
}

void CStatoscope::OnFrameCaptured()
{
    //The renderer has finished copying the screenshot into the buffer. Change state so we write the shot out.
    m_ScreenShotState = eSSCS_DataReceived;
}

int CStatoscope::OnGetFrameWidth(void)
{
    return gEnv->pRenderer->GetWidth() / SCREENSHOT_SCALING_FACTOR;
}

int CStatoscope::OnGetFrameHeight(void)
{
    return gEnv->pRenderer->GetHeight() / SCREENSHOT_SCALING_FACTOR;
}

int CStatoscope::OnCaptureFrameBegin(int* pTexHandle)
{
    //Called at the beginning of the rendering pass, the flags returned determine if the screenshot render target gets written to.
    //For performance reasons we do this infrequently.
    int flags = 0;
    if (m_ScreenShotState == eSSCS_RequestCapture)
    {
        flags |= eCFF_CaptureThisFrame;
        m_ScreenShotState = eSSCS_AwaitingBufferRequest;    //Frame initialised. Wait for the buffer to be requested
    }

    return flags;
}

void CStatoscope::SetupFPSCaptureCVars()
{
    if (m_pStatoscopeAllowFPSOverrideCVar && m_pStatoscopeAllowFPSOverrideCVar->GetIVal() != 0)
    {
        if (m_pStatoscopeDataGroupsCVar)
        {
            m_pStatoscopeDataGroupsCVar->Set("fgmut");
        }
        if (m_pStatoscopeFilenameUseBuildInfoCVar)
        {
            m_pStatoscopeFilenameUseBuildInfoCVar->Set("0");
        }
        if (m_pStatoscopeFilenameUseMapCVar)
        {
            m_pStatoscopeFilenameUseMapCVar->Set("1");
        }
        if (m_pStatoscopeCreateLogFilePerLevelCVar)
        {
            m_pStatoscopeCreateLogFilePerLevelCVar->Set("1");
        }

        ICVar* pCVar = gEnv->pConsole->GetCVar("e_StatoscopeScreenCapWhenGPULimited");
        if (pCVar)
        {
            pCVar->Set(1);
        }
    }
}

bool CStatoscope::RequestScreenShot()
{
    if (m_ScreenShotState == eSSCS_Idle)
    {
        m_ScreenShotState = eSSCS_RequestCapture;
        PrepareScreenShot();
        return true;
    }

    return false;
}

void CStatoscope::RegisterBuiltInDataGroups()
{
    m_pParticleProfilers = new SParticleProfilersDG();
    m_pPhysEntityProfilers = new SPhysEntityProfilersDG();
    m_pFrameProfilers = new SFrameProfilersDG();
    m_pUserMarkers = new SUserMarkerDG();
    m_pCallstacks = new SCallstacksDG();

    RegisterDataGroup(new SFrameLengthDG());
    RegisterDataGroup(new SEF_ListDG());
    RegisterDataGroup(new SMemoryDG());
    RegisterDataGroup(new SStreamingDG());
    RegisterDataGroup(new SStreamingAudioDG());
    RegisterDataGroup(new SStreamingObjectsDG());
    RegisterDataGroup(new SThreadsDG());
    RegisterDataGroup(new SSystemThreadsDG());
    RegisterDataGroup(new SCPUTimesDG());
    RegisterDataGroup(new SVertexCostDG());
    RegisterDataGroup(new SParticlesDG());
    RegisterDataGroup(new SLocationDG());
    RegisterDataGroup(new SPerCGFGPUProfilersDG());
    RegisterDataGroup(m_pParticleProfilers);
    RegisterDataGroup(m_pPhysEntityProfilers);
    RegisterDataGroup(m_pFrameProfilers);
    RegisterDataGroup(new SPerfCountersDG());
    RegisterDataGroup(m_pUserMarkers);
    RegisterDataGroup(m_pCallstacks);
    RegisterDataGroup(new SNetworkDG());
    RegisterDataGroup(new SChannelDG());
    RegisterDataGroup(new SNetworkProfileDG());
}

void CStatoscope::RegisterBuiltInIvDataGroups()
{
    m_intervalGroups.push_back(new CStatoscopeStreamingIntervalGroup());
    m_intervalGroups.push_back(new CStreamingObjectIntervalGroup());
    m_intervalGroups.push_back(new CStatoscopeTextureStreamingIntervalGroup());
}

void CStatoscope::CreateDataWriter()
{
    if (m_pDataWriter == NULL)
    {
        if (m_pStatoscopeLogDestinationCVar->GetIVal() == eLD_File)
        {
            if (m_logFilename.empty())
            {
                SetLogFilename();
            }

            m_pDataWriter = new CFileDataWriter(m_logFilename, m_pStatoscopeWriteTimeout->GetFVal() * 10);
        }
        else if (m_pStatoscopeLogDestinationCVar->GetIVal() == eLD_Socket)
        {
            m_pServer->SetPort(m_pStatoscopePortCVar->GetIVal());
            m_pDataWriter = new CSocketDataWriter(m_pServer, m_pStatoscopeWriteTimeout->GetFVal() * 10);
        }
        else if (m_pStatoscopeLogDestinationCVar->GetIVal() == eLD_Telemetry)
        {
            return;
        }

        assert(m_pDataWriter);
    }
}

void CStatoscope::WriteIntervalClassEvents()
{
    for (IntervalGroupVec::iterator it = m_intervalGroups.begin(), itEnd = m_intervalGroups.end(); it != itEnd; ++it)
    {
        size_t descLength = (*it)->GetDescEventLength();

        DataWriter::EventDefineClass* pEv = m_eventWriter.BeginEvent<DataWriter::EventDefineClass>(descLength);

        pEv->classId = (*it)->GetId();
        pEv->numElements = (*it)->GetNumElements();
        (*it)->WriteDescEvent(pEv + 1);

        m_eventWriter.EndEvent();
    }
}

CStatoscopeServer::CStatoscopeServer(CStatoscope* pStatoscope)
{
    m_socket = AZ_SOCKET_INVALID;
    m_isConnected = false;
    m_pStatoscope = pStatoscope;
    //m_DataThread.SetServer(this);
    m_port = default_statoscope_port;
}

void CStatoscopeServer::StartListening()
{
    CloseConnection();

    AZ_Assert(!AZ::AzSock::IsAzSocketValid(m_socket), "AZSocket already valid");

    m_socket = AZ::AzSock::Socket();
    if (!AZ::AzSock::IsAzSocketValid(m_socket))
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "CStatoscopeServer() - failed to create a valid socket");
        return;
    }

    AZ::AzSock::AzSocketAddress socketAddress;
    socketAddress.SetAddrPort(m_port);

    int result = AZ::AzSock::Bind(m_socket, socketAddress);
    if (CheckError(result, "CStatoscopeServer() - failed to bind the socket"))
    {
        return;
    }

    result = AZ::AzSock::Listen(m_socket, 1);
    if (CheckError(result, "CStatoscopeServer() - failed to set the socket to listen"))
    {
        return;
    }

    CryLog("Statoscope server listening on port: %d\n", socketAddress.GetAddrPort());

    SetBlockingState(false);
    m_pStatoscope->m_pDataWriter->ResetForNewLog();
}

void CStatoscopeServer::CheckForConnection()
{
    if (m_isConnected)
    {
        return;
    }

    if (!AZ::AzSock::IsAzSocketValid(m_socket))
    {
        StartListening();
    }

    if (!AZ::AzSock::IsAzSocketValid(m_socket))
    {
        return;
    }

    AZ::AzSock::AzSocketAddress sa;
    AZSOCKET newSocket = AZ::AzSock::Accept(m_socket, sa);

    if (!AZ::AzSock::IsAzSocketValid(newSocket))
    {
        // this error reflects the absence of a pending connection request
        if (newSocket != static_cast<AZ::s32>(AZ::AzSock::AzSockError::eASE_EWOULDBLOCK))
        {
            CheckError(newSocket, "CStatoscopeServer::CheckForConnection() - invalid socket from accept()");
        }
        return;
    }

    if (CheckError(AZ::AzSock::CloseSocket(m_socket), "CStatoscopeServer::CheckForConnection() - failed to close the listening socket"))
    {
        return;
    }

    m_socket = newSocket;
    SetBlockingState(true);
    m_isConnected = true;

    CryLog("CStatoscopeServer connected to: %s\n", sa.GetAddress().c_str());
}

void CStatoscopeServer::CloseConnection()
{
    if (!AZ::AzSock::IsAzSocketValid(m_socket))
    {
        return;
    }

    SetBlockingState(false);

    int ret = AZ::AzSock::Shutdown(m_socket, SD_SEND);
    if (AZ::AzSock::SocketErrorOccured(ret))
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "CStatoscopeServer::CloseConnection() - shutdown failed");
    }

    do
    {
        char buffer[256];
        ret = AZ::AzSock::Recv(m_socket, buffer, sizeof(buffer), 0);
    } while (ret > 0);

    ret = AZ::AzSock::CloseSocket(m_socket);
    if (AZ::AzSock::SocketErrorOccured(ret))
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "CStatoscopeServer::CloseConnection() - closesocket failed");
    }

    m_socket = AZ_SOCKET_INVALID;
    m_isConnected = false;
}

void CStatoscopeServer::SetBlockingState(bool block)
{
    int result = AZ::AzSock::SetSocketBlockingMode(m_socket, block);
    if (result < 0)
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "CStatoscopeServer::SetBlockingState() failed");
    }
}

int32 CStatoscopeServer::ReceiveData(void* buffer, int bufferSize)
{
    if (!AZ::AzSock::IsAzSocketValid(m_socket) || !m_isConnected)
    {
        return 0;
    }

    int ret = AZ::AzSock::Recv(m_socket, static_cast<char*>(buffer), bufferSize, 0);
    if (CheckError(ret, "CStatoscopeServer::ReceiveData()"))
    {
        return 0;
    }

    return ret;
}

void CStatoscopeServer::SendData(const char* buffer, int bufferSize)
{
    threadID threadID = CryGetCurrentThreadId();

    if (!AZ::AzSock::IsAzSocketValid(m_socket) || !m_isConnected)
    {
        return;
    }

    FUNCTION_PROFILER(gEnv->pSystem, PROFILE_SYSTEM);

    //float startTime = gEnv->pTimer->GetAsyncCurTime();
    //int origBufferSize = bufferSize;
    //const char *origBuffer = buffer;

    while (bufferSize > 0)
    {
        int ret = AZ::AzSock::Send(m_socket, buffer, bufferSize, 0);
        if (CheckError(ret, "CStatoscopeServer::SendData()"))
        {
            return;
        }

        buffer += ret;
        bufferSize -= ret;
    }

    //float endTime = gEnv->pTimer->GetAsyncCurTime();
    //printf("Statoscope Send Data 0x%p size: %d time: %f time taken %f\n", origBuffer, origBufferSize, endTime, endTime - startTime);
}

bool CStatoscopeServer::CheckError(int err, const char* tag)
{
    if(AZ::AzSock::SocketErrorOccured(err))
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "%s error: %s", tag, AZ::AzSock::GetStringForError(err));
        CloseConnection();
        return true;
    }

    return false;
}

CDataWriter::CDataWriter(bool bUseStringPool, float writeTimeout)
{
    m_bUseStringPool = bUseStringPool;
    m_bTimedOut = false;
    m_writeTimeout = writeTimeout;

    m_buffer.resize(128 * 1024);
    m_formatBuffer.resize(64 * 1024);

    m_pWritePtr = &m_buffer[0];
    m_pFlushStartPtr = &m_buffer[0];

    ResetForNewLog();
}

void CDataWriter::Close()
{
    Flush();
}

void CDataWriter::Flush()
{
    m_DataThread.QueueSendData(m_pFlushStartPtr, (int)(m_pWritePtr - m_pFlushStartPtr));
    m_pFlushStartPtr = m_pWritePtr;

    m_DataThread.Flush();
}

void CDataWriter::ResetForNewLog()
{
    m_pFlushStartPtr = m_pWritePtr;
    m_DataThread.Clear();
    m_DataThread.Flush();
    m_bShouldOutputLogTopHeader = true;
    m_bHaveOutputModuleInformation = false;
    m_GlobalStringPoolHashes.clear();
    m_bTimedOut = false;
}

void CDataWriter::FlushIOThread()
{
    printf("[Statoscope]Flush Data Thread\n");
    m_DataThread.Flush();
    m_pWritePtr = &m_buffer[0];
    m_pFlushStartPtr = &m_buffer[0];
}

//align write pointer to 4byte
void CDataWriter::Pad4()
{
    int pad = ((INT_PTR)m_pWritePtr) & 3;

    if (pad)
    {
        char pBuf[4] = {0};
        WriteData(pBuf, pad);
    }
}

void CDataWriter::WriteData(const void* vpData, int vsize)
{
    if (m_bTimedOut)
    {
        return;
    }
    const char* pData = (const char*)vpData;
    const int bufferSize = (int)m_buffer.size() - 1;
    while (vsize > 0)
    {
        const int size = std::min(bufferSize, vsize);

        int capacity = (int)(&m_buffer[bufferSize] - m_pWritePtr);

        bool bWrapBuffer = size > capacity;

        //if we are wrapping the buffer, send the remaining jobs
        if (bWrapBuffer)
        {
            m_DataThread.QueueSendData(m_pFlushStartPtr, (int)(m_pWritePtr - m_pFlushStartPtr));

            //reset write pointer
            m_pWritePtr = &m_buffer[0];
            m_pFlushStartPtr = &m_buffer[0];
        }

        char* pWriteStart = m_pWritePtr;
        char* pWriteEnd = pWriteStart + size;

        // Stall until the read thread clears the buffer we need to use
        CTimeValue startTime = gEnv->pTimer->GetAsyncTime();
        do
        {
            const char* pReadBoundsStart, * pReadBoundsEnd;
            uint32 numBytesInQueue = m_DataThread.GetReadBounds(pReadBoundsStart, pReadBoundsEnd);

            if (numBytesInQueue == 0)
            {
                break;
            }

            // if these are the same, there's no room in the buffer
            if (pReadBoundsStart != pReadBoundsEnd)
            {
                if (pReadBoundsStart <= pReadBoundsEnd)
                {
                    // Simple case, just the one in use section
                    if (pWriteEnd <= pReadBoundsStart || pWriteStart >= pReadBoundsEnd)
                    {
                        break;
                    }
                }
                else
                {
                    // Two in use sections, wrapping at the boundaries
                    if ((pWriteEnd <= pReadBoundsStart) && (pWriteStart >= pReadBoundsEnd))
                    {
                        break;
                    }
                }
            }
            CTimeValue currentTime = gEnv->pTimer->GetAsyncTime();
            if (m_writeTimeout != 0.0f && currentTime.GetDifferenceInSeconds(startTime) > m_writeTimeout)
            {
                CryLog("CDataWriter write timeout exceeded: %f", currentTime.GetDifferenceInSeconds(startTime));
                m_bTimedOut = true;
                return;
            }

            Sleep(1);
        }
        while (true);

        memcpy((void*)pWriteStart, pData, size);
        m_pWritePtr = pWriteEnd;

        if ((m_pWritePtr - m_pFlushStartPtr) >= FlushLength)
        {
            m_DataThread.QueueSendData(m_pFlushStartPtr, (int)(m_pWritePtr - m_pFlushStartPtr));
            m_pFlushStartPtr = m_pWritePtr;
        }

        pData += size;
        vsize -= size;
    }
}
CFileDataWriter::CFileDataWriter(const string& fileName, float writeTimeout)
    : CDataWriter(true, writeTimeout)
{
    m_fileName = fileName;
    m_fileHandle = AZ::IO::InvalidHandle;
    m_bAppend = false;
    m_DataThread.SetDataWriter(this);
}

CFileDataWriter::~CFileDataWriter()
{
    Close();
}

bool CFileDataWriter::Open()
{
    if (m_fileHandle == AZ::IO::InvalidHandle)
    {
        CDebugAllowFileAccess afa;
        const char* modeStr = m_bAppend ? "ab" : "wb";
        m_bAppend = true;

        m_fileHandle = fxopen(m_fileName.c_str(), modeStr);
    }

    return m_fileHandle != AZ::IO::InvalidHandle;
}

void CFileDataWriter::Close()
{
    if (m_fileHandle != AZ::IO::InvalidHandle)
    {
        CDataWriter::Close();
        gEnv->pFileIO->Close(m_fileHandle);
        m_fileHandle = AZ::IO::InvalidHandle;
    }
}

void CFileDataWriter::Flush()
{
    if (m_fileHandle != AZ::IO::InvalidHandle)
    {
        CDataWriter::Flush();
        gEnv->pFileIO->Flush(m_fileHandle);
    }
}

void CFileDataWriter::SendData(const char* pBuffer, int nBytes)
{
    if (m_fileHandle != AZ::IO::InvalidHandle)
    {
        gEnv->pFileIO->Write(m_fileHandle, pBuffer, nBytes);

        //float endTime = gEnv->pTimer->GetAsyncCurTime();
        //printf("Statoscope Write Data 0x%p size: %d time: %f time taken %f\n", pBuffer, nBytes, endTime, endTime - startTime);
    }
    else
    {
        CryFatalError("Statoscope file not open");
    }
}

CSocketDataWriter::CSocketDataWriter(CStatoscopeServer* pStatoscopeServer, float writeTimeout)
    : CDataWriter(true, writeTimeout)
{
    m_pStatoscopeServer = pStatoscopeServer;
    m_DataThread.SetDataWriter(this);
}

bool CSocketDataWriter::Open()
{
    m_pStatoscopeServer->CheckForConnection();
    return m_pStatoscopeServer->IsConnected();
}

void CSocketDataWriter::SendData(const char* pBuffer, int nBytes)
{
    m_pStatoscopeServer->SendData(pBuffer, nBytes);
}


CTelemetryDataWriter::CTelemetryDataWriter(const char* postHeader, const char* hostname, int port, float writeTimeout, float connectTimeout)
    : CDataWriter(true, writeTimeout)
    , m_socket(AZ_SOCKET_INVALID)
    , m_hasSentHeader(false)
    , m_socketErrorTriggered(false)
    , m_connectTimeout(connectTimeout)
{
    m_postHeader = postHeader;
    m_hostname = hostname;
    m_port = port;
    m_DataThread.SetDataWriter(this);
}

bool CTelemetryDataWriter::Open()
{
    if (!AZ::AzSock::IsAzSocketValid(m_socket) && !m_socketErrorTriggered)
    {
        AZSOCKET s = AZ::AzSock::Socket();
        if (!AZ::AzSock::IsAzSocketValid(s))
        {
            CryLog("CTelemetryDataWriter failed to create a valid socket");
            m_socketErrorTriggered = true;
            return false;
        }
        if (!AZ::AzSock::SetSocketBlockingMode(s, false))
        {
            CryLog("CTelemetryDataWriter failed to make socket non-blocking");
            m_socketErrorTriggered = true;
            return false;
        }
        AZ::AzSock::AzSocketAddress socketAddress;
        
        if (socketAddress.SetAddress(m_hostname.c_str(), m_port))
        {
            m_socket = s;
            int result = AZ::AzSock::Connect(m_socket, socketAddress);
            if (result != static_cast<AZ::s32>(AZ::AzSock::AzSockError::eASE_EWOULDBLOCK_CONN))
            {
                CheckSocketError(result, "Connect to telemetry server");
            }
        }
        else
        {
            CryLog("CTelemetryDataWriter failed to resolve host name '%s'", m_hostname.c_str());
            m_socketErrorTriggered = true;
        }
    }
    return (AZ::AzSock::IsAzSocketValid(m_socket));
}

void CTelemetryDataWriter::Close()
{
    CDataWriter::Close();
    if (AZ::AzSock::IsAzSocketValid(m_socket))
    {
        AZSOCKET sock = m_socket;
        SendToSocket("0\r\n\r\n", 5, "End HTTP chunked data stream");
        CheckSocketError(AZ::AzSock::Shutdown(sock, SD_SEND), "Shutdown socket for sending");
        while (!m_bTimedOut)
        {
            const int recvBufferSize = 1024;
            char recvBuffer[recvBufferSize];
            int result = AZ::AzSock::Recv(sock, recvBuffer, recvBufferSize, 0);
            CheckSocketError(result, "Wait for response");
            if (!result || AZ::AzSock::SocketErrorOccured(result))
            {
                break;
            }
            // TODO: Check recvBuffer contains OK message
        }
        CheckSocketError(AZ::AzSock::CloseSocket(sock), "Close socket");
        m_socket = AZ_SOCKET_INVALID;
    }
}

void CTelemetryDataWriter::SendData(const char* pBuffer, int nBytes)
{
    if (AZ::AzSock::IsAzSocketValid(m_socket))
    {
        if (!m_hasSentHeader)
        {
            // Wait until the connection is fully established
            timeval timeout;
            timeout.tv_sec = (long)(m_connectTimeout);
            timeout.tv_usec = (long)((m_connectTimeout - timeout.tv_sec) * 1e6);

            int result = AZ::AzSock::WaitForWritableSocket(m_socket, &timeout);

            if (result < 0)
            {
                CheckSocketError(result, "Waiting for connection to be fully established");
            }
            else if (result == 0)
            {
                int sockError = static_cast<AZ::s32>(AZ::AzSock::AzSockError::eASE_ETIMEDOUT);
                CheckSocketError(sockError, "Waiting for connection to be fully established");
            }
            SendToSocket(m_postHeader.c_str(), m_postHeader.size(), "Begin HTTP chunked data stream");
            m_hasSentHeader = true;
        }
        const int maxChunkHeaderSize = 16;
        char chunkHeaderBuffer[maxChunkHeaderSize];
        int chunkHeaderSize = sprintf_s(chunkHeaderBuffer, maxChunkHeaderSize, "%x\r\n", nBytes);
        assert(chunkHeaderSize >= 0);
        SendToSocket(chunkHeaderBuffer, chunkHeaderSize, "Write HTTP chunk size");
        SendToSocket(pBuffer, nBytes, "Write HTTP chunk data");
        SendToSocket("\r\n", 2, "Terminate HTTP chunk data");
    }
}

void CTelemetryDataWriter::CheckSocketError(int result, const char* description)
{
    if (result < 0)
    {
        CryLog("CTelemetryDataWriter socket error '%s' - '%d'", description, result);
        if (AZ::AzSock::IsAzSocketValid(m_socket))
        {
            AZ::AzSock::CloseSocket(m_socket);
            m_socket = AZ_SOCKET_INVALID;
        }
        m_socketErrorTriggered = true;
    }
}

void CTelemetryDataWriter::SendToSocket(const char* pData, size_t nSize, const char* sDescription)
{
    if (AZ::AzSock::IsAzSocketValid(m_socket))
    {
        size_t nSent = 0;
        while (!m_bTimedOut)
        {
            int err = AZ::AzSock::Send(m_socket, pData + nSent, nSize - nSent, 0);
            if (AZ::AzSock::SocketErrorOccured(err))
            {
                if (err != static_cast<AZ::s32>(AZ::AzSock::AzSockError::eASE_EWOULDBLOCK))
                {
                    CheckSocketError(err, sDescription);
                    return;
                }
            }
            else
            {
                nSent += err;
                if (nSent >= nSize)
                {
                    return;
                }
            }
            CrySleep(1);
        }
    }
}


CStatoscopeIOThread::CStatoscopeIOThread()
{
    m_pDataWriter = NULL;
    m_threadID = THREADID_NULL;
    m_numBytesInQueue = 0;

    Start();
}

CStatoscopeIOThread::~CStatoscopeIOThread()
{
    //ensure all data is sent
    Flush();

    // Stop thread task.
    Stop();
    WaitForThread();
}

void CStatoscopeIOThread::Flush()
{
    CTimeValue startTime = gEnv->pTimer->GetAsyncTime();
    while (m_sendJobs.size())
    {
        CrySleep(1);
        CTimeValue currentTime = gEnv->pTimer->GetAsyncTime();
        float timeout = m_pDataWriter->GetWriteTimeout();
        if (timeout != 0.0f && currentTime.GetDifferenceInSeconds(startTime) > timeout)
        {
            // This should cause the data writer to abort attempting to write data and clear the send jobs queue
            CryLog("CDataWriter write timeout exceeded during flush: %f", currentTime.GetDifferenceInSeconds(startTime));
            m_pDataWriter->TimeOut();
        }
    }
}

void CStatoscopeIOThread::Run()
{
    m_threadID = CryGetCurrentThreadId();
    CryThreadSetName(threadID(THREADID_NULL), "StatoscopeDataWriter");

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION STATOSCOPE_CPP_SECTION_2
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/Statoscope_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/Statoscope_cpp_provo.inl"
    #endif
#endif

    while (IsStarted())
    {
        if (m_sendJobs.size() > 0)
        {
            SendJob job;
            {
                CryMT::queue<SendJob>::AutoLock lock(m_sendJobs.get_lock());
                job = m_sendJobs.front();
            }

            m_pDataWriter->SendData(job.pBuffer, job.nBytes);

            {
                CryMT::queue<SendJob>::AutoLock lock(m_sendJobs.get_lock());
                SendJob j;
                m_sendJobs.try_pop(j);
                m_numBytesInQueue -= j.nBytes;
            }

            //PIXEndNamedEvent();
        }
        else
        {
            CrySleep(1);
        }
    }
}

void CStatoscopeIOThread::QueueSendData(const char* pBuffer, int nBytes)
{
    if (nBytes > 0)
    {
        bool bWait = false;

        //PIXSetMarker(0, "[STATOSCOPE]Queue Data\n");

        assert(pBuffer);
        assert(nBytes > 0);

        SendJob newJob = {pBuffer, nBytes};

        {
            CryMT::queue<SendJob>::AutoLock lock(m_sendJobs.get_lock());
            m_sendJobs.push(newJob);
            m_numBytesInQueue += newJob.nBytes;
        }

        //printf("Statoscope Queue Data 0x%p size %d at time: %f\n", pBuffer, nBytes, gEnv->pTimer->GetAsyncCurTime());
    }
    else if (nBytes < 0)
    {
        CryFatalError("Borked!");
    }
}

#undef EWOULDBLOCK
#undef GetLastError

#endif // ENABLE_STATOSCOPE
