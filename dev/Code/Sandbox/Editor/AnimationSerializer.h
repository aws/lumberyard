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

#ifndef CRYINCLUDE_EDITOR_ANIMATIONSERIALIZER_H
#define CRYINCLUDE_EDITOR_ANIMATIONSERIALIZER_H

#pragma once

// forward declarations.
struct IAnimSequence;
class CPakFile;

/** Used by Editor to serialize animation data.
*/
class CAnimationSerializer
{
public:
    CAnimationSerializer();
    ~CAnimationSerializer();

    /** Save all animation sequences to files in given directory.
    */
    void SerializeSequences(XmlNodeRef& xmlNode, bool bLoading);

    /** Saves single animation sequence to file in given directory.
    */
    void SaveSequence(IAnimSequence* seq, const char* szFilePath, bool bSaveEmpty = true);

    /** Load sequence from file.
    */
    IAnimSequence* LoadSequence(const char* szFilePath);

    /** Save all animation sequences to files in given directory.
    */
    void SaveAllSequences(const char* szPath, CPakFile& pakFile);

    /** Load all animation sequences from given directory.
    */
    void LoadAllSequences(const char* szPath);
};

#endif // CRYINCLUDE_EDITOR_ANIMATIONSERIALIZER_H
