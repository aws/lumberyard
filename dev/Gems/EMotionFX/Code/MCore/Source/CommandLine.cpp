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

// include required headers
#include "CommandLine.h"
#include "Command.h"
#include "CommandSyntax.h"
#include "UnicodeStringIterator.h"
#include "UnicodeCharacter.h"
#include "LogManager.h"

namespace MCore
{
    // default constructor
    CommandLine::CommandLine()
    {
    }


    // extended constructor, which automatically sets the command line to parse
    CommandLine::CommandLine(const char* commandLine)
    {
        SetCommandLine(commandLine);
    }


    // extended constructor, which automatically sets the command line to parse
    CommandLine::CommandLine(const MCore::String& commandLine)
    {
        SetCommandLine(commandLine);
    }


    // the destructor, which cleans the parameters
    CommandLine::~CommandLine()
    {
        mParameters.Clear(true);
    }


    // get the value for a given parameter
    void CommandLine::GetValue(const char* paramName, const char* defaultValue, MCore::String* outResult) const
    {
        // try to find the parameter index
        const uint32 paramIndex = FindParameterIndex(paramName);
        if (paramIndex == MCORE_INVALIDINDEX32)
        {
            *outResult = defaultValue;
            return;
        }

        // return the default value if the parameter value is empty
        if (mParameters[paramIndex].mValue.GetIsEmpty())
        {
            *outResult = defaultValue;
            return;
        }

        // return the parameter value
        *outResult = mParameters[paramIndex].mValue;
    }


    // Get the value for a given parameter.
    void CommandLine::GetValue(const char* paramName, const char* defaultValue, AZStd::string& outResult) const
    {
        // Try to find the parameter index.
        const uint32 paramIndex = FindParameterIndex(paramName);
        if (paramIndex == MCORE_INVALIDINDEX32)
        {
            outResult = defaultValue;
            return;
        }

        // Return the default value if the parameter value is empty.
        if (mParameters[paramIndex].mValue.GetIsEmpty())
        {
            outResult = defaultValue;
            return;
        }

        // Return the actual parameter value.
        outResult = mParameters[paramIndex].mValue.AsChar();
    }


    // get the value as an integer
    int32 CommandLine::GetValueAsInt(const char* paramName, int32 defaultValue) const
    {
        // try to find the parameter index
        const uint32 paramIndex = FindParameterIndex(paramName);
        if (paramIndex == MCORE_INVALIDINDEX32)
        {
            return defaultValue;
        }

        // return the default value if the parameter value is empty
        if (mParameters[paramIndex].mValue.GetIsEmpty())
        {
            return defaultValue;
        }

        // return the parameter value
        return mParameters[paramIndex].mValue.ToInt();
    }


    // get the value as a float
    float CommandLine::GetValueAsFloat(const char* paramName, float defaultValue) const
    {
        // try to find the parameter index
        const uint32 paramIndex = FindParameterIndex(paramName);
        if (paramIndex == MCORE_INVALIDINDEX32)
        {
            return defaultValue;
        }

        // return the default value if the parameter value is empty
        if (mParameters[paramIndex].mValue.GetIsEmpty())
        {
            return defaultValue;
        }

        // return the parameter value
        return mParameters[paramIndex].mValue.ToFloat();
    }


    // get the value as a boolean
    bool CommandLine::GetValueAsBool(const char* paramName, bool defaultValue) const
    {
        // try to find the parameter index
        const uint32 paramIndex = FindParameterIndex(paramName);
        if (paramIndex == MCORE_INVALIDINDEX32)
        {
            return defaultValue;
        }

        // return the default value if the parameter value is empty
        if (mParameters[paramIndex].mValue.GetIsEmpty())
        {
            return true;
        }

        // return the parameter value
        return mParameters[paramIndex].mValue.ToBool();
    }


    // get the value as a three component vector
    Vector3 CommandLine::GetValueAsVector3(const char* paramName, const Vector3& defaultValue) const
    {
        // try to find the parameter index
        const uint32 paramIndex = FindParameterIndex(paramName);
        if (paramIndex == MCORE_INVALIDINDEX32)
        {
            return defaultValue;
        }

        // return the default value if the parameter value is empty
        if (mParameters[paramIndex].mValue.GetIsEmpty())
        {
            return defaultValue;
        }

        // return the parameter value
        return mParameters[paramIndex].mValue.ToVector3();
    }


    // get the value as a four component vector
    AZ::Vector4 CommandLine::GetValueAsVector4(const char* paramName, const AZ::Vector4& defaultValue) const
    {
        // try to find the parameter index
        const uint32 paramIndex = FindParameterIndex(paramName);
        if (paramIndex == MCORE_INVALIDINDEX32)
        {
            return defaultValue;
        }

        // return the default value if the parameter value is empty
        if (mParameters[paramIndex].mValue.GetIsEmpty())
        {
            return defaultValue;
        }

        // return the parameter value
        return mParameters[paramIndex].mValue.ToVector4();
    }


    // get the value of a given param
    void CommandLine::GetValue(const char* paramName, Command* command, MCore::String* outResult) const
    {
        // try to find the parameter index
        const uint32 paramIndex = FindParameterIndex(paramName);
        if (paramIndex == MCORE_INVALIDINDEX32)
        {
            command->GetOriginalCommand()->GetSyntax().GetDefaultValue(paramName, *outResult);
            return;
        }

        // return the parameter value
        *outResult = mParameters[paramIndex].mValue;
    }


    // Get the value of a given param.
    void CommandLine::GetValue(const char* paramName, Command* command, AZStd::string& outResult) const
    {
        // Try to find the parameter index.
        const uint32 paramIndex = FindParameterIndex(paramName);
        if (paramIndex == MCORE_INVALIDINDEX32)
        {
            command->GetOriginalCommand()->GetSyntax().GetDefaultValue(paramName, outResult);
            return;
        }

        // return the parameter value
        outResult = mParameters[paramIndex].mValue.AsChar();
    }


    // get the value as int
    int32 CommandLine::GetValueAsInt(const char* paramName, Command* command) const
    {
        // try to find the parameter index
        const uint32 paramIndex = FindParameterIndex(paramName);
        if (paramIndex == MCORE_INVALIDINDEX32)
        {
            MCore::String result;
            if (command->GetOriginalCommand()->GetSyntax().GetDefaultValue(paramName, result))
            {
                return result.ToInt();
            }
            else
            {
                return MCORE_INVALIDINDEX32;
            }
        }

        // return the parameter value
        return mParameters[paramIndex].mValue.ToInt();
    }

    // get the value as float
    float CommandLine::GetValueAsFloat(const char* paramName, Command* command) const
    {
        // try to find the parameter index
        const uint32 paramIndex = FindParameterIndex(paramName);
        if (paramIndex == MCORE_INVALIDINDEX32)
        {
            MCore::String result;
            if (command->GetOriginalCommand()->GetSyntax().GetDefaultValue(paramName, result))
            {
                return result.ToFloat();
            }
            else
            {
                return 0.0f;
            }
        }

        // return the parameter value
        return mParameters[paramIndex].mValue.ToFloat();
    }


    // get the value as bool
    bool CommandLine::GetValueAsBool(const char* paramName, Command* command) const
    {
        // try to find the parameter index
        const uint32 paramIndex = FindParameterIndex(paramName);
        if (paramIndex == MCORE_INVALIDINDEX32)
        {
            MCore::String result;
            if (command->GetOriginalCommand()->GetSyntax().GetDefaultValue(paramName, result))
            {
                return result.ToBool();
            }
            else
            {
                return false;
            }
        }

        // return the parameter value
        return mParameters[paramIndex].mValue.ToBool();
    }


    // get the value as three component vector
    Vector3 CommandLine::GetValueAsVector3(const char* paramName, Command* command) const
    {
        // try to find the parameter index
        const uint32 paramIndex = FindParameterIndex(paramName);
        if (paramIndex == MCORE_INVALIDINDEX32)
        {
            MCore::String result;
            if (command->GetOriginalCommand()->GetSyntax().GetDefaultValue(paramName, result))
            {
                return result.ToVector3();
            }
            else
            {
                return Vector3(0.0f, 0.0f, 0.0);
            }
        }

        // return the parameter value
        return mParameters[paramIndex].mValue.ToVector3();
    }


    // get the value as four component vector
    AZ::Vector4 CommandLine::GetValueAsVector4(const char* paramName, Command* command) const
    {
        // try to find the parameter index
        const uint32 paramIndex = FindParameterIndex(paramName);
        if (paramIndex == MCORE_INVALIDINDEX32)
        {
            MCore::String result;
            if (command->GetOriginalCommand()->GetSyntax().GetDefaultValue(paramName, result))
            {
                return result.ToVector4();
            }
            else
            {
                return AZ::Vector4(0.0f, 0.0f, 0.0f, 1.0f);
            }
        }

        // return the parameter value
        return mParameters[paramIndex].mValue.ToVector4();
    }


    // get the number of parameters
    uint32 CommandLine::GetNumParameters() const
    {
        return mParameters.GetLength();
    }


    // get the parameter name for a given parameter
    const MCore::String& CommandLine::GetParameterName(uint32 nr) const
    {
        return mParameters[nr].mName;
    }


    // get the parameter value for a given parameter number
    const MCore::String& CommandLine::GetParameterValue(uint32 nr) const
    {
        return mParameters[nr].mValue;
    }


    // check if a given parameter has a value
    bool CommandLine::CheckIfHasValue(const char* paramName) const
    {
        // try to find the parameter index
        const uint32 paramIndex = FindParameterIndex(paramName);
        if (paramIndex == MCORE_INVALIDINDEX32)
        {
            return false;
        }

        // return true the parameter has a value that is not empty
        return (mParameters[paramIndex].mValue.GetIsEmpty() == false);
    }


    // try to find a given parameter's index into the parameter array
    uint32 CommandLine::FindParameterIndex(const char* paramName) const
    {
        // compare all parameter names on a non-case sensitive way
        const uint32 numParams = mParameters.GetLength();
        for (uint32 i = 0; i < numParams; ++i)
        {
            if (mParameters[i].mName.CheckIfIsEqualNoCase(paramName))
            {
                return i;
            }
        }

        // not found
        return MCORE_INVALIDINDEX32;
    }


    // check if we have a parameter with a given name defined
    bool CommandLine::CheckIfHasParameter(const char* paramName) const
    {
        return (FindParameterIndex(paramName) != MCORE_INVALIDINDEX32);
    }


    // extract the next parameter, starting from a given offset
    bool CommandLine::ExtractNextParam(const MCore::String& paramString, MCore::String& outParamName, MCore::String& outParamValue, uint32* inOutStartOffset)
    {
        outParamName.Clear(true);
        outParamValue.Clear(true);

        // check if we already reached the end of the string
        uint32 offset = *inOutStartOffset;
        if (offset >= paramString.GetLength())
        {
            return false;
        }

        // filter out the next parameter
        UnicodeStringIterator iterator(paramString);
        iterator.SetIndex(offset);
        uint32  paramNameStart      = MCORE_INVALIDINDEX32;
        uint32  paramValueStart     = MCORE_INVALIDINDEX32;
        bool    readingParamName    = false;
        bool    readingParamValue   = false;
        bool    foundNextParam      = false;
        bool    insideQuotes        = false;
        bool    prevCharWasSpace    = false;
        int32   bracketDepth        = 0;

        UnicodeCharacter bracketOpen('{');
        UnicodeCharacter bracketClose('}');

        while (foundNextParam == false)
        {
            // check if we reached the end now
            if (offset >= paramString.GetLength())
            {
                *inOutStartOffset = offset;

                // if we were reading a parameter value
                if (readingParamValue)
                {
                    outParamValue.Copy(&paramString.AsChar()[paramValueStart], offset - paramValueStart);
                    readingParamValue = false;
                    outParamValue.TrimRight(UnicodeCharacter::space);
                    outParamValue.RemoveLastCharacterIfEqualTo(bracketClose);
                    outParamValue.RemoveFirstCharacterIfEqualTo(bracketOpen);
                    outParamValue.RemoveLastCharacterIfEqualTo(UnicodeCharacter('\"'));
                    outParamValue.RemoveFirstCharacterIfEqualTo(UnicodeCharacter('\"'));
                }

                return true;
            }

            // get the current character and check if it was a space or not
            UnicodeCharacter curChar = iterator.GetNextCharacter();
            if (offset > 0)
            {
                // get previous character
                iterator.SetIndex(offset);
                UnicodeCharacter prevChar = iterator.GetPreviousCharacter();

                // get the next character
                iterator.SetIndex(offset);
                curChar = iterator.GetNextCharacter();

                //const char prevChar = paramString[offset-1];
                if (prevChar == UnicodeCharacter::space || prevChar == UnicodeCharacter::tab)
                {
                    prevCharWasSpace = true;
                }
                else
                {
                    prevCharWasSpace = false;
                }
            }
            else
            {
                prevCharWasSpace = false;
            }

            // toggle inside quotes flag
            if (curChar == UnicodeCharacter::doubleQuote)
            {
                insideQuotes ^= true;
            }
            else
            if (curChar == bracketOpen)
            {
                bracketDepth++;
            }
            else
            if (curChar == bracketClose)
            {
                bracketDepth--;
            }

            // if its the start of a parameter
            if (curChar == UnicodeCharacter::dash && insideQuotes == false && readingParamValue == false && outParamName.GetLength() == 0 && bracketDepth == 0)
            {
                paramNameStart = offset;
                readingParamName = true;
            }

            // if we found a parameter name
            if ((curChar == UnicodeCharacter::space || curChar == UnicodeCharacter::tab) && readingParamName)
            {
                outParamName.Copy(&paramString.AsChar()[paramNameStart + 1], offset - paramNameStart - 1);
                readingParamName = false;
            }

            // detect the start of the parameter value
            if (readingParamName == false && readingParamValue == false && curChar != UnicodeCharacter::space && curChar != UnicodeCharacter::tab)
            {
                readingParamValue = true;
                paramValueStart = offset;
            }

            // the end of a value
            if (curChar == UnicodeCharacter::dash && insideQuotes == false && readingParamValue && readingParamName == false && paramValueStart != offset && prevCharWasSpace && bracketDepth == 0)
            {
                outParamValue.Copy(&paramString.AsChar()[paramValueStart], offset - paramValueStart);
                readingParamValue = false;
                *inOutStartOffset = offset;
                outParamValue.TrimRight(UnicodeCharacter::space);
                outParamValue.RemoveLastCharacterIfEqualTo(bracketClose);
                outParamValue.RemoveFirstCharacterIfEqualTo(bracketOpen);
                outParamValue.RemoveLastCharacterIfEqualTo(UnicodeCharacter::doubleQuote);
                outParamValue.RemoveFirstCharacterIfEqualTo(UnicodeCharacter::doubleQuote);
                return true;
            }

            // go to the next character
            offset = iterator.GetIndex();
        }

        return false;
    }


    // parse the command line (build the parameter array)
    void CommandLine::SetCommandLine(const char* commandLine)
    {
        SetCommandLine(MCore::String(commandLine));
    }


    // parse the command line (build the parameter array)
    void CommandLine::SetCommandLine(const MCore::String& commandLine)
    {
        // get rid of previous parameters
        mParameters.Clear(true);

        // extract all parameters
        MCore::String paramName;
        MCore::String paramValue;
        uint32 offset = 0;
        while (ExtractNextParam(commandLine, paramName, paramValue, &offset))
        {
            // if the parameter name is empty then it isn't a real parameter
            if (paramName.GetLength() > 0)
            {
                mParameters.AddEmpty();
                mParameters.GetLast().mName     = paramName;
                mParameters.GetLast().mValue    = paramValue;
            }
        }
    }


    // log the command line cotents
    void CommandLine::Log(const char* debugName) const
    {
        const uint32 numParameters = mParameters.GetLength();
        LogInfo("Command line '%s' has %d parameters", debugName, numParameters);
        for (uint32 i = 0; i < numParameters; ++i)
        {
            LogInfo("Param %d (name='%s'  value='%s'", i, mParameters[i].mName.AsChar(), mParameters[i].mValue.AsChar());
        }
    }
}   // namespace MCore
