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

#include "CryLegacy_precompiled.h"
#include "SelectionVariables.h"

#include <StringUtils.h>
#include <DebugDrawContext.h>

#include "BlockyXml.h"


SelectionVariables::SelectionVariables()
    : m_changed(false)
{
}

bool SelectionVariables::GetVariable(const SelectionVariableID& variableID, bool* value) const
{
    Variables::const_iterator it = m_variables.find(variableID);

    if (it != m_variables.end())
    {
        const Variable& variable = it->second;
        *value = variable.value;

        return true;
    }

    return false;
}

bool SelectionVariables::SetVariable(const SelectionVariableID& variableID, bool value)
{
    Variable& variable = stl::map_insert_or_get(m_variables, variableID, Variable());
    if (variable.value != value)
    {
        variable.value = value;
        m_changed = true;

        CTimeValue now = gEnv->pTimer->GetFrameStartTime();

        m_history.push_front(VariableChangeEvent(now, variableID, value));
        while (m_history.size() > 25)
        {
            m_history.pop_back();
        }
    }

    return false;
}

void SelectionVariables::ResetChanged(bool state)
{
    m_changed = state;
}

bool SelectionVariables::Changed() const
{
    return m_changed;
}

void SelectionVariables::Swap(SelectionVariables& other)
{
    std::swap(m_changed, other.m_changed);
    m_variables.swap(other.m_variables);
    m_history.swap(other.m_history);
}

struct DebugVariable
{
    const char* name;
    bool value;

    bool operator <(const DebugVariable& other) const
    {
        return strcmp(name, other.name) < 0;
    }
};

void SelectionVariables::Serialize(TSerialize ser)
{
    ser.Value("m_variables", m_variables);

    if (ser.IsReading())
    {
        m_changed = true; // Force the selection tree to be re-evaluated
    }
}


#if defined(CRYAISYSTEM_DEBUG)


// ===========================================================================
//  Debug: Track incoming signals.
//
//  In:     The name of the signal (NULL will ignore).
void SelectionVariables::DebugTrackSignalHistory(const char* signalName)
{
    static const int historySize = 16;

    if (signalName == NULL)
    {
        return;
    }

    if (m_DebugSignalHistory.size() > historySize)
    {
        m_DebugSignalHistory.pop_back();
    }
    m_DebugSignalHistory.push_front(DebugSignalHistoryEntry(signalName, gEnv->pTimer->GetFrameStartTime()));
}


void SelectionVariables::DebugDraw(bool history, const SelectionVariableDeclarations& declarations) const
{
    CDebugDrawContext dc;

    float minY = 420.0f;
    float maxY = -FLT_MAX;

    float x = 10.0f;
    float y = minY;
    const float fontSize = 1.25f;
    const float lineHeight = 11.5f * fontSize;
    const float columnWidth = 145.0f;

    std::vector<DebugVariable> sorted;

    for (Variables::const_iterator it = m_variables.begin(), end = m_variables.end()
         ; it != end
         ; ++it)
    {
        const SelectionVariableID& variableID = it->first;
        bool value = it->second.value;

        const SelectionVariableDeclarations::VariableDescription& description = declarations.GetVariableDescription(variableID);

        sorted.resize(sorted.size() + 1);
        sorted.back().name = description.name.c_str();
        sorted.back().value = value;
    }

    std::sort(sorted.begin(), sorted.end());

    ColorB trueColor(Col_SlateBlue, 1.0f);
    ColorB falseColor(Col_DarkGray, 1.0f);

    stack_string text;

    if (sorted.empty())
    {
        return;
    }

    uint32 variableCount = sorted.size();
    for (uint32 i = 0; i < variableCount; ++i)
    {
        dc->Draw2dLabel(x, y, fontSize, sorted[i].value ? trueColor : falseColor, false, "%s", sorted[i].name);
        y += lineHeight;

        if (y > maxY)
        {
            maxY = y;
        }

        if (y + lineHeight > 760.0f)
        {
            y = minY;
            x += columnWidth;
        }
    }

    //dc->SetAlphaBlended(false);
    //dc->SetDepthTest(false);

    //dc->Init2DMode();
    //dc->Draw2dImage(x + columnWidth, minY, 1, maxY - minY, -1);

    if (history)
    {
        y = minY;
        CTimeValue now = gEnv->pTimer->GetFrameStartTime();

        History::const_iterator it = m_history.begin();
        History::const_iterator end = m_history.end();

        for (; it != end; ++it)
        {
            const VariableChangeEvent& changeEvent = *it;
            float alpha = 1.0f - (now - changeEvent.when).GetSeconds() / 10.0f;
            if (alpha > 0.01f)
            {
                alpha = clamp_tpl(alpha, 0.33f, 1.0f);
                const SelectionVariableDeclarations::VariableDescription& description =
                    declarations.GetVariableDescription(changeEvent.variableID);

                trueColor.a = (uint8)(alpha * 255.5f);
                falseColor.a = (uint8)(alpha * 255.5f);

                text = description.name;
                dc->Draw2dLabel(x + columnWidth + 2.0f, y, fontSize, changeEvent.value ? trueColor : falseColor, false,
                    "%s", text.c_str());

                y += lineHeight;
            }
        }
    }

    DebugDrawSignalHistory(dc, x, minY - (lineHeight * 2.0f), lineHeight, fontSize);
}


// ===========================================================================
//  Debug: Draw the signal history.
//
//  In,out:     The debug drawing context.
//  In:         The bottom-left X-position where to start drawing at.
//  In:         The bottom-left Y-position where to start drawing at.
//  In:         The line height.
//  In:         The font size.
//
void SelectionVariables::DebugDrawSignalHistory(CDebugDrawContext& dc, float bottomLeftX, float bottomLeftY, float lineHeight, float fontSize) const
{
    const CTimeValue now = gEnv->pTimer->GetFrameStartTime();

    float posY = bottomLeftY;
    ColorF color = Col_Yellow;
    DebugSignalHistory::const_iterator signalIter;
    DebugSignalHistory::const_iterator signalEndIter = m_DebugSignalHistory.end();
    for (signalIter = m_DebugSignalHistory.begin(); signalIter != signalEndIter; ++signalIter)
    {
        posY -= lineHeight;

        float alpha = 1.0f - (now - signalIter->m_TimeStamp).GetSeconds() / 10.0f;
        if (alpha > 0.01f)
        {
            alpha = clamp_tpl(1.0f - alpha, 0.33f, 1.0f);

            color.a = (uint8)(alpha * 255.5f);

            dc->Draw2dLabel(bottomLeftX, posY, fontSize, color, false, "%s", signalIter->m_Name.c_str());
        }
    }
}


SelectionVariables::DebugSignalHistoryEntry::DebugSignalHistoryEntry()
    : m_Name()
    , m_TimeStamp()
{
}


SelectionVariables::DebugSignalHistoryEntry::DebugSignalHistoryEntry(
    const char* signalName,
    const CTimeValue timeStamp)
    : m_Name(signalName)
    , m_TimeStamp(timeStamp)
{
}


#endif // defined(CRYAISYSTEM_DEBUG)


SelectionVariableDeclarations::SelectionVariableDeclarations()
{
}

bool SelectionVariableDeclarations::LoadFromXML(const _smart_ptr<BlockyXmlBlocks>& blocks, const XmlNodeRef& rootNode,
    const char* scopeName, const char* fileName)
{
    assert(scopeName);

    BlockyXmlNodeRef blockyNode(blocks, scopeName, rootNode, fileName);
    while (XmlNodeRef childNode = blockyNode.next())
    {
        if (!_stricmp(childNode->getTag(), "Variable"))
        {
            const char* variableName = 0;
            if (childNode->haveAttr("name"))
            {
                childNode->getAttr("name", &variableName);
            }
            else
            {
                AIWarning("Missing 'name' attribute for tag '%s' in file '%s' at line %d.",
                    childNode->getTag(), fileName, childNode->getLine());

                return false;
            }

            bool defaultValue = false;
            if (childNode->haveAttr("default"))
            {
                const char* value = 0;
                childNode->getAttr("default", &value);

                if (!_stricmp(value, "true"))
                {
                    defaultValue = true;
                }
                else if (!_stricmp(value, "false"))
                {
                    defaultValue = false;
                }
                else
                {
                    AIWarning("Invalid variable value '%s' for variable '%s' in file '%s' at line %d.",
                        value, variableName, fileName, childNode->getLine());

                    return false;
                }
            }

            SelectionVariableID variableID = GetVariableID(variableName);
            std::pair<VariableDescriptions::iterator, bool> iresult = m_descriptions.insert(
                    VariableDescriptions::value_type(variableID, VariableDescription(variableName)));

            if (!iresult.second)
            {
                if (!_stricmp(iresult.first->second.name, variableName))
                {
                    AIWarning("Duplicate variable declaration '%s' in file '%s' at line %d.",
                        variableName, fileName, childNode->getLine());
                }
                else
                {
                    AIWarning("Hash collision for variable declaration '%s' in file '%s' at line %d.",
                        variableName, fileName, childNode->getLine());
                }

                return false;
            }

            m_defaults.SetVariable(variableID, defaultValue);
            m_defaults.ResetChanged(true);
        }
        else
        {
            AIWarning("Unexpected tag '%s' in file '%s' at line %d. 'Variable' expected.",
                childNode->getTag(), fileName, childNode->getLine());

            return false;
        }
    }

    return true;
}

bool SelectionVariableDeclarations::IsDeclared(const SelectionVariableID& variableID) const
{
    return m_descriptions.find(variableID) != m_descriptions.end();
}

SelectionVariableID SelectionVariableDeclarations::GetVariableID(const char* name) const
{
    return CryStringUtils::CalculateHashLowerCase(name);
}

const SelectionVariableDeclarations::VariableDescription& SelectionVariableDeclarations::GetVariableDescription(
    const SelectionVariableID& variableID) const
{
    VariableDescriptions::const_iterator it = m_descriptions.find(variableID);
    if (it != m_descriptions.end())
    {
        const VariableDescription& description = it->second;
        return description;
    }

    static VariableDescription empty("<invalid>");
    return empty;
}

void SelectionVariableDeclarations::GetVariablesNames(
    const char** variableNamesBuffer, const size_t maxSize, size_t& actualSize) const
{
    const size_t totalVariablesAmount = m_descriptions.size();
    if (maxSize < totalVariablesAmount)
    {
        AIWarning("Only %" PRISIZE_T " can be inserted into the buffer while %" PRISIZE_T " are currently present.", (totalVariablesAmount - maxSize), totalVariablesAmount);
    }

    VariableDescriptions::const_iterator it = m_descriptions.begin();
    VariableDescriptions::const_iterator end = m_descriptions.end();
    for (; it != end; ++it)
    {
        if (actualSize < maxSize)
        {
            const VariableDescription& description = it->second;
            variableNamesBuffer[actualSize++] = description.name.c_str();
        }
    }
}

const SelectionVariables& SelectionVariableDeclarations::GetDefaults() const
{
    return m_defaults;
}

const SelectionVariableDeclarations::VariableDescriptions& SelectionVariableDeclarations::GetDescriptions() const
{
    return m_descriptions;
}