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

#include "StarterGameGem_precompiled.h"
#include "StarterGameMaterialUtility.h"

#include "StarterGameEntityUtility.h"
#include "StarterGameUtility.h"

#include <AzCore/RTTI/BehaviorContext.h>

#include <LmbrCentral/Rendering/MaterialOwnerBus.h>
#include <LmbrCentral/Rendering/MeshComponentBus.h>


namespace StarterGameGem
{
    _smart_ptr<IMaterial> StarterGameMaterialUtility::GetMaterial(AZ::EntityId entityId)
    {
        _smart_ptr<IMaterial> mat = nullptr;
		bool isReady = false;
		LmbrCentral::MaterialOwnerRequestBus::EventResult(isReady, entityId, &LmbrCentral::MaterialOwnerRequestBus::Events::IsMaterialOwnerReady);
		if (isReady)
		{
			LmbrCentral::MaterialOwnerRequestBus::EventResult(mat, entityId, &LmbrCentral::MaterialOwnerRequestBus::Events::GetMaterial);
		}
        return mat;
    }

	_smart_ptr<IMaterial> StarterGameMaterialUtility::GetSubMaterial(_smart_ptr<IMaterial> parentMaterial, int subMtlIndex)
	{
		return subMtlIndex >= 0 ? parentMaterial->GetSubMtl(subMtlIndex) : parentMaterial;
	}

    //===========================================================================================
    //
    // Core Get/Set functions (private)
    //
    //===========================================================================================

    bool StarterGameMaterialUtility::SetMaterialParam(_smart_ptr<IMaterial> mat, const AZStd::string& paramName, UParamVal var, EParamType type)
    {
        bool set = false;
        switch (type)
        {
        case eType_FLOAT:
            set = mat->SetGetMaterialParamFloat(paramName.c_str(), var.m_Float, false);
            break;
        case eType_VECTOR:
            Vec3 vecValue = Vec3(var.m_Vector[0], var.m_Vector[1], var.m_Vector[2]);
            set = mat->SetGetMaterialParamVec3(paramName.c_str(), vecValue, false);
            break;
        }

        return set;
    }

    bool StarterGameMaterialUtility::SetShaderParam(AZ::EntityId entityId, _smart_ptr<IMaterial> mat, const AZStd::string& paramName, UParamVal var)
    {
        bool set = false;
        SShaderItem shaderItem = mat->GetShaderItem();
        if (shaderItem.m_pShaderResources != nullptr)
        {
            DynArray<SShaderParam> params = shaderItem.m_pShaderResources->GetParameters();
            if (params.size() == 0)
            {
                AZ_Warning("StarterGame", false, "%s found no shader parameters on %s (%llu)", __FUNCTION__, StarterGameEntityUtility::GetEntityName(entityId), (AZ::u64)entityId);
            }

            set = SShaderParam::SetParam(paramName.c_str(), &params, var);
            if (set)
            {
                SInputShaderResources res;
                shaderItem.m_pShaderResources->ConvertToInputResource(&res);
                res.m_ShaderParams = params;
                shaderItem.m_pShaderResources->SetShaderParams(&res, shaderItem.m_pShader);
            }
        }
        else
        {
            AZ_Warning("StarterGame", false, "%s found an invalid shader item on %s (%llu)", __FUNCTION__, StarterGameEntityUtility::GetEntityName(entityId), (AZ::u64)entityId);
        }

        return set;
    }

    bool StarterGameMaterialUtility::GetMaterialParam(_smart_ptr<IMaterial> mat, const AZStd::string& paramName, float &value)
    {
        bool set = mat && mat->SetGetMaterialParamFloat(paramName.c_str(), value, true);
        return set;
    }

    bool StarterGameMaterialUtility::GetMaterialParam(_smart_ptr<IMaterial> mat, const AZStd::string& paramName, AZ::Vector3& value)
    {
        Vec3 vecValue = Vec3(value);
        bool set = mat && mat->SetGetMaterialParamVec3(paramName.c_str(), vecValue, true);
        if (set) 
        { 
            value = AZ::Vector3(vecValue.x, vecValue.y, vecValue.z); 
        }
        return set;
    }

    bool StarterGameMaterialUtility::GetShaderParam(_smart_ptr<IMaterial> mat, const AZStd::string& paramName, float& value)
    {
        bool got = false;
        SShaderItem shaderItem = mat->GetShaderItem();
        const char * szName = paramName.c_str();
        if (shaderItem.m_pShaderResources != nullptr)
        {
            DynArray<SShaderParam> params = shaderItem.m_pShaderResources->GetParameters();
            for (int i = 0; i < params.size(); i++)
            {
                SShaderParam* sp = &(params)[i];
                if (!sp)
                {
                    continue;
                }

                if (!_stricmp(sp->m_Name, szName))
                {
                    got = true;
                    switch (sp->m_Type)
                    {
                    case eType_HALF:
                    case eType_FLOAT:
                        value = sp->m_Value.m_Float;
                        break;
                    }

                    break;
                }
            }
        }
        return got;
    }

    bool StarterGameMaterialUtility::GetShaderParam(_smart_ptr<IMaterial> mat, const AZStd::string& paramName, AZ::Vector3& value)
    {
        bool got = false;
        SShaderItem shaderItem = mat->GetShaderItem();
        const char * szName = paramName.c_str();
        if (shaderItem.m_pShaderResources != nullptr)
        {
            DynArray<SShaderParam> params = shaderItem.m_pShaderResources->GetParameters();
            for (int i = 0; i < params.size(); i++)
            {
                SShaderParam* sp = &(params)[i];
                if (!sp)
                {
                    continue;
                }

                if (!_stricmp(sp->m_Name, szName))
                {
                    got = true;
                    switch (sp->m_Type)
                    {
                    case eType_VECTOR:
                        value.Set(sp->m_Value.m_Vector);
                        break;

                    case eType_FCOLOR:
                    case eType_FCOLORA:
                        value.Set(sp->m_Value.m_Color[0], sp->m_Value.m_Color[1], sp->m_Value.m_Color[2]);
                        break;
                    }

                    break;
                }
            }
        }
        return got;
    }
    
    //===========================================================================================
    //
    // Get/Set helper functions (private)
    //
    //===========================================================================================

    bool StarterGameMaterialUtility::SetShaderMatFloat(AZ::EntityId entityId, _smart_ptr<IMaterial> mat, const AZStd::string& paramName, float var)
    {
        bool set = false;

        UParamVal val;
        val.m_Float = var;

        // Check if it's a default parameter of the material.
        set = SetMaterialParam(mat, paramName, val, eType_FLOAT);
        if (set)
        {
            return set;
        }

        // If it's not a default parameter then we'll need to go deeper.
        set = SetShaderParam(entityId, mat, paramName, val);

        return set;
    }

    bool StarterGameMaterialUtility::GetShaderMatFloat(AZ::EntityId entityId, _smart_ptr<IMaterial> mat, const AZStd::string& paramName, float& var)
    {
        bool got = false;

        // Check if it's a default parameter of the material.
        got = GetMaterialParam(mat, paramName, var);
        if (!got)
        {
            // If it's not a default parameter then we'll need to go deeper.
            got = GetShaderParam(mat, paramName, var);
        }

        return got;
    }
    

    bool StarterGameMaterialUtility::SetShaderMatVec3(AZ::EntityId entityId, _smart_ptr<IMaterial> mat, const AZStd::string& paramName, const AZ::Vector3& var)
    {
        bool set = false;

        UParamVal val;
        val.m_Vector[0] = var.GetX();
        val.m_Vector[1] = var.GetY();
        val.m_Vector[2] = var.GetZ();

        // Check if it's a default parameter of the material.
        set = SetMaterialParam(mat, paramName, val, eType_VECTOR);
        if (set)
        {
            return set;
        }

        // If it's not a default parameter then we'll need to go deeper.
        set = SetShaderParam(entityId, mat, paramName, val);

        return set;
    }

    bool StarterGameMaterialUtility::GetShaderMatVec3(AZ::EntityId entityId, _smart_ptr<IMaterial> mat, const AZStd::string& paramName, AZ::Vector3& var)
    {
        bool got = false;

        // Check if it's a default parameter of the material.
        got = GetMaterialParam(mat, paramName, var);
        if (!got)
        {
            // If it's not a default parameter then we'll need to go deeper.
            got = GetShaderParam(mat, paramName, var);
        }

        return got;
    }

    //===========================================================================================
    //
    // Get/Set functions (public)
    //
    //===========================================================================================

    bool StarterGameMaterialUtility::SetShaderFloat(AZ::EntityId entityId, const AZStd::string& paramName, float var)
    {
        bool set = false;
        _smart_ptr<IMaterial> mat = GetMaterial(entityId);

        if (!mat)
        {
            AZ_Warning("StarterGame", false, "%s couldn't find a material on %s (%llu)", __FUNCTION__, StarterGameEntityUtility::GetEntityName(entityId), (AZ::u64)entityId);
            return set;
        }

        set = SetShaderMatFloat(entityId, mat, paramName, var);

        return set;
    }

    bool StarterGameMaterialUtility::SetSubMtlShaderFloat(AZ::EntityId entityId, const AZStd::string& paramName, int subMtlIndex, float var)
    {
        bool set = false;
        _smart_ptr<IMaterial> mat = GetMaterial(entityId);

        if (!mat)
        {
            AZ_Warning("StarterGame", false, "%s couldn't find a material on %s (%llu)", __FUNCTION__, StarterGameEntityUtility::GetEntityName(entityId), (AZ::u64)entityId);
            return set;
        }
        if (mat->GetSubMtlCount() < subMtlIndex)
        {
            AZ_Warning("StarterGame", false, "%s material on %s (%llu) doesn't have sub material at index %d", __FUNCTION__, StarterGameEntityUtility::GetEntityName(entityId), (AZ::u64)entityId, subMtlIndex);
            return set;
        }

        mat = GetSubMaterial(mat, subMtlIndex);

        set = SetShaderMatFloat(entityId, mat, paramName, var);

        return set;
    }
 
    float StarterGameMaterialUtility::GetShaderFloat(AZ::EntityId entityId, const AZStd::string& paramName)
    {
        bool got = false;
        float var = 0.0f;
        _smart_ptr<IMaterial> mat = GetMaterial(entityId);

        if (!mat)
        {
            AZ_Warning("StarterGame", false, "%s couldn't find a material on %s (%llu)", __FUNCTION__, StarterGameEntityUtility::GetEntityName(entityId), (AZ::u64)entityId);
            return var;
        }

        got = GetShaderMatFloat(entityId, mat, paramName, var);

        return var;
    }

    float StarterGameMaterialUtility::GetSubMtlShaderFloat(AZ::EntityId entityId, const AZStd::string& paramName, int subMtlIndex)
    {
        bool got = false;
        float var = 0.0f;
        _smart_ptr<IMaterial> mat = GetMaterial(entityId);

        if (!mat)
        {
            AZ_Warning("StarterGame", false, "%s couldn't find a material on %s (%llu)", __FUNCTION__, StarterGameEntityUtility::GetEntityName(entityId), (AZ::u64)entityId);
            return var;
        }
        if (mat->GetSubMtlCount() < subMtlIndex)
        {
            AZ_Warning("StarterGame", false, "%s material on %s (%llu) doesn't have sub material at index %d", __FUNCTION__, StarterGameEntityUtility::GetEntityName(entityId), (AZ::u64)entityId, subMtlIndex);
            return var;
        }

		mat = GetSubMaterial(mat, subMtlIndex);

        got = GetShaderMatFloat(entityId, mat, paramName, var);

        return var;
    }


    bool StarterGameMaterialUtility::SetShaderVec3(AZ::EntityId entityId, const AZStd::string& paramName, const AZ::Vector3& var)
    {
        bool set = false;
        _smart_ptr<IMaterial> mat = GetMaterial(entityId);

        if (!mat)
        {
            AZ_Warning("StarterGame", false, "%s couldn't find a material on %s (%llu)", __FUNCTION__, StarterGameEntityUtility::GetEntityName(entityId), (AZ::u64)entityId);
            return set;
        }

        set = SetShaderMatVec3(entityId, mat, paramName, var);

        return set;
    }

    bool StarterGameMaterialUtility::SetSubMtlShaderVec3(AZ::EntityId entityId, const AZStd::string& paramName, int subMtlIndex, const AZ::Vector3& var)
    {
        bool set = false;
        _smart_ptr<IMaterial> mat = GetMaterial(entityId);

        if (!mat)
        {
            AZ_Warning("StarterGame", false, "%s couldn't find a material on %s (%llu)", __FUNCTION__, StarterGameEntityUtility::GetEntityName(entityId), (AZ::u64)entityId);
            return set;
        }
        if (mat->GetSubMtlCount() < subMtlIndex)
        {
            AZ_Warning("StarterGame", false, "%s material on %s (%llu) doesn't have sub material at index %d", __FUNCTION__, StarterGameEntityUtility::GetEntityName(entityId), (AZ::u64)entityId, subMtlIndex);
            return set;
        }

		mat = GetSubMaterial(mat, subMtlIndex);

        set = SetShaderMatVec3(entityId, mat, paramName, var);

        return set;
    }

    AZ::Vector3 StarterGameMaterialUtility::GetShaderVec3(AZ::EntityId entityId, const AZStd::string& paramName)
    {
        bool got = false;
        AZ::Vector3 var;
        _smart_ptr<IMaterial> mat = GetMaterial(entityId);

        if (!mat)
        {
            AZ_Warning("StarterGame", false, "%s couldn't find a material on %s (%llu)", __FUNCTION__, StarterGameEntityUtility::GetEntityName(entityId), (AZ::u64)entityId);
            return var;
        }
        
        got = GetShaderMatVec3(entityId, mat, paramName, var);

        return var;
    }

    AZ::Vector3 StarterGameMaterialUtility::GetSubMtlShaderVec3(AZ::EntityId entityId, const AZStd::string& paramName, int subMtlIndex)
    {
        bool got = false;
        AZ::Vector3 var;
        _smart_ptr<IMaterial> mat = GetMaterial(entityId);

        if (!mat)
        {
            AZ_Warning("StarterGame", false, "%s couldn't find a material on %s (%llu)", __FUNCTION__, StarterGameEntityUtility::GetEntityName(entityId), (AZ::u64)entityId);
            return var;
        }
        if (mat->GetSubMtlCount() < subMtlIndex)
        {
            AZ_Warning("StarterGame", false, "%s material on %s (%llu) doesn't have sub material at index %d", __FUNCTION__, StarterGameEntityUtility::GetEntityName(entityId), (AZ::u64)entityId, subMtlIndex);
            return var;
        }

		mat = GetSubMaterial(mat, subMtlIndex);

        got = GetShaderMatVec3(entityId, mat, paramName, var);

        return var;
    }

    //===========================================================================================
    //
    // Clone/restore functions (public)
    //
    //===========================================================================================

    bool StarterGameMaterialUtility::ReplaceMaterialWithClone(AZ::EntityId entityId)
    {
        bool isReady = false;
        LmbrCentral::MaterialOwnerRequestBus::EventResult(isReady, entityId, &LmbrCentral::MaterialOwnerRequestBus::Events::IsMaterialOwnerReady);
        if (isReady)
        {
            _smart_ptr<IMaterial> mat = nullptr;
            LmbrCentral::MaterialOwnerRequestBus::EventResult(mat, entityId, &LmbrCentral::MaterialOwnerRequestBus::Events::GetMaterial);
            if (mat)
            {
                _smart_ptr<IMaterial> clonedMaterial = gEnv->p3DEngine->GetMaterialManager()->CloneMaterial(mat);
                LmbrCentral::MaterialOwnerRequestBus::Event(entityId, &LmbrCentral::MaterialOwnerRequestBus::Events::SetMaterial, clonedMaterial);
                return true;
            }
        }
        return false;
    }

    void StarterGameMaterialUtility::RestoreOriginalMaterial(AZ::EntityId entityId)
    {
        // setting material to null restores original material on the mesh
		bool isReady = false;
		LmbrCentral::MaterialOwnerRequestBus::EventResult(isReady, entityId, &LmbrCentral::MaterialOwnerRequestBus::Events::IsMaterialOwnerReady);
		if (isReady)
		{
			LmbrCentral::MaterialOwnerRequestBus::Event(entityId, &LmbrCentral::MaterialOwnerRequestBus::Events::SetMaterial, nullptr);
		}
    }

    int StarterGameMaterialUtility::GetSurfaceIndexFromName(const AZStd::string surfaceName)
    {
        return gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeIdByName(surfaceName.c_str());
    }

    AZStd::string StarterGameMaterialUtility::GetSurfaceNameFromIndex(int surfaceId)
    {
        ISurfaceType* surfaceType = gEnv->p3DEngine->GetMaterialManager()->GetSurfaceType(surfaceId);
        if (surfaceType == nullptr)
        {
            return "unknown";
        }

        return surfaceType->GetName();
    }

    void StarterGameMaterialUtility::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection);
        if (behaviorContext)
        {
            behaviorContext->Class<StarterGameMaterialUtility>("StarterGameMaterialUtility")
                ->Method("SetShaderFloat", &SetShaderFloat)
                ->Method("SetSubMtlShaderFloat", &SetSubMtlShaderFloat)
                ->Method("SetShaderVec3", &SetShaderVec3)
                ->Method("SetSubMtlShaderVec3", &SetSubMtlShaderVec3)
                ->Method("GetShaderFloat", &GetShaderFloat)
                ->Method("GetSubMtlShaderFloat", &GetSubMtlShaderFloat)
                ->Method("GetShaderVec3", &GetShaderVec3)
                ->Method("GetSubMtlShaderVec3", &GetSubMtlShaderVec3)
                ->Method("ReplaceMaterialWithClone", &ReplaceMaterialWithClone)
                ->Method("RestoreOriginalMaterial", &RestoreOriginalMaterial)

                ->Method("GetSurfaceIndexFromName", &GetSurfaceIndexFromName)
                ->Method("GetSurfaceNameFromIndex", &GetSurfaceNameFromIndex)
            ;
        }
    }

}
