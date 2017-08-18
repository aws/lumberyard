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

#pragma once
#ifndef CRYINCLUDE_EDITOR_VEGETATIONOBJECT_H
#define CRYINCLUDE_EDITOR_VEGETATIONOBJECT_H

#include <AzCore/EBus/EBus.h>

#define VEGETATION_ELEVATION_MIN "ElevationMin"
#define VEGETATION_ELEVATION_MAX "ElevationMax"
#define VEGETATION_SLOPE_MIN "SlopeMin"
#define VEGETATION_SLOPE_MAX "SlopeMax"


class CMaterial;
class CVegetationMap;

/** This is description of single static vegetation object instance.
*/
struct CVegetationInstance
{
    CVegetationInstance* next;  //! Next object instance
    CVegetationInstance* prev;  //! Prev object instance.

    Vec3    pos;                                    //!< Instance position.
    float scale;                                //!< Instance scale factor.
    CVegetationObject* object;  //!< Object of this instance.
    IRenderNode* pRenderNode;       // Render node of this object.
    IDecalRenderNode* pRenderNodeGroundDecal; // Decal under vegetation.
    uint8 brightness;                       //!< Brightness of this object.
    float angle;                                //!< Instance rotation.
    float angleX;         //!< Angle X for rotation
    float angleY;         //!< Angle Y for rotation

    int m_refCount;                             //!< Number of references to this vegetation instance.

    bool                            m_boIsSelected;

    //! Add new refrence to this object.
    void AddRef() { m_refCount++; };

    //! Release refrence to this object.
    //! when reference count reaches zero, object is deleted.
    void Release()
    {
        if ((--m_refCount) <= 0)
        {
            delete this;
        }
    }
private:
    // Private destructor, to not be able to delete it explicitly.
    ~CVegetationInstance() { };
};


// Vegetation objects that share the same stat obj instance actually have to synchronize with each other
// on any values that come from that same stat object
class VegetationObjectEvents : public AZ::EBusTraits
{
public:
    virtual ~VegetationObjectEvents() = default;

    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
    typedef IStatObj* BusIdType;

    virtual void OnStatObjVegetationDataChanged() = 0;
};

using VegetationObjectBus = AZ::EBus<VegetationObjectEvents>;

//////////////////////////////////////////////////////////////////////////
/** Description of vegetation object which can be place on the map.
*/
class CVegetationObject
    : public CVarObject, public VegetationObjectBus::Handler
{
    Q_OBJECT
public:
    CVegetationObject(int id);
    virtual ~CVegetationObject();

    void Serialize(const XmlNodeRef& node, bool bLoading);

    // Member access
    QString GetFileName() const { mv_fileName->Get(m_fileNameTmp); return m_fileNameTmp; };
    void SetFileName(const QString& strFileName);

    //! Get this vegetation object GUID.
    REFGUID GetGUID() const { return m_guid; }

    //! Set current object category.
    void SetCategory(const QString& category);
    //! Get object's category.
    QString GetCategory() const { return m_category; };

    //! Temporary unload IStatObj.
    void UnloadObject();
    //! Load IStatObj again.
    void LoadObject();

    float GetSize() { return mv_size; };
    void SetSize(float fSize) { mv_size = fSize; };

    float GetSizeVar() { return mv_sizevar; };
    void SetSizeVar(float fSize) { mv_sizevar = fSize; };

    //! Get actual size of object.
    float GetObjectSize() const { return m_objectSize; }

    //////////////////////////////////////////////////////////////////////////
    // Operators.
    //////////////////////////////////////////////////////////////////////////
    void SetElevation(float min, float max) { mv_hmin = min; mv_hmax = max; };
    void SetSlope(float min, float max) { mv_slope_min = min; mv_slope_max = max; };
    void SetDensity(float dens) { mv_density = dens; };
    void SetSelected(bool bSelected);
    void SetHidden(bool bHidden);
    void SetNumInstances(int numInstances);
    void SetBending(float fBending) { mv_bending = fBending; }
    void SetFrozen(bool bFrozen) { mv_layerFrozen = bFrozen; }
    void SetInUse(bool bNewState) { m_bInUse = bNewState; };

    //////////////////////////////////////////////////////////////////////////
    // Accessors.
    //////////////////////////////////////////////////////////////////////////
    float GetElevationMin() const { return mv_hmin; };
    float GetElevationMax() const { return mv_hmax; };
    float GetSlopeMin() const { return mv_slope_min; };
    float GetSlopeMax() const { return mv_slope_max; };
    float GetDensity() const { return mv_density; };
    bool    IsHidden() const;
    bool    IsSelected() const { return m_bSelected; }
    int     GetNumInstances() { return m_numInstances; };
    float GetBending() { return mv_bending; };
    bool  GetFrozen() const { return mv_layerFrozen; };
    bool    GetInUse() { return m_bInUse; };
    bool  IsAutoMerged() { return mv_autoMerged; }

    //////////////////////////////////////////////////////////////////////////
    void SetIndex(int i) { m_index = i; };
    int GetIndex() const { return m_index; };

    //! Set is object cast shadow.
    void SetCastShadow(bool on) { mv_castShadows = on; }
    //! Check if object casting shadow.
    bool IsCastShadow() const { return mv_castShadows; };

    //! Set if object recieve shadow.
    void SetReceiveShadow(bool on) { mv_recvShadows = on; }
    //! Check if object recieve shadow.
    bool IsReceiveShadow() const { return mv_recvShadows; };

    //////////////////////////////////////////////////////////////////////////
    void SetPrecalcShadow(bool on) { mv_precalcShadows = on; }
    bool IsPrecalcShadow() const { return mv_precalcShadows; };

    //! Set to true if characters can hide behind this object.
    void SetHideable(bool on) { mv_hideable = on; }
    //! Check if chracters can hide behind this object.
    bool IsHideable() const { return mv_hideable; };

    //! if < 0 then AI calculates the navigation radius automatically
    //! if >= 0 then AI uses this radius
    float GetAIRadius() const {return mv_aiRadius; }
    void SetAIRadius(float radius) {mv_aiRadius = radius; }

    bool IsRandomRotation() const { return mv_rotation; }
    int GetRotationRangeToTerrainNormal() const { return mv_rotationRangeToTerrainNormal; }
    bool IsUseSprites() const { return mv_UseSprites; }
    bool IsAlignToTerrain() const { return mv_alignToTerrainCoefficient != 0.f; }
    bool IsUseTerrainColor() const { return mv_useTerrainColor; }
    bool IsMarkedForIntegration() const { return mv_markedForIntegration; }
    bool IsAffectedByBrushes() const { return mv_affectedByBrushes; }
    bool IsAffectedByTerrain() const { return mv_affectedByTerrain; }

    //////////////////////////////////////////////////////////////////////////
    void SetAlphaBlend(bool bEnable) { mv_alphaBlend = bEnable; }
    bool IsAlphaBlend() const { return mv_alphaBlend; };

    //! Copy all parameters from specified vegetation object.
    void CopyFrom(const CVegetationObject& o);

    //! Return pointer to static object.
    IStatObj*   GetObject() { return m_statObj; };

    //! Return true when the brush can paint on a location with the supplied parameters
    bool IsPlaceValid(float height, float slope) const;

    //! Calculate variable size for this object.
    float CalcVariableSize() const;

    //! Id of this object.
    int GetId() const { return m_id; };
    void SetId(int nId) {m_id = nId; SetEngineParams(); };

    void Validate(IErrorReport& report);

    void GetTerrainLayers(QStringList& layers) { layers = m_terrainLayers; };
    void SetTerrainLayers(const QStringList& layers) { m_terrainLayers = layers; };
    bool IsUsedOnTerrainLayer(const QString& layer);

    // Handle changing of the current configuration spec.
    void OnConfigSpecChange();

    // Return texture memory used by this object.
    int GetTextureMemUsage(ICrySizer* pSizer = NULL);

    virtual CMaterial* GetMaterial() const { return m_pMaterial; }

protected:
    Q_DISABLE_COPY(CVegetationObject)
    void SetEngineParams();
    void OnVarChange(IVariable* var);
    void OnMaterialChange(IVariable* var);
    void OnFileNameChange(IVariable* var);
    void UpdateMaterial();

    // --------------- VegetationObjectEvents ---------------
    void OnStatObjVegetationDataChanged() override;
    // ------------------------------------------------------

    GUID m_guid;

    //! Index of object in editor.
    int m_index;

    //! Objects category.
    QString m_category;

    //! Used during generation ?
    bool m_bInUse;

    //! True if all instances of this object must be hidden.
    bool m_bHidden;

    //! True if Selected.
    bool m_bSelected;

    //! Real size of geometrical object.
    float m_objectSize;

    //! Number of instances of this vegetation object placed on the map.
    int m_numInstances;

    //! Current group Id of this object (Need not saving).
    int m_id;

    //! Pointer to object for this group.
    IStatObj* m_statObj;

    //! CGF custom material
    TSmartPtr<CMaterial> m_pMaterial;

    //! Ground decal material
    TSmartPtr<CMaterial> m_pMaterialGroundDecal;

    // Place on terrain layers.
    QStringList m_terrainLayers;

    bool m_bVarIgnoreChange;
    //////////////////////////////////////////////////////////////////////////
    // Variables.
    //////////////////////////////////////////////////////////////////////////
    CSmartVariable<float>     mv_density;
    CSmartVariable<float>     mv_hmin;
    CSmartVariable<float>     mv_hmax;
    CSmartVariable<float>     mv_slope_min;
    CSmartVariable<float>     mv_slope_max;
    CSmartVariable<float>     mv_size;
    CSmartVariable<float>     mv_sizevar;
    CSmartVariable<bool>        mv_castShadows; // Legacy, remains for backward compatibility
    CSmartVariableEnum<int> mv_castShadowMinSpec;
    CSmartVariable<bool>        mv_recvShadows;
    CSmartVariable<bool>        mv_precalcShadows;
    CSmartVariable<float>     mv_bending;
    CSmartVariableEnum<int>     mv_hideable;
    CSmartVariableEnum<int>     mv_playerHideable;
    CSmartVariable<bool>      mv_pickable;
    CSmartVariable<float>       mv_aiRadius;
    CSmartVariable<bool>        mv_alphaBlend;
    CSmartVariable<float>     mv_SpriteDistRatio;
    CSmartVariable<float>   mv_LodDistRatio;
    CSmartVariable<float>     mv_ShadowDistRatio;
    CSmartVariable<float>     mv_MaxViewDistRatio;
    CSmartVariable<float>     mv_brightness;
    CSmartVariable<QString> mv_material;
    CSmartVariable<QString>   mv_materialGroundDecal;
    CSmartVariable<bool>    mv_UseSprites;
    CSmartVariable<bool>      mv_rotation;
    CSmartVariable<int> mv_rotationRangeToTerrainNormal;
    CSmartVariable<bool>        mv_alignToTerrain;
    CSmartVariable<float>     mv_alignToTerrainCoefficient;
    CSmartVariable<bool>      mv_useTerrainColor;
    CSmartVariable<bool>      mv_allowIndoor;
    CSmartVariable<bool>      mv_autoMerged;
    CSmartVariable<float>     mv_stiffness;
    CSmartVariable<float>     mv_damping;
    CSmartVariable<float>     mv_variance;
    CSmartVariable<float>   mv_airResistance;
    CSmartVariable<bool>    mv_markedForIntegration;
    CSmartVariable<bool>      mv_affectedByBrushes;
    CSmartVariable<bool>      mv_affectedByTerrain;
    CSmartVariable<bool>        mv_layerFrozen;
    CSmartVariable<bool>        mv_layerWet;
    CSmartVariableEnum<int> mv_minSpec;

    //! Filename of the associated CGF file
    CSmartVariable< QString > mv_fileName;
    mutable QString m_fileNameTmp;


    friend class CVegetationPanel;
    friend class CVegetationMap;
};

//////////////////////////////////////////////////////////////////////////
inline bool CVegetationObject::IsPlaceValid(float height, float slope) const
{
    if (height < mv_hmin || height > mv_hmax)
    {
        return false;
    }
    if (slope < mv_slope_min || slope > mv_slope_max)
    {
        return false;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
inline float CVegetationObject::CalcVariableSize() const
{
    if (mv_sizevar == 0)
    {
        return mv_size;
    }
    else
    {
        float fval = mv_sizevar * cry_random(-1.0f, 1.0f);
        if (fval >= 0)
        {
            return mv_size * (1.0f + fval);
        }
        else
        {
            return mv_size / (1.0f - fval);
        }
    }
}

#endif // CRYINCLUDE_EDITOR_VEGETATIONOBJECT_H
