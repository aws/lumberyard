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

#include <LyShine/ISprite.h>
#include <platform.h>
#include <StlUtils.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class CSprite
    : public ISprite
{
public: // member functions

    //! Construct an empty sprite
    CSprite();

    // ISprite

    ~CSprite() override;

    const string& GetPathname() const override;
    const string& GetTexturePathname() const override;
    Borders GetBorders() const override;
    void SetBorders(Borders borders) override;
    ITexture* GetTexture() const override;
    void Serialize(TSerialize ser) override;
    bool SaveToXml(const string& pathname) override;
    bool AreBordersZeroWidth() const override;
    AZ::Vector2 GetSize() const override;

    // ~ISprite

public: // static member functions

    static void Initialize();
    static void Shutdown();
    static CSprite* LoadSprite(const string& pathname);
    static CSprite* CreateSprite(const string& renderTargetName);

    //! Replaces baseSprite with newSprite with proper ref-count handling and null-checks.
    static void ReplaceSprite(ISprite** baseSprite, ISprite* newSprite);

private: // types
    typedef AZStd::unordered_map<string, CSprite*, stl::hash_string_caseless<string>, stl::equality_string_caseless<string> > CSpriteHashMap;

private: // member functions
    bool LoadFromXmlFile();

    AZ_DISABLE_COPY_MOVE(CSprite);

private: // data
    string m_pathname;
    string m_texturePathname;
    Borders m_borders;
    ITexture* m_texture;

    //! Used to keep track of all loaded sprites. Sprites are refcounted.
    static CSpriteHashMap* s_loadedSprites;
};
