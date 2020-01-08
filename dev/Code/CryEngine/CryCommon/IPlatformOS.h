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

// Description : Interface to the Platform OS

#pragma once

#include <CryFixedString.h>
#include <ILocalizationManager.h>

struct SStreamingInstallProgress
{
    enum EState
    {
        eState_NotStarted = 0,
        eState_Active,
        eState_Paused,
        eState_Completed
    };
    SStreamingInstallProgress()
        : m_totalSize(0)
        , m_installedSize(0)
        , m_state(eState_NotStarted)
        , m_progress(0.0f) {}

    uint64      m_totalSize;                //bytes
    uint64      m_installedSize;        //bytes
    EState      m_state;
    float           m_progress;                 // [0..1]
};

// Interface platform OS (Operating System) functionality
struct IPlatformOS
{
    enum ECDP_Start
    {
        eCDPS_Success,
        eCDPS_Failure,
        eCDPS_NoFreeSpace,
        eCDPS_AlreadyInUse,
        eCDPS_HDDNotReady,
    };

    enum ECDP_End
    {
        eCDPE_Success,
        eCDPE_Failure,
        eCDPE_WrongUser,
        eCDPE_NotInUse,
    };

    enum ECDP_Open
    {
        eCDPO_Success,
        eCDPO_MD5Failed,
        eCDPO_OpenPakFailed,
        eCDPO_FileNotFound,
        eCDPO_HashNotMatch,
        eCDPO_SizeNotMatch,
        eCDPO_NotInUse,
    };

    enum ECDP_Close
    {
        eCDPC_Success,
        eCDPC_Failure,
        eCDPC_NotInUse,
    };

    enum ECDP_Delete
    {
        eCDPD_Success,
        eCDPD_Failure,
        eCDPD_NotInUse,
    };

    enum ECDP_Write
    {
        eCDPW_Success,
        eCDPW_Failure,
        eCDPW_NoFreeSpace,
        eCDPW_NotInUse,
    };

    enum EFileOperationCode
    {
        eFOC_Success                        = 0x00,     // no error
        eFOC_Failure                        = 0x01,     // generic error

        eFOC_ErrorOpenRead          = 0x12,     // unable to open a file to read
        eFOC_ErrorRead                  = 0x14,     // error reading from a file
        eFOC_ErrorOpenWrite         = 0x22,     // unable to open a file to write
        eFOC_ErrorWrite                 = 0x24,     // error writing to a file

        // Masks
        eFOC_ReadMask                       = 0x10,     // error reading
        eFOC_WriteMask                  = 0x20,     // error writing
        eFOC_OpenMask                       = 0x02,     // error opening files/content
    };

    enum EDLCMountFail
    {
        eDMF_FileCorrupt,
        eDMF_DiskCorrupt,
        eDMF_XmlError,
        eDMF_NoDLCDir,
        eDMF_Unknown,
    };

    enum EZipExtractFail
    {
        eZEF_Success,
        eZEF_Unsupported,
        eZEF_WriteFail,
        eZEF_UnknownError,
    };

    // Derive listeners from this interface
    struct IDLCListener
    {
        virtual ~IDLCListener(){}
        virtual void OnDLCMounted(const XmlNodeRef& rootNode, const char* sDLCRootFolder) = 0;
        virtual void OnDLCMountFailed(EDLCMountFail reason) = 0;
        virtual void OnDLCMountFinished(int nPacksFound) = 0;
    };

    struct IFileFinder
    {
        enum EFileState
        {
            eFS_NotExist,
            eFS_File,
            eFS_Directory,
            eFS_FileOrDirectory = -1, // for Win32 based bool return value we use this - see CFileFinderCryPak::FileExists
        };

        virtual ~IFileFinder(){}

        // FileExists:
        //   Determine if a file or path exists
        virtual EFileState FileExists(const char* path) = 0;

        // FindFirst:
        //   Find the first file matching a specified pattern
        //   Returns: find handle to be used with FindNext() and FindClose()
        virtual intptr_t FindFirst(const char* filePattern, _finddata_t* fd) = 0;

        // FindClose:
        //   Close and release resources associated with FindFirst/FindNext operations
        virtual int FindClose(intptr_t handle) = 0;

        // FindNext:
        //   Find the next file matching a specified pattern
        virtual int FindNext(intptr_t handle, _finddata_t* fd) = 0;

        // MapFilePath
        // Description:
        //     Provide the ability to remap a file path into a platform specific file path.
        //     This is a convenience utility function, the default implementation simply returns the input path and returns true.
        // Arguments:
        //     const char * filePath - the raw input file path
        //     string & filePathMapped - the OS specific mapped file path
        //     bool bCreateMapping - true to record the remapped path or false to only return the mapped file path
        // Return Value:
        //     bool - true if the remapped path already existed in the file mapping table
        virtual bool MapFilePath(const char* filePath, string& filePathMapped, bool bCreateMapping) { filePathMapped = filePath; return true; }

        virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;
    };
    typedef AZStd::shared_ptr<IFileFinder> IFileFinderPtr;

    // Call once to create and initialize. Use delete to destroy.
    static IPlatformOS* Create();

    virtual ~IPlatformOS() {}

    // Collect memory statistics in CrySizer
    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;

    // Start Deprecated: Streaming install functionality that will be replaced with a StreamingInstall Gem
    virtual void QueryStreamingInstallProgressForChunk(const unsigned int chunkID, SStreamingInstallProgress* pProgress) const
    {
        memset(pProgress, 0, sizeof(SStreamingInstallProgress));
        pProgress->m_progress = 1.0f;
        pProgress->m_state = SStreamingInstallProgress::eState_Completed;
    }
    virtual void QueryStreamingInstallProgressForLevel(const char* sLevelName, SStreamingInstallProgress* pProgress) const
    {
        memset(pProgress, 0, sizeof(SStreamingInstallProgress));
        pProgress->m_progress = 1.0f;
        pProgress->m_state = SStreamingInstallProgress::eState_Completed;
    }
    virtual void SwitchStreamingInstallPriorityToLevel(const char* sLevelName) {}
    virtual bool IsFileAvailable(const char* sFileName) const {return true; }
    virtual bool IsPakRequiredForLevel(const char* sPakFileName, const char* sLevelName) const {return true; }
    virtual uint32 GetPakSortIndex(const char* sPakFileName) const { return 0; }
    virtual void RenderEnd() {}
    // End Deprecated: Streaming install functionality that will be replaced with a StreamingInstall Gem

    // Start Deprecated: Miscellaneous functionality that is still being used by some legacy code
    virtual IPlatformOS::IFileFinderPtr GetFileFinder(unsigned int user) = 0;
    virtual void MountDLCContent(IDLCListener* pCallback, unsigned int user, const uint8 keyData[16]) = 0;
    virtual IPlatformOS::ECDP_Start StartUsingCachePaks(const int user, bool* outWritesOccurred) = 0;
    virtual IPlatformOS::ECDP_End EndUsingCachePaks(const int user) = 0;
    virtual IPlatformOS::ECDP_Open DoesCachePakExist(const char* const filename, const size_t size, unsigned char md5[16]) = 0;
    virtual IPlatformOS::ECDP_Open OpenCachePak(const char* const filename, const char* const bindRoot, const size_t size, unsigned char md5[16]) = 0;
    virtual IPlatformOS::ECDP_Close CloseCachePak(const char* const filename) = 0;
    virtual IPlatformOS::ECDP_Delete DeleteCachePak(const char* const filename) = 0;
    virtual IPlatformOS::ECDP_Write WriteCachePak(const char* const filename, const void* const pData, const size_t numBytes) = 0;
    virtual IPlatformOS::EZipExtractFail ExtractZips(const char* path) = 0;
    // End Deprecated: Miscellaneous functionality that is still being used by some legacy code

    // Start Deprecated: Clip capture functionality that will be moved elsewhere
    // Interface for capturing in-game clips
    struct IClipCaptureOS
    {
        struct IListener
        {
            enum ECaptureFinishedStatus
            {
                eCFS_Success = 0,
                eCFS_Failure
            };

            virtual void OnClipCaptureFinished(ECaptureFinishedStatus status) = 0;
        };

        virtual ~IClipCaptureOS() {}
        virtual void GetMinMaxClipsPerTitle(uint32& minNumClips_, uint32& maxNumClips_) const { minNumClips_ = minNumClips; maxNumClips_ = maxNumClips; }
        virtual float GetMaxTotalClipsTimePerTitle() const { return maxTotalClipsTime; }

        struct SSpan
        {
            explicit SSpan(float before = 0.0f, float after = 0.0f)
                : secondsBefore(before)
                , secondsAfter(after) {}
            float secondsBefore; ///< recording starts in the past using the platform's buffering if available
            float secondsAfter;  ///< recording ends in the future, keeping on storing until this delay is elapsed
            ILINE float GetDuration() const { return secondsBefore + secondsAfter; }
        };

        struct SClipTextInfo
        {
            SClipTextInfo(const char* _szGreatestMomentId, const wchar_t* _wszLocalizedClipName = NULL, const char* _szTitleData = NULL)
                : szGreatestMomentId(_szGreatestMomentId)
                , wszLocalizedClipName(_wszLocalizedClipName)
                , szTitleData(_szTitleData)
            {}

            // szGreatestMomentId
            // The title-defined ID string that identifies the description of the clip.
            // Developers set up valid identifiers through relevant console Developer Portals. The cloud service uses this ID to look up the description string in the ApplicationClip class.
            const char* szGreatestMomentId;

            // wszLocalizedClipName
            // Contains the name of the clip localized into the current language.
            // If this string is defined, it is displayed in the toast.
            const wchar_t* wszLocalizedClipName;

            // szTitleData
            // A title-defined string that is stored with the clip.
            // Titles use the ApplicationClipInfo class to set this information in the clip at the time of recording or use the UpdateClipMetadataAsync class to change this information once the clip has been uploaded to the cloud.
            const char* szTitleData;
        };

        virtual bool RecordClip(SClipTextInfo clipTextInfo, const SSpan& span, IListener* pListener = NULL, const bool makeLocalCopy = false) { return false; }

        // this parameters are based on initial MS requirements
        uint32 minNumClips;                 // minimum number of clips to record per game/session
        uint32 maxNumClips;                 // maximum number of clips to record per game/session
        float maxTotalClipsTime;        // all clips combined cannot amount to more than this time per game/session
    };

    virtual IClipCaptureOS* GetClipCapture() { return NULL; }
    // End Deprecated: Clip capture functionality that will be moved elsewhere
};
