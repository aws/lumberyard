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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CHARACTERCOMPILER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CHARACTERCOMPILER_H
#pragma once


#include "IConvertor.h"
#include "../../CryEngine/Cry3DEngine/MeshCompiler/MeshCompiler.h"
#include "../CryEngine/Cry3DEngine/CGF/CGFLoader.h"

struct ConvertContext;
class CContentCGF;
class CChunkFile;
struct CNodeCGF;
class CPhysicsInterface;
class ICryXML;
struct CMaterialCGF;
struct CSkinningInfo;

class CharacterCompiler
    : public IConvertor
    , public ICompiler
{
    struct SAttachmentMap
    {
        CContentCGF* pCGF;
        XmlNodeRef xmlNode;
    };
    typedef std::map<int, std::vector<SAttachmentMap> > LodAttachmentMap;

    struct SSubsetMatMap
    {
        int newSubsetId;
        XmlNodeRef originalMtlXml;
        int oldSubsetId;
        bool merged;
        bool mergeTextures;
    };
    typedef std::vector<SSubsetMatMap> SubsetMatMap;
    typedef std::map<int, std::vector<int> > CombineMap;

    enum EFileType
    {
        eFileType_chr = 0,
        eFileType_cdf,
        eFileType_skin,
    };

    enum ECdfLoadResult
    {
        eCdfLoadResult_Success,
        eCdfLoadResult_Failed,
        eCdfLoadResult_NotMergeCdf,
    };

public:
    CharacterCompiler(ICryXML* pXml);
    ~CharacterCompiler();

    class Listener
        : public ILoaderCGFListener
    {
    public:
        void Warning(const char* format) override;
        void Error(const char* format) override;
    };

    // IConvertor methods.
    virtual ICompiler* CreateCompiler();
    virtual bool SupportsMultithreading() const;
    virtual const char* GetExt(int index) const
    {
        switch (index)
        {
        case eFileType_chr:
            return "chr";
        case eFileType_cdf:
            return "cdf";
        case eFileType_skin:
            return "skin";
        default:
            return 0;
        }
    }

    // ICompiler methods.
    virtual void BeginProcessing(const IConfig* config) { }
    virtual void EndProcessing() { }
    virtual IConvertContext* GetConvertContext() { return &m_CC; }
    virtual bool Process();
    bool ProcessWork(CLoaderCGF* cgfLoader, CContentCGF* pCGF, CChunkFile* chunkFile, bool cdfasset, const string& sourceFile, const string& outputFile, bool updateMatchingFileTime = true);

    // ICompiler + IConvertor methods.
    void Release();

    CContentCGF* MakeCompiledCGF(CContentCGF* pCGF);

private:
    string GetOutputFileNameOnly() const;
    string GetOutputPath() const;
    void DeleteOldChunks(CContentCGF* pCGF, CChunkFile& chunkFile);
    // Physicalize all meshes in cgf.
    bool Physicalize(CContentCGF* pCGF);

    //character merging functions
    bool LoadCDFAssets(const char* sourceFile, const char* finalfilename, CLoaderCGF* cgfLoader, ILoaderCGFListener* pListener, std::vector<CContentCGF*>* cgfArray);
    bool MaterialValidForMerge(XmlNodeRef mtlcontents, XmlNodeRef setupxml);
    ECdfLoadResult FindAllAttachmentModels(const char* sourceFile, CLoaderCGF* cgfLoader, ILoaderCGFListener* pListener, LodAttachmentMap* attachments, XmlNodeRef setupxml);
    void FindAttachmentLods(CLoaderCGF* cgfLoader, ILoaderCGFListener* pListener, const char* lod0model, XmlNodeRef material, LodAttachmentMap* lods);
    bool ValidateSkeletons(const LodAttachmentMap& lods) const;
    CContentCGF* MakeMergedCGF(const char* filename, CMaterialCGF* pMaterial, const std::vector<SAttachmentMap>& attachments, XmlNodeRef setupXml, bool lod, XmlNodeRef basematerial);
    CNodeCGF* SetupNewMergeContents(CContentCGF* pCGF, CMaterialCGF* pMaterial);
    void CollectNodesForMerging(CNodeCGF* pTargetNode, CContentCGF* sourceCgf, XmlNodeRef sourceMtl, std::vector<SMeshSubset>* subsets, std::vector<CNodeCGF*>* merge_nodes, SubsetMatMap* unoptimised, bool lod, XmlNodeRef basematerial, std::vector<CSkinningInfo*>* skinning);
    int FindExistingSubmatId(XmlNodeRef sourceMtl, XmlNodeRef baseMtl);
    void CopyStreamData(CNodeCGF* pNode, const SMeshSubset& pMeshSubset, CNodeCGF* pSubSetNode);
    CSkinningInfo* CopySkinningInfo(CContentCGF* pSourceCgf, const SMeshSubset& pMeshSubset);
    void CalculateVertexIndexTotals(CNodeCGF* pNode, int& vertexCount, int& indexCount);
    void MergeOtherNodes(CContentCGF* targetCgf, CContentCGF* sourceCgf);
    bool DetermineMergedSubsets(std::vector<CNodeCGF*>* nodes, std::vector<SMeshSubset>* subsets, SubsetMatMap* unoptimised, XmlNodeRef setupXml, CombineMap* combinemap, bool lod);
    bool CompareXmlNodesAttributes(XmlNodeRef a, XmlNodeRef b, bool verbose = true);
    float NormalizeTexCoord(float uv, bool& normalised);
    void OffsetUVs(CNodeCGF* pSubSetNode, XmlNodeRef material, XmlNodeRef setupXml);
    bool MergeSubsets(const CombineMap& combinemap, std::vector<SMeshSubset>* subsets, std::vector<CNodeCGF*>* merge_nodes, CSkinningInfo* pDstSkinningInfo, const std::vector<CSkinningInfo*> skinning);
    void MergeSkinning(CSkinningInfo* pSrc, CSkinningInfo* pDst, int nOffset, int nCount, bool master);
    void FixupBoneArrayIdx(CNodeCGF* pNode, std::map<int, uint8>* mapBoneArrayIdx);
    bool MergeMeshNodes(CNodeCGF* pNode, const std::vector<SMeshSubset>& subsets, std::vector<CNodeCGF*>* merge_nodes);
    bool CompileOmptimisedMaterials(CContentCGF* pMergedCgf, CNodeCGF* pNode, const SubsetMatMap& unoptimised, const CombineMap& combinedMap, XmlNodeRef setupXml);
    bool ShouldCombineTextures(const char* submat, const std::vector<string>& ignorelist);
    XmlNodeRef CreateTextureSetupFile(const char* filename, XmlNodeRef setupXml);
    void CombineTextureFiles(XmlNodeRef texMergeSetup, const std::pair<int, std::vector<int> >& map, XmlNodeRef setupXml, XmlNodeRef originalXml, const SubsetMatMap& unoptimised);
    XmlNodeRef WriteOriginalXml(XmlNodeRef originalXml, XmlNodeRef config);
    void SaveTextureMergeSetup(const char* filename, XmlNodeRef texMergeSetup);
    void WriteConfigEntry(XmlNodeRef setup, XmlNodeRef attachment, const char* slot, XmlNodeRef destination);
    bool CreateOptimisedMaterial(const char* filename, const std::vector<XmlNodeRef>& materials);
    int FindSubsetId(CNodeCGF* pNode, int vertid);

private:
    CPhysicsInterface* m_pPhysicsInterface;
    ConvertContext m_CC;
    ICryXML* m_pXML;
    int m_refCount;
    // Confetti: Nicholas Baldwin
    bool m_bOptimizePVRStripify;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CHARACTERCOMPILER_H
