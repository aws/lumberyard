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

#include "LightningArc_precompiled.h"

#include "LightningGameEffectAz.h"

#include "LightningNode.h"
#include "IAttachment.h"

#include <LightningArc/LightningArcBus.h>
#include <AzCore/Component/TransformBus.h>
#include <LmbrCentral/Rendering/MeshComponentBus.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

void LightningArcParams::Reflect(AZ::ReflectContext* context)
{
    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serializeContext->Class<LightningArcParams>()
            ->Version(1)
            ->Field("LightningDeviation", &LightningArcParams::m_lightningDeviation)
            ->Field("LightningFuzziness", &LightningArcParams::m_lightningFuzziness)
            ->Field("LightningVelocity", &LightningArcParams::m_lightningVelocity)
            ->Field("BranchMaxLevel", &LightningArcParams::m_branchMaxLevel)
            ->Field("BranchProbability", &LightningArcParams::m_branchProbability)
            ->Field("StrikeTimeMin", &LightningArcParams::m_strikeTimeMin)
            ->Field("StrikeTimeMax", &LightningArcParams::m_strikeTimeMax)
            ->Field("StrikeFadeOut", &LightningArcParams::m_strikeFadeOut)
            ->Field("StrikeNumSegments", &LightningArcParams::m_strikeNumSegments)
            ->Field("StrikeNumPoints", &LightningArcParams::m_strikeNumPoints)
            ->Field("MaxNumStrikes", &LightningArcParams::m_maxNumStrikes)
            ->Field("BeamSize", &LightningArcParams::m_beamSize)
            ->Field("BeamTexTiling", &LightningArcParams::m_beamTexTiling)
            ->Field("BeamTexShift", &LightningArcParams::m_beamTexShift)
            ->Field("BeamTexFrames", &LightningArcParams::m_beamTexFrames)
            ->Field("BeamTexFPS", &LightningArcParams::m_beamTexFPS)
            ;        
    }
}

void LightningArcParams::EditorReflect(AZ::ReflectContext* context)
{
    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
        {
            editContext->Class<LightningArcParams>("Lightning Arc Params", "")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                ->ClassElement(AZ::Edit::ClassElements::Group, "Lightning")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                ->DataElement(AZ::Edit::UIHandlers::Default, &LightningArcParams::m_lightningDeviation, "Deviation", "The lower the value the more smooth an arc will appear.")

                ->DataElement(AZ::Edit::UIHandlers::Default, &LightningArcParams::m_lightningFuzziness, "Fuzziness", "The amount of noise applied to an arc.")

                ->DataElement(AZ::Edit::UIHandlers::Default, &LightningArcParams::m_lightningVelocity, "Velocity", "The upwards velocity of an arc.")

                ->ClassElement(AZ::Edit::ClassElements::Group, "Branch")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                ->DataElement(AZ::Edit::UIHandlers::Default, &LightningArcParams::m_branchMaxLevel, "Max Level", "The max number of branches allowed to spawn off of an arc.")

                ->DataElement(AZ::Edit::UIHandlers::Default, &LightningArcParams::m_branchProbability, "Probability", "How likely it is for a child branch to spawn off an arc.")

                ->ClassElement(AZ::Edit::ClassElements::Group, "Strike")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                ->DataElement(AZ::Edit::UIHandlers::Default, &LightningArcParams::m_strikeTimeMin, "Time Min", "The minimum amount of time that an arc is kept alive.")

                ->DataElement(AZ::Edit::UIHandlers::Default, &LightningArcParams::m_strikeTimeMax, "Time Max", "The maximum amount of time that an arc is kept alive.")

                ->DataElement(AZ::Edit::UIHandlers::Default, &LightningArcParams::m_strikeFadeOut, "Fade Out", "How long it takes for an arc to fade out.")

                ->DataElement(AZ::Edit::UIHandlers::Default, &LightningArcParams::m_strikeNumSegments, "Segment Count", "The number of segments in an arc.")
                ->Attribute(AZ::Edit::Attributes::Min, 1) //Allowing 0 causes a crash
                ->Attribute(AZ::Edit::Attributes::Max, MAX_STRIKE_SEGMENT_COUNT)

                ->DataElement(AZ::Edit::UIHandlers::Default, &LightningArcParams::m_strikeNumPoints, "Point Count", "The number of points per segment in an arc.")
                ->Attribute(AZ::Edit::Attributes::Min, 1) //Allowing 0 causes a crash
                ->Attribute(AZ::Edit::Attributes::Max, MAX_STRIKE_POINT_COUNT)

                ->DataElement(AZ::Edit::UIHandlers::Default, &LightningArcParams::m_maxNumStrikes, "Max Strike Count", "Sets how many arcs can be alive at one time from this component.")

                ->ClassElement(AZ::Edit::ClassElements::Group, "Beam")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                ->DataElement(AZ::Edit::UIHandlers::Default, &LightningArcParams::m_beamSize, "Size", "The size (width) of the generated arcs.")

                ->DataElement(AZ::Edit::UIHandlers::Default, &LightningArcParams::m_beamTexTiling, "Tex Tiling", "The amount of texture tiling based on the size of the arc beam.")

                ->DataElement(AZ::Edit::UIHandlers::Default, &LightningArcParams::m_beamTexShift, "Tex Shift", "How fast to move through textures in the arc's animation.")

                ->DataElement(AZ::Edit::UIHandlers::Default, &LightningArcParams::m_beamTexFrames, "Tex Frames", "How many frames are in the arc's animation.")

                ->DataElement(AZ::Edit::UIHandlers::Default, &LightningArcParams::m_beamTexFPS, "Tex FPS", "How many frames per second are in the arc's animation.")
                ;
        }
    }
}

LightningGameEffectAZ::Target::Target()
    : m_position(AZ::Vector3::CreateZero())
    , m_entityId(0)
{

}

LightningGameEffectAZ::Target::Target(const AZ::Vector3& position)
    : m_position(position)
    , m_entityId(0)
{

}

LightningGameEffectAZ::Target::Target(AZ::EntityId targetEntity)
    : m_position(AZ::Vector3::CreateZero())
    , m_entityId(targetEntity)
{
    if (m_entityId.IsValid())
    {
        AZ::TransformBus::EventResult(m_position, m_entityId, &AZ::TransformBus::Events::GetWorldTranslation);
    }
}

LightningGameEffectAZ::LightningGameEffectAZ()
{
    for (int i = 0; i < maxNumSparks; ++i)
    {
        m_sparks[i].m_renderNode = nullptr;
        m_sparks[i].m_timer = 0.0f;
    }

    SetFlag(GAME_EFFECT_AUTO_UPDATES_WHEN_ACTIVE, true);
}

LightningGameEffectAZ::~LightningGameEffectAZ()
{
    ClearSparks();
    CGameEffect::Release();
}

void LightningGameEffectAZ::Initialize(const SGameEffectParams* gameEffectParams)
{
    CGameEffect::Initialize(gameEffectParams);
    LoadData(nullptr);
}

const char* LightningGameEffectAZ::GetName() const
{
    return "Lightning";
}

void LightningGameEffectAZ::Update(float frameTime)
{
    CGameEffect::Update(frameTime);

    bool keepUpdating = false;

    m_stats.Restart();

    for (int i = 0; i < maxNumSparks; ++i)
    {
        if (m_sparks[i].m_renderNode)
        {
            m_sparks[i].m_renderNode->AddStats(&m_stats);
        }
        if (m_sparks[i].m_timer <= 0.0f)
        {
            continue;
        }
        m_sparks[i].m_timer -= frameTime;
        m_sparks[i].m_emitter.m_position = ComputeTargetPosition(m_sparks[i].m_emitter);
        m_sparks[i].m_receiver.m_position = ComputeTargetPosition(m_sparks[i].m_receiver);
        m_sparks[i].m_renderNode->SetEmiterPosition(AZVec3ToLYVec3(m_sparks[i].m_emitter.m_position));
        m_sparks[i].m_renderNode->SetReceiverPosition(AZVec3ToLYVec3(m_sparks[i].m_receiver.m_position));
        if (m_sparks[i].m_timer <= 0.0f)
        {
            gEnv->p3DEngine->UnRegisterEntityDirect(m_sparks[i].m_renderNode);
        }
        keepUpdating = true;

        m_stats.m_activeSparks.Increment();
    }

    ICVar* gameFXLightningProfile = gEnv->pConsole->GetCVar("g_gameFXLightningProfile");
    if (gameFXLightningProfile && gameFXLightningProfile->GetIVal())
    {
        m_stats.Draw();
    }

    SetActive(keepUpdating);
}

void LightningGameEffectAZ::ClearSparks()
{
    for (int i = 0; i < maxNumSparks; ++i)
    {
        if (m_sparks[i].m_renderNode)
        {
            gEnv->p3DEngine->UnRegisterEntityDirect(m_sparks[i].m_renderNode);
            m_sparks[i].m_timer = 0.0f;
            delete m_sparks[i].m_renderNode;
            m_sparks[i].m_renderNode = nullptr;
        }
    }
}

void LightningGameEffectAZ::UnloadData()
{
    ClearSparks();
}

AZ::s32 LightningGameEffectAZ::TriggerSpark(const LightningArcParams& params, _smart_ptr<IMaterial> pMaterial, const Target& emitter, const Target& receiver)
{
    AZ::u32 index = FindEmptySlot();

    if (!m_sparks[index].m_renderNode)
    {
        m_sparks[index].m_renderNode = new CLightningRenderNode();
    }

    gEnv->p3DEngine->UnRegisterEntityDirect(m_sparks[index].m_renderNode);

    m_sparks[index].m_renderNode->Reset();
    m_sparks[index].m_renderNode->SetMaterial(pMaterial);
    m_sparks[index].m_renderNode->SetLightningParams(params);

    SetEmitter(index, emitter);
    SetReceiver(index, receiver);

    m_sparks[index].m_timer = m_sparks[index].m_renderNode->TriggerSpark();
    gEnv->p3DEngine->RegisterEntity(m_sparks[index].m_renderNode);

    SetActive(true);

    return index;
}

void LightningGameEffectAZ::RemoveSpark(const AZ::s32 spark)
{
    if (spark == -1)
    {
        return;
    }

    if (spark < 0 || spark >= maxNumSparks)
    {
        return;
    }

    if (m_sparks[spark].m_timer > 0.0f)
    {
        gEnv->p3DEngine->UnRegisterEntityDirect(m_sparks[spark].m_renderNode);
        m_sparks[spark].m_timer = 0.0f;
    }
}

void LightningGameEffectAZ::SetEmitter(const AZ::s32 spark, const Target& target)
{
    if (spark != -1 && m_sparks[spark].m_renderNode != 0)
    {
        m_sparks[spark].m_emitter = target;
        AZ::Vector3 targetPos = ComputeTargetPosition(target);
        m_sparks[spark].m_emitter.m_position = targetPos;
        m_sparks[spark].m_renderNode->SetEmiterPosition(AZVec3ToLYVec3(targetPos));
    }
}

void LightningGameEffectAZ::SetReceiver(const AZ::s32 spark, const Target& target)
{
    if (spark != -1 && m_sparks[spark].m_renderNode != 0)
    {
        m_sparks[spark].m_receiver = target;
        AZ::Vector3 targetPos = ComputeTargetPosition(target);
        m_sparks[spark].m_receiver.m_position = targetPos;
        m_sparks[spark].m_renderNode->SetReceiverPosition(AZVec3ToLYVec3(targetPos));
    }
}

float LightningGameEffectAZ::GetSparkRemainingTime(const AZ::s32 spark) const
{
    if (spark != -1)
    {
        return AZ::GetMax(m_sparks[spark].m_timer, 0.0f);
    }
    return 0.0f;
}

void LightningGameEffectAZ::SetSparkDeviationMult(const AZ::s32 spark, float deviationMult)
{
    if (spark != -1 && m_sparks[spark].m_renderNode != 0)
    {
        m_sparks[spark].m_renderNode->SetSparkDeviationMult(deviationMult);
    }
}

AZ::s32 LightningGameEffectAZ::FindEmptySlot() const
{
    float lowestTime = m_sparks[0].m_timer;
    AZ::s32 bestIdex = 0;
    for (AZ::u32 i = 0; i < maxNumSparks; ++i)
    {
        if (m_sparks[i].m_timer <= 0.0f)
        {
            return i;
        }
        if (m_sparks[i].m_timer <= lowestTime)
        {
            lowestTime = m_sparks[i].m_timer;
            bestIdex = i;
        }
    }
    return bestIdex;
}

AZ::Vector3 LightningGameEffectAZ::ComputeTargetPosition(const Target& target)
{
    AZ::Vector3 result = target.m_position;
    return result;
}
