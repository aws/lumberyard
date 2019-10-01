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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_ENTITYOBJECT_H
#define CRYINCLUDE_EDITOR_OBJECTS_ENTITYOBJECT_H
#pragma once


#include "BaseObject.h"
#include "EntityScript.h"

#include "IMovieSystem.h"
#include "IEntityObjectListener.h"
#include "EntityPrototype.h"
#include "StatObjValidator.h"
#include "Gizmo.h"
#include "CryListenerSet.h"
#include "StatObjBus.h"
#include "CharacterBoundsBus.h"

#include <QObject>

#define CLASS_LIGHT "Light"
#define CLASS_DESTROYABLE_LIGHT "DestroyableLight"
#define CLASS_RIGIDBODY_LIGHT "RigidBodyLight"
#define CLASS_ENVIRONMENT_LIGHT "EnvironmentLight"

class CFlowGraph;
class CEntityObject;
class QMenu;
class IOpticsElementBase;
class CPanelTreeBrowser;
struct SPyWrappedProperty;
class CTrackViewAnimNode;

/*!
 *  CEntityEventTarget is an Entity event target and type.
 */
struct CEntityEventTarget
{
    CBaseObject* target; //! Target object.
    _smart_ptr<CGizmo> pLineGizmo;
    QString event;
    QString sourceEvent;
};

//////////////////////////////////////////////////////////////////////////
// Named link from entity to entity.
//////////////////////////////////////////////////////////////////////////
struct CEntityLink
{
    GUID targetId;   // Target entity id.
    CEntityObject* target; // Target entity.
    QString name;    // Name of the link.
    _smart_ptr<CGizmo> pLineGizmo;
};

inline const EntityGUID& ToEntityGuid(REFGUID guid)
{
    static EntityGUID eguid;
    eguid = *((EntityGUID*)&guid);
    return eguid;
}

struct IPickEntitesOwner
{
    virtual void AddEntity(CBaseObject* pEntity) = 0;
    virtual CBaseObject* GetEntity(int nIdx) = 0;
    virtual int GetEntityCount() = 0;
    virtual void RemoveEntity(int nIdx) = 0;
};

/*!
 *  CEntity is an static object on terrain.
 *
 */
class CRYEDIT_API CEntityObject
    : public CBaseObject
    , protected StatObjEventBus::MultiHandler
    , protected AZ::CharacterBoundsNotificationBus::MultiHandler
    , public IEntityEventListener
{
    Q_OBJECT
public:
    ~CEntityObject();

    //////////////////////////////////////////////////////////////////////////
    // Overrides from CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    //! Return type name of Entity.
    QString GetTypeDescription() const { return GetEntityClass(); };

    //////////////////////////////////////////////////////////////////////////
    bool IsSameClass(CBaseObject* obj);

    virtual bool Init(IEditor* ie, CBaseObject* prev, const QString& file);
    virtual void InitVariables();
    virtual void Done();
    virtual bool CreateGameObject();

    void DrawExtraLightInfo (DisplayContext& disp);

    bool GetEntityPropertyBool(const char* name) const;
    int GetEntityPropertyInteger(const char* name) const;
    float GetEntityPropertyFloat(const char* name) const;
    QString GetEntityPropertyString(const char* name) const;
    void SetEntityPropertyBool(const char* name, bool value);
    void SetEntityPropertyInteger(const char* name, int value);
    void SetEntityPropertyFloat(const char* name, float value);
    void SetEntityPropertyString(const char* name, const QString& value);

    virtual void Display(DisplayContext& disp);
    virtual QString GetTooltip() const;

    virtual int MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);
    virtual void OnContextMenu(QMenu* menu);

    virtual void SetFrozen(bool bFrozen);
    void SetName(const QString& name);
    void SetSelected(bool bSelect);

    virtual bool IsScalable() const;
    virtual bool IsRotatable() const;

    virtual IStatObj* GetIStatObj();

    // Override IsScalable value from script file.
    bool GetForceScale(){return m_bForceScale; };
    void SetForceScale(bool bForceScale){m_bForceScale = bForceScale; };

    virtual void BeginEditParams(IEditor* ie, int flags);

    void SetPropertyPanelsState();

    virtual void EndEditParams(IEditor* ie);
    void BeginEditMultiSelParams(bool bAllOfSameType);
    void EndEditMultiSelParams();

    virtual void GetLocalBounds(AABB& box);

    virtual void SetModified(bool boModifiedTransformOnly);

    virtual bool HitTest(HitContext& hc);
    virtual bool HitHelperTest(HitContext& hc);
    virtual bool HitTestRect(HitContext& hc);
    void UpdateVisibility(bool bVisible);
    bool ConvertFromObject(CBaseObject* object);

    virtual void Serialize(CObjectArchive& ar);
    virtual void PostLoad(CObjectArchive& ar);

    XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode);

    void SetLookAt(CBaseObject* target);

    //////////////////////////////////////////////////////////////////////////
    void OnEvent(ObjectEvent event);

    virtual void SetTransformDelegate(ITransformDelegate* pTransformDelegate) override;

    virtual IPhysicalEntity* GetCollisionEntity() const;

    virtual void SetMaterial(CMaterial* mtl);
    virtual CMaterial* GetRenderMaterial() const;
    virtual void OnMaterialChanged(MaterialChangeFlags change);

    //////////////////////////////////////////////////////////////////////////
    virtual void SetMinSpec(uint32 nSpec, bool bSetChildren = true);
    virtual void SetMaterialLayersMask(uint32 nLayersMask);

    // Gets matrix of parent attachment point
    virtual Matrix34 GetParentAttachPointWorldTM() const override;

    // Checks if the attachment point is valid
    virtual bool IsParentAttachmentValid() const override;

    // Set attach flags and target
    enum EAttachmentType
    {
        eAT_Pivot,
        eAT_GeomCacheNode,
        eAT_CharacterBone,
    };

    void SetAttachType(const EAttachmentType attachmentType) { m_attachmentType = attachmentType; }
    void SetAttachTarget(const char* target) { m_attachmentTarget = target; }
    EAttachmentType GetAttachType() const { return m_attachmentType; }
    QString GetAttachTarget() const { return m_attachmentTarget; }

    // Updates transform if attached
    void UpdateTransform();

    virtual void SetHelperScale(float scale);
    virtual float GetHelperScale();

    virtual void GatherUsedResources(CUsedResources& resources);
    virtual bool IsSimilarObject(CBaseObject* pObject);

    virtual IRenderNode* GetEngineNode() const;
    virtual bool HasMeasurementAxis() const {   return false;   }

    virtual bool IsIsolated() const { return false; }

    //////////////////////////////////////////////////////////////////////////
    // END CBaseObject
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // CEntity interface.
    //////////////////////////////////////////////////////////////////////////
    virtual bool SetClass(const QString& entityClass, bool bForceReload = false, bool bGetScriptProperties = true,
        XmlNodeRef xmlProperties = XmlNodeRef(), XmlNodeRef xmlProperties2 = XmlNodeRef());
    virtual void SpawnEntity();

    void CopyPropertiesToScriptTables();

    virtual void DeleteEntity();
    virtual void UnloadScript();

    QString GetEntityClass() const { return m_entityClass; };
    int GetEntityId() const { return m_entityId; };

    //! Return entity prototype class if present.
    virtual CEntityPrototype* GetPrototype() const { return m_prototype; };

    //! Get EntityScript object associated with this entity.
    CEntityScript* GetScript() { return m_pEntityScript; }

    //! Reload entity script.
    virtual void Reload(bool bReloadScript = false);

    //////////////////////////////////////////////////////////////////////////
    //! Return number of event targets of Script.
    int     GetEventTargetCount() const { return m_eventTargets.size(); };
    CEntityEventTarget& GetEventTarget(int index) { return m_eventTargets[index]; };
    //! Add new event target, returns index of created event target.
    //! Event targets are Always entities.
    int AddEventTarget(CBaseObject* target, const QString& event, const QString& sourceEvent, bool bUpdateScript = true);
    //! Remove existing event target by index.
    void RemoveEventTarget(int index, bool bUpdateScript = true);
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Entity Links.
    //////////////////////////////////////////////////////////////////////////
    //! Return number of event targets of Script.
    int     GetEntityLinkCount() const { return m_links.size(); };
    CEntityLink& GetEntityLink(int index) { return m_links[index]; };
    virtual int AddEntityLink(const QString& name, GUID targetEntityId);
    virtual bool EntityLinkExists(const QString& name, GUID targetEntityId);
    void RenameEntityLink(int index, const QString& newName);
    void RemoveEntityLink(int index);
    void RemoveAllEntityLinks();
    virtual void EntityLinked(const QString& name, GUID targetEntityId){}
    virtual void EntityUnlinked(const QString& name, GUID targetEntityId) {}
    void LoadLink(XmlNodeRef xmlNode, CObjectArchive* pArchive = NULL);
    void SaveLink(XmlNodeRef xmlNode);
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    //! Get Game entity interface.
    virtual IEntity*    GetIEntity() { return m_pEntity; };
    const IEntity*  GetIEntity() const { return m_pEntity; };

    int GetCastShadowMinSpec() const { return mv_castShadowMinSpec; }

    float GetRatioLod() const { return static_cast<float>(mv_ratioLOD); };
    float GetViewDistanceMultiplier() const { return mv_viewDistanceMultiplier; }

    CVarBlock* GetProperties() const { return m_pProperties; };
    CVarBlock* GetProperties2() const { return m_pProperties2; };

    bool IsLight()  const   {   return m_bLight;        }

    void Validate(IErrorReport* report) override;
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    //! Takes position/orientation from the game entity and apply on editor entity.
    void AcceptPhysicsState();
    void ResetPhysicsState();

    bool CreateFlowGraphWithGroupDialog();
    void SetFlowGraph(CFlowGraph* pGraph);
    // Open Flow Graph associated with this entity in the view window.
    void OpenFlowGraph(const QString& groupName);
    void RemoveFlowGraph(bool bInitFlowGraph = true);
    CFlowGraph* GetFlowGraph() const { return m_pFlowGraph; }

    // Find CEntity from game entityId.
    static CEntityObject* FindFromEntityId(EntityId id);
    // Find CEntity from AZ::EntityId, which can also handle legacy game Ids stored as AZ::EntityIds
    static CEntityObject* FindFromEntityId(const AZ::EntityId& id);

    // Retrieve smart object class for this entity if exist.
    QString GetSmartObjectClass() const;

    // Get the name of the light animation node assigned to this, if any.
    QString GetLightAnimation() const;

    static void DeleteUIPanels();

    IVariable* GetLightVariable(const char* name) const;
    IOpticsElementBasePtr GetOpticsElement();
    void SetOpticsElement(IOpticsElementBase* pOptics);
    void ApplyOptics(const QString& opticsFullName, IOpticsElementBasePtr pOptics);
    void SetOpticsName(const QString& opticsFullName);
    bool GetValidFlareName(QString& outFlareName) const;

    void PreInitLightProperty();
    void UpdateLightProperty();
    void OnMenuReloadScripts();
    void UpdatePropertyPanels(const bool bReload = true);

    void EnableReload(bool bEnable)
    {
        m_bEnableReload = bEnable;
    }

    static void StoreUndoEntityLink(CSelectionGroup* pGroup);

    static const char* s_LensFlarePropertyName;
    static const char* s_LensFlareMaterialName;

    // Overridden from CBaseObject.
    void InvalidateTM(int nWhyFlags) override;

    void RegisterListener(IEntityObjectListener* pListener);
    void UnregisterListener(IEntityObjectListener* pListener);

    // IEntityEventListener
    void OnEntityEvent(IEntity* pEntity, SEntityEvent& event) override;
    // ~IEntityEventListener

    CDLight* GetLightProperty() const;

protected:
    template <typename T>
    void SetEntityProperty(const char* name, T value);
    template <typename T>
    T GetEntityProperty(const char* name, T defaultvalue) const;

    void PySetEntityProperty(const char* name, const SPyWrappedProperty& value);
    SPyWrappedProperty PyGetEntityProperty(const char* name) const;

    virtual bool SetEntityScript(CEntityScript* pEntityScript, bool bForceReload = false, bool bGetScriptProperties = true,
        XmlNodeRef xmlProperties = XmlNodeRef(), XmlNodeRef xmlProperties2 = XmlNodeRef());
    virtual void GetScriptProperties(CEntityScript* pEntityScript, XmlNodeRef xmlProperties, XmlNodeRef xmlProperties2);

    virtual bool HitTestEntity(HitContext& hc, bool& bHavePhysics);

    //////////////////////////////////////////////////////////////////////////
    //! Must be called after cloning the object on clone of object.
    //! This will make sure object references are cloned correctly.
    virtual void PostClone(CBaseObject* pFromObject, CObjectCloneContext& ctx);

    //! Draw default object items.
    virtual void DrawDefault(DisplayContext& dc, const QColor& labelColor = QColor(255, 255, 255));
    void DrawTextureIcon(DisplayContext& dc, const Vec3& pos, float alpha);
    void DrawProjectorPyramid(DisplayContext& dc, float dist);
    void DrawProjectorFrustum(DisplayContext& dc, Vec2 size, float dist);

    //! Returns if object is in the camera view.
    virtual bool IsInCameraView(const CCamera& camera);
    //! Returns vis ratio of object in camera
    virtual float GetCameraVisRatio(const CCamera& camera);

    // Do basic intersection tests
    virtual bool IntersectRectBounds(const AABB& bbox);
    virtual bool IntersectRayBounds(const Ray& ray);

    void OnLoadFailed();

    //////////////////////////////////////////////////////////////////////////
    // StatObjEvents
    void OnStatObjReloaded() override;
    void InstallStatObjListeners();
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // CharacterBoundsNotificationBus
    void OnCharacterBoundsReset() override;
    virtual void InstallCharacterBoundsListeners();
    //////////////////////////////////////////////////////////////////////////

    CVarBlock* CloneProperties(CVarBlock* srcProperties);

    //////////////////////////////////////////////////////////////////////////
    //! Callback called when one of entity properties have been modified.
    void OnPropertyChange(IVariable* var);
    virtual void OnPropertyChangeDone(IVariable* var)
    {
        if (var)
        {
            UpdateGroup();
            UpdatePrefab();
        }
    };
    void OnAttributeChange(int attributeIndex);

    //////////////////////////////////////////////////////////////////////////
    virtual void OnEventTargetEvent(CBaseObject* target, int event);
    void ResolveEventTarget(CBaseObject* object, unsigned int index);
    void ReleaseEventTargets();
    void UpdateMaterialInfo();

    // Called when link info changes.
    virtual void UpdateIEntityLinks(bool bCallOnPropertyChange = true);

public:
    CEntityObject();

    static const GUID& GetClassID()
    {
        // {C80F8AEA-90EF-471f-82C7-D14FA80B9203}
        static const GUID guid = {
            0xc80f8aea, 0x90ef, 0x471f, { 0x82, 0xc7, 0xd1, 0x4f, 0xa8, 0xb, 0x92, 0x3 }
        };
        return guid;
    }

protected:
    void DeleteThis() { delete this; };

    //! Recalculate bounding box of entity.
    void CalcBBox();

    //! Force IEntity to the local position/angles/scale.
    void XFormGameEntity();

    //! Sets correct binding for IEntity.
    virtual void BindToParent();
    virtual void BindIEntityChilds();
    virtual void UnbindIEntity();

    void DrawAIInfo(DisplayContext& dc, struct IAIObject* aiObj);

    // Sets common entity params using properties table
    void SetCommonEntityParamsFromProperties();

    //////////////////////////////////////////////////////////////////////////
    // Callbacks.
    //////////////////////////////////////////////////////////////////////////
    void OnRenderFlagsChange(IVariable* var);
    void OnEntityFlagsChange(IVariable* var);
    void OnEntityObstructionMultiplierChange(IVariable* var);
    //////////////////////////////////////////////////////////////////////////
    // Radius callbacks.
    //////////////////////////////////////////////////////////////////////////
    void OnRadiusChange(IVariable* var);
    void OnInnerRadiusChange(IVariable* var);
    void OnOuterRadiusChange(IVariable* var);
    void OnBoxSizeXChange(IVariable* var);
    void OnBoxSizeYChange(IVariable* var);
    void OnBoxSizeZChange(IVariable* var);
    void OnProjectorFOVChange(IVariable* var);
    void OnProjectorTextureChange(IVariable* var);
    void OnProjectInAllDirsChange(IVariable* var);
    //////////////////////////////////////////////////////////////////////////
    // Area light callbacks.
    //////////////////////////////////////////////////////////////////////////
    void OnAreaLightChange(IVariable* var);
    void OnAreaWidthChange(IVariable* var);
    void OnAreaHeightChange(IVariable* var);
    void OnAreaFOVChange(IVariable* var);
    void OnAreaLightSizeChange(IVariable* var);
    void OnColorChange(IVariable* var);
    //////////////////////////////////////////////////////////////////////////
    // Box projection callbacks.
    //////////////////////////////////////////////////////////////////////////
    void OnBoxProjectionChange(IVariable* var);
    void OnBoxWidthChange(IVariable* var);
    void OnBoxHeightChange(IVariable* var);
    void OnBoxLengthChange(IVariable* var);
    //////////////////////////////////////////////////////////////////////////

    void FreeGameData();

    void CheckSpecConfig();

    void AdjustLightProperties(CVarBlockPtr& properties, const char* pSubBlock);
    IVariable* FindVariableInSubBlock(CVarBlockPtr& properties, IVariable* pSubBlockVar, const char* pVarName);

    void OnMenuCreateFlowGraph();
    void OnMenuFlowGraphOpen(CFlowGraph* pFlowGraph);
    void OnMenuScriptEvent(int eventIndex);
    void OnMenuOpenTrackView(CTrackViewAnimNode* pAnimNode);
    void OnMenuReloadAllScripts();
    void OnMenuConvertToPrefab();

    virtual QString GetMouseOverStatisticsText() const override;

    void SetFlareName(const QString& name)
    {
        SetEntityPropertyString(s_LensFlarePropertyName, name);
    }

    unsigned int m_bLoadFailed : 1;
    unsigned int m_bEntityXfromValid : 1;
    unsigned int m_bCalcPhysics : 1;
    unsigned int m_bDisplayBBox : 1;
    unsigned int m_bDisplaySolidBBox : 1;
    unsigned int m_bDisplayAbsoluteRadius : 1;
    unsigned int m_bDisplayArrow : 1;
    unsigned int m_bIconOnTop : 1;
    unsigned int m_bVisible : 1;
    unsigned int m_bLight : 1;
    unsigned int m_bAreaLight : 1;
    unsigned int m_bProjectorHasTexture : 1;
    unsigned int m_bProjectInAllDirs : 1;
    unsigned int m_bBoxProjectedCM : 1;
    unsigned int m_bBBoxSelection : 1;

    Vec3  m_lightColor;

    //! Entity class.
    QString m_entityClass;
    //! Id of spawned entity.
    int m_entityId;

    // Used for light entities
    float m_projectorFOV;

    //  IEntityClass *m_pEntityClass;
    IEntity* m_pEntity;
    IStatObj* m_visualObject;
    AABB m_box;

    // Entity class description.
    _smart_ptr<CEntityScript> m_pEntityScript;

    //////////////////////////////////////////////////////////////////////////
    // Main entity parameters.
    //////////////////////////////////////////////////////////////////////////
    CVariable<bool> mv_outdoor;
    CVariable<bool> mv_castShadow; // Legacy, required for backwards compatibility
    CSmartVariableEnum<int> mv_castShadowMinSpec;
    CVariable<int> mv_ratioLOD;
    CVariable<float> mv_viewDistanceMultiplier;
    CVariable<bool> mv_hiddenInGame; // Entity is hidden in game (on start).
    CVariable<bool> mv_recvWind;
    CVariable<bool> mv_renderNearest;
    CVariable<bool> mv_noDecals;
    CVariable<bool> mv_createdThroughPool;
    CVariable<float> mv_obstructionMultiplier;

    //////////////////////////////////////////////////////////////////////////
    // Temp variables (Not serializable) just to display radii from properties.
    //////////////////////////////////////////////////////////////////////////
    // Used for proximity entities.
    float m_proximityRadius;
    float m_innerRadius;
    float m_outerRadius;
    // Used for probes
    float m_boxSizeX;
    float m_boxSizeY;
    float m_boxSizeZ;
    // Used for area lights
    float m_fAreaWidth;
    float m_fAreaHeight;
    float m_fAreaLightSize;
    // Used for box projected cubemaps
    float m_fBoxWidth;
    float m_fBoxHeight;
    float m_fBoxLength;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Event Targets.
    //////////////////////////////////////////////////////////////////////////
    //! Array of event targets of this Entity.
    typedef std::vector<CEntityEventTarget> EventTargets;
    EventTargets m_eventTargets;

    //////////////////////////////////////////////////////////////////////////
    // Links
    typedef std::vector<CEntityLink> Links;
    Links m_links;

    //! Entity prototype. only used by EntityPrototypeObject.
    TSmartPtr<CEntityPrototype> m_prototype;

    //! Entity properties variables.
    CVarBlockPtr m_pProperties;

    //! Per instance entity properties variables
    CVarBlockPtr m_pProperties2;

    // Physics state, as a string.
    XmlNodeRef m_physicsState;

    //////////////////////////////////////////////////////////////////////////
    // Associated FlowGraph.
    CFlowGraph* m_pFlowGraph;

    static int m_rollupId;
    static class CEntityPanel* m_panel;
    static float m_helperScale;

    CStatObjValidator m_statObjValidator;

    EAttachmentType m_attachmentType;

    // Override IsScalable value from script file.
    bool m_bForceScale;
    bool m_bEnableReload;

    QString m_attachmentTarget;

    static CPanelTreeBrowser* ms_pTreePanel;
    static int ms_treePanelId;

protected:

    virtual void OnAttachChild(CBaseObject* pChild) override;

private:
    void ResetCallbacks();
    void SetVariableCallback(IVariable* pVar, IVariable::OnSetCallback func);
    void ClearCallbacks();

    void ForceVariableUpdate();

    virtual void OnDetachThis() override;

    CListenerSet<IEntityObjectListener*> m_listeners;
    std::vector< std::pair<IVariable*, IVariable::OnSetCallback> > m_callbacks;

    friend SPyWrappedProperty PyGetEntityProperty(const char* entityName, const char* propName);
    friend void PySetEntityProperty(const char* entityName, const char* propName, SPyWrappedProperty value);
    friend SPyWrappedProperty PyGetEntityParam(const char* pObjectName, const char* pVarPath);
    friend void PySetEntityParam(const char* pObjectName, const char* pVarPath, SPyWrappedProperty value);
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_ENTITYOBJECT_H
