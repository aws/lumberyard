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

// include the required headers
#include "CommandSyntax.h"
#include "LogManager.h"
#include "Algorithms.h"


namespace MCore
{
    // the constructor
    CommandSyntax::CommandSyntax(uint32 numParamsToReserve)
    {
        ReserveParameters(numParamsToReserve);
    }


    // the destructor
    CommandSyntax::~CommandSyntax()
    {
        mParameters.Clear();
    }


    // reserve parameter space
    void CommandSyntax::ReserveParameters(uint32 numParamsToReserve)
    {
        if (numParamsToReserve > 0)
        {
            mParameters.Reserve(numParamsToReserve);
        }
    }


    // add a parameter to the syntax
    void CommandSyntax::AddParameter(const char* name, const char* description, CommandSyntax::EParamType paramType, const char* defaultValue)
    {
        mParameters.AddEmpty();
        CommandSyntax::Parameter& param = mParameters.GetLast();
        param.mDescription  = description;
        param.mName         = name;
        param.mRequired     = false;
        param.mParamType    = paramType;
        param.mDefaultValue = defaultValue;
    }


    // add a parameter to the syntax
    void CommandSyntax::AddRequiredParameter(const char* name, const char* description, CommandSyntax::EParamType paramType)
    {
        mParameters.AddEmpty();
        CommandSyntax::Parameter& param = mParameters.GetLast();
        param.mDescription  = description;
        param.mName     = name;
        param.mRequired = true;
        param.mParamType    = paramType;
    }


    // check if this param is a required one or not
    bool CommandSyntax::GetParamRequired(uint32 index) const
    {
        return mParameters[index].mRequired;
    }


    // get the parameter name
    const char* CommandSyntax::GetParamName(uint32 index) const
    {
        return mParameters[index].mName.AsChar();
    }


    // get the parameter description
    const char* CommandSyntax::GetParamDescription(uint32 index) const
    {
        return mParameters[index].mDescription.AsChar();
    }


    // get the parameter type string
    const char* CommandSyntax::GetParamTypeString(uint32 index) const
    {
        // check the type
        switch (mParameters[index].mParamType)
        {
            case PARAMTYPE_STRING:
                return "String";
                break;
            case PARAMTYPE_BOOLEAN:
                return "Boolean";
                break;
            case PARAMTYPE_CHAR:
                return "Char";
                break;
            case PARAMTYPE_INT:
                return "Int";
                break;
            case PARAMTYPE_FLOAT:
                return "Float";
                break;
            case PARAMTYPE_VECTOR3:
                return "Vector3";
                break;
            case PARAMTYPE_VECTOR4:
                return "Vector4";
                break;
            default: ;
        };

        // an unknown type, which should never really happen
        return "Unknown";
    }


    // get the parameter type
    CommandSyntax::EParamType CommandSyntax::GetParamType(uint32 index) const
    {
        return mParameters[index].mParamType;
    }


    // check if we have a given parameter with a given name in this syntax
    bool CommandSyntax::CheckIfHasParameter(const char* parameter) const
    {
        return (FindParameterIndex(parameter) != MCORE_INVALIDINDEX32);
    }


    // find the parameter index of a given parameter name
    uint32 CommandSyntax::FindParameterIndex(const char* parameter) const
    {
        // try to find the parameter with the given name
        const uint32 numParams = mParameters.GetLength();
        for (uint32 i = 0; i < numParams; ++i)
        {
            if (mParameters[i].mName.CheckIfIsEqualNoCase(parameter))
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // get the default value for a given parameter
    const MCore::String& CommandSyntax::GetDefaultValue(uint32 index) const
    {
        return mParameters[index].mDefaultValue;
    }


    // get the default value for a given parameter name
    bool CommandSyntax::GetDefaultValue(const char* paramName, MCore::String& outDefaultValue) const
    {
        const uint32 index = FindParameterIndex(paramName);
        if (index == MCORE_INVALIDINDEX32)
        {
            return false;
        }
        else
        {
            outDefaultValue = mParameters[index].mDefaultValue;
        }

        return true;
    }


    // Get the default value for a given parameter name.
    bool CommandSyntax::GetDefaultValue(const char* paramName, AZStd::string& outDefaultValue) const
    {
        const uint32 index = FindParameterIndex(paramName);
        if (index == MCORE_INVALIDINDEX32)
        {
            return false;
        }
        else
        {
            outDefaultValue = mParameters[index].mDefaultValue.AsChar();
        }

        return true;
    }


    // check if the provided command parameter string is valid with this syntax
    bool CommandSyntax::CheckIfIsValid(const char* parameterList, String& outResult) const
    {
        return CheckIfIsValid(CommandLine(parameterList), outResult);
    }


    // check if the provided commandline parameter list is valid with this syntax
    bool CommandSyntax::CheckIfIsValid(const CommandLine& commandLine, String& outResult) const
    {
        // clear the outresult string
        outResult.Clear(true);

        // for all parameters in the syntax
        // check if the required ones are specified in the command line
        const uint32 numParams = mParameters.GetLength();
        for (uint32 i = 0; i < numParams; ++i)
        {
            // if the required parameter hasn't been specified
            if (mParameters[i].mRequired && commandLine.CheckIfHasParameter(mParameters[i].mName) == false)
            {
                outResult.FormatAdd("Required parameter '%s' has not been specified.\n", mParameters[i].mName.AsChar());
            }
            else
            {
                // find the parameter index
                const uint32 paramIndex = commandLine.FindParameterIndex(mParameters[i].mName);
                if (paramIndex != MCORE_INVALIDINDEX32)
                {
                    const MCore::String& value = commandLine.GetParameterValue(paramIndex);
                    const MCore::String& paramName = mParameters[i].mName;

                    // if the parameter value has not been specified and it is not a boolean parameter
                    if ((value.GetLength() == 0) && mParameters[i].mParamType != PARAMTYPE_BOOLEAN && mParameters[i].mParamType != PARAMTYPE_STRING)
                    {
                        outResult.FormatAdd("Parameter '%s' has no value specified.\n", paramName.AsChar());
                    }
                    else
                    {
                        // check if we specified a valid int
                        if (mParameters[i].mParamType == PARAMTYPE_INT)
                        {
                            if (value.CheckIfIsValidInt() == false)
                            {
                                outResult.FormatAdd("The value (%s) of integer parameter '%s' is not a valid int.\n", value.AsChar(), paramName.AsChar());
                            }
                        }

                        // check if the specified float is valid
                        if (mParameters[i].mParamType == PARAMTYPE_FLOAT)
                        {
                            if (value.CheckIfIsValidFloat() == false)
                            {
                                outResult.FormatAdd("The value (%s) of float parameter '%s' is not a valid float.\n", value.AsChar(), paramName.AsChar());
                            }
                        }

                        // check if this is a valid boolean
                        if (mParameters[i].mParamType == PARAMTYPE_BOOLEAN)
                        {
                            if (value.GetLength() > 0 && value.CheckIfIsValidBool() == false)
                            {
                                outResult.FormatAdd("The value (%s) of boolean parameter '%s' is not a valid boolean (use 1|0|true|false|yes|no).\n", value.AsChar(), paramName.AsChar());
                            }
                        }

                        // check if this is a valid boolean
                        if (mParameters[i].mParamType == PARAMTYPE_CHAR)
                        {
                            if (value.GetLength() > 1)
                            {
                                outResult.FormatAdd("The value (%s) of character parameter '%s' is not a valid character.\n", value.AsChar(), paramName.AsChar());
                            }
                        }

                        // check if the specified vector3 is valid
                        if (mParameters[i].mParamType == PARAMTYPE_VECTOR3)
                        {
                            if (value.CheckIfIsValidVector3() == false)
                            {
                                outResult.FormatAdd("The value (%s) of Vector3 parameter '%s' is not a valid three component vector.\n", value.AsChar(), paramName.AsChar());
                            }
                        }

                        // check if the specified vector3 is valid
                        if (mParameters[i].mParamType == PARAMTYPE_VECTOR4)
                        {
                            if (value.CheckIfIsValidVector4() == false)
                            {
                                outResult.FormatAdd("The value (%s) of Vector4 parameter '%s' is not a valid four component vector.\n", value.AsChar(), paramName.AsChar());
                            }
                        }
                    }
                } // if (paramIndex != MCORE_INVALIDINDEX32)
            }
        }

        // now add parameters that we specified but that are not defined in the syntax
        const uint32 numCommandLineParams = commandLine.GetNumParameters();
        for (uint32 p = 0; p < numCommandLineParams; ++p)
        {
            if (CheckIfHasParameter(commandLine.GetParameterName(p)) == false)
            {
                MCore::LogWarning("Parameter '%s' is not defined by the command syntax and will be ignored. Use the -help flag to show syntax information.", commandLine.GetParameterName(p).AsChar());
            }
        }

        // return true when there are no errors, or false when there are
        return (outResult.GetLength() == 0);
    }


    // log the syntax
    void CommandSyntax::LogSyntax()
    {
        // find the longest command name
        uint32 offset = 0;
        const uint32 numParams = mParameters.GetLength();
        for (uint32 i = 0; i < numParams; ++i)
        {
            offset = MCore::Max<uint32>(mParameters[i].mName.GetLength(), offset);
        }

        uint32 offset2 = offset;
        uint32 offset3 = offset;

        // log the header
        MCore::String header = "Name";
        offset += 5;
        header.Align(offset, UnicodeCharacter::space);
        header += "Type";
        offset += 15;
        header.Align(offset, UnicodeCharacter::space);
        header += "Required";
        offset += 10;
        header.Align(offset, UnicodeCharacter::space);
        header += "Default Value";
        offset += 20;
        header.Align(offset, UnicodeCharacter::space);
        header += "Description";
        MCore::LogInfo(header);
        MCore::LogInfo("--------------------------------------------------------------------------------------------------");

        // log all parameters
        MCore::String final;
        for (uint32 i = 0; i < numParams; ++i)
        {
            offset2 = offset3;
            final = mParameters[i].mName;
            offset2 += 5;
            final.Align(offset2, UnicodeCharacter::space);
            final += GetParamTypeString(i);
            offset2 += 15;
            final.Align(offset2, UnicodeCharacter::space);
            final += mParameters[i].mRequired ? "Yes" : "No";
            offset2 += 10;
            final.Align(offset2, UnicodeCharacter::space);
            final += mParameters[i].mDefaultValue;
            offset2 += 20;
            final.Align(offset2, UnicodeCharacter::space);
            final += mParameters[i].mDescription;

            MCore::LogInfo(final);
        }
    }
}   // namespace MCore
