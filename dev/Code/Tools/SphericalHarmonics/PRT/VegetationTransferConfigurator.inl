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

#if defined(OFFLINE_COMPUTATION)

#include <PRT/SHFrameworkBasis.h>
#include <PRT/ISHMaterial.h>

inline NSH::NTransfer::CVegetationTransferConfigurator::CVegetationTransferConfigurator(const STransferParameters& crParameters)
    : m_TransferParameters(crParameters)
{}

inline const bool NSH::NTransfer::CVegetationTransferConfigurator::ProcessOnlyUpperHemisphere(const NMaterial::EMaterialType cMatType) const
{
    switch (cMatType)
    {
    case NSH::NMaterial::MATERIAL_TYPE_DEFAULT:
        return true;
        break;
    case NSH::NMaterial::MATERIAL_TYPE_BASETEXTURE:
        return true;
        break;
    case NSH::NMaterial::MATERIAL_TYPE_ALPHATEXTURE:
    case NSH::NMaterial::MATERIAL_TYPE_ALPHA_DEFAULT:
        return false;
        break;
    case NSH::NMaterial::MATERIAL_TYPE_BACKLIGHTING:
    case NSH::NMaterial::MATERIAL_TYPE_BACKLIGHTING_DEFAULT:
        return false;
        break;
    default:
        return true;
    }
}

inline const bool NSH::NTransfer::CVegetationTransferConfigurator::UseCoefficientLookupMode() const
{
    return true;
}

inline NSH::ITransferConfiguratorPtr NSH::NTransfer::CVegetationTransferConfigurator::Clone() const
{
    return ITransferConfiguratorPtr(new CVegetationTransferConfigurator(m_TransferParameters));
}

#endif