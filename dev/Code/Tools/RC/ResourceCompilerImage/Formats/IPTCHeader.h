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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_FORMATS_IPTCHEADER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_FORMATS_IPTCHEADER_H
#pragma once


class CIPTCHeader
{
public:
    enum FieldType
    {
        FieldByline                         = 0x50,
        FieldBylineTitle                    = 0x55,
        FieldCredits                        = 0x6E,
        FieldSource                         = 0x73,
        FieldObjectName                     = 0x05,
        FieldDateCreated                    = 0x37,
        FieldCity                           = 0x5A,
        FieldState                          = 0x5F,
        FieldCountry                        = 0x65,
        FieldOriginalTransmissionReference  = 0x67,
        FieldCopyrightNotice                = 0x74,
        FieldCaption                        = 0x78,
        FieldCaptionWriter                  = 0x7A,
        FieldHeadline                       = 0x69,
        FieldSpecialInstructions            = 0x28,
        FieldCategory                       = 0x0F,
        FieldSupplementalCategories         = 0x14,
        FieldKeywords                       = 0x19
    };

    void Parse(const unsigned char* buffer, int length);
    void GetCombinedFields(FieldType field, std::vector<unsigned char>& buffer, const string& fieldSeparator) const;
    void GetHeader(std::vector<unsigned char>& buffer) const;

private:
    typedef std::multimap<int, std::vector<unsigned char> > FieldContainer;
    FieldContainer m_fields;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_FORMATS_IPTCHEADER_H
