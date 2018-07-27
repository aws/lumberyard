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
#include <CSVStaticData.h>

#include <AzCore/std/smart_ptr/make_shared.h>

#include <sstream>

namespace CloudCanvas
{
    namespace StaticData
    {
        CSVStaticData::CSVStaticData()
        {
        }

        bool CSVStaticData::LoadData(const char* initBuffer)
        {
            m_dataVec.clear();
            m_attributes.clear();

            std::stringstream readStream(initBuffer);

            // The first row should be all of our attribute names
            std::string attributeStr;
            std::getline(readStream, attributeStr);

            // Strip off the trailing \r if it is present
            if (attributeStr.length() && attributeStr[attributeStr.length() - 1] == '\r')
            {
                attributeStr.erase(attributeStr.length() - 1);
            }

            std::stringstream attributeStream(attributeStr);
            AttributeValueType thisAttribute;
            do
            {
                thisAttribute = ParseFromStream(attributeStream);
                // We'll just assume that our first column is our key column
                if (!m_attributes.size())
                {
                    m_keyName = thisAttribute.c_str();
                }
                m_attributes.push_back(thisAttribute.c_str());
            } while (thisAttribute.length());

            // Attributes set, now real reading
            while (std::getline(readStream, attributeStr))
            {
                if (attributeStr.length() && attributeStr[attributeStr.length() - 1] == '\r')
                {
                    attributeStr.erase(attributeStr.length() - 1);
                }

                std::stringstream entryStream(attributeStr);

                AttributeMapPtr newMap = AZStd::make_shared<AttributeMap>();

                size_t thisAttrSlot = 0;
                do
                {
                    thisAttribute = ParseFromStream(entryStream);

                    if (thisAttrSlot < m_attributes.size())
                    {
                        newMap->insert({ m_attributes[thisAttrSlot], thisAttribute.c_str() });
                    }
                    ++thisAttrSlot;
                } while (thisAttribute.length());

                m_dataVec.push_back(newMap);
            }
            return true;
        }

        AttributeValueType CSVStaticData::ParseFromStream(std::stringstream& inStream)
        {
            std::string returnString;
            size_t quoteCount = 0;
            bool trimQuotes = false;

            for (std::string readString; std::getline(inStream, readString, ','); returnString += ',')
            {
                returnString += readString;
                quoteCount += std::count(readString.begin(), readString.end(), '"');

                // If there are any quotes there will be a leading/trailing quote pair that we want to trim
                if (quoteCount)
                {
                    trimQuotes = true;
                }

                //In this case we've read through everything (There are no more embedded commas)
                if ((quoteCount % 2) == 0)
                {
                    break;
                }
            }

            if (trimQuotes)
            {
                returnString.erase(0, 1);
                returnString.erase(returnString.length() - 1, 1);
            }

            // Now find if we've got any double quotes that need to be turned back into single quotes
            const std::string doubleQuotes = "\"\"";
            std::string::size_type findPos = 0;
            while ((findPos = returnString.find(doubleQuotes, findPos)) != std::string::npos)
            {
                returnString.erase(findPos, 1);
                findPos += doubleQuotes.length() - 1;
            }
            return returnString.c_str();
        }
        AttributeMapPtr CSVStaticData::GetAttributeMap(const char* keyName) const
        {
            for (auto thisMap : m_dataVec)
            {
                auto thisElement = thisMap->find(m_keyName);
                if (thisElement != thisMap->end())
                {
                    if (thisElement->second == keyName)
                    {
                        return thisMap;
                    }
                }
            }
            return AttributeMapPtr {};
        }

        // Find the attribute value for the given key and attribute combination
        AttributeReturnType CSVStaticData::GetAttributeValue(const char* keyName, const char* attributeName) const
        {
            AttributeMapPtr mapPtr = GetAttributeMap(keyName);
            if (mapPtr)
            {
                auto thisField = mapPtr->find(attributeName);
                if (thisField != mapPtr->end())
                {
                    return AZStd::make_pair(true, thisField->second);
                }
            }
            return AZStd::make_pair(false, "");
        }

        ReturnInt CSVStaticData::GetIntValue(const char* structName, const char* fieldName, bool& wasSuccess) const
        {
            AttributeReturnType searchValue = GetAttributeValue(structName, fieldName);
            if (searchValue.first)
            {
                std::stringstream numStream(searchValue.second.c_str());
                int returnValue;
                wasSuccess = true;
                numStream >> returnValue;
                return returnValue;
            }
            wasSuccess = false;
            return 0;
        }

        ReturnDouble CSVStaticData::GetDoubleValue(const char* structName, const char* fieldName, bool& wasSuccess) const
        {
            AttributeReturnType searchValue = GetAttributeValue(structName, fieldName);
            if (searchValue.first)
            {
                std::stringstream numStream(searchValue.second.c_str());
                double returnValue;
                numStream >> returnValue;
                wasSuccess = true;
                return returnValue;
            }
            wasSuccess = false;
            return 0.0;
        }

        ReturnStr CSVStaticData::GetStrValue(const char* structName, const char* fieldName, bool& wasSuccess) const
        {
            AttributeReturnType searchValue = GetAttributeValue(structName, fieldName);
            if (searchValue.first)
            {
                AZStd::string returnStr;
                returnStr = searchValue.second;
                wasSuccess = true;
                return returnStr;
            }
            wasSuccess = false;
            return "";
        }
    }
}