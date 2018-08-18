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
#include "CryLegacy_precompiled.h"

#include "WrinkleMapShaderParamCallback.h"

#include <ICryAnimation.h>
#include <CryExtension/CryCreateClassInstance.h>

//////////////////////////////////////////////////////////////////////////

#define WRINKLE_BONE_PREFIX "blend_control"
#define MAX_WRINKLE_BONES 4
#define WRINKLE_PARAM_NAME "%WRINKLE_BLENDING"
#define WRINKLE_MASK_NAME0 "WrinkleMask0"
#define WRINKLE_MASK_NAME1 "WrinkleMask1"
#define WRINKLE_MASK_NAME2 "WrinkleMask2"

//////////////////////////////////////////////////////////////////////////

namespace {
    bool HasWrinkleBlending(IMaterial& material)
    {
        IShader* pShader = material.GetShaderItem().m_pShader;
        if (!pShader)
        {
            return false;
        }

        uint64 mask = pShader->GetGenerationMask();
        SShaderGen* pShaderGen = pShader->GetGenerationParams();
        if (!pShaderGen)
        {
            return false;
        }

        for (int i = 0; i < pShaderGen->m_BitMask.size(); ++i)
        {
            SShaderGenBit* pShaderGenBit = pShaderGen->m_BitMask[i];
            if (pShaderGenBit->m_ParamName == WRINKLE_PARAM_NAME
                && (mask & pShaderGenBit->m_Mask) == pShaderGenBit->m_Mask)
            {
                return true;
            }
        }

        return false;
    }

    _smart_ptr<IMaterial> FindFirstMaterialWithWrinkleBlending(IMaterial& material)
    {
        if (HasWrinkleBlending(material))
        {
            return &material;
        }

        for (int i = 0; i < material.GetSubMtlCount(); ++i)
        {
            if (_smart_ptr<IMaterial> pMaterial = material.GetSubMtl(i))
            {
                if (HasWrinkleBlending(*pMaterial))
                {
                    return pMaterial;
                }
            }
        }

        return NULL;
    }

    _smart_ptr<IMaterial> FindFirstMaterialWithWrinkleBlending(ICharacterInstance& instance)
    {
        if (_smart_ptr<IMaterial> pMaterial = instance.GetIMaterial())
        {
            if (pMaterial = FindFirstMaterialWithWrinkleBlending(*pMaterial))
            {
                return pMaterial;
            }
        }

        IAttachmentManager* pAttachmentManager = instance.GetIAttachmentManager();
        if (!pAttachmentManager)
        {
            return NULL;
        }

        uint attachmentCount = pAttachmentManager->GetAttachmentCount();
        for (uint i = 0; i < attachmentCount; ++i)
        {
            IAttachment* pAttachment = pAttachmentManager->GetInterfaceByIndex(i);
            if (!pAttachment)
            {
                continue;
            }

            IAttachmentObject* pAttachmentObject = pAttachment->GetIAttachmentObject();
            if (!pAttachmentObject)
            {
                continue;
            }

            IAttachmentSkin* pAttachmentSkin = pAttachmentObject->GetIAttachmentSkin();
            if (!pAttachmentSkin)
            {
                continue;
            }

            ISkin* pSkin = pAttachmentSkin->GetISkin();
            if (!pSkin)
            {
                continue;
            }

            _smart_ptr<IMaterial> pMaterial = pSkin->GetIMaterial(0);
            if (!pMaterial)
            {
                continue;
            }

            if (pMaterial = FindFirstMaterialWithWrinkleBlending(*pMaterial))
            {
                return pMaterial;
            }
        }

        return NULL;
    }
} // namespace

//////////////////////////////////////////////////////////////////////////

CRYREGISTER_CLASS(CWrinkleMapShaderParamCallback)
CRYIMPLEMENT_AND_REGISTER_SHADERPARAMCALLBACKUI_CLASS(CWrinkleMapShaderParamCallback)

//////////////////////////////////////////////////////////////////////////

CWrinkleMapShaderParamCallback::CWrinkleMapShaderParamCallback()
    : m_pCachedMaterial(0)
    , m_pCharacterInstance(0)
    , m_bWrinklesEnabled(false)
{
    m_eSemantic[0] = 0;
    m_eSemantic[1] = 0;
    m_eSemantic[2] = 0;
}

CWrinkleMapShaderParamCallback::~CWrinkleMapShaderParamCallback()
{
    m_pCachedMaterial = NULL;
    m_pCharacterInstance = NULL;
    m_bWrinklesEnabled = false;
}

//////////////////////////////////////////////////////////////////////////

bool CWrinkleMapShaderParamCallback::Init()
{
    if (m_pCharacterInstance == 0)
    {
        return false;
    }

    SetupBoneWrinkleMapInfo();

    return true;
}

//////////////////////////////////////////////////////////////////////////

void CWrinkleMapShaderParamCallback::SetupBoneWrinkleMapInfo()
{
    IDefaultSkeleton& rIDefaultSkeleton = m_pCharacterInstance->GetIDefaultSkeleton();
    {
        // find the wrinkle bones, if any
        for (uint i = 0; i < MAX_WRINKLE_BONES; ++i)
        {
            char acTmp[32];
            sprintf_s(acTmp, 32, "%s_%02d", WRINKLE_BONE_PREFIX, i + 1);

            SWrinkleBoneInfo info;
            info.m_nChannelID = i;
            info.m_nJointID = rIDefaultSkeleton.GetJointIDByName(acTmp);
            if (info.m_nJointID >= 0)
            {
                m_WrinkleBoneInfo.push_back(info);
            }
        }
    }

    m_eSemantic[0] = 0;
    m_eSemantic[1] = 0;
    m_eSemantic[2] = 0;
}

//////////////////////////////////////////////////////////////////////////

void CWrinkleMapShaderParamCallback::SetupShaderParams(
    IShaderPublicParams* pParams, _smart_ptr<IMaterial> pMaterial)
{
    // setup the bone wrinkle map info if there are any bones initialized
    if (pParams && m_pCharacterInstance && !m_WrinkleBoneInfo.empty())
    {
        if (!pMaterial)
        {
            pMaterial = m_pCharacterInstance->GetIMaterial();
        }
        if (pMaterial != m_pCachedMaterial || gEnv->IsEditor())
        {
            m_bWrinklesEnabled = false;

            if (!pMaterial)
            {
                pMaterial = FindFirstMaterialWithWrinkleBlending(*m_pCharacterInstance);
            }
            m_bWrinklesEnabled = pMaterial != NULL;

            if (!pMaterial)
            {
                pMaterial = m_pCharacterInstance->GetIMaterial();
            }
            m_pCachedMaterial = pMaterial;
        }

        if (m_bWrinklesEnabled && m_pCharacterInstance->GetISkeletonPose())
        {
            ISkeletonPose* pSkeletonPose = m_pCharacterInstance->GetISkeletonPose();

            static const char* wrinkleMask[3] = { WRINKLE_MASK_NAME0, WRINKLE_MASK_NAME1, WRINKLE_MASK_NAME2 };
            SShaderParam* pShaderParams[3] = { NULL, NULL, NULL };
            for (uint i = 0; i < 3; ++i)
            {
                // retrieve the shader param
                if (m_eSemantic[i] == 0)
                {
                    m_eSemantic[i] = pParams->GetSemanticByName(wrinkleMask[i]);
                }

                assert(m_eSemantic[i] != 0);

                if (pParams->GetParamBySemantic(m_eSemantic[i]) == 0)
                {
                    SShaderParam newParam;
                    cry_strcpy(newParam.m_Name, wrinkleMask[i]);
                    newParam.m_Type = eType_FCOLOR;
                    newParam.m_eSemantic = m_eSemantic[i];
                    pParams->AddParam(newParam);
                }
            }
            for (uint i = 0; i < 3; ++i)
            {
                pShaderParams[i] = pParams->GetParamBySemantic(m_eSemantic[i]);
                assert(pShaderParams[i] != NULL);
            }

            uint infoCount = m_WrinkleBoneInfo.size();
            for (uint i = 0; i < infoCount; ++i)
            {
                const SWrinkleBoneInfo& info = m_WrinkleBoneInfo[i];
                const QuatT& jointTransform = pSkeletonPose->GetRelJointByID(info.m_nJointID);

                if (info.m_nChannelID < 4)
                {
                    if (pShaderParams[0])
                    {
                        pShaderParams[0]->m_Value.m_Color[i] = jointTransform.t.x;
                    }
                    if (pShaderParams[1])
                    {
                        pShaderParams[1]->m_Value.m_Color[i] = jointTransform.t.y;
                    }
                    if (pShaderParams[2])
                    {
                        pShaderParams[2]->m_Value.m_Color[i] = jointTransform.t.z;
                    }
                }
            }
        }
    }
}

void CWrinkleMapShaderParamCallback::Disable(IShaderPublicParams* pParams)
{
    static const char* WrinkleMask[3] = { WRINKLE_MASK_NAME0, WRINKLE_MASK_NAME1, WRINKLE_MASK_NAME2 };

    if (pParams)
    {
        for (uint i = 0; i < 3; ++i)
        {
            pParams->RemoveParamByName(WrinkleMask[i]);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
