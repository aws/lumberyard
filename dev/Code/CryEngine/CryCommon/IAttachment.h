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

#ifndef CRYINCLUDE_CRYCOMMON_IATTACHMENT_H
#define CRYINCLUDE_CRYCOMMON_IATTACHMENT_H
#pragma once


#include <ICryAnimation.h>
#include <IParticles.h>
#include <CryName.h>

struct IStatObj;
struct IMaterial;
struct IPhysicalEntity;
struct ICharacterInstance;
struct IAttachmentSkin;
struct IAttachment;
struct IAttachmentObject;
struct IVertexAnimation;
struct IProxy;


//Flags used by game DO NOT REORDER
enum AttachmentTypes
{
    CA_BONE,
    CA_FACE,
    CA_SKIN,
    CA_PROX,
    CA_PROW,
    CA_VCLOTH,
    CA_Invalid,
};

enum AttachmentFlags
{
    //Static Flags
    FLAGS_ATTACH_HIDE_ATTACHMENT                = 0x01, //already stored in CDF, so don't change this
    FLAGS_ATTACH_PHYSICALIZED_RAYS              = 0x02, //already stored in CDF, so don't change this
    FLAGS_ATTACH_PHYSICALIZED_COLLISIONS        = 0x04, //already stored in CDF, so don't change this
    FLAGS_ATTACH_SW_SKINNING                    = 0x08, //already stored in CDF, so don't change this
    FLAGS_ATTACH_RENDER_ONLY_EXISTING_LOD       = 0x10, //already stored in CDF, so don't change this
    FLAGS_ATTACH_LINEAR_SKINNING                = 0x20, //already stored in CDF, so don't change this
    FLAGS_ATTACH_MATRIX_SKINNING                = 0x40, //already stored in CDF, so don't change this

    FLAGS_ATTACH_PHYSICALIZED = FLAGS_ATTACH_PHYSICALIZED_RAYS | FLAGS_ATTACH_PHYSICALIZED_COLLISIONS,

    //Dynamic Flags
    FLAGS_ATTACH_VISIBLE                      = 1 << 13,  //we set this flag if we can render the object
    FLAGS_ATTACH_PROJECTED                  = 1 << 14,    //we set this flag if we can attacht the object to a triangle
    FLAGS_ATTACH_WAS_PHYSICALIZED       = 1 << 15, // the attachment actually was physicalized
    FLAGS_ATTACH_HIDE_MAIN_PASS         = 1 << 16,
    FLAGS_ATTACH_HIDE_SHADOW_PASS       = 1 << 17,
    FLAGS_ATTACH_HIDE_RECURSION         = 1 << 18,
    FLAGS_ATTACH_NEAREST_NOFOV          = 1 << 19,
    FLAGS_ATTACH_NO_BBOX_INFLUENCE  = 1 << 21,
    FLAGS_ATTACH_COMBINEATTACHMENT  = 1 << 24,
};


struct SVClothParams
{
    float thickness; // the radius of the particles
    float collisionDamping; // dampens only non-colliding particles
    float dragDamping; // a value around 1 - smaller for less stretch stiffness, bigger for over-relaxation
    float stretchStiffness; // a value around 1 - smaller for less stretch stiffness, bigger for over-relaxation
    float shearStiffness; // a smaller than 1 value affecting the shear constraints
    float bendStiffness; // a smaller than 1 value affecting the bend constraints
    int numIterations; // number of iterations for the positional PGS solver (contacts & edges)
    float timeStep; // the (pseudo)fixed time step for the simulator
    float rigidDamping; // blending factor between local and world space rotation
    float translationBlend; // blending factor between local and world space translation
    float rotationBlend; // blending factor between local and world space rotation
    float friction; // friction coefficient for particles at contact points
    float pullStiffness; // the strength of pulling pinched vertices towards the anchor position [0..1]
    float tolerance; // collision tolerance expressed by a factor of thickness
    float* weights; // per vertex weights used for blending with animation
    float maxBlendWeight; // maximum value of pull stiffness for blending with animation
    float maxAnimDistance;
    float windBlend;
    int collDampingRange;
    float stiffnessGradient;
    bool halfStretchIterations;
    bool hide;
    bool isMainCharacter;

    string simMeshName;
    string renderMeshName;

    string renderBinding;
    string simBinding;

    SVClothParams()
        : timeStep(0.01f)
        , numIterations(20)
        , thickness(0.01f)
        , pullStiffness(0)
        , stretchStiffness(1)
        , shearStiffness(0)
        , bendStiffness(0)
        , collisionDamping(1)
        , dragDamping(1)
        , rigidDamping(0)
        , translationBlend(1)
        , rotationBlend(1)
        , friction(0)
        , tolerance(1.5f)
        , maxAnimDistance(0)
        , windBlend(0)
        , collDampingRange(3)
        , maxBlendWeight(1)
        , stiffnessGradient(0)
        , halfStretchIterations(false)
        , simMeshName("")
        , renderMeshName("")
        , renderBinding("")
        , simBinding("")
        , hide(false)
        , isMainCharacter(false)
    {
    }
};

struct SimulationParams
{
    enum
    {
        MaxCollisionProxies = 10
    };

    enum ClampType
    {
        DISABLED  = 0x00,

        PENDULUM_CONE        = 0x01, //for pendula
        PENDULUM_HINGE_PLANE = 0x02, //for pendula
        PENDULUM_HALF_CONE   = 0x03, //for pendula
        SPRING_ELLIPSOID     = 0x04, //for springs
        TRANSLATIONAL_PROJECTION = 0x05  //used to project joints out of proxies
    };

    ClampType m_nClampType;
    bool     m_useDebugSetup;
    bool     m_useDebugText;
    bool     m_useSimulation;
    bool     m_useRedirect;
    uint8    m_nSimFPS;

    f32      m_fMaxAngle;
    f32      m_fRadius;
    Vec2     m_vSphereScale;
    Vec2     m_vDiskRotation;

    f32      m_fMass;
    f32      m_fGravity;
    f32      m_fDamping;
    f32      m_fStiffness;

    Vec3     m_vPivotOffset;
    Vec3     m_vSimulationAxis;
    Vec3     m_vStiffnessTarget;
    Vec2     m_vCapsule;
    CCryName m_strProcFunction;

    int32    m_nProjectionType;
    CCryName m_strDirTransJoint;
    DynArray<CCryName> m_arrProxyNames;  //test capsules/sphere joint against these colliders

    SimulationParams()
    {
        m_nClampType      = DISABLED;
        m_useDebugSetup   = 0;
        m_useDebugText    = 0;
        m_useSimulation   = 1;
        m_useRedirect     = 0;
        m_nSimFPS         = 10;       //don't put to 0

        m_fMaxAngle       = 45.0f;
        m_vDiskRotation.set(0, 0);

        m_fRadius         = 0.5f;
        m_vSphereScale.set(1.0f, 1.0f);

        m_fMass           = 1.0f;
        m_fGravity        = 9.81f;
        m_fDamping        = 1.0f;
        m_fStiffness      = 0.0f;

        m_vPivotOffset.zero();
        m_vSimulationAxis = Vec3(0.0f, 0.5f, 0.0f);
        m_vStiffnessTarget.zero();
        m_vCapsule.set(0, 0);
        m_nProjectionType = 0;
    };
};


struct RowSimulationParams
{
    enum ClampMode
    {
        PENDULUM_CONE        = 0x00, //for pendula
        PENDULUM_HINGE_PLANE = 0x01, //for pendula
        PENDULUM_HALF_CONE   = 0x02, //for pendula
        TRANSLATIONAL_PROJECTION = 0x03  //used to project joints out of proxies
    };

    ClampMode m_nClampMode;
    bool     m_useDebugSetup;
    bool     m_useDebugText;
    bool     m_useSimulation;
    uint8    m_nSimFPS;

    f32      m_fConeAngle;
    Vec3     m_vConeRotation;

    f32      m_fMass;
    f32      m_fGravity;
    f32      m_fDamping;
    f32      m_fJointSpring;
    f32      m_fRodLength;
    Vec2     m_vStiffnessTarget;
    Vec2     m_vTurbulence;
    f32      m_fMaxVelocity;

    Vec3     m_worldSpaceDamping;

    bool     m_cycle;
    f32      m_fStretch;
    uint32   m_relaxationLoops;


    Vec3     m_vTranslationAxis;
    CCryName m_strDirTransJoint;

    Vec2     m_vCapsule;
    int32    m_nProjectionType;
    DynArray<CCryName> m_arrProxyNames;  //test capsules/sphere joint against these colliders

    RowSimulationParams()
    {
        m_nClampMode      = PENDULUM_CONE;
        m_useDebugSetup   = 0;
        m_useDebugText    = 0;
        m_useSimulation   = 1;
        m_nSimFPS         = 10;

        m_fConeAngle      = 45.0f;
        m_vConeRotation   = Vec3(0, 0, 0);

        m_fMass           = 1.0f;
        m_fGravity        = 9.81f;
        m_fDamping        = 1.0f;
        m_fJointSpring    = 0.0f;
        m_fRodLength      = 0.0f;
        m_vStiffnessTarget = Vec2(0.0f, 0.0f);
        m_vTurbulence     = Vec2(0.5f, 0.0f);
        m_fMaxVelocity    = 8.0f;

        m_cycle           = 0;
        m_relaxationLoops = 0;
        m_fStretch        = 0.10f;

        m_worldSpaceDamping = Vec3(0.f, 0.f, 0.f);

        m_vCapsule.set(0, 0);
        m_nProjectionType = 0;
        m_vTranslationAxis = Vec3(0.0f, 1.0f, 0.0f);
    };
};


struct IAttachmentManager
{
    // <interfuscator:shuffle>
    virtual ~IAttachmentManager(){}
    virtual uint32 LoadAttachmentList(const char* pathname) = 0;
    virtual IAttachment* CreateAttachment(const char* szName, uint32 type, const char* szJointName = 0, bool bCallProject = true, const char* szSecondJointName = 0) = 0;
    virtual int32 RemoveAttachmentByInterface(const IAttachment* ptr) = 0;
    virtual int32 RemoveAttachmentByName(const char* szName) = 0;
    virtual int32 RemoveAttachmentByNameCRC(uint32 nameCRC) = 0;

    virtual IAttachment* GetInterfaceByName(const char* szName) const = 0;
    virtual IAttachment* GetInterfaceByIndex(uint32 c) const = 0;
    virtual IAttachment* GetInterfaceByNameCRC(uint32 nameCRC) const = 0;

    virtual int32 GetAttachmentCount() const = 0;
    virtual int32 GetIndexByName(const char* szName) const = 0;
    virtual int32 GetIndexByNameCRC(uint32 nameCRC) const = 0;

    virtual uint32 ProjectAllAttachment() = 0;

    virtual void PhysicalizeAttachment(int idx, IPhysicalEntity* pent = 0, int nLod = 0) = 0;
    virtual void DephysicalizeAttachment(int idx, IPhysicalEntity* pent = 0) = 0;

    virtual ICharacterInstance* GetSkelInstance() const = 0;

    virtual int32 GetProxyCount() const = 0;
    virtual IProxy* CreateProxy(const char* szName, const char* szJointName) = 0;
    virtual IProxy* GetProxyInterfaceByIndex(uint32 c) const = 0;
    virtual IProxy* GetProxyInterfaceByName(const char* szName) const = 0;
    virtual IProxy* GetProxyInterfaceByCRC(uint32 nameCRC) const = 0;
    virtual int32   GetProxyIndexByName(const char* szName) const = 0;
    virtual int32   RemoveProxyByInterface(const IProxy* ptr) = 0;
    virtual int32   RemoveProxyByName(const char* szName) = 0;
    virtual int32   RemoveProxyByNameCRC(uint32 nameCRC) = 0;
    virtual void    DrawProxies(uint32 enable) = 0;
    virtual uint32 GetProcFunctionCount() const = 0;
    virtual const char* GetProcFunctionName(uint32 idx) const = 0;

    virtual void ClearAttachmentData() = 0;
    virtual void RebindAttachments(uint32 nLoadingFlags) = 0;

    // </interfuscator:shuffle>
};

struct IAttachment
{
    // <interfuscator:shuffle>
    virtual void AddRef() = 0;
    virtual void Release() = 0;
    virtual const char* GetName() const = 0;
    virtual uint32 GetNameCRC() const = 0;
    virtual uint32 ReName(const char* szSocketName, uint32 crc) = 0;

    virtual uint32 GetType() const = 0;
    virtual uint32 SetJointName(const char* szJointName = 0) = 0;

    virtual uint32 GetFlags() = 0;
    virtual void SetFlags(uint32 flags) = 0;

    //attachment location in default model-pose
    virtual const QuatT& GetAttAbsoluteDefault() const = 0;
    virtual void SetAttAbsoluteDefault(const QuatT& rot) = 0;

    //attachment location relative to the attachment point (bone,face). Similar to an additional rotation
    //its is the location in the default-pose
    virtual void SetAttRelativeDefault(const QuatT& mat) = 0;
    virtual const QuatT& GetAttRelativeDefault() const = 0;

    //its is the location of the attachment in the animated-pose is model-space
    virtual const QuatT& GetAttModelRelative() const = 0;
    //its is the location of the attachment in the animated-pose in world-space
    virtual const QuatTS GetAttWorldAbsolute() const = 0;

    virtual void UpdateAttModelRelative() = 0;

    virtual void HideAttachment(uint32 x) = 0;
    virtual uint32 IsAttachmentHidden() = 0;
    virtual void HideInRecursion(uint32 x) = 0;
    virtual uint32 IsAttachmentHiddenInRecursion() = 0;
    virtual void HideInShadow(uint32 x) = 0;
    virtual uint32 IsAttachmentHiddenInShadow() = 0;

    virtual void AlignJointAttachment() = 0;

    virtual uint32 GetJointID() const = 0;

    virtual uint32 AddBinding(IAttachmentObject* pModel, _smart_ptr<ISkin> pISkin = nullptr, uint32 nLoadingFlags = 0) = 0;
    virtual IAttachmentObject* GetIAttachmentObject() const = 0;
    virtual IAttachmentSkin* GetIAttachmentSkin() = 0;
    virtual void ClearBinding(uint32 nLoadingFlags = 0) = 0;
    virtual uint32 SwapBinding(IAttachment* pNewAttachment) = 0;

    virtual SimulationParams& GetSimulationParams() {   static SimulationParams ap; return ap;  };
    virtual void PostUpdateSimulationParams(bool bAttachmentSortingRequired, const char* pJointName = 0) {};

    virtual RowSimulationParams& GetRowParams() {   static RowSimulationParams ap;  return ap;  };

    virtual size_t SizeOfThis() = 0;
    virtual void Serialize(TSerialize ser) = 0;
    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;
    virtual ~IAttachment(){}
    // </interfuscator:shuffle>
};

//Interface to a skin-attachment
struct IAttachmentSkin
{
    // <interfuscator:shuffle>
    virtual void AddRef() = 0;
    virtual void Release() = 0;
    virtual ISkin* GetISkin() = 0;
    virtual IVertexAnimation* GetIVertexAnimation() = 0;
    virtual float GetExtent(EGeomForm eForm) = 0;
    virtual void GetRandomPos(PosNorm& ran, EGeomForm eForm) const = 0;
    virtual void GetMemoryUsage(class ICrySizer* pSizer) const = 0;
    virtual void ComputeGeometricMean(SMeshLodInfo& lodInfo) const = 0;
    virtual ~IAttachmentSkin(){}
    // </interfuscator:shuffle>

#ifdef EDITOR_PCDEBUGCODE
    virtual void TriggerMeshStreaming(uint32 nDesiredRenderLOD, const SRenderingPassInfo& passInfo) = 0;
    virtual void DrawWireframeStatic(const Matrix34& m34, int nLOD, uint32 color) = 0;
#endif
};




// Description:
//     This interface define a way to allow an object to be bound to a character.
struct IAttachmentObject
{
    enum EType
    {
        eAttachment_Unknown,
        eAttachment_StatObj,
        eAttachment_Skeleton,
        eAttachment_SkinMesh,
        eAttachment_Entity,
        eAttachment_Light,
        eAttachment_Effect,
    };

    // <interfuscator:shuffle>
    virtual ~IAttachmentObject(){}
    virtual EType GetAttachmentType() = 0;

    virtual void ProcessAttachment(IAttachment* pIAttachment) = 0;
    virtual void RenderAttachment(SRendParams& rParams, const SRenderingPassInfo& passInfo) {};

    // returns handled state
    virtual bool PhysicalizeAttachment(IAttachmentManager* pManager, int idx, int nLod, IPhysicalEntity* pent, const Vec3& offset) { return false; }
    virtual bool UpdatePhysicalizedAttachment(IAttachmentManager* pManager, int idx, IPhysicalEntity* pent, const QuatT& offset) { return false; }

    virtual AABB GetAABB() = 0;
    virtual float GetRadiusSqr() = 0;

    virtual IStatObj* GetIStatObj() const;
    virtual ICharacterInstance* GetICharacterInstance() const;
    virtual IAttachmentSkin* GetIAttachmentSkin() const { return 0; };
    virtual const char* GetObjectFilePath() const
    {
        ICharacterInstance* pICharInstance = GetICharacterInstance();
        if (pICharInstance)
        {
            return pICharInstance->GetFilePath();
        }
        IStatObj* pIStaticObject = GetIStatObj();
        if (pIStaticObject)
        {
            return pIStaticObject->GetFilePath();
        }
        IAttachmentSkin* pIAttachmentSkin = GetIAttachmentSkin();
        if (pIAttachmentSkin)
        {
            ISkin* pISkin = pIAttachmentSkin->GetISkin();
            if (pISkin)
            {
                return pISkin->GetModelFilePath();
            }
        }
        return NULL;
    }
    virtual _smart_ptr<IMaterial> GetBaseMaterial(uint32 nLOD = 0) const = 0;
    virtual void SetReplacementMaterial(_smart_ptr<IMaterial> pMaterial, uint32 nLOD = 0) = 0;
    virtual _smart_ptr<IMaterial> GetReplacementMaterial(uint32 nLOD = 0) const = 0;

    virtual void OnRemoveAttachment(IAttachmentManager* pManager, int idx){}
    virtual void Release() = 0;
    // </interfuscator:shuffle>
};


inline ICharacterInstance* IAttachmentObject::GetICharacterInstance() const
{
    return 0;
}

inline IStatObj* IAttachmentObject::GetIStatObj() const
{
    return 0;
}

//

struct CCGFAttachment
    : public IAttachmentObject
{
    virtual EType GetAttachmentType() { return eAttachment_StatObj; };
    void ProcessAttachment(IAttachment* pIAttachment) {}
    void RenderAttachment(SRendParams& rParams, const SRenderingPassInfo& passInfo)
    {
        rParams.pInstance = this;
        _smart_ptr<IMaterial> pPrev = rParams.pMaterial;
        rParams.pMaterial = pObj->GetMaterial();
        if (m_pReplacementMaterial)
        {
            rParams.pMaterial = m_pReplacementMaterial;
        }
        pObj->Render(rParams, passInfo);
        rParams.pMaterial = pPrev;
    };

    AABB GetAABB() { return pObj->GetAABB(); };
    float GetRadiusSqr() { return sqr(pObj->GetRadius()); };
    IStatObj* GetIStatObj() const;
    void Release() { delete this;   }

    _smart_ptr<IMaterial> GetBaseMaterial(uint32 nLOD) const;
    void SetReplacementMaterial(_smart_ptr<IMaterial> pMaterial, uint32 nLOD = 0) override { m_pReplacementMaterial = pMaterial; };
    _smart_ptr<IMaterial> GetReplacementMaterial(uint32 nLOD) const { return m_pReplacementMaterial; }

    _smart_ptr<IStatObj> pObj;
    _smart_ptr<IMaterial> m_pReplacementMaterial;
};


inline IStatObj* CCGFAttachment::GetIStatObj() const
{
    return pObj;
}


inline _smart_ptr<IMaterial> CCGFAttachment::GetBaseMaterial(uint32 nLOD) const
{
    return m_pReplacementMaterial ? m_pReplacementMaterial : pObj->GetMaterial();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
struct CSKELAttachment
    : public IAttachmentObject
{
    CSKELAttachment()
    {
    }

    virtual EType GetAttachmentType() { return eAttachment_Skeleton; };
    void ProcessAttachment(IAttachment* pIAttachment)
    {
        m_pCharInstance->ProcessAttachment(pIAttachment);
    };
    void RenderAttachment(SRendParams& rParams, const SRenderingPassInfo& passInfo)
    {
        rParams.pInstance = this;
        _smart_ptr<IMaterial> pPrev = rParams.pMaterial;
        rParams.pMaterial = (_smart_ptr<IMaterial>)(m_pCharInstance ? m_pCharInstance->GetIMaterial() : 0);
        if (m_pReplacementMaterial)
        {
            rParams.pMaterial = m_pReplacementMaterial;
        }
        m_pCharInstance->Render(rParams, QuatTS(IDENTITY), passInfo);
        rParams.pMaterial = pPrev;
    };
    AABB GetAABB() {  return m_pCharInstance ? m_pCharInstance->GetAABB() : AABB(AABB::RESET);    };
    float GetRadiusSqr() { return m_pCharInstance ? m_pCharInstance->GetRadiusSqr() : 0.0f;     }
    ICharacterInstance* GetICharacterInstance() const;

    void Release()
    {
        if (m_pCharInstance)
        {
            m_pCharInstance->OnDetach();
        }
        delete this;
    }

    _smart_ptr<IMaterial> GetBaseMaterial(uint32 nLOD) const;
    void SetReplacementMaterial(_smart_ptr<IMaterial> pMaterial, uint32 nLOD) override { m_pReplacementMaterial = pMaterial; }
    _smart_ptr<IMaterial> GetReplacementMaterial(uint32 nLOD) const { return m_pReplacementMaterial; }

    _smart_ptr<ICharacterInstance> m_pCharInstance;
    _smart_ptr<IMaterial> m_pReplacementMaterial;
};

inline ICharacterInstance* CSKELAttachment::GetICharacterInstance() const
{
    return m_pCharInstance;
}

inline _smart_ptr<IMaterial> CSKELAttachment::GetBaseMaterial(uint32 nLOD) const
{
    return m_pReplacementMaterial ? m_pReplacementMaterial : (m_pCharInstance ? m_pCharInstance->GetIMaterial() : nullptr);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
struct CSKINAttachment
    : public IAttachmentObject
{
    CSKINAttachment()
    {
    }

    virtual EType GetAttachmentType() { return eAttachment_SkinMesh; };
    AABB GetAABB() { return AABB(AABB::RESET); };
    float GetRadiusSqr() { return 0.0f; }
    IAttachmentSkin* GetIAttachmentSkin() const { return m_pIAttachmentSkin; }
    void Release()  {  delete this; }
    void ProcessAttachment(IAttachment* pIAttachment) { };

    _smart_ptr<IMaterial> GetBaseMaterial(uint32 nLOD) const;
    void SetReplacementMaterial(_smart_ptr<IMaterial> pMaterial, uint32 nLOD) { m_pReplacementMaterial[nLOD] = pMaterial;   }
    _smart_ptr<IMaterial> GetReplacementMaterial(uint32 nLOD) const { return m_pReplacementMaterial[nLOD]; }

    _smart_ptr<IAttachmentSkin> m_pIAttachmentSkin;
    _smart_ptr<IMaterial> m_pReplacementMaterial[6];
};

inline _smart_ptr<IMaterial> CSKINAttachment::GetBaseMaterial(uint32 nLOD) const
{
    return m_pReplacementMaterial[nLOD] ? m_pReplacementMaterial[nLOD] : (m_pIAttachmentSkin ? m_pIAttachmentSkin->GetISkin()->GetIMaterial(nLOD) : nullptr);
}

struct CEntityAttachment
    :  public IAttachmentObject
{
public:
    CEntityAttachment()
        : m_scale(1.0f, 1.0f, 1.0f)
        , m_id(0){}

    virtual EType GetAttachmentType() { return eAttachment_Entity; };
    void SetEntityId(EntityId id) { m_id = id; };
    EntityId GetEntityId() { return m_id; }

    void ProcessAttachment(IAttachment* pIAttachment)
    {
        const QuatTS quatTS = QuatTS(pIAttachment->GetAttWorldAbsolute());
        IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_id);
        if (pEntity)
        {
            pEntity->SetPosRotScale(quatTS.t, quatTS.q, m_scale * quatTS.s, ENTITY_XFORM_NO_PROPOGATE);
        }
    }

    AABB GetAABB()
    {
        IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_id);
        AABB aabb(Vec3(0, 0, 0), Vec3(0, 0, 0));

        if (pEntity)
        {
            pEntity->GetLocalBounds(aabb);
        }
        return aabb;
    };

    float GetRadiusSqr()
    {
        IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_id);
        if (pEntity)
        {
            AABB aabb;
            pEntity->GetLocalBounds(aabb);
            return aabb.GetRadiusSqr();
        }
        else
        {
            return 0.0f;
        }
    };

    void Release() { delete this;   };

    _smart_ptr<IMaterial> GetBaseMaterial(uint32 nLOD) const;
    void SetReplacementMaterial(_smart_ptr<IMaterial> pMaterial, uint32 nLOD) override {};
    _smart_ptr<IMaterial> GetReplacementMaterial(uint32 nLOD) const { return 0; }
    void SetScale(const Vec3& scale)
    {
        m_scale = scale;
    }

private:
    EntityId    m_id;
    Vec3 m_scale;
};

inline _smart_ptr<IMaterial> CEntityAttachment::GetBaseMaterial(uint32 nLOD) const
{
    return 0;
}

struct CLightAttachment
    : public IAttachmentObject
{
public:
    CLightAttachment()
        : m_pLightSource(0)   {};
    virtual ~CLightAttachment()
    {
        if (m_pLightSource)
        {
            gEnv->p3DEngine->DeleteRenderNode(m_pLightSource);
            m_pLightSource = NULL;
        }
    };

    virtual EType GetAttachmentType() { return eAttachment_Light; };

    void LoadLight(const CDLight& light)
    {
        m_pLightSource = gEnv->p3DEngine->CreateLightSource();
        if (m_pLightSource)
        {
            m_pLightSource->SetLightProperties(light);
        }
    }

    ILightSource* GetLightSource() { return m_pLightSource; }

    void ProcessAttachment(IAttachment* pIAttachment)
    {
        if (m_pLightSource)
        {
            CDLight& light = m_pLightSource->GetLightProperties();
            Matrix34 worldMatrix = Matrix34(pIAttachment->GetAttWorldAbsolute());
            Vec3 origin = worldMatrix.GetTranslation();
            light.SetPosition(origin);
            light.SetMatrix(worldMatrix);
            light.m_sName = pIAttachment->GetName();
            m_pLightSource->SetMatrix(worldMatrix);
            f32 r = light.m_fRadius;
            m_pLightSource->SetBBox(AABB(Vec3(origin.x - r, origin.y - r, origin.z - r), Vec3(origin.x + r, origin.y + r, origin.z + r)));
            gEnv->p3DEngine->RegisterEntity(m_pLightSource);
        }
    }

    AABB GetAABB()
    {
        f32 r = m_pLightSource->GetLightProperties().m_fRadius;
        return AABB(Vec3(-r, -r, -r), Vec3(+r, +r, +r));
    };

    float GetRadiusSqr() { return sqr(m_pLightSource->GetLightProperties().m_fRadius); }

    void Release() { delete this; };

    _smart_ptr<IMaterial> GetBaseMaterial(uint32 nLOD) const;
    void SetReplacementMaterial(_smart_ptr<IMaterial> pMaterial, uint32 nLOD) override {};
    _smart_ptr<IMaterial> GetReplacementMaterial(uint32 nLOD) const { return 0; }

private:
    ILightSource* m_pLightSource;
};

inline _smart_ptr<IMaterial> CLightAttachment::GetBaseMaterial(uint32 nLOD) const
{
    return 0;
}

struct CEffectAttachment
    : public IAttachmentObject
{
public:

    virtual EType GetAttachmentType() { return eAttachment_Effect; };

    CEffectAttachment(const char* effectName, const Vec3& offset, const Vec3& dir, f32 scale, unsigned int flags)
        : m_loc(IParticleEffect::ParticleLoc(offset, dir, scale))
        , m_flags(flags)
    {
        m_pEffect = gEnv->pParticleManager->FindEffect(effectName, "Character Attachment", true);
    }

    CEffectAttachment(IParticleEffect* pParticleEffect, const Vec3& offset, const Vec3& dir, f32 scale, unsigned int flags)
        : m_loc(IParticleEffect::ParticleLoc(offset, dir, scale))
        , m_flags(flags)
    {
        m_pEffect = pParticleEffect;
    }

    CEffectAttachment(const char *effectName, const Matrix34& locMat, unsigned int flags)
        : m_loc(locMat), m_flags(flags)
    {
        m_pEffect = gEnv->pParticleManager->FindEffect(effectName, "Character Attachment", true);
    }

    CEffectAttachment(IParticleEffect* pParticleEffect, const Matrix34& locMat, unsigned int flags)
        : m_loc(locMat), m_flags(flags)
    {
        m_pEffect = pParticleEffect;
    }

    virtual ~CEffectAttachment()
    {
        if (m_pEmitter)
        {
            m_pEmitter->Activate(false);
        }
    }

    IParticleEmitter* GetEmitter()
    {
        return m_pEmitter;
    }

    void ProcessAttachment(IAttachment* pIAttachment)
    {
        if (!pIAttachment->IsAttachmentHidden())
        {
            QuatTS loc = pIAttachment->GetAttWorldAbsolute() * m_loc;
            if (!m_pEmitter && m_pEffect != 0)
            {
                m_pEmitter = m_pEffect->Spawn(loc, m_flags);
            }
            else if (m_pEmitter)
            {
                m_pEmitter->SetLocation(loc);
            }
        }
        else
        {
            m_pEmitter = 0;
        }
    }

    AABB GetAABB()
    {
        if (m_pEmitter)
        {
            AABB bb;
            m_pEmitter->GetLocalBounds(bb);
            return bb;
        }
        else
        {
            return AABB(Vec3(-0.1f), Vec3(0.1f));
        }
    };

    float GetRadiusSqr()
    {
        if (m_pEmitter)
        {
            AABB bb;
            m_pEmitter->GetLocalBounds(bb);
            return bb.GetRadiusSqr();
        }
        else
        {
            return 0.1f;
        }
    }

    void Release() { delete this; };

    _smart_ptr<IMaterial> GetBaseMaterial(uint32 nLOD) const;
    void SetReplacementMaterial(_smart_ptr<IMaterial> pMaterial, uint32 nLOD) override {};
    _smart_ptr<IMaterial> GetReplacementMaterial(uint32 nLOD) const { return 0; }

    void SetSpawnParams(const SpawnParams& params)
    {
        if (m_pEmitter)
        {
            m_pEmitter->SetSpawnParams(params);
        }
    }

    unsigned int GetFlags() const { return m_flags; }

private:
    _smart_ptr<IParticleEmitter>    m_pEmitter;
    _smart_ptr<IParticleEffect>     m_pEffect;
    QuatTS                                              m_loc;
    unsigned int                    m_flags;    // MKS 20140920
};

inline _smart_ptr<IMaterial> CEffectAttachment::GetBaseMaterial(uint32 nLOD) const
{
    return 0;
}

struct IProxy
{
    virtual const char* GetName() const = 0;
    virtual uint32 GetNameCRC() const = 0;
    virtual uint32 ReName(const char* szSocketName, uint32 crc) = 0;
    virtual uint32 SetJointName(const char* szJointName = 0) = 0;
    virtual uint32 GetJointID() const = 0;

    virtual const QuatT& GetProxyAbsoluteDefault() const = 0;
    virtual const QuatT& GetProxyRelativeDefault() const = 0;
    virtual const QuatT& GetProxyModelRelative() const = 0;
    virtual void SetProxyAbsoluteDefault(const QuatT& rot) = 0;

    virtual uint32 ProjectProxy() = 0;

    virtual void AlignProxyWithJoint() = 0;
    virtual Vec4 GetProxyParams() const = 0;
    virtual void SetProxyParams(const Vec4& params) = 0;
    virtual int8 GetProxyPurpose() const = 0;
    virtual void SetProxyPurpose(int8 p) = 0;
    virtual void SetHideProxy(uint8 nHideProxy) = 0;

    virtual ~IProxy(){}
};


#endif // CRYINCLUDE_CRYCOMMON_IATTACHMENT_H
