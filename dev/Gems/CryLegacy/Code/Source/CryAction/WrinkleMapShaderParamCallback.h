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
#ifndef CRYINCLUDE_CRYACTION_WRINKLEMAPSHADERPARAMCALLBACK_H
#define CRYINCLUDE_CRYACTION_WRINKLEMAPSHADERPARAMCALLBACK_H
#pragma once

#include <IShaderParamCallback.h>
#include <CryExtension/Impl/ClassWeaver.h>

class CWrinkleMapShaderParamCallback
    : public IShaderParamCallback
{
    CRYINTERFACE_BEGIN()
    CRYINTERFACE_ADD(IShaderParamCallback)
    CRYINTERFACE_END()

    CRYGENERATE_CLASS(CWrinkleMapShaderParamCallback, "WrinkleMapShaderParamCallback", 0x68c7f0e0c36446fe, 0x82a3bc01b54dc7bf)

public:

    //////////////////////////////////////////////////////////////////////////
    //  Implement IShaderParamCallback
    //////////////////////////////////////////////////////////////////////////

    virtual void SetCharacterInstance(ICharacterInstance* pCharInstance)
    {
        m_pCharacterInstance = pCharInstance;
    }

    virtual ICharacterInstance* GetCharacterInstance() const
    {
        return m_pCharacterInstance;
    }

    virtual bool Init();
    virtual void SetupShaderParams(IShaderPublicParams* pParams, _smart_ptr<IMaterial> pMaterial);
    virtual void Disable(IShaderPublicParams* pParams);

protected:

    void SetupBoneWrinkleMapInfo();

    //////////////////////////////////////////////////////////////////////////

    ICharacterInstance* m_pCharacterInstance;

    struct SWrinkleBoneInfo
    {
        int16 m_nChannelID;
        int16 m_nJointID;
    };
    typedef std::vector<SWrinkleBoneInfo> TWrinkleBoneInfo;
    TWrinkleBoneInfo m_WrinkleBoneInfo;

    _smart_ptr<IMaterial> m_pCachedMaterial;

    uint8 m_eSemantic[3];

    bool m_bWrinklesEnabled;
};

CREATE_SHADERPARAMCALLBACKUI_CLASS(CWrinkleMapShaderParamCallback, "bWrinkleMap", 0x1B9D46925918485B, 0xB7312C8FB3F5B763)

#endif // CRYINCLUDE_CRYACTION_WRINKLEMAPSHADERPARAMCALLBACK_H
