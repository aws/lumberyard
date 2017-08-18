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

// Description : Music System Interface.


#pragma once

#include <StlUtils.h>
#include <ISerialize.h>
#include <CryName.h>

//////////////////////////////////////////////////////////////////////////
// String Iterator
struct IStringItVec
{
    virtual ~IStringItVec() {}
    virtual bool IsEnd() = 0;
    virtual const char* Next() = 0;
    virtual void MoveFirst() = 0;
    virtual void AddRef() = 0;
    virtual void Release() = 0;
};

//////////////////////////////////////////////////////////////////////////
// MusicSystem

// Different layers
#define MUSICLAYER_NONE             0x00000000
#define MUSICLAYER_MAIN             0x00000001
#define MUSICLAYER_RHYTHMIC         0x00000002
#define MUSICLAYER_INCIDENTAL       0x00000004
#define MUSICLAYER_STINGER          0x00000008
#define MUSICLAYER_START            0x00000010
#define MUSICLAYER_END              0x00000020
#define MUSICLAYER_PATTERN_BY_NAME  0x00000040
#define MUSICLAYER_MAIN_ANY         0x000000FF

//
#define MUSICLAYER_ANY              0xFFFFFFFF

#define MUSIC_THEME_NAME_LENGTH_MAX     64
#define MUSIC_MOOD_NAME_LENGTH_MAX      64
#define MUSIC_PATTERN_NAME_LENGTH_MAX   256

#define DEFAULT_MOOD_CROSS_FADE_TIME    5.0f
#define DEFAULT_MOOD_FADE_IN_TIME       0.0f
#define DEFAULT_MOOD_FADE_OUT_TIME      5.0f

struct IRenderer;

namespace Audio
{
    // Type of blending
    enum EBlendingType
    {
        EBlend_None,
        EBlend_PlayOnFadePoint,
        EBlend_EndOnFadePoint,
        EBlend_FadeIn,
        EBlend_FadeInOnFadePoint,
        EBlend_FadeOut,
        EBlend_FadeOutOnFadePoint,
        EBlend_FadeOutOnMainEnd,
        EBlend_Any,
        EBlend_MAX,
    };

    // Type of playing
    enum EPlayingType
    {
        EPlaying_Loop,
        EPlaying_Once,
        EPlaying_Trigger_Next,
        EPlaying_Stop,
        EPlaying_Any,
        EPlaying_MAX,
    };

    // Type of ThemeFading
    enum EThemeFadeType
    {
        EThemeFade_StopAtOnce,
        EThemeFade_FadeOut,
        EThemeFade_PlayEnd,
        EThemeFade_PlayEndAtFadePoint,
        EThemeFade_KeepMood,
        EThemeFade_MAX,
    };

    // Type of theme autoending
    enum EAutoEndThemeBehavior
    {
        eAETB_EndThemeOnLoadEnd,
        eAETB_EndThemeOnGamePlayStart,
        eAETB_DoNothing
    };


    //////////////////////////////////////////////////////////////////////////
    // Structures to pass as data-entry for music system
    //////////////////////////////////////////////////////////////////////////

    struct SPatternDef;
    struct SMusicPatternSet;
    struct SMusicMood;
    struct SMusicTheme;
    struct IMusicSystemEventListener;

    // Helper integer-vector
    typedef std::vector<size_t> TIntVec;
    typedef TIntVec::iterator TIntVecIt;

    typedef DynArray<uint8> TVecUint8;

    typedef std::vector<SPatternDef*> TPatternDefVec;
    typedef TPatternDefVec::iterator TPatternDefVecIt;

    typedef std::map<string, string, stl::less_stricmp<string> > TThemeBridgeMap;
    typedef TThemeBridgeMap::iterator TThemeBridgeMapIt;

    typedef std::map<string, SMusicMood*, stl::less_stricmp<string> > TMoodMap;
    typedef TMoodMap::iterator TMoodMapIt;
    typedef TMoodMap::const_iterator TMoodMapItConst;

    typedef std::vector<SMusicPatternSet*> TPatternSetVec;
    typedef TPatternSetVec::iterator TPatternSetVecIt;

    typedef std::map<string, SMusicTheme*, stl::less_stricmp<string> > TThemeMap;
    typedef TThemeMap::iterator TThemeMapIt;
    typedef TThemeMap::const_iterator TThemeMapItConst;

    typedef void* tAssetHandle;

    // Pattern-definition
    struct SPatternDef
    {
        SPatternDef()
            : fLayeringVolume(1.0f)
            , fProbabilityPoints(1.0f)
            , nCustomIndex(0)
            , nPreFadeIn(0)
        {}

        string sName;
        string sFilename;
        TIntVec vecFadePoints;
        float fLayeringVolume;
        float fProbabilityPoints;
        uint8 nCustomIndex;
        int nPreFadeIn;
    };

    // PatternSet-Structure used by moods
    struct SMusicPatternSet
    {
        SMusicPatternSet()
            : fMinTimeout(0.0f)
            , fMaxTimeout(0.0f)
            , fRhythmicLayerProbabilityInPercent(0.0f)
            , fIncidentalLayerProbabilityInPercent(0.0f)
            , fTotalMainPatternProbability(0.0f)
            , fTotalRhythmicPatternProbability(0.0f)
            , fTotalIncidentalPatternProbability(0.0f)
            , fTotalStingerPatternProbability(0.0f)
            , nMaxSimultaneousRhythmicPatterns(1)
            , nMaxSimultaneousIncidentalPatterns(1)
            , bPlayRhythmicPatternSynced(false)
            , bPlayIncidentalPatternSynced(false)
            , bLoopCustomOrder(false)
        {}

        //Pattern set
        float fMinTimeout;
        float fMaxTimeout;

        //Main
        float fTotalMainPatternProbability; // added probabilities of all containing patterns
        TPatternDefVec vecMainPatterns;
        TVecUint8 anCustomOrder;
        bool bLoopCustomOrder;

        //Rhythmic
        int nMaxSimultaneousRhythmicPatterns;
        float fRhythmicLayerProbabilityInPercent;
        float fTotalRhythmicPatternProbability; // added probabilities of all containing patterns
        bool bPlayRhythmicPatternSynced;
        TPatternDefVec vecRhythmicPatterns;

        //Incidental
        int nMaxSimultaneousIncidentalPatterns;
        float fIncidentalLayerProbabilityInPercent;
        float fTotalIncidentalPatternProbability; // added probabilities of all containing patterns
        bool bPlayIncidentalPatternSynced;
        TPatternDefVec vecIncidentalPatterns;

        //Start
        TPatternDefVec vecStartPatterns;

        //End
        TPatternDefVec vecEndPatterns;

        //Stinger
        TPatternDefVec vecStingerPatterns;
        float fTotalStingerPatternProbability;
    };

    // Mood-Structure
    struct SMusicMood
    {
        SMusicMood()
            : nPriority(0)
            , fFadeOutTime(DEFAULT_MOOD_FADE_OUT_TIME)
            , fFadeInTime(DEFAULT_MOOD_FADE_IN_TIME)
            , bPlaySingle(false)
            , bPlayFromStart(false)
            , bMainSpawned(false)
            , bStopped(false)
            , pCurrPatternSet(nullptr)
            , fCurrPatternSetTime(0.0f)
            , fCurrPatternSetTimeout(0.0f)
            , nPatternRandIndex(0)
            , pSpawnedFromTheme(nullptr)
            , nStartLayerPlayCount(-1)
        {}

        string sName;
        int nPriority;
        float fFadeOutTime;
        float fFadeInTime;
        TPatternSetVec vecPatternSets;
        bool bPlaySingle;
        bool bPlayFromStart;
        bool bMainSpawned;
        bool bStopped;
        // internal
        SMusicPatternSet* pCurrPatternSet;
        float fCurrPatternSetTime;
        float fCurrPatternSetTimeout;
        int nPatternRandIndex;
        int nStartLayerPlayCount;
        SMusicTheme* pSpawnedFromTheme;
    };

    // Theme-Structure
    struct SMusicTheme
    {
        SMusicTheme()
            : fDefaultMoodTimeout(0.0f)
            , fMoodCrossFadeTime(DEFAULT_MOOD_CROSS_FADE_TIME)
            , bKeepMood(false)
            , bForceChange(false)
            , pListener(nullptr)
        {}

        string sName;
        TMoodMap mapMoods;
        TThemeBridgeMap mapBridges;
        string sDefaultMood;
        string sStartMood;
        float fDefaultMoodTimeout;
        float fMoodCrossFadeTime;
        bool bKeepMood;
        bool bForceChange;
        const IMusicSystemEventListener* pListener;
    };


    //////////////////////////////////////////////////////////////////////////
    // Music interface structures.
    //////////////////////////////////////////////////////////////////////////
    struct SMusicInfo
    {
        struct Pattern
        {
            Pattern()
                : sTheme(nullptr)
                , sMood(nullptr)
                , nPatternSetIndex(0)
                , nPatternType(MUSICLAYER_NONE)
                , sName(nullptr)
                , sFilename(nullptr)
                , pFadePoints(nullptr)
                , nFadePointsCount(0)
                , nPreFadeIn(0)
                , fLayeringVolume(0.0f)
                , fProbabilityPoints(0.0f)
                , nCustomIndex(0)
            {}

            const char* sTheme;
            const char* sMood;
            int nPatternSetIndex; // Index in mood pattern set.
            int nPatternType;   // Type of pattern (Main/Rhythmic/Incidental)

            const char* sName;
            const char* sFilename;
            size_t* pFadePoints;
            int nFadePointsCount;
            int nPreFadeIn;
            float fLayeringVolume;
            float fProbabilityPoints;
            uint8 nCustomIndex;
        };

        //////////////////////////////////////////////////////////////////////////
        // PatternSet-Structure used by moods
        struct PatternSet
        {
            PatternSet()
                : fMinTimeout(0.0f)
                , fMaxTimeout(0.0f)
                , fTotalMainPatternProbability(0.0f)
                , nMaxSimultaneousRhythmicPatterns(0)
                , fRhythmicLayerProbabilityInPercent(0.0f)
                , fTotalRhythmicPatternProbability(0.0f)
                , bPlayRhythmicPatternSynced(false)
                , nMaxSimultaneousIncidentalPatterns(0)
                , fIncidentalLayerProbabilityInPercent(0.0f)
                , fTotalIncidentalPatternProbability(0.0f)
                , bPlayIncidentalPatternSynced(false)
                , nMaxSimultaneousStartPatterns(0)
                , fStartLayerProbability(0.0f)
                , fTotalStartPatternProbability(0.0f)
                , nMaxSimultaneousEndPatterns(0)
                , fEndLayerProbability(0.0f)
                , fTotalEndPatternProbability(0.0f)
                , fTotalStingerPatternProbability(0.0f)
                , bLoopCustomOrder(false)
            {}

            float fMinTimeout;
            float fMaxTimeout;
            float fTotalMainPatternProbability; // added probabilities of all containing patterns
            TVecUint8 anCustomOrder;
            bool bLoopCustomOrder;

            int nMaxSimultaneousRhythmicPatterns;
            float fRhythmicLayerProbabilityInPercent;
            float fTotalRhythmicPatternProbability; // added probabilities of all containing patterns
            bool bPlayRhythmicPatternSynced;

            int nMaxSimultaneousIncidentalPatterns;
            float fIncidentalLayerProbabilityInPercent;
            float fTotalIncidentalPatternProbability; // added probabilities of all containing patterns
            bool bPlayIncidentalPatternSynced;

            int nMaxSimultaneousStartPatterns;
            float fStartLayerProbability;
            float fTotalStartPatternProbability; // added probabilities of all containing patterns

            int nMaxSimultaneousEndPatterns;
            float fEndLayerProbability;
            float fTotalEndPatternProbability; // added probabilities of all containing patterns

            float fTotalStingerPatternProbability; // added probabilities of all containing patterns
        };

        //////////////////////////////////////////////////////////////////////////
        struct Mood
        {
            Mood()
                : sTheme(nullptr)
                , sName(nullptr)
                , nPriority(0)
                , fFadeOutTime(DEFAULT_MOOD_FADE_OUT_TIME)
                , fFadeInTime(DEFAULT_MOOD_FADE_IN_TIME)
                , bPlaySingle(false)
                , pPatternSets(nullptr)
                , nPatternSetsCount(0)
                , nStartLayerPlayCount(-1)
            {}

            const char* sTheme;
            const char* sName;
            int nPriority;
            float fFadeOutTime;
            float fFadeInTime;
            bool bPlaySingle;
            PatternSet* pPatternSets;
            int nPatternSetsCount;
            int nStartLayerPlayCount;
        };

        //////////////////////////////////////////////////////////////////////////
        struct Theme
        {
            Theme()
                : sName(nullptr)
                , sDefaultMood(nullptr)
                , fDefaultMoodTimeout(0.0f)
                , fMoodCrossFadeTime(DEFAULT_MOOD_CROSS_FADE_TIME)
                , pBridges(nullptr)
                , nBridgeCount(0)
            {}

            const char* sName;
            const char* sDefaultMood;
            float fDefaultMoodTimeout;
            float fMoodCrossFadeTime;
            const char** pBridges; // Pairs of strings.
            int nBridgeCount;
        };

        struct Data
        {
            Data()
                : pThemes(nullptr)
                , nThemes(0)
                , pMoods(nullptr)
                , nMoods(0)
                , pPatterns(nullptr)
                , nPatterns(0)
            {}

            Theme* pThemes;
            int nThemes;
            Mood* pMoods;
            int nMoods;
            Pattern* pPatterns;
            int nPatterns;
        };
    };

    //////////////////////////////////////////////////////////////////////////
    // MusicSystem events to callback MusicSystem events listeners.
    //////////////////////////////////////////////////////////////////////////
    enum EMusicSystemCallbackEvent
    {
        MUSICSYSTEM_NO_EVENT,               // Nothing!
        MUSICSYSTEM_EVENT_ON_UPDATE,        // Fired every time the music system is updated.
        MUSICSYSTEM_EVENT_ON_THEME_START,   // Fired when a theme starts.
        MUSICSYSTEM_EVENT_ON_THEME_END,     // Fired when a theme ends.
        MUSICSYSTEM_EVENT_ON_MOOD_SWITCH,   // Fired when a mood switch occurred.
        MUSICSYSTEM_EVENT_ON_UPDATE_LOOP_COUNT, // Fired when the pattern playing on the main layer looped again.
        MUSICSYSTEM_EVENT_ON_PATTERN_START, // Fired when a pattern starts.
        MUSICSYSTEM_EVENT_ON_PATTERN_FINISHED, // Fired when a pattern played until its end.
        MUSICSYSTEM_EVENT_ON_PATTERN_STOP,  // Fired when a pattern was stopped prematurely.
        MUSICSYSTEM_EVENT_ON_STOP,          // Fired when the music system stops playing.
    };

    //////////////////////////////////////////////////////////////////////////
    // music system info struct
    //////////////////////////////////////////////////////////////////////////
    struct SMusicSystemInfo
    {
        SMusicSystemInfo()
            : nMainPatternCurrentLoopCount(0)
            , fCurrentProgress(0.0f)
            , fPlayTimeInSeconds(0.0f)
            , fRemainingPlayTimeInSeconds(0.0f)
            , pListener(nullptr)
            , eEvent(MUSICSYSTEM_NO_EVENT)
        {}

        size_t nMainPatternCurrentLoopCount;    // Indicates the amount of loops the same pattern performed (gets reset once the pattern changes)
        float fCurrentProgress;
        float fPlayTimeInSeconds;
        float fRemainingPlayTimeInSeconds;
        EMusicSystemCallbackEvent eEvent;
        CryFixedStringT<MUSIC_MOOD_NAME_LENGTH_MAX> sCurrentMoodName;
        const IMusicSystemEventListener* pListener;
    };

    //////////////////////////////////////////////////////////////////////////
    // Event Info.
    //////////////////////////////////////////////////////////////////////////
    struct SMusicSystemEventInfo
    {
        SMusicSystemEventInfo()
            : pListener(nullptr)
            , eEvent(MUSICSYSTEM_NO_EVENT)
        {}

        EMusicSystemCallbackEvent eEvent;
        const IMusicSystemEventListener* pListener;
    };

    //////////////////////////////////////////////////////////////////////////
    //   Listener interface for the musicsystem.
    //////////////////////////////////////////////////////////////////////////
    struct IMusicSystemEventListener
    {
        virtual ~IMusicSystemEventListener() {}

        virtual void OnMusicSystemEvent(const SMusicSystemInfo& rMusicSystemInfo) = 0;
    };

    //////////////////////////////////////////////////////////////////////////
    // Status struct
    //////////////////////////////////////////////////////////////////////////
    struct SPlayingPatternsStatus
    {
        SPlayingPatternsStatus()
            : nInstances(0)
            , nLayer(0)
            , nLength(0)
            , nSamplePos(0)
            , nSamplesToNextFade(0)
            , fVolume(0.0f)
            , fPhase(0.0f)
            , eBlendType(EBlend_None)
            , ePlayingType(EPlaying_Any)
        {}

        string sPatternName;
        string sFileName;
        unsigned int nInstances;
        unsigned int nLayer;
        uint32 nLength;
        int nSamplePos;
        int nSamplesToNextFade;
        float fVolume;
        float fPhase;
        EBlendingType eBlendType;
        EPlayingType ePlayingType;
    };

    typedef std::vector<SPlayingPatternsStatus> TPatternStatusVec;
    typedef TPatternStatusVec::iterator TPatternStatusVecIt;

    class CMusicPatternInstance;

    typedef _smart_ptr<CMusicPatternInstance> TPatternInstancePtr;
    typedef std::vector<TPatternInstancePtr> TPatternInstancesPtrVec;
    typedef std::vector<CMusicPatternInstance*> TPatternInstancesVec;

    struct SMusicSystemStatus
    {
        bool bPlaying;
        string sTheme;
        string sMood;
        TPatternStatusVec m_vecStatusPlayingPatterns;
    };

    enum EMusicLogicEvents
    {
        eMUSICLOGICEVENT_SET_MULTIPLIER = 0,
        eMUSICLOGICEVENT_SET_AI_MULTIPLIER,
        eMUSICLOGICEVENT_SET_AI,
        eMUSICLOGICEVENT_CHANGE_AI,
        eMUSICLOGICEVENT_SET_PLAYER,
        eMUSICLOGICEVENT_CHANGE_PLAYER,
        eMUSICLOGICEVENT_SET_GAME,
        eMUSICLOGICEVENT_CHANGE_GAME,
        eMUSICLOGICEVENT_SET_ALLOWCHANGE,
        eMUSICLOGICEVENT_CHANGE_ALLOWCHANGE,
        eMUSICLOGICEVENT_VEHICLE_ENTER,
        eMUSICLOGICEVENT_VEHICLE_LEAVE,
        eMUSICLOGICEVENT_WEAPON_MOUNT,
        eMUSICLOGICEVENT_WEAPON_UNMOUNT,
        eMUSICLOGICEVENT_SNIPERMODE_ENTER,
        eMUSICLOGICEVENT_SNIPERMODE_LEAVE,
        eMUSICLOGICEVENT_CLOAKMODE_ENTER,
        eMUSICLOGICEVENT_CLOAKMODE_LEAVE,
        eMUSICLOGICEVENT_ENEMY_SPOTTED,
        eMUSICLOGICEVENT_ENEMY_KILLED,
        eMUSICLOGICEVENT_ENEMY_HEADSHOT,
        eMUSICLOGICEVENT_ENEMY_OVERRUN,
        eMUSICLOGICEVENT_PLAYER_WOUNDED,
        eMUSICLOGICEVENT_PLAYER_KILLED,
        eMUSICLOGICEVENT_PLAYER_SPOTTED,
        eMUSICLOGICEVENT_PLAYER_TURRET_ATTACK,
        eMUSICLOGICEVENT_PLAYER_SWIM_ENTER,
        eMUSICLOGICEVENT_PLAYER_SWIM_LEAVE,
        eMUSICLOGICEVENT_EXPLOSION,
        eMUSICLOGICEVENT_FACTORY_CAPTURED,
        eMUSICLOGICEVENT_FACTORY_LOST,
        eMUSICLOGICEVENT_FACTORY_RECAPTURED,
        eMUSICLOGICEVENT_VEHICLE_CREATED,
        eMUSICLOGICEVENT_MAX
    };

    enum EMusicNodeInputOperation
    {
        eMNIO_Equal,
        eMNIO_GreaterThan,
        eMNIO_LessThan,
        eMNIO_NotEqual,
        eMNIO_Count,
    };

    typedef uint32 TMusicLogicEventId;
#define MUSICLOGIC_INVALID_EVENT_ID ((Audio::TMusicLogicEventId)0)

    typedef uint32 TMusicLogicInputId;
#define MUSICLOGIC_INVALID_INPUT_ID ((Audio::TMusicLogicInputId)0)

    //////////////////////////////////////////////////////////////////////////
    // Music-Logic interface
    //////////////////////////////////////////////////////////////////////////

    struct IMusicLogic
    {
        virtual ~IMusicLogic() {}

        virtual bool Init() = 0;
        virtual void Reset() = 0;
        virtual bool Start(const CCryNameCRC& rNamePreset) = 0;
        virtual bool Stop() = 0;
        virtual bool IsActive() const = 0;
        virtual void Pause(const bool bPause) = 0;
        virtual void Update(const CTimeValue& rTimeValue) = 0;
        virtual void SetEvent(EMusicLogicEvents MusicEvent, const float fValue = 0.0f) = 0;
        virtual void SendEvent(TMusicLogicEventId eventId, float fData = 0.f) = 0;
        virtual TMusicLogicEventId GetEventId(const char* pEventName) = 0;
        virtual void GetMemoryStatistics(ICrySizer* pICrySizer) = 0;
        virtual void Serialize(TSerialize ser) = 0;
        virtual bool SetPreset(const CCryNameCRC& rNamePreset) = 0;
        virtual void Reload() = 0;

#if !defined(_RELEASE)
        virtual void DrawInformation(IRenderer* pRenderer, float xpos, float ypos, int nSoundInfo) = 0;
#endif // _RELEASE
    };


    //////////////////////////////////////////////////////////////////////////
    // Main music-interface
    //////////////////////////////////////////////////////////////////////////
    struct IMusicSystem
    {
        virtual ~IMusicSystem() {}

        virtual void PostInit() = 0;
        virtual void Release() = 0;
        virtual void Update() = 0;

        virtual int GetBytesPerSample() = 0;
        virtual struct IMusicSystemSink* SetSink(struct IMusicSystemSink* pSink) = 0;
        virtual bool SetData(SMusicInfo::Data* pMusicData) = 0;
        virtual void Unload() = 0;
        virtual void Pause(bool bPause) = 0;
        virtual bool IsPaused() const = 0;
        virtual void EnableEventProcessing(bool bEnable) = 0;
        virtual void OverrideNativeBackgroundMusicPlayer(const bool bOberride) = 0;
        virtual const bool IsOverrideNativeBackgroundMusicPlayer() const = 0;

        // Theme stuff
        virtual bool SetTheme(const char* pszTheme, bool bForceChange = true, bool bKeepMood = true, int nDelayInSec = -1, const char* const sStartMood = nullptr, const IMusicSystemEventListener* const pListener = nullptr) = 0;
        virtual bool EndTheme(EThemeFadeType ThemeFade = EThemeFade_FadeOut, int nForceEndLimitInSec = 10, bool bEndEverything = true) = 0;
        virtual const char* GetTheme() const = 0;
        virtual IStringItVec* GetThemes() const = 0;
        virtual CTimeValue GetThemeTime() const = 0;

        // Mood stuff
        virtual bool SetMood(const char* pszMood, const bool bPlayFromStart = true, const bool bForceChance = false, const IMusicSystemEventListener* const pListener = nullptr) = 0;
        virtual bool SetDefaultMood(const char* pszMood) = 0;
        virtual const char* GetMood() const = 0;
        virtual CCryNameCRC GetMoodNameCRC() const = 0;
        virtual IStringItVec* GetMoods(const char* pszTheme) const = 0;
        virtual bool AddMusicMoodEvent(const char* pszMood, float fTimeout) = 0;
        virtual CTimeValue GetMoodTime() const = 0;

        // Patterns
        virtual IStringItVec* const GetPatterns() const = 0;

        // general
        virtual SMusicSystemStatus* GetStatus() = 0;    // retrieve status of music-system... don't keep returning pointer !
        virtual void GetMemoryUsage(class ICrySizer* pSizer) const = 0;
        virtual void LogMsg(const int nVerbosity, const char* pszFormat, ...) PRINTF_PARAMS(3, 4) = 0;
        virtual bool IsNullImplementation() const = 0;
        virtual bool IsPlaying() const = 0;
        virtual bool IsPatternFading() const = 0;
        virtual void SetAutoEndThemeBehavior(EAutoEndThemeBehavior behavior) = 0;


        //////////////////////////////////////////////////////////////////////////
        //! Load music data from XML.
        //! @param bReloadData if true data from XML will be added to currently loaded music data.
        virtual bool LoadFromXML(const char* const sFilename, const bool bReloadData) = 0;

        // Summary:
        // Parse the appropriate music xml file depending on the game type.
        virtual bool LoadGameType(const char* const sGameTypeIdentifier) = 0;

        //////////////////////////////////////////////////////////////////////////
        // Editing support.
        //////////////////////////////////////////////////////////////////////////
        virtual void UpdatePattern(SMusicInfo::Pattern* pPattern) = 0;
        virtual void RenamePattern(const char* sOldName, const char* sNewName) = 0;
        virtual void PlayPattern(const char* const sPattern, const bool bStopPrevious, const bool bPlaySynched, const bool bPlayOnTop, const IMusicSystemEventListener* const pListener = nullptr) = 0;
        virtual bool StopPattern(const char* const sPatternNameToStop, const IMusicSystemEventListener* const pListener = nullptr) = 0;
        virtual void DeletePattern(const char* sPattern) = 0;
        virtual const char* const GetPatternNameTrack1() const = 0;
        virtual void Silence() = 0;
        virtual void ClearResources() = 0;

        virtual void PlayStinger() = 0;

        // For serialization where the called passed its own TSerialize object for serialization.
        // bPauseMusicSystem if true will pause the music system when reading.
        virtual void Serialize(TSerialize ser, const bool bPauseMusicSystem = true) = 0;

        // For serialization where the called doesn't provide a TSerialize object for serialization.
        virtual void Serialize(const bool bSave) = 0;

        //main update timing
        virtual float GetUpdateMilliseconds() = 0;

        // Summary:
        //   Registers listener to the music system.
        virtual void AddEventListener(IMusicSystemEventListener* const pListener) = 0;

        // Summary:
        //   Removes listener to the music system.
        virtual void RemoveEventListener(IMusicSystemEventListener* const pListener) = 0;

        // Summary:
        // To get music logic related data.
        virtual IMusicLogic* const GetMusicLogic() const = 0;

        // Summary:
        // This music pattern instance will be preloaded so it plays immediately when called.
        virtual bool PreloadPatternInstance(const char* const sPatternName) = 0;

        // Summary:
        // To remove all preloaded music patterns.
        virtual void ClearPreloadedPatterns() = 0;

#ifndef _RELEASE
        // Production only stuff
        virtual void DrawInformation(IRenderer* const pRenderer, float xpos, float ypos) = 0;
#endif // _RELEASE
    };

    //////////////////////////////////////////////////////////////////////////
    // Sink to release data (if allocated in a different DLL)
    //////////////////////////////////////////////////////////////////////////
    struct IMusicSystemSink
    {
        virtual ~IMusicSystemSink() {}
        virtual void ReleaseData(struct SMusicData* pData) = 0;
    };
} // namespace Audio

#define UPDATE_MUSICSYSTEM_IN_MS 250
