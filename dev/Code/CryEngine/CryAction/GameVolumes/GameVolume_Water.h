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

#ifndef CRYINCLUDE_CRYACTION_GAMEVOLUMES_GAMEVOLUME_WATER_H
#define CRYINCLUDE_CRYACTION_GAMEVOLUMES_GAMEVOLUME_WATER_H
#pragma once

class CGameVolume_Water
    : public CGameObjectExtensionHelper<CGameVolume_Water, IGameObjectExtension>
{
    struct WaterProperties
    {
        WaterProperties(IEntity* pEntity)
        {
            CRY_ASSERT(pEntity != NULL);

            memset(this, 0, sizeof(WaterProperties));

            SmartScriptTable properties;
            IScriptTable* pScriptTable = pEntity->GetScriptTable();

            if ((pScriptTable != NULL) && pScriptTable->GetValue("Properties", properties))
            {
                properties->GetValue("StreamSpeed", streamSpeed);
                properties->GetValue("FogDensity", fogDensity);
                properties->GetValue("UScale", uScale);
                properties->GetValue("VScale", vScale);
                properties->GetValue("Depth", depth);
                properties->GetValue("ViewDistanceMultiplier", viewDistanceMultiplier);
                properties->GetValue("MinSpec", minSpec);
                properties->GetValue("MaterialLayerMask", materialLayerMask);
                properties->GetValue("FogColorMultiplier", fogColorMultiplier);
                properties->GetValue("color_FogColor", fogColor);
                properties->GetValue("bFogColorAffectedBySun", fogColorAffectedBySun);
                properties->GetValue("FogShadowing", fogShadowing);
                properties->GetValue("bCapFogAtVolumeDepth", capFogAtVolumeDepth);
                properties->GetValue("bAwakeAreaWhenMoving", awakeAreaWhenMoving);
                properties->GetValue("bIsRiver", isRiver);
                properties->GetValue("bCaustics", caustics);
                properties->GetValue("CausticIntensity", causticIntensity);
                properties->GetValue("CausticTiling", causticTiling);
                properties->GetValue("CausticHeight", causticHeight);
            }
        }

        float streamSpeed;
        float fogDensity;
        float uScale;
        float vScale;
        float depth;
        float viewDistanceMultiplier;
        int   minSpec;
        int   materialLayerMask;
        float fogColorMultiplier;
        Vec3    fogColor;
        bool  fogColorAffectedBySun;
        float fogShadowing;
        bool  capFogAtVolumeDepth;
        bool  awakeAreaWhenMoving;
        bool  isRiver;
        bool caustics;
        float causticIntensity;
        float causticTiling;
        float causticHeight;
    };

public:
    DECLARE_COMPONENT_TYPE("GameVolume_Water", 0x5F68BF9EF91E413B, 0xBA05B8FF4B204C5A)

    CGameVolume_Water();
    virtual ~CGameVolume_Water();

    // IGameObjectExtension
    virtual bool Init(IGameObject* pGameObject);
    virtual void InitClient(ChannelId channelId) {};
    virtual void PostInit(IGameObject* pGameObject);
    virtual void PostInitClient(ChannelId channelId) {};
    virtual bool ReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params);
    virtual void PostReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) {}
    virtual bool GetEntityPoolSignature(TSerialize signature);
    virtual void FullSerialize(TSerialize ser) {};
    virtual bool NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int flags) { return false; };
    virtual void PostSerialize() {};
    virtual void SerializeSpawnInfo(TSerialize ser, IEntity* entity) {}
    virtual ISerializableInfoPtr GetSpawnInfo() {return 0; }
    virtual void Update(SEntityUpdateContext& ctx, int slot);
    virtual void HandleEvent(const SGameObjectEvent& gameObjectEvent);
    virtual void ProcessEvent(SEntityEvent&);
    virtual void SetChannelId(ChannelId id) {};
    virtual void SetAuthority(bool auth) {};
    virtual void PostUpdate(float frameTime) { CRY_ASSERT(false); }
    virtual void PostRemoteSpawn() {};
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    // ~IGameObjectExtension

private:

    struct SWaterSegment
    {
        SWaterSegment()
            : m_pWaterRenderNode(NULL)
            , m_pWaterArea(NULL)
            , m_physicsLocalAreaCenter(ZERO) {}
        IWaterVolumeRenderNode* m_pWaterRenderNode;
        IPhysicalEntity* m_pWaterArea;
        Vec3 m_physicsLocalAreaCenter;
    };

    typedef std::vector<SWaterSegment> WaterSegments;

    void CreateWaterRenderNode(IWaterVolumeRenderNode*& pWaterRenderNode);
    void SetupVolume();
    void SetupVolumeSegment(const WaterProperties& waterProperties, const uint32 segmentIndex, const Vec3* pVertices, const uint32 vertexCount);

    void CreatePhysicsArea(const uint32 segmentIndex, const Matrix34& baseMatrix, const Vec3* pVertices, uint32 vertexCount, const bool isRiver, const float streamSpeed);
    void DestroyPhysicsAreas();

    void AwakeAreaIfRequired(bool forceAwake);
    void UpdateRenderNode(IWaterVolumeRenderNode* pWaterRenderNode, const Matrix34& newLocation);

    void FillOutRiverSegment(const uint32 segmentIndex, const Vec3* pVertices, const uint32 vertexCount, Vec3* pVerticesOut);

    void DebugDrawVolume();

    WaterSegments m_segments;

    Matrix34                                m_baseMatrix;
    Matrix34                                m_initialMatrix;
    Vec3                                        m_lastAwakeCheckPosition;
    float                                       m_volumeDepth;
    float                                       m_streamSpeed;
    bool                                        m_awakeAreaWhenMoving;
    bool                                        m_isRiver;
};

#endif // CRYINCLUDE_CRYACTION_GAMEVOLUMES_GAMEVOLUME_WATER_H
