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

// Description : Wrapper class for XmlNodeRef which allows to parse different contents
//               from xml depending on game mode SP or MP


#ifndef CRYINCLUDE_CRYACTION_GAMEXMLPARAMREADER_H
#define CRYINCLUDE_CRYACTION_GAMEXMLPARAMREADER_H
#pragma once

class CGameXmlParamReader
{
public:

    explicit CGameXmlParamReader(const XmlNodeRef& xmlNode)
        : m_xmlNode(xmlNode)
    {
        m_gameModeFilter = gEnv->bMultiplayer ? "MP" : "SP";
#if defined(_RELEASE)
        m_devmodeFilter = true;
#else
        m_devmodeFilter = false;
#endif //_RELEASE
    }

    int GetUnfilteredChildCount() const
    {
        if (m_xmlNode)
        {
            return m_xmlNode->getChildCount();
        }
        return 0;
    }

    int GetFilteredChildCount() const
    {
        int filteredChildCount = 0;
        if (m_xmlNode)
        {
            const int childCount = m_xmlNode->getChildCount();
            for (int i = 0; i < childCount; ++i)
            {
                filteredChildCount += !IsNodeFiltered(m_xmlNode->getChild(i));
            }
        }
        return filteredChildCount;
    }

    XmlNodeRef GetFilteredChildAt(int index) const
    {
        if (m_xmlNode)
        {
            XmlNodeRef childNode = m_xmlNode->getChild(index);

            return !IsNodeFiltered(childNode) ? childNode : XmlNodeRef((IXmlNode*)NULL);
        }

        return NULL;
    }

    XmlNodeRef FindFilteredChild(const char* childName) const
    {
        if (m_xmlNode)
        {
            const int childCount = m_xmlNode->getChildCount();
            for (int i = 0; i < childCount; ++i)
            {
                XmlNodeRef childNode = m_xmlNode->getChild(i);

                if (!IsNodeWithTag(childNode, childName) || IsNodeFiltered(childNode))
                {
                    continue;
                }

                return childNode;
            }
        }
        return NULL;
    }

    const char* ReadParamValue(const char* paramName) const
    {
        return ReadParamValue(paramName, "");
    }

    const char* ReadParamValue(const char* paramName, const char* defaultValue) const
    {
        if (m_xmlNode)
        {
            const int childCount = m_xmlNode->getChildCount();
            for (int i = 0; i < childCount; ++i)
            {
                XmlNodeRef childNode = m_xmlNode->getChild(i);

                if (!HasNodeParamAttribute(childNode, paramName) || IsNodeFiltered(childNode))
                {
                    continue;
                }

                return childNode->getAttr("value");
            }
        }
        return defaultValue;
    }

    const char* ReadParamAttributeValue(const char* paramName, const char* attributeName) const
    {
        if (m_xmlNode)
        {
            const int childCount = m_xmlNode->getChildCount();
            for (int i = 0; i < childCount; ++i)
            {
                XmlNodeRef childNode = m_xmlNode->getChild(i);

                if (!HasNodeParamAttribute(childNode, paramName) || IsNodeFiltered(childNode))
                {
                    continue;
                }

                return childNode->getAttr(attributeName);
            }
        }
        return "";
    }

    template<typename T>
    bool ReadParamValue(const char* paramName, T& value) const
    {
        if (m_xmlNode)
        {
            const int childCount = m_xmlNode->getChildCount();
            for (int i = 0; i < childCount; ++i)
            {
                XmlNodeRef childNode = m_xmlNode->getChild(i);

                if (!HasNodeParamAttribute(childNode, paramName) || IsNodeFiltered(childNode))
                {
                    continue;
                }

                return childNode->getAttr("value", value);
            }
        }
        return false;
    }

    template<typename T>
    bool ReadParamValue(const char* paramName, T& value, const T& defaultValue) const
    {
        value = defaultValue;
        return ReadParamValue<T>(paramName, value);
    }

    template<typename T>
    bool ReadParamAttributeValue(const char* paramName, const char* attributeName, T& value) const
    {
        if (m_xmlNode)
        {
            const int childCount = m_xmlNode->getChildCount();
            for (int i = 0; i < childCount; ++i)
            {
                XmlNodeRef childNode = m_xmlNode->getChild(i);

                if (!HasNodeParamAttribute(childNode, paramName) || IsNodeFiltered(childNode))
                {
                    continue;
                }

                return childNode->getAttr(attributeName, value);
            }
        }
        return false;
    }

    template<typename T>
    bool ReadParamAttributeValue(const char* paramName, const char* attributeName, T& value, const T& defaultValue) const
    {
        value = defaultValue;

        return ReadParamAttributeValue<T>(paramName, attributeName, value);
    }

private:

    bool IsNodeFiltered(const XmlNodeRef& xmlNode) const
    {
        CRY_ASSERT (xmlNode != (IXmlNode*)NULL);

        const char* gameAttribute = xmlNode->getAttr("GAME");

        int devmodeFilter = 0;
        xmlNode->getAttr("DEVMODE", devmodeFilter);
        if (devmodeFilter != 0)
        {
            int i = 0;
        }

        const bool devmodeFiltered = devmodeFilter != 0 && m_devmodeFilter;
        const bool gameModeFiltered = (gameAttribute != NULL) && gameAttribute[0] && (strcmp(gameAttribute, m_gameModeFilter.c_str()) != 0);

        return devmodeFiltered || gameModeFiltered;
    }

    bool IsNodeWithTag(const XmlNodeRef& xmlNode, const char* tag) const
    {
        CRY_ASSERT (xmlNode != (IXmlNode*)NULL);

        return (_stricmp(xmlNode->getTag(), tag) == 0);
    }

    bool HasNodeParamAttribute(const XmlNodeRef& xmlNode, const char* paramName) const
    {
        CRY_ASSERT (xmlNode != (IXmlNode*)NULL);

        const char* attributeName = xmlNode->getAttr("name");

        return (attributeName != NULL && (_stricmp(attributeName, paramName) == 0));
    }

    const XmlNodeRef&   m_xmlNode;
    CryFixedStringT<4>  m_gameModeFilter;
    bool m_devmodeFilter;
};

#endif // CRYINCLUDE_CRYACTION_GAMEXMLPARAMREADER_H
