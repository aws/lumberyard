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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_DBAENUMERATOR_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_DBAENUMERATOR_H
#pragma once

class CSkeletonInfo;

struct EnumeratedDBA
{
    string innerPath;
    size_t animationCount;

    EnumeratedDBA()
        : animationCount(0)
    {
    }
};

struct EnumeratedCAF
{
    string path;
    bool skipDBA;

    EnumeratedCAF()
        : skipDBA(false)
    {
    }
};

struct IDBAEnumerator
{
    virtual ~IDBAEnumerator() {}

    virtual int GetDBACount() const = 0;
    virtual void GetDBA(EnumeratedDBA* dba, int index) const = 0;
    virtual bool GetCAF(EnumeratedCAF* caf, int dbaIndex, int animationIndex) const = 0;
};

// ---------------------------------------------------------------------------
class DBAManager;
struct CBAEntry;

class DBAManagerEnumerator
    : public IDBAEnumerator
{
public:
    DBAManagerEnumerator(DBAManager* m_dbaManager, const string& sourceRoot);
    int GetDBACount() const override;
    void GetDBA(EnumeratedDBA* dba, int index) const override;
    bool GetCAF(EnumeratedCAF* caf, int dbaIndex, int animationIndex) const override;
private:
    DBAManager* m_dbaManager;
    string m_sourceRoot;
};

struct CAFFileItem
{
    string m_name;
    uint32 m_size;

    bool operator<(const CAFFileItem& rhs) const
    {
        if (m_size == rhs.m_size)
        {
            return m_name < rhs.m_name;
        }
        else
        {
            return m_size > rhs.m_size;
        }
    }
};

class CAnimationInfoLoader;
class CBAEnumerator
    : public IDBAEnumerator
{
public:
    CBAEnumerator(CAnimationInfoLoader* m_cbaLoader, std::vector<CBAEntry>& m_cbaEntries, const string& cbaFolderPath);
    int GetDBACount() const override;
    void GetDBA(EnumeratedDBA* dba, int index) const override;
    bool GetCAF(EnumeratedCAF* caf, int dbaIndex, int animationIndex) const override;
private:
    void CacheItems(int dbaIndex) const;

    CAnimationInfoLoader* m_cbaLoader;
    string m_cbaFolderPath;
    std::vector<CBAEntry>& m_cbaEntries;
    mutable std::vector<CAFFileItem> m_items;
    mutable int m_itemsDBAIndex;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_CGA_DBAENUMERATOR_H
