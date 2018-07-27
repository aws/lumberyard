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
#ifndef CRYINCLUDE_CRYCOMMON_ARGUMENTPARSER_H
#define CRYINCLUDE_CRYCOMMON_ARGUMENTPARSER_H
#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// UI variant data /////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

typedef boost::mpl::vector<
    int,
    float,
    EntityId,
    Vec3,
    string,
    wstring,
    bool
    > TUIDataTypes;

//  Default conversion uses C++ rules.
template <class From, class To>
struct SUIConversion
{
    static ILINE bool ConvertValue(const From& from, To& to)
    {
        to = (To)from;
        return true;
    }
};

//  same type conversation
#define UIDATA_NO_CONVERSION(T)                                                          \
    template <>                                                                          \
    struct SUIConversion<T, T> {                                                         \
        static ILINE bool ConvertValue(const T&from, T & to) { to = from; return true; } \
    }
UIDATA_NO_CONVERSION(int);
UIDATA_NO_CONVERSION(float);
UIDATA_NO_CONVERSION(EntityId);
UIDATA_NO_CONVERSION(Vec3);
UIDATA_NO_CONVERSION(string);
UIDATA_NO_CONVERSION(wstring);
UIDATA_NO_CONVERSION(bool);
#undef FLOWSYSTEM_NO_CONVERSION

//  Specialization for converting to bool to avoid compiler warnings.
template <class From>
struct SUIConversion<From, bool>
{
    static ILINE bool ConvertValue(const From& from, bool& to)
    {
        to = (from != 0);
        return true;
    }
};

//  Strict conversation from float to int
template <>
struct SUIConversion<float, int>
{
    static ILINE bool ConvertValue(const float& from, int& to)
    {
        int tmp = (int) from;
        if (fabs(from - (float) tmp) < FLT_EPSILON)
        {
            to = tmp;
            return true;
        }
        return false;
    }
};

//  Vec3 conversions...
template <class To>
struct SUIConversion<Vec3, To>
{
    static ILINE bool ConvertValue(const Vec3& from, To& to)
    {
        return SUIConversion<float, To>::ConvertValue(from.x, to);
    }
};

template <class From>
struct SUIConversion<From, Vec3>
{
    static ILINE bool ConvertValue(const From& from, Vec3& to)
    {
        float temp;
        if (!SUIConversion<From, float>::ConvertValue(from, temp))
        {
            return false;
        }
        to.x = to.y = to.z = temp;
        return true;
    }
};

template <>
struct SUIConversion<Vec3, bool>
{
    static ILINE bool ConvertValue(const Vec3& from, bool& to)
    {
        to = from.GetLengthSquared() > 0;
        return true;
    }
};

//  String conversions...
#define UIDATA_STRING_CONVERSION(strtype, type, fmt, fct)                         \
    template <>                                                                   \
    struct SUIConversion<type, CryStringT<strtype> >                              \
    {                                                                             \
        static ILINE bool ConvertValue(const type&from, CryStringT<strtype>&to)   \
        {                                                                         \
            to.Format(fmt, from);                                                 \
            return true;                                                          \
        }                                                                         \
    };                                                                            \
    template <>                                                                   \
    struct SUIConversion<CryStringT<strtype>, type>                               \
    {                                                                             \
        static ILINE bool ConvertValue(const CryStringT<strtype>&from, type & to) \
        {                                                                         \
            strtype* pEnd;                                                        \
            to = fct;                                                             \
            return from.size() > 0 && *pEnd == '\0';                              \
        }                                                                         \
    };

#define SINGLE_FCT(fct) (float) fct (from.c_str(), &pEnd)
#define DOUBLE_FCT(fct) fct (from.c_str(), &pEnd, 10)

UIDATA_STRING_CONVERSION(char, int, "%d", DOUBLE_FCT(strtol));
UIDATA_STRING_CONVERSION(char, float, "%f", SINGLE_FCT(strtod));
UIDATA_STRING_CONVERSION(char, EntityId, "%u", DOUBLE_FCT(strtoul));

UIDATA_STRING_CONVERSION(wchar_t, int, L"%d", DOUBLE_FCT(wcstol));
UIDATA_STRING_CONVERSION(wchar_t, float, L"%f", SINGLE_FCT(wcstod));
UIDATA_STRING_CONVERSION(wchar_t, EntityId, L"%u", DOUBLE_FCT(wcstoul));

#undef UIDATA_STRING_CONVERSION
#undef SINGLE_FCT
#undef DOUBLE_FCT

template <>
struct SUIConversion<bool, string>
{
    static ILINE bool ConvertValue(const bool& from, string& to)
    {
        to.Format("%d", from);
        return true;
    }
};

template <>
struct SUIConversion<string, bool>
{
    static ILINE bool ConvertValue(const string& from, bool& to)
    {
        float to_i;
        if (SUIConversion<string, float>::ConvertValue(from, to_i))
        {
            to = to_i != 0;
            return true;
        }
        if (0 == azstricmp(from.c_str(), "true"))
        {
            to = true;
            return true;
        }
        if (0 == azstricmp(from.c_str(), "false"))
        {
            to = false;
            return true;
        }
        return false;
    }
};

template <>
struct SUIConversion<Vec3, string>
{
    static ILINE bool ConvertValue(const Vec3& from, string& to)
    {
        to.Format("%f,%f,%f", from.x, from.y, from.z);
        return true;
    }
};

template <>
struct SUIConversion<string, Vec3>
{
    static ILINE bool ConvertValue(const string& from, Vec3& to)
    {
        return 3 == azsscanf(from.c_str(), "%f,%f,%f", &to.x, &to.y, &to.z);
    }
};

template <>
struct SUIConversion<bool, wstring>
{
    static ILINE bool ConvertValue(const bool& from, wstring& to)
    {
        to.Format(L"%d", from);
        return true;
    }
};

template <>
struct SUIConversion<wstring, bool>
{
    static ILINE bool ConvertValue(const wstring& from, bool& to)
    {
        int to_i;
        if (SUIConversion<wstring, int>::ConvertValue(from, to_i))
        {
            to = !!to_i;
            return true;
        }
        if (0 == azwcsicmp (from.c_str(), L"true"))
        {
            to = true;
            return true;
        }
        if (0 == azwcsicmp (from.c_str(), L"false"))
        {
            to = false;
            return true;
        }
        return false;
    }
};

template <>
struct SUIConversion<Vec3, wstring>
{
    static ILINE bool ConvertValue(const Vec3& from, wstring& to)
    {
        to.Format(L"%f,%f,%f", from.x, from.y, from.z);
        return true;
    }
};

template <>
struct SUIConversion<wstring, Vec3>
{
    static ILINE bool ConvertValue(const wstring& from, Vec3& to)
    {
        return 3 == azswscanf(from.c_str(), L"%f,%f,%f", &to.x, &to.y, &to.z);
    }
};

template <>
struct SUIConversion<string, wstring>
{
    static ILINE bool ConvertValue(const string& from, wstring& to)
    {
        Unicode::Convert(to, from);
        return true;
    }
};

template <>
struct SUIConversion<wstring, string>
{
    static ILINE bool ConvertValue(const wstring& from, string& to)
    {
        Unicode::Convert(to, from);
        return true;
    }
};


enum EUIDataTypes
{
    eUIDT_Any = -1,
    eUIDT_Bool =            boost::mpl::find<TUIDataTypes, bool>::type::pos::value,
    eUIDT_Int =             boost::mpl::find<TUIDataTypes, int>::type::pos::value,
    eUIDT_Float =           boost::mpl::find<TUIDataTypes, float>::type::pos::value,
    eUIDT_EntityId =    boost::mpl::find<TUIDataTypes, EntityId>::type::pos::value,
    eUIDT_Vec3 =            boost::mpl::find<TUIDataTypes, Vec3>::type::pos::value,
    eUIDT_String =      boost::mpl::find<TUIDataTypes, string>::type::pos::value,
    eUIDT_WString =     boost::mpl::find<TUIDataTypes, wstring>::type::pos::value
};

typedef boost::make_variant_over<TUIDataTypes>::type TUIDataVariant;

class TUIData
{
    class ExtractType
        : public boost::static_visitor<EUIDataTypes>
    {
    public:
        template <typename T>
        EUIDataTypes operator()(const T& value) const
        {
            return (EUIDataTypes) boost::mpl::find<TUIDataTypes, T>::type::pos::value;
        }
    };

    template <typename To>
    class ConvertType_Get
        : public boost::static_visitor<bool>
    {
    public:
        ConvertType_Get(To& to_)
            : to(to_) {}

        template <typename From>
        bool operator()(const From& from) const
        {
            return SUIConversion<From, To>::ConvertValue(from, to);
        }

        To& to;
    };

public:
    TUIData()
        : m_variant()
    {}

    TUIData(const TUIData& rhs)
        : m_variant(rhs.m_variant)
    {}

    template <typename T>
    explicit TUIData(const T& v, bool locked = false)
        : m_variant(v)
    {}

    TUIData& operator=(const TUIData& rhs)
    {
        m_variant = rhs.m_variant;
        return *this;
    }

    template<typename T>
    bool Set(const T& value)
    {
        m_variant = value;
        return true;
    }

    template <typename T>
    bool GetValueWithConversion(T& value) const
    {
        return boost::apply_visitor(ConvertType_Get<T>(value), m_variant);
    }

    EUIDataTypes GetType() const { return boost::apply_visitor(ExtractType(), m_variant); }

    template<typename T>
    T* GetPtr() { return boost::get<T>(&m_variant); }
    template<typename T>
    const T* GetPtr() const { return boost::get<const T>(&m_variant); }

private:
    TUIDataVariant m_variant;
};


////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////// UI Arguments //////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
#define UIARGS_DEFAULT_DELIMITER "|"

// Summary:
//   Variant type to pass values to flash variables
struct SFlashVarValue
{
    union Data
    {
        bool b;
        int i;
        unsigned int ui;
        double d;
        float f;
        const char* pStr;
        const wchar_t* pWstr;
    };

    // Summary:
    //  Enumerates types that can be sent to and received from flash
    enum Type
    {
        eUndefined,
        eNull,

        eBool,
        eInt,
        eUInt,
        eDouble,
        eFloat,
        eConstStrPtr,
        eConstWstrPtr,

        eObject // receive only!
    };

    SFlashVarValue(bool val)
        : type(eBool)
    {
        data.b = val;
    }
    SFlashVarValue(int val)
        : type(eInt)
    {
        data.i = val;
    }
    SFlashVarValue(unsigned int val)
        : type(eUInt)
    {
        data.ui = val;
    }
    SFlashVarValue(double val)
        : type(eDouble)
    {
        data.d = val;
    }
    SFlashVarValue(float val)
        : type(eFloat)
    {
        data.f = val;
    }
    SFlashVarValue(const char* val)
        : type(eConstStrPtr)
    {
        data.pStr = val;
    }
    SFlashVarValue(const wchar_t* val)
        : type(eConstWstrPtr)
    {
        data.pWstr = val;
    }
    static SFlashVarValue CreateUndefined()
    {
        return SFlashVarValue(eUndefined);
    }
    static SFlashVarValue CreateNull()
    {
        return SFlashVarValue(eNull);
    }

    bool GetBool() const
    {
        assert(type == eBool);
        return data.b;
    }
    int GetInt() const
    {
        assert(type == eInt);
        return data.i;
    }
    int GetUInt() const
    {
        assert(type == eUInt);
        return data.ui;
    }
    double GetDouble() const
    {
        assert(type == eDouble);
        return data.d;
    }
    float GetFloat() const
    {
        assert(type == eFloat);
        return data.f;
    }
    const char* GetConstStrPtr() const
    {
        assert(type == eConstStrPtr);
        return data.pStr;
    }
    const wchar_t* GetConstWstrPtr() const
    {
        assert(type == eConstWstrPtr);
        return data.pWstr;
    }

    Type GetType() const
    {
        return type;
    }
    bool IsUndefined() const
    {
        return GetType() == eUndefined;
    }
    bool IsNull() const
    {
        return GetType() == eNull;
    }
    bool IsBool() const
    {
        return GetType() == eBool;
    }
    bool IsInt() const
    {
        return GetType() == eInt;
    }
    bool IsUInt() const
    {
        return GetType() == eUInt;
    }
    bool IsDouble() const
    {
        return GetType() == eDouble;
    }
    bool IsFloat() const
    {
        return GetType() == eFloat;
    }
    bool IsConstStr() const
    {
        return GetType() == eConstStrPtr;
    }
    bool IsConstWstr() const
    {
        return GetType() == eConstWstrPtr;
    }
    bool IsObject() const
    {
        return GetType() == eObject;
    }

protected:
    Type type;
    Data data;

protected:
    // Notes:
    //   Don't define default constructor to enforce efficient default initialization of argument lists!
    SFlashVarValue();

    SFlashVarValue(Type t)
        : type(t)
    {
        //data.d = 0;

        //static const Data init = {0};
        //data = init;

        memset(&data, 0, sizeof(data));
    }
};

template<class T>
struct SUIParamTypeHelper
{
    static EUIDataTypes GetType(const T&)
    {
        return eUIDT_Any;
    }
};
template<>
struct SUIParamTypeHelper<bool>
{
    static EUIDataTypes GetType(const bool&)
    {
        return eUIDT_Bool;
    }
};
template<>
struct SUIParamTypeHelper<int>
{
    static EUIDataTypes GetType(const int&)
    {
        return eUIDT_Int;
    }
};
template<>
struct SUIParamTypeHelper<EntityId>
{
    static EUIDataTypes GetType(const EntityId&)
    {
        return eUIDT_EntityId;
    }
};
template<>
struct SUIParamTypeHelper<float>
{
    static EUIDataTypes GetType(const float&)
    {
        return eUIDT_Float;
    }
};
template<>
struct SUIParamTypeHelper<Vec3>
{
    static EUIDataTypes GetType(const Vec3&)
    {
        return eUIDT_Vec3;
    }
};
template<>
struct SUIParamTypeHelper<string>
{
    static EUIDataTypes GetType(const string&)
    {
        return eUIDT_String;
    }
};
template<>
struct SUIParamTypeHelper<wstring>
{
    static EUIDataTypes GetType(const wstring&)
    {
        return eUIDT_WString;
    }
};
template<>
struct SUIParamTypeHelper<TUIData>
{
    static EUIDataTypes GetType(const TUIData& d)
    {
        return (EUIDataTypes) d.GetType();
    }
};

struct SArgumentParser
{
    SArgumentParser()
        : m_cDelimiter(UIARGS_DEFAULT_DELIMITER)
        , m_Dirty(eBDF_Delimiter) {};
    template <class T>
    SArgumentParser(const T* argStringList, bool bufferStr = false)
        : m_cDelimiter(UIARGS_DEFAULT_DELIMITER) { SetArguments(argStringList, bufferStr); }
    SArgumentParser(const SFlashVarValue* vArgs, int iNumArgs)
        : m_cDelimiter(UIARGS_DEFAULT_DELIMITER) { SetArguments(vArgs, iNumArgs); }
    SArgumentParser(const TUIData& data)
        : m_cDelimiter(UIARGS_DEFAULT_DELIMITER) { AddArgument(data); }

    template <class T>
    void SetArguments(const T* argStringList, bool bufferStr = false)
    {
        Clear();
        AddArguments(argStringList, bufferStr);
    }


    template <class T>
    void AddArguments(const T* argStringList, bool bufferStr = false)
    {
        const CryStringT<T>& delimiter = GetDelimiter<T>();
        const T* delimiter_str = delimiter.c_str();
        const int delimiter_len = delimiter.length();
        const size_t s = StrLenTmpl(argStringList) + 1;
        T* buffer = new T[s];
        memcpy(buffer, argStringList, s * sizeof(T));
        T* found = buffer;
        while (*found)
        {
            T* next = StrStrTmpl(found, delimiter_str);
            if (next)
            {
                next[0] = 0;
                AddArgument(CryStringT<T>(found), eUIDT_Any);
                next[0] = delimiter_str[0];
                found = next + delimiter_len;
            }
            if (!next || (next && !*found))
            {
                AddArgument(CryStringT<T>(found), eUIDT_Any);
                break;
            }
        }
        if (bufferStr)
        {
            setStringBuffer(argStringList);
        }
        delete[] buffer;
    }

    void SetArguments(const SFlashVarValue* vArgs, int iNumArgs)
    {
        Clear();
        AddArguments(vArgs, iNumArgs);
    }

    void AddArguments(const SFlashVarValue* vArgs, int iNumArgs)
    {
        m_ArgList.reserve(m_ArgList.size() + iNumArgs);
        for (int i = 0; i < iNumArgs; ++i)
        {
            switch (vArgs[i].GetType())
            {
            case SFlashVarValue::eBool:
                AddArgument(vArgs[i].GetBool());
                break;
            case SFlashVarValue::eInt:
                AddArgument(vArgs[i].GetInt());
                break;
            case SFlashVarValue::eUInt:
                AddArgument(vArgs[i].GetUInt());
                break;
            case SFlashVarValue::eFloat:
                AddArgument(vArgs[i].GetFloat());
                break;
            case SFlashVarValue::eDouble:
                AddArgument((float) vArgs[i].GetDouble());
                break;
            case SFlashVarValue::eConstStrPtr:
                AddArgument(vArgs[i].GetConstStrPtr());
                break;
            case SFlashVarValue::eConstWstrPtr:
                AddArgument(vArgs[i].GetConstWstrPtr());
                break;
            case SFlashVarValue::eNull:
                AddArgument("NULL");
                break;
            case SFlashVarValue::eObject:
                AddArgument("OBJECT");
                break;
            case SFlashVarValue::eUndefined:
                AddArgument("UNDEFINED");
                break;
            }
        }
    }

    void AddArguments(const SArgumentParser& args)
    {
        const int iNumArgs = args.GetArgCount();
        m_ArgList.reserve(m_ArgList.size() + iNumArgs);
        for (int i = 0; i < iNumArgs; ++i)
        {
            AddArgument(args.GetArg(i), args.GetArgType(i));
        }
    }

    template <class T>
    inline void AddArgument(const T& arg, EUIDataTypes type)
    {
        m_ArgList.push_back(SUIData(type, TUIData(arg)));
        m_Dirty = eBDF_ALL;
    }

    template <class T>
    inline void AddArgument(const T& arg)
    {
        AddArgument(arg, SUIParamTypeHelper<T>::GetType(arg));
    }

    template <class T>
    inline void AddArgument(const T* str)
    {
        AddArgument(CryStringT<T>(str));
    }


    inline void Clear()
    {
        m_ArgList.clear();
        m_Dirty = eBDF_ALL;
    }

    template <class T>
    static SArgumentParser Create(const T& arg)
    {
        SArgumentParser args;
        args.AddArgument(arg);
        return args;
    }

    template <class T>
    static SArgumentParser Create(const T* str)
    {
        SArgumentParser args;
        args.AddArgument(str);
        return args;
    }

    inline int GetArgCount() const { return m_ArgList.size(); }

    const char* GetAsString() const { return updateStringBuffer(m_sArgStringBuffer, eBDF_String); }
    const wchar_t* GetAsWString() const { return updateStringBuffer(m_sArgWStringBuffer, eBDF_WString); }
    const SFlashVarValue* GetAsList() const { return updateFlashBuffer(); }

    inline const TUIData& GetArg(int index) const
    {
        assert(index >= 0 && index < m_ArgList.size());
        return m_ArgList[index].Data;
    }

    inline EUIDataTypes GetArgType(int index) const
    {
        assert(index >= 0 && index < m_ArgList.size());
        return m_ArgList[index].Type;
    }

    template < class T >
    inline bool GetArg(int index, T& val) const
    {
        if (index >= 0 && index < m_ArgList.size())
        {
            return m_ArgList[index].Data.GetValueWithConversion(val);
        }
        return false;
    }

    template < class T >
    inline void GetArgNoConversation(int index, T& val) const
    {
        assert(index >= 0 && index < m_ArgList.size());
        const T* valPtr = m_ArgList[index].Data.GetPtr<T>();
        assert(valPtr);
        val = *valPtr;
    }

    inline void SetDelimiter(const string& delimiter)
    {
        if (delimiter != m_cDelimiter)
        {
            m_Dirty |= eBDF_String | eBDF_WString | eBDF_Delimiter;
        }
        m_cDelimiter = delimiter;
    }

    template < class T >
    inline T* StrStrTmpl(T* str1, const T* str2)
    {
        return strstr(str1, str2);
    }

    template < class T >
    inline size_t StrLenTmpl(const T* str)
    {
        return strlen(str);
    }

private:
    struct SUIData
    {
        SUIData(EUIDataTypes type, const TUIData& data)
            : Type(type)
            , Data(data) {}
        EUIDataTypes Type;
        TUIData Data;
    };
    DynArray< SUIData > m_ArgList;
    mutable string m_sArgStringBuffer;  // buffer for const char* GetAsString()
    mutable wstring m_sArgWStringBuffer;    // buffer for const wchar_t* GetAsWString()
    mutable DynArray< SFlashVarValue > m_FlashValueBuffer; // buffer for const SFlashVarValue* GetAsList()
    string m_cDelimiter;

    enum EBufferDirtyFlag
    {
        eBDF_None      = 0x00,
        eBDF_String    = 0x01,
        eBDF_WString   = 0x02,
        eBDF_FlashVar  = 0x04,
        eBDF_Delimiter = 0x08,
        eBDF_ALL       = 0xFF,
    };
    mutable uint m_Dirty;

    inline SFlashVarValue* updateFlashBuffer() const
    {
        if (m_Dirty & eBDF_FlashVar)
        {
            m_Dirty &= ~eBDF_FlashVar;
            m_FlashValueBuffer.clear();
            for (DynArray< SUIData >::const_iterator it = m_ArgList.begin(); it != m_ArgList.end(); ++it)
            {
                bool bConverted = false;
                switch (it->Type)
                {
                case eUIDT_Bool:
                    AddValue<bool>(it->Data);
                    break;
                case eUIDT_Int:
                    AddValue<int>(it->Data);
                    break;
                case eUIDT_EntityId:
                    AddValue<EntityId>(it->Data);
                    break;
                case eUIDT_Float:
                    AddValue<float>(it->Data);
                    break;
                case eUIDT_String:
                    AddValue<string>(it->Data);
                    break;
                case eUIDT_WString:
                    AddValue<wstring>(it->Data);
                    break;
                case eUIDT_Any:
                {
                    bool bRes = TryAddValue<int>(it->Data)
                        || TryAddValue<float>(it->Data)
                        || (it->Data.GetType() == eUIDT_String && AddValue<string>(it->Data))
                        || (it->Data.GetType() == eUIDT_WString && AddValue<wstring>(it->Data));
                    assert(bRes);
                }
                break;
                default:
                    assert(false);
                    break;
                }
            }
        }
        return m_FlashValueBuffer.size() > 0 ? &m_FlashValueBuffer[0] : NULL;
    }

    template < class T >
    inline bool AddValue(const TUIData& data) const
    {
        const T* val = data.GetPtr<T>();
        assert(val);
        m_FlashValueBuffer.push_back(SFlashVarValue(*val));
        return true;
    }

    template < class T >
    inline bool TryAddValue(const TUIData& data) const
    {
        T val;
        if (data.GetValueWithConversion(val))
        {
            m_FlashValueBuffer.push_back(SFlashVarValue(val));
            return true;
        }
        return false;
    }

    template <class T>
    inline const T* updateStringBuffer(CryStringT<T>& buffer, uint flag) const
    {
        if (m_Dirty & flag)
        {
            m_Dirty &= ~flag;
            CryStringT<T> delimiter_str = GetDelimiter<T>();
            buffer.clear();
            for (DynArray< SUIData >::const_iterator it = m_ArgList.begin(); it != m_ArgList.end(); ++it)
            {
                if (buffer.size() > 0)
                {
                    buffer += delimiter_str;
                }
                CryStringT<T> val;
                bool bRes = it->Data.GetValueWithConversion(val);
                assert(bRes && "try to convert to char* string list but some of the values are unsupported wchar_t*");
                buffer += val;
            }
        }
        return buffer.c_str();
    }

    template <class T>
    inline const CryStringT<T>& GetDelimiter() const
    {
        static CryStringT<T> delimiter_str;
        if (m_Dirty & eBDF_Delimiter)
        {
            m_Dirty &= ~eBDF_Delimiter;
            TUIData delimiter(m_cDelimiter);
            delimiter.GetValueWithConversion(delimiter_str);
        }
        return delimiter_str;
    }

    template <class T>
    inline void setStringBuffer(const T* str) { assert(false); }
};

// Specialize in global scope
template <>
inline const CryStringT<char>& SArgumentParser::GetDelimiter() const
{
    return m_cDelimiter;
}

template <>
inline void SArgumentParser::setStringBuffer(const char* str)
{
    m_sArgStringBuffer = str;
    m_Dirty &= ~eBDF_String;
}

template <>
inline void SArgumentParser::setStringBuffer(const wchar_t* str)
{
    m_sArgWStringBuffer = str;
    m_Dirty &= ~eBDF_WString;
}

template <>
inline wchar_t* SArgumentParser::StrStrTmpl(wchar_t* str1, const wchar_t* str2)
{
    return wcsstr(str1, str2);
}

template <>
inline size_t SArgumentParser::StrLenTmpl(const wchar_t* str)
{
    return wcslen(str);
}

template <>
inline bool SArgumentParser::GetArg(int index, TUIData& val) const
{
    if (index >= 0 && index < m_ArgList.size())
    {
        val = GetArg(index);
        return true;
    }
    return false;
}

#endif // CRYINCLUDE_CRYCOMMON_ARGUMENTPARSER_H
