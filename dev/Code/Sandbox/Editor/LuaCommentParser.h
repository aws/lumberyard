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

// Description : Parses a lua file by text in order to extract the comments and use them
//               for example in the editor
//
// Todo        : Include included-scripts that are defined by "Script.ReloadScript"
//               (example: Grunt -- > grunt_x --> BasicActor)


#ifndef CRYINCLUDE_EDITOR_LUACOMMENTPARSER_H
#define CRYINCLUDE_EDITOR_LUACOMMENTPARSER_H
#pragma once


struct LuaTable
{
    LuaTable(int open, int close, string name)
    {
        m_Name = name;
        m_OpenBracePos = open;
        m_CloseBracePos = close;
    }

    void AddChild(LuaTable* table)
    {
        m_ChildTables.push_back(table);
    }

    LuaTable* FindChildByName(const char* name)
    {
        for (int i = 0; i < m_ChildTables.size(); i++)
        {
            if (m_ChildTables[i]->m_Name == name)
            {
                return m_ChildTables[i];
            }
        }
        return NULL;
    }

    ~LuaTable()
    {
        for (int i = 0; i < m_ChildTables.size(); i++)
        {
            delete m_ChildTables[i];
        }
        m_ChildTables.resize(0);
    }

    string m_Name;
    int m_OpenBracePos;
    int m_CloseBracePos;
    std::vector<LuaTable*> m_ChildTables;
};

class LuaCommentParser
{
public:
    bool OpenScriptFile(const char* path);
    bool ParseComment(const char* tablePath, const char* varName, float* minVal, float* maxVal, float* stepVal, string* desc);
    static LuaCommentParser* GetInstance();
    ~LuaCommentParser(void);
    void CloseScriptFile();
protected:
    int FindTables(LuaTable* parentTable = NULL, int offset = 0);
    string FindTableNameForBracket(int bracketPos);
    bool IsAlphaNumericalChar(char c);
    int GetVarInTable(const char* tablePath, const char* varName);
    int FindStringInFileSkippingComments(string searchString, int offset = 0);
    int FindWordInFileSkippingComments(string searchString, int offset = 0);
protected:
    AZ::IO::HandleType m_fileHandle;
    char* m_AllText;
    char* m_MovingText;
private:
    LuaCommentParser();
    LuaTable* m_RootTable;
};

#endif // CRYINCLUDE_EDITOR_LUACOMMENTPARSER_H
