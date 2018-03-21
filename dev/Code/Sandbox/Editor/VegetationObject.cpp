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
#include "VegetationMap.h"
#include "VegetationTool.h"
#include "Material/MaterialManager.h"
#include "ErrorReport.h"

#include "Terrain/Heightmap.h"

#include "I3DEngine.h"

#include "VegetationObject.h"

#include "Util/BoostPythonHelpers.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CVegetationObject::CVegetationObject(int id)
{
    m_id = id;

    m_statObj = 0;
    m_objectSize = 1;
    m_numInstances = 0;
    m_bSelected = false;
    m_bHidden = false;
    m_index = 0;
    m_bInUse = true;

    m_bVarIgnoreChange = false;

    m_category = tr("Default");

    // Int vars.
    mv_size = 1;
    mv_hmin = GetIEditor()->GetHeightmap()->GetOceanLevel();
    mv_hmax = 4096;
    mv_slope_min = 0;
    mv_slope_max = 255;

    mv_stiffness = 0.5f;
    mv_damping = 2.5f;
    mv_variance = 0.6f;
    mv_airResistance = 1.f;

    mv_density = 1;
    mv_bending = 0.0f;
    mv_sizevar = 0;
    mv_rotationRangeToTerrainNormal = 0;
    mv_alignToTerrain = false;
    mv_alignToTerrainCoefficient = 0.f;
    mv_castShadows = false;
    mv_castShadowMinSpec = CONFIG_LOW_SPEC;
    mv_recvShadows = false;
    mv_precalcShadows = false;
    mv_alphaBlend = false;
    mv_hideable = 0;
    mv_playerHideable = 0;
    mv_affectedByTerrain = true;
    mv_aiRadius = -1.0f;
    mv_SpriteDistRatio = 1;
    mv_LodDistRatio = 1;
    mv_MaxViewDistRatio = 1;
    mv_ShadowDistRatio = 1;
    mv_brightness = 1;
    mv_UseSprites = true;
    mv_layerFrozen = false;
    mv_minSpec = 0;
    mv_allowIndoor = false;
    mv_autoMerged = false;

    m_guid = QUuid::createUuid();

    mv_hideable.AddEnumItem("None", 0);
    mv_hideable.AddEnumItem("Hideable", 1);
    mv_hideable.AddEnumItem("Secondary", 2);

    mv_playerHideable.AddEnumItem("None",       IStatInstGroup::ePlayerHideable_None);
    mv_playerHideable.AddEnumItem("Standing",   IStatInstGroup::ePlayerHideable_High);
    mv_playerHideable.AddEnumItem("Crouching",  IStatInstGroup::ePlayerHideable_Mid);
    mv_playerHideable.AddEnumItem("Proned",     IStatInstGroup::ePlayerHideable_Low);

    mv_minSpec.AddEnumItem("All", 0);
    mv_minSpec.AddEnumItem("Low", CONFIG_LOW_SPEC);
    mv_minSpec.AddEnumItem("Medium", CONFIG_MEDIUM_SPEC);
    mv_minSpec.AddEnumItem("High", CONFIG_HIGH_SPEC);
    mv_minSpec.AddEnumItem("VeryHigh", CONFIG_VERYHIGH_SPEC);

    mv_castShadowMinSpec.AddEnumItem("Never",     END_CONFIG_SPEC_ENUM);
    mv_castShadowMinSpec.AddEnumItem("Low",       CONFIG_LOW_SPEC);
    mv_castShadowMinSpec.AddEnumItem("Medium",    CONFIG_MEDIUM_SPEC);
    mv_castShadowMinSpec.AddEnumItem("High",      CONFIG_HIGH_SPEC);
    mv_castShadowMinSpec.AddEnumItem("VeryHigh",  CONFIG_VERYHIGH_SPEC);

    mv_castShadows->SetFlags(mv_castShadows->GetFlags() | IVariable::UI_INVISIBLE);
    mv_castShadowMinSpec->SetFlags(mv_castShadowMinSpec->GetFlags() | IVariable::UI_UNSORTED);

    // There is a restriction internally for a max size scale of 4, so enforce
    // that here until the vegetation system is componentized
    mv_size->SetLimits(1, 4);

    AddVariable(mv_size, "Size", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_sizevar, "SizeVar", "+ - SizeVar", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_rotation, "RandomRotation", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_rotationRangeToTerrainNormal, "RotationRangeToTerrainNormal", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_alignToTerrain, "AlignToTerrain", functor(*this, &CVegetationObject::OnVarChange));
    mv_alignToTerrain->SetFlags(mv_alignToTerrain->GetFlags() | IVariable::UI_INVISIBLE);
    AddVariable(mv_alignToTerrainCoefficient, "AlignToTerrainCoefficient", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_useTerrainColor, "UseTerrainColor", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_allowIndoor, "AllowIndoor", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_bending, "Bending", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_hideable, "Hideable", "AI Occluder", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_playerHideable, "PlayerHideable", "Player Occluder", functor(*this, &CVegetationObject::OnVarChange));

    AddVariable(mv_affectedByBrushes, "GrowOnBrushes", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_affectedByTerrain, "GrowOnTerrain", functor(*this, &CVegetationObject::OnVarChange));

    AddVariable(mv_autoMerged, "AutoMerged", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_stiffness, "Stiffness", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_damping, "Damping", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_variance, "Variance", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_airResistance, "AirResistance", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_aiRadius, "AIRadius", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_brightness, "Brightness", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_density, "Density", "Density (m)", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_hmin, VEGETATION_ELEVATION_MIN, "Altitude Min (m)", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_hmax, VEGETATION_ELEVATION_MAX, "Altitude Max (m)", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_slope_min, VEGETATION_SLOPE_MIN, functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_slope_max, VEGETATION_SLOPE_MAX, functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_castShadows, "CastShadow", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_castShadowMinSpec, "CastShadowMinSpec", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_recvShadows, "RecvShadow", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_alphaBlend, "AlphaBlend", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_SpriteDistRatio, "SpriteDistRatio", "Sprite Distance", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_LodDistRatio, "LodDistRatio", "LOD Distance", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_MaxViewDistRatio, "MaxViewDistRatio", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_material, "Material", functor(*this, &CVegetationObject::OnMaterialChange), IVariable::DT_MATERIAL);
    AddVariable(mv_UseSprites, "UseSprites", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_minSpec, "MinSpec", functor(*this, &CVegetationObject::OnVarChange));

    AddVariable(mv_layerFrozen, "Frozen", functor(*this, &CVegetationObject::OnVarChange));
    AddVariable(mv_layerWet, "Layer_Wet", "Wet", functor(*this, &CVegetationObject::OnVarChange));

    AddVariable(mv_fileName, "Object", "Object", functor(*this, &CVegetationObject::OnFileNameChange), IVariable::DT_OBJECT);
}

CVegetationObject::~CVegetationObject()
{
    VegetationObjectBus::Handler::BusDisconnect();

    if (m_statObj)
    {
        m_statObj->Release();
    }
    if (m_id >= 0)
    {
        IStatInstGroup grp;
        GetIEditor()->GetSystem()->GetI3DEngine()->SetStatInstGroup(m_id, grp);
    }
    if (m_pMaterial)
    {
        m_pMaterial = NULL;
    }
}

void CVegetationObject::SetFileName(const QString& strFileName)
{
    mv_fileName->Set(strFileName);
}

//////////////////////////////////////////////////////////////////////////
void CVegetationObject::SetCategory(const QString& category)
{
    m_category = category;
};

//////////////////////////////////////////////////////////////////////////
void CVegetationObject::UnloadObject()
{
    VegetationObjectBus::Handler::BusDisconnect();
    if (m_statObj)
    {
        m_statObj->Release();
    }
    m_statObj = 0;
    m_objectSize = 1;

    SetEngineParams();
}

//////////////////////////////////////////////////////////////////////////
void CVegetationObject::LoadObject()
{
    m_objectSize = 1;
    QString filename;
    mv_fileName->Get(filename);
    if (m_statObj == 0 && !filename.isEmpty())
    {
        GetIEditor()->GetErrorReport()->SetCurrentFile(filename);
        if (m_statObj)
        {
            VegetationObjectBus::Handler::BusDisconnect();
            m_statObj->Release();
            m_statObj = 0;
        }
        m_statObj = GetIEditor()->GetSystem()->GetI3DEngine()->LoadStatObjUnsafeManualRef(filename.toUtf8().data(), NULL, NULL, false);
        if (m_statObj)
        {
            VegetationObjectBus::Handler::BusConnect(m_statObj);
            m_statObj->AddRef();
            Vec3 min = m_statObj->GetBoxMin();
            Vec3 max = m_statObj->GetBoxMax();
            m_objectSize = qMax(max.x - min.x, max.y - min.y);

            Validate(*GetIEditor()->GetErrorReport());
        }
        GetIEditor()->GetErrorReport()->SetCurrentFile("");
    }
    SetEngineParams();
}

//////////////////////////////////////////////////////////////////////////
void CVegetationObject::SetHidden(bool bHidden)
{
    m_bHidden = bHidden;
    SetInUse(!bHidden);

    GetIEditor()->SetModifiedFlag();
    GetIEditor()->SetModifiedModule(eModifiedTerrain);
    /*
    for (int i = 0; i < GetObjectCount(); i++)
    {
        CVegetationObject *obj = GetObject(i);
        obj->SetInUse( !bHidden );
    }
    */
    SetEngineParams();
}

//////////////////////////////////////////////////////////////////////////
void CVegetationObject::CopyFrom(const CVegetationObject& o)
{
    CopyVariableValues(const_cast<CVegetationObject*>(&o));

    m_bInUse = o.m_bInUse;
    m_bHidden = o.m_bHidden;
    m_category = o.m_category;

    if (m_statObj == 0)
    {
        LoadObject();
    }
    GetIEditor()->SetModifiedFlag();
    GetIEditor()->SetModifiedModule(eModifiedTerrain);
    SetEngineParams();
}

//////////////////////////////////////////////////////////////////////////
void CVegetationObject::OnVarChange(IVariable* var)
{
    if (m_bVarIgnoreChange)
    {
        return;
    }
    SetEngineParams();
    GetIEditor()->SetModifiedFlag();
    GetIEditor()->SetModifiedModule(eModifiedTerrain);

    if (var == mv_hideable.GetVar() || var == mv_alignToTerrainCoefficient.GetVar() || var == mv_autoMerged.GetVar() || mv_autoMerged)
    {
        // Reposition this object on vegetation map.
        GetIEditor()->GetVegetationMap()->RepositionObject(this);
        CEditTool* pTool = GetIEditor()->GetEditTool();
        if (pTool && qobject_cast<CVegetationTool*>(pTool))
        {
            CVegetationTool* pVegetationTool = static_cast<CVegetationTool*>(pTool);
            pVegetationTool->UpdateTransformManipulator();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationObject::UpdateMaterial()
{
    // Update CGF material
    {
        CMaterial* pMaterial = NULL;
        QString mtlName = mv_material;
        if (!mtlName.isEmpty())
        {
            pMaterial = (CMaterial*)GetIEditor()->GetMaterialManager()->LoadMaterial(mtlName);
        }
        if (pMaterial != m_pMaterial)
        {
            m_pMaterial = pMaterial;
        }
    }

    // Update ground decal material
    {
        CMaterial* pMaterial = NULL;
        QString mtlName = mv_materialGroundDecal;
        if (!mtlName.isEmpty())
        {
            pMaterial = (CMaterial*)GetIEditor()->GetMaterialManager()->LoadMaterial(mtlName);
        }
        if (pMaterial != m_pMaterialGroundDecal)
        {
            m_pMaterialGroundDecal = pMaterial;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationObject::OnMaterialChange(IVariable* var)
{
    if (m_bVarIgnoreChange)
    {
        return;
    }

    UpdateMaterial();
    SetEngineParams();
    GetIEditor()->SetModifiedFlag();
    GetIEditor()->SetModifiedModule(eModifiedTerrain);
}

//////////////////////////////////////////////////////////////////////////
void CVegetationObject::OnFileNameChange(IVariable* var)
{
    if (m_bVarIgnoreChange)
    {
        return;
    }

    UnloadObject();
    if (CFileUtil::FileExists(mv_fileName))
    {
        LoadObject();
    }
    else
    {
        QString value;
        mv_fileName->Get(value);
        AZ_Error("Vegetation", false, "'%s' File not found.", value);
    }
    GetIEditor()->SetModifiedFlag();
    GetIEditor()->SetModifiedModule(eModifiedTerrain);
}
//////////////////////////////////////////////////////////////////////////
void CVegetationObject::SetEngineParams()
{
    I3DEngine* engine = GetIEditor()->GetSystem()->GetI3DEngine();
    if (!engine)
    {
        return;
    }

    IStatInstGroup grp;
    if (!m_bHidden)
    {
        if (m_numInstances > 0 || !m_terrainLayers.empty())
        {
            // Only set object when vegetation actually used in level.
            grp.pStatObj = m_statObj;
        }
    }
    grp.bHideability = mv_hideable == 1;
    grp.bHideabilitySecondary = mv_hideable == 2;
    grp.fBending = mv_bending;
    grp.nCastShadowMinSpec = mv_castShadowMinSpec;
    grp.bRecvShadow = mv_recvShadows;
    grp.bUseAlphaBlending = mv_alphaBlend;
    grp.fSpriteDistRatio = mv_SpriteDistRatio;
    grp.fLodDistRatio = mv_LodDistRatio;
    grp.fShadowDistRatio = mv_ShadowDistRatio;
    grp.fMaxViewDistRatio = mv_MaxViewDistRatio;
    grp.fBrightness = mv_brightness;
    grp.bAutoMerged = mv_autoMerged;
    grp.fStiffness = mv_stiffness;
    grp.fDamping = mv_damping;
    grp.fVariance = mv_variance;
    grp.fAirResistance = mv_airResistance;

    grp.nPlayerHideable = mv_playerHideable;

    int nMinSpec = mv_minSpec;
    grp.minConfigSpec = (ESystemConfigSpec)nMinSpec;
    grp.pMaterial = 0;
    grp.nMaterialLayers = 0;

    if (m_pMaterial)
    {
        grp.pMaterial = m_pMaterial->GetMatInfo();
    }

    // Set material layer flags
    uint8 nMaterialLayersFlags = 0;

    // Activate frozen layer
    if (mv_layerFrozen)
    {
        nMaterialLayersFlags |= MTL_LAYER_FROZEN;
        // temporary fix: disable bending atm for material layers
        grp.fBending = 0.0f;
    }
    if (mv_layerWet)
    {
        nMaterialLayersFlags |= MTL_LAYER_WET;
    }

    grp.nMaterialLayers = nMaterialLayersFlags;
    grp.bRandomRotation = mv_rotation;
    grp.nRotationRangeToTerrainNormal = mv_rotationRangeToTerrainNormal;
    grp.bUseTerrainColor = mv_useTerrainColor;
    grp.bAllowIndoor = mv_allowIndoor;
    grp.fAlignToTerrainCoefficient = mv_alignToTerrainCoefficient;

    // pass parameters for procedural objects placement
    grp.fDensity                                        = mv_density;
    grp.fElevationMax                               = mv_hmax;
    grp.fElevationMin                               = mv_hmin;
    grp.fSize                                               = mv_size;
    grp.fSizeVar                                        = mv_sizevar;
    grp.fSlopeMax                                       = mv_slope_max;
    grp.fSlopeMin                                       = mv_slope_min;


    if (mv_hideable == 2)
    {
        grp.bHideability = false;
        grp.bHideabilitySecondary = true;
    }
    else if (mv_hideable == 1)
    {
        grp.bHideability = true;
        grp.bHideabilitySecondary = false;
    }
    else
    {
        grp.bHideability = false;
        grp.bHideabilitySecondary = false;
    }

    engine->SetStatInstGroup(m_id, grp);
    float r = mv_aiRadius;
    if ((m_statObj) && (m_statObj->GetAIVegetationRadius() != r))
    {
        // we need to update the statobj's AI vegetation radius, we are the canonical source of this value.
        // becuase AI vegetation radius of the STAT OBJ is always used by the nav mesh generator, not the one from here
        // then changing this value has the effect of changing all other objects using the same statobj
        m_statObj->SetAIVegetationRadius(r);
        EBUS_EVENT_ID(m_statObj, VegetationObjectBus, OnStatObjVegetationDataChanged);
    }
}

void CVegetationObject::OnStatObjVegetationDataChanged()
{
    if (m_statObj)
    {
        SetAIRadius(m_statObj->GetAIVegetationRadius());
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationObject::Serialize(const XmlNodeRef& node, bool bLoading)
{
    m_bVarIgnoreChange = true;
    CVarObject::Serialize(node, bLoading);
    m_bVarIgnoreChange = false;
    if (bLoading)
    {
        // Loading
        QString fileName;
        node->getAttr("FileName", fileName);
        fileName = PathUtil::ToUnixPath(fileName.toUtf8().data()).c_str();
        node->getAttr("GUID", m_guid);
        node->getAttr("Hidden", m_bHidden);
        node->getAttr("Category", m_category);

        m_terrainLayers.clear();
        XmlNodeRef layers = node->findChild("TerrainLayers");
        if (layers)
        {
            for (int i = 0; i < layers->getChildCount(); i++)
            {
                QString name;
                XmlNodeRef layer = layers->getChild(i);
                if (layer->getAttr("Name", name) && !name.isEmpty())
                {
                    m_terrainLayers.push_back(name);
                }
            }
        }

        SetFileName(fileName);

        UnloadObject();
        LoadObject();
        UpdateMaterial();
        SetEngineParams();

        if ((mv_castShadowMinSpec == END_CONFIG_SPEC_ENUM) && mv_castShadows) // backwards compatibility check
        {
            mv_castShadowMinSpec = CONFIG_LOW_SPEC;
            mv_castShadows = false;
        }
        if (mv_alignToTerrain) // backwards compatibility check
        {
            mv_alignToTerrainCoefficient = 1.f;
            mv_alignToTerrain = false;
        }
    }
    else
    {
        // Save.
        node->setAttr("Id", m_id);
        node->setAttr("FileName", GetFileName().toUtf8().data());
        node->setAttr("GUID", m_guid);
        node->setAttr("Hidden", m_bHidden);
        node->setAttr("Index", m_index);
        node->setAttr("Category", m_category.toUtf8().data());

        if (!m_terrainLayers.empty())
        {
            XmlNodeRef layers = node->newChild("TerrainLayers");
            for (int i = 0; i < m_terrainLayers.size(); i++)
            {
                XmlNodeRef layer = layers->newChild("Layer");
                layer->setAttr("Name", m_terrainLayers[i].toUtf8().data());
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationObject::Validate(IErrorReport& report)
{
    if (m_statObj && m_statObj->IsDefaultObject())
    {
        // File Not found.
        CErrorRecord err;
        err.error = tr("Geometry file %1 for Vegetation Object not found").arg(GetFileName());
        err.file = GetFileName();
        err.severity = CErrorRecord::ESEVERITY_WARNING;
        err.flags = CErrorRecord::FLAG_NOFILE;
        report.ReportError(err);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CVegetationObject::IsUsedOnTerrainLayer(const QString& layer)
{
    return stl::find(m_terrainLayers, layer);
}

//////////////////////////////////////////////////////////////////////////
int CVegetationObject::GetTextureMemUsage(ICrySizer* pSizer)
{
    int nSize = 0;
    if (m_statObj != NULL && m_statObj->GetRenderMesh() != NULL)
    {
        _smart_ptr<IMaterial> pMtl = (m_pMaterial) ? m_pMaterial->GetMatInfo() : NULL;
        if (!pMtl)
        {
            pMtl = m_statObj->GetMaterial();
        }
        if (pMtl)
        {
            nSize = m_statObj->GetRenderMesh()->GetTextureMemoryUsage(pMtl, pSizer);
        }
    }
    return nSize;
}

//////////////////////////////////////////////////////////////////////////
void CVegetationObject::SetSelected(bool bSelected)
{
    bool bSendEvent = bSelected != m_bSelected;
    m_bSelected = bSelected;
    if (bSendEvent)
    {
        GetIEditor()->Notify(eNotify_OnVegetationObjectSelection);
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationObject::OnConfigSpecChange()
{
    bool bHiddenBySpec = false;
    int nMinSpec = mv_minSpec;

    if (gSettings.bApplyConfigSpecInEditor)
    {
        if (nMinSpec != 0 && gSettings.editorConfigSpec != 0 && nMinSpec > gSettings.editorConfigSpec)
        {
            bHiddenBySpec = true;
        }
    }

    // Hide/unhide object depending if it`s needed for this spec.
    if (bHiddenBySpec && !IsHidden())
    {
        GetIEditor()->GetVegetationMap()->HideObject(this, true);
    }
    else if (!bHiddenBySpec && IsHidden())
    {
        GetIEditor()->GetVegetationMap()->HideObject(this, false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CVegetationObject::SetNumInstances(int numInstances)
{
    if (m_numInstances == 0 && numInstances > 0)
    {
        m_numInstances = numInstances;
        // Object is really used.
        SetEngineParams();
    }
    else
    {
        m_numInstances = numInstances;
    }
}

//////////////////////////////////////////////////////////////////////////
bool CVegetationObject::IsHidden() const
{
    if (m_bHidden)
    {
        return true;
    }

    ICVar* piVariable = gEnv->pConsole->GetCVar("e_Vegetation");
    if (piVariable)
    {
        return (piVariable->GetIVal() != 0) ? FALSE : TRUE;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
namespace
{
    // Handle all Vegetation Object Getters
    //  -> Single Object by Name or ID
    //  -> All Vegetation Objects by Name
    //  -> All Vegetation Objects loaded.
    boost::python::list PyGetVegetation(const QString& vegetationName = "", bool loadedOnly = false)
    {
        boost::python::list result;
        CVegetationMap* pVegMap = GetIEditor()->GetVegetationMap();

        if (vegetationName.isEmpty())
        {
            CSelectionGroup* pSel = GetIEditor()->GetSelection();

            if (pSel->GetCount() == 0)
            {
                for (int i = 0; i < pVegMap->GetObjectCount(); i++)
                {
                    if (loadedOnly && pVegMap->GetObject(i)->IsHidden())
                    {
                        continue;
                    }

                    result.append(PyScript::CreatePyGameVegetation(pVegMap->GetObject(i)));
                }
            }
            else
            {
                for (int i = 0; i < pSel->GetCount(); i++)
                {
                    if (pVegMap->GetObject(i)->IsSelected())
                    {
                        result.append(PyScript::CreatePyGameVegetation(pVegMap->GetObject(i)));
                    }
                }
            }
        }
        else
        {
            for (int i = 0; i < pVegMap->GetObjectCount(); i++)
            {
                if (QString::compare(vegetationName, pVegMap->GetObject(i)->GetFileName()) == 0)
                {
                    if (loadedOnly && pVegMap->GetObject(i)->IsHidden())
                    {
                        continue;
                    }

                    result.append(PyScript::CreatePyGameVegetation(pVegMap->GetObject(i)));
                }
            }
        }

        return result;
    }
}

BOOST_PYTHON_FUNCTION_OVERLOADS(pyGetVegetationOverload, PyGetVegetation, 0, 2);
REGISTER_PYTHON_OVERLOAD_COMMAND(PyGetVegetation, vegetation, get_vegetation, pyGetVegetationOverload,
    "Get all, selected, specific name, loaded vegetation objects in the current level.",
    "general.get_vegetation(str vegetationName=\'\', bool loadedOnly=False)");

#include <VegetationObject.moc>