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
#ifndef GEM_8827E544_50F0_40DB_AED17_76BA4B35B43_CODE_SOURCE_SNOWGEM_H
#define GEM_8827E544_50F0_40DB_AED17_76BA4B35B43_CODE_SOURCE_SNOWGEM_H

#include <IGem.h>

class SnowGem
    : public CryHooksModule
{
public:
    AZ_RTTI(SnowGem, "{40B42DAF-4A52-467E-9268-7BFA9AD5DFE9}", CryHooksModule);

    ~SnowGem() override = default;

    void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
    void PostGameInitialize();
};

#endif//GEM_8827E544_50F0_40DB_AED17_76BA4B35B43_CODE_SOURCE_SNOWGEM_H
