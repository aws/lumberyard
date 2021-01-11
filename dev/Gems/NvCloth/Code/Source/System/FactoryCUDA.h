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

#pragma once

#include <System/Factory.h>

namespace NvCloth
{
    //! All objects constructed by this factory will run on GPU using CUDA.
    //!
    //! @note It will fallback to CPU simulation when CUDA is not available.
    class FactoryCUDA
        : public Factory
    {
    public:
        AZ_RTTI(FactoryCUDA, "{8AB5C8FE-218B-44FF-BF2C-A503C756A631}", Factory);

        void Init() override;
        void Destroy() override;

        //! Returns true if CUDA was successfully initialized.
        bool IsUsingCUDA() const { return m_isUsingCUDA; }

    private:
        bool m_isUsingCUDA = false;
    };

} // namespace NvCloth
