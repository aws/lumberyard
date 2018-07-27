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

#ifndef CRYINCLUDE_CRYAISYSTEM_XMLUTILS_H
#define CRYINCLUDE_CRYAISYSTEM_XMLUTILS_H
#pragma once


namespace XMLUtils
{
    enum BoolType
    {
        Invalid = -1,
        False = 0,
        True = 1,
    };

    inline BoolType ToBoolType(const char* str)
    {
        if (!_stricmp(str, "1") || !_stricmp(str, "true") || !_stricmp(str, "yes"))
        {
            return True;
        }

        if (!_stricmp(str, "0") || !_stricmp(str, "false") || !_stricmp(str, "no"))
        {
            return False;
        }

        return Invalid;
    }

    inline BoolType GetBoolType(const XmlNodeRef& node, const char* attribute, const BoolType& deflt)
    {
        if (node->haveAttr(attribute))
        {
            const char* value;
            node->getAttr(attribute, &value);

            return ToBoolType(value);
        }

        return deflt;
    }
}

#endif // CRYINCLUDE_CRYAISYSTEM_XMLUTILS_H
