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
#include "DeploymentTool_precompiled.h"
#include "JsonPreProcessor.h"

namespace DeployTool
{

    JsonPreProcessor::JsonPreProcessor(const char* jsonText)
        :m_inputText(jsonText)
    {
        ConsumeJsonString();
    }

    JsonPreProcessor::~JsonPreProcessor()
    {
    }

    AZStd::string JsonPreProcessor::GetResult() const
    {
        return m_outputText;
    }

    int JsonPreProcessor::CharactersRemaining() const
    {
        return m_inputText.length() - m_currentPos;
    }

    char JsonPreProcessor::Advance()
    {
        return m_inputText[m_currentPos++];
    }

    void JsonPreProcessor::ConsumeJsonString()
    {
        while (CharactersRemaining())
        {
            char token = Advance();
            if (token == '"')
            {
                StoreCharacter(token);
                ConsumeQuotedString();
            }
            else if (token == '#')
            {
                ConsumeCommentString();
            }
            else
            {
                StoreCharacter(token);
            }
        }
    }

    void JsonPreProcessor::ConsumeQuotedString()
    {
        char lastToken = 0;
        while (CharactersRemaining())
        {
            char token = Advance();
            StoreCharacter(token);
            if (token == '"' && lastToken != '\\')
            {
                return;
            }
            lastToken = token;
        }
    }

    void JsonPreProcessor::ConsumeCommentString()
    {
        while (CharactersRemaining())
        {
            char token = Advance();
            if (token == '\n')
            {
                StoreCharacter(token);
                return;
            }
        }
    }

    void JsonPreProcessor::StoreCharacter(char token)
    {
        m_outputText += token;
    }
}