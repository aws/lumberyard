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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_TEMPLDEF_H
#define CRYINCLUDE_EDITOR_CONTROLS_TEMPLDEF_H
#pragma once


///////////////////////////////////////////////////////////////////////////////
// TemplDef.h: helper macroses for using templates with MFC
//
// Author: Yury Goltsman
// email:   ygprg@go.to
// page:    http://go.to/ygprg
// Copyright 2000, Yury Goltsman
//
// This code provided "AS IS," without any kind of warranty.
// You may freely use or modify this code provided this
// Copyright is included in all derived versions.
//
// version : 1.0
//

///////////////////////////////////////////////////////////////////////////////
// common definitions for any map:

// use it to specify list of arguments for class and as theTemplArgs
// e.g. BEGIN_TEMPLATE_MESSAGE_MAP_CUSTOM(ARGS2(class base_class, int max),
//                                 CMyTClass<ARGS2(base_class, max)>,
//                                 base_class)

#define ARGS2(arg1, arg2)                     arg1, arg2
#define ARGS3(arg1, arg2, arg3)               arg1, arg2, arg3
#define ARGS4(arg1, arg2, arg3, arg4)         arg1, arg2, arg3, arg4
#define ARGS5(arg1, arg2, arg3, arg4, arg5)   arg1, arg2, arg3, arg4, arg5

///////////////////////////////////////////////////////////////////////////////
// definition for MESSAGE_MAP:

#define DECLARE_TEMPLATE_MESSAGE_MAP() DECLARE_MESSAGE_MAP()

#if _MFC_VER >= 0x0800
//  MFC 8 (VS 2005)

#define BEGIN_TEMPLATE_MESSAGE_MAP_CUSTOM(theTemplArgs, theClass, baseClass) \
    PTM_WARNING_DISABLE                                                      \
    template <theTemplArgs>                                                  \
    const AFX_MSGMAP * theClass::GetMessageMap() const                       \
    { return GetThisMessageMap(); }                                          \
    template <theTemplArgs>                                                  \
    const AFX_MSGMAP * PASCAL theClass::GetThisMessageMap()                  \
    {                                                                        \
        typedef theClass ThisClass;                                          \
        typedef baseClass TheBaseClass;                                      \
        static const AFX_MSGMAP_ENTRY _messageEntries[] =                    \
        {
#elif defined(WIN64) && _MFC_VER < 0x700

#ifdef _AFXDLL
#define BEGIN_TEMPLATE_MESSAGE_MAP_CUSTOM(theTemplArgs, theClass, baseClass) \
    template <theTemplArgs>                                                  \
    const AFX_MSGMAP * PASCAL theClass::_GetBaseMessageMap()                 \
    { return &baseClass::messageMap; }                                       \
    template <theTemplArgs>                                                  \
    const AFX_MSGMAP * theClass::GetMessageMap() const                       \
    { return &theClass::messageMap; }                                        \
    template <theTemplArgs>                                                  \
    /*AFX_COMDAT AFX_DATADEF*/ const AFX_MSGMAP theClass::messageMap =       \
    { &theClass::_GetBaseMessageMap, &theClass::_messageEntries[0] };        \
    template <theTemplArgs>                                                  \
    /*AFX_COMDAT */ const AFX_MSGMAP_ENTRY theClass::_messageEntries[] =     \
    {
#else
#define BEGIN_TEMPLATE_MESSAGE_MAP_CUSTOM(theTemplArgs, theClass, baseClass) \
    template <theTemplArgs>                                                  \
    const AFX_MSGMAP * theClass::GetMessageMap() const                       \
    { return &theClass::messageMap; }                                        \
    template <theTemplArgs>                                                  \
    /*AFX_COMDAT */ AFX_DATADEF const AFX_MSGMAP theClass::messageMap =      \
    { &baseClass::messageMap, &theClass::_messageEntries[0] };               \
    template <theTemplArgs>                                                  \
    /*AFX_COMDAT */ const AFX_MSGMAP_ENTRY theClass::_messageEntries[] =     \
    {
#endif // _AFXDLL

#else

#ifdef _AFXDLL

#define BEGIN_TEMPLATE_MESSAGE_MAP_CUSTOM(theTemplArgs, theClass, baseClass) \
    template <theTemplArgs>                                                  \
    const AFX_MSGMAP * PASCAL theClass::GetThisMessageMap()                  \
    { return &theClass::messageMap; }                                        \
    template <theTemplArgs>                                                  \
    const AFX_MSGMAP * theClass::GetMessageMap() const                       \
    { return &theClass::messageMap; }                                        \
    template <theTemplArgs>                                                  \
    /*AFX_COMDAT AFX_DATADEF*/ const AFX_MSGMAP theClass::messageMap =       \
    { &baseClass::GetThisMessageMap, &theClass::_messageEntries[0] };        \
    template <theTemplArgs>                                                  \
    /*AFX_COMDAT */ const AFX_MSGMAP_ENTRY theClass::_messageEntries[] =     \
    {                                                                        \

#else
#define BEGIN_TEMPLATE_MESSAGE_MAP_CUSTOM(theTemplArgs, theClass, baseClass) \
    template <theTemplArgs>                                                  \
    const AFX_MSGMAP * theClass::GetMessageMap() const                       \
    { return &theClass::messageMap; }                                        \
    template <theTemplArgs>                                                  \
    /*AFX_COMDAT */ AFX_DATADEF const AFX_MSGMAP theClass::messageMap =      \
    { &baseClass::messageMap, &theClass::_messageEntries[0] };               \
    template <theTemplArgs>                                                  \
    /*AFX_COMDAT */ const AFX_MSGMAP_ENTRY theClass::_messageEntries[] =     \
    {                                                                        \

#endif // _AFXDLL

#endif //WIN64

#define END_TEMPLATE_MESSAGE_MAP_CUSTOM() END_MESSAGE_MAP()

#endif // CRYINCLUDE_EDITOR_CONTROLS_TEMPLDEF_H
