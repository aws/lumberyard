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

#ifndef CRYINCLUDE_CRYAISYSTEM_SELECTIONTREE_SELECTIONVARIABLES_H
#define CRYINCLUDE_CRYAISYSTEM_SELECTIONTREE_SELECTIONVARIABLES_H
#pragma once

/*
    This file implements variable value and variable declaration collections.
*/

class CDebugDrawContext;

typedef uint32 SelectionVariableID;

class BlockyXmlBlocks;
class SelectionVariableDeclarations;

class SelectionVariables
{
public:
    SelectionVariables();

    bool GetVariable(const SelectionVariableID& variableID, bool* value) const;
    bool SetVariable(const SelectionVariableID& variableID, bool value);

    void ResetChanged(bool state = false);
    bool Changed() const;

    void Swap(SelectionVariables& other);

    void Serialize(TSerialize ser);

    size_t GetVariablesAmount() const { return m_variables.size(); }

#if defined(CRYAISYSTEM_DEBUG)

    void DebugTrackSignalHistory(const char* signalName);

    void DebugDraw(bool history, const SelectionVariableDeclarations& declarations) const;

#endif


private:

#if defined(CRYAISYSTEM_DEBUG)

    void DebugDrawSignalHistory(CDebugDrawContext& dc, float bottomLeftX, float bottomLeftY, float lineHeight, float fontSize) const;

#endif


private:
    struct Variable
    {
        Variable()
            : value(false)
        {
        }

        bool value;

        void Serialize(TSerialize ser)
        {
            ser.Value("value", value);
        }
    };

    struct variable_hash_traits
        : public stl::hash_uint32
    {
        enum
        {
            bucket_size = 1,
            min_buckets = 16
        };
    };

    typedef AZStd::unordered_map<SelectionVariableID, Variable, variable_hash_traits> Variables;
    Variables m_variables;

    bool m_changed;

    struct VariableChangeEvent
    {
        VariableChangeEvent(const CTimeValue& _when, const SelectionVariableID _variableID, bool _value)
            : when(_when)
            , variableID(_variableID)
            , value(_value)
        {
        }

        CTimeValue when;
        SelectionVariableID variableID;
        bool value;
    };

    typedef std::deque<VariableChangeEvent> History;
    History m_history;

#if defined(CRYAISYSTEM_DEBUG)

    // Debug: we track the last few received signals here.
    struct DebugSignalHistoryEntry
    {
        DebugSignalHistoryEntry();
        DebugSignalHistoryEntry(const char* signalName, const CTimeValue timeStamp);


        // The name of the signal.
        string m_Name;

        // The time index at which the signal was delivered.
        CTimeValue m_TimeStamp;
    };

    typedef std::list<DebugSignalHistoryEntry> DebugSignalHistory;
    DebugSignalHistory m_DebugSignalHistory;

#endif
};


class SelectionVariableDeclarations
{
public:
    struct VariableDescription
    {
        VariableDescription(const char* _name)
            : name(_name)
        {
        }

        string name;
    };
    typedef std::map<SelectionVariableID, VariableDescription> VariableDescriptions;

    SelectionVariableDeclarations();
    bool LoadFromXML(const _smart_ptr<BlockyXmlBlocks>& blocks, const XmlNodeRef& rootNode, const char* scopeName, const char* fileName);

    bool IsDeclared(const SelectionVariableID& variableID) const;

    SelectionVariableID GetVariableID(const char* name) const;
    const VariableDescription& GetVariableDescription(const SelectionVariableID& variableID) const;
    void GetVariablesNames(const char** variableNamesBuffer, const size_t maxSize, size_t& actualSize) const;

    const SelectionVariables& GetDefaults() const;
    const VariableDescriptions& GetDescriptions() const;

private:
    VariableDescriptions m_descriptions;
    SelectionVariables m_defaults;
};

#endif // CRYINCLUDE_CRYAISYSTEM_SELECTIONTREE_SELECTIONVARIABLES_H
