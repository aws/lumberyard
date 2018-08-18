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

#ifndef CRYINCLUDE_CRYCOMMON_ICRYPAK_H
#define CRYINCLUDE_CRYCOMMON_ICRYPAK_H
#pragma once

#include <smartptr.h>
#include "CryEndian.h"
#include <ISystem.h> // <> required for Interfuscator
#include "IStreamEngineDefs.h"
#include <AzCore/IO/FileIO.h>

struct IResourceList;
struct _finddata_t;
struct IMemoryBlock;

// This represents one particular archive filcare.
struct ICryArchive
    : public _reference_target_t
{
    // Compression methods
    enum ECompressionMethods
    {
        METHOD_STORE = 0,
        METHOD_COMPRESS = 8,
        METHOD_DEFLATE = 8,
        METHOD_COMPRESS_AND_ENCRYPT = 11
    };

    // Compression levels
    enum ECompressionLevels
    {
        LEVEL_FASTEST = 0,
        LEVEL_FASTER = 2,
        LEVEL_NORMAL = 8,
        LEVEL_BETTER = 8,
        LEVEL_BEST = 9,
        LEVEL_DEFAULT = -1
    };

    enum EPakFlags
    {
        // support for absolute and other complex path specifications -
        // all paths will be treated relatively to the current directory (normally MasterCD)
        FLAGS_ABSOLUTE_PATHS = 1,

        // if this is set, the object will only understand relative to the zip file paths,
        // but this can give an opportunity to optimize for frequent quick accesses
        // FLAGS_SIMPLE_RELATIVE_PATHS and FLAGS_ABSOLUTE_PATHS are mutually exclusive
        FLAGS_RELATIVE_PATHS_ONLY = 1 << 1,

        // if this flag is set, the archive update/remove operations will not work
        // this is useful when you open a read-only or already opened for reading files.
        // If FLAGS_OPEN_READ_ONLY | FLAGS_SIMPLE_RELATIVE_PATHS are set, ICryPak
        // will try to return an object optimized for memory, with long life cycle
        FLAGS_READ_ONLY = 1 << 2,

        // if this flag is set, FLAGS_OPEN_READ_ONLY
        // flags are also implied. The returned object will be optimized for quick access and
        // memory footprint
        FLAGS_OPTIMIZED_READ_ONLY = (1 << 3),

        // if this is set, the existing file (if any) will be overwritten
        FLAGS_CREATE_NEW = 1 << 4,

        // if this flag is set, and the file is opened for writing, and some files were updated
        // so that the archive is no more continuous, the archive will nevertheless NOT be compacted
        // upon closing the archive file. This can be faster if you open/close the archive for writing
        // multiple times
        FLAGS_DONT_COMPACT = 1 << 5,

        // flag is set when complete pak has been loaded into memory
        FLAGS_IN_MEMORY = BIT(6),
        FLAGS_IN_MEMORY_CPU = BIT(7),
        FLAGS_IN_MEMORY_MASK = FLAGS_IN_MEMORY | FLAGS_IN_MEMORY_CPU,

        // Store all file names as crc32 in a flat directory structure.
        FLAGS_FILENAMES_AS_CRC32 = BIT(8),

        // flag is set when pak is stored on HDD
        FLAGS_ON_HDD = BIT(9),

        //Override pak - paks opened with this flag go at the end of the list and contents will be found before other paks
        //Used for patching
        FLAGS_OVERRIDE_PAK = BIT(10),

        // Disable a pak file without unloading it, this flag is used in combination with patches and multiplayer
        // to ensure that specific paks stay in the position(to keep the same priority) but beeing disabled
        // when running multiplayer
        FLAGS_DISABLE_PAK = BIT(11),
    };

    typedef void* Handle;

    // <interfuscator:shuffle>
    virtual ~ICryArchive(){}

    struct IEnumerateArchiveEntries
    {
        virtual bool OnEnumArchiveEntry(const char* pFilename, Handle hEntry, bool bIsFolder, int aSize, int64 aModifiedTime) = 0;
        virtual ~IEnumerateArchiveEntries(){};
    };

    // enumerate the file entries found in the specified folder
    // \return the number of entries
    virtual int EnumEntries(Handle hFolder, IEnumerateArchiveEntries* pEnum) = 0;
    // Get archive's root folder
    virtual Handle GetRootFolderHandle() = 0;

    // Summary:
    //   Adds a new file to the zip or update an existing one.
    // Description:
    //   Adds a new file to the zip or update an existing one
    //   adds a directory (creates several nested directories if needed)
    //   compression methods supported are METHOD_STORE == 0 (store) and
    //   METHOD_DEFLATE == METHOD_COMPRESS == 8 (deflate) , compression
    //   level is LEVEL_FASTEST == 0 till LEVEL_BEST == 9 or LEVEL_DEFAULT == -1
    //   for default (like in zlib)
    virtual int UpdateFile(const char* szRelativePath, void* pUncompressed, unsigned nSize, unsigned nCompressionMethod = 0, int nCompressionLevel = -1) = 0;


    // Summary:
    //   Adds a new file to the zip or update an existing one if it is not compressed - just stored  - start a big file
    //   ( name might be misleading as if nOverwriteSeekPos is used the update is not continuous )
    // Description:
    //   First step for the UpdateFileConinouseSegment
    virtual int StartContinuousFileUpdate(const char* szRelativePath, unsigned nSize) = 0;

    // Summary:
    //   Adds a new file to the zip or update an existing's segment if it is not compressed - just stored
    //   adds a directory (creates several nested directories if needed)
    //   ( name might be misleading as if nOverwriteSeekPos is used the update is not continuous )
    // Arguments:
    //   nOverwriteSeekPos - 0xffffffff means the seek pos should not be overwritten (then it needs UpdateFileCRC() to update CRC)
    virtual int UpdateFileContinuousSegment(const char* szRelativePath, unsigned nSize, void* pUncompressed, unsigned nSegmentSize, unsigned nOverwriteSeekPos = 0xffffffff) = 0;

    // Summary:
    //   needed to update CRC if UpdateFileContinuousSegment() was used with nOverwriteSeekPos
    virtual int UpdateFileCRC(const char* szRelativePath, const uint32 dwCRC) = 0;

    // Summary:
    //   Deletes the file from the archive.
    virtual int RemoveFile(const char* szRelativePath) = 0;

    // Summary:
    //   Deletes the directory, with all its descendants (files and subdirs).
    virtual int RemoveDir(const char* szRelativePath) = 0;

    // Summary:
    //   Deletes all files and directories in the archive.
    virtual int RemoveAll() = 0;

    // Summary:
    //   Finds the file; you don't have to close the returned handle.
    // Returns:
    //   NULL if the file doesn't exist
    virtual Handle FindFile(const char* szPath) = 0;
    // Summary:
    //   Get the file size (uncompressed).
    // Returns:
    //   The size of the file (unpacked) by the handle
    virtual unsigned GetFileSize(Handle) = 0;
    // Summary:
    //   Reads the file into the preallocated buffer
    // Note:
    //    Must be at least the size returned by GetFileSize.
    virtual int ReadFile(Handle, void* pBuffer) = 0;

    // Summary:
    //   Get the full path to the archive file.
    virtual const char* GetFullPath() const = 0;

    // Summary:
    //   Get the flags of this object.
    // Description:
    //   The possibles flags are defined in EPakFlags.
    // See Also:
    //   SetFlags, ResetFlags
    virtual unsigned GetFlags() const = 0;

    // Summary:
    //   Sets the flags of this object.
    // Description:
    //   The possibles flags are defined in EPakFlags.
    // See Also:
    //   GetFlags, ResetFlags
    virtual bool SetFlags(unsigned nFlagsToSet) = 0;

    // Summary:
    //   Resets the flags of this object.
    // See Also:
    //   SetFlags, GetFlags
    virtual bool ResetFlags(unsigned nFlagsToSet) = 0;

    // Summary:
    //   Control if files in this pack can be accessed
    // Returns:
    //   true if archive state was changed
    virtual bool SetPackAccessible(bool bAccessible) = 0;

    // Summary:
    //   Determines if the archive is read only.
    // Returns:
    //   true if this archive is read-only
    inline bool IsReadOnly() const { return (GetFlags() & FLAGS_READ_ONLY) != 0; }

    // Summary:
    //   Get the class id.
    virtual unsigned GetClassId() const = 0;

    // Summary:
    //   Collect allocated memory in CrySizer
    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;
    // </interfuscator:shuffle>
};

TYPEDEF_AUTOPTR(ICryArchive);

struct ICryPakFileAcesssSink
{
    // <interfuscator:shuffle>
    virtual ~ICryPakFileAcesssSink(){}
    // Arguments:
    //   in - 0 if asyncronous read
    //   szFullPath - must not be 0
    virtual void ReportFileOpen(AZ::IO::HandleType inFileHandle, const char* szFullPath) = 0;
    // </interfuscator:shuffle>
};

// this special flag is used for findfirst/findnext routines
// to mark the files that were actually found in Archive
enum
{
    _A_IN_CRYPAK = 0x80000000
};

// Summary
//   Interface to the Pak file system
// See Also
//   CryPak
struct ICryPak
{
    typedef uint64 FileTime;
    // Flags used in file path resolution rules
    enum EPathResolutionRules
    {
        // If used, the source path will be treated as the destination path
        // and no transformations will be done. Pass this flag when the path is to be the actual
        // path on the disk/in the packs and doesn't need adjustment (or after it has come through adjustments already)
        // if this is set, AdjustFileName will not map the input path into the master folder (Ex: Shaders will not be converted to Game\Shaders)
        FLAGS_PATH_REAL = 1L << 16,

        // AdjustFileName will always copy the file path to the destination path:
        // regardless of the returned value, szDestpath can be used
        FLAGS_COPY_DEST_ALWAYS = 1L << 17,

        // Adds trailing slash to the path
        FLAGS_ADD_TRAILING_SLASH = 1L << 18,

        // if this is set, AdjustFileName will not make relative paths into full paths
        FLAGS_NO_FULL_PATH = 1L << 21,

        // if this is set, AdjustFileName will redirect path to disc
        FLAGS_REDIRECT_TO_DISC = 1L << 22,

        // if this is set, AdjustFileName will not adjust path for writing files
        FLAGS_FOR_WRITING = 1L << 23,

        // if this is set, AdjustFileName will not convert the path to low case
        FLAGS_NO_LOWCASE = 1L << 24,

        // if this is set, the pak would be stored in memory (gpu)
        FLAGS_PAK_IN_MEMORY = BIT(25),

        // Store all file names as crc32 in a flat directory structure.
        FLAGS_FILENAMES_AS_CRC32 = BIT(26),

        // if this is set, AdjustFileName will try to find the file under any mod paths we know about
        FLAGS_CHECK_MOD_PATHS = BIT(27),

        // if this is set, AdjustFileName will always check the filesystem/disk and not check inside open paks
        FLAGS_NEVER_IN_PAK = BIT(28),

        // returns existing file name from the local data or existing cache file name
        // used by the resource compiler to pass the real file name
        FLAGS_RESOLVE_TO_CACHE = BIT(29),

        // if this is set, the pak would be stored in memory (cpu)
        FLAGS_PAK_IN_MEMORY_CPU = BIT(30),
    };

    // Used for widening FOpen functionality. They're ignored for the regular File System files.
    enum EFOpenFlags
    {
        // If possible, will prevent the file from being read from memory.
        FOPEN_HINT_DIRECT_OPERATION = BIT(0),
        // Will prevent a "missing file" warnings to be created.
        FOPEN_HINT_QUIET = BIT(1),
        // File should be on disk
        FOPEN_ONDISK = BIT(2),
        // Open is done by the streaming thread.
        FOPEN_FORSTREAMING = BIT(3),
    };

    //
    enum ERecordFileOpenList
    {
        RFOM_Disabled,                          // file open are not recorded (fast, no extra memory)
        RFOM_EngineStartup,                 // before a level is loaded
        RFOM_Level,                                 // during level loading till export2game -> resourcelist.txt, used to generate the list for level2level loading
        RFOM_NextLevel                          // used for level2level loading
    };
    // the size of the buffer that receives the full path to the file
    enum
    {
        g_nMaxPath = 0x800
    };

    //file location enum used in isFileExist to control where the pak system looks for the file.
    enum EFileSearchLocation
    {
        eFileLocation_Any = 0,
        eFileLocation_OnDisk,
        eFileLocation_InPak,
    };

    enum EInMemoryPakLocation
    {
        eInMemoryPakLocale_Unload = 0,
        eInMemoryPakLocale_CPU,
        eInMemoryPakLocale_GPU,
    };

    struct PakInfo
    {
        struct Pak
        {
            const char* szFilePath;
            const char* szBindRoot;
            size_t nUsedMem;
        };
        // the number of packs in the arrPacks array
        unsigned numOpenPaks;
        // the packs
        Pak arrPaks[1];
    };

    typedef int64 SignedFileSize;

    // <interfuscator:shuffle>
    virtual ~ICryPak() {}

    /**
    * given the source relative path, constructs the full path to the file according to the flags
    * returns the pointer to the constructed path (can be either szSourcePath, or szDestPath, or NULL in case of error
    */
    const char* AdjustFileName(const char* src, char *dst, size_t dstSize, unsigned nFlags, bool skipMods = false)
    {
        return AdjustFileNameImpl(src, dst, dstSize, nFlags, skipMods);
    }

    /**
    * @deprecated
    * given the source relative path, constructs the full path to the file according to the flags
    * returns the pointer to the constructed path (can be either szSourcePath, or szDestPath, or NULL in case of error
    */
    template <size_t SIZE> const char* AdjustFileName(const char* src, char(&dst)[SIZE], unsigned nFlags, bool skipMods = false)
    { 
        return AdjustFileNameImpl(src, dst, SIZE, nFlags, skipMods);
    }

    virtual const char* GetDirectoryDelimiter() const = 0;

    virtual bool Init(const char* szBasePath) = 0;
    virtual void Release() = 0;

    // Summary:
    //   Returns true if given pak path is installed to HDD
    //   If no file path is given it will return true if whole application is installed to HDD
    virtual bool IsInstalledToHDD(const char* acFilePath = 0) const = 0;

    // after this call, the pak file will be searched for files when they aren't on the OS file system
    // Arguments:
    //   pName - must not be 0
    virtual bool OpenPack(const char* pName, unsigned nFlags = FLAGS_PATH_REAL, IMemoryBlock* pData = 0, CryFixedStringT<ICryPak::g_nMaxPath>* pFullPath = 0) = 0;
    // after this call, the pak file will be searched for files when they aren't on the OS file system
    virtual bool OpenPack(const char* pBindingRoot, const char* pName, unsigned nFlags = FLAGS_PATH_REAL, IMemoryBlock* pData = 0, CryFixedStringT<ICryPak::g_nMaxPath>* pFullPath = 0) = 0;
    // after this call, the file will be unlocked and closed, and its contents won't be used to search for files
    virtual bool ClosePack(const char* pName, unsigned nFlags = FLAGS_PATH_REAL) = 0;
    // opens pack files by the path and wildcard
    virtual bool OpenPacks(const char* pWildcard, unsigned nFlags = FLAGS_PATH_REAL, std::vector< CryFixedStringT<ICryPak::g_nMaxPath> >* pFullPaths = NULL) = 0;
    // opens pack files by the path and wildcard
    virtual bool OpenPacks(const char* pBindingRoot, const char* pWildcard, unsigned nFlags = FLAGS_PATH_REAL, std::vector< CryFixedStringT<ICryPak::g_nMaxPath> >* pFullPaths = NULL) = 0;
    // closes pack files by the path and wildcard
    virtual bool ClosePacks(const char* pWildcard, unsigned nFlags = FLAGS_PATH_REAL) = 0;
    //returns if a pak exists matching the wildcard
    virtual bool FindPacks(const char* pWildcardIn) = 0;

    // Set access status of a pak files with a wildcard
    virtual bool SetPacksAccessible(bool bAccessible, const char* pWildcard, unsigned nFlags = FLAGS_PATH_REAL) = 0;

    // Set access status of a pack file
    virtual bool SetPackAccessible(bool bAccessible, const char* pName, unsigned nFlags = FLAGS_PATH_REAL) = 0;

    // this marks as Accessible all paks required for specific level
    // and vice versa for all other paks - useful in case game have per level split assets
    virtual void SetPacksAccessibleForLevel(const char* sLevelName) = 0;

    // Load or unload pak file completely to memory.
    virtual bool LoadPakToMemory(const char* pName, EInMemoryPakLocation eLoadToMemory, IMemoryBlock* pMemoryBlock = NULL) = 0;
    virtual void LoadPaksToMemory(int nMaxPakSize, bool bLoadToMemory) = 0;

    // adds a mod to the list
    virtual void AddMod(const char* szMod) = 0;
    // removes a mod from the list
    virtual void RemoveMod(const char* szMod) = 0;
    // returns indexed mod path, or NULL if out of range
    virtual const char* GetMod(int index) = 0;
    // Processes an alias command line containing multiple aliases.
    virtual void ParseAliases(const char* szCommandLine) = 0;
    // adds or removes an alias from the list
    virtual void SetAlias(const char* szName, const char* szAlias, bool bAdd) = 0;
    // gets an alias from the list, if any exist.
    // if bReturnSame==true, it will return the input name if an alias doesn't exist. Otherwise returns NULL
    virtual const char* GetAlias(const char* szName, bool bReturnSame = true) = 0;

    // lock all the operations
    virtual void Lock() = 0;
    virtual void Unlock() = 0;

    // Set and Get the localization folder name (Languages, Localization, ...)
    virtual void SetLocalizationFolder(const char* sLocalizationFolder) = 0;
    virtual const char* GetLocalizationFolder() const = 0;

    // Only returns useful results on a dedicated server at present - and only if the pak is already opened
    virtual void GetCachedPakCDROffsetSize(const char* szName, uint32& offset, uint32& size) = 0;

    // returns an array of PackInfo structures inside OpenPacks structure
    // you MUST call FreeOpenPackInfo
    virtual ICryPak::PakInfo* GetPakInfo() = 0;
    virtual void FreePakInfo(PakInfo*) = 0;

    // Open file handle, file can be on disk or in PAK file.
    // Possible mode is r,b,x
    // ex: AZ::IO::HandleType fileHandle = FOpen( "test.txt","rbx" );
    // mode x is a direct access mode, when used file reads will go directly into the low level file system without any internal data caching.
    // Text mode is not supported for files in PAKs.
    // for nFlags @see ICryPak::EFOpenFlags
    virtual AZ::IO::HandleType FOpen(const char* pName, const char* mode, unsigned nFlags = 0) = 0;
    virtual AZ::IO::HandleType FOpen(const char* pName, const char* mode, char* szFileGamePath, int nLen) = 0;

    // Read raw data from file, no endian conversion.
    virtual size_t FReadRaw(void* data, size_t length, size_t elems, AZ::IO::HandleType fileHandle) = 0;

    // Read all file contents into the provided memory, nSizeOfFile must be the same as returned by GetFileSize(handle)
    // Current seek pointer is ignored and reseted to 0.
    // no endian conversion.
    virtual size_t FReadRawAll(void* data, size_t nFileSize, AZ::IO::HandleType fileHandle) = 0;

    // Get pointer to the internally cached, loaded data of the file.
    // WARNING! The returned pointer is only valid while the fileHandle has not been closed.
    virtual void* FGetCachedFileData(AZ::IO::HandleType fileHandle, size_t& nFileSize) = 0;

    // Write file data, cannot be used for writing into the PAK.
    // Use ICryArchive interface for writing into the pak files.
    virtual size_t FWrite(const void* data, size_t length, size_t elems, AZ::IO::HandleType fileHandle) = 0;

    virtual int FPrintf(AZ::IO::HandleType fileHandle, const char* format, ...) PRINTF_PARAMS(3, 4) = 0;
    virtual char* FGets(char*, int, AZ::IO::HandleType) = 0;
    virtual int Getc(AZ::IO::HandleType) = 0;
    virtual size_t FGetSize(AZ::IO::HandleType fileHandle) = 0;
    virtual size_t FGetSize(const char* pName, bool bAllowUseFileSystem = false) = 0;
    virtual int Ungetc(int c, AZ::IO::HandleType fileHandle) = 0;
    virtual bool IsInPak(AZ::IO::HandleType fileHandle) = 0;
    virtual bool RemoveFile(const char* pName) = 0; // remove file from FS (if supported)
    virtual bool RemoveDir(const char* pName) = 0;  // remove directory from FS (if supported)
    virtual bool IsAbsPath(const char* pPath) = 0; // determines if pPath is an absolute or relative path

    virtual bool CopyFileOnDisk(const char* source, const char* dest, bool bFailIfExist) = 0;

    virtual size_t FSeek(AZ::IO::HandleType fileHandle, long seek, int mode) = 0;
    virtual long FTell(AZ::IO::HandleType fileHandle) = 0;
    virtual int FClose(AZ::IO::HandleType fileHandle) = 0;
    virtual int FEof(AZ::IO::HandleType fileHandle) = 0;
    virtual int FFlush(AZ::IO::HandleType fileHandle) = 0;

    //! Return pointer to pool if available
    virtual void* PoolMalloc(size_t size) = 0;
    //! Free pool
    virtual void PoolFree(void* p) = 0;

    // Return an interface to the Memory Block allocated on the File Pool memory.
    // sUsage indicates for what usage this memory was requested.
    virtual IMemoryBlock* PoolAllocMemoryBlock(size_t nSize, const char* sUsage, size_t nAlign = 1) = 0;

    // Arguments:
    //   nFlags is a combination of EPathResolutionRules flags.
    virtual intptr_t FindFirst(const char* pDir, _finddata_t* fd, unsigned int nFlags = 0, bool bAllOwUseFileSystem = false) = 0;
    virtual int FindNext(intptr_t handle, _finddata_t* fd) = 0;
    virtual int FindClose(intptr_t handle) = 0;
    //  virtual bool IsOutOfDate(const char * szCompiledName, const char * szMasterFile)=0;
    //returns file modification time
    virtual ICryPak::FileTime GetModificationTime(AZ::IO::HandleType fileHandle) = 0;

    // Description:
    //    Checks if specified file exist in filesystem.
    virtual bool IsFileExist(const char* sFilename, EFileSearchLocation = eFileLocation_Any) = 0;

    // Checks if path is a folder
    virtual bool IsFolder(const char* sPath) = 0;

    virtual ICryPak::SignedFileSize GetFileSizeOnDisk(const char* filename) = 0;

    // checks if file is compressed inside a pak
    virtual bool IsFileCompressed(const char* filename) = 0;

    // creates a directory
    virtual bool MakeDir(const char* szPath, bool bGamePathMapping = false) = 0;

    // open the physical archive file - creates if it doesn't exist
    // returns NULL if it's invalid or can't open the file
    // nFlags is a combination of flags from EPakFlags enum.
    virtual ICryArchive* OpenArchive(const char* szPath, const char* bindRoot = nullptr,  unsigned int nFlags = 0, IMemoryBlock* pData = 0) = 0;

    // returns the path to the archive in which the file was opened
    // returns NULL if the file is a physical file, and "" if the path to archive is unknown (shouldn't ever happen)
    virtual const char* GetFileArchivePath(AZ::IO::HandleType fileHandle) = 0;

    // compresses the raw data into raw data. The buffer for compressed data itself with the heap passed. Uses method 8 (deflate)
    // returns one of the Z_* errors (Z_OK upon success)
    // MT-safe
    virtual int RawCompress(const void* pUncompressed, unsigned long* pDestSize, void* pCompressed, unsigned long nSrcSize, int nLevel = -1) = 0;

    // Uncompresses raw (without wrapping) data that is compressed with method 8 (deflated) in the Zip file
    // returns one of the Z_* errors (Z_OK upon success)
    // This function just mimics the standard uncompress (with modification taken from unzReadCurrentFile)
    // with 2 differences: there are no 16-bit checks, and
    // it initializes the inflation to start without waiting for compression method byte, as this is the
    // way it's stored into zip file
    virtual int RawUncompress(void* pUncompressed, unsigned long* pDestSize, const void* pCompressed, unsigned long nSrcSize) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Files collector.
    //////////////////////////////////////////////////////////////////////////

    // Turn on/off recording of filenames of opened files.
    virtual void RecordFileOpen(ERecordFileOpenList eList) = 0;
    // Record this file if recording is enabled.
    // Arguments:
    //   in - 0 if asyncronous read
    virtual void RecordFile(AZ::IO::HandleType infileHandle, const char* szFilename) = 0;
    // Summary:
    //    Get resource list of all recorded files, the next level, ...
    virtual IResourceList* GetResourceList(ERecordFileOpenList eList) = 0;
    virtual void SetResourceList(ERecordFileOpenList eList, IResourceList* pResourceList) = 0;

    // get the current mode, can be set by RecordFileOpen()
    virtual ICryPak::ERecordFileOpenList GetRecordFileOpenList() = 0;

    // computes CRC (zip compatible) for a file
    // useful if a huge uncompressed file is generation in non continuous way
    // good for big files - low memory overhead (1MB)
    // Arguments:
    //   szPath - must not be 0
    // Returns:
    //   error code
    virtual uint32 ComputeCRC(const char* szPath, uint32 nFileOpenFlags = 0) = 0;

    // computes MD5 checksum for a file
    // good for big files - low memory overhead (1MB)
    // Arguments:
    //   szPath - must not be 0
    //   md5 - destination array of unsigned char [16]
    // Returns:
    //   true if success, false on failure
    virtual bool ComputeMD5(const char* szPath, unsigned char* md5, uint32 nFileOpenFlags = 0, bool useDirectFileAccess = false) = 0;

    virtual int ComputeCachedPakCDR_CRC(const char* filename, bool useCryFile = true, IMemoryBlock* pData = NULL) = 0;

    // useful for gathering file access statistics, assert if it was inserted already but then it does not become insersted
    // Arguments:
    //   pSink - must not be 0
    virtual void RegisterFileAccessSink(ICryPakFileAcesssSink* pSink) = 0;
    // assert if it was not registered already
    // Arguments:
    //   pSink - must not be 0
    virtual void UnregisterFileAccessSink(ICryPakFileAcesssSink* pSink) = 0;

    // LvlRes can be enabled by command line - then asset resource recording is enabled
    // and some modules can do more asset tracking work based on that
    // Returns:
    //   true=on, false=off
    virtual bool GetLvlResStatus() const = 0;

    // When enabled, files accessed at runtime will be tracked
    virtual void DisableRuntimeFileAccess(bool status) = 0;
    virtual bool DisableRuntimeFileAccess(bool status, threadID threadId) = 0;
    virtual bool CheckFileAccessDisabled(const char* name, const char* mode) = 0;
    virtual void SetRenderThreadId(threadID renderThreadId) = 0;

    // gets the current pak priority
    virtual int GetPakPriority() = 0;

    // Summary:
    // Return offset in pak file (ideally has to return offset on DVD) for streaming requests sorting
    virtual uint64 GetFileOffsetOnMedia(const char* szName) = 0;

    // Summary:
    //   Return media type for the file
    virtual EStreamSourceMediaType GetFileMediaType(const char* szName) = 0;

    // PerfHUD widget for tracking pak file stats
    virtual void CreatePerfHUDWidget() = 0;
    // </interfuscator:shuffle>

    // Type-safe endian conversion read.
    template<class T>
    size_t FRead(T* data, size_t elems, AZ::IO::HandleType fileHandle, bool bSwapEndian = eLittleEndian)
    {
        size_t count = FReadRaw(data, sizeof(T), elems, fileHandle);
        SwapEndian(data, count, bSwapEndian);
        return count;
    }
    // Type-independent Write.
    template<class T>
    void FWrite(T* data, size_t elems, AZ::IO::HandleType fileHandle)
    {
        FWrite((void*)data, sizeof(T), elems, fileHandle);
    }

    const static ICryPak::SignedFileSize FILE_NOT_PRESENT = -1;

protected:
    /**
    * given the source relative path, constructs the full path to the file according to the flags
    * returns the pointer to the constructed path (can be either szSourcePath, or szDestPath, or NULL in case of error
    */
    virtual const char* AdjustFileNameImpl(const char* src, char* dst, size_t dstSize, unsigned nFlags, bool skipMods) = 0;
};

class ScopedFileHandle
{
public:
    ScopedFileHandle(const char* fileName, const char* mode)
    {
        m_fileHandle = gEnv->pCryPak->FOpen(fileName, mode);
    }

    ~ScopedFileHandle()
    {
        if (m_fileHandle != AZ::IO::InvalidHandle)
        {
            gEnv->pCryPak->FClose(m_fileHandle);
        }
    }

    operator AZ::IO::HandleType()
    {
        return m_fileHandle;
    }

    bool IsValid() const
    {
        return m_fileHandle != AZ::IO::InvalidHandle;
    }

private:
    AZ::IO::HandleType m_fileHandle;
};


// The IResourceList provides an access to the collection of the resource`s file names.
// Client can add a new file names to the resource list and check if resource already in the list.
struct IResourceList
    : public _reference_target_t
{
    // <interfuscator:shuffle>
    // Description:
    //    Adds a new resource to the list.
    virtual void Add(const char* sResourceFile) = 0;

    // Description:
    //    Clears resource list.
    virtual void Clear() = 0;

    // Description:
    //    Checks if specified resource exist in the list.
    virtual bool IsExist(const char* sResourceFile) = 0;

    // Description:
    //    Loads a resource list from the resource list file.
    virtual bool Load(const char* sResourceListFilename) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Enumeration.
    //////////////////////////////////////////////////////////////////////////

    // Description:
    //    Returns the file name of the first resource or NULL if resource list is empty.
    virtual const char* GetFirst() = 0;
    // Description:
    //    Returns the file name of the next resource or NULL if reached the end.
    //    Client must call GetFirst before calling GetNext.
    virtual const char* GetNext() = 0;

    // Return memory usage stats.
    virtual void GetMemoryStatistics(class ICrySizer* pSizer) = 0;
    // </interfuscator:shuffle>
};

//! Everybody should use fxopen instead of fopen so it opens on all platforms
inline AZ::IO::HandleType fxopen(const char* file, const char* mode, bool bGameRelativePath = false)
{
    if (gEnv && gEnv->pCryPak)
    {
        gEnv->pCryPak->CheckFileAccessDisabled(file, mode);
    }
    bool bWriteAccess = false;
    for (const char* s = mode; *s; s++)
    {
        if (*s == 'w' || *s == 'W' || *s == 'a' || *s == 'A' || *s == '+')
        {
            bWriteAccess = true;
            break;
        }
        ;
    }

    if (gEnv && gEnv->pCryPak)
    {
        int nAdjustFlags = 0;
        if (!bGameRelativePath)
        {
            nAdjustFlags |= ICryPak::FLAGS_PATH_REAL;
        }
        if (bWriteAccess)
        {
            nAdjustFlags |= ICryPak::FLAGS_FOR_WRITING;
        }
        char path[_MAX_PATH];
        const char* szAdjustedPath = gEnv->pCryPak->AdjustFileName(file, path, AZ_ARRAY_SIZE(path), nAdjustFlags);

#if !defined(LINUX) && !defined(APPLE)
        if (bWriteAccess)
        {
            // Make sure folder is created.
            gEnv->pCryPak->MakeDir(szAdjustedPath);
        }
#endif
        AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
        AZ::IO::FileIOBase::GetInstance()->Open(szAdjustedPath, AZ::IO::GetOpenModeFromStringMode(mode), fileHandle);
        return fileHandle;
    }
    else
    {
        return AZ::IO::InvalidHandle;
    }
}

class CDebugAllowFileAccess
{
public:
#if defined(_RELEASE)
    ILINE CDebugAllowFileAccess() { }
    ILINE void End() { }
#else
    CDebugAllowFileAccess()
    {
        m_threadId = CryGetCurrentThreadId();
        m_oldDisable = gEnv->pCryPak ? gEnv->pCryPak->DisableRuntimeFileAccess(false, m_threadId) : false;
        m_active = true;
    }
    ILINE ~CDebugAllowFileAccess()
    {
        End();
    }
    void End()
    {
        if (m_active)
        {
            if (gEnv && gEnv->pCryPak)
            {
                gEnv->pCryPak->DisableRuntimeFileAccess(m_oldDisable, m_threadId);
            }
            m_active = false;
        }
    }
protected:
    threadID    m_threadId;
    bool        m_oldDisable;
    bool        m_active;
#endif
};

//////////////////////////////////////////////////////////////////////////


class CInMemoryFileLoader
{
public:
    CInMemoryFileLoader(ICryPak* pCryPak)
        : m_pPak(pCryPak)
        , m_fileHandle(AZ::IO::InvalidHandle)
        , m_pBuffer(0)
        , m_pCursor(0)
        , m_nFileSize(0) {}
    ~CInMemoryFileLoader()
    {
        Close();
    }

    bool IsFileExists() const
    {
        return m_fileHandle != AZ::IO::InvalidHandle;
    }

    AZ::IO::HandleType GetFileHandle() const
    {
        return m_fileHandle;
    }

    bool FOpen(const char* name, const char* mode, bool bImmediateCloseFile = false)
    {
        if (m_pPak)
        {
            assert(m_fileHandle == AZ::IO::InvalidHandle);
            m_fileHandle = m_pPak->FOpen(name, mode);
            if (m_fileHandle == AZ::IO::InvalidHandle)
            {
                return false;
            }

            m_nFileSize = m_pPak->FGetSize(m_fileHandle);
            if (m_nFileSize == 0)
            {
                Close();
                return false;
            }

            m_pCursor = m_pBuffer = (char*)m_pPak->PoolMalloc(m_nFileSize);

            size_t nReaded = m_pPak->FReadRawAll(m_pBuffer, m_nFileSize, m_fileHandle);
            if (nReaded != m_nFileSize)
            {
                Close();
                return false;
            }

            if (bImmediateCloseFile)
            {
                m_pPak->FClose(m_fileHandle);
                m_fileHandle = AZ::IO::InvalidHandle;
            }

            return true;
        }

        return false;
    }

    void FClose()
    {
        Close();
    }

    size_t FReadRaw(void* data, size_t length, size_t elems)
    {
        ptrdiff_t dist = m_pCursor - m_pBuffer;

        size_t count = length;
        if (dist + count * elems > m_nFileSize)
        {
            count = (m_nFileSize - dist) / elems;
        }

        memmove(data, m_pCursor, count * elems);
        m_pCursor += count * elems;

        return count;
    }

    template<class T>
    size_t FRead(T* data, size_t elems, bool bSwapEndian = eLittleEndian)
    {
        ptrdiff_t dist = m_pCursor - m_pBuffer;

        size_t count = elems;
        if (dist + count * sizeof(T) > m_nFileSize)
        {
            count = (m_nFileSize - dist) / sizeof(T);
        }

        memmove(data, m_pCursor, count * sizeof(T));
        m_pCursor += count * sizeof(T);

        SwapEndian(data, count, bSwapEndian);
        return count;
    }

    size_t FTell()
    {
        ptrdiff_t dist = m_pCursor - m_pBuffer;
        return dist;
    }

    int FSeek(int64 origin, int command)
    {
        int retCode = -1;
        int64 newPos;
        char* newPosBuf;
        switch (command)
        {
        case SEEK_SET:
            newPos = origin;
            if (newPos <= (int64)m_nFileSize)
            {
                m_pCursor = m_pBuffer + newPos;
                retCode = 0;
            }
            break;
        case SEEK_CUR:
            newPosBuf = m_pCursor + origin;
            if (newPosBuf <= m_pBuffer + m_nFileSize)
            {
                m_pCursor = newPosBuf;
                retCode = 0;
            }
            break;
        case SEEK_END:
            newPos = m_nFileSize - origin;
            if (newPos <= (int64)m_nFileSize)
            {
                m_pCursor = m_pBuffer + newPos;
                retCode = 0;
            }
            break;
        default:
            // Not valid disk operation!
            assert(0);
        }
        return retCode;
    }


private:

    void Close()
    {
        if (m_fileHandle != AZ::IO::InvalidHandle)
        {
            m_pPak->FClose(m_fileHandle);
        }

        if (m_pBuffer)
        {
            m_pPak->PoolFree(m_pBuffer);
        }

        m_pBuffer = m_pCursor = 0;
        m_nFileSize = 0;
        m_fileHandle = AZ::IO::InvalidHandle;
    }

private:
    AZ::IO::HandleType m_fileHandle;
    char* m_pBuffer;
    ICryPak* m_pPak;
    char* m_pCursor;
    size_t m_nFileSize;
};

//////////////////////////////////////////////////////////////////////////

#if !defined(RESOURCE_COMPILER)

//////////////////////////////////////////////////////////////////////////
// Include File helpers.
//////////////////////////////////////////////////////////////////////////
#include "CryPath.h"
#include "CryFile.h"

//////////////////////////////////////////////////////////////////////////
// Helper class that can be used to recusrively scan the directory.
//////////////////////////////////////////////////////////////////////////
struct SDirectoryEnumeratorHelper
{
public:
    void ScanDirectoryRecursive(const string& root, const string& pathIn, const string& fileSpec, std::vector<string>& files)
    {
        string path = PathUtil::AddSlash(pathIn);
        string dir = PathUtil::AddSlash(PathUtil::AddSlash(root) + path);

        ScanDirectoryFiles(root, path, fileSpec, files);

        string findFilter = PathUtil::Make(dir, "*.*");
        ICryPak* pIPak = gEnv->pCryPak;

        // Add all directories.
        _finddata_t fd;
        intptr_t fhandle;

        fhandle = pIPak->FindFirst(findFilter, &fd);
        if (fhandle != -1)
        {
            do
            {
                // Skip back folders.
                if (fd.name[0] == '.')
                {
                    continue;
                }
                if (fd.name[0] == '\0')
                {
                    CryFatalError("ICryPak FindFirst/FindNext returned empty name while looking for '%s'", findFilter.c_str());
                    continue;
                }
                if (fd.attrib & _A_SUBDIR) // skip sub directories.
                {
                    ScanDirectoryRecursive(root, PathUtil::AddSlash(path) + fd.name + "/", fileSpec, files);
                    continue;
                }
            } while (pIPak->FindNext(fhandle, &fd) == 0);
            pIPak->FindClose(fhandle);
        }
    }
private:
    void ScanDirectoryFiles(const string& root, const string& path, const string& fileSpec, std::vector<string>& files)
    {
        string dir = PathUtil::AddSlash(root) + path;

        string findFilter = PathUtil::Make(dir, fileSpec);
        ICryPak* pIPak = gEnv->pCryPak;

        _finddata_t fd;
        intptr_t fhandle;

        fhandle = pIPak->FindFirst(findFilter, &fd);
        if (fhandle != -1)
        {
            do
            {
                // Skip back folders.
                if (fd.name[0] == '.')
                {
                    continue;
                }
                if (fd.attrib & _A_SUBDIR) // skip sub directories.
                {
                    continue;
                }
                files.push_back(path + fd.name);
            } while (pIPak->FindNext(fhandle, &fd) == 0);
            pIPak->FindClose(fhandle);
        }
    }
};
#endif // !RESOURCE_COMPILER

#endif // CRYINCLUDE_CRYCOMMON_ICRYPAK_H
