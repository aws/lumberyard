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

#include "StdAfx.h"
#include "ObjectManager.h"

#include "../DisplaySettings.h"

#include "TagPoint.h"
#include "TagComment.h"
#include "EntityObject.h"
#include "AIWave.h"
#include "Group.h"
#include "Volume.h"
#include "SoundObject.h"
#include "ShapeObject.h"
#include "ParticleEffectObject.h"
#include "AiPoint.h"
#include "BrushObject.h"
#include "CloudObject.h"
#include "CloudGroup.h"
#include "DecalObject.h"
#include "DistanceCloudObject.h"
#include "GravityVolumeObject.h"
#include "GeomEntity.h"
#include "ActorEntity.h"
#include "SimpleEntity.h"
#include "MiscEntities.h"
#include "Vehicles/VehiclePrototype.h"
#include "Vehicles/VehicleHelperObject.h"
#include "Vehicles/VehiclePart.h"
#include "Vehicles/VehicleSeat.h"
#include "Vehicles/VehicleWeapon.h"
#include "Vehicles/VehicleComp.h"
#include "SmartObjectHelperObject.h"

#include "CameraObject.h"
#include "AIAnchor.h"
#include "AIReinforcementSpot.h"
#include "SmartObject.h"
#include "AreaBox.h"
#include "AreaSphere.h"
#include "WaterShapeObject.h"
#include "VisAreaShapeObject.h"
#include "ProtEntityObject.h"
#include "PrefabObject.h"
#include "Prefabs/PrefabManager.h"
#include "SequenceObject.h"
#include "RopeObject.h"
#include "CharAttachHelper.h"
#include "EnvironmentProbeObject.h"
#include "AICoverSurface.h"
#include "RefPicture.h"

#include "Viewport.h"

#include "GizmoManager.h"
#include "ObjectLayerManager.h"
#include "AxisGizmo.h"

#include "ObjectPhysicsManager.h"

#include "EditMode/ObjectMode.h"

#include "IAgent.h"
#include <IEntitySystem.h>

#include "Geometry/EdMesh.h"

#include "Material/MaterialManager.h"

#include "IAIObject.h"
#include "../EditMode/DeepSelection.h"
#include "Objects/EnvironmentProbeObject.h"
#include "HyperGraph/FlowGraphManager.h"

#include "Util/BoostPythonHelpers.h"
#include "HyperGraph/FlowGraphHelpers.h"

#include "GameEngine.h"

#include <IRemoteCommand.h>

#include "TrackView/TrackViewSequence.h"
#include "TrackView/TrackViewSequenceManager.h"

#include "Util/GuidUtil.h"

#include <AzCore/Debug/Profiler.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>

#include <QMessageBox>

#include <AzToolsFramework/Metrics/LyEditorMetricsBus.h>
#include <LegacyEntityConversion/LegacyEntityConversion.h>

#include "ObjectManagerLegacyUndo.h"

/*!
 *  Class Description used for object templates.
 *  This description filled from Xml template files.
 */
class CXMLObjectClassDesc
    : public CObjectClassDesc
{
public:
    CObjectClassDesc*   superType;
    QString type;
    QString category;
    QString fileSpec;
    GUID guid;

public:
    REFGUID ClassID()
    {
        return guid;
    }
    ObjectType GetObjectType() { return superType->GetObjectType(); };
    QString ClassName() { return type; };
    QString Category() { return category; };
    QObject* CreateQObject() const override { return superType->CreateQObject(); }
    QString GetTextureIcon() { return superType->GetTextureIcon(); };
    QString GetFileSpec()
    {
        if (!fileSpec.isEmpty())
        {
            return fileSpec;
        }
        else
        {
            return superType->GetFileSpec();
        }
    };
    virtual int GameCreationOrder() { return superType->GameCreationOrder(); };
};


//////////////////////////////////////////////////////////////////////////
// CObjectManager implementation.
//////////////////////////////////////////////////////////////////////////
CObjectManager* g_pObjectManager = 0;

//////////////////////////////////////////////////////////////////////////
CObjectManager::CObjectManager()
    : m_lastHideMask(0)
    , m_maxObjectViewDistRatio(0.00001f)
    , m_currSelection(&m_defaultSelection)
    , m_nLastSelCount(0)
    , m_bSelectionChanged(false)
    , m_selectCallback(nullptr)
    , m_currEditObject(nullptr)
    , m_bSingleSelection(false)
    , m_createGameObjects(true)
    , m_bGenUniqObjectNames(true)
    , m_gizmoManager(new CGizmoManager())
    , m_pLayerManager(new CObjectLayerManager(this))
    , m_pLoadProgress(nullptr)
    , m_loadedObjects(0)
    , m_totalObjectsToLoad(0)
    , m_pPhysicsManager(new CObjectPhysicsManager())
    , m_bExiting(false)
    , m_isUpdateVisibilityList(false)
    , m_currentHideCount(CBaseObject::s_invalidHiddenID)
    , m_bInReloading(false)
    , m_bSkipObjectUpdate(false)
    , m_bLevelExporting(false)
{
    g_pObjectManager = this;

    RegisterObjectClasses();

    m_objectsByName.reserve(1024);
    m_converter.reset(aznew AZ::LegacyConversion::Converter());
    LoadRegistry();
}

//////////////////////////////////////////////////////////////////////////
CObjectManager::~CObjectManager()
{
    m_converter.reset();
    m_bExiting = true;
    SaveRegistry();
    DeleteAllObjects();

    delete m_gizmoManager;
    delete m_pLayerManager;
    delete m_pPhysicsManager;
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::RegisterObjectClasses()
{
    // Register default classes.
    CClassFactory* cf = CClassFactory::Instance();
    cf->RegisterClass(new CTemplateObjectClassDesc<CTagPoint>("StdTagPoint", "", "Editor/ObjectIcons/TagPoint.bmp", OBJTYPE_TAGPOINT));
    //cf->RegisterClass( new CTemplateObjectClassDesc<CRespawnPoint>("Respawn", "TagPoint", "", OBJTYPE_TAGPOINT) );
    //cf->RegisterClass( new CTemplateObjectClassDesc<CSpawnPoint>("SpawnPoint", "TagPoint", "", OBJTYPE_TAGPOINT) );
    cf->RegisterClass(new CTemplateObjectClassDesc<CTagComment>("Comment", "Misc", "", OBJTYPE_TAGPOINT));
    //cf->RegisterClass( new CStaticObjectClassDesc );
    //cf->RegisterClass( new CBuildingClassDesc );
    cf->RegisterClass(new CTemplateObjectClassDesc<CEntityObject>("StdEntity", "", "", OBJTYPE_ENTITY, 200, "*EntityClass"));
    cf->RegisterClass(new CTemplateObjectClassDesc<CSimpleEntity>("SimpleEntity", "", "", OBJTYPE_ENTITY, 202, "*.cgf;*.chr;*.cga;*.cdf"));
    cf->RegisterClass(new CTemplateObjectClassDesc<CGeomEntity>("GeomEntity", "Geom Entity", "", OBJTYPE_ENTITY, 201, "*.cgf;*.chr;*.cga;*.cdf"));
    cf->RegisterClass(new CTemplateObjectClassDesc<CActorEntity>("ActorEntity", "Actor Entity", "", OBJTYPE_ENTITY, 201, "*.prototype"));
    cf->RegisterClass(new CTemplateObjectClassDesc<CParticleEffectObject>("ParticleEntity", "Particle Entity", "", OBJTYPE_ENTITY, 200, "*ParticleEffects"));
    cf->RegisterClass(new CTemplateObjectClassDesc<CGroup>("Group", "", "", OBJTYPE_GROUP));
    cf->RegisterClass(new CTemplateObjectClassDesc<CVolume>("StdVolume", "", "", OBJTYPE_VOLUME, 15));
    cf->RegisterClass(new CTemplateObjectClassDesc<CSoundObject>("StdSoundObject", "", "", OBJTYPE_TAGPOINT));
    cf->RegisterClass(new CTemplateObjectClassDesc<CShapeObject>("Shape", "Area", "", OBJTYPE_SHAPE, 50));
    cf->RegisterClass(new CTemplateObjectClassDesc<CAIPathObject>("AIPath", "AI", "", OBJTYPE_SHAPE, 50));
    cf->RegisterClass(new CTemplateObjectClassDesc<CAIShapeObject>("AIShape", "AI", "", OBJTYPE_SHAPE, 50));

    cf->RegisterClass(new CTemplateObjectClassDesc<CAIOcclusionPlaneObject>("AIHorizontalOcclusionPlane", "AI", "", OBJTYPE_SHAPE, 50));
    cf->RegisterClass(new CTemplateObjectClassDesc<CAIPerceptionModifierObject>("AIPerceptionModifier", "AI", "", OBJTYPE_SHAPE, 50));
    cf->RegisterClass(new CTemplateObjectClassDesc<CAITerritoryObject>("Entity::AITerritory", "", "", OBJTYPE_SHAPE, 50));
    cf->RegisterClass(new CTemplateObjectClassDesc<CAIWaveObject>("Entity::AIWave", "", "", OBJTYPE_ENTITY, 51));
    cf->RegisterClass(new CTemplateObjectClassDesc<CAIPoint>("AIPoint", "AI", "", OBJTYPE_AIPOINT, 110));
    cf->RegisterClass(new CTemplateObjectClassDesc<CAIAnchor>("AIAnchor", "AI", "", OBJTYPE_AIPOINT, 111));
    cf->RegisterClass(new CTemplateObjectClassDesc<CAIReinforcementSpot>("AIReinforcementSpot", "AI", "", OBJTYPE_AIPOINT, 111));

    cf->RegisterClass(new CTemplateObjectClassDesc<CAICoverSurface>("CoverSurface", "AI", "", OBJTYPE_AIPOINT, 151));
    cf->RegisterClass(new CTemplateObjectClassDesc<CSmartObject>("SmartObject", "AI", "", OBJTYPE_AIPOINT, 111));
    cf->RegisterClass(new CTemplateObjectClassDesc<CGameShapeObject>("GameVolume", "Custom", "", OBJTYPE_SHAPE, 50));
    cf->RegisterClass(new CTemplateObjectClassDesc<CGameShapeLedgeObject>("Ledge", "Custom", "", OBJTYPE_SHAPE, 50));
    cf->RegisterClass(new CTemplateObjectClassDesc<CGameShapeLedgeStaticObject>("LedgeStatic", "Custom", "", OBJTYPE_SHAPE, 50));
    cf->RegisterClass(new CTemplateObjectClassDesc<CNavigationAreaObject>("NavigationArea", "AI", "", OBJTYPE_SHAPE, 50));
    //cf->RegisterClass( new CAIStreamingAreaObjectClassDesc );
    cf->RegisterClass(new CTemplateObjectClassDesc<CBrushObject>("Brush", "Brush", "", OBJTYPE_BRUSH, 150, "Objects/*.cgf"));
    cf->RegisterClass(new CTemplateObjectClassDesc<CCameraObject>("Camera", "Misc", "", OBJTYPE_ENTITY, 202));
    cf->RegisterClass(new CTemplateObjectClassDesc<CCameraObjectTarget>("CameraTarget", "", "", OBJTYPE_ENTITY, 202));
    cf->RegisterClass(new CTemplateObjectClassDesc<CAreaBox>("AreaBox", "Area", "", OBJTYPE_VOLUME, 52));
    cf->RegisterClass(new CTemplateObjectClassDesc<CAreaSphere>("AreaSphere", "Area", "", OBJTYPE_VOLUME, 51));
    cf->RegisterClass(new CTemplateObjectClassDesc<CWaterShapeObject>("WaterVolume", "Area", "", OBJTYPE_VOLUME, 16));
    cf->RegisterClass(new CTemplateObjectClassDesc<CVisAreaShapeObject>("VisArea", "Area", "", OBJTYPE_VOLUME, 10));
    cf->RegisterClass(new CTemplateObjectClassDesc<CPortalShapeObject>("Portal", "Area", "", OBJTYPE_VOLUME, 11));
    cf->RegisterClass(new CTemplateObjectClassDesc<COccluderPlaneObject>("OccluderPlane", "Area", "", OBJTYPE_VOLUME, 12));
    cf->RegisterClass(new CTemplateObjectClassDesc<COccluderAreaObject>("OccluderArea", "Area", "", OBJTYPE_VOLUME, 12));
    cf->RegisterClass(new CTemplateObjectClassDesc<CProtEntityObject>("EntityArchetype", "Archetype Entity", "", OBJTYPE_ENTITY, 205, "*EntityArchetype"));
    cf->RegisterClass(new CTemplateObjectClassDesc<CPrefabObject>(PREFAB_OBJECT_CLASS_NAME, CATEGORY_PREFABS, "Editor/ObjectIcons/prefab.bmp", OBJTYPE_PREFAB, 210, "*Prefabs"));
    cf->RegisterClass(new CTemplateObjectClassDesc<CCloudObject>("CloudVolume", "", "", OBJTYPE_CLOUD, 150));
    cf->RegisterClass(new CTemplateObjectClassDesc<CCloudGroup>("Cloud", "", "", OBJTYPE_CLOUD));
    cf->RegisterClass(new CTemplateObjectClassDesc<CVehiclePrototype>("VehiclePrototype", "", "", OBJTYPE_OTHER, 100, "Scripts/Entities/Vehicles/Implementations/Xml/*.xml"));
    cf->RegisterClass(new CTemplateObjectClassDesc<CVehicleHelper>("VehicleHelper", "", "", OBJTYPE_OTHER));
    cf->RegisterClass(new CTemplateObjectClassDesc<CVehiclePart>("VehiclePart", "", "", OBJTYPE_OTHER));
    cf->RegisterClass(new CTemplateObjectClassDesc<CVehicleSeat>("VehicleSeat", "", "", OBJTYPE_OTHER));
    cf->RegisterClass(new CTemplateObjectClassDesc<CVehicleWeapon>("VehicleWeapon", "", "", OBJTYPE_OTHER));
    cf->RegisterClass(new CTemplateObjectClassDesc<CVehicleComponent>("VehicleComponent", "", "", OBJTYPE_OTHER));
    cf->RegisterClass(new CTemplateObjectClassDesc<CSmartObjectHelperObject>("SmartObjectHelper", "", "", OBJTYPE_OTHER));
    cf->RegisterClass(new CTemplateObjectClassDesc<CDecalObject>("Decal", "Misc", "Editor/ObjectIcons/Decal.bmp", OBJTYPE_DECAL, 150));
    cf->RegisterClass(new CTemplateObjectClassDesc<CSequenceObject>("SequenceObject", "", "Editor/ObjectIcons/sequence.bmp", OBJTYPE_OTHER, 950));
    cf->RegisterClass(new CTemplateObjectClassDesc<CGravityVolumeObject>("GravityVolume", "Misc", "", OBJTYPE_OTHER, 50));
    cf->RegisterClass(new CTemplateObjectClassDesc<CDistanceCloudObject>("DistanceCloud", "Misc", "Editor/ObjectIcons/Clouds.bmp", OBJTYPE_DISTANCECLOUD, 200));
    cf->RegisterClass(new CTemplateObjectClassDesc<CRopeObject>("Rope", "Misc", "Editor/ObjectIcons/rope.bmp", OBJTYPE_OTHER, 300));
    cf->RegisterClass(new CTemplateObjectClassDesc<CCharacterAttachHelperObject>("CharAttachHelper", "Misc", "Editor/ObjectIcons/Magnet.bmp", OBJTYPE_ENTITY, 200));
    cf->RegisterClass(new CTemplateObjectClassDesc<CEnvironementProbeObject>("EnvironmentProbe", "Misc", "Editor/ObjectIcons/environmentProbe.bmp", OBJTYPE_ENTITY, 202));
    //  cf->RegisterClass( new CLightClassDesc );
    cf->RegisterClass(new CTemplateObjectClassDesc<CRefPicture>("ReferencePicture", "Misc", "", OBJTYPE_REFPICTURE));
    cf->RegisterClass(new CTemplateObjectClassDesc<CConstraintEntity>("Entity::Constraint", "", "", OBJTYPE_ENTITY, 203, "*.cgf;*.chr;*.cga;*.cdf"));
    cf->RegisterClass(new CTemplateObjectClassDesc<CWindAreaEntity>("Entity::WindArea", "", "", OBJTYPE_ENTITY, 203, "*.cgf;*.chr;*.cga;*.cdf"));
    cf->RegisterClass(new CTemplateObjectClassDesc<CNavigationSeedPoint>("NavigationSeedPoint", "AI", "", OBJTYPE_TAGPOINT));
    //  cf->RegisterClass( new CTemplateObjectClassDesc<CEnvironementProbeTODObject>("EnvironmentProbeTOD", "Misc", "Editor/ObjectIcons/environmentProbe.bmp", OBJTYPE_ENTITY, 202) );
#if defined(USE_GEOM_CACHES)
    cf->RegisterClass(new CTemplateObjectClassDesc<CGeomCacheEntity>("Entity::GeomCache", "", "", OBJTYPE_GEOMCACHE, 204, "*.cax"));
#endif

    LoadRegistry();
}

//////////////////////////////////////////////////////////////////////////
void    CObjectManager::SaveRegistry()
{
}

void    CObjectManager::LoadRegistry()
{
}

//////////////////////////////////////////////////////////////////////////
CBaseObject* CObjectManager::NewObject(CObjectClassDesc* cls, CBaseObject* prev, const QString& file, const char* newObjectName)
{
    // Suspend undo operations when initializing object.
    GetIEditor()->SuspendUndo();

    CBaseObjectPtr obj;
    {
        obj = qobject_cast<CBaseObject*>(cls->CreateQObject());
        obj->SetClassDesc(cls);
        CObjectLayer* destLayer = obj->SupportsLayers() ? m_pLayerManager->GetCurrentLayer() : m_pLayerManager->FindLayerByName("Main");
        obj->SetLayer(destLayer);
        obj->InitVariables();
        obj->m_guid = QUuid::createUuid();    // generate uniq GUID for this object.

        GetIEditor()->GetErrorReport()->SetCurrentValidatorObject(obj);
        if (obj->Init(GetIEditor(), prev, file))
        {
            if ((newObjectName)&&(newObjectName[0]))
            {
                obj->SetName(newObjectName);
            }
            else
            {
                if (obj->GetName().isEmpty())
                {
                    obj->GenerateUniqueName();
                }
            }

            // Create game object itself.
            obj->CreateGameObject();

            if (!AddObject(obj))
            {
                obj = 0;
            }
        }
        else
        {
            obj = 0;
        }
        GetIEditor()->GetErrorReport()->SetCurrentValidatorObject(NULL);
    }

    GetIEditor()->ResumeUndo();

    if (obj != 0 && GetIEditor()->IsUndoRecording())
    {
        // AZ entity creations are handled through the AZ undo system.
        if (obj->GetType() != OBJTYPE_AZENTITY)
        {
            GetIEditor()->RecordUndo(new CUndoBaseObjectNew(obj));

            // check for script entities
            const char* scriptClassName = "";
            CEntityObject* entityObj = qobject_cast<CEntityObject*>(obj);
            QByteArray entityClass; // Leave it outside of the if. Otherwise buffer is deleted.
            if (entityObj)
            {
                entityClass = entityObj->GetEntityClass().toUtf8();
                scriptClassName = entityClass.data();
            }

            using namespace AzToolsFramework;
            EditorMetricsEventsBus::Broadcast(&EditorMetricsEventsBus::Events::LegacyEntityCreated, cls->ClassName().toUtf8().data(), scriptClassName);
        }
    }

    return obj;
}

//////////////////////////////////////////////////////////////////////////
CBaseObject* CObjectManager::NewObject(CObjectArchive& ar, CBaseObject* pUndoObject, bool bMakeNewId)
{
    XmlNodeRef objNode = ar.node;

    // Load all objects from XML.
    QString typeName;
    GUID id = GUID_NULL;
    GUID idInPrefab = GUID_NULL;

    if (!objNode->getAttr("Type", typeName))
    {
        return 0;
    }

    if (!objNode->getAttr("Id", id))
    {
        // Make new ID for object that doesn't have if.
        id = QUuid::createUuid();
    }

    idInPrefab = id;

    if (bMakeNewId)
    {
        // Make new guid for this object.
        GUID newId = QUuid::createUuid();
        ar.RemapID(id, newId);  // Mark this id remapped.
        id = newId;
    }

    CBaseObjectPtr pObject;
    if (pUndoObject)
    {
        // if undoing restore object pointer.
        pObject = pUndoObject;
    }
    else
    {
        // New object creation.

        // Suspend undo operations when initializing object.
        CUndoSuspend undoSuspender;

        QString entityClass;
        if (objNode->getAttr("EntityClass", entityClass))
        {
            typeName = typeName + "::" + entityClass;
        }

        CObjectClassDesc* cls = FindClass(typeName);
        if (!cls)
        {
            CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "RuntimeClass %s not registered", typeName.toUtf8().data());
            return 0;
        }

        pObject = qobject_cast<CBaseObject*>(cls->CreateQObject());
        assert(pObject);
        pObject->SetClassDesc(cls);
        pObject->m_guid = id;

        CObjectLayer* destLayer = pObject->SupportsLayers() ? m_pLayerManager->GetCurrentLayer() : m_pLayerManager->FindLayerByName("Main");

        pObject->SetLayer(destLayer);
        pObject->InitVariables();

        QString objName;
        objNode->getAttr("Name", objName);
        pObject->m_name = objName;
        pObject->SetIdInPrefab(idInPrefab);

        CBaseObject* obj = FindObject(pObject->GetId());
        if (obj)
        {
            QString layerName;
            if (obj->GetLayer())
            {
                layerName = " [" + obj->GetLayer()->GetName() + "]";
            }

            // If id is taken.
            QString error;
            error = QObject::tr("[Error] Object %1 already exists in the Object Manager and has been deleted as it is a duplicate of object %2 in layer %3.").arg(pObject->m_name, obj->GetName(), layerName);
            CLogFile::WriteLine(error.toUtf8().data());

            if (!GetIEditor()->IsInTestMode() && !GetIEditor()->IsInLevelLoadTestMode())
            {
                CErrorRecord errorRecord;
                errorRecord.pObject = obj;
                errorRecord.count = 1;
                errorRecord.severity = CErrorRecord::ESEVERITY_ERROR;
                errorRecord.error = error;
                errorRecord.description = "Possible duplicate objects being loaded, potential fix is to remove duplicate objects from level files.";
                GetIEditor()->GetErrorReport()->ReportError(errorRecord);
            }

            return 0;
            //CoCreateGuid( &pObject->m_guid ); // generate uniq GUID for this object.
        }
    }

    GetIEditor()->GetErrorReport()->SetCurrentValidatorObject(pObject);
    if (!pObject->Init(GetIEditor(), 0, ""))
    {
        GetIEditor()->GetErrorReport()->SetCurrentValidatorObject(NULL);
        return 0;
    }

    if (!AddObject(pObject))
    {
        GetIEditor()->GetErrorReport()->SetCurrentValidatorObject(NULL);
        return 0;
    }

    //pObject->Serialize( ar );

    GetIEditor()->GetErrorReport()->SetCurrentValidatorObject(NULL);

    assert(pObject->GetLayer());

    if (pObject != 0 && pUndoObject == 0)
    {
        // If new object with no undo, record it.
        if (CUndo::IsRecording())
        {
            GetIEditor()->RecordUndo(new CUndoBaseObjectNew(pObject));
        }
    }

    m_loadedObjects++;
    if (m_pLoadProgress && m_totalObjectsToLoad > 0)
    {
        m_pLoadProgress->Step((m_loadedObjects * 100) / m_totalObjectsToLoad);
    }

    return pObject;
}

//////////////////////////////////////////////////////////////////////////
CBaseObject* CObjectManager::NewObject(const QString& typeName, CBaseObject* prev, const QString& file, const char* newObjectName)
{
    // [9/22/2009 evgeny] If it is "Entity", figure out if a CEntity subclass is actually needed
    QString fullName = typeName + "::" + file;
    CObjectClassDesc* cls = FindClass(fullName);
    if (!cls)
    {
        cls = FindClass(typeName);
    }

    if (!cls)
    {
        GetIEditor()->GetSystem()->GetILog()->Log("Warning: RuntimeClass %s (as well as %s) not registered", typeName.toUtf8().data(), fullName.toUtf8().data());
        return 0;
    }
    CBaseObject* pObject = NewObject(cls, prev, file, newObjectName);
    return pObject;
}

//////////////////////////////////////////////////////////////////////////
void    CObjectManager::DeleteObject(CBaseObject* obj)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);
    if (m_currEditObject == obj)
    {
        EndEditParams();
    }

    if (!obj)
    {
        return;
    }

    // If object already deleted.
    if (obj->CheckFlags(OBJFLAG_DELETED))
    {
        return;
    }

    NotifyObjectListeners(obj, CBaseObject::ON_PREDELETE);
    obj->NotifyListeners(CBaseObject::ON_PREDELETE);

    // Check if object is a group then delete all childs.
    if (qobject_cast<CGroup*>(obj))
    {
        ((CGroup*)obj)->DeleteAllMembers();
    }
    else if (qobject_cast<CAITerritoryObject*>(obj))
    {
        FindAndRenameProperty2("aiterritory_Territory", obj->GetName(), "<None>");
    }
    else if (qobject_cast<CAIWaveObject*>(obj))
    {
        FindAndRenameProperty2("aiwave_Wave", obj->GetName(), "<None>");
    }

    // Must be after object DetachAll to support restoring Parent/Child relations.
    // AZ entity deletions are handled through the AZ undo system.
    if (CUndo::IsRecording() && obj->GetType() != OBJTYPE_AZENTITY)
    {
        // Store undo for all child objects.
        for (int i = 0; i < obj->GetChildCount(); i++)
        {
            obj->GetChild(i)->StoreUndo("DeleteParent");
        }
        CUndo::Record(new CUndoBaseObjectDelete(obj));
    }

    OnObjectModified(obj, true, false);

    AABB objAAB;
    obj->GetBoundBox(objAAB);
    GetIEditor()->GetGameEngine()->OnAreaModified(objAAB);

    // Release game resources.
    CBaseObject* pParent = obj->GetParent();

    obj->Done();
    if (pParent)
    {
        if (qobject_cast<CGroup*>(pParent) && !qobject_cast<CPrefabObject*>(pParent))
        {
            ((CGroup*)pParent)->Sync();
            AABB aabb;
            ((CGroup*)pParent)->GetBoundBox(aabb);
            ((CGroup*)pParent)->UpdatePivot(aabb.min);
        }
        else
        {
            pParent->UpdateGroup();
        }
    }

    NotifyObjectListeners(obj, CBaseObject::ON_DELETE);

    RemoveObject(obj);

    RefreshEntitiesAssignedToSelectedTnW();
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::DeleteSelection(CSelectionGroup* pSelection)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);
    if (pSelection == NULL)
    {
        return;
    }

    // if the object contains an entity which has link, the link information should be recorded for undo separately. p

    if (CUndo::IsRecording())
    {
        for (int i = 0, iSize(pSelection->GetCount()); i < iSize; ++i)
        {
            CBaseObject* pObj = pSelection->GetObject(i);
            if (!qobject_cast<CEntityObject*>(pObj))
            {
                continue;
            }

            CEntityObject* pEntity = (CEntityObject*)pObj;
            if (pEntity->GetEntityLinkCount() <= 0)
            {
                continue;
            }

            CEntityObject::StoreUndoEntityLink(pSelection);
            break;
        }
    }

    AzToolsFramework::EntityIdList selectedComponentEntities;
    for (int i = 0, iObjSize(pSelection->GetCount()); i < iObjSize; i++)
    {
        CBaseObject* object = pSelection->GetObject(i);

        // AZ::Entity deletion is handled through AZ undo system (DeleteSelected bus call below).
        if (object->GetType() != OBJTYPE_AZENTITY)
        {
            DeleteObject(object);
        }
        else
        {
            AZ::EntityId id;
            EBUS_EVENT_ID_RESULT(id, object, AzToolsFramework::ComponentEntityObjectRequestBus, GetAssociatedEntityId);
            if (id.IsValid())
            {
                selectedComponentEntities.push_back(id);
            }
        }
    }

    // Delete AZ (component) entities.
    if (QApplication::keyboardModifiers() & Qt::ShiftModifier)
    {
        EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, DeleteEntities, selectedComponentEntities);
    }
    else
    {
        EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, DeleteEntitiesAndAllDescendants, selectedComponentEntities);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::DeleteAllObjects()
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);
    GetIEditor()->GetPrefabManager()->SetSkipPrefabUpdate(true);

    EndEditParams();

    ClearSelection();
    int i;

    InvalidateVisibleList();

    // Delete all selection groups.
    std::vector<CSelectionGroup*> sel;
    stl::map_to_vector(m_selections, sel);
    for (i = 0; i < sel.size(); i++)
    {
        delete sel[i];
    }
    m_selections.clear();

    TBaseObjects objectsHolder;
    GetAllObjects(objectsHolder);

    // Clear map. Need to do this before deleting objects in case someone tries to get object list during shutdown.
    m_objects.clear();
    m_objectsByName.clear();

    for (i = 0; i < objectsHolder.size(); i++)
    {
        objectsHolder[i]->Done();
    }

    //! Delete object instances.
    objectsHolder.clear();

    // Clear name map.
    m_nameNumbersMap.clear();

    m_aiTerritoryObjects.clear();
    m_aiWaveObjects.clear();
    m_animatedAttachedEntities.clear();

    RefreshEntitiesAssignedToSelectedTnW();

    GetIEditor()->GetPrefabManager()->SetSkipPrefabUpdate(false);
}

CBaseObject* CObjectManager::CloneObject(CBaseObject* obj)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);
    assert(obj);
    //CRuntimeClass *cls = obj->GetRuntimeClass();
    //CBaseObject *clone = (CBaseObject*)cls->CreateObject();
    //clone->CloneCopy( obj );
    CBaseObject* clone = NewObject(obj->GetClassDesc(), obj);
    return clone;
}

//////////////////////////////////////////////////////////////////////////
CBaseObject* CObjectManager::FindObject(REFGUID guid) const
{
    CBaseObject* result = stl::find_in_map(m_objects, guid, (CBaseObject*)0);
    return result;
}

//////////////////////////////////////////////////////////////////////////
CBaseObject* CObjectManager::FindObject(const QString& sName) const
{
    const AZ::Crc32 crc(sName.toUtf8().data(), sName.toUtf8().count(), true);

    auto iter = m_objectsByName.find(crc);
    if (iter != m_objectsByName.end())
    {
        return iter->second;
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::FindObjectsOfType(ObjectType type, std::vector<CBaseObject*>& result)
{
    result.clear();

    CBaseObjectsArray objects;
    GetObjects(objects);

    for (size_t i = 0, n = objects.size(); i < n; ++i)
    {
        if (objects[i]->GetType() == type)
        {
            result.push_back(objects[i]);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::FindObjectsOfType(const QMetaObject* pClass, std::vector<CBaseObject*>& result)
{
    result.clear();

    CBaseObjectsArray objects;
    GetObjects(objects);

    for (size_t i = 0, n = objects.size(); i < n; ++i)
    {
        CBaseObject* pObject = objects[i];
        if (pObject->metaObject() == pClass)
        {
            result.push_back(pObject);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::FindObjectsInAABB(const AABB& aabb, std::vector<CBaseObject*>& result) const
{
    result.clear();

    CBaseObjectsArray objects;
    GetObjects(objects);

    for (size_t i = 0, n = objects.size(); i < n; ++i)
    {
        CBaseObject* pObject = objects[i];
        AABB aabbObj;
        pObject->GetBoundBox(aabbObj);
        if (aabb.IsIntersectBox(aabbObj))
        {
            result.push_back(pObject);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CObjectManager::AddObject(CBaseObject* obj)
{
    CBaseObjectPtr p = stl::find_in_map(m_objects, obj->GetId(), 0);
    if (p)
    {
        CErrorRecord err;
        err.error = QObject::tr("New Object %1 has Duplicate GUID %2, New Object Ignored").arg(obj->GetName()).arg(GuidUtil::ToString(obj->GetId()));
        err.severity = CErrorRecord::ESEVERITY_ERROR;
        err.pObject = obj;
        err.flags = CErrorRecord::FLAG_OBJECTID;
        GetIEditor()->GetErrorReport()->ReportError(err);

        return false;
    }
    m_objects[obj->GetId()] = obj;

    // Handle adding object to type-specific containers if needed
    {
        if (CAITerritoryObject* territoryObj = qobject_cast<CAITerritoryObject*>(obj))
        {
            m_aiTerritoryObjects.insert(territoryObj);
        }
        else if (CAIWaveObject* waveObj = qobject_cast<CAIWaveObject*>(obj))
        {
            m_aiWaveObjects.insert(waveObj);
        }
        else if (CEntityObject* entityObj = qobject_cast<CEntityObject*>(obj))
        {
            CEntityObject::EAttachmentType attachType = entityObj->GetAttachType();
            if (attachType == CEntityObject::EAttachmentType::eAT_GeomCacheNode || attachType == CEntityObject::EAttachmentType::eAT_CharacterBone)
            {
                m_animatedAttachedEntities.insert(entityObj);
            }
        }
    }

    const AZ::Crc32 nameCrc(obj->GetName().toUtf8().data(), obj->GetName().toUtf8().count(), true);
    m_objectsByName[nameCrc] = obj;

    RegisterObjectName(obj->GetName());
    InvalidateVisibleList();
    NotifyObjectListeners(obj, CBaseObject::ON_ADD);
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::RemoveObject(CBaseObject* obj)
{
    AzToolsFramework::EditorMetricsEventBusSelectionChangeHelper selectionChangeMetricsHelper;

    assert(obj != 0);

    InvalidateVisibleList();

    // Handle removing object from type-specific containers if needed
    {
        if (CAITerritoryObject* territoryObj = qobject_cast<CAITerritoryObject*>(obj))
        {
            m_aiTerritoryObjects.erase(territoryObj);
        }
        else if (CAIWaveObject* waveObj = qobject_cast<CAIWaveObject*>(obj))
        {
            m_aiWaveObjects.erase(waveObj);
        }
        else if (CEntityObject* entityObj = qobject_cast<CEntityObject*>(obj))
        {
            m_animatedAttachedEntities.erase(entityObj);
        }
    }

    // Remove this object from selection groups.
    m_currSelection->RemoveObject(obj);
    std::vector<CSelectionGroup*> sel;
    stl::map_to_vector(m_selections, sel);
    for (int i = 0; i < sel.size(); i++)
    {
        sel[i]->RemoveObject(obj);
    }

    m_objectsByName.erase(AZ::Crc32(obj->GetName().toUtf8().data(), obj->GetName().toUtf8().count(), true));

    // Need to erase this last since it is a smart pointer and can end up deleting the object if it is the last reference to it being kept
    m_objects.erase(obj->GetId());
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::GetAllObjects(TBaseObjects& objects) const
{
    objects.clear();
    objects.reserve(m_objects.size());
    for (Objects::const_iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        objects.push_back(it->second);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::ChangeObjectId(REFGUID oldGuid, REFGUID newGuid)
{
    Objects::iterator it = m_objects.find(oldGuid);
    if (it != m_objects.end())
    {
        CBaseObjectPtr pRemappedObject = (*it).second;
        pRemappedObject->SetId(newGuid);
        m_objects.erase(it);
        m_objects.insert(std::make_pair(newGuid, pRemappedObject));
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::NotifyPrefabObjectChanged(CBaseObject* pObject)
{
    NotifyObjectListeners(pObject, CBaseObject::ON_PREFAB_CHANGED);
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::ShowDuplicationMsgWarning(CBaseObject* obj, const QString& newName, bool bShowMsgBox) const
{
    CBaseObject* pExisting = FindObject(newName);
    if (pExisting)
    {
        QString sRenameWarning = QObject::tr("%1 \"%2\" was NOT renamed to \"%3\" because %4 with the same name already exists.")
            .arg(obj->GetClassDesc()->ClassName())
            .arg(obj->GetName())
            .arg(newName)
            .arg(pExisting->GetClassDesc()->ClassName()
        );

        // If id is taken.
        CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, sRenameWarning.toUtf8().data());

        if (bShowMsgBox)
        {
            QMessageBox::critical(QApplication::activeWindow(), QString(), sRenameWarning);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::ChangeObjectName(CBaseObject* obj, const QString& newName)
{
    assert(obj);

    if (newName != obj->GetName())
    {
        if (IsDuplicateObjectName(newName))
        {
            return;
        }

        // Remove previous name from map
        const AZ::Crc32 oldNameCrc(obj->GetName().toUtf8().data(), obj->GetName().count(), true);
        m_objectsByName.erase(oldNameCrc);

        obj->SetName(newName);

        // Make sure object name edit field is updated in the object properties panel.
        obj->UpdateEditParams();

        // Add new name to map
        const AZ::Crc32 nameCrc(newName.toUtf8().data(), newName.count(), true);
        m_objectsByName[nameCrc] = obj;
    }
}

//////////////////////////////////////////////////////////////////////////
int CObjectManager::GetObjectCount() const
{
    return m_objects.size();
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::GetObjects(CBaseObjectsArray& objects, const CObjectLayer* layer) const
{
    objects.clear();
    objects.reserve(m_objects.size());
    for (Objects::const_iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        if (layer == 0 || it->second->GetLayer() == layer)
        {
            objects.push_back(it->second);
        }
    }
}

void CObjectManager::GetObjects(DynArray<CBaseObject*>& objects, const CObjectLayer* layer) const
{
    CBaseObjectsArray objectArray;
    GetObjects(objectArray, layer);
    objects.clear();
    for (int i = 0, iCount(objectArray.size()); i < iCount; ++i)
    {
        objects.push_back(objectArray[i]);
    }
}

void CObjectManager::GetObjects(CBaseObjectsArray& objects, BaseObjectFilterFunctor const& filter) const
{
    objects.clear();
    objects.reserve(m_objects.size());
    for (Objects::const_iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        assert(it->second);
        if (filter.first(*it->second, filter.second))
        {
            objects.push_back(it->second);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::GetCameras(std::vector<CCameraObject*>& objects)
{
    objects.clear();
    for (Objects::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        CBaseObject* object = it->second;
        if (qobject_cast<CCameraObject*>(object))
        {
            // Only consider camera sources.
            if (object->IsLookAtTarget())
            {
                continue;
            }
            objects.push_back((CCameraObject*)object);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::SendEvent(ObjectEvent event)
{
    if (event == EVENT_RELOAD_ENTITY)
    {
        m_bInReloading = true;
    }

    if (event == EVENT_INGAME || event == EVENT_OUTOFGAME || event == EVENT_UNLOAD_ENTITY || event == EVENT_RELOAD_ENTITY)
    {
        GetIEditor()->GetPrefabManager()->SetSkipPrefabUpdate(true);
    }

    for (Objects::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        CBaseObject* obj = it->second;
        if (obj->GetGroup() && event != EVENT_PRE_EXPORT)
        {
            continue;
        }
        obj->OnEvent(event);
    }

    if (event == EVENT_INGAME || event == EVENT_OUTOFGAME || event == EVENT_UNLOAD_ENTITY || event == EVENT_RELOAD_ENTITY)
    {
        GetIEditor()->GetPrefabManager()->SetSkipPrefabUpdate(false);
    }

    if (event == EVENT_RELOAD_ENTITY)
    {
        m_bInReloading = false;
        GetIEditor()->Notify(eNotify_OnReloadTrackView);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::SendEvent(ObjectEvent event, const AABB& bounds)
{
    AABB box;
    for (Objects::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        CBaseObject* obj = it->second;
        if (obj->GetGroup())
        {
            continue;
        }
        obj->GetBoundBox(box);
        if (bounds.IsIntersectBox(box))
        {
            obj->OnEvent(event);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::Update()
{
    if (m_bSkipObjectUpdate)
    {
        return;
    }

    QWidget* prevActiveWindow = QApplication::activeWindow();

    CheckAndFixSelection();

    // Restore focus if it changed.
    if (prevActiveWindow && QApplication::activeWindow() != prevActiveWindow)
    {
        prevActiveWindow->setFocus();
    }

    m_pPhysicsManager->Update();

    UpdateAttachedEntities();
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::HideObject(CBaseObject* obj, bool hide)
{
    assert(obj != 0);
    if (hide)
    {
        obj->SetHidden(hide, ++m_currentHideCount);
    }
    else
    {
        obj->SetHidden(false);
    }
    InvalidateVisibleList();
}

void CObjectManager::ShowLastHiddenObject()
{
    uint64 mostRecentID = CBaseObject::s_invalidHiddenID;
    CBaseObject* mostRecentObject = nullptr;
    for (auto it : m_objects)
    {
        CBaseObject* obj = it.second;

        uint64 hiddenID = obj->GetHideOrder();

        if (hiddenID > mostRecentID)
        {
            mostRecentID = hiddenID;
            mostRecentObject = obj;
        }
    }

    if (mostRecentObject != nullptr)
    {
        mostRecentObject->SetHidden(false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::UnhideAll()
{
    for (Objects::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        CBaseObject* obj = it->second;
        obj->SetHidden(false);
    }

    InvalidateVisibleList();
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::FreezeObject(CBaseObject* obj, bool freeze)
{
    assert(obj != 0);
    // Remove object from main object set and put it to hidden set.
    obj->SetFrozen(freeze);
    InvalidateVisibleList();
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::UnfreezeAll()
{
    for (Objects::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        CBaseObject* obj = it->second;
        obj->SetFrozen(false);
    }
    InvalidateVisibleList();
}

//////////////////////////////////////////////////////////////////////////
bool CObjectManager::SelectObject(CBaseObject* obj, bool bUseMask)
{
    assert(obj);
    if (obj == NULL)
    {
        return false;
    }

    // Check if can be selected.
    if (bUseMask && (!(obj->GetType() & gSettings.objectSelectMask)))
    {
        return false;
    }

    if (!obj->IsSelectable())
    {
        return false;
    }

    if (m_selectCallback)
    {
        if (!m_selectCallback->OnSelectObject(obj))
        {
            return true;
        }
    }

    AzToolsFramework::EditorMetricsEventBusSelectionChangeHelper selectionChangeMetricsHelper;

    m_currSelection->AddObject(obj);
    SetObjectSelected(obj, true);

    GetIEditor()->Notify(eNotify_OnSelectionChange);

    return true;
}

void CObjectManager::SelectEntities(std::set<CEntityObject*>& s)
{
    AzToolsFramework::EditorMetricsEventBusSelectionChangeHelper selectionChangeMetricsHelper;

    for (std::set<CEntityObject*>::iterator it = s.begin(), end = s.end(); it != end; ++it)
    {
        SelectObject(*it);
    }
}

void CObjectManager::UnselectObject(CBaseObject* obj)
{
    AzToolsFramework::EditorMetricsEventBusSelectionChangeHelper selectionChangeMetricsHelper;

    SetObjectSelected(obj, false);
    m_currSelection->RemoveObject(obj);
}

CSelectionGroup* CObjectManager::GetSelection(const QString& name) const
{
    CSelectionGroup* selection = stl::find_in_map(m_selections, name, (CSelectionGroup*)0);
    return selection;
}

void CObjectManager::GetNameSelectionStrings(QStringList& names)
{
    for (TNameSelectionMap::iterator it = m_selections.begin(); it != m_selections.end(); ++it)
    {
        names.push_back(it->first);
    }
}

void CObjectManager::NameSelection(const QString& name)
{
    if (m_currSelection->IsEmpty())
    {
        return;
    }

    CSelectionGroup* selection = stl::find_in_map(m_selections, name, (CSelectionGroup*)0);
    if (selection)
    {
        assert(selection != 0);
        // Check if trying to rename itself to the same name.
        if (selection == m_currSelection)
        {
            return;
        }
        m_selections.erase(name);
        delete selection;
    }
    selection = new CSelectionGroup;
    selection->Copy(*m_currSelection);
    selection->SetName(name);
    m_selections[name] = selection;
    m_currSelection = selection;
    m_defaultSelection.RemoveAll();
}

void CObjectManager::SerializeNameSelection(XmlNodeRef& rootNode, bool bLoading)
{
    if (!rootNode)
    {
        return;
    }

    _smart_ptr<CSelectionGroup> tmpGroup(0);

    QString selRootStr("NameSelection");
    QString selNodeStr("NameSelectionNode");
    QString selNodeNameStr("name");
    QString idStr("id");
    QString objAttrStr("obj");

    XmlNodeRef startNode = rootNode->findChild(selRootStr.toUtf8().data());

    if (bLoading)
    {
        m_selections.erase(m_selections.begin(), m_selections.end());

        if (startNode)
        {
            for (int selNodeNo = 0; selNodeNo < startNode->getChildCount(); ++selNodeNo)
            {
                XmlNodeRef selNode = startNode->getChild(selNodeNo);
                tmpGroup = new CSelectionGroup;

                for (int objIDNodeNo = 0; objIDNodeNo < selNode->getChildCount(); ++objIDNodeNo)
                {
                    GUID curID = GUID_NULL;
                    XmlNodeRef idNode = selNode->getChild(objIDNodeNo);
                    if (!idNode->getAttr(idStr.toUtf8().data(), curID))
                    {
                        continue;
                    }

                    if (curID != GUID_NULL)
                    {
                        if (GetIEditor()->GetObjectManager()->FindObject(curID))
                        {
                            tmpGroup->AddObject(GetIEditor()->GetObjectManager()->FindObject(curID));
                        }
                    }
                }

                if (tmpGroup->GetCount() > 0)
                {
                    QString nameStr;
                    if (!selNode->getAttr(selNodeNameStr.toUtf8().data(), nameStr))
                    {
                        continue;
                    }
                    tmpGroup->SetName(nameStr);
                    m_selections[nameStr] = tmpGroup;
                }
            }
        }
    }
    else
    {
        startNode = rootNode->newChild(selRootStr.toUtf8().data());
        CSelectionGroup* objSelection = 0;

        for (TNameSelectionMap::iterator it = m_selections.begin(); it != m_selections.end(); ++it)
        {
            XmlNodeRef selectionNameNode = startNode->newChild(selNodeStr.toUtf8().data());
            selectionNameNode->setAttr(selNodeNameStr.toUtf8().data(), it->first.toUtf8().data());
            objSelection = it->second;

            if (!objSelection)
            {
                continue;
            }

            if (objSelection->GetCount() == 0)
            {
                continue;
            }

            for (int i = 0; i < objSelection->GetCount(); ++i)
            {
                if (objSelection->GetObject(i))
                {
                    XmlNodeRef objNode = selectionNameNode->newChild(objAttrStr.toUtf8().data());
                    objNode->setAttr(idStr.toUtf8().data(), GuidUtil::ToString(objSelection->GetObject(i)->GetId()));
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
int CObjectManager::ClearSelection()
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);

    AzToolsFramework::EditorMetricsEventBusSelectionChangeHelper selectionChangeMetricsHelper;

    // Make sure to unlock selection.
    GetIEditor()->LockSelection(false);

    int numSel = m_currSelection->GetCount();

    // Handle Undo/Redo of Component Entities
    bool isUndoRecording = GetIEditor()->IsUndoRecording();
    if (isUndoRecording)
    {
        m_processingBulkSelect = true;
        GetIEditor()->RecordUndo(new CUndoBaseObjectClearSelection(*m_currSelection));
    }

    // Handle legacy entities separately so the selection group can be cleared safely. 
    // This prevents every AzEntity from being removed one by one from a vector.
    m_currSelection->RemoveAllExceptLegacySet();

    // Kick off Deselect for Legacy Entities
    for (CBaseObjectPtr legacyObject : m_currSelection->GetLegacyObjects())
    {
        if (isUndoRecording && legacyObject->IsSelected())
        {
            GetIEditor()->RecordUndo(new CUndoBaseObjectSelect(legacyObject));
        }

        SetObjectSelected(legacyObject, false);
    }

    // Legacy set is cleared
    m_defaultSelection.RemoveAll();
    m_currSelection = &m_defaultSelection;
    m_bSelectionChanged = true;

    // Unselect all component entities as one bulk operation instead of individually
    AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
        &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, 
        AzToolsFramework::EntityIdList());

    m_processingBulkSelect = false;

    if (!m_bExiting)
    {
        GetIEditor()->Notify(eNotify_OnSelectionChange);
    }

    return numSel;
}

//////////////////////////////////////////////////////////////////////////
int CObjectManager::InvertSelection()
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);

    AzToolsFramework::EditorMetricsEventBusSelectionChangeHelper selectionChangeMetricsHelper;

    int selCount = 0;
    // iterate all objects.
    for (Objects::const_iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        CBaseObject* pObj = it->second;
        if (pObj->IsSelected())
        {
            UnselectObject(pObj);
        }
        else
        {
            if (SelectObject(pObj))
            {
                selCount++;
            }
        }
    }
    return selCount;
}

void CObjectManager::SetSelection(const QString& name)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);
    CSelectionGroup* selection = stl::find_in_map(m_selections, name, (CSelectionGroup*)0);
    if (selection)
    {
        AzToolsFramework::EditorMetricsEventBusSelectionChangeHelper selectionChangeMetricsHelper;

        UnselectCurrent();
        assert(selection != 0);
        m_currSelection = selection;
        SelectCurrent();
    }
}

void CObjectManager::RemoveSelection(const QString& name)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);

    AzToolsFramework::EditorMetricsEventBusSelectionChangeHelper selectionChangeMetricsHelper;

    QString selName = name;
    CSelectionGroup* selection = stl::find_in_map(m_selections, name, (CSelectionGroup*)0);
    if (selection)
    {
        if (selection == m_currSelection)
        {
            UnselectCurrent();
            m_currSelection = &m_defaultSelection;
            m_defaultSelection.RemoveAll();
        }
        delete selection;
        m_selections.erase(selName);
    }
}

//! Checks the state of the current selection and fixes it if necessary - Used when AZ Code modifies the selection
void CObjectManager::CheckAndFixSelection()
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);
    bool bObjectMode = qobject_cast<CObjectMode*>(GetIEditor()->GetEditTool()) != nullptr;

    AzToolsFramework::EditorMetricsEventBusSelectionChangeHelper selectionChangeMetricsHelper;

    if (m_currSelection->GetCount() == 0)
    {
        // Nothing selected.
        EndEditParams();
        if (bObjectMode)
        {
            GetIEditor()->ShowTransformManipulator(false);
        }
    }
    else if (m_currSelection->GetCount() == 1)
    {
        if (!m_bSingleSelection)
        {
            EndEditParams();
        }

        CBaseObject* newSelObject = m_currSelection->GetObject(0);
        // Single object selected.
        if (m_currEditObject != m_currSelection->GetObject(0))
        {
            m_bSelectionChanged = false;
            if (!m_currEditObject || (m_currEditObject->metaObject() != newSelObject->metaObject()))
            {
                // If old object and new objects are of different classes.
                EndEditParams();
            }
            if (GetIEditor()->GetEditTool() && GetIEditor()->GetEditTool()->IsUpdateUIPanel())
            {
                BeginEditParams(newSelObject, OBJECT_EDIT);
            }

            //AfxGetMainWnd()->SetFocus();
        }
    }
    else if (m_currSelection->GetCount() > 1)
    {
        // Multiple objects are selected.
        if (m_bSelectionChanged && bObjectMode)
        {
            m_bSelectionChanged = false;
            m_nLastSelCount = m_currSelection->GetCount();
            EndEditParams();
            bool bAllSameType = m_currSelection->SameObjectType();
            if (bAllSameType && m_currSelection->GetObject(0)->GetType() == OBJTYPE_SOLID)
            {
                m_currEditObject = m_currSelection->GetObject(m_nLastSelCount - 1);
            }
            else
            {
                m_currEditObject = m_currSelection->GetObject(0);
            }
            m_currEditObject->BeginEditMultiSelParams(bAllSameType);
        }
    }
}

void CObjectManager::SelectCurrent()
{
    AzToolsFramework::EditorMetricsEventBusSelectionChangeHelper selectionChangeMetricsHelper;

    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);
    for (int i = 0; i < m_currSelection->GetCount(); i++)
    {
        CBaseObject* obj = m_currSelection->GetObject(i);
        if (GetIEditor()->IsUndoRecording() && !obj->IsSelected())
        {
            GetIEditor()->RecordUndo(new CUndoBaseObjectSelect(obj));
        }

        SetObjectSelected(obj, true);
    }
}

void CObjectManager::UnselectCurrent()
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);
    
    // Make sure to unlock selection.
    GetIEditor()->LockSelection(false);

    // Unselect all component entities as one bulk operation instead of individually
    AzToolsFramework::EntityIdList selectedEntities;
    EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, SetSelectedEntities, selectedEntities);

    for (int i = 0; i < m_currSelection->GetCount(); i++)
    {
        CBaseObject* obj = m_currSelection->GetObject(i);
        if (GetIEditor()->IsUndoRecording() && obj->IsSelected())
        {
            GetIEditor()->RecordUndo(new CUndoBaseObjectSelect(obj));
        }

        SetObjectSelected(obj, false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::Display(DisplayContext& dc)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);

    int currentHideMask = GetIEditor()->GetDisplaySettings()->GetObjectHideMask();
    if (m_lastHideMask != currentHideMask)
    {
        // a setting has changed which may cause the set of currently visible objects to change, so invalidate the serial number
        // so that viewports and anyone else that needs to update settings knows it has to.
        m_lastHideMask = currentHideMask;
        ++m_visibilitySerialNumber;
    }

    // the object manager itself has a visibility list, so it also has to update its cache when the serial has changed
    if (m_visibilitySerialNumber != m_lastComputedVisibility)
    {
        m_lastComputedVisibility = m_visibilitySerialNumber;
        UpdateVisibilityList();
    }
    
    bool viewIsDirty = dc.settings->IsDisplayHelpers(); // displaying helpers require computing all the bound boxes and things anyway.

    if (!viewIsDirty)
    {
        if (CBaseObjectsCache* cache = dc.view->GetVisibleObjectsCache())
        {
            // if the current rendering viewport has an out-of-date cache serial number, it needs to be refreshed too.
            // views set their cache empty when they indicate they need to force a refresh.
            if ((cache->GetObjectCount() == 0) || (cache->GetSerialNumber() != m_visibilitySerialNumber))
            {
                viewIsDirty = true;
            }
        }
    }
    
    if (viewIsDirty)
    {
        FindDisplayableObjects(dc, true);  // this also actually draws the helpers.
    }

    if (m_gizmoManager)
    {
        m_gizmoManager->Display(dc);
    }
}

void CObjectManager::ForceUpdateVisibleObjectCache(DisplayContext& dc)
{
    FindDisplayableObjects(dc, false);
}

void CObjectManager::FindDisplayableObjects(DisplayContext& dc, bool bDisplay)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);

    CBaseObjectsCache* pDispayedViewObjects = dc.view->GetVisibleObjectsCache();
    if (!pDispayedViewObjects)
    {
        return;
    }

    pDispayedViewObjects->SetSerialNumber(m_visibilitySerialNumber); // update viewport to be latest serial number

    const CCamera& camera = GetIEditor()->GetSystem()->GetViewCamera();
    AABB bbox;
    bbox.min.zero();
    bbox.max.zero();

    pDispayedViewObjects->ClearObjects();
    pDispayedViewObjects->Reserve(m_visibleObjects.size());

    CEditTool* pEditTool = GetIEditor()->GetEditTool();

    if (dc.flags & DISPLAY_2D)
    {
        int numVis = m_visibleObjects.size();
        for (int i = 0; i < numVis; i++)
        {
            CBaseObject* obj = m_visibleObjects[i];

            obj->GetBoundBox(bbox);
            if (dc.box.IsIntersectBox(bbox))
            {
                pDispayedViewObjects->AddObject(obj);

                if (bDisplay && dc.settings->IsDisplayHelpers() && (gSettings.viewports.nShowFrozenHelpers || !obj->IsFrozen()))
                {
                    obj->Display(dc);
                    if (pEditTool)
                    {
                        pEditTool->DrawObjectHelpers(obj, dc);
                    }
                }
            }
        }
    }
    else
    {
        CSelectionGroup* pSelection = GetSelection();
        if (pSelection && pSelection->GetCount() > 1)
        {
            AABB mergedAABB;
            mergedAABB.Reset();
            bool bAllSolids = true;
            for (int i = 0, iCount(pSelection->GetCount()); i < iCount; ++i)
            {
                CBaseObject* pObj(pSelection->GetObject(i));
                if (pObj == NULL)
                {
                    continue;
                }
                AABB aabb;
                pObj->GetBoundBox(aabb);
                mergedAABB.Add(aabb);
                if (bAllSolids && pObj->GetType() != OBJTYPE_SOLID)
                {
                    bAllSolids = false;
                }
            }

            if (!bAllSolids)
            {
                pSelection->GetObject(0)->CBaseObject::DrawDimensions(dc, &mergedAABB);
            }
            else
            {
                pSelection->GetObject(0)->DrawDimensions(dc, &mergedAABB);
            }
        }

        int numVis = m_visibleObjects.size();
        for (int i = 0; i < numVis; i++)
        {
            CBaseObject* obj = m_visibleObjects[i];

            if (obj && obj->IsInCameraView(camera))
            {
                // Check if object is too far.
                float visRatio = obj->GetCameraVisRatio(camera);
                if (visRatio > m_maxObjectViewDistRatio || (dc.flags & DISPLAY_SELECTION_HELPERS) || obj->IsSelected())
                {
                    pDispayedViewObjects->AddObject(obj);

                    if (bDisplay && dc.settings->IsDisplayHelpers() && (gSettings.viewports.nShowFrozenHelpers || !obj->IsFrozen()) && !obj->CheckFlags(OBJFLAG_HIDE_HELPERS))
                    {
                        obj->Display(dc);
                        if (pEditTool)
                        {
                            pEditTool->DrawObjectHelpers(obj, dc);
                        }
                    }
                }
            }
        }
    }
}

void CObjectManager::BeginEditParams(CBaseObject* obj, int flags)
{
    assert(obj != 0);
    if (obj == m_currEditObject)
    {
        return;
    }

    if (GetSelection()->GetCount() > 1)
    {
        return;
    }

    QWidget* prevActiveWindow = QApplication::activeWindow();

    if (m_currEditObject)
    {
        //if (obj->GetClassDesc() != m_currEditObject->GetClassDesc())
        if (!obj->IsSameClass(m_currEditObject))
        {
            EndEditParams(flags);
        }
    }

    m_currEditObject = obj;

    if (flags & OBJECT_CREATE)
    {
        // Unselect all other objects.
        ClearSelection();
        // Select this object.
        SelectObject(obj, false);
    }

    m_bSingleSelection = true;
    m_currEditObject->BeginEditParams(GetIEditor(), flags);

    // Restore focus if it changed.
    //  OBJECT_EDIT is used by the EntityOutliner when items are selected. Using it here to prevent shifting focus to the EntityInspector on select.
    if (!(flags & OBJECT_EDIT) && prevActiveWindow && QApplication::activeWindow() != prevActiveWindow)
    {
        prevActiveWindow->setFocus();
    }
}

void CObjectManager::EndEditParams(int flags)
{
    if (m_currEditObject)
    {
        if (m_bSingleSelection)
        {
            m_currEditObject->EndEditParams(GetIEditor());
        }
        else
        {
            m_currEditObject->EndEditMultiSelParams();
        }
    }
    m_bSingleSelection = false;
    m_currEditObject = 0;
    //m_bSelectionChanged = false; // don't need to clear for ungroup
}

//! Select objects within specified distance from given position.
int CObjectManager::SelectObjects(const AABB& box, bool bUnselect)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);
    int numSel = 0;

    AABB objBounds;
    for (Objects::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        CBaseObject* obj = it->second;

        if (obj->IsHidden())
        {
            continue;
        }
        if (obj->IsFrozen())
        {
            continue;
        }

        if (obj->GetGroup())
        {
            continue;
        }

        obj->GetBoundBox(objBounds);
        if (box.IsIntersectBox(objBounds))
        {
            numSel++;
            if (!bUnselect)
            {
                SelectObject(obj);
            }
            else
            {
                UnselectObject(obj);
            }
        }
        // If its group.
        if (qobject_cast<CGroup*>(obj))
        {
            numSel += ((CGroup*)obj)->SelectObjects(box, bUnselect);
        }
    }
    return numSel;
}

//////////////////////////////////////////////////////////////////////////
int CObjectManager::MoveObjects(const AABB& box, const Vec3& offset, ImageRotationDegrees rotation, bool bIsCopy)
{
    AABB objBounds;

    Vec3 src = (box.min + box.max) / 2;
    Vec3 dst = src + offset;
    float alpha = 0.0f;
    switch (rotation)
    {
        case ImageRotationDegrees::Rotate90:
            alpha = gf_halfPI;
            break;
        case ImageRotationDegrees::Rotate180:
            alpha = gf_PI;
            break;
        case ImageRotationDegrees::Rotate270:
            alpha = gf_PI + gf_halfPI;
            break;
        default:
            break;
    }

    float cosa = cos(alpha);
    float sina = sin(alpha);

    for (Objects::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        CBaseObject* obj = it->second;

        if (obj->GetParent())
        {
            continue;
        }

        if (obj->GetGroup())
        {
            continue;
        }

        obj->GetBoundBox(objBounds);
        if (box.IsIntersectBox(objBounds))
        {
            if (rotation == ImageRotationDegrees::Rotate0)
            {
                obj->SetPos(obj->GetPos() - src + dst);
            }
            else
            {
                Vec3 pos = obj->GetPos() - src;
                Vec3 newPos(pos);
                newPos.x = cosa * pos.x - sina * pos.y;
                newPos.y = sina * pos.x + cosa * pos.y;
                obj->SetPos(newPos + dst);
                Quat q;
                obj->SetRotation(q.CreateRotationZ(alpha) * obj->GetRotation());
            }
        }
    }
    return 0;
}

bool CObjectManager::IsObjectDeletionAllowed(CBaseObject* pObject)
{
    if (!pObject)
    {
        return false;
    }

    // Test AI object against AI/Physics activation
    uint32 flags = GetIEditor()->GetDisplaySettings()->GetSettings();
    if ((flags & SETTINGS_PHYSICS) != 0)
    {
        if (qobject_cast<CEntityObject*>(pObject))
        {
            CEntityObject* pEntityObj = (CEntityObject*)pObject;

            if (pEntityObj)
            {
                IEntity* pIEntity = pEntityObj->GetIEntity();
                if (pIEntity)
                {
                    if (pIEntity->HasAI())
                    {
                        QMessageBox::critical(QApplication::activeWindow(), QString(), QObject::tr("AI object %1 cannot be deleted when AI/Physics mode is activated.").arg(pObject->GetName()));
                        return false;
                    }
                }
            }
        }
    }

    return true;
};

//////////////////////////////////////////////////////////////////////////
void CObjectManager::DeleteSelection()
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);

    AzToolsFramework::EditorMetricsEventBusSelectionChangeHelper selectionChangeMetricsHelper;

    // Make sure to unlock selection.
    GetIEditor()->LockSelection(false);

    GUID bID = GUID_NULL;

    int i;
    CSelectionGroup objects;
    for (i = 0; i < m_currSelection->GetCount(); i++)
    {
        // Check condition(s) if object could be deleted
        if (!IsObjectDeletionAllowed(m_currSelection->GetObject(i)))
        {
            return;
        }

        objects.AddObject(m_currSelection->GetObject(i));
    }

    RemoveSelection(m_currSelection->GetName());
    m_currSelection = &m_defaultSelection;
    m_defaultSelection.RemoveAll();

    DeleteSelection(&objects);

    GetIEditor()->GetFlowGraphManager()->SendNotifyEvent(EHG_GRAPH_INVALIDATE);
}

//////////////////////////////////////////////////////////////////////////
bool CObjectManager::HitTestObject(CBaseObject* obj, HitContext& hc)
{
    if (obj->IsFrozen())
    {
        return false;
    }

    if (obj->IsHidden())
    {
        return false;
    }

    // This object is rejected by deep selection.
    if (obj->CheckFlags(OBJFLAG_NO_HITTEST))
    {
        return false;
    }

    ObjectType objType = obj->GetType();

    // Check if this object type is masked for selection.
    if (!(objType & gSettings.objectSelectMask))
    {
        return false;
    }

    const bool bSelectionHelperHit = obj->HitHelperTest(hc);

    if (hc.bUseSelectionHelpers && !bSelectionHelperHit)
    {
        return false;
    }

    if (!bSelectionHelperHit)
    {
        // Fast checking.
        if (hc.camera && !obj->IsInCameraView(*hc.camera))
        {
            return false;
        }
        else if (hc.bounds && !obj->IntersectRectBounds(*hc.bounds))
        {
            return false;
        }

        // Do 2D space testing.
        if (hc.nSubObjFlags == 0)
        {
            Ray ray(hc.raySrc, hc.rayDir);
            if (!obj->IntersectRayBounds(ray))
            {
                return false;
            }
        }
        else if (!obj->HitTestRect(hc))
        {
            return false;
        }

        CEditTool* pEditTool = GetIEditor()->GetEditTool();
        if (pEditTool && pEditTool->HitTest(obj, hc))
        {
            return true;
        }
    }

    return (bSelectionHelperHit || obj->HitTest(hc));
}


//////////////////////////////////////////////////////////////////////////
bool CObjectManager::HitTest(HitContext& hitInfo)
{
    FUNCTION_PROFILER(GetIEditor()->GetSystem(), PROFILE_EDITOR);

    hitInfo.object = nullptr;
    hitInfo.dist = FLT_MAX;
    hitInfo.axis = 0;
    hitInfo.manipulatorMode = 0;

    HitContext hcOrg = hitInfo;
    if (hcOrg.view)
    {
        hcOrg.view->GetPerpendicularAxis(0, &hcOrg.b2DViewport);
    }
    hcOrg.rayDir = hcOrg.rayDir.GetNormalized();

    HitContext hc = hcOrg;

    float mindist = FLT_MAX;

    if (!hitInfo.bIgnoreAxis && !hc.bUseSelectionHelpers)
    {
        // Test gizmos.
        if (m_gizmoManager->HitTest(hc))
        {
            if (hc.axis != 0)
            {
                hitInfo.object = hc.object;
                hitInfo.gizmo = hc.gizmo;
                hitInfo.axis = hc.axis;
                hitInfo.manipulatorMode = hc.manipulatorMode;
                hitInfo.dist = hc.dist;
                return true;
            }
        }
    }

    if (hitInfo.bOnlyGizmo)
    {
        return false;
    }

    // Only HitTest objects, that where previously Displayed.
    CBaseObjectsCache* pDispayedViewObjects = hitInfo.view->GetVisibleObjectsCache();

    const bool iconsPrioritized = true; // Force icons to always be prioritized over other things you hit. Can change to be a configurable option in the future.

    CBaseObject* selected = 0;
    const char* name = nullptr;
    bool iconHit = false;
    int numVis = pDispayedViewObjects->GetObjectCount();
    for (int i = 0; i < numVis; i++)
    {
        CBaseObject* obj = pDispayedViewObjects->GetObject(i);

        //! Only check root objects.
        //! One exception: a child should be checked, if belonged group is opened.
        if (obj->GetGroup())
        {
            if (!obj->GetGroup()->IsOpen())
            {
                continue;
            }
        }

        if (obj == hitInfo.pExcludedObject)
        {
            continue;
        }

        if (HitTestObject(obj, hc))
        {
            if (m_selectCallback && !m_selectCallback->CanSelectObject(obj))
            {
                continue;
            }

            // Check if this object is nearest.
            if (hc.axis != 0)
            {
                hitInfo.object = obj;
                hitInfo.axis = hc.axis;
                hitInfo.dist = hc.dist;
                return true;
            }

            // When prioritizing icons, we don't allow non-icon hits to beat icon hits
            if (iconsPrioritized && iconHit && !hc.iconHit)
            {
                continue;
            }

            if (hc.dist < mindist || (!iconHit && hc.iconHit))
            {
                if (hc.iconHit)
                {
                    iconHit = true;
                }

                mindist = hc.dist;
                name = hc.name;
                selected = obj;
            }
            
            // Clear the object pointer if an object was hit, not just if the collision 
            // was closer than any previous. Not all paths from HitTestObject set the object pointer and so you could get 
            // an object from a previous (rejected) result but with collision information about a closer hit.
            hc.object = nullptr;
            hc.iconHit = false;

            // If use deep selection
            if (hitInfo.pDeepSelection)
            {
                hitInfo.pDeepSelection->AddObject(hc.dist, obj);
            }
        }
    }

    if (selected)
    {
        hitInfo.object = selected;
        hitInfo.dist = mindist;
        hitInfo.name = name;
        hitInfo.iconHit = iconHit;
        return true;
    }
    return false;
}
void CObjectManager::FindObjectsInRect(CViewport* view, const QRect& rect, std::vector<GUID>& guids)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);

    if (rect.width() < 1 || rect.height() < 1)
    {
        return;
    }

    HitContext hc;
    hc.view = view;
    hc.b2DViewport = view->GetType() != ET_ViewportCamera;
    hc.rect = rect;
    hc.bUseSelectionHelpers = view->GetAdvancedSelectModeFlag();

    guids.clear();

    CBaseObjectsCache* pDispayedViewObjects = view->GetVisibleObjectsCache();

    int numVis = pDispayedViewObjects->GetObjectCount();
    for (int i = 0; i < numVis; ++i)
    {
        CBaseObject* pObj = pDispayedViewObjects->GetObject(i);

        HitTestObjectAgainstRect(pObj, view, hc, guids);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::SelectObjectsInRect(CViewport* view, const QRect& rect, bool bSelect)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);

    // Ignore too small rectangles.
    if (rect.width() < 1 || rect.height() < 1)
    {
        return;
    }

    CUndo undo("Select Object(s)");

    HitContext hc;
    hc.view = view;
    hc.b2DViewport = view->GetType() != ET_ViewportCamera;
    hc.rect = rect;
    hc.bUseSelectionHelpers = view->GetAdvancedSelectModeFlag();

    bool isUndoRecording = GetIEditor()->IsUndoRecording();
    if (isUndoRecording)
    {
        m_processingBulkSelect = true;
    }

    CBaseObjectsCache* displayedViewObjects = view->GetVisibleObjectsCache();
    int numVis = displayedViewObjects->GetObjectCount();

    // Tracking the previous selection allows proper undo/redo functionality of additional 
    // selections (CTRL + drag select)
    AZStd::unordered_set<const CBaseObject*> previousSelection;

    for (int i = 0; i < numVis; ++i)
    {
        CBaseObject* object = displayedViewObjects->GetObject(i);

        if (object->IsSelected())
        {
            previousSelection.insert(object);
        }
        else
        {
            // This will update m_currSelection
            SelectObjectInRect(object, view, hc, bSelect);

            // Legacy undo/redo does not go through the Ebus system and must be done individually 
            if (isUndoRecording && object->GetType() != OBJTYPE_AZENTITY)
            {
                GetIEditor()->RecordUndo(new CUndoBaseObjectSelect(object, true));
            }
        }
    }

    if (isUndoRecording && m_currSelection)
    {
        // Component Entities can handle undo/redo in bulk due to Ebuses
        GetIEditor()->RecordUndo(new CUndoBaseObjectBulkSelect(previousSelection, *m_currSelection));
    }

    m_processingBulkSelect = false;
}

//////////////////////////////////////////////////////////////////////////
uint16 FindPossibleObjectNameNumber(std::set<uint16>& numberSet)
{
    const int LIMIT = 65535;
    size_t nSetSize = numberSet.size();
    for (uint16 i = 1; i < LIMIT; ++i)
    {
        uint16 candidateNumber = (i + nSetSize) % LIMIT;
        if (numberSet.find(candidateNumber) == numberSet.end())
        {
            numberSet.insert(candidateNumber);
            return candidateNumber;
        }
    }
    return 0;
}

void CObjectManager::RegisterObjectName(const QString& name)
{
    // Remove all numbers from the end of typename.
    QString typeName = name;
    int nameLen = typeName.length();
    int len = nameLen;
    while (len > 0 && typeName[len - 1].isDigit())
    {
        len--;
    }

    typeName = typeName.left(len);

    uint16 num = 1;
    if (len < nameLen)
    {
        num = (uint16)atoi((const char*)name.toUtf8().data() + len) + 0;
    }

    NameNumbersMap::iterator iNameNumber = m_nameNumbersMap.find(typeName);
    if (iNameNumber == m_nameNumbersMap.end())
    {
        std::set<uint16> numberSet;
        numberSet.insert(num);
        m_nameNumbersMap[typeName] = numberSet;
    }
    else
    {
        std::set<uint16>& numberSet = iNameNumber->second;
        numberSet.insert(num);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::UpdateRegisterObjectName(const QString& name)
{
    // Remove all numbers from the end of typename.
    QString typeName = name;
    int nameLen = typeName.length();
    int len = nameLen;

    while (len > 0 && typeName[len - 1].isDigit())
    {
        len--;
    }

    typeName = typeName.left(len);

    uint16 num = 1;
    if (len < nameLen)
    {
        num = (uint16)atoi((const char*)name.toUtf8().data() + len) + 0;
    }

    NameNumbersMap::iterator it = m_nameNumbersMap.find(typeName);

    if (it != m_nameNumbersMap.end())
    {
        if (it->second.end() != it->second.find(num))
        {
            it->second.erase(num);
            if (it->second.empty())
            {
                m_nameNumbersMap.erase(it);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
QString CObjectManager::GenerateUniqueObjectName(const QString& theTypeName)
{
    if (!m_bGenUniqObjectNames)
    {
        return theTypeName;
    }

    QString typeName = theTypeName;
    const int subIndex = theTypeName.indexOf("::");
    if (subIndex != -1 && subIndex > typeName.length() - 2)
    {
        typeName.remove(0, subIndex + 2);
    }

    // Remove all numbers from the end of typename.
    int len = typeName.length();
    while (len > 0 && typeName[len - 1].isDigit())
    {
        len--;
    }

    typeName = typeName.left(len);

    NameNumbersMap::iterator ii = m_nameNumbersMap.find(typeName);
    uint16 lastNumber = 1;
    if (ii != m_nameNumbersMap.end())
    {
        lastNumber = FindPossibleObjectNameNumber(ii->second);
    }
    else
    {
        std::set<uint16> numberSet;
        numberSet.insert(lastNumber);
        m_nameNumbersMap[typeName] = numberSet;
    }

    QString str = QStringLiteral("%1%2").arg(typeName).arg(lastNumber);

    return str;
}

//////////////////////////////////////////////////////////////////////////
bool CObjectManager::EnableUniqObjectNames(bool bEnable)
{
    bool bPrev = m_bGenUniqObjectNames;
    m_bGenUniqObjectNames = bEnable;
    return bPrev;
}

//////////////////////////////////////////////////////////////////////////
CObjectClassDesc* CObjectManager::FindClass(const QString& className)
{
    IClassDesc* cls = CClassFactory::Instance()->FindClass(className.toUtf8().data());
    if (cls != NULL && cls->SystemClassID() == ESYSTEM_CLASS_OBJECT)
    {
        return (CObjectClassDesc*)cls;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::GetClassCategories(QStringList& categories)
{
    std::vector<IClassDesc*> classes;
    CClassFactory::Instance()->GetClassesBySystemID(ESYSTEM_CLASS_OBJECT, classes);
    std::set<QString> cset;
    for (int i = 0; i < classes.size(); i++)
    {
        QString category = classes[i]->Category();
        if (!category.isEmpty())
        {
            cset.insert(category);
        }
    }
    categories.clear();
    categories.reserve(cset.size());
    for (std::set<QString>::iterator cit = cset.begin(); cit != cset.end(); ++cit)
    {
        categories.push_back(*cit);
    }
}

void CObjectManager::GetClassCategoryToolClassNamePairs(std::vector< std::pair<QString, QString> >& categoryToolClassNamePairs)
{
    std::vector<IClassDesc*> classes;
    CClassFactory::Instance()->GetClassesBySystemID(ESYSTEM_CLASS_OBJECT, classes);
    std::set< std::pair<QString, QString> > cset;
    for (int i = 0; i < classes.size(); i++)
    {
        QString category = classes[i]->Category();
        QString toolClassName = ((CObjectClassDesc*)classes[i])->GetToolClassName();
        if (!category.isEmpty())
        {
            cset.insert(std::pair<QString, QString>(category, toolClassName));
        }
    }
    categoryToolClassNamePairs.clear();
    categoryToolClassNamePairs.reserve(cset.size());
    for (std::set< std::pair<QString, QString> >::iterator cit = cset.begin(); cit != cset.end(); ++cit)
    {
        categoryToolClassNamePairs.push_back(*cit);
    }
}

void CObjectManager::GetClassTypes(const QString& category, QStringList& types)
{
    std::vector<IClassDesc*> classes;
    CClassFactory::Instance()->GetClassesBySystemID(ESYSTEM_CLASS_OBJECT, classes);
    for (int i = 0; i < classes.size(); i++)
    {
        QString cat = classes[i]->Category();
        if (QString::compare(cat, category, Qt::CaseInsensitive) == 0 && classes[i]->IsEnabled())
        {
            types.push_back(classes[i]->ClassName());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::RegisterClassTemplate(const XmlNodeRef& templ)
{
    QString typeName = templ->getTag();
    QString superTypeName;
    if (!templ->getAttr("SuperType", superTypeName))
    {
        return;
    }

    CObjectClassDesc* superType = FindClass(superTypeName);
    if (!superType)
    {
        return;
    }

    QString category, fileSpec, initialName;
    templ->getAttr("Category", category);
    templ->getAttr("File", fileSpec);
    templ->getAttr("Name", initialName);

    CXMLObjectClassDesc* classDesc = new CXMLObjectClassDesc;
    classDesc->superType = superType;
    classDesc->type = typeName;
    classDesc->category = category;
    classDesc->fileSpec = fileSpec;
    classDesc->guid = QUuid::createUuid();
    //classDesc->properties = templ->findChild( "Properties" );

    CClassFactory::Instance()->RegisterClass(classDesc);
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::LoadClassTemplates(const QString& path)
{
    QString dir = Path::AddPathSlash(path);

    IFileUtil::FileArray files;
    CFileUtil::ScanDirectory(dir, "*.xml", files, false);

    for (int k = 0; k < files.size(); k++)
    {
        // Construct the full filepath of the current file
        XmlNodeRef node = XmlHelpers::LoadXmlFromFile((dir + files[k].filename).toUtf8().data());
        if (node != 0 && node->isTag("ObjectTemplates"))
        {
            QString name;
            for (int i = 0; i < node->getChildCount(); i++)
            {
                RegisterClassTemplate(node->getChild(i));
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::RegisterCVars()
{
    REGISTER_CVAR2("AxisHelperHitRadius",
        &m_axisHelperHitRadius,
        20,
        VF_DEV_ONLY,
        "Adjust the hit radius used for axis helpers, like the transform gizmo.");
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::Serialize(XmlNodeRef& xmlNode, bool bLoading, int flags)
{
    if (!xmlNode)
    {
        return;
    }

    if (bLoading)
    {
        m_loadedObjects = 0;

        if (flags == SERIALIZE_ONLY_NOTSHARED)
        {
            DeleteNotSharedObjects();
        }
        else if (flags == SERIALIZE_ONLY_SHARED)
        {
            DeleteSharedObjects();
        }
        else
        {
            DeleteAllObjects();
        }


        XmlNodeRef root = xmlNode->findChild("Objects");

        int totalObjects = 0;

        if (root)
        {
            root->getAttr("NumObjects", totalObjects);
        }


        StartObjectsLoading(totalObjects);

        // Load layers.
        CObjectArchive ar(this, xmlNode, true);

        // Load layers.
        m_pLayerManager->Serialize(ar);

        // Loading.
        if (root)
        {
            ar.node = root;
            LoadObjects(ar, false);
        }
        EndObjectsLoading();
    }
    else
    {
        // Saving.
        XmlNodeRef root = xmlNode->newChild("Objects");

        CObjectArchive ar(this, root, false);

        // Save all objects to XML.
        for (Objects::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
        {
            CBaseObject* obj = it->second;

            // Not save objects in prefabs or groups.
            if (obj->GetGroup() || obj->CheckFlags(OBJFLAG_PREFAB))
            {
                continue;
            }

            if (obj->CheckFlags(OBJFLAG_DONT_SAVE))
            {
                continue;
            }

            if ((flags == SERIALIZE_ONLY_SHARED) && !obj->CheckFlags(OBJFLAG_SHARED))
            {
                continue;
            }
            else if ((flags == SERIALIZE_ONLY_NOTSHARED) && obj->CheckFlags(OBJFLAG_SHARED))
            {
                continue;
            }

            CObjectLayer* pLayer = obj->GetLayer();
            if (pLayer->IsExternal())
            {
                continue;
            }

            XmlNodeRef objNode = root->newChild("Object");
            ar.node = objNode;
            obj->Serialize(ar);
        }

        // Save layers.
        ar.node = xmlNode;
        m_pLayerManager->Serialize(ar);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::LoadObjects(CObjectArchive& objectArchive, bool bSelect)
{
    m_bLoadingObjects = true;

    GetIEditor()->GetPrefabManager()->SetSkipPrefabUpdate(true);

    XmlNodeRef objectsNode = objectArchive.node;
    int numObjects = objectsNode->getChildCount();
    for (int i = 0; i < numObjects; i++)
    {
        objectArchive.node = objectsNode->getChild(i);
        CBaseObject* obj = objectArchive.LoadObject(objectsNode->getChild(i));
        if (obj && bSelect)
        {
            SelectObject(obj);
        }
    }
    EndObjectsLoading(); // End progress bar, here, Resolve objects have his own.
    objectArchive.ResolveObjects();

    InvalidateVisibleList();

    GetIEditor()->GetPrefabManager()->SetSkipPrefabUpdate(false);

    m_bLoadingObjects = false;
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::Export(const QString& levelPath, XmlNodeRef& rootNode, bool onlyShared)
{
    // Clear export files.
    QFile::remove(QStringLiteral("%1TagPoints.ini").arg(levelPath));
    QFile::remove(QStringLiteral("%1Volumes.ini").arg(levelPath));

    // Save all objects to XML.
    for (Objects::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        CBaseObject* obj = it->second;
        CObjectLayer* pLayer = obj->GetLayer();
        if (!pLayer->IsExportable())
        {
            continue;
        }
        // Export Only shared objects.
        if ((obj->CheckFlags(OBJFLAG_SHARED) && onlyShared) ||
            (!obj->CheckFlags(OBJFLAG_SHARED) && !onlyShared))
        {
            obj->Export(levelPath, rootNode);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::ExportEntities(XmlNodeRef& rootNode)
{
    // Save all objects to XML.
    for (Objects::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        CBaseObject* obj = it->second;
        CObjectLayer* pLayer = obj->GetLayer();
        if (!pLayer->IsExportable())
        {
            continue;
        }
        if (qobject_cast<CEntityObject*>(obj))
        {
            obj->Export("", rootNode);
        }
    }
}

void CObjectManager::DeleteNotSharedObjects()
{
    TBaseObjects objects;
    GetAllObjects(objects);
    for (int i = 0; i < objects.size(); i++)
    {
        CBaseObject* obj = objects[i];
        if (!obj->CheckFlags(OBJFLAG_SHARED))
        {
            DeleteObject(obj);
        }
    }
}

void CObjectManager::DeleteSharedObjects()
{
    TBaseObjects objects;
    GetAllObjects(objects);
    for (int i = 0; i < objects.size(); i++)
    {
        CBaseObject* obj = objects[i];
        if (obj->CheckFlags(OBJFLAG_SHARED))
        {
            DeleteObject(obj);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
IObjectSelectCallback* CObjectManager::SetSelectCallback(IObjectSelectCallback* callback)
{
    IObjectSelectCallback* prev = m_selectCallback;
    m_selectCallback = callback;
    return prev;
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::InvalidateVisibleList()
{
    if (m_isUpdateVisibilityList)
    {
        return;
    }
    ++m_visibilitySerialNumber;
    m_visibleObjects.clear();
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::UpdateVisibilityList()
{
    m_isUpdateVisibilityList = true;
    m_visibleObjects.clear();

    bool isInIsolationMode = false;
    AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(isInIsolationMode, &AzToolsFramework::ToolsApplicationRequestBus::Events::IsEditorInIsolationMode);

    for (Objects::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        CBaseObject* obj = it->second;
        bool visible = obj->IsPotentiallyVisible();

        // entities not isolated in Isolation Mode will be invisible
        bool isObjectIsolated = obj->IsIsolated();
        visible = visible && (!isInIsolationMode || isObjectIsolated);

        obj->UpdateVisibility(visible);
        if (visible)
        {
            // Prefabs are not added into visible list.
            if (!obj->CheckFlags(OBJFLAG_PREFAB))
            {
                m_visibleObjects.push_back(obj);
            }
        }
    }
    m_isUpdateVisibilityList = false;
}

//////////////////////////////////////////////////////////////////////////
CBaseObject* CObjectManager::FindAnimNodeOwner(CTrackViewAnimNode* pNode) const
{
    CEntityObject* entityObject = nullptr;

    if (pNode)
    {
        entityObject = pNode->GetNodeEntity(false);
        if (!entityObject)
        {
            // Find owner entity.
            IEntity* pIEntity = pNode->GetEntity();
            if (pIEntity)
            {
                // Find owner editor entity.
                entityObject = CEntityObject::FindFromEntityId(pIEntity->GetId());
            }

            // If we haven't found the entityObject using the legacy CryEntity methods, finally try searching AZ Entities
            if (!entityObject && pNode->GetAzEntityId().IsValid())
            {
                EBUS_EVENT_ID_RESULT(entityObject, pNode->GetAzEntityId(), AzToolsFramework::ComponentEntityEditorRequestBus, GetSandboxObject);
            }
        }
    }
    return entityObject;
}

//////////////////////////////////////////////////////////////////////////
bool CObjectManager::ConvertToType(CBaseObject* pObject, const QString& typeName)
{
    QString message = QString("Convert ") + pObject->GetName() + " to " + typeName;
    CUndo undo(message.toUtf8().data());

    CBaseObjectPtr pNewObject = GetIEditor()->NewObject(typeName.toUtf8().data());
    if (pNewObject)
    {
        if (pNewObject->ConvertFromObject(pObject))
        {
            DeleteObject(pObject);
            return true;
        }
        DeleteObject(pNewObject);
    }

    Log((message + " is failed.").toUtf8().data());
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::SetObjectSelected(CBaseObject* pObject, bool bSelect)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);
    // Only select/unselect once.
    if ((pObject->IsSelected() && bSelect) || (!pObject->IsSelected() && !bSelect))
    {
        return;
    }

    // Store selection undo.
    if (CUndo::IsRecording() && !m_processingBulkSelect)
    {
        CUndo::Record(new CUndoBaseObjectSelect(pObject));
    }

    pObject->SetSelected(bSelect);
    m_bSelectionChanged = true;


    if (bSelect && !GetIEditor()->GetTransformManipulator())
    {
        if (CAxisGizmo::GetGlobalAxisGizmoCount() < gSettings.gizmo.axisGizmoMaxCount)
        {
            // Create axis gizmo for this object.
            m_gizmoManager->AddGizmo(new CAxisGizmo(pObject));
        }
    }

    if (bSelect)
    {
        NotifyObjectListeners(pObject, CBaseObject::ON_SELECT);
    }
    else
    {
        NotifyObjectListeners(pObject, CBaseObject::ON_UNSELECT);
    }

    if (qobject_cast<CAITerritoryObject*>(pObject) || qobject_cast<CAIWaveObject*>(pObject))
    {
        RefreshEntitiesAssignedToSelectedTnW();
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::HideTransformManipulators()
{
    m_gizmoManager->DeleteAllTransformManipulators();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CObjectManager::AddObjectEventListener(const EventCallback& cb)
{
    stl::push_back_unique(m_objectEventListeners, cb);
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::RemoveObjectEventListener(const EventCallback& cb)
{
    stl::find_and_erase(m_objectEventListeners, cb);
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::NotifyObjectListeners(CBaseObject* pObject, CBaseObject::EObjectListenerEvent event)
{
    std::list<EventCallback>::iterator next;
    for (std::list<EventCallback>::iterator it = m_objectEventListeners.begin(); it != m_objectEventListeners.end(); it = next)
    {
        next = it;
        ++next;
        // Call listener callback.
        (*it)(pObject, event);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::StartObjectsLoading(int numObjects)
{
    if (m_pLoadProgress)
    {
        return;
    }
    m_pLoadProgress = new CWaitProgress("Loading Objects");
    m_totalObjectsToLoad = numObjects;
    m_loadedObjects = 0;
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::EndObjectsLoading()
{
    if (m_pLoadProgress)
    {
        delete m_pLoadProgress;
    }
    m_pLoadProgress = 0;
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::GatherUsedResources(CUsedResources& resources, CObjectLayer* pLayer)
{
    CBaseObjectsArray objects;
    GetIEditor()->GetObjectManager()->GetObjects(objects, pLayer);

    for (int i = 0; i < objects.size(); i++)
    {
        CBaseObject* pObject = objects[i];
        pObject->GatherUsedResources(resources);
    }
}

//////////////////////////////////////////////////////////////////////////
IGizmoManager* CObjectManager::GetGizmoManager()
{
    return m_gizmoManager;
}

//////////////////////////////////////////////////////////////////////////
IStatObj* CObjectManager::GetGeometryFromObject(CBaseObject* pObject)
{
    assert(pObject);

    if (qobject_cast<CBrushObject*>(pObject))
    {
        CBrushObject* pBrushObj = (CBrushObject*)pObject;
        return pBrushObj->GetIStatObj();
    }
    if (qobject_cast<CEntityObject*>(pObject))
    {
        CEntityObject* pEntityObj = (CEntityObject*)pObject;
        if (pEntityObj->GetIEntity())
        {
            IEntity* pGameEntity = pEntityObj->GetIEntity();
            for (int i = 0; pGameEntity != NULL && i < pGameEntity->GetSlotCount(); i++)
            {
                if (pGameEntity->GetStatObj(i))
                {
                    return pGameEntity->GetStatObj(i);
                }
            }
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
ICharacterInstance* CObjectManager::GetCharacterFromObject(CBaseObject* pObject)
{
    assert(pObject);
    if (qobject_cast<CEntityObject*>(pObject))
    {
        CEntityObject* pEntityObj = (CEntityObject*)pObject;
        if (pEntityObj->GetIEntity())
        {
            IEntity* pGameEntity = pEntityObj->GetIEntity();
            for (int i = 0; pGameEntity != NULL && i < pGameEntity->GetSlotCount(); i++)
            {
                if (pGameEntity->GetCharacter(i))
                {
                    return pGameEntity->GetCharacter(i);
                }
            }
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
CBaseObject* CObjectManager::FindPhysicalObjectOwner(IPhysicalEntity* pPhysicalEntity)
{
    if (!pPhysicalEntity)
    {
        return 0;
    }

    int itype = pPhysicalEntity->GetiForeignData();
    switch (itype)
    {
    case PHYS_FOREIGN_ID_ROPE:
    {
        IRopeRenderNode* pRenderNode = (IRopeRenderNode*)pPhysicalEntity->GetForeignData(itype);
        if (pRenderNode)
        {
            EntityId id = (EntityId)pRenderNode->GetEntityOwner();
            CEntityObject* pEntity = CEntityObject::FindFromEntityId(id);
            return pEntity;
        }
    }
    break;
    case PHYS_FOREIGN_ID_ENTITY:
    {
        IEntity* pIEntity = gEnv->pEntitySystem ? gEnv->pEntitySystem->GetEntityFromPhysics(pPhysicalEntity) : nullptr;
        if (pIEntity)
        {
            return CEntityObject::FindFromEntityId(pIEntity->GetId());
        }
    }
    break;
    case PHYS_FOREIGN_ID_STATIC:
    {
        IRopeRenderNode* pRenderNode = (IRopeRenderNode*)pPhysicalEntity->GetForeignData(itype);
        if (pRenderNode)
        {
            // Find brush who created this render node.
        }
    }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::OnObjectModified(CBaseObject* pObject, bool bDelete, bool boModifiedTransformOnly)
{
    if (!m_bLoadingObjects)
    {
        if (qobject_cast<CEntityObject*>(pObject))
        {
            if (qobject_cast<CAITerritoryObject*>(pObject) != nullptr || qobject_cast<CAIWaveObject*>(pObject) != nullptr)
            {
                RefreshEntitiesAssignedToSelectedTnW();
            }
            else
            {
                CEntityObject* pEntity = static_cast<CEntityObject*>(pObject);  // Editor's class CEntity
                IEntity* pIEntity = pEntity->GetIEntity();  // CryEntitySystem's interface IEntity
                if (pIEntity)
                {
                    IAIObject* pAIObject = pIEntity->GetAI();
                    if (pAIObject && pAIObject->IsAgent())
                    {
                        RefreshEntitiesAssignedToSelectedTnW();
                    }
                }
            }
        }
    }

    if (IRenderNode* pRenderNode = pObject->GetEngineNode())
    {
        GetIEditor()->Get3DEngine()->OnObjectModified(pRenderNode, pRenderNode->GetRndFlags());
    }
}


//////////////////////////////////////////////////////////////////////////
void CObjectManager::UnregisterNoExported()
{
    I3DEngine* p3DEngine = GetIEditor()->Get3DEngine();
    for (Objects::const_iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        CBaseObject* pObj = it->second;
        CObjectLayer* pLayer = pObj->GetLayer();
        if (pLayer && !pLayer->IsExportable())
        {
            IRenderNode* pRenderNode = pObj->GetEngineNode();
            if (pRenderNode && pRenderNode->GetEntityStatObj())
            {
                p3DEngine->UnRegisterEntityAsJob(pRenderNode);
            }
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CObjectManager::RegisterNoExported()
{
    I3DEngine* p3DEngine = GetIEditor()->Get3DEngine();
    for (Objects::const_iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        CBaseObject* pObj = it->second;
        CObjectLayer* pLayer = pObj->GetLayer();
        if (pLayer && !pLayer->IsExportable())
        {
            IRenderNode* pRenderNode = pObj->GetEngineNode();
            if (pRenderNode && pRenderNode->GetEntityStatObj())
            {
                p3DEngine->RegisterEntity(pRenderNode);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CObjectManager::IsEntityAssignedToSelectedTerritory(CEntityObject* pEntity)
{
    return m_setEntitiesAssignedToSelectedTerritory.find(pEntity) != m_setEntitiesAssignedToSelectedTerritory.end();
}

//////////////////////////////////////////////////////////////////////////
bool CObjectManager::IsEntityAssignedToSelectedWave(CEntityObject* pEntity)
{
    return m_setEntitiesAssignedToSelectedWave.find(pEntity) != m_setEntitiesAssignedToSelectedWave.end();
}

//////////////////////////////////////////////////////////////////////////
bool CObjectManager::IsLightClass(CBaseObject* pObject)
{
    if (qobject_cast<CEntityObject*>(pObject))
    {
        CEntityObject* pEntity = (CEntityObject*)pObject;
        if (pEntity)
        {
            if (pEntity->GetEntityClass().compare(CLASS_LIGHT) == 0)
            {
                return TRUE;
            }
            if (pEntity->GetEntityClass().compare(CLASS_RIGIDBODY_LIGHT) == 0)
            {
                return TRUE;
            }
            if (pEntity->GetEntityClass().compare(CLASS_DESTROYABLE_LIGHT) == 0)
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::RefreshEntitiesAssignedToSelectedTnW()
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);

    m_setEntitiesAssignedToSelectedTerritory.clear();
    m_setEntitiesAssignedToSelectedWave.clear();

    // No need to refresh entities assigned to selected waves/territories when there are no waves/territories
    if (m_aiTerritoryObjects.empty() && m_aiWaveObjects.empty())
    {
        return;
    }

    std::vector<CAITerritoryObject*> vSelectedTerritories;
    std::set<QString> setSelectedTerritories;
    std::set<QString> setSelectedWaves;

    std::map<CEntityObject*, QString> mapEntityTerritories;
    std::vector<CEntityObject*> vEntitiesWithAutoAssignment;
    std::map<CEntityObject*, QString> mapEntityWaves;

    QString sEntityTerritory;
    QString sEntityWave;

    CBaseObjectsArray objects;
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Editor, "CObjectManager::RefreshEntitiesAssignedToSelectedTnW:GetObjects");
        GetObjects(objects);
    }

    // First clarify relationships between Entities (e.g. Grunts) and AI Territories & Waves
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Editor, "CObjectManager::RefreshEntitiesAssignedToSelectedTnW:ClarifyRelationships");
        for (size_t i = 0, n = objects.size(); i < n; ++i)
        {
            CBaseObject* pObject = objects[i];
            if (!qobject_cast<CEntityObject*>(pObject))
            {
                continue;
            }

            CEntityObject* pEntity = static_cast<CEntityObject*>(pObject);

            if (pEntity->IsHidden() || pEntity->IsHiddenBySpec() || pEntity->IsFrozen())
            {
                continue;
            }

            if (qobject_cast<CAITerritoryObject*>(pEntity))
            {
                if (pEntity->IsSelected())
                {
                    vSelectedTerritories.push_back(static_cast<CAITerritoryObject*>(pEntity));
                    setSelectedTerritories.insert(pEntity->GetName());
                }
            }
            else if (qobject_cast<CAIWaveObject*>(pEntity))
            {
                if (pEntity->IsSelected())
                {
                    setSelectedWaves.insert(pEntity->GetName());
                }
            }
            else
            {
                // Associate Entities (e.g. Grunts) with their Territories and Waves

                CVarBlock* pProperties2 = pEntity->GetProperties2();
                if (pProperties2)
                {
                    IVariable* pVarTerritory = pProperties2->FindVariable("aiterritory_Territory");
                    if (pVarTerritory)
                    {
                        pVarTerritory->Get(sEntityTerritory);
                        if (!sEntityTerritory.isEmpty() && (sEntityTerritory != "<None>"))
                        {
    #ifndef USE_SIMPLIFIED_AI_TERRITORY_SHAPE
                            if (sEntityTerritory == "<Auto>")
                            {
                                vEntitiesWithAutoAssignment.push_back(pEntity);
                            }
                            else
    #endif
                            {
                                mapEntityTerritories.insert(std::make_pair(pEntity, sEntityTerritory));

                                IVariable* pVarWave = pProperties2->FindVariable("aiwave_Wave");
                                if (pVarWave)
                                {
                                    pVarWave->Get(sEntityWave);
                                    if (!sEntityWave.isEmpty() && (sEntityWave != "<None>"))
                                    {
                                        mapEntityWaves.insert(std::make_pair(pEntity, sEntityWave));
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Now figure out what to select
    {
        AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Editor, "CObjectManager::RefreshEntitiesAssignedToSelectedTnW:DetermineSelection");
        for (std::map<CEntityObject*, QString>::iterator it = mapEntityTerritories.begin(), end = mapEntityTerritories.end(); it != end; ++it)
        {
            CEntityObject* pEntity = it->first;
            const QString& sTerritoryName = it->second;

            if (setSelectedTerritories.find(sTerritoryName) != setSelectedTerritories.end())
            {
                m_setEntitiesAssignedToSelectedTerritory.insert(pEntity);
            }
        }

        for (std::vector<CAITerritoryObject*>::iterator it = vSelectedTerritories.begin(), end = vSelectedTerritories.end(); it != end; ++it)
        {
            CAITerritoryObject* territory = *it;

            const Matrix34& tm = territory->GetWorldTM();
            std::vector<Vec3> territoryPoints;
            for (int i = 0, n = territory->GetPointCount(); i < n; ++i)
            {
                Vec3 point = tm.TransformPoint(territory->GetPoint(i));
                territoryPoints.push_back(point);
            }

            for (std::vector<CEntityObject*>::iterator it2 = vEntitiesWithAutoAssignment.begin(), end = vEntitiesWithAutoAssignment.end(); it2 != end; ++it2)
            {
                CEntityObject* pEntity = *it2;

                const Vec3& pos = pEntity->GetPos();
                float height = territory->GetHeight();
                float h = pos.z - territory->GetPos().z;
                if ((height < 0.01f) || ((0.0f < h) && (h < height)))
                {
                    if (Overlap::Point_Polygon2D(pos, territoryPoints))
                    {
                        m_setEntitiesAssignedToSelectedTerritory.insert(pEntity);
                    }
                }
            }
        }

        for (std::map<CEntityObject*, QString>::iterator it = mapEntityWaves.begin(), end = mapEntityWaves.end(); it != end; ++it)
        {
            CEntityObject* pEntity = it->first;
            const QString& sWaveName = it->second;

            if (setSelectedWaves.find(sWaveName) != setSelectedWaves.end())
            {
                m_setEntitiesAssignedToSelectedWave.insert(pEntity);
            }
        }
    }
}

size_t CObjectManager::NumberOfAssignedEntities()
{
    std::set<CEntityObject*> result;
    std::set_union(
        m_setEntitiesAssignedToSelectedTerritory.begin(), m_setEntitiesAssignedToSelectedTerritory.end(),
        m_setEntitiesAssignedToSelectedWave.begin(), m_setEntitiesAssignedToSelectedWave.end(),
        inserter(result, result.begin()));
    return result.size();
}

void CObjectManager::SelectAssignedEntities()
{
    // Memorize what to select before call to ClearSelection()
    std::set<CEntityObject*> t = m_setEntitiesAssignedToSelectedTerritory;
    std::set<CEntityObject*> w = m_setEntitiesAssignedToSelectedWave;

    ClearSelection();

    SelectEntities(t);
    SelectEntities(w);
}

void CObjectManager::FindAndRenameProperty2(const char* property2Name, const QString& oldValue, const QString& newValue)
{
    CBaseObjectsArray objects;
    GetObjects(objects);

    for (size_t i = 0, n = objects.size(); i < n; ++i)
    {
        CBaseObject* pObject = objects[i];
        if (qobject_cast<CEntityObject*>(pObject))
        {
            CEntityObject* pEntity = static_cast<CEntityObject*>(pObject);
            CVarBlock* pProperties2 = pEntity->GetProperties2();
            if (pProperties2)
            {
                IVariable* pVariable = pProperties2->FindVariable(property2Name);
                if (pVariable)
                {
                    QString sValue;
                    pVariable->Get(sValue);
                    if (sValue == oldValue)
                    {
                        pEntity->StoreUndo("Rename Property2");

                        pVariable->Set(newValue);

                        // Special case
#ifdef USE_SIMPLIFIED_AI_TERRITORY_SHAPE
                        if (strcmp(property2Name, "aiterritory_Territory") == 0 && ((newValue == "<None>") || (newValue != oldValue)))
#else
                        if (strcmp(property2Name, "aiterritory_Territory") == 0 && ((newValue == "<Auto>") || (newValue == "<None>") || (newValue != oldValue)))
#endif
                        {
                            IVariable* pVariableWave = pProperties2->FindVariable("aiwave_Wave");
                            if (pVariableWave)
                            {
                                pVariableWave->Set("<None>");
                            }
                        }
                    }
                }
            }
        }
    }
}

void CObjectManager::FindAndRenameProperty2If(const char* property2Name, const QString& oldValue, const QString& newValue, const char* otherProperty2Name, const QString& otherValue)
{
    CBaseObjectsArray objects;
    GetObjects(objects);

    for (size_t i = 0, n = objects.size(); i < n; ++i)
    {
        CBaseObject* pObject = objects[i];
        if (qobject_cast<CEntityObject*>(pObject))
        {
            CEntityObject* pEntity = static_cast<CEntityObject*>(pObject);
            CVarBlock* pProperties2 = pEntity->GetProperties2();
            if (pProperties2)
            {
                IVariable* pVariable      = pProperties2->FindVariable(property2Name);
                IVariable* pOtherVariable = pProperties2->FindVariable(otherProperty2Name);
                if (pVariable && pOtherVariable)
                {
                    QString sValue;
                    pVariable->Get(sValue);

                    QString sOtherValue;
                    pOtherVariable->Get(sOtherValue);

                    if ((sValue == oldValue) && (sOtherValue == otherValue))
                    {
                        pEntity->StoreUndo("Rename Property2 If");

                        pVariable->Set(newValue);

                        // Special case
#ifdef USE_SIMPLIFIED_AI_TERRITORY_SHAPE
                        if ((strcmp(property2Name, "aiterritory_Territory") == 0) && (newValue == "<None>"))
#else
                        if ((strcmp(property2Name, "aiterritory_Territory") == 0) && ((newValue == "<Auto>") || (newValue == "<None>")))
#endif
                        {
                            IVariable* pVariableWave = pProperties2->FindVariable("aiwave_Wave");
                            if (pVariableWave)
                            {
                                pVariableWave->Set("<None>");
                            }
                        }
                    }
                }
            }
        }
    }
}


void CObjectManager::ResolveMissingObjects()
{
    enum
    {
        eInit = 0,
        eNoForAll = QMessageBox::NoToAll,
        eNo = QMessageBox::No,
        eYesForAll = QMessageBox::YesToAll,
        eYes = QMessageBox::Yes
    };

    typedef std::map<QString, QString> LocationMap;
    LocationMap locationMap;

    int locationState = eInit;
    bool isUpdated = false;

    Log("Resolving missed objects...");

    for (Objects::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        CBaseObject* obj = it->second;

        CGeomEntity* pGeomEntity = 0;
        CSimpleEntity* pSimpleEntity = 0;
        CBrushObject* pBrush = 0;
        IEntity* pEntity = 0;
        QString geometryFile;
        IVariable* pModelVar = 0;

        if (qobject_cast<CGeomEntity*>(obj))
        {
            IEntity* pEntityObj = ((CGeomEntity*)obj)->GetIEntity();
            if (pEntityObj && pEntityObj->GetStatObj(0) && pEntityObj->GetStatObj(0)->IsDefaultObject())
            {
                pGeomEntity = (CGeomEntity*)obj;
                geometryFile = pGeomEntity->GetGeometryFile();
            }
        }
        else if (qobject_cast<CSimpleEntity*>(obj))
        {
            IEntity* pEntityObj = ((CSimpleEntity*)obj)->GetIEntity();
            if (pEntityObj && pEntityObj->GetStatObj(0) && pEntityObj->GetStatObj(0)->IsDefaultObject())
            {
                pSimpleEntity = (CSimpleEntity*)obj;
                geometryFile = pSimpleEntity->GetGeometryFile();
            }
        }
        else if (qobject_cast<CBrushObject*>(obj))
        {
            CBrushObject* pBrushObj = (CBrushObject*)obj;
            if (pBrushObj->GetGeometry() && ((CEdMesh*)pBrushObj->GetGeometry())->IsDefaultObject())
            {
                pBrush = (CBrushObject*)obj;
                geometryFile = pBrush->GetGeometryFile();
            }
        }
        else if (qobject_cast<CEntityObject*>(obj))
        {
            IEntity* pEntityObj = ((CEntityObject*)obj)->GetIEntity();
            if (pEntityObj && pEntityObj->GetStatObj(0) && pEntityObj->GetStatObj(0)->IsDefaultObject())
            {
                CVarBlock* pVars = ((CEntityObject*)obj)->GetProperties();
                if (pVars)
                {
                    for (int i = 0; i < pVars->GetNumVariables(); i++)
                    {
                        pModelVar = pVars->GetVariable(i);
                        if (pModelVar && pModelVar->GetDataType() == IVariable::DT_FILE)
                        {
                            pModelVar->Get(geometryFile);
                            QString ext = PathUtil::GetExt(geometryFile.toUtf8().data());
                            if (QString::compare(ext, CRY_GEOMETRY_FILE_EXT, Qt::CaseInsensitive) == 0 || QString::compare(ext, CRY_SKEL_FILE_EXT, Qt::CaseInsensitive) == 0 || QString::compare(ext, CRY_CHARACTER_DEFINITION_FILE_EXT, Qt::CaseInsensitive) == 0 || QString::compare(ext, CRY_ANIM_GEOMETRY_FILE_EXT, Qt::CaseInsensitive) == 0)
                            {
                                if (!gEnv->pCryPak->IsFileExist(geometryFile.toUtf8().data()))
                                {
                                    pEntity = pEntityObj;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (!pGeomEntity && !pSimpleEntity && !pBrush && !pEntity)
        {
            continue;
        }

        int nKey = 0;

        QString newFilename = stl::find_in_map(locationMap, geometryFile, QString(""));
        if (newFilename != "")
        {
            if (pGeomEntity)
            {
                pGeomEntity->SetGeometryFile(newFilename);
            }
            else if (pSimpleEntity)
            {
                pSimpleEntity->SetGeometryFile(newFilename);
            }
            else if (pBrush)
            {
                pBrush->CreateBrushFromMesh(newFilename.toUtf8().data());
                if (pBrush->GetGeometry() && !((CEdMesh*)pBrush->GetGeometry())->IsDefaultObject())
                {
                    pBrush->SetGeometryFile(newFilename);
                }
            }
            else if (pEntity)
            {
                pModelVar->Set(newFilename);
            }
            Log("%s: %s <- %s", obj->GetName().toUtf8().constData(), geometryFile.toUtf8().constData(), newFilename.toUtf8().constData());
            continue;
        }

        if (locationState == eNoForAll)
        {
            continue;
        }

        if (locationState != eYesForAll) // Skip, if "Yes for All" pressed before
        {
            QString mes = QObject::tr("Geometry file for object \"%1\" is missing/removed. \r\nFile: %2\r\nAttempt to locate this file?").arg(obj->GetName(), geometryFile);
            nKey = QMessageBox::question(QApplication::activeWindow(), QObject::tr("Object missing"), mes, QMessageBox::NoToAll | QMessageBox::No | QMessageBox::Yes | QMessageBox::YesToAll);

            if (nKey == eNoForAll && locationMap.size() == 0)
            {
                break;
            }

            if (nKey == eNoForAll || nKey == eYesForAll)
            {
                locationState = nKey;
            }
        }

        if (nKey == eYes || locationState == eYesForAll)
        {
            IFileUtil::FileArray cFiles;
            QString filemask = PathUtil::GetFile(geometryFile.toUtf8().data());
            CFileUtil::ScanDirectory(Path::GetEditingGameDataFolder().c_str(), filemask, cFiles, true);

            if (cFiles.size())
            {
                QString newFilename = cFiles[0].filename;
                if (pGeomEntity)
                {
                    pGeomEntity->SetGeometryFile(newFilename);
                }
                else if (pSimpleEntity)
                {
                    pSimpleEntity->SetGeometryFile(newFilename);
                }
                else if (pBrush)
                {
                    pBrush->CreateBrushFromMesh(newFilename.toUtf8().data());
                    if (pBrush->GetGeometry() && !((CEdMesh*)pBrush->GetGeometry())->IsDefaultObject())
                    {
                        pBrush->SetGeometryFile(newFilename);
                    }
                }
                else if (pEntity)
                {
                    pModelVar->Set(newFilename);
                }
                locationMap[geometryFile] = newFilename;
                Log("%s: %s <- %s", obj->GetName().toUtf8().constData(), geometryFile.toUtf8().constData(), newFilename.toUtf8().constData());
                isUpdated = true;
            }
            else
            {
                GetIEditor()->GetSystem()->GetILog()->LogWarning("Can't resolve object: %s: %s", obj->GetName().toUtf8().constData(), geometryFile.toUtf8().constData());
            }
        }
    }
    if (isUpdated)
    {
        GetIEditor()->SetModifiedFlag();
    }
    else
    {
        Log("No objects has been resolved.");
    }

    ResolveMissingMaterials();
}


void CObjectManager::ResolveMissingMaterials()
{
    enum
    {
        eInit = 0,
        eNoForAll = QMessageBox::NoToAll,
        eNo = QMessageBox::No,
        eYesForAll = QMessageBox::YesToAll,
        eYes = QMessageBox::Yes
    };

    typedef std::map<QString, QString> LocationMap;
    LocationMap locationMap;

    int locationState = eInit;
    bool isUpdated = false;

    QString oldFilename;

    Log("Resolving missed materials...");

    for (Objects::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        CBaseObject* obj = it->second;

        CMaterial* pMat = obj->GetMaterial();

        if (pMat && pMat->GetMatInfo() && pMat->GetMatInfo()->IsDefault())
        {
            oldFilename = pMat->GetFilename();
        }
        else
        {
            continue;
        }

        int nKey = 0;

        QString newFilename = stl::find_in_map(locationMap, oldFilename, QString(""));
        if (newFilename != "")
        {
            CMaterial* pNewMaterial = GetIEditor()->GetMaterialManager()->LoadMaterial(newFilename);
            if (pNewMaterial)
            {
                obj->SetMaterial(pNewMaterial);
                Log("%s: %s <- %s", pMat->GetName().toUtf8().constData(), oldFilename.toUtf8().constData(), newFilename.toUtf8().constData());
            }
            continue;
        }

        if (locationState == eNoForAll)
        {
            continue;
        }

        if (locationState != eYesForAll) // Skip, if "Yes for All" pressed before
        {
            QString mes = QObject::tr("Material for object \"%1\" is missing/removed. \r\nFile: %2\r\nAttempt to locate this file?").arg(obj->GetName(), oldFilename);
            nKey = QMessageBox::question(QApplication::activeWindow(), QObject::tr("Material missing"), mes, QMessageBox::NoToAll | QMessageBox::No | QMessageBox::Yes | QMessageBox::YesToAll);

            if (nKey == eNoForAll && locationMap.size() == 0)
            {
                break;
            }

            if (nKey == eNoForAll || nKey == eYesForAll)
            {
                locationState = nKey;
            }
        }

        if (nKey == eYes || locationState == eYesForAll)
        {
            IFileUtil::FileArray cFiles;
            QString filemask = PathUtil::GetFile(oldFilename.toUtf8().data());
            CFileUtil::ScanDirectory(Path::GetEditingGameDataFolder().c_str(), filemask, cFiles, true);

            if (cFiles.size())
            {
                QString newFilename = cFiles[0].filename;

                CMaterial* pNewMaterial = GetIEditor()->GetMaterialManager()->LoadMaterial(newFilename);
                if (pNewMaterial)
                {
                    obj->SetMaterial(pNewMaterial);
                    locationMap[oldFilename] = newFilename;
                    Log("%s: %s <- %s", pMat->GetName().toUtf8().constData(), oldFilename.toUtf8().constData(), newFilename.toUtf8().constData());
                    isUpdated = true;
                }
            }
            else
            {
                GetIEditor()->GetSystem()->GetILog()->LogWarning("Can't resolve material: %s: %s", pMat->GetName().toUtf8().constData(), oldFilename.toUtf8().constData());
            }
        }
    }
    if (isUpdated)
    {
        GetIEditor()->SetModifiedFlag();
    }
    else
    {
        Log("No materials has been resolved.");
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::UpdateAttachedEntities()
{
    for (CEntityObject* attachedEntityObj : m_animatedAttachedEntities)
    {
        attachedEntityObj->UpdateTransform();
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::AssignLayerIDsToRenderNodes()
{
    m_pLayerManager->AssignLayerIDsToRenderNodes();
}


//////////////////////////////////////////////////////////////////////////
void CObjectManager::HitTestObjectAgainstRect(CBaseObject* pObj, CViewport* view, HitContext hc, std::vector<GUID>& guids)
{
    if (!pObj->IsSelectable())
    {
        return;
    }

    AABB box;

    // Retrieve world space bound box.
    pObj->GetBoundBox(box);

    // Check if object visible in viewport.
    if (!view->IsBoundsVisible(box))
    {
        return;
    }

    if (qobject_cast<CGroup*>(pObj))
    {
        CGroup* pGroup = static_cast<CGroup*>(pObj);
        // If the group is open check children
        if (pGroup->IsOpen())
        {
            for (int i = 0; i < pGroup->GetChildCount(); ++i)
            {
                HitTestObjectAgainstRect(pGroup->GetChild(i), view, hc, guids);
            }
        }
    }

    if (pObj->HitTestRect(hc))
    {
        stl::push_back_unique(guids, pObj->GetId());
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::SelectObjectInRect(CBaseObject* pObj, CViewport* view, HitContext hc, bool bSelect)
{
    if (!pObj->IsSelectable())
    {
        return;
    }

    AABB box;

    // Retrieve world space bound box.
    pObj->GetBoundBox(box);

    // Check if object visible in viewport.
    if (!view->IsBoundsVisible(box))
    {
        return;
    }

    if (qobject_cast<CGroup*>(pObj))
    {
        CGroup* pGroup = static_cast<CGroup*>(pObj);
        // If the group is open check children
        if (pGroup->IsOpen())
        {
            for (int i = 0; i < pGroup->GetChildCount(); ++i)
            {
                SelectObjectInRect(pGroup->GetChild(i), view, hc, bSelect);
            }
        }
    }

    if (pObj->HitTestRect(hc))
    {
        if (bSelect)
        {
            SelectObject(pObj);
        }
        else
        {
            UnselectObject(pObj);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
namespace
{
    std::vector<std::string> PyGetAllObjects(const QString& className, const QString& layerName)
    {
        IObjectManager* pObjMgr = GetIEditor()->GetObjectManager();
        CObjectLayer* pLayer = NULL;
        if (layerName.isEmpty() == false)
        {
            pLayer = pObjMgr->GetLayersManager()->FindLayerByName(layerName);
        }
        CBaseObjectsArray objects;
        pObjMgr->GetObjects(objects, pLayer);
        int count = pObjMgr->GetObjectCount();
        std::vector<std::string> result;
        for (int i = 0; i < count; ++i)
        {
            if (className.isEmpty() || objects[i]->GetTypeDescription() == className)
            {
                result.push_back(objects[i]->GetName().toUtf8().data());
            }
        }
        return result;
    }

    std::vector<std::string> PyGetAllLayers()
    {
        CObjectLayerManager* pLayerMgr = GetIEditor()->GetObjectManager()->GetLayersManager();
        std::vector<std::string> result;
        std::vector<CObjectLayer*> layers;
        pLayerMgr->GetLayers(layers);
        for (size_t i = 0; i < layers.size(); ++i)
        {
            result.push_back(layers[i]->GetName().toUtf8().data());
        }
        return result;
    }

    std::vector<std::string> PyGetNamesOfSelectedObjects()
    {
        CSelectionGroup* pSel = GetIEditor()->GetSelection();
        std::vector<std::string> result;
        const int selectionCount = pSel->GetCount();
        result.reserve(selectionCount);
        for (int i = 0; i < selectionCount; i++)
        {
            result.push_back(pSel->GetObject(i)->GetName().toUtf8().data());
        }
        return result;
    }

    void PySelectObject(const char* objName)
    {
        CUndo undo("Select Object");

        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(objName);
        if (pObject)
        {
            GetIEditor()->GetObjectManager()->SelectObject(pObject);
        }
    }

    void PyUnselectObjects(const std::vector<std::string>& names)
    {
        CUndo undo("Unselect Objects");

        std::vector<CBaseObject*> pBaseObjects;
        for (int i = 0; i < names.size(); i++)
        {
            if (!GetIEditor()->GetObjectManager()->FindObject(names[i].c_str()))
            {
                throw std::logic_error((QString("\"") + names[i].c_str() + "\" is an invalid entity.").toUtf8().data());
            }
            pBaseObjects.push_back(GetIEditor()->GetObjectManager()->FindObject(names[i].c_str()));
        }

        for (int i = 0; i < pBaseObjects.size(); i++)
        {
            GetIEditor()->GetObjectManager()->UnselectObject(pBaseObjects[i]);
        }
    }

    void PySelectObjects(const std::vector<std::string>& names)
    {
        CUndo undo("Select Objects");
        CBaseObject* pObject;
        for (size_t i = 0; i < names.size(); ++i)
        {
            pObject = GetIEditor()->GetObjectManager()->FindObject(names[i].c_str());
            if (!pObject)
            {
                throw std::logic_error((QString("\"") + names[i].c_str() + "\" is an invalid entity.").toUtf8().data());
            }
            GetIEditor()->GetObjectManager()->SelectObject(pObject);
        }
    }

    bool PyIsObjectHidden(const char* objName)
    {
        CBaseObject* pObject =  GetIEditor()->GetObjectManager()->FindObject(objName);
        if (!pObject)
        {
            throw std::logic_error((QString("\"") + objName + "\" is an invalid object name.").toUtf8().data());
        }
        return pObject->IsHidden();
    }

    void PyHideAllObjects()
    {
        CBaseObjectsArray baseObjects;
        GetIEditor()->GetObjectManager()->GetObjects(baseObjects);

        if (baseObjects.size() <= 0)
        {
            throw std::logic_error("Objects not found.");
        }

        CUndo undo("Hide All Objects");
        for (int i = 0; i < baseObjects.size(); i++)
        {
            GetIEditor()->GetObjectManager()->HideObject(baseObjects[i], true);
        }
    }

    void PyUnHideAllObjects()
    {
        CUndo undo("Unhide All Objects");
        GetIEditor()->GetObjectManager()->UnhideAll();
    }

    void PyHideObject(const char* objName)
    {
        CUndo undo("Hide Object");

        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(objName);
        if (pObject)
        {
            GetIEditor()->GetObjectManager()->HideObject(pObject, true);
        }
    }

    void PyUnhideObject(const char* objName)
    {
        CUndo undo("Unhide Object");

        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(objName);
        if (pObject)
        {
            GetIEditor()->GetObjectManager()->HideObject(pObject, false);
        }
    }

    void PyFreezeObject(const char* objName)
    {
        CUndo undo("Freeze Object");

        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(objName);
        if (pObject)
        {
            GetIEditor()->GetObjectManager()->FreezeObject(pObject, true);
        }
    }

    void PyUnfreezeObject(const char* objName)
    {
        CUndo undo("Unfreeze Object");

        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(objName);
        if (pObject)
        {
            GetIEditor()->GetObjectManager()->FreezeObject(pObject, false);
        }
    }

    void PyDeleteObject(const char* objName)
    {
        CUndo undo("Delete Object");

        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(objName);
        if (pObject)
        {
            GetIEditor()->GetObjectManager()->DeleteObject(pObject);
        }
    }

    int PyClearSelection()
    {
        CUndo undo("Clear Selection");
        return GetIEditor()->GetObjectManager()->ClearSelection();
    }

    void PyDeleteSelected()
    {
        CUndo undo("Delete Selected Object");
        GetIEditor()->GetObjectManager()->DeleteSelection();
    }

    int PyGetNumSelectedObjects()
    {
        if (CSelectionGroup* pGroup = GetIEditor()->GetObjectManager()->GetSelection())
        {
            return pGroup->GetCount();
        }

        return 0;
    }

    boost::python::tuple PyGetSelectionCenter()
    {
        if (CSelectionGroup* pGroup = GetIEditor()->GetObjectManager()->GetSelection())
        {
            if (pGroup->GetCount() == 0)
            {
                throw std::runtime_error("Nothing selected");
            }

            const Vec3 center = pGroup->GetCenter();
            return boost::python::make_tuple(center.x, center.y, center.z);
        }

        throw std::runtime_error("Nothing selected");
    }

    boost::python::tuple PyGetSelectionAABB()
    {
        if (CSelectionGroup* pGroup = GetIEditor()->GetObjectManager()->GetSelection())
        {
            if (pGroup->GetCount() == 0)
            {
                throw std::runtime_error("Nothing selected");
            }

            const AABB aabb = pGroup->GetBounds();
            return boost::python::make_tuple(aabb.min.x, aabb.min.y, aabb.min.z, aabb.max.x, aabb.max.y, aabb.max.z);
        }

        throw std::runtime_error("Nothing selected");
    }

    QString PyGetEntityGeometryFile(const char* objName)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(objName);
        if (pObject == NULL)
        {
            return "";
        }

        QString result = "";
        if (qobject_cast<CBrushObject*>(pObject))
        {
            result = static_cast<CBrushObject*>(pObject)->GetGeometryFile();
        }
        else if (qobject_cast<CGeomEntity*>(pObject))
        {
            result = static_cast<CGeomEntity*>(pObject)->GetGeometryFile();
        }
        else if (qobject_cast<CEntityObject*>(pObject))
        {
            result = static_cast<CEntityObject*>(pObject)->GetEntityPropertyString("object_Model");
        }

        result = result.toLower();
        result.replace("/", "\\");
        return result;
    }

    void PySetEntityGeometryFile(const char* objName, const char* filePath)
    {
        CUndo undo("Set entity geometry file");
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(objName);
        if (pObject == NULL)
        {
            return;
        }

        if (qobject_cast<CBrushObject*>(pObject))
        {
            static_cast<CBrushObject*>(pObject)->SetGeometryFile(filePath);
        }
        else if (qobject_cast<CGeomEntity*>(pObject))
        {
            static_cast<CGeomEntity*>(pObject)->SetGeometryFile(filePath);
        }
        else if (qobject_cast<CEntityObject*>(pObject))
        {
            static_cast<CEntityObject*>(pObject)->SetEntityPropertyString("object_Model", filePath);
        }
    }

    QString PyGetDefaultMaterial(const char* objName)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(objName);
        if (pObject == NULL)
        {
            return "";
        }

        QString matName = "";
        if (qobject_cast<CBrushObject*>(pObject))
        {
            CBrushObject* pBrush = static_cast<CBrushObject*>(pObject);

            if (pBrush->GetIStatObj() == NULL)
            {
                return "";
            }

            if (pBrush->GetIStatObj()->GetMaterial() == NULL)
            {
                return "";
            }

            matName = pBrush->GetIStatObj()->GetMaterial()->GetName();
        }
        else if (qobject_cast<CEntityObject*>(pObject))
        {
            CEntityObject* pEntity = static_cast<CEntityObject*>(pObject);
            IRenderNode* pEngineNode = pEntity->GetEngineNode();

            if (pEngineNode == NULL)
            {
                return "";
            }

            IStatObj* pEntityStatObj = pEngineNode->GetEntityStatObj();
            if (pEntityStatObj == NULL)
            {
                return "";
            }

            matName = pEntityStatObj->GetMaterial()->GetName();
        }

        matName = matName.toLower();
        matName.replace("/", "\\");
        return matName;
    }

    QString PyGetCustomMaterial(const char* objName)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(objName);
        if (pObject == NULL)
        {
            return "";
        }

        CMaterial* pMtl = pObject->GetMaterial();
        if (pMtl == NULL)
        {
            return "";
        }

        QString matName = pMtl->GetName().toLower();
        matName.replace("/", "\\");
        return matName;
    }

    QString PyGetAssignedMaterial(const char* pName)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(pName);
        if (!pObject)
        {
            throw std::runtime_error("Invalid object name.");
        }

        QString assignedMaterialName = PyGetCustomMaterial(pName);

        if (assignedMaterialName.isEmpty())
        {
            assignedMaterialName = PyGetDefaultMaterial(pName);
        }

        return assignedMaterialName;
    }

    void PySetCustomMaterial(const char* objName, const char* matName)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(objName);
        if (pObject == NULL)
        {
            return;
        }

        CUndo undo("Set Custom Material");
        pObject->SetMaterial(matName);
    }

    std::vector<std::string> PyGetSequencesUsingThis(const char* objName)
    {
        std::vector<std::string> list;
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(objName);
        if (pObject == NULL)
        {
            return list;
        }

        if (qobject_cast<CEntityObject*>(pObject))
        {
            CTrackViewAnimNodeBundle bundle = GetIEditor()->GetSequenceManager()->GetAllRelatedAnimNodes(static_cast<CEntityObject*>(pObject));

            for (unsigned int i = 0; i < bundle.GetCount(); ++i)
            {
                CTrackViewSequence* pSequence = bundle.GetNode(i)->GetSequence();
                if (pSequence)
                {
                    stl::push_back_unique(list, pSequence->GetName());
                }
            }
        }

        return list;
    }

    std::vector<std::string> PyGetFlowGraphsUsingThis(const char* objName)
    {
        std::vector<std::string> list;
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(objName);
        if (pObject == NULL)
        {
            return list;
        }

        if (qobject_cast<CEntityObject*>(pObject))
        {
            CEntityObject* pEntity = static_cast<CEntityObject*>(pObject);
            std::vector<CFlowGraph*> flowgraphs;
            CFlowGraph* pEntityFG = 0;
            FlowGraphHelpers::FindGraphsForEntity(pEntity, flowgraphs, pEntityFG);
            for (size_t i = 0; i < flowgraphs.size(); ++i)
            {
                QString name;
                FlowGraphHelpers::GetHumanName(flowgraphs[i], name);
                list.push_back(name.toStdString());
            }
        }

        return list;
    }

    boost::python::tuple PyGetObjectPosition(const char* pName)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(pName);
        if (!pObject)
        {
            throw std::logic_error((QString("\"") + pName + "\" is an invalid object.").toUtf8().data());
        }
        Vec3 position = pObject->GetPos();
        return boost::python::make_tuple(position.x, position.y, position.z);
    }

    boost::python::tuple PyGetWorldObjectPosition(const char* pName)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(pName);
        if (!pObject)
        {
            throw std::logic_error((QString("\"") + pName + "\" is an invalid object.").toUtf8().data());
        }
        Vec3 position = pObject->GetWorldPos();
        return boost::python::make_tuple(position.x, position.y, position.z);
    }

    void PySetObjectPosition(const char* pName, float fValueX, float fValueY, float fValueZ)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(pName);
        if (!pObject)
        {
            throw std::logic_error((QString("\"") + pName + "\" is an invalid object.").toUtf8().data());
        }
        CUndo undo("Set Object Base Position");
        pObject->SetPos(Vec3(fValueX, fValueY, fValueZ));
    }

    boost::python::tuple PyGetObjectRotation(const char* pName)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(pName);
        if (!pObject)
        {
            throw std::logic_error((QString("\"") + pName + "\" is an invalid object.").toUtf8().data());
        }
        Ang3 ang = RAD2DEG(Ang3(pObject->GetRotation()));
        return boost::python::make_tuple(ang.x, ang.y, ang.z);
    }

    void PySetObjectRotation(const char* pName, float fValueX, float fValueY, float fValueZ)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(pName);
        if (!pObject)
        {
            throw std::logic_error((QString("\"") + pName + "\" is an invalid object.").toUtf8().data());
        }
        CUndo undo("Set Object Rotation");
        pObject->SetRotation(Quat(DEG2RAD(Ang3(fValueX, fValueY, fValueZ))));
    }

    boost::python::tuple PyGetObjectScale(const char* pName)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(pName);
        if (!pObject)
        {
            throw std::logic_error((QString("\"") + pName + "\" is an invalid object.").toUtf8().data());
        }
        Vec3 scaleVec3 = pObject->GetScale();
        return boost::python::make_tuple(scaleVec3.x, scaleVec3.y, scaleVec3.z);
    }

    void PySetObjectScale(const char* pName, float fValueX, float fValueY, float fValueZ)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(pName);
        if (!pObject)
        {
            throw std::logic_error((QString("\"") + pName + "\" is an invalid object.").toUtf8().data());
        }
        CUndo undo("Set Object Scale");
        pObject->SetScale(Vec3(fValueX, fValueY, fValueZ));
    }

    std::vector<std::string> PyGetObjectLayer(const std::vector<std::string>& names)
    {
        std::vector<std::string> result;
        std::set<std::string> tempSet;
        CBaseObjectsArray objectArray;
        GetIEditor()->GetObjectManager()->GetObjects(objectArray);

        for (int i = 0; i < objectArray.size(); i++)
        {
            for (int j = 0; j < names.size(); j++)
            {
                if ((objectArray.at(i))->GetName() == names[j].c_str())
                {
                    tempSet.insert(std::string((objectArray.at(i))->GetLayer()->GetName().toUtf8().data()));
                }
            }
        }
        std::set<std::string>::iterator it;
        for (it = tempSet.begin(); it != tempSet.end(); it++)
        {
            result.push_back(it->c_str());
        }
        return result;
    }

    void PySetObjectLayer(const std::vector<std::string>& names, const char* pLayerName)
    {
        CObjectLayer* pLayer = GetIEditor()->GetObjectManager()->GetLayersManager()->FindLayerByName(pLayerName);
        if (!pLayer)
        {
            throw std::logic_error("Invalid layer.");
        }

        CBaseObjectsArray objectArray;
        GetIEditor()->GetObjectManager()->GetObjects(objectArray);
        bool isLayerChanged(false);
        CUndo undo("Set Object Layer");
        for (int i = 0; i < objectArray.size(); i++)
        {
            for (int j = 0; j < names.size(); j++)
            {
                if ((objectArray.at(i))->GetName() == names[j].c_str() && objectArray.at(i)->SupportsLayers())
                {
                    (objectArray.at(i))->SetLayer(pLayer);
                    isLayerChanged = true;
                }
            }
        }
        if (isLayerChanged)
        {
            GetIEditor()->GetObjectManager()->GetLayersManager()->NotifyLayerChange(pLayer);
        }
    }

    std::vector<std::string> PyGetAllObjectsOnLayer(const char* pLayerName)
    {
        CObjectLayer* pLayer = GetIEditor()->GetObjectManager()->GetLayersManager()->FindLayerByName(pLayerName);
        if (!pLayer)
        {
            throw std::logic_error("Invalid layer.");
        }

        CBaseObjectsArray objectArray;
        GetIEditor()->GetObjectManager()->GetObjects(objectArray);

        std::vector<std::string> vectorObjects;

        for (int i = 0; i < objectArray.size(); i++)
        {
            if ((objectArray.at(i))->GetLayer()->GetName() == QString(pLayerName))
            {
                vectorObjects.push_back(static_cast<std::string>((objectArray.at(i))->GetName().toUtf8().data()));
            }
        }

        return vectorObjects;
    }

    QString PyGetObjectParent(const char* pName)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(pName);
        if (!pObject)
        {
            throw std::runtime_error((QString("\"") + pName + "\" is an invalid object.").toUtf8().data());
        }

        CBaseObject* pParentObject = pObject->GetParent();
        if (!pParentObject)
        {
            return "";
        }
        return pParentObject->GetName();
    }

    std::vector<std::string> PyGetObjectChildren(const char* pName)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(pName);
        if (!pObject)
        {
            throw std::runtime_error((QString("\"") + pName + "\" is an invalid object.").toUtf8().data());
        }
        std::vector<_smart_ptr<CBaseObject> > objectVector;
        std::vector<std::string> result;
        pObject->GetAllChildren(objectVector);
        if (objectVector.empty())
        {
            return result;
        }

        for (std::vector<_smart_ptr<CBaseObject> >::iterator it = objectVector.begin(); it != objectVector.end(); it++)
        {
            result.push_back(static_cast<std::string>(it->get()->GetName().toUtf8().data()));
        }
        return result;
    }

    void PyGenerateCubemap(const char* pObjectName)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(pObjectName);
        if (!pObject)
        {
            throw std::runtime_error("Invalid object.");
        }

        if (pObject->GetTypeName() != "EnvironmentProbe")
        {
            throw std::runtime_error("Invalid environment probe.");
        }
        static_cast<CEnvironementProbeObject*>(pObject)->GenerateCubemap();
    }

    QString PyGetObjectType(const char* pName)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(pName);
        if (!pObject)
        {
            throw std::runtime_error("Invalid object.");
        }

        return pObject->GetTypeName();
    }

    int PyGetObjectLodsCount(const char* pName)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(pName);
        if (!pObject)
        {
            throw std::runtime_error("Invalid object name.");
        }

        if (!qobject_cast<CBrushObject*>(pObject))
        {
            throw std::runtime_error("Invalid object type.");
        }

        IStatObj* pStatObj = pObject->GetIStatObj();
        if (!pObject)
        {
            throw std::runtime_error("Invalid stat object");
        }

        IStatObj::SStatistics objectStats;
        pStatObj->GetStatistics(objectStats);

        return objectStats.nLods;
    }

    void PyRenameObject(const char* pOldName, const char* pNewName)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(pOldName);
        if (!pObject)
        {
            throw std::runtime_error("Could not find object");
        }

        if (strcmp(pNewName, "") == 0 || GetIEditor()->GetObjectManager()->FindObject(pNewName))
        {
            throw std::runtime_error("Invalid object name.");
        }

        CUndo undo("Rename object");
        pObject->SetName(pNewName);
    }

    void PyAttachObject(const char* pParent, const char* pChild, const char* pAttachmentType, const char* pAttachmentTarget)
    {
        CBaseObject* pParentObject = GetIEditor()->GetObjectManager()->FindObject(pParent);
        if (!pParentObject)
        {
            throw std::runtime_error("Could not find parent object");
        }

        CBaseObject* pChildObject = GetIEditor()->GetObjectManager()->FindObject(pChild);
        if (!pChildObject)
        {
            throw std::runtime_error("Could not find child object");
        }

        CEntityObject::EAttachmentType attachmentType = CEntityObject::eAT_Pivot;
        if (strcmp(pAttachmentType, "CharacterBone") == 0)
        {
            attachmentType = CEntityObject::eAT_CharacterBone;
        }
        else if (strcmp(pAttachmentType, "GeomCacheNode") == 0)
        {
            attachmentType = CEntityObject::eAT_GeomCacheNode;
        }
        else if (strcmp(pAttachmentType, "") != 0)
        {
            throw std::runtime_error("Invalid attachment type");
        }

        if (attachmentType != CEntityObject::eAT_Pivot)
        {
            if (qobject_cast<CEntityObject*>(pParentObject) == nullptr || qobject_cast<CEntityObject*>(pChildObject) == nullptr)
            {
                throw std::runtime_error("Both parent and child must be entities if attaching to bone or node");
            }

            if (strcmp(pAttachmentTarget, "") == 0)
            {
                throw std::runtime_error("Please specify a target");
            }

            CEntityObject* pChildEntity = static_cast<CEntityObject*>(pChildObject);
            pChildEntity->SetAttachType(attachmentType);
            pChildEntity->SetAttachTarget(pAttachmentTarget);
        }

        CUndo undo("Attach object");
        pParentObject->AttachChild(pChildObject);
    }

    void PyDetachObject(const char* pObjectName)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(pObjectName);
        if (!pObject)
        {
            throw std::runtime_error("Could not find object");
        }

        if (!pObject->GetParent())
        {
            throw std::runtime_error("Object has no parent");
        }

        CUndo undo("Detach object");
        pObject->DetachThis(true);
    }

    void PyAddEntityLink(const char* pObjectName, const char* pTargetName, const char* pName)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(pObjectName);
        if (!pObject)
        {
            throw std::runtime_error("Could not find object");
        }

        CBaseObject* pTargetObject = GetIEditor()->GetObjectManager()->FindObject(pTargetName);
        if (!pTargetObject)
        {
            throw std::runtime_error("Could not find target object");
        }

        if (qobject_cast<CEntityObject*>(pObject) == nullptr || qobject_cast<CEntityObject*>(pTargetObject) == nullptr)
        {
            throw std::runtime_error("Both object and target must be entities");
        }

        if (strcmp(pName, "") == 0)
        {
            throw std::runtime_error("Please specify a name");
        }

        CUndo undo("Add entity link");
        CEntityObject* pEntity = static_cast<CEntityObject*>(pObject);
        pEntity->AddEntityLink(pName, pTargetObject->GetId());
    }
}

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyGetAllObjects, general, get_all_objects,
    "Gets the name list of all objects of a certain type in a specific layer or in the whole level if an invalid layer name given.",
    "general.get_all_objects(str className, str layerName)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyGetAllLayers, general, get_all_layers,
    "Gets the list of all layer names in the level.",
    "general.get_all_layers()");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyGetNamesOfSelectedObjects, general, get_names_of_selected_objects,
    "Get the name from selected object/objects.",
    "general.get_names_of_selected_objects()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PySelectObject, general, select_object,
    "Selects a specified object.",
    "general.select_object(str objectName)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyUnselectObjects, general, unselect_objects,
    "Unselects a list of objects.",
    "general.unselect_objects(list [str objectName,])");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PySelectObjects, general, select_objects,
    "Selects a list of objects.",
    "general.select_objects(list [str objectName,])");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyIsObjectHidden, general, is_object_hidden,
    "Checks if object is hidden and returns a bool value.",
    "general.is_object_hidden(str objectName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyHideAllObjects, general, hide_all_objects,
    "Hides all objects.",
    "general.hide_all_objects()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyUnHideAllObjects, general, unhide_all_objects,
    "Unhides all object.",
    "general.unhide_all_objects()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyHideObject, general, hide_object,
    "Hides a specified object.",
    "general.hide_object(str objectName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyUnhideObject, general, unhide_object,
    "Unhides a specified object.",
    "general.unhide_object(str objectName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyFreezeObject, general, freeze_object,
    "Freezes a specified object.",
    "general.freeze_object(str objectName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyUnfreezeObject, general, unfreeze_object,
    "Unfreezes a specified object.",
    "general.unfreeze_object(str objectName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyDeleteObject, general, delete_object,
    "Deletes a specified object.",
    "general.delete_object(str objectName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyClearSelection, general, clear_selection,
    "Clears selection.",
    "general.clear_selection()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyDeleteSelected, general, delete_selected,
    "Deletes selected object(s).",
    "general.delete_selected()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyGetNumSelectedObjects, general, get_num_selected,
    "Returns the number of selected objects",
    "general.get_num_selected()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyGetSelectionCenter, general, get_selection_center,
    "Returns the center point of the selection group",
    "general.get_selection_center()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyGetSelectionAABB, general, get_selection_aabb,
    "Returns the aabb of the selection group",
    "general.selection_aabb()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyGetEntityGeometryFile, general, get_entity_geometry_file,
    "Gets the geometry file name of a given entity.",
    "general.get_entity_geometry_file(str geometryName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PySetEntityGeometryFile, general, set_entity_geometry_file,
    "Sets the geometry file name of a given entity.",
    "general.set_entity_geometry_file(str geometryName, str cgfName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyGetDefaultMaterial, general, get_default_material,
    "Gets the default material of a given object geometry.",
    "general.get_default_material(str objectName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyGetCustomMaterial, general, get_custom_material,
    "Gets the user material assigned to a given object geometry.",
    "general.get_custom_material(str objectName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PySetCustomMaterial, general, set_custom_material,
    "Assigns a user material to a given object geometry.",
    "general.set_custom_material(str objectName, str materialName)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyGetSequencesUsingThis, general, get_sequences_using_this,
    "Gets the name list of all sequences which control this object.",
    "general.get_sequences_using_this(str objectName)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyGetFlowGraphsUsingThis, general, get_flowgraphs_using_this,
    "Gets the name list of all flow graphs which control this object.",
    "general.get_flowgraphs_using_this");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyGetObjectPosition, general, get_position,
    "Gets the position of an object.",
    "general.get_position(str objectName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyGetWorldObjectPosition, general, get_world_position,
    "Gets the world position of an object.",
    "general.get_world_position(str objectName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PySetObjectPosition, general, set_position,
    "Sets the position of an object.",
    "general.set_position(str objectName, float xValue, float yValue, float zValue)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyGetObjectRotation, general, get_rotation,
    "Gets the rotation of an object.",
    "general.get_rotation(str objectName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PySetObjectRotation, general, set_rotation,
    "Sets the rotation of the object.",
    "general.set_rotation(str objectName, float xValue, float yValue, float zValue)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyGetObjectScale, general, get_scale,
    "Gets the scale of an object.",
    "general.get_scale(str objectName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PySetObjectScale, general, set_scale,
    "Sets the scale of ab object.",
    "general.set_scale(str objectName, float xValue, float yValue, float zValue)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyGetObjectLayer, general, get_object_layer,
    "Gets the name of the layer of an object.",
    "general.get_object_layer(list [str objectName,])");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PySetObjectLayer, general, set_object_layer,
    "Moves an object to an other layer.",
    "general.set_object_layer(list [str objectName,], str layerName)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyGetAllObjectsOnLayer, general, get_all_objects_of_layer,
    "Gets all objects of a layer.",
    "general.get_all_objects_of_layer(str layerName)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyGetObjectParent, general, get_object_parent,
    "Gets parent name of an object.",
    "general.get_object_parent(str objectName)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyGetObjectChildren, general, get_object_children,
    "Gets children names of an object.",
    "general.get_object_children(str objectName)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyGenerateCubemap, general, generate_cubemap,
    "Generates a cubemap (only for environment probes).",
    "general.generate_cubemap(str environmentProbeName)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyGetObjectType, general, get_object_type,
    "Gets the type of an object as a string.",
    "general.get_object_type(str objectName)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyGetAssignedMaterial, general, get_assigned_material,
    "Gets the name of assigned material.",
    "general.get_assigned_material(str objectName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyGetObjectLodsCount, general, get_object_lods_count,
    "Gets the number of lods of the material of an object.",
    "general.get_object_lods_count(str objectName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyRenameObject, general, rename_object,
    "Renames object with oldObjectName to newObjectName",
    "general.rename_object(str oldObjectName, str newObjectName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyAttachObject, general, attach_object,
    "Attaches object with childObjectName to parentObjectName. If attachmentType is 'CharacterBone' or 'GeomCacheNode' attachmentTarget specifies the bone or node path",
    "general.attach_object(str parentObjectName, str childObjectName, str attachmentType, str attachmentTarget)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyDetachObject, general, detach_object,
    "Detaches object from its parent",
    "general.detach_object(str object)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyAddEntityLink, general, add_entity_link,
    "Adds an entity link to objectName to targetName with name",
    "general.add_entity_link(str objectName, str targetName, str name)");

REGISTER_PYTHON_ENUM_BEGIN(ObjectType, general, object_type)
REGISTER_PYTHON_ENUM_ITEM(OBJTYPE_GROUP, group)
REGISTER_PYTHON_ENUM_ITEM(OBJTYPE_TAGPOINT, tagpoint)
REGISTER_PYTHON_ENUM_ITEM(OBJTYPE_AIPOINT, aipoint)
REGISTER_PYTHON_ENUM_ITEM(OBJTYPE_ENTITY, entity)
REGISTER_PYTHON_ENUM_ITEM(OBJTYPE_SHAPE, shape)
REGISTER_PYTHON_ENUM_ITEM(OBJTYPE_VOLUME, volume)
REGISTER_PYTHON_ENUM_ITEM(OBJTYPE_BRUSH, brush)
REGISTER_PYTHON_ENUM_ITEM(OBJTYPE_PREFAB, prefab)
REGISTER_PYTHON_ENUM_ITEM(OBJTYPE_SOLID, solid)
REGISTER_PYTHON_ENUM_ITEM(OBJTYPE_CLOUD, cloud)
REGISTER_PYTHON_ENUM_ITEM(OBJTYPE_CLOUDGROUP, cloudgroup)
REGISTER_PYTHON_ENUM_ITEM(OBJTYPE_ROAD, road)
REGISTER_PYTHON_ENUM_ITEM(OBJTYPE_OTHER, other)
REGISTER_PYTHON_ENUM_ITEM(OBJTYPE_DECAL, decal)
REGISTER_PYTHON_ENUM_ITEM(OBJTYPE_DISTANCECLOUD, distanceclound)
REGISTER_PYTHON_ENUM_ITEM(OBJTYPE_TELEMETRY, telemetry)
REGISTER_PYTHON_ENUM_ITEM(OBJTYPE_REFPICTURE, refpicture)
REGISTER_PYTHON_ENUM_ITEM(OBJTYPE_GEOMCACHE, geomcache)
REGISTER_PYTHON_ENUM_ITEM(OBJTYPE_ANY, any)
REGISTER_PYTHON_ENUM_END
