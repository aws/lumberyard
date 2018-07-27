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

#ifndef CRYINCLUDE_CRYANIMATION_LOADERTCB_H
#define CRYINCLUDE_CRYANIMATION_LOADERTCB_H
#pragma once

#include <IChunkFile.h>


struct IChunkFile;
class CContentCGF;
struct ChunkDesc;
#define ROTCHANNEL (0xaa)
#define POSCHANNEL (0x55)
//////////////////////////////////////////////////////////////////////////
struct CHeaderTCB
    : public _reference_target_t
{
    DynArray<IController_AutoPtr> m_pControllers;
    DynArray<uint32> m_arrControllerId;

    uint32  m_nAnimFlags;
    uint32  m_nCompression;

    int32       m_nTicksPerFrame;
    f32         m_secsPerTick;
    int32       m_nStart;
    int32       m_nEnd;

    DynArray<TCBFlags> m_TrackVec3FlagsQQQ;
    DynArray<TCBFlags> m_TrackQuatFlagsQQQ;
    DynArray<DynArray<CryTCB3Key>*> m_TrackVec3QQQ;
    DynArray<DynArray<CryTCBQKey>*> m_TrackQuat;
    DynArray<CControllerType> m_arrControllersTCB;

    CHeaderTCB()
    {
        m_nAnimFlags = 0;
        m_nCompression = -1;
    }

    ~CHeaderTCB()
    {
        for (DynArray<DynArray<CryTCB3Key>*>::iterator it = m_TrackVec3QQQ.begin(), itEnd = m_TrackVec3QQQ.end(); it != itEnd; ++it)
        {
            delete *it;
        }
        for (DynArray<DynArray<CryTCBQKey>*>::iterator it = m_TrackQuat.begin(), itEnd = m_TrackQuat.end(); it != itEnd; ++it)
        {
            delete *it;
        }
    }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
        pSizer->AddObject(m_pControllers);
        pSizer->AddObject(m_arrControllerId);
        //  pSizer->AddObject(  m_FootPlantBits );

        //pSizer->AddObject( m_TrackVec3Flags );
        //  pSizer->AddObject( m_TrackQuatFlags );
        //pSizer->AddObject( m_TrackVec3 );
        //pSizer->AddObject( m_TrackQuat );
        //pSizer->AddObject( m_arrControllers );
    }




private:
    CHeaderTCB(const CHeaderTCB&);
    CHeaderTCB& operator = (const CHeaderTCB&);
};


//////////////////////////////////////////////////////////////////////////
class ILoaderCAFListener
{
public:
    virtual ~ILoaderCAFListener(){}
    virtual void Warning(const char* format) = 0;
    virtual void Error(const char* format) = 0;
};

class CLoaderTCB
{
public:
    CLoaderTCB();
    ~CLoaderTCB();


    CHeaderTCB* LoadTCB(const char* filename, ILoaderCAFListener* pListener);
    CHeaderTCB* LoadTCB(const char* filename, IChunkFile* pChunkFile, ILoaderCAFListener* pListener);

    const char* GetLastError() const { return m_LastError; }
    //  CContentCGF* GetCContentCAF() { return m_pCompiledCGF; }
    bool GetHasNewControllers() const { return m_bHasNewControllers; };
    bool GetHasOldControllers() const { return m_bHasOldControllers; };
    void SetLoadOldChunks(bool v) { m_bLoadOldChunks = v; };
    bool GetLoadOldChunks() const { return m_bLoadOldChunks; };

    void SetLoadOnlyCommon(bool b) { m_bLoadOnlyCommon = b; }
    bool GetLoadOnlyOptions() const { return m_bLoadOnlyCommon; }

private:
    bool LoadChunksTCB(IChunkFile* chunkFile);
    bool ReadTiming (IChunkFile::ChunkDesc* pChunkDesc);
    bool ReadMotionParameters (IChunkFile::ChunkDesc* pChunkDesc);
    bool ReadController (IChunkFile::ChunkDesc* pChunkDesc);
    //void Warning(const char* szFormat, ...);


private:
    string m_LastError;

    string m_filename;

    CHeaderTCB* m_pSkinningInfo;

    bool m_bHasNewControllers;
    bool m_bLoadOldChunks;
    bool m_bHasOldControllers;
    bool m_bLoadOnlyCommon;



    //bool m_bHaveVertexColors;
    //bool m_bOldMaterials;
};

#endif // CRYINCLUDE_CRYANIMATION_LOADERTCB_H
