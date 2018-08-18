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

#include "CryLegacy_precompiled.h"
#include "CharacterManager.h"
#include "Model.h"

static const float s_DefaultSegmentParametricValue = 1.0f;

//////////////////////////////////////////////////////////////////////////
GlobalAnimationHeaderCAF::GlobalAnimationHeaderCAF()
{
    m_FilePathDBACRC32 = -1;
    m_nRef_by_Model = 0;
    m_nRef_at_Runtime = 0;
    m_nTouchedCounter = 0;
    m_bEmpty = 0;

    m_fStartSec = -1;       // Start time in seconds.
    m_fEndSec = -1;     // End time in seconds.
    m_fTotalDuration = -1.0f;               //asset-features
    m_StartLocation.SetIdentity();      //asset-features

    m_Segments = 1;                     //asset-features

    m_SegmentsTime[0] = 0.0f;                   //asset-features
    m_SegmentsTime[1] = s_DefaultSegmentParametricValue;    //asset-features
    m_SegmentsTime[2] = s_DefaultSegmentParametricValue;    //asset-features
    m_SegmentsTime[3] = s_DefaultSegmentParametricValue;    //asset-features
    m_SegmentsTime[4] = s_DefaultSegmentParametricValue;    //asset-features

    m_nControllers = 0;
    m_nControllers2 = 0;

    m_fSampleRate = ANIMATION_30Hz;
}

void GlobalAnimationHeaderCAF::ResetSegmentTime(int segmentIndex)
{
    if (segmentIndex >= 1 && segmentIndex < (CAF_MAX_SEGMENTS - 1))
    {
        m_SegmentsTime[segmentIndex] = s_DefaultSegmentParametricValue;
    }
}

uint32 GlobalAnimationHeaderCAF::LoadCAF()
{
    LOADING_TIME_PROFILE_SECTION(GetISystem());
    const char* pname = m_FilePath.c_str();
    MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Other, 0, "CAF Animation %s", pname);

    m_nFlags = 0;
    OnAssetNotFound();

    _smart_ptr<IChunkFile> pChunkFile = g_pI3DEngine->CreateChunkFile(true);
    if (!pChunkFile->Read(m_FilePath))
    {
        g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING,  VALIDATOR_FLAG_FILE, GetFilePath(), "CAF-File Not Found: %s", GetFilePath());
        return 0;
    }

    GlobalAnimationHeaderCAFStreamContent content;
    content.m_id = (int)std::distance(g_AnimationManager.m_arrGlobalCAF.begin(), this);
    content.m_pPath = GetFilePath();
    content.m_nFlags = m_nFlags;
    content.m_FilePathDBACRC32 = m_FilePathDBACRC32;
    content.m_StartLocation.SetIdentity();

    bool bLoadOldChunks = false;
    bool bLoadInPlace = false;
    size_t nUsefulSize = 0;
    content.ParseChunkHeaders(pChunkFile, bLoadOldChunks, bLoadInPlace, nUsefulSize);

    content.m_fSampleRate = GetSampleRate();

    content.m_hControllerData = g_controllerHeap.AllocPinned(nUsefulSize, NULL);
    if (content.m_hControllerData.IsValid())
    {
        uint32 numChunks = pChunkFile->NumChunks();

        IControllerRelocatableChain* pRelocateHead = NULL;
        char* pStorage = (char*)g_controllerHeap.WeakPin(content.m_hControllerData);
        if (content.ParseChunkRange(pChunkFile, 0, numChunks, bLoadOldChunks, pStorage, pRelocateHead))
        {
            g_controllerHeap.ChangeContext(content.m_hControllerData, pRelocateHead);

            CommitContent(content);

            //---> file loaded successfully
            m_FilePathDBACRC32  =   0; //if 0, then this is a streamable CAF file
            ControllerInit();
#ifndef _RELEASE
            if (Console::GetInst().ca_DebugAnimUsageOnFileAccess)
            {
                g_AnimationManager.DebugAnimUsage(0);
            }
#endif

            return true;
        }
    }

    return false;
}


void GlobalAnimationHeaderCAF::LoadControllersCAF()
{
    uint32 OnDemand = IsAssetOnDemand();
    if (OnDemand)
    {
        LoadCAF();
    }
    else
    {
        LoadDBA();
    }
}

//----------------------------------------------------------------------------------------

void GlobalAnimationHeaderCAF::LoadDBA()
{
    MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Animation DBA");

    if (m_nControllers == 0 && m_FilePathDBACRC32)
    {
        size_t numDBA_Files = g_AnimationManager.m_arrGlobalHeaderDBA.size();
        for (uint32 d = 0; d < numDBA_Files; d++)
        {
            CGlobalHeaderDBA& pGlobalHeaderDBA = g_AnimationManager.m_arrGlobalHeaderDBA[d];
            if (m_FilePathDBACRC32 != pGlobalHeaderDBA.m_FilePathDBACRC32)
            {
                continue;
            }
            pGlobalHeaderDBA.m_nLastUsedTimeDelta = 0;
            if (pGlobalHeaderDBA.m_pDatabaseInfo == 0)
            {
                pGlobalHeaderDBA.LoadDatabaseDBA("");
            }

            break;
        }
    }
#ifndef _RELEASE
    if (Console::GetInst().ca_DebugAnimUsageOnFileAccess)
    {
        g_AnimationManager.DebugAnimUsage(0);
    }
#endif
}






//////////////////////////////////////////////////////////////////////////
void GlobalAnimationHeaderCAF::StartStreamingCAF()
{
    //LoadCAF( 0 );
    //return;

    if (IsAssetCreated() == 0)
    {
        if (Console::GetInst().ca_UseIMG_CAF)
        {
            //data-mismatch between IMG file and Animation-PAK. Most likely an issue with the build-process
#ifndef _RELEASE
            uint32 num = g_DataMismatch.size();
            for (uint32 i = 0; i < num; i++)
            {
                if (g_DataMismatch[i] == GetFilePath())
                {
                    return;
                }
            }
            g_DataMismatch.push_back(GetFilePath());
            g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING,  VALIDATOR_FLAG_FILE, GetFilePath(),  "GlobalAnimationHeaderCAF is not created. This is a data-corruption issue!  m_nControllers2: %08x  m_FilePathDBACRC32: %08x", m_nControllers2, m_FilePathDBACRC32);
#endif
            return;
        }
        else
        {
            m_nControllers = 0;
            m_nControllers2 = 0;
            return;
        }
    }

    uint32 IsLoaded    = IsAssetLoaded();
    if (IsLoaded)
    {
        return;
    }
    uint32 IsRequested = IsAssetRequested();
    if (IsRequested)
    {
        return;
    }

    if (IsAssetOnDemand())
    {
        ClearControllers();

        OnAssetRequested();
        CRY_ASSERT(!m_pStream);

        GlobalAnimationHeaderCAFStreamContent* pContent = new GlobalAnimationHeaderCAFStreamContent;
        pContent->m_id = (int)std::distance(g_AnimationManager.m_arrGlobalCAF.begin(), this);
        pContent->m_pPath = GetFilePath();
        pContent->m_nFlags = m_nFlags;
        pContent->m_FilePathDBACRC32 = m_FilePathDBACRC32;

        // start streaming
        StreamReadParams params;
        params.dwUserData = 0;
        params.nSize = 0;
        params.pBuffer = NULL;
        params.nLoadTime = 10000;
        params.nMaxLoadTime = 1000;
        params.ePriority = estpAboveNormal;
        m_pStream = g_pISystem->GetStreamEngine()->StartRead(eStreamTaskTypeAnimation, GetFilePath(), pContent, &params);
    }
    else
    {
        size_t numDBA_Files = g_AnimationManager.m_arrGlobalHeaderDBA.size();
        for (uint32 d = 0; d < numDBA_Files; d++)
        {
            CGlobalHeaderDBA& pGlobalHeaderDBA = g_AnimationManager.m_arrGlobalHeaderDBA[d];
            if (m_FilePathDBACRC32 != pGlobalHeaderDBA.m_FilePathDBACRC32)
            {
                continue;
            }
            pGlobalHeaderDBA.m_nLastUsedTimeDelta = 0;
            if (pGlobalHeaderDBA.m_pDatabaseInfo == 0)
            {
                pGlobalHeaderDBA.StartStreamingDBA(false);
            }
            else
            {
                m_nControllers  = m_nControllers2;
            }

            break;
        }
    }
}

GlobalAnimationHeaderCAFStreamContent::GlobalAnimationHeaderCAFStreamContent()
    : m_fSampleRate(ANIMATION_30Hz)
{
}

GlobalAnimationHeaderCAFStreamContent::~GlobalAnimationHeaderCAFStreamContent()
{
    if (m_hControllerData.IsValid())
    {
        g_controllerHeap.Free(m_hControllerData);
        m_hControllerData = CControllerDefragHdl();
    }
}

void* GlobalAnimationHeaderCAFStreamContent::StreamOnNeedStorage(IReadStream* pStream, unsigned nSize, bool& bAbortOnFailToAlloc)
{
    char* pData = NULL;

    if (static_cast<int>(nSize) >= Console::GetInst().ca_MinInPlaceCAFStreamSize)
    {
        MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_CAF, 0, m_pPath);

        m_hControllerData = g_controllerHeap.AllocPinned(nSize, NULL);
        if (m_hControllerData.IsValid())
        {
            pData = (char*)g_controllerHeap.WeakPin(m_hControllerData);
        }
    }

    return pData;
}

void GlobalAnimationHeaderCAFStreamContent::StreamAsyncOnComplete(IReadStream* pStream, unsigned nError)
{
    MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_CAF, 0, m_pPath);

    if (pStream->IsError())
    {
        return;
    }

    _smart_ptr<IChunkFile> pChunkFile = g_pI3DEngine->CreateChunkFile(true);
    if (!pChunkFile->ReadFromMemory(pStream->GetBuffer(), pStream->GetBytesRead()))
    {
        pStream->FreeTemporaryMemory();
        return;
    }

    bool bLoadOldChunks = false;
    bool bLoadInPlace = false;
    size_t nUsefulSize = 0;
    ParseChunkHeaders(pChunkFile, bLoadOldChunks, bLoadInPlace, nUsefulSize);

    CControllerDefragHdl hStorage;
    if (!bLoadInPlace)
    {
        hStorage = g_controllerHeap.AllocPinned(nUsefulSize, NULL);
    }

    uint32 numChunks = pChunkFile->NumChunks();

    char* pStorage = (char*)(hStorage.IsValid() ? g_controllerHeap.WeakPin(hStorage) : NULL);
    IControllerRelocatableChain* pRelocateHead = NULL;
    if (ParseChunkRange(pChunkFile, 0, numChunks, bLoadOldChunks, pStorage, pRelocateHead))
    {
        if (!bLoadInPlace)
        {
            if (m_hControllerData.IsValid())
            {
                g_controllerHeap.Free(m_hControllerData);
            }

            m_hControllerData = hStorage;
        }

        g_controllerHeap.ChangeContext(m_hControllerData, pRelocateHead);
    }

    pStream->FreeTemporaryMemory();
}

void GlobalAnimationHeaderCAFStreamContent::StreamOnComplete(IReadStream* pStream, unsigned nError)
{
    DEFINE_PROFILER_FUNCTION();
    LOADING_TIME_PROFILE_SECTION(g_pISystem);

    GlobalAnimationHeaderCAF& caf = g_AnimationManager.m_arrGlobalCAF[m_id];

    if (pStream->IsError())
    {
        // file was not loaded successfully
        caf.FinishLoading(nError);
    }
    else
    {
        caf.CommitContent(*this);
        caf.FinishLoading(0);
    }

    delete this;
}

void GlobalAnimationHeaderCAF::FinishLoading(unsigned nError)
{
    m_pStream.reset();

    if (nError)
    {
        g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING,  VALIDATOR_FLAG_FILE, GetFilePath(), "Failed to parse CAF-file %x", nError);
        m_nFlags = 0;
        OnAssetNotFound();
        return;  //error
    }

    //----------------------------------------------------------------------------------------------

    ControllerInit();
    if (g_pCharacterManager->m_pStreamingListener)
    {
        int32 globalID = g_AnimationManager.GetGlobalIDbyFilePath_CAF(GetFilePath());
        g_pCharacterManager->m_pStreamingListener->NotifyAnimLoaded(globalID);
    }
#ifndef _RELEASE
    if (Console::GetInst().ca_DebugAnimUsageOnFileAccess)
    {
        g_AnimationManager.DebugAnimUsage(0);
    }
#endif
}


void GlobalAnimationHeaderCAF::ClearControllers()
{
    if (m_pStream)
    {
        m_pStream->Abort();
        m_pStream.reset();
    }

    if (m_FilePathDBACRC32 == 0 || m_FilePathDBACRC32 == -1)
    {
        ClearAssetRequested();
        m_nControllers = 0;
        m_arrController.clear();
        m_arrControllerLookupVector.clear();
#ifndef _RELEASE
        if (Console::GetInst().ca_DebugAnimUsageOnFileAccess)
        {
            g_AnimationManager.DebugAnimUsage(0);
        }
#endif
    }
    ClearControllerData();
}

void GlobalAnimationHeaderCAF::ClearControllerData()
{
    if (m_hControllerData.IsValid())
    {
        g_controllerHeap.Free(m_hControllerData);
        m_hControllerData = CControllerDefragHdl();
    }
}

bool GlobalAnimationHeaderCAFStreamContent::ParseChunkRange(IChunkFile* pChunkFile, uint32 min, uint32 max, bool bLoadOldChunks, char*& pStorage, IControllerRelocatableChain*& pRelocateHead)
{
    if ((int)max > pChunkFile->NumChunks())
    {
        g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, m_pPath, "GlobalAnimationHeaderCAFStreamContent::ParseChunkRange: number of chunks is out of range");
        max = pChunkFile->NumChunks();
    }

    //first we initialize the controllers
    for (uint32 i = min; i < max; i++)
    {
        IChunkFile::ChunkDesc* const pChunkDesc = pChunkFile->GetChunk(i);
        switch (pChunkDesc->chunkType)
        {
        case ChunkType_MotionParameters:
            if (!ReadMotionParameters(pChunkDesc))
            {
                return false;
            }
            break;

        case ChunkType_Timing:
            if (!ReadTiming(pChunkDesc))
            {
                return false;
            }
            break;

        case ChunkType_Controller:
            if (!ReadController(pChunkDesc, bLoadOldChunks, pStorage, pRelocateHead))
            {
                return false;
            }
            break;
        }
    }
    return true;
}


uint32 GlobalAnimationHeaderCAF::DoesExistCAF()
{
    LOADING_TIME_PROFILE_SECTION(GetISystem());
    _smart_ptr<IChunkFile> pChunkFile = g_pI3DEngine->CreateChunkFile(true);
    if (!pChunkFile->Read(m_FilePath))
    {
        return 0;
    }

    return 1;
}


void GlobalAnimationHeaderCAF::ControllerInit()
{
    uint32 numController = m_arrController.size();
    CRY_ASSERT(numController);
    std::sort(m_arrController.begin(),  m_arrController.end(), AnimCtrlSortPred());
    InitControllerLookup(numController);
    m_nControllers  = numController;
    m_nControllers2 = numController;
    if (m_nControllers2 == 0)
    {
        CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "CryAnimation CAF: Assets has no controllers. Probably compressed to death: %s", GetFilePath());
    }

    ClearAssetRequested();
    ClearAssetNotFound();
    OnAssetCreated();
}

uint32 GlobalAnimationHeaderCAFStreamContent::ParseChunkHeaders(IChunkFile* pChunkFile, bool& bLoadOldChunksOut, bool& bLoadInPlaceOut, size_t& nUsefulSizeOut)
{
    // Load Nodes.
    uint32 numChunck = pChunkFile->NumChunks();
    uint32 numOldControllers = 0;
    uint32 numNewControllers = 0;
    uint32 bLoadOldChunks = 0;
    bool bLoadInPlace = m_hControllerData.IsValid();
    size_t nUsefulSize = 0;

    for (uint32 i = 0; i < numChunck; i++)
    {
        const IChunkFile::ChunkDesc* const pChunkDesc = pChunkFile->GetChunk(i);

        if (pChunkDesc->chunkType == ChunkType_Controller)
        {
            if (pChunkDesc->chunkVersion == CONTROLLER_CHUNK_DESC_0826::VERSION)
            {
                CryFatalError("TCB chunks");
                continue;
            }
            if (pChunkDesc->chunkVersion == CONTROLLER_CHUNK_DESC_0827::VERSION)
            {
                numOldControllers++;
                continue;
            }
            if (pChunkDesc->chunkVersion == CONTROLLER_CHUNK_DESC_0828::VERSION)
            {
                numOldControllers++;
                continue;
            }


            if (pChunkDesc->chunkVersion == CONTROLLER_CHUNK_DESC_0829::VERSION)
            {
                if (pChunkDesc->bSwapEndian)
                {
                    CryFatalError("%s: data are stored in non-native endian format", __FUNCTION__);
                }

                CONTROLLER_CHUNK_DESC_0829* pCtrlChunk = (CONTROLLER_CHUNK_DESC_0829*)pChunkDesc->data;

                if (!pCtrlChunk->TracksAligned)
                {
                    bLoadInPlace = false;
                }

                if (pCtrlChunk->numRotationKeys)
                {
                    nUsefulSize += Align(ControllerHelper::GetRotationFormatSizeOf(pCtrlChunk->RotationFormat) * pCtrlChunk->numRotationKeys, 4);
                    nUsefulSize += Align(ControllerHelper::GetKeyTimesFormatSizeOf(pCtrlChunk->RotationTimeFormat) * pCtrlChunk->numRotationKeys, 4);
                }

                if (pCtrlChunk->numPositionKeys)
                {
                    nUsefulSize += Align(ControllerHelper::GetPositionsFormatSizeOf(pCtrlChunk->PositionFormat) * pCtrlChunk->numPositionKeys, 4);
                    if (pCtrlChunk->PositionKeysInfo != CONTROLLER_CHUNK_DESC_0829::eKeyTimeRotation)
                    {
                        nUsefulSize += Align(ControllerHelper::GetKeyTimesFormatSizeOf(pCtrlChunk->PositionTimeFormat) * pCtrlChunk->numPositionKeys, 4);
                    }
                }

                numNewControllers++;
                continue;
            }

            if (pChunkDesc->chunkVersion == CONTROLLER_CHUNK_DESC_0830::VERSION)
            {
                numOldControllers++;
                continue;
            }
            if (pChunkDesc->chunkVersion == CONTROLLER_CHUNK_DESC_0831::VERSION)
            {
                if (pChunkDesc->bSwapEndian)
                {
                    CryFatalError("%s: data are stored in non-native endian format", __FUNCTION__);
                }

                CONTROLLER_CHUNK_DESC_0831* pCtrlChunk = (CONTROLLER_CHUNK_DESC_0831*)pChunkDesc->data;

                if (!pCtrlChunk->TracksAligned)
                {
                    bLoadInPlace = false;
                }

                if (pCtrlChunk->numRotationKeys)
                {
                    nUsefulSize += Align(ControllerHelper::GetRotationFormatSizeOf(pCtrlChunk->RotationFormat) * pCtrlChunk->numRotationKeys, 4);
                    nUsefulSize += Align(ControllerHelper::GetKeyTimesFormatSizeOf(pCtrlChunk->RotationTimeFormat) * pCtrlChunk->numRotationKeys, 4);
                }

                if (pCtrlChunk->numPositionKeys)
                {
                    nUsefulSize += Align(ControllerHelper::GetPositionsFormatSizeOf(pCtrlChunk->PositionFormat) * pCtrlChunk->numPositionKeys, 4);
                    if (pCtrlChunk->PositionKeysInfo != CONTROLLER_CHUNK_DESC_0831::eKeyTimeRotation)
                    {
                        nUsefulSize += Align(ControllerHelper::GetKeyTimesFormatSizeOf(pCtrlChunk->PositionTimeFormat) * pCtrlChunk->numPositionKeys, 4);
                    }
                }

                nUsefulSize += Align(pCtrlChunk->nFlags, 4);

                numNewControllers++;
                continue;
            }

            CryFatalError("Unsupported controller chunk version: 0x%04x", pChunkDesc->chunkVersion);
        }
    }

    if (numNewControllers == 0)
    {
        bLoadOldChunks = true;
    }

    if (bLoadOldChunks)
    {
        bLoadInPlace = false;
    }

    if (numOldControllers)
    {
        bLoadInPlace = false;
    }

    if (Console::GetInst().ca_AnimWarningLevel > 2 && numOldControllers)
    {
        if (Console::GetInst().ca_UseIMG_CAF)
        {
            g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING,  VALIDATOR_FLAG_FILE, m_pPath,   "Animation file has uncompressed data");
        }
    }
    if (Console::GetInst().ca_AnimWarningLevel > 2 && numNewControllers == 0)
    {
        if (Console::GetInst().ca_UseIMG_CAF)
        {
            g_pISystem->Warning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING,  VALIDATOR_FLAG_FILE, m_pPath,   "Animation file has no compressed data");
        }
    }

    // Only old data will be loaded.
    uint32 numController = 0;
    if (bLoadOldChunks)
    {
        numController = numOldControllers;
    }
    else
    {
        numController = numNewControllers;
    }

    m_arrController.reserve(numController);
    m_arrController.resize(0);

    bLoadOldChunksOut = bLoadOldChunks != 0;
    bLoadInPlaceOut = bLoadInPlace;
    nUsefulSizeOut = nUsefulSize;

    return numController;
}




//========================================
//SpeedInfo Header
//========================================
struct SPEED_CHUNK_DESC
{
    enum
    {
        VERSION = 0x0920
    };

    float           Speed;
    float           Distance;
    float           Slope;
    int       Looped;

    AUTO_STRUCT_INFO
};

struct SPEED_CHUNK_DESC_1
{
    enum
    {
        VERSION = 0x0921
    };

    float           Speed;
    float           Distance;
    float           Slope;
    int       Looped;
    f32             MoveDir[3];
    AUTO_STRUCT_INFO
};

//////////////////////////////////////////////////////////////////////////
bool GlobalAnimationHeaderCAFStreamContent::ReadTiming(IChunkFile::ChunkDesc* pChunkDesc)
{
    TIMING_CHUNK_DESC_0918* const pChunk = (TIMING_CHUNK_DESC_0918*)pChunkDesc->data;
    SwapEndian(*pChunk, pChunkDesc->bSwapEndian);
    pChunkDesc->bSwapEndian = false;

    int32 nStartKey = pChunk->global_range.start;
    int32 nEndKey   = pChunk->global_range.end;
    // Make time zero-based
    nEndKey -= nStartKey;
    nStartKey = 0;
    m_fSampleRate = 1.f / (pChunk->m_SecsPerTick * pChunk->m_TicksPerFrame);
    m_fStartSec   = nStartKey / GetSampleRate();
    m_fEndSec     = nEndKey / GetSampleRate();
    if (m_fEndSec <= m_fStartSec)
    {
        m_fEndSec = m_fStartSec;
    }
    m_fTotalDuration = m_fEndSec - m_fStartSec;

    return 1;
}


//////////////////////////////////////////////////////////////////////////
bool GlobalAnimationHeaderCAFStreamContent::ReadMotionParameters(IChunkFile::ChunkDesc* pChunkDesc)
{
    if (pChunkDesc->chunkVersion == SPEED_CHUNK_DESC::VERSION)
    {
        CryFatalError("SPEED_CHUNK_DESC not supported any more");
        return true;
    }

    if (pChunkDesc->chunkVersion == SPEED_CHUNK_DESC_1::VERSION)
    {
        CryFatalError("SPEED_CHUNK_DESC_1 not supported any more");
        return true;
    }

    if (pChunkDesc->chunkVersion == SPEED_CHUNK_DESC_2::VERSION)
    {
        //  CryFatalError("SPEED_CHUNK_DESC_2 not supported any more");
        SPEED_CHUNK_DESC_2* pChunk = (SPEED_CHUNK_DESC_2*)pChunkDesc->data;
        SwapEndian(*pChunk, pChunkDesc->bSwapEndian);
        pChunkDesc->bSwapEndian = false;
        m_nFlags = FlagsSanityFilter(pChunk->AnimFlags) | (m_nFlags & (CA_ASSET_REQUESTED));
        m_FilePathDBACRC32  =   0; //if 0, then this is a streamable CAF file

        m_StartLocation = pChunk->StartPosition;
        return true;
    }


    if (pChunkDesc->chunkVersion == CHUNK_MOTION_PARAMETERS::VERSION)
    {
        if (pChunkDesc->bSwapEndian)
        {
            CryFatalError("%s: data are stored in non-native endian format", __FUNCTION__);
        }

        CHUNK_MOTION_PARAMETERS* pChunk = (CHUNK_MOTION_PARAMETERS*)pChunkDesc->data;

        m_nFlags            = FlagsSanityFilter(pChunk->mp.m_nAssetFlags) | (m_nFlags & (CA_ASSET_REQUESTED));

        // this had to be done for DBAs as well, apparently some
        // assets dont have the CREATED flag set...
        m_nFlags |= CA_ASSET_CREATED;

        m_FilePathDBACRC32  = 0; //if 0, then this is a streamable CAF file
        uint32 nCompression = pChunk->mp.m_nCompression;
        int32 nStartKey     = pChunk->mp.m_nStart;
        int32 nEndKey       = pChunk->mp.m_nEnd;
        if (pChunk->mp.m_nAssetFlags & CA_ASSET_ADDITIVE)
        {
            nStartKey++;
        }

        m_fSampleRate = 1.f / (pChunk->mp.m_fSecsPerTick * pChunk->mp.m_nTicksPerFrame);
        m_fStartSec   = nStartKey / GetSampleRate();
        m_fEndSec     = nEndKey / GetSampleRate();
        if (m_fEndSec <= m_fStartSec)
        {
            m_fEndSec  = m_fStartSec;
        }
        m_fTotalDuration = m_fEndSec - m_fStartSec;

        m_StartLocation = pChunk->mp.m_StartLocation;
        return true;
    }


    return true;
}



//////////////////////////////////////////////////////////////////////////
bool GlobalAnimationHeaderCAFStreamContent::ReadController (IChunkFile::ChunkDesc* pChunkDesc, bool bLoadOldChunks, char*& pStorage, IControllerRelocatableChain*& pRelocateHead)
{
    if (pChunkDesc->chunkVersion == CONTROLLER_CHUNK_DESC_0826::VERSION)
    {
        CryFatalError("CONTROLLER_CHUNK_DESC_0826");
        return false;
    }


    if (pChunkDesc->chunkVersion == CONTROLLER_CHUNK_DESC_0827::VERSION)
    {
        if (bLoadOldChunks)
        {
            CONTROLLER_CHUNK_DESC_0827* pCtrlChunk = (CONTROLLER_CHUNK_DESC_0827*)pChunkDesc->data;
            const bool bSwapEndianness = pChunkDesc->bSwapEndian;
            SwapEndian(*pCtrlChunk, bSwapEndianness);
            pChunkDesc->bSwapEndian = false;

            size_t numKeys = pCtrlChunk->numKeys;

            CryKeyPQLog* pCryKey = (CryKeyPQLog*)(pCtrlChunk + 1);
            SwapEndian(pCryKey, numKeys, bSwapEndianness);

            CControllerPQLog* pController = new CControllerPQLog;
            pController->m_nControllerId    = pCtrlChunk->nControllerId;

            pController->m_arrKeys.resize(numKeys);
            pController->m_arrTimes.resize(numKeys);

            const int startTime = (numKeys > 0) ? pCryKey[0].nTime : 0;
            for (size_t i = 0; i < numKeys; ++i)
            {
                pController->m_arrTimes[i]  = (pCryKey[i].nTime - startTime) / TICKS_CONVERT;
                pController->m_arrKeys[i].vPos      = pCryKey[i].vPos / 100.0f;
                pController->m_arrKeys[i].vRotLog   = pCryKey[i].vRotLog;
            }
            m_arrController.push_back(pController);
        }

        return true;
    }

    if (pChunkDesc->chunkVersion == CONTROLLER_CHUNK_DESC_0828::VERSION)
    {
        CryFatalError("CONTROLLER_CHUNK_DESC_0828 is not supported anymore");
        return true;
    }


    //--------------------------------------------------------------

    if (pChunkDesc->chunkVersion == CONTROLLER_CHUNK_DESC_0829::VERSION)
    {
        if (bLoadOldChunks)
        {
            return true;
        }

        if (pChunkDesc->bSwapEndian)
        {
            CryFatalError("%s: data are stored in non-native endian format", __FUNCTION__);
        }

        CONTROLLER_CHUNK_DESC_0829* pCtrlChunk = (CONTROLLER_CHUNK_DESC_0829*)pChunkDesc->data;

        CController* pController = new CController;
        pController->m_nControllerId = pCtrlChunk->nControllerId;
        m_arrController.push_back(pController);

        char* pData = (char*)(pCtrlChunk + 1);

        _smart_ptr<IKeyTimesInformation> pRotTimeKeys;
        _smart_ptr<IKeyTimesInformation> pPosTimeKeys;

        RotationControllerPtr pRotation;
        PositionControllerPtr pPosition;

        uint32 trackAlignment = pCtrlChunk->TracksAligned ? 4 : 1;

        if (pCtrlChunk->numRotationKeys)
        {
            // we have a rotation info

            pRotation =  RotationControllerPtr(new RotationTrackInformation);

            size_t rawRotationSize = ControllerHelper::GetRotationFormatSizeOf(pCtrlChunk->RotationFormat) * pCtrlChunk->numRotationKeys;

            if (pStorage)
            {
                memcpy(pStorage, pData, rawRotationSize);
            }

            ITrackRotationStorage* pTrackRotStorage = ControllerHelper::GetRotationControllerPtr(pCtrlChunk->RotationFormat, pStorage ? pStorage : pData, pCtrlChunk->numRotationKeys);
            if (!pTrackRotStorage)
            {
                return false;
            }

            pData += Align(rawRotationSize, trackAlignment);
            if (pStorage)
            {
                pStorage += Align(rawRotationSize, 4);
            }

            pRotation->SetRotationStorage(pTrackRotStorage);

            pTrackRotStorage->LinkTo(pRelocateHead);
            pRelocateHead = pTrackRotStorage;

            size_t rawTimeSize = ControllerHelper::GetKeyTimesFormatSizeOf(pCtrlChunk->RotationTimeFormat) * pCtrlChunk->numRotationKeys;

            if (pStorage)
            {
                memcpy(pStorage, pData, rawTimeSize);
            }

            pRotTimeKeys = ControllerHelper::GetKeyTimesControllerPtr(pCtrlChunk->RotationTimeFormat, pStorage ? pStorage : pData, pCtrlChunk->numRotationKeys);
            if (!pRotTimeKeys)
            {
                return false;
            }

            pData += Align(rawTimeSize, trackAlignment);
            if (pStorage)
            {
                pStorage += Align(rawTimeSize, 4);
            }

            pRotation->SetKeyTimesInformation(pRotTimeKeys);

            IControllerRelocatableChain* pRotTimeKeysReloc = pRotTimeKeys->GetRelocateChain();
            if (pRotTimeKeysReloc)
            {
                pRotTimeKeysReloc->LinkTo(pRelocateHead);
                pRelocateHead = pRotTimeKeysReloc;
            }

            pController->SetRotationController(pRotation);
        }

        if (pCtrlChunk->numPositionKeys)
        {
            pPosition = PositionControllerPtr(new PositionTrackInformation);

            size_t posSize = ControllerHelper::GetPositionsFormatSizeOf(pCtrlChunk->PositionFormat) * pCtrlChunk->numPositionKeys;

            if (pStorage)
            {
                memcpy(pStorage, pData, posSize);
            }

            TrackPositionStoragePtr pTrackPosStorage = ControllerHelper::GetPositionControllerPtr(pCtrlChunk->PositionFormat, pStorage ? pStorage : pData, pCtrlChunk->numPositionKeys);
            if (!pTrackPosStorage)
            {
                return false;
            }

            pData += Align(posSize, trackAlignment);
            if (pStorage)
            {
                pStorage += Align(posSize, 4);
            }

            pPosition->SetPositionStorage(pTrackPosStorage);

            pTrackPosStorage->LinkTo(pRelocateHead);
            pRelocateHead = pTrackPosStorage;

            if (pCtrlChunk->PositionKeysInfo == CONTROLLER_CHUNK_DESC_0829::eKeyTimeRotation)
            {
                pPosition->SetKeyTimesInformation(pRotTimeKeys);
            }
            else
            {
                size_t keySize = ControllerHelper::GetKeyTimesFormatSizeOf(pCtrlChunk->PositionTimeFormat) * pCtrlChunk->numPositionKeys;

                if (pStorage)
                {
                    memcpy(pStorage, pData, keySize);
                }

                // load from chunk
                pPosTimeKeys = ControllerHelper::GetKeyTimesControllerPtr(pCtrlChunk->PositionTimeFormat, pStorage ? pStorage : pData, pCtrlChunk->numPositionKeys);

                pData += Align(keySize, trackAlignment);
                if (pStorage)
                {
                    pStorage += Align(keySize, 4);
                }

                pPosition->SetKeyTimesInformation(pPosTimeKeys);

                IControllerRelocatableChain* pPosTimeKeysReloc = pPosTimeKeys->GetRelocateChain();
                if (pPosTimeKeysReloc)
                {
                    pPosTimeKeysReloc->LinkTo(pRelocateHead);
                    pRelocateHead = pPosTimeKeysReloc;
                }
            }

            pController->SetPositionController(pPosition);
        }

        return true;
    }

    if (pChunkDesc->chunkVersion == CONTROLLER_CHUNK_DESC_0830::VERSION)
    {
        if (bLoadOldChunks)
        {
            CONTROLLER_CHUNK_DESC_0830* pCtrlChunk = (CONTROLLER_CHUNK_DESC_0830*)pChunkDesc->data;
            const bool bSwapEndianness = pChunkDesc->bSwapEndian;
            SwapEndian(*pCtrlChunk, bSwapEndianness);
            pChunkDesc->bSwapEndian = false;

            size_t numKeys = pCtrlChunk->numKeys;

            CryKeyPQLog* pCryKey = (CryKeyPQLog*)(pCtrlChunk + 1);
            SwapEndian(pCryKey, numKeys, bSwapEndianness);

            CControllerPQLog* pController = new CControllerPQLog;
            pController->m_nControllerId = pCtrlChunk->nControllerId;
            pController->m_nFlags = pCtrlChunk->nFlags;

            pController->m_arrKeys.resize(numKeys);
            pController->m_arrTimes.resize(numKeys);

            const int startTime = (numKeys > 0) ? pCryKey[0].nTime : 0;
            for (size_t i = 0; i < numKeys; ++i)
            {
                pController->m_arrTimes[i] = (pCryKey[i].nTime - startTime) / TICKS_CONVERT;
                pController->m_arrKeys[i].vPos = pCryKey[i].vPos / 100.0f;
                pController->m_arrKeys[i].vRotLog = pCryKey[i].vRotLog;
            }
            m_arrController.push_back(pController);
        }

        return true;
    }

    if (pChunkDesc->chunkVersion == CONTROLLER_CHUNK_DESC_0831::VERSION)
    {
        if (bLoadOldChunks)
        {
            return true;
        }

        if (pChunkDesc->bSwapEndian)
        {
            CryFatalError("%s: data are stored in non-native endian format", __FUNCTION__);
        }

        CONTROLLER_CHUNK_DESC_0831* pCtrlChunk = (CONTROLLER_CHUNK_DESC_0831*)pChunkDesc->data;

        CController* pController = new CController;
        pController->m_nControllerId = pCtrlChunk->nControllerId;
        pController->m_nFlags = pCtrlChunk->nFlags;
        m_arrController.push_back(pController);

        char* pData = (char*)(pCtrlChunk + 1);

        _smart_ptr<IKeyTimesInformation> pRotTimeKeys;
        _smart_ptr<IKeyTimesInformation> pPosTimeKeys;

        RotationControllerPtr pRotation;
        PositionControllerPtr pPosition;

        uint32 trackAlignment = pCtrlChunk->TracksAligned ? 4 : 1;

        if (pCtrlChunk->numRotationKeys)
        {
            // we have a rotation info

            pRotation = RotationControllerPtr(new RotationTrackInformation);

            size_t rawRotationSize = ControllerHelper::GetRotationFormatSizeOf(pCtrlChunk->RotationFormat) * pCtrlChunk->numRotationKeys;

            if (pStorage)
            {
                memcpy(pStorage, pData, rawRotationSize);
            }

            ITrackRotationStorage* pTrackRotStorage = ControllerHelper::GetRotationControllerPtr(pCtrlChunk->RotationFormat, pStorage ? pStorage : pData, pCtrlChunk->numRotationKeys);
            if (!pTrackRotStorage)
            {
                return false;
            }

            pData += Align(rawRotationSize, trackAlignment);
            if (pStorage)
            {
                pStorage += Align(rawRotationSize, 4);
            }

            pRotation->SetRotationStorage(pTrackRotStorage);

            pTrackRotStorage->LinkTo(pRelocateHead);
            pRelocateHead = pTrackRotStorage;

            size_t rawTimeSize = ControllerHelper::GetKeyTimesFormatSizeOf(pCtrlChunk->RotationTimeFormat) * pCtrlChunk->numRotationKeys;

            if (pStorage)
            {
                memcpy(pStorage, pData, rawTimeSize);
            }

            pRotTimeKeys = ControllerHelper::GetKeyTimesControllerPtr(pCtrlChunk->RotationTimeFormat, pStorage ? pStorage : pData, pCtrlChunk->numRotationKeys);
            if (!pRotTimeKeys)
            {
                return false;
            }

            pData += Align(rawTimeSize, trackAlignment);
            if (pStorage)
            {
                pStorage += Align(rawTimeSize, 4);
            }

            pRotation->SetKeyTimesInformation(pRotTimeKeys);

            IControllerRelocatableChain* pRotTimeKeysReloc = pRotTimeKeys->GetRelocateChain();
            if (pRotTimeKeysReloc)
            {
                pRotTimeKeysReloc->LinkTo(pRelocateHead);
                pRelocateHead = pRotTimeKeysReloc;
            }

            pController->SetRotationController(pRotation);
        }

        if (pCtrlChunk->numPositionKeys)
        {
            pPosition = PositionControllerPtr(new PositionTrackInformation);

            size_t posSize = ControllerHelper::GetPositionsFormatSizeOf(pCtrlChunk->PositionFormat) * pCtrlChunk->numPositionKeys;

            if (pStorage)
            {
                memcpy(pStorage, pData, posSize);
            }

            TrackPositionStoragePtr pTrackPosStorage = ControllerHelper::GetPositionControllerPtr(pCtrlChunk->PositionFormat, pStorage ? pStorage : pData, pCtrlChunk->numPositionKeys);
            if (!pTrackPosStorage)
            {
                return false;
            }

            pData += Align(posSize, trackAlignment);
            if (pStorage)
            {
                pStorage += Align(posSize, 4);
            }

            pPosition->SetPositionStorage(pTrackPosStorage);

            pTrackPosStorage->LinkTo(pRelocateHead);
            pRelocateHead = pTrackPosStorage;

            if (pCtrlChunk->PositionKeysInfo == CONTROLLER_CHUNK_DESC_0831::eKeyTimeRotation)
            {
                pPosition->SetKeyTimesInformation(pRotTimeKeys);
            }
            else
            {
                size_t keySize = ControllerHelper::GetKeyTimesFormatSizeOf(pCtrlChunk->PositionTimeFormat) * pCtrlChunk->numPositionKeys;

                if (pStorage)
                {
                    memcpy(pStorage, pData, keySize);
                }

                // load from chunk
                pPosTimeKeys = ControllerHelper::GetKeyTimesControllerPtr(pCtrlChunk->PositionTimeFormat, pStorage ? pStorage : pData, pCtrlChunk->numPositionKeys);

                pData += Align(keySize, trackAlignment);
                if (pStorage)
                {
                    pStorage += Align(keySize, 4);
                }

                pPosition->SetKeyTimesInformation(pPosTimeKeys);

                IControllerRelocatableChain* pPosTimeKeysReloc = pPosTimeKeys->GetRelocateChain();
                if (pPosTimeKeysReloc)
                {
                    pPosTimeKeysReloc->LinkTo(pRelocateHead);
                    pRelocateHead = pPosTimeKeysReloc;
                }
            }

            pController->SetPositionController(pPosition);
        }

        return true;
    }

    return false;
}

void GlobalAnimationHeaderCAF::CommitContent(GlobalAnimationHeaderCAFStreamContent& content)
{
    using std::swap;

    m_nFlags = content.m_nFlags;
    m_FilePathDBACRC32 = content.m_FilePathDBACRC32;

    m_fSampleRate = content.m_fSampleRate;
    m_fStartSec = content.m_fStartSec;
    m_fEndSec = content.m_fEndSec;
    m_fTotalDuration = content.m_fTotalDuration;

    m_StartLocation = content.m_StartLocation;

    m_arrController.swap(content.m_arrController);
    swap(m_hControllerData, content.m_hControllerData);

    if (m_hControllerData.IsValid())
    {
        g_controllerHeap.Unpin(m_hControllerData);
    }
}

uint32 GlobalAnimationHeaderCAFStreamContent::FlagsSanityFilter(uint32 flags)
{
    // sanity check
    uint32 validFlags =  (CA_ASSET_BIG_ENDIAN | CA_ASSET_CREATED | CA_ASSET_ONDEMAND | CA_ASSET_LOADED | CA_ASSET_ADDITIVE | CA_ASSET_CYCLE | CA_ASSET_NOT_FOUND);
    if (flags & ~validFlags)
    {
        CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_WARNING, "Badly exported animation-asset: flags: %08x %s", flags, m_pPath);
        flags &= validFlags;
    }
    return flags;
}

uint32 GlobalAnimationHeaderCAF::GetTotalPosKeys() const
{
    uint32 pos = 0;
    for (uint32 i = 0; i < m_nControllers; i++)
    {
        pos += (m_arrController[i]->GetPositionKeysNum() != 0);
    }
    return pos;
}

uint32 GlobalAnimationHeaderCAF::GetTotalRotKeys() const
{
    uint32 rot = 0;
    for (uint32 i = 0; i < m_nControllers; i++)
    {
        rot += (m_arrController[i]->GetRotationKeysNum() != 0);
    }
    return rot;
}

#ifdef EDITOR_PCDEBUGCODE
bool GlobalAnimationHeaderCAF::Export2HTR(const char* szAnimationName, const char* savePath, const CDefaultSkeleton* pDefaultSkeleton)
{
    std::vector<string> jointNameArray;
    std::vector<string> jointParentArray;

    const QuatT* parrDefJoints = pDefaultSkeleton->m_poseDefaultData.GetJointsRelative();
    uint32 numJoints = pDefaultSkeleton->m_arrModelJoints.size();
    for (uint32 j = 0; j < numJoints; j++)
    {
        const CDefaultSkeleton::SJoint* pJoint  = &pDefaultSkeleton->m_arrModelJoints[j];
        CRY_ASSERT(pJoint);
        jointNameArray.push_back(pJoint->m_strJointName.c_str());

        int16 parentID = pJoint->m_idxParent;
        if (parentID == -1)
        {
            jointParentArray.push_back("INVALID");
        }
        else
        {
            const CDefaultSkeleton::SJoint* parentJoint = &pDefaultSkeleton->m_arrModelJoints[parentID];
            if (parentJoint)
            {
                jointParentArray.push_back(parentJoint->m_strJointName.c_str());
            }
        }
    }


    //fetch animation
    std::vector< DynArray<QuatT> > arrAnimation;
    f32 fFrames = m_fTotalDuration * GetSampleRate();
    uint32 nFrames = uint32(fFrames + 1.5f); //roundup
    f32 timestep = 1.0f / f32(nFrames - 1);

    arrAnimation.resize(numJoints);
    for (uint32 j = 0; j < numJoints; j++)
    {
        arrAnimation[j].resize(nFrames);
    }

    for (uint32 j = 0; j < numJoints; j++)
    {
        const CDefaultSkeleton::SJoint* pJoint  = &pDefaultSkeleton->m_arrModelJoints[j];
        CRY_ASSERT(pJoint);
        IController* pController = GetControllerByJointCRC32(pJoint->m_nJointCRC32);
        f32 t = 0.0f;
        for (uint32 k = 0; k < nFrames; k++)
        {
            QuatT qt = parrDefJoints[j];
            if (pController)
            {
                // For the last frame, due to floating point computation, the value sometimes ends up being slightly higher than 1, causing a spurious assert in NTime2KTime function. 
                // To avoid that assert, clamping to 1.
                if (t > 1.0f)
                    t = 1.0f;
                pController->GetOP(NTime2KTime(t), qt.q, qt.t);
            }
            arrAnimation[j][k] = qt;
            t += timestep;
        }
    }

    bool htr = SaveHTR(szAnimationName, savePath, jointNameArray, jointParentArray, arrAnimation, parrDefJoints);
    bool caf = SaveICAF(szAnimationName, savePath, jointNameArray, arrAnimation, GetSampleRate());
    return htr;
}
#endif


// replace ' ' with '_'
void ChangeBoneName(const char* name, char newName[])
{
    int lenName = strlen(name);
    memcpy(newName, name, lenName * sizeof(char));
    for (int i = 0; i < lenName; ++i)
    {
        if (newName[i] == ' ')
        {
            newName[i] = '_';
        }
    }
    newName[lenName] = '\0';
}

#ifdef EDITOR_PCDEBUGCODE

#include "../Cry3DEngine/CGF/ChunkFileWriters.h"

bool GlobalAnimationHeaderCAF::SaveHTR(const char* szAnimationName, const char* savePath, const std::vector<string>& jointNameArray, const std::vector<string>& jointParentArray, const std::vector< DynArray<QuatT> >& arrAnimation, const QuatT* parrDefJoints)
{
    if (jointNameArray.empty() || arrAnimation.empty())
    {
        return false;
    }

    /*CFileDialog dlg(FALSE, "htr", NULL, OFN_OVERWRITEPROMPT, "HTR (*.htr)|*.htr|CAF(*.caf)|*.caf||");

    if(dlg.DoModal() != IDOK)
        return;

    string path = dlg.GetPathName().GetBuffer();*/

    ICryPak* g_pIPak = gEnv->pCryPak;

    string saveName = string(savePath) + szAnimationName + string(".htr");
    AZ::IO::HandleType exportFileHandle = g_pIPak->FOpen(saveName.c_str(), "wt");
    if (exportFileHandle == AZ::IO::InvalidHandle)
    {
        return false;
    }

    uint32 numFrames = arrAnimation[0].size();
    uint32 numBipedJoints = jointNameArray.size();

    //------------------------------------------------------------------------------
    // Export header

    g_pIPak->FPrintf(exportFileHandle, "#Comment line ignore any data following # character \n");
    g_pIPak->FPrintf(exportFileHandle, "#Hierarchical Translation and Rotation (.htr) file \n");
    g_pIPak->FPrintf(exportFileHandle, "[Header]	\n");
    g_pIPak->FPrintf(exportFileHandle, "#Header keywords are followed by a single value \n");
    g_pIPak->FPrintf(exportFileHandle, "FileType HTR		#Single word string\n");
    g_pIPak->FPrintf(exportFileHandle, "DataType HTRS		#Translation followed by rotation and scale data\n");
    g_pIPak->FPrintf(exportFileHandle, "FileVersion 1		#integer\n");

    g_pIPak->FPrintf(exportFileHandle, "NumSegments %d		#integer\n", numBipedJoints);

    g_pIPak->FPrintf(exportFileHandle, "NumFrames %d		#integer\n", numFrames);

    int32 nFrameRate = 30; // ivo are you reading framrate from Vicon? //Xiaomao: make sure if we need to fix it.
    g_pIPak->FPrintf(exportFileHandle, "DataFrameRate %d		#integer, data frame rate in this file\n", nFrameRate);
    g_pIPak->FPrintf(exportFileHandle, "EulerRotationOrder ZYX\n");

    // ivo i cuoldnt find how to specify data in meters in HTR format (since you already divided by 1000).
    // So i will leave mm (millimiters) here and specify 1000 for scale. Result should be the same.
    g_pIPak->FPrintf(exportFileHandle, "CalibrationUnits mm\n");
    g_pIPak->FPrintf(exportFileHandle, "RotationUnits Degrees\n");
    g_pIPak->FPrintf(exportFileHandle, "GlobalAxisofGravity Y\n");
    g_pIPak->FPrintf(exportFileHandle, "BoneLengthAxis Y\n");
    g_pIPak->FPrintf(exportFileHandle, "ScaleFactor 1.00\n"); // ivo see previous note
    g_pIPak->FPrintf(exportFileHandle, "[SegmentNames&Hierarchy]\n");
    g_pIPak->FPrintf(exportFileHandle, "#CHILD	PARENT\n");

    // export the hierarchy
    char newName[128], newNameParent[128];
    for (uint32 v = 0; v < numBipedJoints; v++)
    {
        const char* pBonename = jointNameArray[v];

        ChangeBoneName(pBonename, newName); //replace " " with "_"
        const char* pParentBonename = jointParentArray[v];
        if (string(jointParentArray[v]) != string("INVALID"))
        {
            ChangeBoneName(jointParentArray[v], newNameParent);
        }

        bool bParentValid = (string(pParentBonename) != string("INVALID"));
        string parentName = "GLOBAL";
        if (bParentValid)
        {
            parentName = newNameParent;
        }
        g_pIPak->FPrintf(exportFileHandle, "%s %s\n", newName, parentName.c_str());
    } //v


    g_pIPak->FPrintf(exportFileHandle, "[BasePosition]\n");
    g_pIPak->FPrintf(exportFileHandle, "#SegmentName Tx, Ty, Tz, Rx, Ry, Rz, BoneLength\n");

    //------------------------------------------------------------------------------
    // Export base pose

    //ivo Each line in this section indicates how each bone is initially orientated within
    //it's own local coordinate system.
    for (uint32 v = 0; v < numBipedJoints; v++)
    {
        const char* pBonename = jointNameArray[v];

        ChangeBoneName(pBonename, newName);

        Quat q = parrDefJoints[v].q;
        Vec3 t = parrDefJoints[v].t;

        // ivo do you have bone lenght somewhere after conversion?
        float fBoneLen = 0.0f;
        if (jointParentArray[v] != string("INVALID"))
        {
            fBoneLen = t.GetLength();
        }

        Ang3 Ang = RAD2DEG(Ang3::GetAnglesXYZ(q));

        t.Set(0, 0, 0);
        Ang.Set(0, 0, 0);
        if (v == 0)
        {
            Ang.Set(-90, 0, 0);
        }
        fBoneLen = 10.0f;

        g_pIPak->FPrintf(exportFileHandle, "%s %f %f %f %f %f %f %f \n", newName, t.x, t.y, t.z, Ang.x, Ang.y, Ang.z, fBoneLen);
    } //v

    //------------------------------------------------------------------------------
    // Export motion data joint by joint

    g_pIPak->FPrintf(exportFileHandle, "#Beginning of Data. Separated by tabs\n");
    g_pIPak->FPrintf(exportFileHandle, "#Fr	Tx	Ty	Tz	Rx	Ry	Rz	SF\n");

    for (uint32 v = 0; v < numBipedJoints; v++)
    {
        const char* pBonename = jointNameArray[v];

        ChangeBoneName(pBonename, newName);

        g_pIPak->FPrintf(exportFileHandle, "[%s]\n", newName);
        for (uint32 k = 0; k < numFrames; k++)
        {
            //Ivo do you have bone length somewhere after conversion?
            Quat q = arrAnimation[v][k].q;
            Vec3 t = arrAnimation[v][k].t * 1000.0f;
            float fBoneLen = t.GetLength();
            Ang3 Ang = RAD2DEG(Ang3::GetAnglesXYZ(q));
            g_pIPak->FPrintf(exportFileHandle, "%d %f %f %f %f %f %f %f\n", k + 1, t.x, t.y, t.z, Ang.x, Ang.y, Ang.z, fBoneLen);
        }
    }
    g_pIPak->FClose(exportFileHandle);

    return true;
}

//-------------------------------------------------------------------------------------------------

bool GlobalAnimationHeaderCAF::SaveICAF(const char* szAnimationName, const char* savePath, const std::vector<string>& arrJointNames, const std::vector< DynArray<QuatT> >& arrAnimation, f32 sampleRate)
{
    // the following comment is found in Code/Tools/RC/ResourceCompilerPC/CGA/Controller.h
    // The concept of ticks seems tied to an older TCB format animation representation
    // For modern CAF files, ticks and frames are equivalent.
    //#define TICKS_PER_FRAME 1
    const int ticksPerFrame = 1;
    
    // Adding a "_Exported" suffix to the name to avoid overwriting the original source i_caf file.
    const string saveName = string(savePath) + szAnimationName + string("_Exported.i_caf");

    I3DEngine* const p3DE = GetISystem()->GetI3DEngine();

    ChunkFile::IChunkFileWriter* const pWriter = p3DE->CreateChunkFileWriter(I3DEngine::eChunkFileFormat_0x745, gEnv->pCryPak, saveName.c_str());
    if (!pWriter)
    {
        return false;
    }

    pWriter->SetAlignment(4);

    while (pWriter->StartPass())
    {
        const uint32 numBipedJoints = arrAnimation.size();
        const size_t numKeys = arrAnimation[0].size();
        int chunkId = 0;

        //-------------------------------------
        //---  write the timing             ---
        //-------------------------------------
        TIMING_CHUNK_DESC_0918 timing;
        timing.global_range.start   =   0;
        timing.global_range.end     =   uint32(numKeys - 1);
        timing.m_TicksPerFrame      = ticksPerFrame;
        timing.m_SecsPerTick        = 1.0f / ticksPerFrame / sampleRate;
        pWriter->StartChunk(eEndianness_Native, ChunkType_Timing, timing.VERSION, chunkId++);
        pWriter->AddChunkData(&timing, sizeof(timing));

        //-------------------------------------
        //---  write the controller header  ---
        //-------------------------------------
        for (uint32 j = 0; j < numBipedJoints; j++)
        {
            CONTROLLER_CHUNK_DESC_0830 controller;
            controller.numKeys          =   uint32(numKeys);
            controller.nControllerId    =   CCrc32::Compute(arrJointNames[j].c_str());
            controller.nFlags           =   0;

            pWriter->StartChunk(eEndianness_Native, ChunkType_Controller, controller.VERSION, chunkId++);

            pWriter->AddChunkData(&controller, sizeof(controller));

            std::vector<CryKeyPQLog> arrPQLogs;
            arrPQLogs.resize(numKeys);
            uint32 time = 0;
            for (uint32 k = 0; k < numKeys; k++)
            {
                Quat q = arrAnimation[j][k].q;
                Vec3 t = arrAnimation[j][k].t * 100.0f;
                arrPQLogs[k].nTime  = time;
                arrPQLogs[k].vRotLog = Quat::log(!q);
                arrPQLogs[k].vPos       = t;
                time += 0xa0;
            }

            pWriter->AddChunkData(&arrPQLogs[0], numKeys * sizeof(arrPQLogs[0]));
        }
    }

    const bool bOk = pWriter->HasWrittenSuccessfully();

    p3DE->ReleaseChunkFileWriter(pWriter);

    return bOk;
}
#endif

void GlobalAnimationHeaderCAF::ConnectCAFandDBA()
{
    if (m_FilePathDBACRC32 == 0)
    {
        return; //its a normal CAF file
    }
    if (m_nControllers)
    {
        return; //this CAF belongs to a DBA and its already connected
    }
    size_t numDBA_Files = g_AnimationManager.m_arrGlobalHeaderDBA.size();
    for (uint32 d = 0; d < numDBA_Files; d++)
    {
        CGlobalHeaderDBA& pGlobalHeaderDBA = g_AnimationManager.m_arrGlobalHeaderDBA[d];
        if (pGlobalHeaderDBA.m_pDatabaseInfo == 0)
        {
            continue;
        }
        if (m_FilePathDBACRC32 != pGlobalHeaderDBA.m_FilePathDBACRC32)
        {
            continue;
        }
        pGlobalHeaderDBA.m_nLastUsedTimeDelta = 0; //DBA in use. Reset unload count-down
        break;
    }
}


//--------------------------------------------------------------------------------------

size_t GlobalAnimationHeaderCAF::SizeOfCAF(const bool bForceControllerCalcu) const
{
    size_t nSize = 0; //sizeof(*this)

    size_t nTemp00 = m_FilePath.capacity();
    nSize += nTemp00;
    size_t nTemp07 = m_AnimEventsCAF.GetAllocSize();
    nSize += nTemp07;

    size_t nTemp08 = m_arrControllerLookupVector.get_alloc_size();
    nSize += nTemp08;
    size_t nTemp09 = m_arrController.get_alloc_size();
    nSize += nTemp09;

    if (m_FilePathDBACRC32 && m_FilePathDBACRC32 != -1)
    {
        //it's part of a DBA file
        bool InMem = g_AnimationManager.IsDatabaseInMemory(m_FilePathDBACRC32);
        if (InMem && bForceControllerCalcu)
        {
            for (uint16 i = 0; i < m_nControllers; ++i)
            {
                nSize += m_arrController[i]->ApproximateSizeOfThis();
            }
        }
    }
    else
    {
        //it's a CAF or an ANM file
        for (uint16 i = 0; i < m_nControllers; ++i)
        {
            nSize += m_arrController[i]->SizeOfController();
        }
    }

    return nSize;
}



void GlobalAnimationHeaderCAF::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(m_FilePath);
    pSizer->AddObject(m_AnimEventsCAF);
    pSizer->AddObject(m_arrControllerLookupVector);
    if (m_arrController.size())
    {
        pSizer->AddObject(m_arrController);
    }

    CRY_ASSERT(m_nControllers <= m_arrController.size());

    for (int i = 0; i < m_nControllers; ++i)
    {
        pSizer->AddObject(m_arrController[i].get());
    }
}
