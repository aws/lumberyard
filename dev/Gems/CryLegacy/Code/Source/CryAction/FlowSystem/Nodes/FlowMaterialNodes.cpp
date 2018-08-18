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

// Description : flownodes copied and updated from FlowEngineNodes.cpp


#include "CryLegacy_precompiled.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include "Components/IComponentRender.h"

//////////////////////////////////////////////////////////////////////////
class CFlowNodeEntityMaterial
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate,
        MaterialName,
        Reset,
        SerializeChanges,
    };

    enum OutputPorts
    {
        Value,
    };

public:
    CFlowNodeEntityMaterial(SActivationInfo* pActInfo)
    {
    }

    void Serialize(SActivationInfo* pActInfo, TSerialize ser) override
    {
        if (GetPortBool(pActInfo, InputPorts::SerializeChanges))
        {
            IEntity* pEntity = pActInfo->pEntity;

            if (ser.IsReading())
            {
                if (pEntity)
                {
                    string matSavedName;
                    ser.Value("material", matSavedName);

                    if (matSavedName.empty())
                    {
                        pEntity->SetMaterial(nullptr);
                    }
                    else
                    {
                        _smart_ptr<IMaterial> pMatCurr = pEntity->GetMaterial();
                        if (!pMatCurr || matSavedName != pMatCurr->GetName())
                        {
                            _smart_ptr<IMaterial> pMtl = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(matSavedName.c_str());
                            pEntity->SetMaterial(pMtl);
                        }
                    }
                }
            }
            else
            {
                string matName;
                if (pEntity)
                {
                    _smart_ptr<IMaterial> pMat = pEntity->GetMaterial();
                    if (pMat)
                    {
                        matName = pMat->GetName();
                    }
                }
                ser.Value("material", matName);
            }
        }
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<string>("mat_MaterialName", _HELP("Name of material to apply")),
            InputPortConfig_Void("Reset", _HELP("Trigger to reset the original material")),
            InputPortConfig<bool>("SerializeChanges", _HELP("Serialize this change")),
            {0}
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<string>("Value", _HELP("Returns the name of the material"), _HELP("Name")),
            {0}
        };

        config.sDescription = _HELP("Apply material to an Entity.");
        config.nFlags |= (EFLN_TARGET_ENTITY | EFLN_ACTIVATION_INPUT);
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    };

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        IEntity* pEntity = pActInfo->pEntity;
        if (!pEntity)
        {
            return;
        }

        switch (event)
        {
        case eFE_Initialize:
        {
            // workaround to the fact that the entity does not clear the material when going back to editor mode,
            // and to the fact that there is not any special event to signal when its going back to editor mode
            if (gEnv->IsEditor())
            {
                if (pEntity)
                {
                    if (gEnv->IsEditing())
                    {
                        m_pEditorMaterial = pEntity->GetMaterial();
                    }
                    else
                    {
                        pEntity->SetMaterial(m_pEditorMaterial);
                    }
                }
            }
            break;
        }

        case eFE_PrecacheResources:
        {
            // Preload target material.
            const string& mtlName = GetPortString(pActInfo, InputPorts::MaterialName);
            m_pMaterial = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(mtlName.c_str());
            break;
        }

        case eFE_Activate:
        {
            if (pEntity)
            {
                if (IsPortActive(pActInfo, InputPorts::Activate))
                {
                    ChangeMat(pActInfo, pEntity);
                }

                if (IsPortActive(pActInfo, InputPorts::Reset))
                {
                    pEntity->SetMaterial(nullptr);
                }

                if (!IsPortActive(pActInfo, InputPorts::MaterialName) && !IsPortActive(pActInfo, InputPorts::SerializeChanges))
                {
                    if (pEntity->GetMaterial())
                    {
                        ActivateOutput(pActInfo, OutputPorts::Value, string(pEntity->GetMaterial()->GetName()));
                    }
                    else
                    {
                        IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
                        if (pRenderComponent)
                        {
                            _smart_ptr<IMaterial> pMtl = pRenderComponent->GetRenderMaterial(0);
                            if (pMtl)
                            {
                                ActivateOutput(pActInfo, OutputPorts::Value, string(pMtl->GetName()));
                            }
                        }
                    }
                }
            }
            break;
        }
        }
    }

    void ChangeMat(SActivationInfo* pActInfo, IEntity* pEntity)
    {
        const string& mtlName = GetPortString(pActInfo, InputPorts::MaterialName);
        _smart_ptr<IMaterial> pMtl = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(mtlName.c_str());
        if (pMtl)
        {
            pEntity->SetMaterial(pMtl);
        }
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }

private:
    _smart_ptr<IMaterial> m_pMaterial;
    _smart_ptr<IMaterial> m_pEditorMaterial;
};


////////////////////////////////////////////////////////////////////////////////////////
// Flow node for setting entity render parameters (opacity, glow, motionblur, etc).
// Same as CFlowNodeMaterialShaderParam, but it serializes the changes.
////////////////////////////////////////////////////////////////////////////////////////
class CFlowNodeMaterialShaderParams
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate,
        MaterialName,
        SubMtlId,
        ParamFloat,
        ValueFloat,
        ParamColor,
        ValueColor,
        SerializeChanges,
    };

    enum OutputPorts
    {
        FloatValue,
        ColorValue,
    };

public:
    CFlowNodeMaterialShaderParams(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] = {
            InputPortConfig<string> ("mat_MaterialName", _HELP("Material Name")),
            InputPortConfig<int> ("SubMtlId", 0, _HELP("Sub Material Id, starting at 0"), _HELP("SubMaterialId")),
            InputPortConfig<string> ("ParamFloat", _HELP("Float Parameter to be set/get"), 0, _UICONFIG("dt=matparamname,name_ref=mat_Material,sub_ref=SubMtlId,param=float")),
            InputPortConfig<float>("ValueFloat", 0.0f, _HELP("Trigger and value to set the Float parameter")),
            InputPortConfig<string> ("ParamColor", _HELP("Color Parameter to be set/get"), 0, _UICONFIG("dt=matparamname,name_ref=mat_Material,sub_ref=SubMtlId,param=vec")),
            InputPortConfig<Vec3>("color_ValueColor", _HELP("Trigger and value to set the Color parameter")),
            InputPortConfig<bool>("SerializeChanges", _HELP("Serialize this change")),
            {0}
        };

        static const SOutputPortConfig out_config [] =
        {
            OutputPortConfig<float>("FloatValue", _HELP("Current float value")),
            OutputPortConfig<Vec3>("ColorValue", _HELP("Current color value")),
            {0}
        };

        config.sDescription = _HELP("Get Material's Shader Parameters.");
        config.nFlags |= EFLN_ACTIVATION_INPUT;
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_ADVANCED);
    }

    void Serialize(SActivationInfo* pActInfo, TSerialize ser) override
    {
        if (GetPortBool(pActInfo, InputPorts::SerializeChanges))
        {
            if (ser.IsWriting())
            {
                bool validFloat = false;
                bool validColor = false;
                float floatValue = 0.f;
                Vec3 colorValue(0.f, 0.f, 0.f);

                GetValues(pActInfo, floatValue, colorValue, validFloat, validColor);
                ser.Value("validFloat", validFloat);
                ser.Value("validColor", validColor);
                ser.Value("floatValue", floatValue);
                ser.Value("colorValue", colorValue);
            }

            if (ser.IsReading())
            {
                bool validFloat = false;
                bool validColor = false;
                float floatValue = 0.f;
                Vec3 colorValue(0.f, 0.f, 0.f);

                ser.Value("validFloat", validFloat);
                ser.Value("validColor", validColor);
                ser.Value("floatValue", floatValue);
                ser.Value("colorValue", colorValue);

                SetValues(pActInfo, validFloat, validColor, floatValue, colorValue);
            }
        }
    }

    _smart_ptr<IMaterial> ObtainMaterial(SActivationInfo* pActInfo)
    {
        const string& matName = GetPortString(pActInfo, InputPorts::MaterialName);
        _smart_ptr<IMaterial> pMtl = gEnv->p3DEngine->GetMaterialManager()->FindMaterial(matName);
        if (!pMtl)
        {
            GameWarning("[flow] CFlowNodeMaterialShaderParamSerialize: Material '%s' not found.", matName.c_str());
            return nullptr;
        }

        const int& subMtlId = GetPortInt(pActInfo, InputPorts::SubMtlId);
        pMtl = pMtl->GetSafeSubMtl(subMtlId);
        if (!pMtl)
        {
            GameWarning("[flow] CFlowNodeMaterialShaderParamSerialize: Material '%s' has no sub-material %d", matName.c_str(), subMtlId);
            return nullptr;
        }
        return pMtl;
    }

    IRenderShaderResources* ObtainRendRes(SActivationInfo* pActInfo, _smart_ptr<IMaterial> pMtl)
    {
        assert(pMtl);
        const SShaderItem& shaderItem = pMtl->GetShaderItem();
        IRenderShaderResources* pRendRes = shaderItem.m_pShaderResources;
        if (!pRendRes)
        {
            const string& matName = GetPortString(pActInfo, InputPorts::MaterialName);
            const int& subMtlId = GetPortInt(pActInfo, InputPorts::SubMtlId);
            GameWarning("[flow] CFlowNodeMaterialShaderParamSerialize: Material '%s' (submtl %d) has no shader-resources.", matName.c_str(), subMtlId);
        }
        return pRendRes;
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        if (event != eFE_Activate)
        {
            return;
        }

        const bool bGet = IsPortActive(pActInfo, InputPorts::Activate);
        const bool bSetFloat = IsPortActive(pActInfo, InputPorts::ValueFloat);
        const bool bSetColor = IsPortActive(pActInfo, InputPorts::ValueColor);

        if (!bGet && !bSetFloat && !bSetColor)
        {
            return;
        }

        if (bSetFloat || bSetColor)
        {
            float floatValue = GetPortFloat(pActInfo, InputPorts::ValueFloat);
            Vec3 colorValue = GetPortVec3(pActInfo, InputPorts::ValueColor);

            SetValues(pActInfo, bSetFloat, bSetColor, floatValue, colorValue);
        }

        if (bGet)
        {
            bool validFloat = false;
            bool validColor = false;
            float floatValue = 0;
            Vec3 colorValue(0.f, 0.f, 0.f);

            GetValues(pActInfo, floatValue, colorValue, validFloat, validColor);

            if (validFloat)
            {
                ActivateOutput(pActInfo, OutputPorts::FloatValue, floatValue);
            }
            if (validColor)
            {
                ActivateOutput(pActInfo, OutputPorts::ColorValue, colorValue);
            }
        }
    }

    void GetValues(SActivationInfo* pActInfo, float& floatValue, Vec3& colorValue, bool& validFloat, bool& validColor)
    {
        validFloat = false;
        validColor = false;
        const string& paramNameFloat = GetPortString(pActInfo, InputPorts::ParamFloat);
        const string& paramNameColor = GetPortString(pActInfo, InputPorts::ParamColor);

        _smart_ptr<IMaterial> pMtl = ObtainMaterial(pActInfo);
        if (!pMtl)
        {
            return;
        }

        IRenderShaderResources* pRendRes = ObtainRendRes(pActInfo, pMtl);
        if (!pRendRes)
        {
            return;
        }
        DynArrayRef<SShaderParam>& params = pRendRes->GetParameters();

        validFloat = pMtl->SetGetMaterialParamFloat(paramNameFloat, floatValue, true);
        validColor = pMtl->SetGetMaterialParamVec3(paramNameColor, colorValue, true);

        if (!validFloat)
        {
            for (int i = 0; i < params.size(); ++i)
            {
                SShaderParam& param = params[i];
                if (_stricmp(param.m_Name, paramNameFloat) == 0)
                {
                    switch (param.m_Type)
                    {
                    case eType_BOOL:
                    {
                        floatValue = param.m_Value.m_Bool;
                        validFloat = true;
                        break;
                    }
                    case eType_SHORT:
                    {
                        floatValue = param.m_Value.m_Short;
                        validFloat = true;
                        break;
                    }
                    case eType_INT:
                    {
                        floatValue = (float)param.m_Value.m_Int;
                        validFloat = true;
                        break;
                    }
                    case eType_FLOAT:
                    {
                        floatValue = param.m_Value.m_Float;
                        validFloat = true;
                        break;
                    }
                    default:
                    {
                        break;
                    }
                    }
                }
            }
        }

        if (!validColor)
        {
            for (int i = 0; i < params.size(); ++i)
            {
                SShaderParam& param = params[i];

                if (_stricmp(param.m_Name, paramNameColor) == 0)
                {
                    if (param.m_Type == eType_VECTOR)
                    {
                        colorValue.Set(param.m_Value.m_Vector[0], param.m_Value.m_Vector[1], param.m_Value.m_Vector[2]);
                        validColor = true;
                    }
                    if (param.m_Type == eType_FCOLOR)
                    {
                        colorValue.Set(param.m_Value.m_Color[0], param.m_Value.m_Color[1], param.m_Value.m_Color[2]);
                        validColor = true;
                    }
                    break;
                }
            }
        }
    }

    void SetValues(SActivationInfo* pActInfo, bool bSetFloat, bool bSetColor, float floatValue, Vec3& colorValue)
    {
        _smart_ptr<IMaterial> pMtl = ObtainMaterial(pActInfo);
        if (!pMtl)
        {
            return;
        }

        IRenderShaderResources* pRendRes = ObtainRendRes(pActInfo, pMtl);
        if (!pRendRes)
        {
            return;
        }

        DynArrayRef<SShaderParam>& params = pRendRes->GetParameters();
        const SShaderItem& shaderItem = pMtl->GetShaderItem();

        const string& paramNameFloat = GetPortString(pActInfo, InputPorts::ParamFloat);
        const string& paramNameColor = GetPortString(pActInfo, InputPorts::ParamColor);

        bool bUpdateShaderConstants = false;
        if (bSetFloat)
        {
            if (pMtl->SetGetMaterialParamFloat(paramNameFloat, floatValue, false) == false)
            {
                UParamVal val;
                val.m_Float = floatValue;
                SShaderParam::SetParam(paramNameFloat, &params, val);
                bUpdateShaderConstants = true;
            }
        }
        if (bSetColor)
        {
            if (pMtl->SetGetMaterialParamVec3(paramNameColor, colorValue, false) == false)
            {
                UParamVal val;
                val.m_Color[0] = colorValue[0];
                val.m_Color[1] = colorValue[1];
                val.m_Color[2] = colorValue[2];
                val.m_Color[3] = 1.0f;
                SShaderParam::SetParam(paramNameColor, &params, val);
                bUpdateShaderConstants = true;
            }
        }
        if (bUpdateShaderConstants)
        {
            shaderItem.m_pShaderResources->UpdateConstants(shaderItem.m_pShader);
        }
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }
};

////////////////////////////////////////////////////////////////////////////////////////
// Flow node for setting entity render parameters (opacity, glow, motionblur, etc)
////////////////////////////////////////////////////////////////////////////////////////
class CFlowNodeEntityMaterialShaderParams
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate,
        Slot,
        SubMtlId,
        ParamFloat,
        ValueFloat,
        ParamColor,
        ValueColor,
    };

    enum OutputPorts
    {
        FloatValue,
        ColorValue,
    };

public:
    CFlowNodeEntityMaterialShaderParams(SActivationInfo* pActInfo)
    {
        m_anyChangeDone = false;
        m_pMaterial = nullptr;
    }

    // Singleton nodes don't override the Clone method!

    //////////////////////////////////////////////////////////////////////////
    /// this serialize is *NOT* perfect. If any of the following inputs:
    /// "slot, submtlid, paramFloat, paramColor" do change and the node is
    /// triggered more than once with different values for them, the serialization
    /// won't be 100% correct.
    /// To make it perfect, either the materials should be serialized at engine
    /// level, or else we would need to create an intermediate module to handle
    /// changes on materials from game side, make the flownodes use that module
    /// instead of the materials directly, and serialize the changes there instead
    /// of in the flownodes.
    void Serialize(SActivationInfo* pActInfo, TSerialize ser) override
    {
        if (!pActInfo->pEntity)
        {
            return;
        }

        // we do assume that those inputs never change, which is ok for crysis2, but of course is not totally right
        const int slot = GetPortInt(pActInfo, InputPorts::Slot);
        const int subMtlId = GetPortInt(pActInfo, InputPorts::SubMtlId);
        const string& paramNameFloat = GetPortString(pActInfo, InputPorts::ParamFloat);
        const string& paramNameColor = GetPortString(pActInfo, InputPorts::ParamColor);

        ser.Value("m_anyChangeDone", m_anyChangeDone);

        //---------------------
        if (ser.IsWriting())
        {
            _smart_ptr<IMaterial> pSubMtl;
            IRenderShaderResources* pRendShaderRes;
            if (!ObtainMaterialPtrs(pActInfo->pEntity, slot, subMtlId, false, pSubMtl, pRendShaderRes))
            {
                return;
            }
            DynArrayRef<SShaderParam>& shaderParams = pRendShaderRes->GetParameters();

            if (!paramNameFloat.empty())
            {
                float floatValue = 0;
                ReadFloatValueFromMat(pSubMtl, pRendShaderRes, paramNameFloat, floatValue);
                ser.Value("floatValue", floatValue);
            }

            if (!paramNameColor.empty())
            {
                Vec3 colorValue(ZERO);
                ReadColorValueFromMat(pSubMtl, pRendShaderRes, paramNameColor, colorValue);
                ser.Value("colorValue", colorValue);
            }
        }

        //----------------
        if (ser.IsReading())
        {
            // if there was no change done, we still need to set values to the material, in case it was changed after the savegame.
            // But in that case, we dont need to create a clone, we just use the current material
            bool cloneMaterial = m_anyChangeDone;
            _smart_ptr<IMaterial> pSubMtl;
            IRenderShaderResources* pRendShaderRes;
            if (!ObtainMaterialPtrs(pActInfo->pEntity, slot, subMtlId, cloneMaterial, pSubMtl, pRendShaderRes))
            {
                return;
            }
            DynArrayRef<SShaderParam>& shaderParams = pRendShaderRes->GetParameters();

            // if we *potentially* can change one of them, we have to restore them, even if we in theory didnt change any.
            // that is why this checks the name of the parameters, instead of actualy changedone
            {
                bool bUpdateShaderConstants = false;
                if (!paramNameFloat.empty())
                {
                    float floatValue;
                    ser.Value("floatValue", floatValue);
                    if (pSubMtl->SetGetMaterialParamFloat(paramNameFloat, floatValue, false) == false)
                    {
                        UParamVal val;
                        val.m_Float = floatValue;
                        SShaderParam::SetParam(paramNameFloat, &shaderParams, val);
                        bUpdateShaderConstants = true;
                    }
                }
                if (!paramNameColor.empty())
                {
                    Vec3 colorValue;
                    ser.Value("colorValue", colorValue);
                    if (pSubMtl->SetGetMaterialParamVec3(paramNameColor, colorValue, false) == false)
                    {
                        UParamVal val;
                        val.m_Color[0] = colorValue[0];
                        val.m_Color[1] = colorValue[1];
                        val.m_Color[2] = colorValue[2];
                        val.m_Color[3] = 1.0f;
                        SShaderParam::SetParam(paramNameColor, &shaderParams, val);
                        bUpdateShaderConstants = true;
                    }
                }

                if (bUpdateShaderConstants)
                {
                    const SShaderItem& shaderItem = pSubMtl->GetShaderItem();
                    shaderItem.m_pShaderResources->UpdateConstants(shaderItem.m_pShader);
                }
            }
        }
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config[] =
        {
            InputPortConfig<int> ("Slot", 0, _HELP("Material Slot")),
            InputPortConfig<int> ("SubMtlId", 0, _HELP("Sub Material Id")),
            InputPortConfig<string>("ParamFloat", _HELP("Float parameter to be set/get"), 0, _UICONFIG("dt=matparamslot,slot_ref=Slot,sub_ref=SubMtlId,param=float")),
            InputPortConfig<float>("ValueFloat", 0.0f, _HELP("Activate to set float value")),
            InputPortConfig<string>("ParamColor", _HELP("Color parameter to be set/get"), 0, _UICONFIG("dt=matparamslot,slot_ref=Slot,sub_ref=SubMtlId,param=vec")),
            InputPortConfig<Vec3>("color_ValueColor", _HELP("Activate to set color value")),
            {0}
        };

        static const SOutputPortConfig out_config[] =
        {
            OutputPortConfig<float>("FloatValue", _HELP("Current float value")),
            OutputPortConfig<Vec3>("ColorValue", _HELP("Current color value")),
            {0}
        };

        config.sDescription = _HELP("Get Entity's Material Shader Parameters");
        config.nFlags |= (EFLN_TARGET_ENTITY | EFLN_ACTIVATION_INPUT);
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        if (!pActInfo->pEntity)
        {
            return;
        }

        if (event == eFE_Uninitialize)
        {
            // Clear out the material when we uninitialize the node. Not
            // doing so causes a crash when we are next activated since the pointer
            // to the material we are referencing gets forcefully deleted by the
            // material manager when a level is unloaded.
            m_pMaterial = nullptr;
            return;
        }

        if (event != eFE_Activate)
        {
            return;
        }

        const bool bGet = IsPortActive(pActInfo, InputPorts::Activate);
        const bool bSetFloat = IsPortActive(pActInfo, InputPorts::ValueFloat);
        const bool bSetColor = IsPortActive(pActInfo, InputPorts::ValueColor);
        const int slot = GetPortInt(pActInfo, InputPorts::Slot);
        const int subMtlId = GetPortInt(pActInfo, InputPorts::SubMtlId);
        const string& paramNameFloat = GetPortString(pActInfo, InputPorts::ParamFloat);
        const string& paramNameColor = GetPortString(pActInfo, InputPorts::ParamColor);

        if (!bGet && !bSetFloat && !bSetColor)
        {
            return;
        }

        bool cloneMaterial = bSetFloat || bSetColor;
        _smart_ptr<IMaterial> pSubMtl;
        IRenderShaderResources* pRendShaderRes;

        if (!ObtainMaterialPtrs(pActInfo->pEntity, slot, subMtlId, cloneMaterial, pSubMtl, pRendShaderRes))
        {
            return;
        }

        DynArrayRef<SShaderParam>& shaderParams = pRendShaderRes->GetParameters();

        //..........sets.......
        {
            bool bUpdateShaderConstants = false;
            if (bSetFloat)
            {
                m_anyChangeDone = true;
                float floatValue = GetPortFloat(pActInfo, InputPorts::ValueFloat);
                if (pSubMtl->SetGetMaterialParamFloat(paramNameFloat, floatValue, false) == false)
                {
                    UParamVal val;
                    val.m_Float = floatValue;
                    SShaderParam::SetParam(paramNameFloat, &shaderParams, val);
                    bUpdateShaderConstants = true;
                }
            }
            if (bSetColor)
            {
                m_anyChangeDone = true;
                Vec3 colorValue = GetPortVec3(pActInfo, InputPorts::ValueColor);
                if (pSubMtl->SetGetMaterialParamVec3(paramNameColor, colorValue, false) == false)
                {
                    UParamVal val;
                    val.m_Color[0] = colorValue[0];
                    val.m_Color[1] = colorValue[1];
                    val.m_Color[2] = colorValue[2];
                    val.m_Color[3] = 1.0f;
                    SShaderParam::SetParam(paramNameColor, &shaderParams, val);
                    bUpdateShaderConstants = true;
                }
            }

            if (bUpdateShaderConstants)
            {
                const SShaderItem& shaderItem = pSubMtl->GetShaderItem();
                shaderItem.m_pShaderResources->UpdateConstants(shaderItem.m_pShader);
            }
        }

        //........get
        if (bGet)
        {
            float floatValue;
            if (ReadFloatValueFromMat(pSubMtl, pRendShaderRes, paramNameFloat, floatValue))
            {
                ActivateOutput(pActInfo, OutputPorts::FloatValue, floatValue);
            }

            Vec3 colorValue;
            if (ReadColorValueFromMat(pSubMtl, pRendShaderRes, paramNameColor, colorValue))
            {
                ActivateOutput(pActInfo, OutputPorts::ColorValue, colorValue);
            }
        }
    }

    bool ReadFloatValueFromMat(_smart_ptr<IMaterial> pSubMtl, IRenderShaderResources* pRendShaderRes, const char* paramName, float& floatValue)
    {
        if (pSubMtl->SetGetMaterialParamFloat(paramName, floatValue, true))
        {
            return true;
        }

        DynArrayRef<SShaderParam>& shaderParams = pRendShaderRes->GetParameters();

        for (int i = 0; i < shaderParams.size(); ++i)
        {
            SShaderParam& param = shaderParams[i];
            if (_stricmp(param.m_Name, paramName) == 0)
            {
                floatValue = 0.0f;
                switch (param.m_Type)
                {
                case eType_BOOL:
                {
                    floatValue = param.m_Value.m_Bool;
                    break;
                }
                case eType_SHORT:
                {
                    floatValue = param.m_Value.m_Short;
                    break;
                }
                case eType_INT:
                {
                    floatValue = (float)param.m_Value.m_Int;
                    break;
                }
                case eType_FLOAT:
                {
                    floatValue = param.m_Value.m_Float;
                    break;
                }
                }
                return true;
            }
        }
        return false;
    }

    bool ReadColorValueFromMat(_smart_ptr<IMaterial> pSubMtl, IRenderShaderResources* pRendShaderRes, const char* paramName, Vec3& colorValue)
    {
        if (pSubMtl->SetGetMaterialParamVec3(paramName, colorValue, true))
        {
            return true;
        }

        DynArrayRef<SShaderParam>& shaderParams = pRendShaderRes->GetParameters();

        for (int i = 0; i < shaderParams.size(); ++i)
        {
            SShaderParam& param = shaderParams[i];
            if (_stricmp(param.m_Name, paramName) == 0)
            {
                colorValue.Set(0, 0, 0);
                if (param.m_Type == eType_VECTOR)
                {
                    colorValue.Set(param.m_Value.m_Vector[0], param.m_Value.m_Vector[1], param.m_Value.m_Vector[2]);
                }
                if (param.m_Type == eType_FCOLOR)
                {
                    colorValue.Set(param.m_Value.m_Color[0], param.m_Value.m_Color[1], param.m_Value.m_Color[2]);
                }
                return true;
            }
        }
        return false;
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    //////////////////////////////////////////////////////////////////////////
    bool ObtainMaterialPtrs(IEntity* pEntity, int slot, int subMtlId, bool cloneMaterial, _smart_ptr<IMaterial>& pSubMtl, IRenderShaderResources*& pRenderShaderRes)
    {
        if (!pEntity)
        {
            return false;
        }

        IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
        if (pRenderComponent == 0)
        {
            return false;
        }

        _smart_ptr<IMaterial>   pMaterial = pRenderComponent->GetRenderMaterial(slot);
        if (!pMaterial)
        {
            GameWarning("[flow] CFlowNodeEntityMaterialShaderParam: Entity '%s' [%d] has no material at slot %d", pEntity->GetName(), pEntity->GetId(), slot);
            return false;
        }

        if (cloneMaterial && m_pMaterial != pMaterial)
        {
            pMaterial = gEnv->p3DEngine->GetMaterialManager()->CloneMultiMaterial(pMaterial);
            if (!pMaterial)
            {
                GameWarning("[flow] CFlowNodeEntityMaterialShaderParam: Entity '%s' [%d] Can't clone material at slot %d", pEntity->GetName(), pEntity->GetId(), slot);
                return false;
            }
            pRenderComponent->SetSlotMaterial(slot, pMaterial);
            m_pMaterial = pMaterial;
        }

        pSubMtl = pMaterial->GetSafeSubMtl(subMtlId);
        if (!pSubMtl)
        {
            GameWarning("[flow] CFlowNodeEntityMaterialShaderParam: Entity '%s' [%d] has no sub-material %d at slot %d", pEntity->GetName(), pEntity->GetId(), subMtlId, slot);
            return false;
        }

        const SShaderItem& shaderItem = pSubMtl->GetShaderItem();
        pRenderShaderRes = shaderItem.m_pShaderResources;
        if (!pRenderShaderRes)
        {
            GameWarning("[flow] CFlowNodeEntityMaterialShaderParam: Entity '%s' [%d] Material's '%s' sub-material [%d] '%s' has not shader-resources", pEntity->GetName(), pEntity->GetId(), pMaterial->GetName(), subMtlId, pSubMtl->GetName());
            return false;
        }
        return true;
    }

protected:
    bool m_anyChangeDone;
    _smart_ptr<IMaterial> m_pMaterial;// this is only used to know if we need to clone or not. And is in purpose not reseted on flow initialize, and also not serialized.
    // the situation is:
    // if we already cloned the material, it will stay cloned even if we load a checkpoint previous to the point where the change happened, so we dont need to clone it again.
    // On the other side, if we are doing a 'resumegame", we need to clone the material even if we already cloned it when the checkpoint happened.
    // By not serializing or reseting this member, we cover those cases correctly
};

////////////////////////////////////////////////////////////////////////////////////////
// Flow node for cloning an entity's material
////////////////////////////////////////////////////////////////////////////////////////
class CFlowNodeEntityCloneMaterial
    : public CFlowBaseNode<eNCT_Singleton>
{
    enum InputPorts
    {
        Activate,
        Reset,
        Slot,
    };

    enum OutputPorts
    {
        OnCloned,
        OnReset
    };

public:
    CFlowNodeEntityCloneMaterial(SActivationInfo* pActInfo)
    {
    }

    void GetConfiguration(SFlowNodeConfig& config) override
    {
        static const SInputPortConfig in_config [] =
        {
            InputPortConfig_Void("Reset", _HELP("Activate to reset material")),
            InputPortConfig<int> ("Slot", 0, _HELP("Material Slot")),
            {0}
        };

        static const SOutputPortConfig out_config [] =
        {
            OutputPortConfig_Void("OnCloned", _HELP("Activated when cloned (via [Activate])")),
            OutputPortConfig_Void("OnReset", _HELP("Activated when reset (via [Reset])")),
            {0}
        };

        config.sDescription = _HELP("Clone an Entity's Material or Reset it back to original.");
        config.nFlags |= (EFLN_TARGET_ENTITY | EFLN_ACTIVATION_INPUT);
        config.pInputPorts = in_config;
        config.pOutputPorts = out_config;
        config.SetCategory(EFLN_APPROVED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo) override
    {
        switch (event)
        {
        case eFE_Initialize:
        {
            const int slot = GetPortInt(pActInfo, InputPorts::Slot);
            UnApplyMaterial(pActInfo->pEntity, slot);
            break;
        }

        case eFE_Activate:
        {
            if (IsPortActive(pActInfo, InputPorts::Reset))
            {
                const int slot = GetPortInt(pActInfo, InputPorts::Slot);
                UnApplyMaterial(pActInfo->pEntity, slot);
                ActivateOutput(pActInfo, OutputPorts::OnReset, true);
            }
            if (IsPortActive(pActInfo, InputPorts::Activate))
            {
                const int slot = GetPortInt(pActInfo, InputPorts::Slot);
                UnApplyMaterial(pActInfo->pEntity, slot);
                CloneAndApplyMaterial(pActInfo->pEntity, slot);
                ActivateOutput(pActInfo, OutputPorts::OnCloned, true);
            }
            break;
        }
        default:
            break;
        }
    }

    void UnApplyMaterial(IEntity* pEntity, int slot)
    {
        if (!pEntity)
        {
            return;
        }
        IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
        if (pRenderComponent == 0)
        {
            return;
        }
        pRenderComponent->SetSlotMaterial(slot, 0);
    }

    void CloneAndApplyMaterial(IEntity* pEntity, int slot)
    {
        if (!pEntity)
        {
            return;
        }

        IComponentRenderPtr pRenderComponent = pEntity->GetComponent<IComponentRender>();
        if (pRenderComponent == 0)
        {
            return;
        }

        _smart_ptr<IMaterial> pMtl = pRenderComponent->GetSlotMaterial(slot);
        if (pMtl)
        {
            return;
        }

        pMtl = pRenderComponent->GetRenderMaterial(slot);
        if (!pMtl)
        {
            GameWarning("[flow] CFlowNodeEntityCloneMaterial: Entity '%s' [%d] has no material at slot %d", pEntity->GetName(), pEntity->GetId(), slot);
            return;
        }
        pMtl = gEnv->p3DEngine->GetMaterialManager()->CloneMultiMaterial(pMtl);
        pRenderComponent->SetSlotMaterial(slot, pMtl);
    }

    void GetMemoryUsage(ICrySizer* s) const override
    {
        s->Add(*this);
    }
};


//////////////////////////////////////////////////////////////////////////
/// Material Flow Node Registration
//////////////////////////////////////////////////////////////////////////

REGISTER_FLOW_NODE("Material:EntityMaterialChange", CFlowNodeEntityMaterial);
REGISTER_FLOW_NODE("Material:MaterialParams", CFlowNodeMaterialShaderParams);
REGISTER_FLOW_NODE("Material:EntityMaterialParams", CFlowNodeEntityMaterialShaderParams);
REGISTER_FLOW_NODE("Material:MaterialClone", CFlowNodeEntityCloneMaterial);
