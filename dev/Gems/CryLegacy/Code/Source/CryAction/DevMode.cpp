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

// Description : Helper class for CCryAction implementing developer mode-only
//               functionality


#include "CryLegacy_precompiled.h"
#include "DevMode.h"
#include "ILevelSystem.h"
#include "CryAction.h"
#include "IActorSystem.h"
#include <IMovementController.h>
#include <ArgumentParser.h> // for easy string splitting
#include <AzCore/IO/FileIO.h>
#include <AzFramework/IO/FileOperations.h>

CDevMode::CDevMode()
{
    if (gEnv->pInput)
    {
        gEnv->pInput->AddEventListener(this);
    }
    if (gEnv->pSystem)
    {
        gEnv->pSystem->GetIRemoteConsole()->RegisterListener(this, "CDevMode");
    }
    m_bSlowDownGameSpeed = false;
    m_bHUD = false;
}

CDevMode::~CDevMode()
{
    if (gEnv->pInput)
    {
        gEnv->pInput->RemoveEventListener(this);
    }
    if (gEnv->pSystem)
    {
        gEnv->pSystem->GetIRemoteConsole()->UnregisterListener(this);
    }
}

void CDevMode::GotoTagPoint(int i)
{
    std::vector<STagFileEntry> tags = LoadTagFile();
    if (tags.size() > size_t(i))
    {
        STagFileEntry ent = tags[i];
        IActor* pActor = CCryAction::GetCryAction()->GetClientActor();
        if (!pActor)
        {
            return;
        }
        IEntity* pEntity = pActor->GetEntity();
        Quat rot;

        Vec3 pos = ent.pos;

        // pos loaded is a camera position, we must convert it into the player position.
        if (IMovementController* pMC = pActor->GetMovementController())
        {
            SMovementState ms;
            pMC->GetMovementState(ms);
            pos -= (ms.eyePosition - ms.pos);
        }

        rot.SetRotationXYZ(ent.ang);
        Vec3 scale = pEntity->GetScale();
        pEntity->SetPosRotScale(pos, rot, scale);
        pActor->SetViewRotation(rot);
    }
}

void CDevMode::SaveTagPoint(int i)
{
    std::vector<STagFileEntry> tags = LoadTagFile();
    if (tags.size() <= size_t(i))
    {
        tags.resize(i + 1);
    }

    tags[i].pos = GetISystem()->GetViewCamera().GetMatrix().GetTranslation();
    tags[i].ang = Ang3(GetISystem()->GetViewCamera().GetMatrix());

    SaveTagFile(tags);
}

void CDevMode::OnGameplayCommand(const char* cmd)
{
    SArgumentParser args;
    args.SetDelimiter(":");
    args.SetArguments(cmd);

    string action;
    args.GetArg(0, action);
    if (action == "GotoTagPoint")
    {
        int tagpoint;
        args.GetArg(1, tagpoint);
        if (tagpoint >= 0 && tagpoint <= 11)
        {
            GotoTagPoint(tagpoint);
        }
    }
    else if (action == "SetViewMode" && gEnv->pGame && gEnv->pGame->GetIGameFramework())
    {
        int mode;
        args.GetArg(1, mode);
        IActor* pPlayer = gEnv->pGame->GetIGameFramework()->GetClientActor();
        static ICVar* pCamVar = gEnv->pConsole->GetCVar("cl_cam_orbit");
        if (pPlayer && pCamVar)
        {
            switch (mode)
            {
            case 0:
                pCamVar->Set(0);
                if (pPlayer->IsThirdPerson())
                {
                    pPlayer->ToggleThirdPerson();
                }
                break;
            case 1:
                pCamVar->Set(0);
                if (!pPlayer->IsThirdPerson())
                {
                    pPlayer->ToggleThirdPerson();
                }
                break;
            case 2:
                pCamVar->Set(1);
                break;
            }
        }
    }
    else if (action == "SetFlyMode" && gEnv->pGame && gEnv->pGame->GetIGameFramework())
    {
        int mode;
        args.GetArg(1, mode);
        IActor* pPlayer = gEnv->pGame->GetIGameFramework()->GetClientActor();
        if (pPlayer && mode >= 0 && mode < 3)
        {
            pPlayer->SetFlyMode(mode);
        }
    }
}


bool CDevMode::OnInputEvent(const SInputEvent& evt)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_ACTION);
    bool handled = false;
    bool canCheat = CCryAction::GetCryAction()->CanCheat();

    IActor* pActor = CCryAction::GetCryAction()->GetClientActor();
    if (!pActor)
    {
        return false;
    }
    IEntity* pEntity = pActor->GetEntity();
    if (!pEntity)
    {
        return false;
    }

    // tag-point functionality is provided by the editor already, so we should ignore it
    // when running in the editor
    if (!gEnv->IsEditor())
    {
        if ((evt.state == eIS_Pressed) && canCheat)
        {
            if ((evt.modifiers & eMM_Shift) != 0 && (evt.modifiers & eMM_Alt) == 0) // Shift ONLY
            {
                if (handled = (evt.keyId == eKI_F1))
                {
                    GotoTagPoint(0);
                }
                if (handled = (evt.keyId == eKI_F2))
                {
                    GotoTagPoint(1);
                }
                if (handled = (evt.keyId == eKI_F3))
                {
                    GotoTagPoint(2);
                }
                if (handled = (evt.keyId == eKI_F4))
                {
                    GotoTagPoint(3);
                }
                if (handled = (evt.keyId == eKI_F5))
                {
                    GotoTagPoint(4);
                }
                if (handled = (evt.keyId == eKI_F6))
                {
                    GotoTagPoint(5);
                }
                if (handled = (evt.keyId == eKI_F7))
                {
                    GotoTagPoint(6);
                }
                if (handled = (evt.keyId == eKI_F8))
                {
                    GotoTagPoint(7);
                }
                if (handled = (evt.keyId == eKI_F9))
                {
                    GotoTagPoint(8);
                }
                if (handled = (evt.keyId == eKI_F10))
                {
                    GotoTagPoint(9);
                }
                if (handled = (evt.keyId == eKI_F11))
                {
                    GotoTagPoint(10);
                }
                if (handled = (evt.keyId == eKI_F12))
                {
                    GotoTagPoint(11);
                }
            }
            else if ((evt.modifiers & eMM_Shift) == 0 && ((evt.modifiers & eMM_Alt) != 0 || (evt.modifiers & eMM_Ctrl) != 0)) // Alt or Ctrl and NO Shift
            {
                if (handled = (evt.keyId == eKI_F1))
                {
                    SaveTagPoint(0);
                }
                if (handled = (evt.keyId == eKI_F2))
                {
                    SaveTagPoint(1);
                }
                if (handled = (evt.keyId == eKI_F3))
                {
                    SaveTagPoint(2);
                }
                if (handled = (evt.keyId == eKI_F4))
                {
                    SaveTagPoint(3);
                }
                if (handled = (evt.keyId == eKI_F5))
                {
                    SaveTagPoint(4);
                }
                if (handled = (evt.keyId == eKI_F6))
                {
                    SaveTagPoint(5);
                }
                if (handled = (evt.keyId == eKI_F7))
                {
                    SaveTagPoint(6);
                }
                if (handled = (evt.keyId == eKI_F8))
                {
                    SaveTagPoint(7);
                }
                if (handled = (evt.keyId == eKI_F9))
                {
                    SaveTagPoint(8);
                }
                if (handled = (evt.keyId == eKI_F10))
                {
                    SaveTagPoint(9);
                }
                if (handled = (evt.keyId == eKI_F11))
                {
                    SaveTagPoint(10);
                }
                if (handled = (evt.keyId == eKI_F12))
                {
                    SaveTagPoint(11);
                }
            }
        }
    }
    else
    {
        // place commands which should only be dealt with in game mode here
    }

    // shared commands
    if (canCheat)
    {
        if (!handled && (evt.state == eIS_Pressed) && (evt.modifiers & eMM_Shift) != 0 && (evt.modifiers & eMM_Alt) != 0)
        {
            if (handled = (evt.keyId == eKI_NP_1))
            {
                GotoSpecialSpawnPoint(1);
            }
            if (handled = (evt.keyId == eKI_NP_2))
            {
                GotoSpecialSpawnPoint(2);
            }
            if (handled = (evt.keyId == eKI_NP_3))
            {
                GotoSpecialSpawnPoint(3);
            }
            if (handled = (evt.keyId == eKI_NP_4))
            {
                GotoSpecialSpawnPoint(4);
            }
            if (handled = (evt.keyId == eKI_NP_5))
            {
                GotoSpecialSpawnPoint(5);
            }
            if (handled = (evt.keyId == eKI_NP_6))
            {
                GotoSpecialSpawnPoint(6);
            }
            if (handled = (evt.keyId == eKI_NP_7))
            {
                GotoSpecialSpawnPoint(7);
            }
            if (handled = (evt.keyId == eKI_NP_8))
            {
                GotoSpecialSpawnPoint(8);
            }
            if (handled = (evt.keyId == eKI_NP_9))
            {
                GotoSpecialSpawnPoint(9);
            }
        }
        else if (!handled && (evt.state == eIS_Pressed) && !(evt.modifiers & eMM_Modifiers))
        {
            if (handled = (evt.keyId == eKI_NP_1) && !gEnv->bMultiplayer)   // give all items
            {
                CCryAction::GetCryAction()->GetIItemSystem()->GetIEquipmentManager()->GiveEquipmentPack(pActor, "Player_Default", true, true);
            }
            else if (handled = (evt.keyId == eKI_F2))   // go to next spawnpoint
            {
                if (gEnv->pScriptSystem->BeginCall("BasicActor", "OnNextSpawnPoint"))
                {
                    gEnv->pScriptSystem->PushFuncParam(pEntity->GetScriptTable());
                    gEnv->pScriptSystem->EndCall();
                }
            }
        }
    }

    if (handled == false && evt.state == eIS_Pressed && (evt.modifiers & eMM_Alt) != 0)
    {
        if (evt.keyId == eKI_F7)
        {
            SwitchSlowDownGameSpeed();
        }
        else if (evt.keyId == eKI_F8)
        {
            SwitchHUD();
        }
    }

    // AlexL: don't mark commands as handled for now. Would stop processing/propagating to other
    //        listeners. especially for non-modifier keys like f7,f8,f9 problematic
    return false; // handled;
}

std::vector<STagFileEntry> CDevMode::LoadTagFile()
{
    std::vector<STagFileEntry> out;

    // tag file is data that is authored at run time but actually should be placed on disk.
    if ((!gEnv) || (!gEnv->pCryPak))
    {
        return out; // no direct fileIO is available.
    }
    AZ::IO::HandleType finput = gEnv->pCryPak->FOpen(TagFileName().c_str(), "rt");

    if (finput != AZ::IO::InvalidHandle)
    {
        while (!gEnv->pCryPak->FEof(finput))
        {
            STagFileEntry ent;
            char readBuffer[256] = { 0 }; // 256 is enough for 6 floats, even if they're maximum length
            if (!gEnv->pCryPak->FGets(readBuffer, 256, finput))
            {
                break;
            }

            if (6 != azsscanf(readBuffer, "%f,%f,%f,%f,%f,%f",
                    &ent.pos.x, &ent.pos.y, &ent.pos.z,
                    &ent.ang.x, &ent.ang.y, &ent.ang.z
                    ))
            {
                break;
            }

            out.push_back(ent);
        }
        gEnv->pCryPak->FClose(finput);
    }

    return out;
}

void CDevMode::SaveTagFile(const std::vector<STagFileEntry>& tags)
{
    CrySetFileAttributes(TagFileName().c_str(), FILE_ATTRIBUTE_NORMAL);

    AZ::IO::HandleType foutput = gEnv->pCryPak->FOpen(TagFileName().c_str(), "wt");

    if (foutput != AZ::IO::InvalidHandle)
    {
        for (std::vector<STagFileEntry>::const_iterator iter = tags.begin(); iter != tags.end(); ++iter)
        {
            AZ::IO::Print(foutput, "%f,%f,%f,%f,%f,%f\n",
                iter->pos.x, iter->pos.y, iter->pos.z,
                iter->ang.x, iter->ang.y, iter->ang.z);
        }
        gEnv->pCryPak->FClose(foutput);
    }
}

string CDevMode::TagFileName()
{
    ILevelSystem* pLevelSystem = CCryAction::GetCryAction()->GetILevelSystem();
    ILevel* pLevel = pLevelSystem->GetCurrentLevel();
    if (!pLevel)
    {
        return "";
    }

    string path = pLevel->GetLevelInfo()->GetPath();
    path = string("@user@/") + path + "/tags.txt"; // @assets@ is not wrtable!

    return path;
}

void CDevMode::SwitchSlowDownGameSpeed()
{
    if (m_bSlowDownGameSpeed)
    {
        gEnv->pConsole->ExecuteString("t_FixedStep 0");
        m_bSlowDownGameSpeed = false;
    }
    else
    {
        gEnv->pConsole->ExecuteString("t_FixedStep 0.001");
        m_bSlowDownGameSpeed = true;
    }
}

void CDevMode::SwitchHUD()
{
    if (m_bHUD)
    {
        gEnv->pConsole->ExecuteString("hud 0");
        m_bHUD = false;
    }
    else
    {
        gEnv->pConsole->ExecuteString("hud 2");
        m_bHUD = true;
    }
}

void CDevMode::GotoSpecialSpawnPoint(int i)
{
    char cmd[256];
    azsnprintf(cmd, sizeof(cmd) - 1, "#g_localActor:SpawnAtSpawnPoint(\"SpawnPoint%d\")", i);
    cmd[sizeof(cmd) - 1] = '\0';
    gEnv->pConsole->ExecuteString(cmd);
}
