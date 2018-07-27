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
#include "CryAction.h"
#include "GameplayRecorder.h"
#include "IGameStateRecorder.h"
#include "CryActionCVars.h"

//------------------------------------------------------------------------
CGameplayRecorder::CGameplayRecorder(CCryAction* pGameFramework)
    : m_pGameFramework(pGameFramework)
    , m_lastdiscreet(0.0f)
    , m_sampleinterval(0.5f)
    , m_pGameStateRecorder(NULL)
{
    //m_pMetadataRecorder->InitSave("GameData.meta");
    m_listeners.reserve(12);
}

//------------------------------------------------------------------------
CGameplayRecorder::~CGameplayRecorder()
{
    //m_pMetadataRecorder->Reset();
}

//------------------------------------------------------------------------
void CGameplayRecorder::Init()
{
}

//------------------------------------------------------------------------
void CGameplayRecorder::Update(float frameTime)
{
    // modify this as you wish
    //m_example.RecordGameData();

    // if anyone needs to touch this, talk to Lin first
    m_pMetadataRecorder->Flush();

    //CTimeValue now(gEnv->pTimer->GetFrameStartTime());
    //if (gEnv->pTimer->GetFrameStartTime()-m_lastdiscreet<m_sampleinterval)
    //  return;

    //m_lastdiscreet=now;
    /* disabled
        // only actors for now
        IActorIteratorPtr actorIt=m_pGameFramework->GetIActorSystem()->CreateActorIterator();
        while (IActor *pActor=actorIt->Next())
            Event(pActor->GetEntity(), GameplayEvent(eGE_DiscreetSample));
        */
    if (m_pGameStateRecorder && m_pGameStateRecorder->IsEnabled())
    {
        m_pGameStateRecorder->Update();
    }
}

//------------------------------------------------------------------------
void CGameplayRecorder::Release()
{
    SAFE_RELEASE(m_pGameStateRecorder);

    delete this;
}

//------------------------------------------------------------------------
void CGameplayRecorder::RegisterListener(IGameplayListener* pGameplayListener)
{
    stl::push_back_unique(m_listeners, pGameplayListener);
}

//------------------------------------------------------------------------
void CGameplayRecorder::UnregisterListener(IGameplayListener* pGameplayListener)
{
    stl::find_and_erase(m_listeners, pGameplayListener);

    if (m_listeners.empty())
    {
        stl::free_container(m_listeners);
    }
}

//------------------------------------------------------------------------
void CGameplayRecorder::Event(IEntity* pEntity, const GameplayEvent& event)
{
    std::vector<IGameplayListener*>::iterator it = m_listeners.begin();
    std::vector<IGameplayListener*>::iterator end = m_listeners.end();

    if (CCryActionCVars::Get().g_debug_stats)
    {
        CryLog("GamePlay event %d, %p, value:%f extra:%p desc:%s", event.event, pEntity, event.value, event.extra, event.description ? event.description : "");
    }

    for (; it != end; ++it)
    {
        (*it)->OnGameplayEvent(pEntity, event);
    }
}

void CGameplayRecorder::OnGameData(const IMetadata* pGameData)
{
    m_pMetadataRecorder->RecordIt(pGameData); // will be flushed together with all other gamedata collected in this frame
    // flushing is performed in Update
}

void CGameplayRecorder::CExampleMetadataListener::RecordGameData()
{
    IGameplayRecorder* pRecorder = CCryAction::GetCryAction()->GetIGameplayRecorder();

    {
        int64 t = gEnv->pTimer->GetFrameStartTime().GetValue();
        IMetadataPtr pMetadata;
        pMetadata->SetTag(eDT_frame);
        pMetadata->SetValue(eBT_i64, (uint8*)&t, 8);
        //m_pMetadataRecorder->RecordIt( pMetadata.get() );
        pRecorder->OnGameData(pMetadata.get());
    }

    IActorSystem* pActorSystem = CCryAction::GetCryAction()->GetIActorSystem();

    IEntityItPtr it = gEnv->pEntitySystem->GetEntityIterator();
    while (!it->IsEnd())
    {
        IEntity* pEntity = it->Next();
        if (!pEntity->IsActive())
        {
            continue;
        }

        if (IActor* pActor = pActorSystem->GetActor(pEntity->GetId()))
        {
            IMetadataPtr pActorMetadata;

            // 'actr'
            pActorMetadata->SetTag(eDT_actor);

            string sActorName = pActor->GetEntity()->GetName();
            pActorMetadata->AddField(eDT_name, eBT_str, (uint8*)sActorName.data(), sActorName.size());
            Vec3 pos = pActor->GetEntity()->GetWorldPos();
            pActorMetadata->AddField(eDT_position, eBT_vec3, (uint8*)&pos, sizeof(pos));
            if (pActor != NULL)
            {
                uint8 health = pActor->GetHealthAsRoundedPercentage();
                pActorMetadata->AddField(eDT_health, eBT_i08, &health, 1);
                uint8 armor = pActor->GetArmor();
                pActorMetadata->AddField(eDT_armor, eBT_i08, &armor, 1);
                uint8 isgod = pActor->IsGod() ? 0xff : 0x00;
                pActorMetadata->AddField(eDT_isgod, eBT_i08, &isgod, 1);
                //uint8 third = pOldActor->IsThirdPerson() ? 0xff : 0x00;
                //pActorMetadata->AddField(eDT_3rdperson, eBT_i08, &third, 1);
                if (IItem* pItem = pActor->GetCurrentItem())
                {
                    IMetadataPtr pItemMetadata;
                    pItemMetadata->SetTag(eDT_item);
                    string name = pItem->GetEntity()->GetName();
                    pItemMetadata->AddField(eDT_name, eBT_str, (uint8*)name.data(), name.size());
                    if (IWeapon* pWeapon = pItem->GetIWeapon())
                    {
                        IMetadataPtr pWeaponMetadata;
                        pWeaponMetadata->SetTag(eDT_weapon);
                        IFireMode* pFireMode = pWeapon->GetFireMode(pWeapon->GetCurrentFireMode());
                        string fire = pFireMode->GetName();
                        pWeaponMetadata->AddField(eDT_firemode, eBT_str, (uint8*)fire.data(), name.size());
                        if (pFireMode->GetAmmoType())
                        {
                            string ammt = pFireMode->GetAmmoType()->GetName();
                            pWeaponMetadata->AddField(eDT_ammotype, eBT_str, (uint8*)ammt.data(), name.size());
                        }
                        uint32 ammo = pFireMode->GetAmmoCount();
                        pWeaponMetadata->AddField(eDT_ammocount, eBT_u32, (uint8*)&ammo, sizeof(ammo));

                        pItemMetadata->AddField(pWeaponMetadata.get());
                    }
                    pActorMetadata->AddField(pItemMetadata.get());
                }
            }


            //m_pMetadataRecorder->RecordIt(pActorMetadata.get());
            pRecorder->OnGameData(pActorMetadata.get());
        }
        else
        {
            // 'entt' name/class/position
            IMetadataPtr pEntityMetadata;
            pEntityMetadata->SetTag(eDT_entity);
            string name = pEntity->GetName();
            pEntityMetadata->AddField(eDT_name, eBT_str, (uint8*)name.data(), name.size());
            string type = pEntity->GetClass()->GetName();
            pEntityMetadata->AddField(eDT_type, eBT_str, (uint8*)type.data(), type.size());
            Vec3 pos = pEntity->GetWorldPos();
            pEntityMetadata->AddField(eDT_position, eBT_vec3, (uint8*)&pos, sizeof(pos));

            //m_pMetadataRecorder->RecordIt(pEntityMetadata.get());
            pRecorder->OnGameData(pEntityMetadata.get());
        }
    }
}

void CGameplayRecorder::CExampleMetadataListener::OnData(const IMetadata* metadata)
{
    //DumpMetadata(metadata, 0);

    CCompositeData data;
    data.Compose(metadata);

    switch (metadata->GetTag())
    {
    case eDT_frame:
    {
        CTimeValue tv;
        if (const int64* temp = boost::get<const int64>(&data.GetValue()))
        {
            tv = *temp;
        }
        CryLog("frame %f", tv.GetSeconds());
    }
    break;

    case eDT_entity:
    {
        string name;
        if (const string* temp = boost::get<const string>(&data.GetField(eDT_name).GetValue()))
        {
            name = *temp;
        }
        string type;
        if (const string* temp = boost::get<const string>(&data.GetField(eDT_type).GetValue()))
        {
            type = *temp;
        }
        Vec3 pos;
        if (const Vec3* temp = boost::get<const Vec3>(&data.GetField(eDT_position).GetValue()))
        {
            pos = *temp;
        }

        CryLog("entity [%s] [%s] [%f,%f,%f]", name.c_str(), type.c_str(), pos.x, pos.y, pos.z);
    }
    break;

    case eDT_actor:
    {
        string name;
        if (const string* temp = boost::get<const string>(&data.GetField(eDT_name).GetValue()))
        {
            name = *temp;
        }
        Vec3 pos;
        if (const Vec3* temp = boost::get<const Vec3>(&data.GetField(eDT_position).GetValue()))
        {
            pos = *temp;
        }
        string item;
        if (const string* temp = boost::get<const string>(&data.GetField(eDT_item).GetField(eDT_name).GetValue()))
        {
            item = *temp;
        }
        string firemode;
        if (const string* temp = boost::get<const string>(&data.GetField(eDT_item).GetField(eDT_weapon).GetField(eDT_firemode).GetValue()))
        {
            firemode = *temp;
        }
        CryLog("actor [%s] [%s] [%s] [%f,%f,%f]", name.c_str(), item.c_str(), firemode.c_str(), pos.x, pos.y, pos.z);
    }
    break;
    }
}


IGameStateRecorder* CGameplayRecorder::EnableGameStateRecorder(bool bEnable, IGameplayListener* pL, bool bRecording)
{
    if (!m_pGameStateRecorder)
    {
        m_pGameStateRecorder = gEnv->pGame->CreateGameStateRecorder(pL);//pTestManager);
    }
    if (m_pGameStateRecorder)
    {
        m_pGameStateRecorder->Enable(bEnable, bRecording);
    }
    return m_pGameStateRecorder;
}


////////////////////////////////////////////////////////////////////////////
void CGameplayRecorder::GetMemoryStatistics(ICrySizer* s)
{
    SIZER_SUBCOMPONENT_NAME(s, "GameplayRecorder");
    s->Add(*this);
    s->AddContainer(m_listeners);

    if (m_pGameStateRecorder)
    {
        m_pGameStateRecorder->GetMemoryStatistics(s);
    }
}
