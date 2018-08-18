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
#ifndef CRYINCLUDE_CRYCOMMON_PARSECONFIG_H
#define CRYINCLUDE_CRYCOMMON_PARSECONFIG_H
#pragma once

#include <stdio.h>
#include "platform.h"

#if defined(AZ_PLATFORM_ANDROID)
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <AzCore/Android/Utils.h>
#endif

//Base Class for reading Configuration
//////////////////////////////////////////////////////////////////////////
class CParseConfig
{
protected:
    // convert a string in-place and return false if the entire line is a comment or all whitespace
    // note:  does not trim out whitespace in the front or back if its present, but does tolerate it

    bool FilterOutComments(char* buffer)
    {
        if ((!buffer) || (buffer[0] == 0))
        {
            return false;
        }

        char* firstNonWhitespace = nullptr;
        char* commentStart = nullptr;
        char* checkPos = buffer;
        while (*checkPos)
        {
            char currentChar = *checkPos;
            // current assumption is comments start with '--'
            if (currentChar == '-')
            {
                if (commentStart)
                {
                    // its the second consecutive dash symbol, we're now in a comment.
                    // reminder of line is comment
                    *commentStart = 0;
                    return (firstNonWhitespace != nullptr);
                }
                else
                {
                    commentStart = checkPos;
                    // we might be in a comment, if we see another dash - symbol then its a comment.
                }
            }
            else
            {
                commentStart = nullptr; // not a consecutive - symbol

                if (!firstNonWhitespace)
                {
                    if (!strchr("\t ", currentChar))
                    {
                        firstNonWhitespace = checkPos;
                    }
                }
            }
            ++checkPos;
        }
        // we got to the end of the string and didn't find a comment.
        return (firstNonWhitespace != nullptr);
    }

    const int textBufferOverflowGuard = 16;

    //////////////////////////////////////////////////////////////////////////
    // NOTE: caller assumes responsibility for deleting the buffer allocated by either of these two functions
#if defined(AZ_PLATFORM_ANDROID)
    char* androidOpenAndReadCFG(const char* filename, int& length)
    {
        AAssetManager* am = nullptr;
        char* sAllText = nullptr;
        const char* fname = filename;
        {
            using namespace AZ::Android::Utils;

            if (IsApkPath(fname))
            {
                fname = StripApkPrefix(fname);
            }

            am = GetAssetManager();
        }

        if (am != nullptr)
        {
            AAsset* pAsset = AAssetManager_open(am, fname, AASSET_MODE_UNKNOWN);

            if (pAsset != nullptr)
            {
                length = AAsset_getLength(pAsset);
                sAllText = new char[length + textBufferOverflowGuard];
                AAsset_read(pAsset, sAllText, length);
                AAsset_close(pAsset);
                sAllText[length] = '\0';
                sAllText[length + 1] = '\0'; //guard against buffer overflows
            }
        }
        return sAllText;
    }
#endif // (AZ_PLATFORM_ANDROID)

    char* OpenAndReadCFG(const char* filename, int& length)
    {
        char* sAllText = nullptr;
        FILE* file = nullptr;
        azfopen(&file, filename, "rb");
        if (file != nullptr)
        {
            fseek(file, 0, SEEK_END);
            length = ftell(file);
            fseek(file, 0, SEEK_SET);

            sAllText = new char[length + textBufferOverflowGuard];

            fread(sAllText, 1, length, file);

            fclose(file);

            sAllText[length] = '\0';
            sAllText[length + 1] = '\0';
        }
#if defined(AZ_PLATFORM_ANDROID)
        else
        {
            sAllText = androidOpenAndReadCFG(filename, length);
        }
#endif
        return sAllText;
    }

    //////////////////////////////////////////////////////////////////////////
    bool ParseConfig(const char* filename)
    {
        int nLen = 0;
        char* sAllText = OpenAndReadCFG(filename, nLen);

        if (sAllText == nullptr)
        {
            return false;
        }

        string strGroup;            // current group e.g. "[General]"

        char* strLast = sAllText + nLen;
        char* str = sAllText;
        while (str < strLast)
        {
            char* s = str;
            while (str < strLast && *str != '\n' && *str != '\r')
            {
                str++;
            }
            *str = '\0';
            str++;
            while (str < strLast && (*str == '\n' || *str == '\r'))
            {
                str++;
            }

            string strLine = s;

            // detect groups e.g. "[General]"   should set strGroup="General"
            {
                string strTrimmedLine(RemoveWhiteSpaces(strLine));
                size_t size = strTrimmedLine.size();

                if (size >= 3)
                {
                    if (strTrimmedLine[0] == '[' && strTrimmedLine[size - 1] == ']')        // currently no comments are allowed to be behind groups
                    {
                        strGroup = &strTrimmedLine[1];
                        strGroup.resize(size - 2);                                      // remove [ and ]
                        continue;                                                                                                   // next line
                    }
                }
            }

            // trim comments and ignore all-whitespace lines.
            // we know this fits in sAllText because sAllText is big enough to contain the entire file
            // and that we've already read past that point in the buffer above.
            // note that we don't want to do a strcpy here since 'safe' strcpy versions can fill the remainder with blank characters
            // and that would destroy the remainder of sAllText, which contains the entire file that we haven't read yet.
            memcpy(sAllText, strLine.data(), strLine.size() + 1);
            if (FilterOutComments(sAllText))
            {
                strLine = sAllText;
                // extract key
                string::size_type posEq(strLine.find("=", 0));
                if (string::npos != posEq)
                {
                    string stemp(strLine, 0, posEq);
                    string strKey(RemoveWhiteSpaces(stemp));

                    {
                        // extract value
                        string::size_type posValueStart(strLine.find("\"", posEq + 1) + 1);
                        string::size_type posValueEnd(strLine.rfind('\"'));

                        string strValue;

                        if (string::npos != posValueStart && string::npos != posValueEnd)
                        {
                            strValue = string(strLine, posValueStart, posValueEnd - posValueStart);
                        }
                        else
                        {
                            string strTmp(strLine, posEq + 1, strLine.size() - (posEq + 1));
                            strValue = RemoveWhiteSpaces(strTmp);
                        }

                        {
                            string strTemp;
                            strTemp.reserve(strValue.length() + 1);
                            // replace '\\\\' with '\\' and '\\\"' with '\"'
                            for (string::const_iterator iter = strValue.begin(); iter != strValue.end(); ++iter)
                            {
                                if (*iter == '\\')
                                {
                                    ++iter;
                                    if (iter == strValue.end())
                                    {
                                    }
                                    else if (*iter == '\\')
                                    {
                                        strTemp += '\\';
                                    }
                                    else if (*iter == '\"')
                                    {
                                        strTemp += '\"';
                                    }
                                }
                                else
                                {
                                    strTemp += *iter;
                                }
                            }
                            strValue.swap(strTemp);

                            OnLoadConfigurationEntry(strKey, strValue, strGroup);
                        }
                    }
                }
            }
        }
        delete[] sAllText;


        return true;
    }

    virtual void OnLoadConfigurationEntry(const string& strKey, const string& strValue, const string& strGroup) = 0;


    string RemoveWhiteSpaces(string& s)
    {
        s.Trim();
        return s;
    }
};



#endif // CRYINCLUDE_CRYCOMMON_PARSECONFIG_H
