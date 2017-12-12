To create Dia2Lib.dll:

C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin>"C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\Tools\vsvars32.bat"
Setting environment for using Microsoft Visual Studio 2008 x86 tools.

C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin>midl /I "C:\Program Files (x86)\Microsoft Visual Studio 9.0\DIA SDK\include" "C:\Program Files (x86)\Microsoft Visual Studio 9.0\DIA SDK\idl\dia2.idl" /tlb dia2.tlb
Microsoft (R) 32b/64b MIDL Compiler Version 7.00.0555
Copyright (c) Microsoft Corporation. All rights reserved.
Processing C:\Program Files (x86)\Microsoft Visual Studio 9.0\DIA SDK\idl\dia2.idl
dia2.idl
Processing C:\Program Files\Microsoft SDKs\Windows\v6.0A\include\objidl.idl
objidl.idl
Processing C:\Program Files\Microsoft SDKs\Windows\v6.0A\include\unknwn.idl
unknwn.idl
Processing C:\Program Files\Microsoft SDKs\Windows\v6.0A\include\wtypes.idl
wtypes.idl
Processing C:\Program Files\Microsoft SDKs\Windows\v6.0A\include\basetsd.h
basetsd.h
Processing C:\Program Files\Microsoft SDKs\Windows\v6.0A\include\guiddef.h
guiddef.h
Processing C:\Program Files\Microsoft SDKs\Windows\v6.0A\include\oaidl.idl
oaidl.idl
Processing C:\Program Files\Microsoft SDKs\Windows\v6.0A\include\propidl.idl
propidl.idl
Processing C:\Program Files (x86)\Microsoft Visual Studio 9.0\DIA SDK\include\cvconst.h
cvconst.h
Processing C:\Program Files\Microsoft SDKs\Windows\v6.0A\include\oaidl.acf
oaidl.acf

C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin>TlbImp.exe dia2.tlb
Microsoft (R) .NET Framework Type Library to Assembly Converter 3.5.30729.1
Copyright (C) Microsoft Corporation.  All rights reserved.

Type library imported to Dia2Lib.dll

