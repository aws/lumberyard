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

#pragma once

#include <memory>
#include <vector>

#include "DBAEnumerator.h"

struct DBATableEntry
{
    string path;
    std::vector<string> animations;
};
typedef std::vector<DBATableEntry> DBATableEntries;

struct IPakSystem;
class ICryXML;
struct SDBATable;

class DBATableEnumerator
    : public IDBAEnumerator
{
public:
    DBATableEnumerator();
    ~DBATableEnumerator();
    bool LoadDBATable(const string& animConfigFolder, const string& sourceFolder, IPakSystem* pak, ICryXML* xml);

    int GetDBACount() const override { return m_dbas.size(); }
    virtual void GetDBA(EnumeratedDBA* dba, int index) const override;
    virtual bool GetCAF(EnumeratedCAF* caf, int dbaIndex, int animationIndex) const override;
    const char* FindDBAPath(const char* animationPath, const char* skeleton, const std::vector<string>& tags) const;

private:
    std::auto_ptr<SDBATable> m_table;
    typedef std::map<string, size_t> TAnimationDBAMap;
    DBATableEntries m_dbas;
};
