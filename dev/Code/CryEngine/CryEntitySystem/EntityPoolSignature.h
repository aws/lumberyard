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

// Description : Signature generated to describe the blueprint of proxies
//               and their internal status that comprise an Entity generated
//               from the given class


#ifndef CRYINCLUDE_CRYENTITYSYSTEM_ENTITYPOOLSIGNATURE_H
#define CRYINCLUDE_CRYENTITYSYSTEM_ENTITYPOOLSIGNATURE_H
#pragma once


#include "ISerialize.h"

class CEntityPoolSignature
{
public:
    CEntityPoolSignature();

    operator bool() const {
        return m_bGenerated;
    }
    bool operator ==(const CEntityPoolSignature& otherSignature) const { return CompareSignature(otherSignature); }
    bool operator !=(const CEntityPoolSignature& otherSignature) const { return !(*this == otherSignature); }

    //! Returns if this signature has been generated yet
    bool IsGenerated() const { return m_bGenerated; }

    //! Calculate the signature using an instantiated Entity container as a reference
    bool CalculateFromEntity(IEntity* pEntity);

    //! Compare two signatures to see if they match up
    bool CompareSignature(const CEntityPoolSignature& otherSignature) const;

private:
    //! Helpers for signature comparing
    static bool CompareNodes(const XmlNodeRef& a, const XmlNodeRef& b, bool bRecursive = true);
    static bool CompareNodeAttributes(const XmlNodeRef& a, const XmlNodeRef& b);

private:
    bool m_bGenerated;

    // Serialized signature for in-depth checking
    _smart_ptr<IXmlSerializer> m_pSignatureSerializer;
    XmlNodeRef m_Signature;
};

#endif // CRYINCLUDE_CRYENTITYSYSTEM_ENTITYPOOLSIGNATURE_H
