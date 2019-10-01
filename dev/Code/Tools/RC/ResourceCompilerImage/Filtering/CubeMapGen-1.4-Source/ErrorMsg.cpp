//--------------------------------------------------------------------------------------
// ErrorMsg.cpp
// 
//   Common error message handling functions
//
//--------------------------------------------------------------------------------------
// (C) 2005 ATI Research, Inc., All rights reserved.
//--------------------------------------------------------------------------------------
// modifications by Crytek GmbH

#include "Types.h"
#include "ErrorMsg.h"
#include <AzCore/base.h>

#if AZ_TRAIT_OS_PLATFORM_APPLE
#define SUCCEEDED(x) ((x) >= 0)
#define FAILED(x) (!(SUCCEEDED(x)))
#endif

uint32 sg_MessageType = EM_DEFAULT_MESSAGE_MEDIUM;

//MessageOutputCallback
void (*sg_MessageOutputCallback)(const WCHAR *, const WCHAR *) = DefaultErrorMessageCallback;


//--------------------------------------------------------------------------------------
// SetErrorMessageCallback
//
//--------------------------------------------------------------------------------------
void DefaultErrorMessageCallback(const WCHAR *a_Title, const WCHAR *a_Message )
{
//   MessageBox(NULL, a_Message, a_Title, MB_OK);
}


//--------------------------------------------------------------------------------------
// SetErrorMessageCallback
//
//--------------------------------------------------------------------------------------
void SetErrorMessageCallback( void(*a_MessageOutputFunc)(const WCHAR *, const WCHAR *) )
{
   sg_MessageType = EM_MESSAGE_MEDIUM_CALLBACK_FUNCTION;
   sg_MessageOutputCallback = a_MessageOutputFunc;
}


//--------------------------------------------------------------------------------------
//  Pulls up a message box, or calls a custom message function with an error message
//
//  note: this function is used to route all error messages though a common output 
//   mechanism..  in the future output could be rerouted through the console using 
//   this mechanism.
//--------------------------------------------------------------------------------------
void OutputMessageString(const WCHAR *a_Title, const WCHAR *a_Message )
{
    switch(sg_MessageType)
    {
       case EM_MESSAGE_MEDIUM_MESSAGEBOX:
       {
#if defined(AZ_PLATFORM_WINDOWS)
            MessageBoxW(NULL, a_Message, a_Title, MB_OK);
#else
            //TODO:Implement alert box functionality with an 'ok' button for other platforms
#endif
            break;
       }
       case EM_MESSAGE_MEDIUM_CALLBACK_FUNCTION:
       {
            sg_MessageOutputCallback(a_Title, a_Message);
            break;
       }
       default:
            break;
    }
}

//--------------------------------------------------------------------------------------
// variable arguement version of output message
//--------------------------------------------------------------------------------------
void OutputMessage(const WCHAR *a_Message, ... )
{   
    int32 numCharOutput = 0;
    WCHAR msgBuffer[EM_MAX_MESSAGE_LENGTH];
    va_list args;
    va_start(args, a_Message);

    numCharOutput = azvsnwprintf( msgBuffer, EM_MAX_MESSAGE_LENGTH, a_Message, args);

    //va_end(args, a_Message);
    OutputMessageString(L"Message:", msgBuffer);
}

//--------------------------------------------------------------------------------------
// displays the message with a YES / NO response; returns true on YES and false on NO
//--------------------------------------------------------------------------------------
bool OutputQuestion( const WCHAR *a_Message, ... )
{
   WCHAR msgBuffer[ EM_MAX_MESSAGE_LENGTH ];
   va_list args;
   va_start( args, a_Message );

   azvsnwprintf( msgBuffer, EM_MAX_MESSAGE_LENGTH, a_Message, args );

    switch( sg_MessageType )
    {
        case EM_MESSAGE_MEDIUM_MESSAGEBOX:
        {
#if defined(AZ_PLATFORM_WINDOWS)
            return MessageBoxW( NULL, msgBuffer, L"Question:", MB_YESNO ) == IDYES;
#else
            //TODO:Implement message box functionality with an 'ok' button for other platforms
#endif
            break;
        }
        case EM_MESSAGE_MEDIUM_CALLBACK_FUNCTION:
        {
            sg_MessageOutputCallback( L"Question:", msgBuffer );
            break;
        }
        default:
            break;
    }
    return false;
}

//--------------------------------------------------------------------------------------
// output message if HRESULT indicates failure
//  
//--------------------------------------------------------------------------------------
HRESULT OutputMessageOnFail(HRESULT a_hr, WCHAR *a_Message, ... )
{   
    int32 numCharOutput = 0;
    WCHAR msgBuffer[EM_MAX_MESSAGE_LENGTH];
    va_list args;

    if(FAILED(a_hr))
    {
        va_start(args, a_Message);

        numCharOutput = azvsnwprintf( msgBuffer, EM_MAX_MESSAGE_LENGTH, a_Message, args);

        //va_end(args, a_Message);

        OutputMessageString(L"Error!", msgBuffer);
    }

    return a_hr;
}


//--------------------------------------------------------------------------------------
// output message and exit program if HRESULT indicates failure
//  
//--------------------------------------------------------------------------------------
HRESULT OutputFatalMessageOnFail(HRESULT a_hr, WCHAR *a_Message, ... )
{   
    int32 numCharOutput = 0;
    WCHAR msgBuffer[EM_MAX_MESSAGE_LENGTH];
    va_list args;

    if(FAILED(a_hr))
    {
        va_start(args, a_Message);

        numCharOutput = azvsnwprintf( msgBuffer, EM_MAX_MESSAGE_LENGTH, a_Message, args);

        //va_end(args, a_Message);

        OutputMessageString(L"Fatal Error!", msgBuffer);
        exit(EM_FATAL_ERROR);
    }

    return a_hr;
}


