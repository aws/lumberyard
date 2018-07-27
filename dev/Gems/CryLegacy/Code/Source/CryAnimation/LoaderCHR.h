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

//  Contains:
//  loading of model and animations

#ifndef CRYINCLUDE_CRYANIMATION_LOADERCHR_H
#define CRYINCLUDE_CRYANIMATION_LOADERCHR_H
#pragma once

#include "ModelAnimationSet.h"
#include "IStreamEngine.h"

class CSkin;
class CDefaultSkeleton;
#include "ParamLoader.h"

class CryCHRLoader
    : public IStreamCallback
{
public:
    CryCHRLoader()
    {
        m_pModelSkel = 0;
        m_pModelSkin = 0;
    }
    ~CryCHRLoader()
    {
#ifndef _RELEASE
        if (m_pStream)
        {
            __debugbreak();
        }
#endif
        ClearModel();
    }

    bool BeginLoadCHRRenderMesh(CDefaultSkeleton* pSkel, EStreamTaskPriority estp);
    bool BeginLoadSkinRenderMesh(CSkin* pSkin, int nRenderLod, EStreamTaskPriority estp);
    void AbortLoad();

    static void ClearCachedLodSkeletonResults();

    // the controller manager for the new model; this remains the same during the whole lifecycle the file without extension
    string m_strGeomFileNameNoExt;

public: // IStreamCallback Members
    virtual void StreamAsyncOnComplete (IReadStream* pStream, unsigned nError);
    virtual void StreamOnComplete (IReadStream* pStream, unsigned nError);

private:
    struct SmartContentCGF
    {
        CContentCGF* pCGF;
        SmartContentCGF(CContentCGF* cgf)
            : pCGF(cgf) {}
        ~SmartContentCGF()
        {
            if (pCGF)
            {
                g_pI3DEngine->ReleaseChunkfileContent(pCGF);
            }
        }
        CContentCGF* operator -> () { return pCGF; }
        CContentCGF& operator * () { return *pCGF; }
        operator CContentCGF* () const {
            return pCGF;
        }
        operator bool () const {
            return pCGF != NULL;
        }
    };

private:


    void EndStreamSkel(IReadStream* pStream);
    void EndStreamSkinAsync(IReadStream* pStream);
    void EndStreamSkinSync(IReadStream* pStream);

    void PrepareMesh(CMesh* pMesh);
    void PrepareRenderChunks(DynArray<RChunk>& arrRenderChunks, CMesh* pMesh);
    _smart_ptr<IRenderMesh> CreateRenderMesh(CDefaultSkeleton* pSkel, CMesh* pMesh);
    _smart_ptr<IRenderMesh> CreateRenderMesh(CSkin* pSkin, CMesh* pMesh, int lod);

    void ClearModel();

private:
    CDefaultSkeleton* m_pModelSkel;
    CSkin* m_pModelSkin;
    IReadStreamPtr m_pStream;
    DynArray<RChunk> m_arrNewRenderChunks;
    _smart_ptr<IRenderMesh> m_pNewRenderMesh;
};

#endif // CRYINCLUDE_CRYANIMATION_LOADERCHR_H
