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

#include "stdafx.h"

#if defined(OFFLINE_COMPUTATION)

#include "TransferConfiguratorFactory.h"
#include "DefaultTransferConfigurator.h"
#include "VegetationTransferConfigurator.h"
#include <PRT/TransferParameters.h>
#include <PRT/ITransferConfigurator.h>

NSH::NTransfer::CTransferConfiguratorFactory* NSH::NTransfer::CTransferConfiguratorFactory::Instance()
{
    static CTransferConfiguratorFactory inst;
    return &inst;
}

const NSH::ITransferConfiguratorPtr NSH::NTransfer::CTransferConfiguratorFactory::GetTransferConfigurator
(
    const NSH::NTransfer::STransferParameters& crParameters
)
{
    switch (crParameters.configType)
    {
    case TRANSFER_CONFIG_VEGETATION:
        return ITransferConfiguratorPtr(new CVegetationTransferConfigurator(crParameters));
        break;
    case TRANSFER_CONFIG_DEFAULT:
    default:
        return ITransferConfiguratorPtr(new CDefaultTransferConfigurator(crParameters));
        break;
    }
}

#endif