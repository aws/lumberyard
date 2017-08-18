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

#ifndef CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_VEGETATIONTRANSFERCONFIGURATOR_H
#define CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_VEGETATIONTRANSFERCONFIGURATOR_H
#pragma once


#if defined(OFFLINE_COMPUTATION)

#include <PRT/ITransferConfigurator.h>
#include <PRT/TransferParameters.h>
#include <PRT/ISHMaterial.h>
#include <PRT/SHFrameworkBasis.h>

namespace NSH
{
    namespace NTransfer
    {
        //!< implements ITransferConfigurator for default transfers
        class CVegetationTransferConfigurator
            : public ITransferConfigurator
        {
        public:
            INSTALL_CLASS_NEW(CVegetationTransferConfigurator)

            CVegetationTransferConfigurator(const STransferParameters& crParameters);   //!< ctor with transfer parameters
            virtual const bool ProcessOnlyUpperHemisphere(const NMaterial::EMaterialType) const;
            virtual const bool UseCoefficientLookupMode() const;
            virtual ~CVegetationTransferConfigurator(){}
            virtual ITransferConfiguratorPtr Clone() const;

        protected:
            STransferParameters m_TransferParameters;   //!< copied transfer parameters

            CVegetationTransferConfigurator(){};                    //!< only valid with transfer parameters
        };
    }
}

#include "VegetationTransferConfigurator.inl"

#endif
#endif // CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_VEGETATIONTRANSFERCONFIGURATOR_H
