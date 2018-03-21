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

//     Got rid of unzip usage, now using ZipDir for much more effective
//     memory usage (~3-6 times less memory, and no allocator overhead)
//     to keep the directory of the zip file; better overall effectiveness and
//     more readable and manageable code, made the connection to Streaming Engine


#ifndef CRYINCLUDE_CRYSYSTEM_CRYPAK_H
#define CRYINCLUDE_CRYSYSTEM_CRYPAK_H
#pragma once


#include <ICryPak.h>
#include <CryThread.h>
#include "IMiniLog.h"
#include "ZipDir.h"
#include "MTSafeAllocator.h"
#include "StlUtils.h"
#include "PakVars.h"
#include <VectorMap.h>
#include "IPerfHud.h"

#include "System.h"

#include <unordered_map> // remove after https://issues.labcollab.net/browse/LMBR-17710 is resolved
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

class CCryPak;

// this is the header in the cache of the file data
struct CCachedFileData
    : public _i_reference_target_t
{
    CCachedFileData (class CCryPak* pPak, ZipDir::Cache* pZip, unsigned int nArchiveFlags, ZipDir::FileEntry* pFileEntry, const char* szFilename);
    ~CCachedFileData();

    // return the data in the file, or NULL if error
    // by default, if bRefreshCache is true, and the data isn't in the cache already,
    // the cache is refreshed. Otherwise, it returns whatever cache is (NULL if the data isn't cached yet)
    // decompress and decrypt can be harmlessly set to true if you want the data back decrypted/decompressed.
    // set them to false only if you want to operate on the raw data while its still encrypted/compressed.
    void* GetData(bool bRefreshCache = true, bool decompress = true, bool decrypt = true);
    // Uncompress file data directly to provided memory.
    bool GetDataTo(void* pFileData, int nDataSize, bool bDecompress = true);

    // Return number of copied bytes, or -1 if did not read anything
    int64 ReadData(void* pBuffer, int64 nFileOffset, int64 nReadSize);

    ILINE ZipDir::Cache* GetZip(){return m_pZip; }
    ILINE ZipDir::FileEntry* GetFileEntry() {return m_pFileEntry; }

    /*
    // the memory needs to be allocated out of a MT-safe heap here
    void * __cdecl operator new   (size_t size) { return g_pPakHeap->Alloc(size, "CCachedFileData::new"); }
    void * __cdecl operator new   (size_t size, const std::nothrow_t &nothrow) { return g_pPakHeap->Alloc(size, "CCachedFileData::new(std::nothrow)"); }
    void __cdecl operator delete  (void *p) { g_pPakHeap->Free(p); };
    */

    unsigned GetFileDataOffset()
    {
        return m_pZip->GetFileDataOffset(m_pFileEntry);
    }

    size_t sizeofThis() const
    {
        return sizeof(*this) + (m_pFileData && m_pFileEntry ? m_pFileEntry->desc.lSizeUncompressed : 0);
    }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_pZip);
        pSizer->AddObject(m_pFileEntry);
    }

    // need to overload addref and release to prevent a race condition in
    virtual void AddRef();
    virtual void Release();

public:
    void* m_pFileData;

    // the zip file in which this file is opened
    ZipDir::CachePtr m_pZip;
    unsigned int m_nArchiveFlags;
    // the file entry : if this is NULL, the entry is free and all the other fields are meaningless
    ZipDir::FileEntry* m_pFileEntry;

    class CCryPak* m_pPak;

#ifdef _DEBUG
    string m_sDebugFilename;
#endif


    // file I/O guard: guarantees thread-safe decompression operation and safe allocation
    CryCriticalSection m_csDecompressDecryptLock;

private:
    CCachedFileData(const CCachedFileData&);
    CCachedFileData& operator = (const CCachedFileData&);
};

TYPEDEF_AUTOPTR(CCachedFileData);
typedef CCachedFileData_AutoPtr CCachedFileDataPtr;

//////////////////////////////////////////////////////////////////////////
struct CCachedFileRawData
{
    void* m_pCachedData;

    CCachedFileRawData(int nAlloc);
    ~CCachedFileRawData();
};


// an (inside zip) emultated open file
struct CZipPseudoFile
{
    CZipPseudoFile()
    {
        Construct();
    }
    ~CZipPseudoFile()
    {
    }

    enum
    {
        _O_COMMIT_FLUSH_MODE = 1 << 31,
        _O_DIRECT_OPERATION  = 1 << 30
    };

    // this object must be constructed before usage
    // nFlags is a combination of _O_... flags
    void Construct(CCachedFileData* pFileData = NULL, unsigned nFlags = 0);
    // this object needs to be freed manually when the CryPak shuts down..
    void Destruct();

    CCachedFileData* GetFile() {return m_pFileData; }

    long FTell() {return m_nCurSeek; }

    unsigned GetFileSize() { return GetFile() ? GetFile()->GetFileEntry()->desc.lSizeUncompressed : 0; }

    int FSeek (long nOffset, int nMode);
    size_t FRead(void* pDest, size_t nSize, size_t nCount, AZ::IO::HandleType fileHandle);
    size_t FReadAll(void* pDest, size_t nFileSize, AZ::IO::HandleType fileHandle);
    void*  GetFileData(size_t& nFileSize, AZ::IO::HandleType fileHandle);
    int FEof();
    char* FGets(char* pBuf, int n);
    int Getc();
    int Ungetc(int c);

    uint64 GetModificationTime() { return m_pFileData->GetFileEntry()->GetModificationTime(); }
    const char* GetArchivePath() { return m_pFileData->GetZip()->GetFilePath(); }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_pFileData);
    }
protected:
    unsigned long m_nCurSeek;
    CCachedFileDataPtr m_pFileData;
    // nFlags is a combination of _O_... flags
    unsigned m_nFlags;
};


struct CIStringOrder
{
    bool operator () (const string& left, const string& right) const
    {
        return _stricmp(left.c_str(), right.c_str()) < 0;
    }
};

class CCryPakFindData
    : public _reference_target_t
{
public:
    // the directory wildcard must already be adjusted
    CCryPakFindData ();
    bool    empty() const;
    bool    Fetch(_finddata_t* pfd);
    virtual void    Scan(CCryPak* pPak, const char* szDir, bool bAllowUseFS = false);

    size_t sizeofThis() const;
    void GetMemoryUsage(ICrySizer* pSizer) const;
protected:
    void ScanFS(CCryPak* pPak, const char* szDir);
    void ScanZips(CCryPak* pPak, const char* szDir);

    struct FileDesc
    {
        unsigned nAttrib;
        unsigned nSize;
        time_t tAccess;
        time_t tCreate;
        time_t tWrite;

        FileDesc (struct _finddata_t* fd);
        FileDesc (struct __finddata64_t* fd);

        FileDesc (ZipDir::FileEntry* fe);

        // default initialization is as a directory entry
        FileDesc ();

        void GetMemoryUsage(ICrySizer* pSizer) const { /*nothing*/}
    };
    typedef std::map<string, FileDesc, CIStringOrder> FileMap;
    FileMap m_mapFiles;
};

TYPEDEF_AUTOPTR(CCryPakFindData);

//////////////////////////////////////////////////////////////////////
class CCryPak
    : public ICryPak
    , public ISystemEventListener
{
    friend class CReadStream;
    friend struct CCachedFileData;

    // the array of pseudo-files : emulated files in the virtual zip file system
    // the handle to the file is its index inside this array.
    // some of the entries can be free. The entries need to be destructed manually
    CryReadModifyLock m_csOpenFiles;
    typedef std::vector<CZipPseudoFile> ZipPseudoFileArray;
    ZipPseudoFileArray m_arrOpenFiles;

    // the array of file datas; they are relatively self-contained and can
    // read and cache the file data on-demand. It's up to the clients
    // to use caching or access the zips directly
    CryReadModifyLock m_csCachedFiles;
    typedef std::vector<CCachedFileData*> CachedFileDataSet;
    CachedFileDataSet m_setCachedFiles;

    // Swith to AZStd::unordered_map after https://issues.labcollab.net/browse/LMBR-17710 is resolved
    // This is a cached data for the FGetCachedFileData call.
    struct CachedRawDataEntry
    {
        AZStd::unique_ptr<CCachedFileRawData> m_data;
        size_t m_fileSize = 0;
    };
    using CachedFileRawDataSet = std::unordered_map<AZ::IO::HandleType, CachedRawDataEntry>;
    CachedFileRawDataSet m_cachedFileRawDataSet;

    // For m_pCachedFileRawDataSet
    AZStd::mutex m_cachedFileRawDataMutex;
    using RawDataCacheLockGuard = AZStd::lock_guard<decltype(m_cachedFileRawDataMutex)>;

    // The F* emulation functions critical sectio: protects all F* functions
    // that don't have a chance to be called recursively (to avoid deadlocks)
    CryReadModifyLock m_csMain;

    // open zip cache objects that can be reused. They're self-[un]registered
    // they're sorted by the path and
    typedef std::vector<ICryArchive*> ArchiveArray;
    ArchiveArray m_arrArchives;

    //Pak file comment data. Supplied in key=value pairs in the zip archive file comment
    typedef VectorMap<string, string> TCommentDataMap;
    typedef std::pair<string, string> TCommentDataPair;

    // the array of opened caches - they get destructed by themselves (these are auto-pointers, see the ZipDir::Cache documentation)
    struct PackDesc
    {
        string strBindRoot; // the zip binding root WITH the trailing native slash
        string strFileName; // the zip file name (with path) - very useful for debugging so please don't remove

        TCommentDataMap m_commentData;  //VectorMap of key=value pairs from the zip archive comments
        const char* GetFullPath() const {return pZip->GetFilePath(); }

        ICryArchive_AutoPtr pArchive;
        ZipDir::CachePtr pZip;
        size_t sizeofThis()
        {
            return strBindRoot.capacity() + strFileName.capacity() + pZip->GetSize();
        }

        void GetMemoryUsage(ICrySizer* pSizer) const
        {
            pSizer->AddObject(strBindRoot);
            pSizer->AddObject(strFileName);
            pSizer->AddObject(pArchive);
            pSizer->AddObject(pZip);
        }
    };
    typedef std::vector<PackDesc, stl::STLGlobalAllocator<PackDesc> > ZipArray;
    CryReadModifyLock m_csZips;
    ZipArray m_arrZips;
    friend class CCryPakFindData;

protected:
    CryReadModifyLock m_csFindData;
    typedef std::set<CCryPakFindData_AutoPtr> CryPakFindDataSet;
    CryPakFindDataSet m_setFindData;

private:
    IMiniLog* m_pLog;

    // the root: "C:\MasterCD\"
    string m_strDataRoot;
    string m_strDataRootWithSlash;

    bool m_bInstalledToHDD;

    // this is the list of MOD subdirectories that will be prepended to the actual relative file path
    // they all have trailing forward slash. "" means the root dir
    std::vector<string> m_arrMods;

    //////////////////////////////////////////////////////////////////////////
    // Opened files collector.
    //////////////////////////////////////////////////////////////////////////

    ICryPak::ERecordFileOpenList                                                    m_eRecordFileOpenList;
    typedef std::set<string, stl::less_stricmp<string> >     RecordedFilesSet;
    RecordedFilesSet                                                                            m_recordedFilesSet;

    _smart_ptr<IResourceList>                                                           m_pEngineStartupResourceList;
    _smart_ptr<IResourceList>                                                           m_pLevelResourceList;
    _smart_ptr<IResourceList>                                                           m_pNextLevelResourceList;

    _smart_ptr<ICustomMemoryHeap>                         m_pInMemoryPaksCPUHeap;

    ITimer*                                                                                            m_pITimer;
    float                                                                                                   m_fFileAcessTime;                   // Time used to perform file operations
    std::vector<ICryPakFileAcesssSink*>                                    m_FileAccessSinks;               // useful for gathering file access statistics

    const PakVars*                                                                              m_pPakVars;
    bool                                                                                                    m_bLvlRes;                              // if asset tracking is enabled for GetResourceList() - all assets since executable start are recorded
    bool                                                  m_bGameFolderWritable;
    bool                                                                                                    m_disableRuntimeFileAccess[2];

    //threads which we don't want to access files from during the game
    threadID                                                                                            m_mainThreadId;
    threadID                                                                                            m_renderThreadId;

    CryFixedStringT<128>                                  m_sLocalizationFolder;

    //////////////////////////////////////////////////////////////////////////

private:

    bool InitPack(const char* szBasePath, unsigned nFlags = FLAGS_PATH_REAL);

    const char* AdjustFileNameInternal(const char* src, char dst[g_nMaxPath], unsigned nFlags);

public:
    // given the source relative path, constructs the full path to the file according to the flags
    const char* AdjustFileName(const char* src, char dst[g_nMaxPath], unsigned nFlags, bool skipMods = false);

    // this is the start of indexation of pseudofiles:
    // to the actual index , this offset is added to get the valid handle
    enum
    {
        g_nPseudoFileIdxOffset = 1
    };

    // this defines which slash will be kept internally
#if AZ_LEGACY_CRYSYSTEM_TRAIT_CRYPAK_POSIX
    enum
    {
        g_cNativeSlash = '/', g_cNonNativeSlash = '\\'
    };
#else
    enum
    {
        g_cNativeSlash = '\\', g_cNonNativeSlash = '/'
    };
#endif
    // makes the path lower-case (if requested) and removes the duplicate and non native slashes
    // may make some other fool-proof stuff
    // may NOT write beyond the string buffer (may not make it longer)
    // returns: the pointer to the ending terminator \0
    static char* BeautifyPath(char* dst, bool bMakeLowercase);
    static void RemoveRelativeParts(char* dst);

    CCryPak(IMiniLog* pLog, PakVars* pPakVars, const bool bLvlRes, const IGameStartup* pGameStartup);
    ~CCryPak();

    const PakVars* GetPakVars() const {return m_pPakVars; }

public: // ---------------------------------------------------------------------------------------

    //////////////////////////////////////////////////////////////////////////
    // ISystemEventListener implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);
    //////////////////////////////////////////////////////////////////////////

    ICryPak::PakInfo* GetPakInfo();
    void FreePakInfo (PakInfo*);

    // Set from outside if game folder is writable
    void SetGameFolderWritable(bool bWritable);
    void SetLog(IMiniLog* pLog);

    //! adds a mod to the list of mods
    void AddMod(const char* szMod);
    //! removes a mod from the list of mods
    void RemoveMod(const char* szMod);
    //! returns indexed mod path, or NULL if out of range
    virtual const char* GetMod(int index);

    //! Processes an alias command line containing multiple aliases.
    void ParseAliases(const char* szCommandLine);
    //! adds or removes an alias from the list - if bAdd set to false will remove it
    void SetAlias(const char* szName, const char* szAlias, bool bAdd);
    //! gets an alias from the list, if any exist.
    //! if bReturnSame==true, it will return the input name if an alias doesn't exist. Otherwise returns NULL
    const char* GetAlias(const char* szName, bool bReturnSame = true);

    // Set the localization folder
    virtual void SetLocalizationFolder(const char* sLocalizationFolder);
    virtual const char* GetLocalizationFolder() const { return m_sLocalizationFolder.c_str(); }

    // Only returns useful results on a dedicated server at present - and only if the pak is already opened
    void GetCachedPakCDROffsetSize(const char* szName, uint32& offset, uint32& size);

    //! Puts the memory statistics into the given sizer object
    //! According to the specifications in interface ICrySizer
    void GetMemoryStatistics(ICrySizer* pSizer);

    // lock all the operations
    virtual void Lock();
    virtual void Unlock();

    // open the physical archive file - creates if it doesn't exist
    // returns NULL if it's invalid or can't open the file
    virtual ICryArchive* OpenArchive (const char* szPath, const char* bindRoot = nullptr, unsigned int nFlags = 0, IMemoryBlock* pData = 0);

    // returns the path to the archive in which the file was opened
    virtual const char* GetFileArchivePath(AZ::IO::HandleType fileHandle);

    void Register (CCachedFileData* p)
    {
        ScopedSwitchToGlobalHeap globalHeap;

        // actually, registration may only happen when the set is already locked, but for generality..
        AUTO_MODIFYLOCK(m_csCachedFiles);
        m_setCachedFiles.push_back(p);
    }

    void Unregister (CCachedFileData* p)
    {
        AUTO_MODIFYLOCK(m_csCachedFiles);
        stl::find_and_erase(m_setCachedFiles, p);
    }

    // Get access to the special memory heap used to preload pak files into memory.
    ICustomMemoryHeap* GetInMemoryPakHeap();

    //////////////////////////////////////////////////////////////////////////

    //! Return pointer to pool if available
    virtual void* PoolMalloc(size_t size);
    //! Free pool
    virtual void PoolFree(void* p);

    virtual IMemoryBlock* PoolAllocMemoryBlock(size_t nSize, const char* sUsage, size_t nAlign);

    // interface ICryPak ---------------------------------------------------------------------------

    virtual void RegisterFileAccessSink(ICryPakFileAcesssSink* pSink);
    virtual void UnregisterFileAccessSink(ICryPakFileAcesssSink* pSink);
    virtual bool GetLvlResStatus() const { return m_bLvlRes; }

    virtual bool Init (const char* szBasePath);
    virtual void Release();

    virtual bool IsInstalledToHDD(const char* acFilePath = 0) const;
    void SetInstalledToHDD(bool bValue);

    virtual bool OpenPack(const char* pName, unsigned nFlags = 0, IMemoryBlock* pData = 0, CryFixedStringT<ICryPak::g_nMaxPath>* pFullPath = NULL);
    virtual bool OpenPack(const char* szBindRoot, const char* pName, unsigned nFlags = 0, IMemoryBlock* pData = 0, CryFixedStringT<ICryPak::g_nMaxPath>* pFullPath = NULL);
    // after this call, the file will be unlocked and closed, and its contents won't be used to search for files
    virtual bool ClosePack(const char* pName, unsigned nFlags = 0);
    virtual bool OpenPacks(const char* pWildcard, unsigned nFlags = 0, std::vector< CryFixedStringT<ICryPak::g_nMaxPath> >* pFullPaths = NULL);
    virtual bool OpenPacks(const char* szBindRoot, const char* pWildcard, unsigned nFlags = 0, std::vector< CryFixedStringT<ICryPak::g_nMaxPath> >* pFullPaths = NULL);

    bool OpenPackCommon(const char* szBindRoot, const char* pName, unsigned int nPakFlags, IMemoryBlock* pData = 0);
    bool OpenPacksCommon(const char* szDir, const char* pWildcardIn, char* cWork, int nPakFlags, std::vector< CryFixedStringT<ICryPak::g_nMaxPath> >* pFullPaths = NULL);

    // closes pack files by the path and wildcard
    virtual bool ClosePacks(const char* pWildcard, unsigned nFlags = 0);

    //returns if a pak exists matching the wildcard
    virtual bool FindPacks(const char* pWildcardIn);

    // prevent access to specific pak files
    bool SetPacksAccessible(bool bAccessible, const char* pWildcard, unsigned nFlags = 0);
    bool SetPackAccessible(bool bAccessible, const char* pName, unsigned nFlags = 0);
    void SetPacksAccessibleForLevel(const char* sLevelName);

    // Remount the packs to another file system
    typedef bool (* CBShouldPackReOpen)(const char* filePath);
    int RemountPacks(DynArray<AZ::IO::HandleType>& outHandlesToClose, CBShouldPackReOpen shouldPackReOpen);

    //ReOpen pak file - used for devices when pak is successfully cached to HDD
    bool ReOpenPack(const char* pPath);

    // returns the file modification time
    virtual uint64 GetModificationTime(AZ::IO::HandleType fileHandle);


    // this function gets the file data for the given file, if found.
    // The file data object may be created in this function,
    // and it's important that the autoptr is returned: another thread may release the existing
    // cached data before the function returns
    // the path must be absolute normalized lower-case with forward-slashes
    CCachedFileDataPtr GetFileData(const char* szName, unsigned int& nArchiveFlags);
    CCachedFileDataPtr GetFileData(const char* szName){unsigned int archiveFlags; return GetFileData(szName, archiveFlags); }

    // Return cached file data for entries inside pak file.
    CCachedFileDataPtr GetOpenedFileDataInZip(AZ::IO::HandleType file);

    // tests if the given file path refers to an existing file inside registered (opened) packs
    // the path must be absolute normalized lower-case with forward-slashes
    bool WillOpenFromPak(const char* szPath);
    ZipDir::FileEntry* FindPakFileEntry(const char* szPath, unsigned int& nArchiveFlags, ZipDir::CachePtr* pZip = 0, bool bSkipInMemoryPaks = false);
    ZipDir::FileEntry* FindPakFileEntry(const char* szPath){unsigned int flags; return FindPakFileEntry(szPath, flags); }

    virtual bool LoadPakToMemory(const char* pName, EInMemoryPakLocation nLoadPakToMemory, IMemoryBlock* pMemoryBlock = NULL);
    virtual void LoadPaksToMemory(int nMaxPakSize, bool bLoadToMemory);

    virtual const char* GetDirectoryDelimiter() const;

    virtual AZ::IO::HandleType FOpen(const char* pName, const char* mode, unsigned nPathFlags = 0);
    virtual AZ::IO::HandleType FOpen(const char* pName, const char* mode, char* szFileGamePath, int nLen);
    virtual size_t FReadRaw(void* data, size_t length, size_t elems, AZ::IO::HandleType handle);
    virtual size_t FReadRawAll(void* data, size_t nFileSize, AZ::IO::HandleType handle);
    virtual void* FGetCachedFileData(AZ::IO::HandleType handle, size_t& nFileSize);
    virtual size_t FWrite(const void* data, size_t length, size_t elems, AZ::IO::HandleType handle);
    virtual size_t FSeek(AZ::IO::HandleType handle, long seek, int mode);
    virtual long FTell(AZ::IO::HandleType handle);
    virtual int FFlush(AZ::IO::HandleType handle);
    virtual int FClose(AZ::IO::HandleType handle);
    virtual intptr_t FindFirst(const char* pDir, _finddata_t* fd, unsigned int nPathFlags = 0, bool bAllOwUseFileSystem = false);
    virtual int FindNext(intptr_t handle, _finddata_t* fd);
    virtual int FindClose(intptr_t handle);
    virtual int FEof(AZ::IO::HandleType handle);
    virtual char* FGets(char*, int, AZ::IO::HandleType);
    virtual int Getc(AZ::IO::HandleType);
    virtual int Ungetc(int c, AZ::IO::HandleType);
    virtual int FPrintf(AZ::IO::HandleType handle, const char* format, ...) PRINTF_PARAMS(3, 4);
    virtual size_t FGetSize(AZ::IO::HandleType fileHandle);
    virtual size_t FGetSize(const char* sFilename, bool bAllowUseFileSystem = false);
    virtual bool IsInPak(AZ::IO::HandleType handle);
    virtual bool RemoveFile(const char* pName); // remove file from FS (if supported)
    virtual bool RemoveDir(const char* pName);  // remove directory from FS (if supported)
    virtual bool IsAbsPath(const char* pPath);
    virtual CCryPakFindData* CreateFindData();

    virtual bool CopyFileOnDisk(const char* source, const char* dest, bool bFailIfExist);

    virtual bool IsFileExist(const char* sFilename, EFileSearchLocation fileLocation = eFileLocation_Any);
    virtual bool IsFolder(const char* sPath);
    virtual bool IsFileCompressed(const char* filename);
    virtual ICryPak::SignedFileSize GetFileSizeOnDisk(const char* filename);

    // creates a directory
    virtual bool MakeDir (const char* szPath, bool bGamePathMapping = false);

    // returns the current game directory, with trailing slash (or empty string if it's right in MasterCD)
    // this is used to support Resource Compiler which doesn't have access to this interface:
    // in case all the contents is located in a subdirectory of MasterCD, this string is the subdirectory name with slash
    //virtual const char* GetGameDir();

    void Register (ICryArchive* pArchive);
    void Unregister (ICryArchive* pArchive);
    ICryArchive* FindArchive (const char* szFullPath);
    const ICryArchive* FindArchive (const char* szFullPath) const;

    // compresses the raw data into raw data. The buffer for compressed data itself with the heap passed. Uses method 8 (deflate)
    // returns one of the Z_* errors (Z_OK upon success)
    // MT-safe
    int RawCompress (const void* pUncompressed, unsigned long* pDestSize, void* pCompressed, unsigned long nSrcSize, int nLevel = -1);

    // Uncompresses raw (without wrapping) data that is compressed with method 8 (deflated) in the Zip file
    // returns one of the Z_* errors (Z_OK upon success)
    // This function just mimics the standard uncompress (with modification taken from unzReadCurrentFile)
    // with 2 differences: there are no 16-bit checks, and
    // it initializes the inflation to start without waiting for compression method byte, as this is the
    // way it's stored into zip file
    int RawUncompress (void* pUncompressed, unsigned long* pDestSize, const void* pCompressed, unsigned long nSrcSize);

    //////////////////////////////////////////////////////////////////////////
    // Files opening recorder.
    //////////////////////////////////////////////////////////////////////////

    void RecordFileOpen(ERecordFileOpenList eMode);
    ICryPak::ERecordFileOpenList GetRecordFileOpenList();
    void RecordFile(AZ::IO::HandleType in, const char* szFilename);

    virtual IResourceList* GetResourceList(ERecordFileOpenList eList);
    virtual void SetResourceList(ERecordFileOpenList eList, IResourceList* pResourceList);

    virtual uint32 ComputeCRC(const char* szPath, uint32 nFileOpenFlags = 0);
    virtual bool ComputeMD5(const char* szPath, unsigned char* md5, uint32 nFileOpenFlags = 0, bool useDirectFileAccess = false);

    int ComputeCachedPakCDR_CRC(ZipDir::CachePtr pInZip);
    virtual int ComputeCachedPakCDR_CRC(const char* filename, bool useCryFile /*=true*/, IMemoryBlock* pData /*=NULL*/);


    void OnMissingFile (const char* szPath);

    virtual void DisableRuntimeFileAccess(bool status)
    {
        m_disableRuntimeFileAccess[0] = status;
        m_disableRuntimeFileAccess[1] = status;
    }

    virtual bool DisableRuntimeFileAccess(bool status, threadID threadId);
    virtual bool CheckFileAccessDisabled(const char* name, const char* mode);

    void LogFileAccessCallStack(const char* name, const char* nameFull, const char* mode);

    virtual void SetRenderThreadId(threadID renderThreadId)
    {
        m_renderThreadId = renderThreadId;
    }

    // missing file -> count of missing files
    CryCriticalSection m_csMissingFiles;
    typedef std::map<string, uint32, std::less<string> > MissingFileMap;
    MissingFileMap m_mapMissingFiles;

    friend struct SAutoCollectFileAcessTime;

    std::set< uint32, std::less<uint32>, stl::STLGlobalAllocator<uint32> > m_filesCachedOnHDD;

    // gets the current pak priority
    virtual int GetPakPriority();

    virtual uint64 GetFileOffsetOnMedia(const char* szName);

    virtual EStreamSourceMediaType GetFileMediaType(const char* szName);
    EStreamSourceMediaType GetMediaType(ZipDir::Cache* pCache, unsigned int nArchiveFlags);

    virtual void CreatePerfHUDWidget();

#if defined(LINUX) || defined(APPLE)
private:
    intptr_t m_HandleSource;
    std::map<intptr_t, CCryPakFindData* > m_Handles;
#endif

    //PerfHUD Widget for tracking open pak files:
    class CPakFileWidget
        : public ICryPerfHUDWidget
    {
    public:
        CPakFileWidget(minigui::IMiniCtrl* pParentMenu, ICryPerfHUD* pPerfHud, CCryPak* pPak);
        ~CPakFileWidget() {}

        virtual void Reset() {}
        virtual void Update();
        virtual bool ShouldUpdate() { return !m_pTable->IsHidden(); }
        virtual void LoadBudgets(XmlNodeRef perfXML) {}
        virtual void SaveStats(XmlNodeRef statsXML) {}
        virtual void Enable(int mode) { m_pTable->Hide(false); }
        virtual void Disable() { m_pTable->Hide(true); }

    protected:
        minigui::IMiniTable* m_pTable;
        CCryPak* m_pPak;
    };

    CPakFileWidget* m_pWidget;
};

#endif // CRYINCLUDE_CRYSYSTEM_CRYPAK_H

