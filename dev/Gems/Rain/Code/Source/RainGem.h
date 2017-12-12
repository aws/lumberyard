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
#ifndef GEM_E5F049AD_7F53_4847_A89C_27B7339CF6A6_CODE_SOURCE_RAINGEM_H
#define GEM_E5F049AD_7F53_4847_A89C_27B7339CF6A6_CODE_SOURCE_RAINGEM_H

#include <IGem.h>

class RainGem
    : public CryHooksModule
{
public:
    AZ_RTTI(RainGem, "{A5E81E5A-FD45-4875-A1B1-ECD0F3C94D7C}", CryHooksModule);

    ~RainGem() override = default;

    void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
    void PostGameInit();
};

#endif//GEM_E5F049AD_7F53_4847_A89C_27B7339CF6A6_CODE_SOURCE_RAINGEM_H
