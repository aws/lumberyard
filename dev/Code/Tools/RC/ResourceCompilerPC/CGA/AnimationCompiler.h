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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_ANIMATIONCOMPILER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_ANIMATIONCOMPILER_H
#pragma once

#include "IConvertor.h"
#include "../CryEngine/Cry3DEngine/CGF/CGFLoader.h"

#include "AnimationInfoLoader.h"
#include "IResCompiler.h"
#include "GlobalAnimationHeaderCAF.h"

#include "SkeletonLoader.h"
#include "SkeletonManager.h"
#include "DBAManager.h"
#include <AzCore/std/parallel/atomic.h>

struct ConvertContext;
class CSkeletonInfo;
class CAnimationManager;
class DBATableEnumerator;
struct SCompressionPresetTable;
struct SAnimationDesc;
class CTrackStorage;

namespace ThreadUtils
{
    class StealingThreadPool;
}
class CAnimationCompiler;
class SkeletonLoader;
class CInternalSkinningInfo;

struct AnimationJob
{
    CAnimationCompiler* m_compiler;

    // Path to animation database
    string m_dbaPath;

    // Unified animation path (path from Game folder)
    string m_animationPath;

    // Actual source path. Can have a different extension if single file (/file) processed
    string m_sourcePath;

    // Name of temporary file. This file is always stored in little-endian (PC)
    // format. It is used when final animation stored in .DBA or converted to
    // big-endian (console format). We can't load big-endian CAF's on PC
    // at the moment.
    string m_intermediatePath;

    // Output file
    string m_destinationPath;

    SAnimationDesc m_animDesc;
    const CSkeletonInfo* m_skeleton;

    bool m_debugCompression;
    bool m_bigEndianOutput;
    bool m_singleFileMode;
    bool m_fileChanged;
    bool m_alignTracks;
    const CInternalSkinningInfo* m_controllerSkinningInfo;

    bool m_writeIntermediateFile; // e.g. .$caf
    bool m_writeDestinationFile;
    size_t m_resSize;

    AnimationJob()
        : m_compiler(0)
        , m_fileChanged(false)
        , m_debugCompression(false)
        , m_bigEndianOutput(false)
        , m_alignTracks(false)
        , m_writeIntermediateFile(false)
        , m_writeDestinationFile(false)
        , m_resSize(0)
        , m_singleFileMode(false)
        , m_skeleton(0)
    {
    }
};


class ICryXML;

// processing state corresponding to entry in CBA-file (DBA)
struct CBAEntry
{
    SkeletonLoader m_skeleton;
    bool m_changed;

    CBAEntry()
        : m_changed(false)
    {}
};

class CAnimationConvertor
    : public IConvertor
{
public:

    CAnimationConvertor(ICryXML* pXMLParser, IPakSystem* pPakSystem, IResourceCompiler* pRC);

    // interface IConvertor ------------------------------------------------------------
    virtual void Init(const ConvertorInitContext& context);

    virtual void DeInit();

    virtual void Release();

    virtual ICompiler* CreateCompiler();

    virtual const char* GetExt(int index) const
    {
        switch (index)
        {
        case 0:
            return "caf";
        case 1:
            return "i_caf";
        case 2:
            return "cba";
        default:
            return 0;
        }
    }

    // ---------------------------------------------------------------------------------

    bool LoadCBA(const char* cbaPath, bool loadCharacters);
    CAnimationInfoLoader* GetCBALoader() { return m_cbaLoader.get(); }

    // thread-safe methods
    void SetPlatform(int platform);
    const string& GetTempPath_() const{ return m_tempPath; } // _ because we don't want to be affected by windows.h
    const string& GetSourceGameFolderPath() const{ return m_sourceGameFolderPath; }
    const string& GetSourceWatchFolderPath() const { return m_sourceWatchFolderPath; }
    const SPlatformAnimationSetup& GetPlatformSetup() const{ return m_platformAnimationSetup; }
    const SAnimationDefinition* GetAnimationDefinition(const char* cafFilename) const;
    CBAEntry* GetEntryForDefinition(const SAnimationDefinition* definition);
    const CBAEntry* GetEntryForDefinition(const SAnimationDefinition* definition) const;
    const CSkeletonInfo* FindSkeleton(const SAnimationDesc* pAnimationDesc, const SAnimationDefinition* pDefinition) const;

    void IncrementChangedAnimationCount();
    int IncrementFancyAnimationIndex();

    ICryXML* GetXMLParser() { return m_pXMLParser; }
    IPakSystem* GetPakSystem() { return m_pPakSystem; }

    bool UsingIntemediateCAF() const { return m_usingIntermediateCAF; }
    bool InLocalUpdateMode() const;

    std::unique_ptr<SkeletonManager> m_skeletonManager;
    std::unique_ptr<DBAManager> m_dbaManager;
    std::unique_ptr<DBATableEnumerator> m_dbaTableEnumerator;
    std::unique_ptr<SCompressionPresetTable> m_compressionPresetTable;
private:
    bool RebuildDatabases();

    void InitDBAManager();
    void InitSkeletonManager(const std::set<string>& usedSkeletons);

    std::unique_ptr<CAnimationInfoLoader> m_cbaLoader;
    std::vector<CBAEntry> m_cbaEntries;


    int m_platform;
    SPlatformAnimationSetup m_platformAnimationSetup;

    string m_tempPath;
    string m_cbaPath;

    // Folder that contains SkeletonList.xml and DBATable.json, relative to game data folder.
    // "Animations", if /animConfigFolder is not specified.
    string m_configSubfolder;

    // Source game data folder path is a location relative to which animation paths are specified.
    // Exception is .cba file - it uses GameFolder/Animations.
    string m_sourceGameFolderPath;
    string m_sourceWatchFolderPath;

    bool m_usingIntermediateCAF;

    IResourceCompiler* m_rc;
    IPakSystem* m_pPakSystem;
    ICryXML* m_pXMLParser;
    const IConfig* m_config;
    AZStd::atomic_long m_fancyAnimationIndex;
    AZStd::atomic_long m_changedAnimationCount;
    int m_refCount;
};

class CAnimationCompiler
    : public ICompiler
{
public:
    CAnimationCompiler();
    CAnimationCompiler(CAnimationConvertor* convertor);
    virtual ~CAnimationCompiler();

    // interface ICompiler -------------------------------------------------------------
    virtual void Release();

    virtual void BeginProcessing(const IConfig* config) { }
    virtual void EndProcessing() { }

    virtual IConvertContext* GetConvertContext() { return &m_CC; }
    virtual bool Process();
    bool ProcessInMemoryData(CContentCGF* content, CInternalSkinningInfo* controllerSkinningInfo, const string& outputFile, const AZ::SceneAPI::DataTypes::IAnimationGroup* animationGroup = nullptr);
    // ---------------------------------------------------------------------------

    bool ProcessCBA();
    CAnimationConvertor* Convertor() const{ return m_convertor; }

private:
    string GetOutputFileNameOnly() const;
    string GetOutputPath() const;

    void GetFilenamesForUpToDateCheck(std::vector<string>& upToDateSrcFilenames, std::vector<string>& upToDateCheckFilenames);
    bool CheckIfAllFilesAreUpToDate(const std::vector<string>& upToDateSrcFilenames, const std::vector<string>& upToDateCheckFilenames);
    void HandleUpToDateCheckFilesOnReturn(const std::vector<string>& upToDateSrcFilenames, const std::vector<string>& upToDateCheckFilenames);

    CAnimationConvertor* m_convertor;
    ICryXML* m_pXMLParser;
    ConvertContext m_CC;
};


// a little helper to replace SWAP macro
struct SEndiannessSwapper
{
    SEndiannessSwapper(bool bigEndianOutput)
        : m_bigEndianOutput(bigEndianOutput)
    {
    }

    template<class T>
    void operator()(T& value)
    {
        if (m_bigEndianOutput)
        {
            SwapEndianness<T>(value);
        }
    }

    template<class T>
    void operator()(T* value, size_t count)
    {
        if (m_bigEndianOutput)
        {
            SwapEndianness<T>(value, count);
        }
    }

private:
    bool m_bigEndianOutput;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_ANIMATIONCOMPILER_H
