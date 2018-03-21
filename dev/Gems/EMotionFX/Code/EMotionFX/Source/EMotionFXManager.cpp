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

// include the required headers
#include "EMotionFXConfig.h"
#include "EMotionFXManager.h"
#include "Importer/Importer.h"
#include "ActorManager.h"
#include "MotionManager.h"
#include "EventManager.h"
#include "SoftSkinManager.h"
#include "Importer/Importer.h"
#include "WaveletCache.h"
#include "Recorder.h"
#include "MotionInstancePool.h"
#include "AnimGraphManager.h"
#include <MCore/Source/UnicodeString.h>
#include <MCore/Source/UnicodeStringIterator.h>
#include <MCore/Source/UnicodeCharacter.h>
#include <MCore/Source/JobManager.h>
#include <MCore/Source/MCoreSystem.h>
#include <MCore/Source/MemoryTracker.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/IO/FileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/API/ApplicationAPI.h>


namespace EMotionFX
{
    // the global EMotion FX manager object
    AZ::EnvironmentVariable<EMotionFXManager*> gEMFX;

    //-----------------------------------------------------------------------------

    // initialize the global EMotion FX object
    bool Initializer::Init(InitSettings* initSettings)
    {
        // if we already have initialized, there is nothing to do
        if (gEMFX)
        {
            return true;
        }

        // create the new object
        gEMFX = AZ::Environment::CreateVariable<EMotionFXManager*>(kEMotionFXInstanceVarName);
        gEMFX.Set(EMotionFXManager::Create());

        InitSettings finalSettings;
        if (initSettings)
        {
            finalSettings = *initSettings;
        }

        // set the unit type
        gEMFX.Get()->SetUnitType(finalSettings.mUnitType);

        // create and set the objects
        gEMFX.Get()->SetImporter              (Importer::Create());
        gEMFX.Get()->SetActorManager          (ActorManager::Create());
        gEMFX.Get()->SetMotionManager         (MotionManager::Create());
        gEMFX.Get()->SetEventManager          (EventManager::Create());
        gEMFX.Get()->SetSoftSkinManager       (SoftSkinManager::Create());
        gEMFX.Get()->SetWaveletCache          (WaveletCache::Create());
        gEMFX.Get()->SetAnimGraphManager      (AnimGraphManager::Create());
        gEMFX.Get()->GetAnimGraphManager()->Init();
        gEMFX.Get()->SetRecorder              (Recorder::Create());
        gEMFX.Get()->SetMotionInstancePool    (MotionInstancePool::Create());
        //gEMFX->SetRigManager          ( RigManager::Create() );
        gEMFX.Get()->SetGlobalSimulationSpeed (1.0f);

        // set the number of threads
        uint32 numThreads = MCore::GetJobManager().GetNumThreads();
        if (numThreads > 0)
        {
            gEMFX.Get()->SetNumThreads(numThreads, false, MCore::GetMCore().GetJobListExecuteFunc());
        }
        else
        {
            gEMFX.Get()->SetNumThreads(1, false);
        }

        // show details
        gEMFX.Get()->LogInfo();
        return true;
    }


    // shutdown EMotion FX
    void Initializer::Shutdown()
    {
        // delete the global object and reset it to nullptr
        gEMFX.Get()->Destroy();
        gEMFX.Reset();
    }

    //-----------------------------------------------------------------------------


    // constructor
    EMotionFXManager::EMotionFXManager()
    {
        mThreadDatas.SetMemoryCategory(EMFX_MEMCATEGORY_EMOTIONFXMANAGER);
        // build the low version string
        MCore::String lowVersionString;
        BuildLowVersionString(lowVersionString);

        mVersionString.Format("EMotion FX v%d.%s RC4", EMFX_HIGHVERSION, lowVersionString.AsChar());
        mCompilationDate        = MCORE_DATE;
        mHighVersion            = EMFX_HIGHVERSION;
        mLowVersion             = EMFX_LOWVERSION;
        mImporter               = nullptr;
        mActorManager           = nullptr;
        mMotionManager          = nullptr;
        mEventManager           = nullptr;
        mSoftSkinManager        = nullptr;
        mWaveletCache           = nullptr;
        mRecorder               = nullptr;
        mMotionInstancePool     = nullptr;
        //mRigManager           = nullptr;
        mUnitType               = MCore::Distance::UNITTYPE_METERS;
        mGlobalSimulationSpeed  = 1.0f;

        if (MCore::GetMCore().GetIsTrackingMemory())
        {
            RegisterMemoryCategories(MCore::GetMemoryTracker());
        }
    }


    // destructor
    EMotionFXManager::~EMotionFXManager()
    {
        // the motion manager has to get destructed before the anim graph manager as the motion manager kills all motion instances
        // from the motion nodes when destructing the motions itself
        //mRigManager->Destroy();
        mMotionManager->Destroy();
        mAnimGraphManager->Destroy();
        mImporter->Destroy();
        mActorManager->Destroy();
        mMotionInstancePool->Destroy();
        mEventManager->Destroy();
        mSoftSkinManager->Destroy();
        mWaveletCache->Destroy();
        mRecorder->Destroy();

        // delete the thread datas
        for (uint32 i = 0; i < mThreadDatas.GetLength(); ++i)
        {
            mThreadDatas[i]->Destroy();
        }
        mThreadDatas.Clear();
    }


    // create
    EMotionFXManager* EMotionFXManager::Create()
    {
        return new EMotionFXManager();
    }


    // update
    void EMotionFXManager::Update(float timePassedInSeconds)
    {
        // update the wavelet cache
        mWaveletCache->Update(timePassedInSeconds);

        // update the recorder in playback mode
        mRecorder->UpdatePlayMode(timePassedInSeconds);

        // update all actor instances (the main thing)
        mActorManager->UpdateActorInstances(timePassedInSeconds);

        // update physics
        mEventManager->OnSimulatePhysics(timePassedInSeconds);

        // update the recorder
        mRecorder->Update(timePassedInSeconds);

        // sample and apply all anim graphs we recorded
        if (mRecorder->GetIsInPlayMode() && mRecorder->GetRecordSettings().mRecordAnimGraphStates)
        {
            mRecorder->SampleAndApplyAnimGraphs(mRecorder->GetCurrentPlayTime());
        }

        // wait for all jobs to be completed
        MCore::GetJobManager().NextFrame();
    }


    // log information about EMotion FX
    void EMotionFXManager::LogInfo()
    {
        // turn "0.010" into "01" to for example build the string v3.01 later on
        MCore::String lowVersionString;
        BuildLowVersionString(lowVersionString);

        MCore::LogInfo("-----------------------------------------------");
        MCore::LogInfo("EMotion FX - Information");
        MCore::LogInfo("-----------------------------------------------");
        MCore::LogInfo("Version:          v%d.%s", mHighVersion, lowVersionString.AsChar());
        MCore::LogInfo("Version string:   %s", mVersionString.AsChar());
        MCore::LogInfo("Compilation date: %s", mCompilationDate.AsChar());

    #ifdef MCORE_OPENMP_ENABLED
        MCore::LogInfo("OpenMP enabled:   Yes");
    #else
        MCore::LogInfo("OpenMP enabled:   No");
    #endif

        //#if (defined(MCORE_EVALUATION) && defined(MCORE_PLATFORM_WINDOWS) && !defined(MCORE_NO_LICENSESYSTEM))
        //  MCore::LogInfo("License System:   Yes");
        //#else
        //MCore::LogInfo("License System:   No");
        //#endif

        MCore::LogInfo("-----------------------------------------------");
    }


    // get the version string
    const char* EMotionFXManager::GetVersionString() const
    {
        return mVersionString.AsChar();
    }


    // get the compilation date string
    const char* EMotionFXManager::GetCompilationDate() const
    {
        return mCompilationDate.AsChar();
    }


    // get the high version
    uint32 EMotionFXManager::GetHighVersion() const
    {
        return mHighVersion;
    }


    // get the low version
    uint32 EMotionFXManager::GetLowVersion() const
    {
        return mLowVersion;
    }


    // set the importer
    void EMotionFXManager::SetImporter(Importer* importer)
    {
        mImporter = importer;
    }


    // set the actor manager
    void EMotionFXManager::SetActorManager(ActorManager* manager)
    {
        mActorManager = manager;
    }


    // set the motion manager
    void EMotionFXManager::SetMotionManager(MotionManager* manager)
    {
        mMotionManager = manager;
    }


    // set the event manager
    void EMotionFXManager::SetEventManager(EventManager* manager)
    {
        mEventManager = manager;
    }


    // set the softskin manager
    void EMotionFXManager::SetSoftSkinManager(SoftSkinManager* manager)
    {
        mSoftSkinManager = manager;
    }


    // set the anim graph manager
    void EMotionFXManager::SetAnimGraphManager(AnimGraphManager* manager)
    {
        mAnimGraphManager = manager;
    }

    /*
    // set the rig manager
    void EMotionFXManager::SetRigManager(RigManager* manager)
    {
        mRigManager = manager;
    }
    */

    // set the recorder
    void EMotionFXManager::SetRecorder(Recorder* recorder)
    {
        mRecorder = recorder;
    }


    // set the motion instance pool
    void EMotionFXManager::SetMotionInstancePool(MotionInstancePool* pool)
    {
        mMotionInstancePool = pool;
        pool->Init();
    }


    // set the wavelet cache
    void EMotionFXManager::SetWaveletCache(WaveletCache* cache)
    {
        mWaveletCache = cache;
    }


    // set the path of the media root directory
    void EMotionFXManager::SetMediaRootFolder(const char* path)
    {
        mMediaRootFolder = path;

        // Make sure the media root folder has an ending slash.
        if (mMediaRootFolder.empty() == false)
        {
            const char lastChar = AzFramework::StringFunc::LastCharacter(mMediaRootFolder.c_str());
            if (lastChar != AZ_CORRECT_FILESYSTEM_SEPARATOR && lastChar != AZ_WRONG_FILESYSTEM_SEPARATOR)
            {
                AzFramework::StringFunc::Path::AppendSeparator(mMediaRootFolder);
            }
        }

        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, mMediaRootFolder);
    }


    void EMotionFXManager::InitAssetFolderPaths()
    {
        // Initialize the asset source folder path.
        const char* assetSourcePath = AZ::IO::FileIOBase::GetInstance()->GetAlias("@devassets@");
        if (assetSourcePath)
        {
            mAssetSourceFolder = assetSourcePath;

            // Add an ending slash in case there is none yet.
            // TODO: Remove this and adopt EMotionFX code to work with folder paths without slash at the end like Lumberyard does.
            if (mAssetSourceFolder.empty() == false)
            {
                const char lastChar = AzFramework::StringFunc::LastCharacter(mAssetSourceFolder.c_str());
                if (lastChar != AZ_CORRECT_FILESYSTEM_SEPARATOR && lastChar != AZ_WRONG_FILESYSTEM_SEPARATOR)
                {
                    AzFramework::StringFunc::Path::AppendSeparator(mAssetSourceFolder);
                }
            }

            EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, mAssetSourceFolder);
        }
        else
        {
            AZ_Warning("EMotionFX", false, "Failed to set asset source path for alias '@devassets@'.");
        }


        // Initialize the asset cache folder path.
        const char* assetCachePath = AZ::IO::FileIOBase::GetInstance()->GetAlias("@assets@");
        if (assetCachePath)
        {
            mAssetCacheFolder = assetCachePath;

            // Add an ending slash in case there is none yet.
            // TODO: Remove this and adopt EMotionFX code to work with folder paths without slash at the end like Lumberyard does.
            if (mAssetCacheFolder.empty() == false)
            {
                const char lastChar = AzFramework::StringFunc::LastCharacter(mAssetCacheFolder.c_str());
                if (lastChar != AZ_CORRECT_FILESYSTEM_SEPARATOR && lastChar != AZ_WRONG_FILESYSTEM_SEPARATOR)
                {
                    AzFramework::StringFunc::Path::AppendSeparator(mAssetCacheFolder);
                }
            }

            EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, mAssetCacheFolder);
        }
        else
        {
            AZ_Warning("EMotionFX", false, "Failed to set asset cache path for alias '@assets@'.");
        }
    }


    void EMotionFXManager::ConstructAbsoluteFilename(const char* relativeFilename, AZStd::string& outAbsoluteFilename)
    {
        outAbsoluteFilename = relativeFilename;
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, outAbsoluteFilename);
        AzFramework::StringFunc::Replace(outAbsoluteFilename, EMFX_MEDIAROOTFOLDER_STRING, mMediaRootFolder.c_str(), true);
    }


    AZStd::string EMotionFXManager::ConstructAbsoluteFilename(const char* relativeFilename)
    {
        AZStd::string result;
        ConstructAbsoluteFilename(relativeFilename, result);
        return result;
    }


    void EMotionFXManager::GetFilenameRelativeTo(MCore::String* inOutFilename, const char* folderPath)
    {
        AZStd::string baseFolderPath = folderPath;
        AZStd::string filename = inOutFilename->AsChar();

        // TODO: Add parameter to not lower case the path once it is in and working.
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, baseFolderPath);
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);

        // TODO: Remove the following code as soon as the Systems team has fixed the folder path capitalisation of AZ::IO::FileIOBase::GetInstance()->GetAlias().
        //       To work around it we'll lower case the media root folder and lower case the same amount of characters for the filename to be actually able to remove the media root from the absolute file path.
        const size_t baseFolderPathSize = baseFolderPath.size();
        AZStd::to_lower(baseFolderPath.begin(), baseFolderPath.begin() + baseFolderPathSize);
        AZStd::to_lower(filename.begin(), filename.begin() + baseFolderPathSize);

        // Remove the media root folder from the absolute motion filename so that we get the relative one to the media root folder.
        *inOutFilename = filename.c_str();
        inOutFilename->RemoveAllParts(baseFolderPath.c_str());
    }


    void EMotionFXManager::GetFilenameRelativeToMediaRoot(MCore::String* inOutFilename) const
    {
        GetFilenameRelativeTo(inOutFilename, GetMediaRootFolder());
    }


    // build the low version string
    // this turns 900 into 9, and 50 into 05
    void EMotionFXManager::BuildLowVersionString(MCore::String& outLowVersionString)
    {
        outLowVersionString.Format("%.2f", EMFX_LOWVERSION / 100.0f);
        outLowVersionString.TrimLeft(MCore::UnicodeCharacter('0'));
        outLowVersionString.TrimLeft(MCore::UnicodeCharacter('.'));
        outLowVersionString.TrimRight(MCore::UnicodeCharacter('0'));
        outLowVersionString.TrimRight(MCore::UnicodeCharacter('.'));

        if (outLowVersionString.GetIsEmpty())
        {
            outLowVersionString = "0";
        }
    }


    // get the global speed factor
    float EMotionFXManager::GetGlobalSimulationSpeed() const
    {
        return mGlobalSimulationSpeed;
    }


    // set the global speed factor
    void EMotionFXManager::SetGlobalSimulationSpeed(float speedFactor)
    {
        mGlobalSimulationSpeed = MCore::Max<float>(0.0f, speedFactor);
    }


    // set the number of threads
    void EMotionFXManager::SetNumThreads(uint32 numThreads, bool adjustJobManagerNumThreads, MCore::JobListExecuteFunctionType jobListExecuteFunction)
    {
        MCORE_ASSERT(numThreads > 0 && numThreads <= 1024);
        if (numThreads == 0)
        {
            MCore::LogWarning("EMotionFXManager::SetNumThreads() - Cannot set the number of threads to 0, using 1 instead.");
            numThreads = 1;
        }

        if (mThreadDatas.GetLength() == numThreads)
        {
            if (adjustJobManagerNumThreads)
            {
                if (MCore::GetJobManager().GetNumThreads() == numThreads)
                {
                    return;
                }

                // if we want one thread, kill all threads and force going onto the main thread instead
                if (numThreads == 1)
                {
                    MCore::GetJobManager().RemoveAllThreads();
                    MCore::GetMCore().SetJobListExecuteFunc(MCore::JobListExecuteSerial);
                }
                else
                {
                    MCore::GetJobManager().SetNumThreads(numThreads);
                    MCore::GetMCore().SetJobListExecuteFunc(jobListExecuteFunction);
                }
            }
            return;
        }

        // get rid of old data
        for (uint32 i = 0; i < mThreadDatas.GetLength(); ++i)
        {
            mThreadDatas[i]->Destroy();
        }

        mThreadDatas.Clear(false); // force calling constructors again to reset everything
        mThreadDatas.Resize(numThreads);

        mWaveletCache->InitThreads(numThreads);
        for (uint32 i = 0; i < numThreads; ++i)
        {
            mThreadDatas[i] = ThreadData::Create(i);
        }

        // set the number of threads inside the job manager (only does that when we use the job system job execute function)
        if (adjustJobManagerNumThreads)
        {
            MCore::GetJobManager().SetNumThreads(numThreads);
        }

        // if we want one thread, kill all threads and force going onto the main thread instead
        if (numThreads == 1)
        {
            MCore::GetJobManager().RemoveAllThreads();
            MCore::GetMCore().SetJobListExecuteFunc(MCore::JobListExecuteSerial);
        }
        else
        {
            MCore::GetMCore().SetJobListExecuteFunc(jobListExecuteFunction);
        }
    }

    // shrink internal pools to minimize memory usage
    void EMotionFXManager::ShrinkPools()
    {
        mAnimGraphManager->GetObjectDataPool().Shrink();
        mMotionInstancePool->Shrink();
        //MCore::GetAttributePool().Shrink();
    }


    // get the unit type
    MCore::Distance::EUnitType EMotionFXManager::GetUnitType() const
    {
        return mUnitType;
    }


    // set the unit type
    void EMotionFXManager::SetUnitType(MCore::Distance::EUnitType unitType)
    {
        mUnitType = unitType;
    }


    // register the EMotion FX memory categories
    void EMotionFXManager::RegisterMemoryCategories(MCore::MemoryTracker& memTracker)
    {
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_GEOMETRY_MATERIALS,                       "EMFX_MEMCATEGORY_GEOMETRY_MATERIALS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_GEOMETRY_MESHES,                          "EMFX_MEMCATEGORY_GEOMETRY_MESHES");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_GEOMETRY_DEFORMERS,                       "EMFX_MEMCATEGORY_GEOMETRY_DEFORMERS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_GEOMETRY_VERTEXATTRIBUTES,                "EMFX_MEMCATEGORY_GEOMETRY_VERTEXATTRIBUTES");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_GEOMETRY_PMORPHTARGETS,                   "EMFX_MEMCATEGORY_GEOMETRY_PMORPHTARGETS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MOTIONS_MOTIONINSTANCES,                  "EMFX_MEMCATEGORY_MOTIONS_MOTIONINSTANCES");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MOTIONS_MOTIONSYSTEMS,                    "EMFX_MEMCATEGORY_MOTIONS_MOTIONSYSTEMS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MOTIONS_SKELETALMOTIONS,                  "EMFX_MEMCATEGORY_MOTIONS_SKELETALMOTIONS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MOTIONS_INTERPOLATORS,                    "EMFX_MEMCATEGORY_MOTIONS_INTERPOLATORS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MOTIONS_KEYTRACKS,                        "EMFX_MEMCATEGORY_MOTIONS_KEYTRACKS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MOTIONS_MOTIONLINKS,                      "EMFX_MEMCATEGORY_MOTIONS_MOTIONLINKS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_EVENTS,                                   "EMFX_MEMCATEGORY_EVENTS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MOTIONS_MISC,                             "EMFX_MEMCATEGORY_MOTIONS_MISC");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MOTIONS_WAVELETSKELETALMOTIONS,           "EMFX_MEMCATEGORY_MOTIONS_WAVELETSKELETALMOTIONS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MOTIONS_MOTIONSETS,                       "EMFX_MEMCATEGORY_MOTIONS_MOTIONSETS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MOTIONS_MOTIONMANAGER,                    "EMFX_MEMCATEGORY_MOTIONS_MOTIONMANAGER");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_EVENTHANDLERS,                            "EMFX_MEMCATEGORY_EVENTHANDLERS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_EYEBLINKER,                               "EMFX_MEMCATEGORY_EYEBLINKER");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MOTIONS_GROUPS,                           "EMFX_MEMCATEGORY_MOTIONS_GROUPS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MOTIONINSTANCEPOOL,                       "EMFX_MEMCATEGORY_MOTIONINSTANCEPOOL");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_NODES,                                    "EMFX_MEMCATEGORY_NODES");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ACTORS,                                   "EMFX_MEMCATEGORY_ACTORS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ACTORINSTANCES,                           "EMFX_MEMCATEGORY_ACTORINSTANCES");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_NODEATTRIBUTES,                           "EMFX_MEMCATEGORY_NODEATTRIBUTES");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_NODESMISC,                                "EMFX_MEMCATEGORY_NODESMISC");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_NODEMAP,                                  "EMFX_MEMCATEGORY_NODEMAP");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_RIGSYSTEM,                                "EMFX_MEMCATEGORY_RIGSYSTEM");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_TRANSFORMDATA,                            "EMFX_MEMCATEGORY_TRANSFORMDATA");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_POSE,                                     "EMFX_MEMCATEGORY_POSE");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_TRANSFORM,                                "EMFX_MEMCATEGORY_TRANSFORM");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_SKELETON,                                 "EMFX_MEMCATEGORY_SKELETON");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_CONSTRAINTS,                              "EMFX_MEMCATEGORY_CONSTRAINTS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH,                               "EMFX_MEMCATEGORY_ANIMGRAPH");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_MANAGER,                       "EMFX_MEMCATEGORY_ANIMGRAPH_MANAGER");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_INSTANCE,                      "EMFX_MEMCATEGORY_ANIMGRAPH_INSTANCE");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREES,                    "EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREES");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES,                "EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_STATEMACHINES,                 "EMFX_MEMCATEGORY_ANIMGRAPH_STATEMACHINES");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_STATES,                        "EMFX_MEMCATEGORY_ANIMGRAPH_STATES");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_CONNECTIONS,                   "EMFX_MEMCATEGORY_ANIMGRAPH_CONNECTIONS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_ATTRIBUTEVALUES,               "EMFX_MEMCATEGORY_ANIMGRAPH_ATTRIBUTEVALUES");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_ATTRIBUTEINFOS,                "EMFX_MEMCATEGORY_ANIMGRAPH_ATTRIBUTEINFOS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTUNIQUEDATA,              "EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTUNIQUEDATA");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTS,                       "EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_CONDITIONS,                    "EMFX_MEMCATEGORY_ANIMGRAPH_CONDITIONS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_TRANSITIONS,                   "EMFX_MEMCATEGORY_ANIMGRAPH_TRANSITIONS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_SYNCTRACK,                     "EMFX_MEMCATEGORY_ANIMGRAPH_SYNCTRACK");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_POSE,                          "EMFX_MEMCATEGORY_ANIMGRAPH_POSE");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_PROCESSORS,                    "EMFX_MEMCATEGORY_ANIMGRAPH_PROCESSORS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_GAMECONTROLLER,                "EMFX_MEMCATEGORY_ANIMGRAPH_GAMECONTROLLER");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_EVENTBUFFERS,                  "EMFX_MEMCATEGORY_ANIMGRAPH_EVENTBUFFERS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_POSEPOOL,                      "EMFX_MEMCATEGORY_ANIMGRAPH_POSEPOOL");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_NODES,                         "EMFX_MEMCATEGORY_ANIMGRAPH_NODES");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_NODEGROUP,                     "EMFX_MEMCATEGORY_ANIMGRAPH_NODEGROUP");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_BLENDSPACE,                    "EMFX_MEMCATEGORY_ANIMGRAPH_BLENDSPACE");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTDATAPOOL,                "EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTDATAPOOL");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ANIMGRAPH_REFCOUNTEDDATA,                "EMFX_MEMCATEGORY_ANIMGRAPH_REFCOUNTEDDATA");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_WAVELETCACHE,                             "EMFX_MEMCATEGORY_WAVELETCACHE");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_WAVELETSKELETONMOTION,                    "EMFX_MEMCATEGORY_WAVELETSKELETONMOTION");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_IMPORTER,                                 "EMFX_MEMCATEGORY_IMPORTER");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_IDGENERATOR,                              "EMFX_MEMCATEGORY_IDGENERATOR");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ACTORMANAGER,                             "EMFX_MEMCATEGORY_ACTORMANAGER");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_UPDATESCHEDULERS,                         "EMFX_MEMCATEGORY_UPDATESCHEDULERS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_ATTACHMENTS,                              "EMFX_MEMCATEGORY_ATTACHMENTS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_EMOTIONFXMANAGER,                         "EMFX_MEMCATEGORY_EMOTIONFXMANAGER");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_FILEPROCESSORS,                           "EMFX_MEMCATEGORY_FILEPROCESSORS");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_EMSTUDIODATA,                             "EMFX_MEMCATEGORY_EMSTUDIODATA");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_RECORDER,                                 "EMFX_MEMCATEGORY_RECORDER");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_IK,                                       "EMFX_MEMCATEGORY_IK");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MESHBUILDER,                              "EMFX_MEMCATEGORY_MESHBUILDER");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MESHBUILDER_SKINNINGINFO,                 "EMFX_MEMCATEGORY_MESHBUILDER_SKINNINGINFO");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MESHBUILDER_SUBMESH,                      "EMFX_MEMCATEGORY_MESHBUILDER_SUBMESH");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MESHBUILDER_VERTEXLOOKUP,                 "EMFX_MEMCATEGORY_MESHBUILDER_VERTEXLOOKUP");
        memTracker.RegisterCategory(EMFX_MEMCATEGORY_MESHBUILDER_VERTEXATTRIBUTELAYER,         "EMFX_MEMCATEGORY_MESHBUILDER_VERTEXATTRIBUTELAYER");

        // actor group
        std::vector<uint32> idValues;
        idValues.reserve(100);
        idValues.push_back(EMFX_MEMCATEGORY_NODES);
        idValues.push_back(EMFX_MEMCATEGORY_ACTORS);
        idValues.push_back(EMFX_MEMCATEGORY_NODEATTRIBUTES);
        idValues.push_back(EMFX_MEMCATEGORY_NODESMISC);
        idValues.push_back(EMFX_MEMCATEGORY_ACTORINSTANCES);
        idValues.push_back(EMFX_MEMCATEGORY_TRANSFORMDATA);
        idValues.push_back(EMFX_MEMCATEGORY_POSE);
        idValues.push_back(EMFX_MEMCATEGORY_TRANSFORM);
        idValues.push_back(EMFX_MEMCATEGORY_SKELETON);
        idValues.push_back(EMFX_MEMCATEGORY_CONSTRAINTS);
        idValues.push_back(EMFX_MEMCATEGORY_GEOMETRY_MATERIALS);
        idValues.push_back(EMFX_MEMCATEGORY_GEOMETRY_MESHES);
        idValues.push_back(EMFX_MEMCATEGORY_GEOMETRY_DEFORMERS);
        idValues.push_back(EMFX_MEMCATEGORY_GEOMETRY_VERTEXATTRIBUTES);
        idValues.push_back(EMFX_MEMCATEGORY_GEOMETRY_PMORPHTARGETS);
        idValues.push_back(EMFX_MEMCATEGORY_EVENTHANDLERS);
        idValues.push_back(EMFX_MEMCATEGORY_EYEBLINKER);
        idValues.push_back(EMFX_MEMCATEGORY_ATTACHMENTS);
        idValues.push_back(EMFX_MEMCATEGORY_MESHBUILDER);
        idValues.push_back(EMFX_MEMCATEGORY_MESHBUILDER_SKINNINGINFO);
        idValues.push_back(EMFX_MEMCATEGORY_MESHBUILDER_SUBMESH);
        idValues.push_back(EMFX_MEMCATEGORY_MESHBUILDER_VERTEXLOOKUP);
        idValues.push_back(EMFX_MEMCATEGORY_MESHBUILDER_VERTEXATTRIBUTELAYER);
        idValues.push_back(EMFX_MEMCATEGORY_RIGSYSTEM);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_MANAGER);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_INSTANCE);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREES);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_STATEMACHINES);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_STATES);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_CONNECTIONS);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_ATTRIBUTEVALUES);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_ATTRIBUTEINFOS);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTUNIQUEDATA);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTS);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_CONDITIONS);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_TRANSITIONS);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_SYNCTRACK);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_POSE);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_PROCESSORS);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_GAMECONTROLLER);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_EVENTBUFFERS);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_POSEPOOL);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_NODES);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_NODEGROUP);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_BLENDSPACE);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_OBJECTDATAPOOL);
        idValues.push_back(EMFX_MEMCATEGORY_ANIMGRAPH_REFCOUNTEDDATA);
        idValues.push_back(MCore::MCORE_MEMCATEGORY_ATTRIBUTEPOOL);
        idValues.push_back(MCore::MCORE_MEMCATEGORY_ATTRIBUTEFACTORY);
        idValues.push_back(MCore::MCORE_MEMCATEGORY_ATTRIBUTES);
        memTracker.RegisterGroup(EMFX_MEMORYGROUP_ACTORS, "EMFX_MEMORYGROUP_ACTORS", idValues);

        // motion group
        idValues.clear();
        idValues.push_back(EMFX_MEMCATEGORY_MOTIONS_MOTIONINSTANCES);
        idValues.push_back(EMFX_MEMCATEGORY_MOTIONS_MOTIONSYSTEMS);
        idValues.push_back(EMFX_MEMCATEGORY_MOTIONS_SKELETALMOTIONS);
        idValues.push_back(EMFX_MEMCATEGORY_MOTIONS_INTERPOLATORS);
        idValues.push_back(EMFX_MEMCATEGORY_MOTIONS_KEYTRACKS);
        idValues.push_back(EMFX_MEMCATEGORY_MOTIONS_MOTIONLINKS);
        idValues.push_back(EMFX_MEMCATEGORY_EVENTS);
        idValues.push_back(EMFX_MEMCATEGORY_MOTIONS_MISC);
        idValues.push_back(EMFX_MEMCATEGORY_MOTIONS_WAVELETSKELETALMOTIONS);
        idValues.push_back(EMFX_MEMCATEGORY_MOTIONS_MOTIONSETS);
        idValues.push_back(EMFX_MEMCATEGORY_MOTIONS_MOTIONMANAGER);
        idValues.push_back(EMFX_MEMCATEGORY_MOTIONS_GROUPS);
        idValues.push_back(EMFX_MEMCATEGORY_MOTIONINSTANCEPOOL);
        idValues.push_back(EMFX_MEMCATEGORY_NODEMAP);
        idValues.push_back(MCore::MCORE_MEMCATEGORY_WAVELETS);
        idValues.push_back(MCore::MCORE_MEMCATEGORY_HUFFMAN);
        memTracker.RegisterGroup(EMFX_MEMORYGROUP_MOTIONS, "EMFX_MEMORYGROUP_MOTIONS", idValues);
    }
} //  namespace EMotionFX
