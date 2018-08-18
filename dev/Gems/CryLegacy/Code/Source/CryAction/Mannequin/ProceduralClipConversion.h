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

#ifndef __PROCEDURAL_CLIP_CONVERSION__H__
#define __PROCEDURAL_CLIP_CONVERSION__H__
#pragma once

#include <vector>
#include <map>

struct SProcClipConversionEntry
{
    typedef std::vector< string > ParamConversionStringList;

    XmlNodeRef Convert(const XmlNodeRef& pOldNode) const;

    XmlNodeRef CreateNodeStructure(XmlNodeRef pNewNode, const ParamConversionStringList& paramList) const;

    void CreateAttribute(XmlNodeRef pNewNode, const ParamConversionStringList& newAttributeName, const char* const newAttributeValue) const;
    void ConvertAttribute(const XmlNodeRef& pOldNode, XmlNodeRef pNewNode, const char* const oldAttributeName, const ParamConversionStringList& newAttributeName) const;

    ParamConversionStringList animRef;
    ParamConversionStringList crcString;
    ParamConversionStringList dataString;

    typedef std::vector< ParamConversionStringList > ParameterConversionVector;
    ParameterConversionVector parameters;
};


class CProcClipConversionHelper
{
public:
    CProcClipConversionHelper();
    XmlNodeRef Convert(const XmlNodeRef& pOldXmlNode);

private:
    void LoadEntry(const XmlNodeRef& pXmlEntryNode);

    typedef std::map< string, SProcClipConversionEntry > ConversionEntryMap;
    ConversionEntryMap conversionMap;
};

#endif