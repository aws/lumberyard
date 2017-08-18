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
#ifndef CRYINCLUDE_CRYCOMMON_IGEMREGISTRY_H
#define CRYINCLUDE_CRYCOMMON_IGEMREGISTRY_H
#pragma once

#include <AzCore/PlatformDef.h>

#include "CryExtension/ICryUnknown.h"
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

class IGem;

class IGemManager
{
public:
    /**
     * Public functions
     */
    virtual ~IGemManager() { }

    /**
    * Get a Gem by type
    *
    * \tparam GemT     The Gem type to get
    *
    * \returns         A pointer to the Gem instance requested, or nullptr if not found
    */
    template<typename GemT>
    const GemT* GetGem() const
    {
        return static_cast<const GemT*>(GetGem(cryiidof<GemT>()));
    }

    /**
     * Get a Gem by type
     *
     * \tparam GemT     The Gem type to get
     *
     * \returns         A pointer to the Gem instance requested, or nullptr if not found
     */
    template<typename GemT>
    GemT* GetGem()
    {
        return static_cast<GemT*>(GetGem(cryiidof<GemT>()));
    }

    /**
    * Checks if a Gem of the specified uuid and version constraints is enabled.
    *
    * \param[in] id                     The ID of the Gem to check.
    * \param[in] versionConstraints     An array of strings, each of which is a condition using the gem dependency syntax
    *
    * \returns                          True if the Gem is enabled and passes every version constraint condition, False if it is disabled or does not match the version constraints
    */
    virtual bool IsGemEnabled(const AZ::Uuid& id, const AZStd::vector<AZStd::string>& versionConstraints) const = 0;

    /**
     * Utility Methods
     */

    /**
     * Should only be called by the Engine
     */

    /**
     * Initialize the registry.
     */
    virtual bool Initialize(const SSystemInitParams& initParams) = 0;

private:
    /**
    * Get a Gem by name
    *
    * \param classID   The CryClassID for this IGem
    *
    * \returns         A pointer to the Gem instance requested, or nullptr if not found
    */
    virtual const IGem* GetGem(const CryClassID& classID) const = 0;

    /**
    * Get a Gem by name
    *
    * \param classID   The CryClassID for this IGem
    *
    * \returns         A pointer to the Gem instance requested, or nullptr if not found
    */
    IGem* GetGem(const CryClassID& classID)
    {
        return const_cast<IGem*>(static_cast<const IGemManager*>(this)->GetGem(classID));
    }
};

#endif // CRYINCLUDE_CRYCOMMON_IGEMREGISTRY_H
