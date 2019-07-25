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

#include "CryLegacy_precompiled.h"
#include "PlayerProfileManager.h"
#include "PlayerProfile.h"
#include "CryAction.h"
#include "IActionMapManager.h"
#include "IPlatformOS.h"
#include "CryCrc32.h"
#include <PlayerProfileNotificationBus.h>

//AzCore
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/ObjectStream.h>

#define SHARED_SAVEGAME_FOLDER "@user@/SaveGames"

#define ONLINE_ATTRIBUTES_DEFINITION_FILE "Scripts/Network/OnlineAttributes.xml"
#define ONLINE_VERSION_ATTRIBUTE_NAME "OnlineAttributes/version"

const char* CPlayerProfileManager::FACTORY_DEFAULT_NAME = "default";
const char* CPlayerProfileManager::PLAYER_DEFAULT_PROFILE_NAME = "default";

namespace TESTING
{
    void DumpProfiles(IPlayerProfileManager* pFS, const char* userId)
    {
        int nProfiles = pFS->GetProfileCount(userId);
        IPlayerProfileManager::SProfileDescription desc;
        CryLogAlways("User %s has %d profiles", userId, nProfiles);
        for (int i = 0; i < nProfiles; ++i)
        {
            pFS->GetProfileInfo(userId, i, desc);
            CryLogAlways("#%d: '%s'", i, desc.name);
        }
    }

    void DumpAttrs(IPlayerProfile* pProfile)
    {
        if (pProfile == 0)
        {
            return;
        }
        IAttributeEnumeratorPtr pEnum = pProfile->CreateAttributeEnumerator();
        IAttributeEnumerator::SAttributeDescription desc;
        CryLogAlways("Attributes of profile %s", pProfile->GetName());
        int i = 0;
        TFlowInputData val;
        while (pEnum->Next(desc))
        {
            pProfile->GetAttribute(desc.name, val);
            string sVal;
            val.GetValueWithConversion(sVal);
            CryLogAlways("Attr %d: %s=%s", i, desc.name, sVal.c_str());
            ++i;
        }
    }

    void DumpSaveGames(IPlayerProfile* pProfile)
    {
        ISaveGameEnumeratorPtr pSGE = pProfile->CreateSaveGameEnumerator();
        ISaveGameEnumerator::SGameDescription desc;
        CryLogAlways("SaveGames for Profile '%s'", pProfile->GetName());
        char timeBuf[256];
#ifdef AZ_COMPILER_MSVC
        tm time;
        for (int i = 0; i < pSGE->GetCount(); ++i)
        {
            pSGE->GetDescription(i, desc);
            localtime_s(&time, &desc.metaData.saveTime);
            if (strftime(timeBuf, sizeof(timeBuf), "%#c", &time) == 0)
            {
                asctime_s(timeBuf, AZ_ARRAY_SIZE(timeBuf), &time);
            }
            CryLogAlways("SaveGame %d/%d: name='%s' humanName='%s' desc='%s'", i, pSGE->GetCount() - 1, desc.name, desc.humanName, desc.description);
            CryLogAlways("MetaData: level=%s gr=%s version=%d build=%s savetime=%s",
                desc.metaData.levelName, desc.metaData.gameRules, desc.metaData.fileVersion, desc.metaData.buildVersion,
                timeBuf);
        }
#else
        struct tm* timePtr;
        for (int i = 0; i < pSGE->GetCount(); ++i)
        {
            pSGE->GetDescription(i, desc);
            timePtr = localtime(&desc.metaData.saveTime);
            const char* timeString = timeBuf;
            if (strftime(timeBuf, sizeof(timeBuf), "%#c", timePtr) == 0)
            {
                timeString = asctime(timePtr);
            }
            CryLogAlways("SaveGame %d/%d: name='%s' humanName='%s' desc='%s'", i, pSGE->GetCount() - 1, desc.name, desc.humanName, desc.description);
            CryLogAlways("MetaData: level=%s gr=%s version=%d build=%s savetime=%s",
                desc.metaData.levelName, desc.metaData.gameRules, desc.metaData.fileVersion, desc.metaData.buildVersion,
                timeString);
        }
#endif
    }

    void DumpActionMap(IPlayerProfile* pProfile, const char* name)
    {
        IActionMap* pMap = pProfile->GetActionMap(name);
        if (pMap)
        {
            int iAction = 0;
            IActionMapActionIteratorPtr pIter = pMap->CreateActionIterator();
            while (const IActionMapAction* pAction = pIter->Next())
            {
                CryLogAlways("Action %d: '%s'", iAction++, pAction->GetActionId().c_str());

                int iNumInputData = pAction->GetNumActionInputs();
                for (int i = 0; i < iNumInputData; ++i)
                {
                    const SActionInput* pActionInput = pAction->GetActionInput(i);
                    CRY_ASSERT(pActionInput != NULL);
                    CryLogAlways("Key %d/%d: '%s'", i, iNumInputData - 1, pActionInput->input.c_str());
                }
            }
        }
    }

    void DumpMap(IConsoleCmdArgs* args)
    {
        IActionMapManager* pAM = CCryAction::GetCryAction()->GetIActionMapManager();
        IActionMapIteratorPtr iter = pAM->CreateActionMapIterator();
        while (IActionMap* pMap = iter->Next())
        {
            CryLogAlways("ActionMap: '%s' 0x%p", pMap->GetName(), pMap);
            int iAction = 0;
            IActionMapActionIteratorPtr pIter = pMap->CreateActionIterator();
            while (const IActionMapAction* pAction = pIter->Next())
            {
                CryLogAlways("Action %d: '%s'", iAction++, pAction->GetActionId().c_str());

                int iNumInputData = pAction->GetNumActionInputs();
                for (int i = 0; i < iNumInputData; ++i)
                {
                    const SActionInput* pActionInput = pAction->GetActionInput(i);
                    CRY_ASSERT(pActionInput != NULL);
                    CryLogAlways("Key %d/%d: '%s'", i, iNumInputData - 1, pActionInput->input.c_str());
                }
            }
        }
    }

    void TestProfile(IConsoleCmdArgs* args)
    {
        const char* userName = GetISystem()->GetUserName();
        IPlayerProfileManager::EProfileOperationResult result;
        IPlayerProfileManager* pFS = CCryAction::GetCryAction()->GetIPlayerProfileManager();

        // test renaming current profile
#if 0
        pFS->RenameProfile(userName, "newOne4", result);
        return;
#endif

        bool bFirstTime = false;
        pFS->LoginUser(userName, bFirstTime);
        DumpProfiles(pFS, userName);
        pFS->DeleteProfile(userName, "PlayerCool", result);
        IPlayerProfile* pProfile = pFS->ActivateProfile(userName, "default");
        if (pProfile)
        {
            DumpActionMap(pProfile, "default");
            DumpAttrs(pProfile);
            pProfile->SetAttribute("hallo2", TFlowInputData(222));
            pProfile->SetAttribute("newAddedAttribute", TFlowInputData(24.10f));
            DumpAttrs(pProfile);
            pProfile->SetAttribute("newAddedAttribute", TFlowInputData(25.10f));
            DumpAttrs(pProfile);
            pProfile->ResetAttribute("newAddedAttribute");
            pProfile->ResetAttribute("hallo2");
            DumpAttrs(pProfile);
            DumpSaveGames(pProfile);
            float fVal;
            pProfile->GetAttribute("newAddedAttribute", fVal);
            pProfile->SetAttribute("newAddedAttribute", 2.22f);
        }
        else
        {
            CryLogAlways("Can't activate profile 'default'");
        }

        const IPlayerProfile* pPreviewProfile = pFS->PreviewProfile(userName, "previewTest");
        if (pPreviewProfile)
        {
            float fVal;
            pPreviewProfile->GetAttribute("previewData", fVal);
            pPreviewProfile->GetAttribute("previewData2", fVal, true);
        }

        DumpProfiles(pFS, userName);
        // pFS->RenameProfile(userName, "new new profile");
        pFS->LogoutUser(userName);
    }
}

int CPlayerProfileManager::sUseRichSaveGames = 0;
int CPlayerProfileManager::sRSFDebugWrite = 0;
int CPlayerProfileManager::sRSFDebugWriteOnLoad = 0;
int CPlayerProfileManager::sLoadOnlineAttributes = 1;

//------------------------------------------------------------------------
CPlayerProfileManager::CPlayerProfileManager(CPlayerProfileManager::IPlatformImpl* pImpl)
    : m_bInitialized(false)
    , m_pDefaultProfile(0)
    , m_pImpl(pImpl)
    , m_loadingProfile(false)
    , m_savingProfile(false)
{
    assert (m_pImpl != 0);

    // FIXME: TODO: temp stuff
    static bool testInit = false;
    if (testInit == false)
    {
        testInit = true;
        REGISTER_CVAR2("pp_RichSaveGames", &CPlayerProfileManager::sUseRichSaveGames, 0, 0, "Enable RichSaveGame Format for SaveGames");
        REGISTER_CVAR2("pp_RSFDebugWrite", &CPlayerProfileManager::sRSFDebugWrite, gEnv->pSystem->IsDevMode() ? 1 : 0, 0, "When RichSaveGames are enabled, save plain XML Data alongside for debugging");
        REGISTER_CVAR2("pp_RSFDebugWriteOnLoad", &CPlayerProfileManager::sRSFDebugWriteOnLoad, 0, 0, "When RichSaveGames are enabled, save plain XML Data alongside for debugging when loading a savegame");
        REGISTER_CVAR2("pp_LoadOnlineAttributes", &CPlayerProfileManager::sLoadOnlineAttributes, CPlayerProfileManager::sLoadOnlineAttributes, VF_REQUIRE_APP_RESTART, "Load online attributes");

        REGISTER_COMMAND("test_profile", TESTING::TestProfile, VF_NULL, "");
        REGISTER_COMMAND("dump_action_maps", TESTING::DumpMap, VF_NULL, "Prints all action map bindings to console");

#if PROFILE_DEBUG_COMMANDS
        REGISTER_COMMAND("loadOnlineAttributes", &CPlayerProfileManager::DbgLoadOnlineAttributes, VF_NULL, "Loads online attributes");
        REGISTER_COMMAND("saveOnlineAttributes", &CPlayerProfileManager::DbgSaveOnlineAttributes, VF_NULL, "Saves online attributes");
        REGISTER_COMMAND("testOnlineAttributes", &CPlayerProfileManager::DbgTestOnlineAttributes, VF_NULL, "Tests online attributes");

        REGISTER_COMMAND("loadProfile", &CPlayerProfileManager::DbgLoadProfile, VF_NULL, "Load current user profile");
        REGISTER_COMMAND("saveProfile", &CPlayerProfileManager::DbgSaveProfile, VF_NULL, "Save current user profile");
#endif
    }

    m_sharedSaveGameFolder = SHARED_SAVEGAME_FOLDER; // by default, use a shared savegame folder (Games For Windows Requirement)
    m_sharedSaveGameFolder.TrimRight("/\\");

    m_curUserIndex = 0;
    memset(m_onlineOnlyData, 0, sizeof(m_onlineOnlyData));
    m_onlineDataCount = 0;
    m_onlineDataByteCount = 0;
    m_onlineAttributeAutoGeneratedVersion = 0;
    m_registered = false;
    m_pReadingProfile = NULL;
    m_onlineAttributesListener = NULL;
    m_enableOnlineAttributes = false;
    m_allowedToProcessOnlineAttributes = true;
    m_exclusiveControllerDeviceIndex = INVALID_CONTROLLER_INDEX; // start uninitialized
    memset(m_onlineAttributesState, IOnlineAttributesListener::eOAS_None, sizeof(m_onlineAttributesState));

#if PROFILE_DEBUG_COMMANDS
    m_testingPhase = 0;
#endif
}

//------------------------------------------------------------------------
CPlayerProfileManager::~CPlayerProfileManager()
{
    // don't call virtual Shutdown or any other virtual function,
    // but delete things directly
    if (m_bInitialized)
    {
        GameWarning("[PlayerProfiles] CPlayerProfileManager::~CPlayerProfileManager Shutdown not called!");
    }
    std::for_each(m_userVec.begin(), m_userVec.end(), stl::container_object_deleter());
    SAFE_DELETE(m_pDefaultProfile);
    if (m_pImpl)
    {
        m_pImpl->Release();
        m_pImpl = 0;
    }

    IConsole* pC = ::gEnv->pConsole;
    pC->UnregisterVariable("pp_RichSaveGames", true);
    pC->RemoveCommand("test_profile");
    pC->RemoveCommand("dump_action_maps");
}

//------------------------------------------------------------------------
bool CPlayerProfileManager::Initialize()
{
    if (m_bInitialized)
    {
        return true;
    }

    m_pImpl->Initialize(this);

    CPlayerProfile* pDefaultProfile = new CPlayerProfile(this, FACTORY_DEFAULT_NAME, "", false);
    bool ok = LoadProfile(0, pDefaultProfile, pDefaultProfile->GetName());
    if (!ok)
    {
        GameWarning("[PlayerProfiles] Cannot load factory default profile '%s'", FACTORY_DEFAULT_NAME);
        SAFE_DELETE(pDefaultProfile);
    }

    m_pDefaultProfile = pDefaultProfile;

    m_bInitialized = ok;
    AZ::PlayerProfileRequestBus::Handler::BusConnect();
    return m_bInitialized;
}

//------------------------------------------------------------------------
bool CPlayerProfileManager::Shutdown()
{
    while (m_userVec.empty () == false)
    {
        SUserEntry* pEntry = m_userVec.back();
        LogoutUser(pEntry->userId);
    }
    SAFE_DELETE(m_pDefaultProfile);
    m_bInitialized = false;
    AZ::PlayerProfileRequestBus::Handler::BusDisconnect();
    return true;
}

//------------------------------------------------------------------------
int CPlayerProfileManager::GetUserCount()
{
    return m_userVec.size();
}

//------------------------------------------------------------------------
bool CPlayerProfileManager::GetUserInfo(int index, IPlayerProfileManager::SUserInfo& outInfo)
{
    if (index < 0 || index >= m_userVec.size())
    {
        assert (index >= 0 && index < m_userVec.size());
        return false;
    }

    SUserEntry* pEntry = m_userVec[index];
    outInfo.userId = pEntry->userId;
    return true;
}

//------------------------------------------------------------------------
bool CPlayerProfileManager::LoginUser(const char* userId, bool& bOutFirstTime)
{
    if (m_bInitialized == false)
    {
        return false;
    }

    bOutFirstTime = false;

    m_curUserID = userId;

    // user already logged in
    SUserEntry* pEntry = FindEntry(userId);
    if (pEntry)
    {
        m_curUserIndex = pEntry->userIndex;
        return true;
    }

    SUserEntry* newEntry = new SUserEntry(userId);
    newEntry->pCurrentProfile = 0;
    newEntry->userIndex = m_userVec.size();
    // login the user and fill SUserEntry
    bool ok = m_pImpl->LoginUser(newEntry);

    if (!ok)
    {
        delete newEntry;
        GameWarning("[PlayerProfiles] Cannot login user '%s'", userId);
        return false;
    }

    // no entries yet -> create default profile by copying/saving factory default
    if (m_pDefaultProfile && newEntry->profileDesc.empty())
    {
        // save the default profile
        ok = m_pImpl->SaveProfile(newEntry, m_pDefaultProfile, PLAYER_DEFAULT_PROFILE_NAME);
        if (!ok)
        {
            GameWarning("[PlayerProfiles] Login of user '%s' failed because default profile '%s' cannot be created", userId, PLAYER_DEFAULT_PROFILE_NAME);

            delete newEntry;
            // TODO: maybe assign use factory default in this case, but then
            // cannot save anything incl. save-games
            return false;
        }
        CryLog("[PlayerProfiles] Login of user '%s': First Time Login - Successfully created default profile '%s'", userId, PLAYER_DEFAULT_PROFILE_NAME);
        SLocalProfileInfo info(PLAYER_DEFAULT_PROFILE_NAME);
        time_t lastLoginTime;
        time(&lastLoginTime);
        info.SetLastLoginTime(lastLoginTime);
        newEntry->profileDesc.push_back(info);
        bOutFirstTime = true;
    }

    CryLog("[PlayerProfiles] Login of user '%s' successful.", userId);
    if (bOutFirstTime == false)
    {
        CryLog("[PlayerProfiles] Found %" PRISIZE_T " profiles.", newEntry->profileDesc.size());
        for (int i = 0; i < newEntry->profileDesc.size(); ++i)
        {
            CryLog("   Profile %d : '%s'", i, newEntry->profileDesc[i].GetName().c_str());
        }
    }

    m_curUserIndex = newEntry->userIndex;
    m_userVec.push_back(newEntry);
    return true;
}

//------------------------------------------------------------------------
bool CPlayerProfileManager::LogoutUser(const char* userId)
{
    if (!m_bInitialized)
    {
        return false;
    }

    // check if user already logged in
    TUserVec::iterator iter;
    SUserEntry* pEntry = FindEntry(userId, iter);
    if (pEntry == 0)
    {
        GameWarning("[PlayerProfiles] Logout of user '%s' failed [was not logged in]", userId);
        return false;
    }

#if defined(AZ_RESTRICTED_PLATFORM)
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/PlayerProfileManager_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/PlayerProfileManager_cpp_provo.inl"
    #endif
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    // auto-save profile
    if (pEntry->pCurrentProfile && (gEnv && gEnv->pSystem && (!gEnv->pSystem->IsQuitting())))
    {
        EProfileOperationResult result;
        bool ok = SaveProfile(userId, result, ePR_Options);
        if (!ok)
        {
            GameWarning("[PlayerProfiles] Logout of user '%s': Couldn't save profile '%s'", userId, pEntry->pCurrentProfile->GetName());
        }
    }
#endif

    m_pImpl->LogoutUser(pEntry);

    if (m_curUserIndex == std::distance(m_userVec.begin(), iter))
    {
        m_curUserIndex = 0;
        m_curUserID.clear();
    }

    // delete entry from vector
    m_userVec.erase(iter);

    ClearOnlineAttributes();

    SAFE_DELETE(pEntry->pCurrentProfile);
    SAFE_DELETE(pEntry->pCurrentPreviewProfile);

    // delete entry
    delete pEntry;

    return true;
}

//------------------------------------------------------------------------
bool CPlayerProfileManager::IsUserSignedIn(const char* userId, unsigned int& outUserIndex)
{
    return m_pImpl->IsUserSignedIn(userId, outUserIndex);
}

//------------------------------------------------------------------------
int CPlayerProfileManager::GetProfileCount(const char* userId)
{
    SUserEntry* pEntry = FindEntry(userId);
    if (pEntry == 0)
    {
        return 0;
    }
    return pEntry->profileDesc.size();
}

//------------------------------------------------------------------------
bool CPlayerProfileManager::GetProfileInfo(const char* userId, int index, IPlayerProfileManager::SProfileDescription& outInfo)
{
    if (!m_bInitialized)
    {
        return false;
    }

    SUserEntry* pEntry = FindEntry(userId);
    if (pEntry == 0)
    {
        GameWarning("[PlayerProfiles] GetProfileInfo: User '%s' not logged in", userId);
        return false;
    }
    int count = pEntry->profileDesc.size();
    if (index >= count)
    {
        assert (index < count);
        return false;
    }
    SLocalProfileInfo& info = pEntry->profileDesc[index];
    outInfo.name = info.GetName().c_str();
    outInfo.lastLoginTime = info.GetLastLoginTime();
    return true;
}

//------------------------------------------------------------------------
void CPlayerProfileManager::SetProfileLastLoginTime(const char* userId, int index, time_t lastLoginTime)
{
    if (!m_bInitialized)
    {
        return;
    }

    SUserEntry* pEntry = FindEntry(userId);
    if (pEntry == 0)
    {
        GameWarning("[PlayerProfiles] GetProfileInfo: User '%s' not logged in", userId);
        return;
    }
    int count = pEntry->profileDesc.size();
    if (index >= count)
    {
        assert (index < count);
        return;
    }
    SLocalProfileInfo& info = pEntry->profileDesc[index];
    info.SetLastLoginTime(lastLoginTime);
}

//------------------------------------------------------------------------
bool CPlayerProfileManager::CreateProfile(const char* userId, const char* profileName, bool bOverride, IPlayerProfileManager::EProfileOperationResult& result)
{
    if (!m_bInitialized)
    {
        result = ePOR_NotInitialized;
        return false;
    }
    SUserEntry* pEntry = FindEntry(userId);
    if (pEntry == 0)
    {
        GameWarning("[PlayerProfiles] CreateProfile: User '%s' not logged in", userId);
        result = ePOR_UserNotLoggedIn;
        return false;
    }

    SLocalProfileInfo* pInfo = GetLocalProfileInfo(pEntry, profileName);
    if (pInfo != 0 && !bOverride) // profile with this name already exists
    {
        GameWarning("[PlayerProfiles] CreateProfile: User '%s' already has a profile with name '%s'", userId, profileName);
        result = ePOR_NameInUse;
        return false;
    }

    result = ePOR_Unknown;
    bool ok = true;
    if (pEntry->pCurrentProfile == 0)
    {
        // save the default profile
        ok = m_pImpl->SaveProfile(pEntry, m_pDefaultProfile, profileName, bOverride);
    }
    if (ok)
    {
        result = ePOR_Success;

        if (pInfo == 0) // if we override, it's already present
        {
            SLocalProfileInfo info (profileName);
            time_t lastPlayTime;
            time(&lastPlayTime);
            info.SetLastLoginTime(lastPlayTime);
            pEntry->profileDesc.push_back(info);

            // create default profile by copying/saving factory default
            ok = m_pImpl->SaveProfile(pEntry, m_pDefaultProfile, profileName);

            if (!ok)
            {
                pEntry->profileDesc.pop_back();
                result = ePOR_Unknown;
            }
        }
    }
    if (!ok)
    {
        GameWarning("[PlayerProfiles] CreateProfile: User '%s' cannot create profile '%s'", userId, profileName);
    }
    return ok;
}

//------------------------------------------------------------------------
bool CPlayerProfileManager::DeleteProfile(const char* userId, const char* profileName, IPlayerProfileManager::EProfileOperationResult& result)
{
    if (!m_bInitialized)
    {
        result = ePOR_NotInitialized;
        return false;
    }
    SUserEntry* pEntry = FindEntry(userId);
    if (pEntry == 0)
    {
        GameWarning("[PlayerProfiles] DeleteProfile: User '%s' not logged in", userId);
        result = ePOR_UserNotLoggedIn;
        return false;
    }

    // cannot delete last profile
    if (pEntry->profileDesc.size() <= 1)
    {
        GameWarning("[PlayerProfiles] DeleteProfile: User '%s' cannot delete last profile", userId);
        result = ePOR_ProfileInUse;
        return false;
    }

    // make sure there is such a profile
    std::vector<SLocalProfileInfo>::iterator iter;
    SLocalProfileInfo* info = GetLocalProfileInfo(pEntry, profileName, iter);
    if (info)
    {
        bool ok = m_pImpl->DeleteProfile(pEntry, profileName);
        if (ok)
        {
            pEntry->profileDesc.erase(iter);
            // if the profile was the current profile, delete it
            if (pEntry->pCurrentProfile != 0 && azstricmp(profileName, pEntry->pCurrentProfile->GetName()) == 0)
            {
                delete pEntry->pCurrentProfile;
                pEntry->pCurrentProfile = 0;
                // TODO: Maybe auto-select a profile
            }
            result = ePOR_Success;
            return true;
        }
        result = ePOR_Unknown;
    }
    else
    {
        result = ePOR_NoSuchProfile;
    }

    return false;
}

//------------------------------------------------------------------------
bool CPlayerProfileManager::RenameProfile(const char* userId, const char* newName, IPlayerProfileManager::EProfileOperationResult& result)
{
    if (!m_bInitialized)
    {
        result = ePOR_NotInitialized;
        return false;
    }

    SUserEntry* pEntry = FindEntry(userId);
    if (pEntry == 0)
    {
        GameWarning("[PlayerProfiles] RenameProfile: User '%s' not logged in", userId);
        result =  ePOR_UserNotLoggedIn;
        return false;
    }

    // make sure there is no such profile
    if (GetLocalProfileInfo(pEntry, newName) != 0)
    {
        result = ePOR_NoSuchProfile;
        return false;
    }

    // can only rename current active profile
    if (pEntry->pCurrentProfile == 0)
    {
        result = ePOR_NoActiveProfile;
        return false;
    }

    if (azstricmp(pEntry->pCurrentProfile->GetName(), PLAYER_DEFAULT_PROFILE_NAME) == 0)
    {
        GameWarning("[PlayerProfiles] RenameProfile: User '%s' cannot rename default profile", userId);
        result = ePOR_DefaultProfile;
        return false;
    }

    result = ePOR_Unknown;
    bool ok = m_pImpl->RenameProfile(pEntry, newName);

    if (ok)
    {
        // assign a new name in the info DB
        SLocalProfileInfo* info = GetLocalProfileInfo(pEntry, pEntry->pCurrentProfile->GetName());
        info->SetName(newName);

        // assign a new name in the profile itself
        pEntry->pCurrentProfile->SetName(newName);

        result = ePOR_Success;
    }
    else
    {
        GameWarning("[PlayerProfiles] RenameProfile: Failed to rename profile to '%s' for user '%s' ", newName, userId);
    }
    return ok;
}

//------------------------------------------------------------------------
bool CPlayerProfileManager::SaveProfile(const char* userId, IPlayerProfileManager::EProfileOperationResult& result, unsigned int reason)
{
    EBUS_EVENT(AZ::PlayerProfileNotificationBus, OnProfileSaving);
    if (!m_bInitialized)
    {
        result = ePOR_NotInitialized;
        EBUS_EVENT(AZ::PlayerProfileNotificationBus, OnProfileSaveFailure, AZ::ProfileSaveLoadError::SystemNotInitialized);
        return false;
    }

    SUserEntry* pEntry = FindEntry(userId);
    if (pEntry == 0)
    {
        GameWarning("[PlayerProfiles] SaveProfile: User '%s' not logged in", userId);
        result = ePOR_UserNotLoggedIn;
        EBUS_EVENT(AZ::PlayerProfileNotificationBus, OnProfileSaveFailure, AZ::ProfileSaveLoadError::UserNotLoggedIn);
        return false;
    }

    if (pEntry->pCurrentProfile == 0)
    {
        GameWarning("[PlayerProfiles] SaveProfile: Profile for '%s' not active", userId);
        result = ePOR_NoActiveProfile;
        EBUS_EVENT(AZ::PlayerProfileNotificationBus, OnProfileSaveFailure, AZ::ProfileSaveLoadError::ProfileNotActive);
        return false;
    }

    if (m_loadingProfile || m_savingProfile)
    {
        result = ePOR_LoadingProfile;
        EBUS_EVENT(AZ::PlayerProfileNotificationBus, OnProfileSaveFailure, AZ::ProfileSaveLoadError::AnotherOperationInProgress);
        return false;
    }
    else
    {
        m_savingProfile = true;
    }

    result = ePOR_Success;

    // notify game systems that the profile is about to be saved
    const int listenerSize = m_listeners.size();
    for (int i = 0; i < listenerSize; i++)
    {
        m_listeners[i]->SaveToProfile(pEntry->pCurrentProfile, false, reason);
    }

    bool ok = m_pImpl->SaveProfile(pEntry, pEntry->pCurrentProfile, pEntry->pCurrentProfile->GetName());

    if (!ok)
    {
        GameWarning("[PlayerProfiles] SaveProfile: User '%s' cannot save profile '%s'", userId, pEntry->pCurrentProfile->GetName());
        result = ePOR_Unknown;
        EBUS_EVENT(AZ::PlayerProfileNotificationBus, OnProfileSaveFailure, AZ::ProfileSaveLoadError::Unknown);
    }

    m_savingProfile = false;
    if (ok)
    {
        EBUS_EVENT(AZ::PlayerProfileNotificationBus, OnProfileSaved);
    }
    return ok;
}

//------------------------------------------------------------------------
bool CPlayerProfileManager::SaveInactiveProfile(const char* userId, const char* profileName, EProfileOperationResult& result, unsigned int reason)
{
    EBUS_EVENT(AZ::PlayerProfileNotificationBus, OnProfileSaving);
    if (!m_bInitialized)
    {
        result = ePOR_NotInitialized;
        return false;
    }

    SUserEntry* pEntry = FindEntry(userId);
    if (pEntry == 0)
    {
        GameWarning("[PlayerProfiles] SaveProfile: User '%s' not logged in", userId);
        result = ePOR_UserNotLoggedIn;
        return false;
        EBUS_EVENT(AZ::PlayerProfileNotificationBus, OnProfileSaveFailure, AZ::ProfileSaveLoadError::UserNotLoggedIn);
    }

    CPlayerProfile* pProfile = profileName ? new CPlayerProfile(this, profileName, userId, false) : 0;

    if (!pProfile || !LoadProfile(pEntry, pProfile, profileName, true))
    {
        result = ePOR_NoSuchProfile;
        EBUS_EVENT(AZ::PlayerProfileNotificationBus, OnProfileSaveFailure, AZ::ProfileSaveLoadError::ProfileNotActive);
        return false;
    }

    result = ePOR_Success;
    
    // notify game systems that the profile is about to be saved
    const int listenerSize = m_listeners.size();
    for (int i = 0; i < listenerSize; i++)
    {
        m_listeners[i]->SaveToProfile(pProfile, false, reason);
    }

    bool ok = m_pImpl->SaveProfile(pEntry, pProfile, pProfile->GetName());

    if (!ok)
    {
        GameWarning("[PlayerProfiles] SaveProfile: User '%s' cannot save profile '%s'", userId, pProfile->GetName());
        result = ePOR_Unknown;
    }

    delete pProfile;

    EBUS_EVENT(AZ::PlayerProfileNotificationBus, OnProfileSaved);
    return ok;
}

//------------------------------------------------------------------------
bool CPlayerProfileManager::IsLoadingProfile() const
{
    return m_loadingProfile;
}

//------------------------------------------------------------------------
bool CPlayerProfileManager::IsSavingProfile() const
{
    return m_savingProfile;
}

//------------------------------------------------------------------------
bool CPlayerProfileManager::LoadProfile(SUserEntry* pEntry, CPlayerProfile* pProfile, const char* name, bool bPreview /*false*/)
{
    EBUS_EVENT(AZ::PlayerProfileNotificationBus, OnProfileLoading);
    if (m_loadingProfile || m_savingProfile)
    {
        EBUS_EVENT(AZ::PlayerProfileNotificationBus, OnProfileLoadFailure, AZ::ProfileSaveLoadError::AnotherOperationInProgress);
        return false;
    }
    else
    {
        m_loadingProfile = true;
    }

    bool ok = m_pImpl->LoadProfile(pEntry, pProfile, name);

    if (ok && !bPreview)
    {
        // notify game systems about the new profile data
        const int listenerSize = m_listeners.size();
        for (int i = 0; i < listenerSize; i++)
        {
            m_listeners[i]->LoadFromProfile(pProfile, false, ePR_All);
        }
    }
    else
    {
        EBUS_EVENT(AZ::PlayerProfileNotificationBus, OnProfileLoadFailure, AZ::ProfileSaveLoadError::Unknown);
    }

    m_loadingProfile = false;

    EBUS_EVENT(AZ::PlayerProfileNotificationBus, OnProfileLoaded);
    return ok;
}

//------------------------------------------------------------------------
const IPlayerProfile* CPlayerProfileManager::PreviewProfile(const char* userId, const char* profileName)
{
    SUserEntry* pEntry = FindEntry(userId);
    if (pEntry == 0) // no such user
    {
        GameWarning("[PlayerProfiles] ActivateProfile: User '%s' not logged in", userId);
        return 0;
    }

    // if this is the current profile, do nothing
    if (pEntry->pCurrentPreviewProfile != 0 && profileName && azstricmp(profileName, pEntry->pCurrentPreviewProfile->GetName()) == 0)
    {
        return pEntry->pCurrentPreviewProfile;
    }

    if (pEntry->pCurrentPreviewProfile != 0)
    {
        delete pEntry->pCurrentPreviewProfile;
        pEntry->pCurrentPreviewProfile = 0;
    }

    if (profileName == 0 || *profileName == '\0')
    {
        return 0;
    }

    SLocalProfileInfo* info = GetLocalProfileInfo(pEntry, profileName);
    if (info == 0) // no such profile
    {
        GameWarning("[PlayerProfiles] PreviewProfile: User '%s' has no profile '%s'", userId, profileName);
        return 0;
    }

    pEntry->pCurrentPreviewProfile = new CPlayerProfile(this, profileName, userId, true);
    const bool ok = LoadProfile(pEntry, pEntry->pCurrentPreviewProfile, profileName, true);
    if (!ok)
    {
        GameWarning("[PlayerProfiles] PreviewProfile: User '%s' cannot load profile '%s'", userId, profileName);
        delete pEntry->pCurrentPreviewProfile;
        pEntry->pCurrentPreviewProfile = 0;
    }

    return pEntry->pCurrentPreviewProfile;
}

//------------------------------------------------------------------------
IPlayerProfile* CPlayerProfileManager::ActivateProfile(const char* userId, const char* profileName)
{
    SUserEntry* pEntry = FindEntry(userId);
    if (pEntry == 0) // no such user
    {
        GameWarning("[PlayerProfiles] ActivateProfile: User '%s' not logged in", userId);
        return 0;
    }

    // if this is the current profile, do nothing
    if (pEntry->pCurrentProfile != 0 && azstricmp(profileName, pEntry->pCurrentProfile->GetName()) == 0)
    {
        return pEntry->pCurrentProfile;
    }

    SLocalProfileInfo* info = GetLocalProfileInfo(pEntry, profileName);
    time_t lastPlayTime;
    time(&lastPlayTime);
    if (info == 0) // no such profile
    {
        GameWarning("[PlayerProfiles] ActivateProfile: User '%s' has no profile '%s'", userId, profileName);
        return 0;
    }
    info->SetLastLoginTime(lastPlayTime);
    CPlayerProfile* pNewProfile = new CPlayerProfile(this, profileName, userId, false);

    gEnv->pGame->SetUserProfileChanged(true);

    const bool ok = LoadProfile(pEntry, pNewProfile, profileName);
    if (ok)
    {
        delete pEntry->pCurrentProfile;
        pEntry->pCurrentProfile = pNewProfile;
    }
    else
    {
        GameWarning("[PlayerProfiles] ActivateProfile: User '%s' cannot load profile '%s'", userId, profileName);
        delete pNewProfile;
    }
    return pEntry->pCurrentProfile;
}

//------------------------------------------------------------------------
IPlayerProfile* CPlayerProfileManager::GetCurrentProfile(const char* userId)
{
    SUserEntry* pEntry = FindEntry(userId);
    return pEntry ? pEntry->pCurrentProfile : 0;
}

//------------------------------------------------------------------------
const char* CPlayerProfileManager::GetCurrentUser()
{
    return m_curUserID.c_str();
}

//------------------------------------------------------------------------
int CPlayerProfileManager::GetCurrentUserIndex()
{
    return m_curUserIndex;
}

void CPlayerProfileManager::SetExclusiveControllerDeviceIndex(unsigned int exclusiveDeviceIndex)
{
    m_exclusiveControllerDeviceIndex = exclusiveDeviceIndex;
}

unsigned int CPlayerProfileManager::GetExclusiveControllerDeviceIndex() const
{
    return m_exclusiveControllerDeviceIndex;
}

//------------------------------------------------------------------------
bool CPlayerProfileManager::ResetProfile(const char* userId)
{
    SUserEntry* pEntry = FindEntry(userId);
    if (pEntry == 0) // no such user
    {
        GameWarning("[PlayerProfiles] ResetProfile: User '%s' not logged in", userId);
        return false;
    }

    if (pEntry->pCurrentProfile == 0)
    {
        return false;
    }

    pEntry->pCurrentProfile->Reset();
    return true;
}

//------------------------------------------------------------------------
IPlayerProfile* CPlayerProfileManager::GetDefaultProfile()
{
    return m_pDefaultProfile;
}

//------------------------------------------------------------------------
CPlayerProfileManager::SUserEntry*
CPlayerProfileManager::FindEntry(const char* userId) const
{
    TUserVec::const_iterator iter = m_userVec.begin();
    while (iter != m_userVec.end())
    {
        const SUserEntry* pEntry = *iter;
        if (pEntry->userId.compare(userId) == 0)
        {
            return (const_cast<SUserEntry*> (pEntry));
        }
        ++iter;
    }
    return 0;
}

//------------------------------------------------------------------------
CPlayerProfileManager::SUserEntry*
CPlayerProfileManager::FindEntry(const char* userId, CPlayerProfileManager::TUserVec::iterator& outIter)
{
    TUserVec::iterator iter = m_userVec.begin();
    while (iter != m_userVec.end())
    {
        SUserEntry* pEntry = *iter;
        if (pEntry->userId.compare(userId) == 0)
        {
            outIter = iter;
            return pEntry;
        }
        ++iter;
    }
    outIter = iter;
    return 0;
}

//------------------------------------------------------------------------
CPlayerProfileManager::SLocalProfileInfo*
CPlayerProfileManager::GetLocalProfileInfo(SUserEntry* pEntry, const char* profileName) const
{
    std::vector<SLocalProfileInfo>::iterator iter = pEntry->profileDesc.begin();
    while (iter != pEntry->profileDesc.end())
    {
        SLocalProfileInfo& info = *iter;
        if (info.compare(profileName) == 0)
        {
            return &(const_cast<SLocalProfileInfo&> (info));
        }
        ++iter;
    }
    return 0;
}

//------------------------------------------------------------------------
CPlayerProfileManager::SLocalProfileInfo*
CPlayerProfileManager::GetLocalProfileInfo(SUserEntry* pEntry, const char* profileName, std::vector<SLocalProfileInfo>::iterator& outIter) const
{
    std::vector<SLocalProfileInfo>::iterator iter = pEntry->profileDesc.begin();
    while (iter != pEntry->profileDesc.end())
    {
        SLocalProfileInfo& info = *iter;
        if (info.compare(profileName) == 0)
        {
            outIter = iter;
            return &(const_cast<SLocalProfileInfo&> (info));
        }
        ++iter;
    }
    outIter = iter;
    return 0;
}

const char* CPlayerProfileManager::GetCurrentProfileForCurrentUser()
{
    return GetCurrentProfileName(GetCurrentUser());
}

const char* CPlayerProfileManager::GetCurrentProfileName(const char* userName)
{

    if (auto* currenProfile = GetCurrentProfile(userName))
    {
        return currenProfile->GetName();
    }
    return CPlayerProfileManager::PLAYER_DEFAULT_PROFILE_NAME;
}

const char* CPlayerProfileManager::GetCurrentUserName()
{
    return GetCurrentUser();
}

bool CPlayerProfileManager::RequestProfileSave(AZ::ProfileSaveReason reason)
{
    EProfileOperationResult saveResult;
    return SaveProfile(GetCurrentUser(), saveResult, static_cast<unsigned int>(reason));
}

bool CPlayerProfileManager::StoreData(const char* dataKey, const void* classPtr, const AZ::Uuid& classId, AZ::SerializeContext* serializeContext)
{
    if (IPlayerProfile* pProfile = GetCurrentProfile(GetCurrentUser()))
    {
        if (!serializeContext)
        {
            EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
        }
        if (serializeContext)
        {
            AZStd::vector<char> buffer;
            AZ::IO::ByteContainerStream<AZStd::vector<char>> memoryRepresentation = AZ::IO::ByteContainerStream<AZStd::vector<char>>(&buffer);
            AZ::ObjectStream* objectStream = AZ::ObjectStream::Create(&memoryRepresentation, *serializeContext, AZ::DataStream::StreamType::ST_XML);
            objectStream->WriteClass(classPtr, classId, nullptr);
            objectStream->Finalize();
            buffer.push_back('\0');
            CryStringT<char> stringRepresentationOfData = CryStringT<char>(memoryRepresentation.GetData()->begin());
            pProfile->SetAttribute(dataKey, stringRepresentationOfData);
            return true;
        }
    }

    return false;
}

bool CPlayerProfileManager::RetrieveData(const char* dataKey, void** outClassPtr, AZ::SerializeContext* serializeContext)
{
    if (IPlayerProfile* pProfile = GetCurrentProfile(GetCurrentUser()))
    {
        CryStringT<char> dataAsString;
        if (pProfile->GetAttribute(dataKey, dataAsString))
        {
            if (!serializeContext)
            {
                EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
            }

            AZ::IO::MemoryStream memoryRepresentation(dataAsString.c_str(), dataAsString.length());
            AZ::ObjectStream::ClassReadyCB classReadyCallBack = [&outClassPtr](void* classPtr, const AZ::Uuid& classId, AZ::SerializeContext* context)
            {
                if (classPtr)
                {
                    *outClassPtr = classPtr;
                }
            };
            AZ::ObjectStream::LoadBlocking(&memoryRepresentation, *serializeContext, classReadyCallBack);
            return true;
        }
    }
    return false;
}

struct CSaveGameThumbnail
    : public ISaveGameThumbnail
{
    CSaveGameThumbnail(const string& name)
        : m_nRefs(0)
        , m_name(name)
    {
    }

    void AddRef()
    {
        ++m_nRefs;
    }

    void Release()
    {
        if (0 == --m_nRefs)
        {
            delete this;
        }
    }

    void GetImageInfo(int& width, int& height, int& depth)
    {
        width = m_image.width;
        height = m_image.height;
        depth = m_image.depth;
    }

    int GetWidth()
    {
        return m_image.width;
    }

    int GetHeight()
    {
        return m_image.height;
    }

    int GetDepth()
    {
        return m_image.depth;
    }

    const uint8* GetImageData()
    {
        return m_image.data.begin();
    }

    const char* GetSaveGameName()
    {
        return m_name;
    }

    CPlayerProfileManager::SThumbnail m_image;
    string m_name;
    int m_nRefs;
};


// TODO: currently we don't cache thumbnails.
// every entry in CPlayerProfileManager::TSaveGameInfoVec could store the thumbnail
// or we store ISaveGameThumbailPtr there.
// current access pattern (only one thumbnail at a time) doesn't call for optimization/caching
// but: maybe store also image format, so we don't need to swap RB
class CSaveGameEnumerator
    : public ISaveGameEnumerator
{
public:
    CSaveGameEnumerator(CPlayerProfileManager::IPlatformImpl* pImpl, CPlayerProfileManager::SUserEntry* pEntry)
        : m_nRefs(0)
        , m_pImpl(pImpl)
        , m_pEntry(pEntry)
    {
        assert (m_pImpl != 0);
        assert (m_pEntry != 0);
        pImpl->GetSaveGames(m_pEntry, m_saveGameInfoVec);
    }

    int GetCount()
    {
        return m_saveGameInfoVec.size();
    }

    bool GetDescription(int index, SGameDescription& desc)
    {
        if (index < 0 || index >= m_saveGameInfoVec.size())
        {
            return false;
        }

        CPlayerProfileManager::SSaveGameInfo& info = m_saveGameInfoVec[index];
        info.CopyTo(desc);
        return true;
    }

    virtual ISaveGameThumbailPtr GetThumbnail(int index)
    {
        if (index >= 0 && index < m_saveGameInfoVec.size())
        {
            ISaveGameThumbailPtr pThumbnail = new CSaveGameThumbnail(m_saveGameInfoVec[index].name);
            CSaveGameThumbnail* pCThumbnail = static_cast<CSaveGameThumbnail*> (pThumbnail.get());
            if (m_pImpl->GetSaveGameThumbnail(m_pEntry, m_saveGameInfoVec[index].name, pCThumbnail->m_image))
            {
                return pThumbnail;
            }
        }
        return 0;
    }

    virtual ISaveGameThumbailPtr GetThumbnail(const char* saveGameName)
    {
        CPlayerProfileManager::TSaveGameInfoVec::const_iterator iter = m_saveGameInfoVec.begin();
        CPlayerProfileManager::TSaveGameInfoVec::const_iterator iterEnd = m_saveGameInfoVec.end();
        while (iter != iterEnd)
        {
            if (iter->name.compareNoCase(saveGameName) == 0)
            {
                ISaveGameThumbailPtr pThumbnail = new CSaveGameThumbnail(iter->name);
                CSaveGameThumbnail* pCThumbnail = static_cast<CSaveGameThumbnail*> (pThumbnail.get());
                if (m_pImpl->GetSaveGameThumbnail(m_pEntry, pCThumbnail->m_name, pCThumbnail->m_image))
                {
                    return pThumbnail;
                }
                return 0;
            }
            ++iter;
        }
        return 0;
    }

    void AddRef()
    {
        ++m_nRefs;
    }

    void Release()
    {
        if (0 == --m_nRefs)
        {
            delete this;
        }
    }

private:
    int m_nRefs;
    CPlayerProfileManager::IPlatformImpl* m_pImpl;
    CPlayerProfileManager::SUserEntry* m_pEntry;
    CPlayerProfileManager::TSaveGameInfoVec m_saveGameInfoVec;
};

//------------------------------------------------------------------------
ISaveGameEnumeratorPtr CPlayerProfileManager::CreateSaveGameEnumerator(const char* userId, CPlayerProfile* pProfile)
{
    if (!m_bInitialized)
    {
        return 0;
    }

    SUserEntry* pEntry = FindEntry(userId);
    if (pEntry == 0) // no such user
    {
        GameWarning("[PlayerProfiles] CreateSaveGameEnumerator: User '%s' not logged in", userId);
        return 0;
    }

    if (pEntry->pCurrentProfile == 0 || pEntry->pCurrentProfile != pProfile)
    {
        return 0;
    }


    return new CSaveGameEnumerator(m_pImpl, pEntry);
}


//------------------------------------------------------------------------
ISaveGame* CPlayerProfileManager::CreateSaveGame(const char* userId, CPlayerProfile* pProfile)
{
    if (!m_bInitialized)
    {
        return 0;
    }

    SUserEntry* pEntry = FindEntry(userId);
    if (pEntry == 0) // no such user
    {
        GameWarning("[PlayerProfiles] CreateSaveGame: User '%s' not logged in", userId);
        return 0;
    }

    if (pEntry->pCurrentProfile == 0 || pEntry->pCurrentProfile != pProfile)
    {
        return 0;
    }

    return m_pImpl->CreateSaveGame(pEntry);
}

//------------------------------------------------------------------------
ILoadGame* CPlayerProfileManager::CreateLoadGame(const char* userId, CPlayerProfile* pProfile)
{
    if (!m_bInitialized)
    {
        return 0;
    }

    SUserEntry* pEntry = FindEntry(userId);
    if (pEntry == 0) // no such user
    {
        GameWarning("[PlayerProfiles] CreateLoadGame: User '%s' not logged in", userId);
        return 0;
    }

    if (pEntry->pCurrentProfile == 0 || pEntry->pCurrentProfile != pProfile)
    {
        return 0;
    }

    return m_pImpl->CreateLoadGame(pEntry);
}

//------------------------------------------------------------------------
bool CPlayerProfileManager::DeleteSaveGame(const char* userId, CPlayerProfile* pProfile, const char* name)
{
    if (!m_bInitialized)
    {
        return false;
    }

    SUserEntry* pEntry = FindEntry(userId);
    if (pEntry == 0) // no such user
    {
        GameWarning("[PlayerProfiles] DeleteSaveGame: User '%s' not logged in", userId);
        return false;
    }

    if (pEntry->pCurrentProfile == 0 || pEntry->pCurrentProfile != pProfile)
    {
        return false;
    }

    return m_pImpl->DeleteSaveGame(pEntry, name);
}

//------------------------------------------------------------------------
ILevelRotationFile* CPlayerProfileManager::GetLevelRotationFile(const char* userId, CPlayerProfile* pProfile, const char* name)
{
    if (!m_bInitialized)
    {
        return 0;
    }

    SUserEntry* pEntry = FindEntry(userId);
    if (pEntry == 0) // no such user
    {
        GameWarning("[PlayerProfiles] GetLevelRotationFile: User '%s' not logged in", userId);
        return NULL;
    }

    if (pEntry->pCurrentProfile == 0 || pEntry->pCurrentProfile != pProfile)
    {
        return NULL;
    }

    return m_pImpl->GetLevelRotationFile(pEntry, name);
}

//------------------------------------------------------------------------
bool CPlayerProfileManager::ResolveAttributeBlock(const char* userId, const char* attrBlockName, IResolveAttributeBlockListener* pListener, int reason)
{
    SUserEntry* pEntry = FindEntry(userId);
    if (pEntry == 0) // no such user
    {
        GameWarning("[PlayerProfiles] ResolveAttributeBlock: User '%s' not logged in", userId);
        if (pListener)
        {
            pListener->OnFailure();
        }

        return false;
    }

    if (pEntry->pCurrentProfile == NULL)
    {
        if (pListener)
        {
            pListener->OnFailure();
        }

        return false;
    }

    return m_pImpl->ResolveAttributeBlock(pEntry, pEntry->pCurrentProfile, attrBlockName, pListener, reason);
}

//------------------------------------------------------------------------
bool CPlayerProfileManager::ResolveAttributeBlock(const char* userId, const SResolveAttributeRequest& attrBlockNameRequest, IResolveAttributeBlockListener* pListener, int reason)
{
    SUserEntry* pEntry = FindEntry(userId);
    if (pEntry == 0) // no such user
    {
        GameWarning("[PlayerProfiles] ResolveAttributeBlock: User '%s' not logged in", userId);
        if (pListener)
        {
            pListener->OnFailure();
        }

        return false;
    }

    if (pEntry->pCurrentProfile == NULL)
    {
        GameWarning("[PlayerProfiles] ResolveAttributeBlock: User '%s' doesn't have a profile", userId);
        if (pListener)
        {
            pListener->OnFailure();
        }

        return false;
    }

    return m_pImpl->ResolveAttributeBlock(pEntry, pEntry->pCurrentProfile, attrBlockNameRequest, pListener, reason);
}

//------------------------------------------------------------------------
void CPlayerProfileManager::SetSharedSaveGameFolder(const char* sharedSaveGameFolder)
{
    m_sharedSaveGameFolder = sharedSaveGameFolder;
    m_sharedSaveGameFolder.TrimRight("/\\");
}

//------------------------------------------------------------------------
void CPlayerProfileManager::AddListener(IPlayerProfileListener* pListener, bool updateNow)
{
    m_listeners.reserve(8);
    stl::push_back_unique(m_listeners, pListener);

    if (updateNow)
    {
        // allow new listener to init from the current profile
        if (IPlayerProfile* pCurrentProfile = GetCurrentProfile(GetCurrentUser()))
        {
            pListener->LoadFromProfile(pCurrentProfile, false, ePR_All);
        }
    }
}

//------------------------------------------------------------------------
void CPlayerProfileManager::RemoveListener(IPlayerProfileListener* pListener)
{
    stl::find_and_erase(m_listeners, pListener);
}

//------------------------------------------------------------------------
void CPlayerProfileManager::AddOnlineAttributesListener(IOnlineAttributesListener* pListener)
{
    CRY_ASSERT_MESSAGE(m_onlineAttributesListener == NULL, "PlayerProfileManager only handles a single OnlineAttributes Listener");
    m_onlineAttributesListener = pListener;
}

//------------------------------------------------------------------------
void CPlayerProfileManager::RemoveOnlineAttributesListener(IOnlineAttributesListener* pListener)
{
    CRY_ASSERT_MESSAGE(m_onlineAttributesListener == pListener, "Can't remove listener that hasn't been added!");
    if (m_onlineAttributesListener == pListener)
    {
        m_onlineAttributesListener = NULL;
    }
}

//------------------------------------------------------------------------
void CPlayerProfileManager::SetOnlineAttributesState(const IOnlineAttributesListener::EEvent event, const IOnlineAttributesListener::EState newState)
{
    if (m_onlineAttributesState[event] != newState)
    {
        m_onlineAttributesState[event] = newState;
        SendOnlineAttributeState(event, newState);
    }
}

//------------------------------------------------------------------------
void CPlayerProfileManager::SendOnlineAttributeState(const IOnlineAttributesListener::EEvent event, const IOnlineAttributesListener::EState newState)
{
    if (m_onlineAttributesListener)
    {
        m_onlineAttributesListener->OnlineAttributeState(event, newState);
    }
}

//------------------------------------------------------------------------
void CPlayerProfileManager::GetOnlineAttributesState(const IOnlineAttributesListener::EEvent event, IOnlineAttributesListener::EState& state) const
{
    CRY_ASSERT(event > IOnlineAttributesListener::eOAE_Invalid && event < IOnlineAttributesListener::eOAE_Max);
    state = m_onlineAttributesState[event];
}

//------------------------------------------------------------------------
void CPlayerProfileManager::EnableOnlineAttributes(bool enable)
{
    m_enableOnlineAttributes = enable;
}

//------------------------------------------------------------------------
// if saving and loading online is enabled
bool CPlayerProfileManager::HasEnabledOnlineAttributes() const
{
    return m_enableOnlineAttributes;
}

//------------------------------------------------------------------------
// if saving and loading online is possible
bool CPlayerProfileManager::CanProcessOnlineAttributes() const
{
    return m_allowedToProcessOnlineAttributes;
}

//------------------------------------------------------------------------
// whether the game is in a state that it can save/load online attributes - e.g. when in a mp game session is in progress saving online is disabled
void CPlayerProfileManager::SetCanProcessOnlineAttributes(bool enable)
{
    m_allowedToProcessOnlineAttributes = enable;
}

//------------------------------------------------------------------------
bool CPlayerProfileManager::IsOnlineOnlyAttribute(const char* name)
{
    if (m_enableOnlineAttributes && m_registered)
    {
        TOnlineAttributeMap::const_iterator end = m_onlineAttributeMap.end();

        TOnlineAttributeMap::const_iterator attr = m_onlineAttributeMap.find(name);
        if (attr != end)
        {
            uint32 index = attr->second;
            CRY_ASSERT(index >= 0 && index <= m_onlineDataCount);
            return m_onlineOnlyData[index];
        }
    }

    return false;
}

//------------------------------------------------------------------------
bool CPlayerProfileManager::RegisterOnlineAttributes()
{
    return true;
}

//------------------------------------------------------------------------
bool CPlayerProfileManager::RegisterOnlineAttribute(const char* name, const char* type, const bool onlineOnly, const SCryLobbyUserData& defaultValue, CCrc32& crc)
{
    return true;
}

//------------------------------------------------------------------------
bool CPlayerProfileManager::SetUserDataType(SCryLobbyUserData* data, const char* type)
{
    return true;
}

//------------------------------------------------------------------------
void CPlayerProfileManager::GetDefaultValue(const char* type, XmlNodeRef attributeNode, SCryLobbyUserData* pOutData)
{
}

//------------------------------------------------------------------------
void CPlayerProfileManager::LoadOnlineAttributes(IPlayerProfile* pProfile)
{
}

//------------------------------------------------------------------------
void CPlayerProfileManager::SaveOnlineAttributes(IPlayerProfile* pProfile)
{
}

void CPlayerProfileManager::ClearOnlineAttributes()
{
}

void CPlayerProfileManager::LoadGamerProfileDefaults(IPlayerProfile* pProfile)
{
    CPlayerProfile* pDefaultProfile = m_pDefaultProfile;
    m_pDefaultProfile = NULL;
    pProfile->LoadGamerProfileDefaults();
    m_pDefaultProfile = pDefaultProfile;
}

void CPlayerProfileManager::ReloadProfile(IPlayerProfile* pProfile, unsigned int reason)
{
    TListeners::const_iterator it = m_listeners.begin(), itend = m_listeners.end();
    for (; it != itend; ++it)
    {
        (*it)->LoadFromProfile(pProfile, false, reason);
    }
}

void CPlayerProfileManager::SetOnlineAttributes(IPlayerProfile* pProfile, const SCryLobbyUserData* pData, const int32 onlineDataCount)
{
    CRY_ASSERT(pProfile);

    if (pProfile)
    {
        ReadOnlineAttributes(pProfile, pData, onlineDataCount);
    }
}

//------------------------------------------------------------------------
uint32 CPlayerProfileManager::GetOnlineAttributes(SCryLobbyUserData* pData, uint32 numData)
{
    return 0;
}

//------------------------------------------------------------------------
uint32 CPlayerProfileManager::GetDefaultOnlineAttributes(SCryLobbyUserData* pData, uint32 numData)
{
    return 0;
}

//------------------------------------------------------------------------
bool CPlayerProfileManager::SetUserData(SCryLobbyUserData* data, const TFlowInputData& value)
{
    return true;
}

//------------------------------------------------------------------------
bool CPlayerProfileManager::ReadUserData(const SCryLobbyUserData* data, TFlowInputData& val)
{
    return true;
}

//------------------------------------------------------------------------
uint32 CPlayerProfileManager::UserDataSize(const SCryLobbyUserData* data)
{
    return 0;
}

//static------------------------------------------------------------------------
void CPlayerProfileManager::ReadUserDataCallback(CryLobbyTaskID taskID, ECryLobbyError error, SCryLobbyUserData* pData, uint32 numData, void* pArg)
{
}

//-----------------------------------------------------------------------------
void CPlayerProfileManager::ReadOnlineAttributes(IPlayerProfile* pProfile, const SCryLobbyUserData* pData, const uint32 numData)
{
}

//------------------------------------------------------------------------
int CPlayerProfileManager::GetOnlineAttributesVersion() const
{
    return m_onlineAttributeAutoGeneratedVersion + m_onlineAttributeDefinedVersion;
}

//------------------------------------------------------------------------
int CPlayerProfileManager::GetOnlineAttributeIndexByName(const char* name)
{
    TOnlineAttributeMap::const_iterator end = m_onlineAttributeMap.end();
    TOnlineAttributeMap::const_iterator versionIter = m_onlineAttributeMap.find(name);
    int index = -1;

    if (versionIter != end)
    {
        index = versionIter->second;
    }

    return index;
}

//------------------------------------------------------------------------
void CPlayerProfileManager::GetOnlineAttributesDataFormat(SCryLobbyUserData* pData, uint32 numData)
{
}

//------------------------------------------------------------------------
uint32 CPlayerProfileManager::GetOnlineAttributeCount()
{
    return m_onlineDataCount;
}

//------------------------------------------------------------------------
int CPlayerProfileManager::ChecksumConvertValueToInt(const SCryLobbyUserData* pData)
{
    return 0;
}

//------------------------------------------------------------------------
void CPlayerProfileManager::ApplyChecksums(SCryLobbyUserData* pData, uint32 numData)
{
}

//------------------------------------------------------------------------
bool CPlayerProfileManager::ValidChecksums(const SCryLobbyUserData* pData, uint32 numData)
{
    return true;
}

//------------------------------------------------------------------------
int CPlayerProfileManager::Checksum(const int checksum, const SCryLobbyUserData* pData, uint32 numData)
{
    return 0;
}

//------------------------------------------------------------------------
int CPlayerProfileManager::ChecksumHash(const int seed, const int value) const
{
    int hash = seed + value;
    hash += (hash << 10);
    hash ^= (hash >> 6);
    hash += (hash << 3);

    return hash;
}

//------------------------------------------------------------------------
int CPlayerProfileManager::ChecksumHash(const int value0, const int value1, const int value2, const int value3, const int value4) const
{
    uint32 a = (uint32) value0;
    uint32 b = (uint32) value1;
    uint32 c = (uint32) value2;
    uint32 d = (uint32) value3;
    uint32 e = (uint32) value4;
    for (int i = 0; i < 16; i++)
    {
        a += (e >> 11);
        b += (a ^ d);
        c += (b << 3);
        d ^= (c >> 5);
        e += (d + 233);
    }

    return a + b + c + d + e;
}

//static------------------------------------------------------------------------
void CPlayerProfileManager::RegisterUserDataCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pArg)
{
}

//static------------------------------------------------------------------------
void CPlayerProfileManager::WriteUserDataCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* pArg)
{
}

#if PROFILE_DEBUG_COMMANDS
//static------------------------------------------------------------------------
void CPlayerProfileManager::DbgLoadProfile(IConsoleCmdArgs* args)
{
    CPlayerProfileManager* pPlayerProfMan = (CPlayerProfileManager*) CCryAction::GetCryAction()->GetIPlayerProfileManager();
    CryLogAlways("Loading profile '%s'", pPlayerProfMan->GetCurrentUser());

    SUserEntry* pEntry = pPlayerProfMan->FindEntry(pPlayerProfMan->GetCurrentUser());

    CPlayerProfile* pProfile = (CPlayerProfile*) pPlayerProfMan->GetCurrentProfile(pPlayerProfMan->GetCurrentUser());

    pPlayerProfMan->LoadProfile(pEntry, pProfile, pPlayerProfMan->GetCurrentUser());
}

//static------------------------------------------------------------------------
void CPlayerProfileManager::DbgSaveProfile(IConsoleCmdArgs* args)
{
    CPlayerProfileManager* pPlayerProfMan = (CPlayerProfileManager*) CCryAction::GetCryAction()->GetIPlayerProfileManager();
    CryLogAlways("Saving profile '%s'", pPlayerProfMan->GetCurrentUser());

    EProfileOperationResult result;
    pPlayerProfMan->SaveProfile(pPlayerProfMan->GetCurrentUser(), result, ePR_All);
    CryLogAlways("Result %d", result);
}

//static------------------------------------------------------------------------
void CPlayerProfileManager::DbgLoadOnlineAttributes(IConsoleCmdArgs* args)
{
    CPlayerProfileManager* pPlayerProfMan = (CPlayerProfileManager*) CCryAction::GetCryAction()->GetIPlayerProfileManager();
    CPlayerProfile* pProfile = (CPlayerProfile*) pPlayerProfMan->GetCurrentProfile(pPlayerProfMan->GetCurrentUser());
    CryLogAlways("%s", pProfile->GetName());
    pPlayerProfMan->LoadOnlineAttributes(pProfile);
}

//static------------------------------------------------------------------------
void CPlayerProfileManager::DbgSaveOnlineAttributes(IConsoleCmdArgs* args)
{
    CPlayerProfileManager* pPlayerProfMan = (CPlayerProfileManager*) CCryAction::GetCryAction()->GetIPlayerProfileManager();
    CPlayerProfile* pProfile = (CPlayerProfile*) pPlayerProfMan->GetCurrentProfile(pPlayerProfMan->GetCurrentUser());
    CryLogAlways("%s", pProfile->GetName());
    pPlayerProfMan->SaveOnlineAttributes(pProfile);
}

//static------------------------------------------------------------------------
void CPlayerProfileManager::DbgTestOnlineAttributes(IConsoleCmdArgs* args)
{
    CPlayerProfileManager* pPlayerProfMan = (CPlayerProfileManager*) CCryAction::GetCryAction()->GetIPlayerProfileManager();

    if (pPlayerProfMan->m_enableOnlineAttributes)
    {
        if (pPlayerProfMan->m_registered)
        {
            CryLogAlways("---Testing Phase %d---", pPlayerProfMan->m_testingPhase);
            switch (pPlayerProfMan->m_testingPhase)
            {
            case 0:
            {
                CryLogAlways("Version %d", pPlayerProfMan->GetOnlineAttributesVersion());
                CryLogAlways("DefinedVersion %d", pPlayerProfMan->m_onlineAttributeDefinedVersion);
                CryLogAlways("AutoVersion %d", pPlayerProfMan->m_onlineAttributeAutoGeneratedVersion);
                CryLogAlways("OnlineDataCount %d/%d", pPlayerProfMan->m_onlineDataCount, pPlayerProfMan->k_maxOnlineDataCount);
                CryLogAlways("OnlineDataBytes %d/%d", pPlayerProfMan->m_onlineDataByteCount, pPlayerProfMan->k_maxOnlineDataBytes);

                pPlayerProfMan->m_testingPhase = 1;
                DbgSaveOnlineAttributes(args);
            }
            break;

            case 1:
            {
                CryLogAlways("Saved Online Data!");

                CryLogAlways("Copying Profile FlowData");
                int arrayIndex = 0;
                CPlayerProfile* pProfile = (CPlayerProfile*) pPlayerProfMan->GetCurrentProfile(pPlayerProfMan->GetCurrentUser());

                TOnlineAttributeMap::const_iterator end = pPlayerProfMan->m_onlineAttributeMap.end();
                TOnlineAttributeMap::const_iterator iter = pPlayerProfMan->m_onlineAttributeMap.begin();
                while (iter != end)
                {
                    TFlowInputData inputData;
                    pProfile->GetAttribute(iter->first.c_str(), inputData);
                    int inputIntData;
                    inputData.GetValueWithConversion(inputIntData);

                    pPlayerProfMan->m_testFlowData[arrayIndex] = inputIntData;

                    arrayIndex++;
                    ++iter;
                }

                pPlayerProfMan->m_testingPhase = 2;
                DbgLoadOnlineAttributes(args);
            }
            break;

            case 2:
            {
                CryLogAlways("Checking Profile FlowData");
                int arrayIndex = 0;
                CPlayerProfile* pProfile = (CPlayerProfile*) pPlayerProfMan->GetCurrentProfile(pPlayerProfMan->GetCurrentUser());

                TOnlineAttributeMap::const_iterator end = pPlayerProfMan->m_onlineAttributeMap.end();
                TOnlineAttributeMap::const_iterator iter = pPlayerProfMan->m_onlineAttributeMap.begin();
                while (iter != end)
                {
                    if (iter->second >= k_onlineChecksums)
                    {
                        TFlowInputData data;
                        pProfile->GetAttribute(iter->first.c_str(), data);
                        int intValue = -1;
                        data.GetValueWithConversion(intValue);
                        if (pPlayerProfMan->m_testFlowData[arrayIndex] != intValue)
                        {
                            CryLogAlways("Non-matching Flow Data! FAILED!!!!!");
                            pPlayerProfMan->m_testingPhase = 0;
                            return;
                        }
                    }
                    arrayIndex++;
                    ++iter;
                }

                CryLogAlways("Complete Success!!!!!");

                pPlayerProfMan->m_testingPhase = 0;
                return;
            }
            break;
            }
        }
        else
        {
            CryLogAlways("Online Attributes aren't not registered");
        }
    }
    else
    {
        CryLogAlways("Online Attributes aren't not enabled");
    }
}

#endif

//-----------------------------------------------------------------------------
// FIXME: need something in crysystem or crypak to move files or directories
#if defined(WIN32) || defined(WIN64)
#include "CryWindows.h"
bool CPlayerProfileManager::MoveFileHelper(const char* existingFileName, const char* newFileName)
{
    char oldPath[ICryPak::g_nMaxPath];
    char newPath[ICryPak::g_nMaxPath];
    // need to adjust aliases and paths (use FLAGS_FOR_WRITING)
    gEnv->pCryPak->AdjustFileName(existingFileName, oldPath, AZ_ARRAY_SIZE(oldPath), ICryPak::FLAGS_FOR_WRITING);
    gEnv->pCryPak->AdjustFileName(newFileName, newPath, AZ_ARRAY_SIZE(newPath), ICryPak::FLAGS_FOR_WRITING);
    return ::MoveFile(oldPath, newPath) != 0;
}
#else
// on all other platforms, just a warning
bool CPlayerProfileManager::MoveFileHelper(const char* existingFileName, const char* newFileName)
{
    char oldPath[ICryPak::g_nMaxPath];
    gEnv->pCryPak->AdjustFileName(existingFileName, oldPath, AZ_ARRAY_SIZE(oldPath), ICryPak::FLAGS_FOR_WRITING);
    string msg;
    msg.Format("CPlayerProfileManager::MoveFileHelper for this Platform not implemented yet.\nOriginal '%s' will be lost!", oldPath);
    CRY_ASSERT_MESSAGE(0, msg.c_str());
    GameWarning(msg.c_str());
    return false;
}
#endif

void CPlayerProfileManager::GetMemoryStatistics(ICrySizer* s)
{
    SIZER_SUBCOMPONENT_NAME(s, "PlayerProfiles");
    s->Add(*this);
    if (m_pDefaultProfile)
    {
        m_pDefaultProfile->GetMemoryStatistics(s);
    }
    s->AddContainer(m_userVec);
    m_pImpl->GetMemoryStatistics(s);
    s->AddObject(m_curUserID);
}


