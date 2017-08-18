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
#ifndef GEM_D48CA459_D064_4521_AAD3_7F08466EF83A_CODE_SOURCE_TORNADOESGEM_H
#define GEM_D48CA459_D064_4521_AAD3_7F08466EF83A_CODE_SOURCE_TORNADOESGEM_H

#include <IGem.h>

class TornadoesGem
    : public CryHooksModule
{
public:
    AZ_RTTI(TornadoesGem, "{4CE1D199-1B67-4694-9829-02CFBA6205D9}", CryHooksModule)

    ~TornadoesGem() override = default;

    void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
    void PostGameInitialize();
};

#endif//GEM_D48CA459_D064_4521_AAD3_7F08466EF83A_CODE_SOURCE_TORNADOESGEM_H
