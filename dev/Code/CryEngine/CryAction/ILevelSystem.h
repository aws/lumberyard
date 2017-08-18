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

// Description : Gathers level information. Loads a level.


#ifndef CRYINCLUDE_CRYACTION_ILEVELSYSTEM_H
#define CRYINCLUDE_CRYACTION_ILEVELSYSTEM_H
#pragma once

#include <CrySizer.h>
#include <IFlowSystem.h>

struct ILevelRotationFile;
struct IConsoleCmdArgs;

struct ILevelRotation
{
    virtual ~ILevelRotation() {}
    typedef uint32  TExtInfoId;

    struct IExtendedInfo
    {
    };


    typedef uint8 TRandomisationFlags;

    enum EPlaylistRandomisationFlags
    {
        ePRF_None = 0,
        ePRF_Shuffle = 1 << 0,
        ePRF_MaintainPairs = 1 << 1,
    };

    virtual bool Load(ILevelRotationFile* file) = 0;
    virtual bool LoadFromXmlRootNode(const XmlNodeRef rootNode, const char* altRootTag) = 0;

    virtual void Reset() = 0;
    virtual int  AddLevel(const char* level) = 0;
    virtual void AddGameMode(int level, const char* gameMode) = 0;

    virtual int  AddLevel(const char* level, const char* gameMode) = 0;


    //call to set the playlist ready for a new session
    virtual void Initialise(int nSeed) = 0;

    virtual bool First() = 0;
    virtual bool Advance() = 0;
    virtual bool AdvanceAndLoopIfNeeded() = 0;

    virtual const char* GetNextLevel() const = 0;
    virtual const char* GetNextGameRules() const = 0;
    virtual int GetLength() const = 0;
    virtual int GetTotalGameModeEntries() const = 0;
    virtual int GetNext() const = 0;

    virtual const char* GetLevel(uint32 idx, bool accessShuffled = true) const = 0;
    virtual int GetNGameRulesForEntry(uint32 idx, bool accessShuffled = true) const = 0;
    virtual const char* GetGameRules(uint32 idx, uint32 iMode, bool accessShuffled = true) const = 0;

    virtual const char* GetNextGameRulesForEntry(int idx) const = 0;

    virtual const int NumAdvancesTaken() const = 0;
    virtual void ResetAdvancement() = 0;

    virtual bool IsRandom() const = 0;

    virtual ILevelRotation::TRandomisationFlags GetRandomisationFlags() const = 0;
    virtual void SetRandomisationFlags(TRandomisationFlags flags) = 0;

    virtual void ChangeLevel(IConsoleCmdArgs* pArgs = NULL) = 0;

    virtual bool NextPairMatch() const = 0;
};


struct ILevelInfo
{
    virtual ~ILevelInfo() {}
    typedef std::vector<string> TStringVec;

    typedef struct
    {
        string  name;
        string  xmlFile;
        int         cgfCount;
        void GetMemoryUsage(ICrySizer* pSizer) const
        {
            pSizer->AddObject(name);
            pSizer->AddObject(xmlFile);
        }
    } TGameTypeInfo;

    struct SMinimapInfo
    {
        SMinimapInfo()
            : fStartX(0)
            , fStartY(0)
            , fEndX(1)
            , fEndY(1)
            , fDimX(1)
            , fDimY(1)
            , iWidth(1024)
            , iHeight(1024) {}

        string sMinimapName;
        int iWidth;
        int iHeight;
        float fStartX;
        float fStartY;
        float fEndX;
        float fEndY;
        float fDimX;
        float fDimY;
    };

    virtual const char* GetName() const = 0;
    virtual const bool IsOfType(const char* sType) const = 0;
    virtual const char* GetPath() const = 0;
    virtual const char* GetPaks() const = 0;
    virtual const char* GetDisplayName() const = 0;
    virtual const char* GetPreviewImagePath() const = 0;
    virtual const char* GetBackgroundImagePath() const = 0;
    virtual const char* GetMinimapImagePath() const = 0;
    //virtual const ILevelInfo::TStringVec& GetMusicLibs() const = 0; // Gets reintroduced when level specific music data loading is implemented.
    virtual int GetHeightmapSize() const = 0;
    virtual const bool MetadataLoaded() const = 0;
    virtual bool GetIsModLevel() const = 0;
    virtual const uint32 GetScanTag() const = 0;
    virtual const uint32 GetLevelTag() const = 0;

    virtual int GetGameTypeCount() const = 0;
    virtual const ILevelInfo::TGameTypeInfo* GetGameType(int gameType) const = 0;
    virtual bool SupportsGameType(const char* gameTypeName) const = 0;
    virtual const ILevelInfo::TGameTypeInfo* GetDefaultGameType() const = 0;
    virtual ILevelInfo::TStringVec GetGameRules() const = 0;
    virtual bool HasGameRules() const = 0;

    virtual const ILevelInfo::SMinimapInfo& GetMinimapInfo() const = 0;

    virtual const char* GetDefaultGameRules() const = 0;

    virtual bool GetAttribute(const char* name, TFlowInputData& val) const = 0;

    template<typename T>
    bool GetAttribute(const char* name, T& outVal) const
    {
        TFlowInputData val;
        if (GetAttribute(name, val) == false)
        {
            return false;
        }
        return val.GetValueWithConversion(outVal);
    }
};


struct ILevel
{
    virtual ~ILevel() {}
    virtual void Release() = 0;
    virtual ILevelInfo* GetLevelInfo() = 0;
};

/*!
 * Extend this class and call ILevelSystem::AddListener() to receive level system related events.
 */
struct ILevelSystemListener
{
    virtual ~ILevelSystemListener() {}
    //! Called when loading a level fails due to it not being found.
    virtual void OnLevelNotFound(const char* levelName) {}
    //! Called after ILevelSystem::PrepareNextLevel() completes.
    virtual void OnPrepareNextLevel(ILevelInfo* pLevel) {}
    //! Called after ILevelSystem::OnLoadingStart() completes, before the level actually starts loading.
    virtual void OnLoadingStart(ILevelInfo* pLevel) {}
    //! Called immediately before the EntitySystem starts loading a level.
    virtual void OnLoadingLevelEntitiesStart(ILevelInfo* pLevel) {}
    //! Called after the level finished
    virtual void OnLoadingComplete(ILevel* pLevel) {}
    //! Called when there's an error loading a level, with the level info and a description of the error.
    virtual void OnLoadingError(ILevelInfo* pLevel, const char* error) {}
    //! Called whenever the loading status of a level changes. progressAmount goes from 0->100.
    virtual void OnLoadingProgress(ILevelInfo* pLevel, int progressAmount) {}
    //! Called after a level is unloaded, before the data is freed.
    virtual void OnUnloadComplete(ILevel* pLevel) {}

    void GetMemoryUsage(ICrySizer* pSizer) const { }
};

struct ILevelSystem
    : public ILevelSystemListener
{
    enum
    {
        TAG_MAIN = 'MAIN',
        TAG_UNKNOWN = 'ZZZZ'
    };

    virtual void Rescan(const char* levelsFolder, const uint32 tag) = 0;
    virtual void LoadRotation() = 0;
    virtual int GetLevelCount() = 0;
    virtual DynArray<string>* GetLevelTypeList() = 0;
    virtual ILevelInfo* GetLevelInfo(int level) = 0;
    virtual ILevelInfo* GetLevelInfo(const char* levelName) = 0;

    virtual void AddListener(ILevelSystemListener* pListener) = 0;
    virtual void RemoveListener(ILevelSystemListener* pListener) = 0;

    virtual ILevel* GetCurrentLevel() const = 0;
    virtual ILevel* LoadLevel(const char* levelName) = 0;
    virtual void UnLoadLevel() = 0;
    virtual ILevel* SetEditorLoadedLevel(const char* levelName, bool bReadLevelInfoMetaData = false) = 0;
    virtual bool IsLevelLoaded() = 0;
    virtual void PrepareNextLevel(const char* levelName) = 0;

    virtual ILevelRotation* GetLevelRotation() = 0;

    virtual ILevelRotation* FindLevelRotationForExtInfoId(const ILevelRotation::TExtInfoId findId) = 0;

    virtual bool AddExtendedLevelRotationFromXmlRootNode(const XmlNodeRef rootNode, const char* altRootTag, const ILevelRotation::TExtInfoId extInfoId) = 0;
    virtual void ClearExtendedLevelRotations() = 0;
    virtual ILevelRotation* CreateNewRotation(const ILevelRotation::TExtInfoId id) = 0;

    // Retrieve`s last level level loading time.
    virtual float GetLastLevelLoadTime() = 0;

    // If the level load failed then we need to have a different shutdown procedure vs when a level is naturally unloaded
    virtual void SetLevelLoadFailed(bool loadFailed) = 0;
    virtual bool GetLevelLoadFailed() = 0;
};

#endif // CRYINCLUDE_CRYACTION_ILEVELSYSTEM_H
