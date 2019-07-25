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

#include <AzCore/std/string/string.h>

namespace DeployTool
{
    /*
    * JsonPreProcessor is a json parser that removes # style line comments
    */
    class JsonPreProcessor
    {
    public:
        // Pass in the json text to process
        JsonPreProcessor(const char* jsonText);
        ~JsonPreProcessor();

        // Return the processed text without any comments
        AZStd::string GetResult() const;

    private:

        // Returns the number of characters remaining to parse.
        int CharactersRemaining() const;

        // Return the current character and move the current position to the next one.
        char Advance();

        // Parse characters in a json string
        void ConsumeJsonString();

        // Parse characters in a double quoted string.
        void ConsumeQuotedString();

        // Parse characters inside a # style line comment
        void ConsumeCommentString();

        // Store a character for output, comment characters are not stored.
        void StoreCharacter(char token);

        AZStd::string m_inputText;
        AZStd::string m_outputText;
        int m_currentPos = 0;
    };

}