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

#ifndef CRYINCLUDE_CRYACTION_TESTSYSTEM_TIMEDEMORECORDER_H
#define CRYINCLUDE_CRYACTION_TESTSYSTEM_TIMEDEMORECORDER_H
#pragma once

#include <CryListenerSet.h>
#include <IEntitySystem.h>
#include <IGameplayRecorder.h>
#include <IInput.h>
#include <ITimeDemoRecorder.h>
#include <ITestModule.h>

struct SRecordedGameEvent;

struct STimeDemoGameEvent
{
    string entityName;
    uint8 gameEventType;
    string description;
    string description2;
    float value;
    int extra;

    STimeDemoGameEvent() {}
    STimeDemoGameEvent(IEntity* pEntity, const GameplayEvent& event);

    STimeDemoGameEvent(const SRecordedGameEvent& event);

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(entityName);
        pSizer->AddObject(description);
        pSizer->AddObject(description2);
    }
};
typedef std::vector<STimeDemoGameEvent> TGameEventRecords;

class CTimeDemoRecorder
    : public TimeDemoRecorderBus::Handler
    , IFrameProfilePeakCallback
    , IInputEventListener
    , IEntitySystemSink
    , IGameplayListener
{
public:
    CTimeDemoRecorder();
    virtual ~CTimeDemoRecorder();




    //////////////////////////////////////////////////////////////////////////
    // TimeDemoRecorder interface implementation.
    //////////////////////////////////////////////////////////////////////////
    bool IsRecording() const override { return m_bRecording; }
    bool IsPlaying() const override { return m_bPlaying; }
    void RegisterListener(ITimeDemoListener* pListener) override;
    void UnregisterListener(ITimeDemoListener* pListener) override;
    void GetCurrentFrameRecord(STimeDemoFrameRecord& externalRecord) const override;

    void PreUpdate() override;
    void PostUpdate() override;
    void Reset() override;

    bool IsChainLoading() const override { return m_bChainloadingDemo; }
    bool IsTimeDemoActive() const override { return m_bChainloadingDemo || m_bPlaying || m_bRecording; }
    void GetMemoryStatistics(class ICrySizer* pSizer) const override;
    //////////////////////////////////////////////////////////////////////////

private:
    //////////////////////////////////////////////////////////////////////////
    // Implements IFrameProfilePeakCallback interface.
    //////////////////////////////////////////////////////////////////////////
    virtual void OnFrameProfilerPeak(CFrameProfiler* pProfiler, float fPeakTime) override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Implements IInputEventListener interface.
    //////////////////////////////////////////////////////////////////////////
    virtual bool OnInputEvent(const SInputEvent& event) override;
    //////////////////////////////////////////////////////////////////////////
    // Implements IGameplayListener interface.
    //////////////////////////////////////////////////////////////////////////
    virtual void OnGameplayEvent(IEntity* pEntity, const GameplayEvent& event) override;

    //////////////////////////////////////////////////////////////////////////
    // IEntitySystemSink
    //////////////////////////////////////////////////////////////////////////
    virtual bool OnBeforeSpawn(SEntitySpawnParams& params) override;
    virtual void OnSpawn(IEntity* pEntity, SEntitySpawnParams& params) override;
    virtual bool OnRemove(IEntity* pEntity) override;
    virtual void OnReused(IEntity* pEntity, SEntitySpawnParams& params) override;
    virtual void OnEvent(IEntity* pEntity, SEntityEvent& event) override;
    //////////////////////////////////////////////////////////////////////////

private:
    // Input event list.
    struct EntityEventRecord
    {
        EntityId   entityId;      // What entity performed event.
        EntityGUID guid;          // What entity performed event.

        uint32 eventType;         // What event.
        uint64 nParam[4];         // event params.
        Vec3 pos;
        Quat q;

        enum Flags
        {
            HIDDEN = BIT(1),
        };

        // Special flags.
        uint32 flags;

        void GetMemoryUsage(ICrySizer* pSizer) const{}
    };
    typedef std::vector<SInputEvent> InputEventsList;
    typedef std::vector<EntityEventRecord> EntityEventRecords;

    //! This structure saved for every frame of time demo.
    struct FrameRecord
    {
        FrameRecord()
            : playerPosition(ZERO)
            , playerRotation(IDENTITY)
            , playerViewRotation(IDENTITY)
            , hmdPositionOffset(ZERO)
            , hmdViewRotation(IDENTITY)
            , cameraAngles(ZERO)
            , frameTime(0.0f)
            , fLeaning(0.0f)
            , nPolygons(0)
        {
            nActionFlags[0] = 0;
            nActionFlags[1] = 0;
        }

        Vec3 playerPosition;
        Quat playerRotation;
        Quat playerViewRotation;
        Vec3 hmdPositionOffset;
        Quat hmdViewRotation;
        Ang3 cameraAngles; // Legacy
        float frameTime; // Immediate frame rate, for this frame.

        // Snapshot of current processing command.
        unsigned int nActionFlags[2];
        float fLeaning;
        int nPolygons; // Polys rendered in this frame.

        InputEventsList inputEventsList;
        EntityEventRecords entityEvents;
        TGameEventRecords gameEvents;

        void GetMemoryUsage(ICrySizer* pSizer) const{}
    };

private:
    void StartSession();
    void StopSession();

    float RenderInfo(float y = 0);
    void Record(bool bEnable);
    void Play(bool bEnable);
    ETestModuleType GetType() const {return TM_TIMEDEMO; }
    void ParseParams(XmlNodeRef node);
    void SetVariable(const char* name, const char* szValue);
    void SetVariable(const char* name, float value);

    //! Get number of frames in record.
    int GetNumFrames() const;
    float GetAverageFrameRate() const;

    void Save(const char* filename);
    bool Load(const char* filename);

    void StartChainDemo(const char* levelsListFilename, bool bAutoLoadChainConfig);
    void StartDemoLevel(const char** levelNames, int levelCount);
    void StartDemoDelayed(int nFrames);

    void Pause(bool paused) {m_bPaused = paused; }

    bool RecordFrame();
    bool PlayFrame();
    int GetNumberOfFrames();
    int GetTotalPolysRecorded() {return m_nTotalPolysRecorded; }
    void LogEndOfLoop();

    static const char* GetCurrentLevelPath();

    CTimeValue GetTime();
    // Set Value of console variable.
    void SetConsoleVar(const char* sVarName, float value);
    // Get value of console variable.
    float GetConsoleVar(const char* sVarName);

    int ComputePolyCount();

    void ResetSessionLoop();

    void EraseLogFile();
    void LogInfo(const char* format, ...) PRINTF_PARAMS(2, 3);

    void PlayBackEntityEvent(const EntityEventRecord& rec);
    void SaveAllEntitiesState();
    void RestoreAllEntitiesState();

    string GetCurrentLevelName();
    string GetInitSaveFileName();

    void StartNextChainedLevel();
    void SaveChainloadingJUnitResults();
    void EndDemo();
    void QuitGame();
    void ProcessKeysInput();
    void ReplayGameState(struct FrameRecord& rec);
    void SaveDemoFinishedLog();

    void AddFrameRecord(const FrameRecord& rec);

    void SignalPlayback(bool bEnable);
    void SignalRecording(bool bEnable);
    void SignalPlayFrame();

protected:
    static void cmd_StartRecordingTimeDemo(IConsoleCmdArgs* pArgs);
    static void cmd_Play(IConsoleCmdArgs* pArgs);
    static void cmd_StartDemoChain(IConsoleCmdArgs* pArgs);
    static void cmd_StartDemoLevel(IConsoleCmdArgs* pArgs);
    static void cmd_Stop(IConsoleCmdArgs* pArgs);

private:
    typedef CListenerSet<ITimeDemoListener*> TTimeDemoListeners;
    TTimeDemoListeners m_listeners;

    typedef std::vector<FrameRecord> FrameRecords;
    FrameRecords m_records;

    bool m_bRecording;
    bool m_bPlaying;
    bool m_bPaused;
    bool m_bDemoFinished;
    bool m_demoEnded;
    bool m_bChainloadingDemo;

    //! Current play or record frame.
    int m_currentFrame;

    std::vector<SInputEvent> m_currentFrameInputEvents;
    EntityEventRecords m_currentFrameEntityEvents;
    EntityEventRecords m_firstFrameEntityState;

    TGameEventRecords m_currentFrameGameEvents;

    //////////////////////////////////////////////////////////////////////////
    // Old values of console vars.
    //////////////////////////////////////////////////////////////////////////

    //! Timings.
    CTimeValue m_recordStartTime;
    CTimeValue m_recordEndTime;
    //CTimeValue m_recordLastFrameTime;
    CTimeValue m_lastFrameTime;
    CTimeValue m_totalDemoTime;
    CTimeValue m_recordedDemoTime;

    // How many polygons per frame where recorded.
    int m_nTotalPolysRecorded;
    // How many polygons per frame where played.
    int m_nTotalPolysPlayed;

    float m_lastPlayedTotalTime;
    float m_lastAveFrameRate;
    float m_sumFPS;
    float m_minFPS;
    float m_maxFPS;
    float m_currFPS;


    int m_minFPSCounter;
    int m_minFPS_Frame;
    int m_maxFPS_Frame;

    int m_nCurrPolys;
    int m_nMaxPolys;
    int m_nMinPolys;
    int m_nPolysPerSec;
    int m_nPolysCounter;

    // For calculating current last second fps.
    CTimeValue m_lastFpsTimeRecorded;
    int m_fpsCounter;

    int m_fileVersion;

    bool m_bEnabledProfiling, m_bVisibleProfiling;

    float m_oldPeakTolerance;
    float m_fixedTimeStep;

    string m_file;

    //  IGameStateRecorder* m_pGameStateRecorder;

    struct STimeDemoInfo* m_pTimeDemoInfo;

public:
    static ICVar* s_timedemo_file;
    static CTimeDemoRecorder* s_pTimeDemoRecorder;

    int m_numLoops;
    int m_maxLoops;
    int m_numOrientations;

    int m_demo_scroll_pause;
    int m_demo_quit;
    int m_finish_replaysizer;
    int m_finish_replaystop;
    int m_demo_screenshot_frame;
    int m_demo_max_frames;
    int m_demo_savestats;
    int m_demo_ai;
    int m_demo_restart_level;
    int m_demo_panoramic;
    int m_demo_fixed_timestep;
    int m_demo_vtune;
    int m_demo_time_of_day;
    int m_demo_gameState;
    int m_demo_profile;
    int m_demo_noinfo;
    int m_demo_save_every_frame;
    int m_demo_use_hmd_rotation;

    bool m_bAIEnabled;

    int m_countDownPlay;
    int m_prevGodMode;

    struct SChainDemoLevel
    {
        string level;   // Level name
        float time;     // Time of test in seconds
        bool bSuccess; // If test was successful.
        bool bRun;     // If test was successful.
    };
    std::vector<SChainDemoLevel> m_demoLevels;
    int m_nCurrentDemoLevel;
    float m_lastChainDemoTime;
};

#endif // CRYINCLUDE_CRYACTION_TESTSYSTEM_TIMEDEMORECORDER_H
