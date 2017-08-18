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

#ifndef CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_TRANSFERCONFIGURATORFACTORY_H
#define CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_TRANSFERCONFIGURATORFACTORY_H
#pragma once

#if defined(OFFLINE_COMPUTATION)


#include <PRT/PRTTypes.h>
#include <PRT/ITransferConfigurator.h>

namespace NSH
{
    namespace NTransfer
    {
        enum ETransferConfigurator;
        struct ITransferConfigurator;
        struct STransferParameters;

        //!< singleton to abstract transfer configurator factory
        class CTransferConfiguratorFactory
        {
        public:
            //!< singleton stuff
            static CTransferConfiguratorFactory* Instance();

            const ITransferConfiguratorPtr GetTransferConfigurator
            (
                const NSH::NTransfer::STransferParameters& crParameters
            );

        private:
            //!< singleton stuff
            CTransferConfiguratorFactory(){}
            CTransferConfiguratorFactory(const CTransferConfiguratorFactory&);
            CTransferConfiguratorFactory& operator= (const CTransferConfiguratorFactory&);
        };
    }
}

#endif

#endif // CRYINCLUDE_TOOLS_SPHERICALHARMONICS_PRT_TRANSFERCONFIGURATORFACTORY_H
