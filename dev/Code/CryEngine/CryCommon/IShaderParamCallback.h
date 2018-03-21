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

#ifndef __IShaderParamCallback_h__
#define __IShaderParamCallback_h__
#pragma once

#include <CryExtension/ICryUnknown.h>

struct ICharacterInstance;
struct IShaderPublicParams;

// callback object which can be used to override shader params for the game side
struct IShaderParamCallback
    : public ICryUnknown
{
    CRYINTERFACE_DECLARE(IShaderParamCallback, 0x4fb87a5f83f74323, 0xa7e42ca947c549d8)

    // <interfuscator:shuffle>
    // setting actual object to be worked on, but should ideally all derive
    // from a same base pointer for characters, rendermeshes, vegetation
    virtual void SetCharacterInstance(ICharacterInstance* pCharInstance) {}
    virtual ICharacterInstance* GetCharacterInstance() const { return NULL; }

    virtual bool Init() = 0;

    // Called just before submitting the render component for rendering,
    // and can be used to setup game specific shader params
    virtual void SetupShaderParams(IShaderPublicParams* pParams, _smart_ptr<IMaterial> pMaterial) = 0;

    virtual void Disable(IShaderPublicParams* pParams) = 0;
    // </interfuscator:shuffle>
};

DECLARE_SMART_POINTERS(IShaderParamCallback);

// These macros are used to create an extra friendly-named class used to instantiate ShaderParamCallbacks based on lua properties

#define CREATE_SHADERPARAMCALLBACKUI_CLASS(classname, UIname, guidhigh, guidlow) \
    class classname##UI                                                          \
        : public classname                                                       \
    {                                                                            \
                                                                                 \
        CRYINTERFACE_BEGIN()                                                     \
        CRYINTERFACE_ADD(classname)                                              \
        CRYINTERFACE_END()                                                       \
                                                                                 \
        CRYGENERATE_CLASS(classname##UI, UIname, guidhigh, guidlow)              \
    };                                                                           \

#define CRYIMPLEMENT_AND_REGISTER_SHADERPARAMCALLBACKUI_CLASS(classname) \
    CRYREGISTER_CLASS(classname##UI)                                     \
    classname##UI::classname##UI(){}                                     \
    classname##UI::~classname##UI(){}                                    \

#endif //__IShaderParamCallback_h__
