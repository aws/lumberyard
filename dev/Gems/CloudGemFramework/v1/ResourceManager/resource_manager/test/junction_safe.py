#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the 'License'). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
# $Revision: #1 $

# Origionally From http://code.activestate.com/recipes/578849-reimplementation-of-rmtree-supporting-windows-repa/
# License at time of copy: http://code.activestate.com/recipes/tags/meta:license=cc0/ (public domain)

#!python.exe
# encoding: utf-8
from __future__ import with_statement
import os, ctypes, sys
from ctypes.wintypes import DWORD, INT, LPWSTR, LONG, WORD, BYTE, WCHAR

# Some utility wrappers for pointer stuff. 
class c_void(ctypes.Structure):
	# c_void_p is a buggy return type, converting to int, so
	# POINTER(None) == c_void_p is actually written as
	# POINTER(c_void), so it can be treated as a real pointer.
	_fields_ = [('dummy', ctypes.c_int)]

	def __init__(self, value=None):
		if value is None: value = 0
		super(c_void, self).__init__(value)

def POINTER(obj):
	ptr = ctypes.POINTER(obj)
	# Convert None to a real NULL pointer to work around bugs
	# in how ctypes handles None on 64-bit platforms
	if not isinstance(ptr.from_param, classmethod):
		def from_param(cls, x):
			if x is None: return cls()
			return x
		ptr.from_param = classmethod(from_param)
	return ptr

# Shadow built-in c_void_p
LPVOID = c_void_p = POINTER(c_void)

# Globals
NULL = LPVOID()
kernel32 = ctypes.WinDLL('kernel32')
advapi32 = ctypes.WinDLL('advapi32')
_obtained_privileges = []

# Aliases to functions/classes, and utility lambdas
cast = ctypes.cast
byref = ctypes.byref
sizeof = ctypes.sizeof
WinError = ctypes.WinError
hasflag = lambda value, flag: (value & flag) == flag

# Constants derived from C
INVALID_HANDLE_VALUE = -1

# Desired access for OpenProcessToken
TOKEN_ADJUST_PRIVILEGES = 0x0020

# SE Privilege Names
SE_RESTORE_NAME = 'SeRestorePrivilege'
SE_BACKUP_NAME = 'SeBackupPrivilege'

# SE Privilege Attributes
SE_PRIVILEGE_ENABLED = 0x00000002L

# Access
FILE_ANY_ACCESS = 0

# CreateFile flags
FILE_FLAG_BACKUP_SEMANTICS   = 0x02000000
FILE_FLAG_OPEN_REPARSE_POINT = 0x00200000

# Generic access
GENERIC_READ = 0x80000000L
GENERIC_WRITE = 0x40000000L
GENERIC_RW = GENERIC_READ | GENERIC_WRITE

# File shared access
FILE_SHARE_READ = 0x00000001
FILE_SHARE_WRITE = 0x00000002
FILE_SHARE_DELETE = 0x00000004
FILE_SHARE_ALL = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE

# File stuff
OPEN_EXISTING = 3
FILE_ATTRIBUTE_REPARSE_POINT = 0x00000400
FILE_DEVICE_FILE_SYSTEM = 0x00000009

# Utility lambdas for figuring out ctl codes
CTL_CODE = lambda devtype, func, meth, acc: (devtype << 16) | (acc << 14) | (func << 2) | meth

# Methods
METHOD_BUFFERED = 0

# WinIoCtl Codes
FSCTL_GET_REPARSE_POINT = CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 42, METHOD_BUFFERED, FILE_ANY_ACCESS)
FSCTL_DELETE_REPARSE_POINT = CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 43, METHOD_BUFFERED, FILE_ANY_ACCESS)

# Reparse Point buffer constants
MAX_NAME_LENGTH = 1024
MAX_REPARSE_BUFFER = 16 * MAX_NAME_LENGTH
REPARSE_GUID_DATA_BUFFER_HEADER_SIZE = (2 * sizeof(WORD)) + (sizeof(DWORD) * 5)

# For our generic reparse buffer
MAX_GENERIC_REPARSE_BUFFER = MAX_REPARSE_BUFFER - REPARSE_GUID_DATA_BUFFER_HEADER_SIZE

# Type aliases
UCHAR = ctypes.c_ubyte
ULONG_PTR = ctypes.c_ssize_t
LPDWORD = POINTER(DWORD)

# CTypes-based wrapper classes
class BOOL(INT):
	"""
	Wrapper around ctypes.wintypes.INT (ctypes.c_int) to make BOOL act a bit more like
	a boolean.
	"""
	@classmethod
	def from_param(cls, value):
		if isinstance(value, ctypes._SimpleCData):
			return BOOL(value.value)
		elif not value or value is None:
			return BOOL(0)
		else:
			raise TypeError('Dont know what to do with instance of {0}'.format(type(value)))

	def __eq__(self, other):
		value = bool(self.value)
		if isinstance(other, bool):
			return value and other
		elif isinstance(other, ctypes._SimpleCData):
			return value and bool(other.value)
		else:
			return value and bool(other)

	def __hash__(self):
		return hash(self._as_parameter_)

class HANDLE(ULONG_PTR):
	"""
	Wrapper around the numerical representation of a pointer to
	add checks for INVALID_HANDLE_VALUE
	"""
	NULL = None
	INVALID = None

	def __init__(self, value=None):
		if value is None: value = 0
		super(HANDLE, self).__init__(value)
		self.autoclose = False

	@classmethod
	def from_param(cls, value):
		if value is None:
			return HANDLE(0)
		elif isinstance(value, ctypes._SimpleCData):
			return value
		else:
			return HANDLE(value)

	def close(self):
		if bool(self):
			try: CloseHandle(self)
			except: pass

	def __enter__(self):
		self.autoclose = True
		return self

	def __exit__(self, exc_typ, exc_val, trace):
		self.close()
		return False
	
	def __del__(self):
		if hasattr(self, 'autoclose') and self.autoclose:
			CloseHandle(self)

	def __nonzero__(self):
		return super(HANDLE, self).__nonzero__() and \
			self.value != HANDLE.INVALID.value

class GUID(ctypes.Structure):
	""" Borrowed small parts of this from the comtypes module. """
	_fields_ = [
		('Data1', DWORD),
		('Data2', WORD),
		('Data3', WORD),
		('Data4', (BYTE * 8)),
	]

# Ctypes Structures
class LUID(ctypes.Structure):
	_fields_ = [
		('LowPart', DWORD),
		('HighPart', LONG),
	]

class LUID_AND_ATTRIBUTES(LUID):
	_fields_ = [ ('Attributes', DWORD) ]

class GenericReparseBuffer(ctypes.Structure):
	_fields_ = [ ('PathBuffer', UCHAR * MAX_GENERIC_REPARSE_BUFFER) ]

class ReparsePoint(ctypes.Structure):
	"""
	Originally, Buffer was a union made up of SymbolicLinkBuffer, MountpointBuffer,
	and GenericReparseBuffer. Since we're not actually doing anything with the buffer
	aside from passing it along to the native functions, however, I've gone ahead
	and cleaned up some of of the unnecessary code.
	"""
	
	_fields_ = [
		('ReparseTag', DWORD),
		('ReparseDataLength', WORD),
		('Reserved', WORD),
		('ReparseGuid', GUID),
		('Buffer', GenericReparseBuffer)
	]

# Common uses of HANDLE
HANDLE.NULL = HANDLE()
HANDLE.INVALID = HANDLE(INVALID_HANDLE_VALUE)
LPHANDLE = POINTER(HANDLE)

# C Function Prototypes
CreateFile = kernel32.CreateFileW
CreateFile.restype = HANDLE
CreateFile.argtypes = [ LPWSTR, DWORD, DWORD, LPVOID, DWORD, DWORD, HANDLE ]

GetFileAttributes = kernel32.GetFileAttributesW
GetFileAttributes.restype = DWORD
GetFileAttributes.argtypes = [ LPWSTR ]

RemoveDirectory = kernel32.RemoveDirectoryW
RemoveDirectory.restype = BOOL
RemoveDirectory.argtypes = [ LPWSTR ]

CloseHandle = kernel32.CloseHandle
CloseHandle.restype = BOOL
CloseHandle.argtypes = [ HANDLE ]

GetCurrentProcess = kernel32.GetCurrentProcess
GetCurrentProcess.restype = HANDLE
GetCurrentProcess.argtypes = []

OpenProcessToken = advapi32.OpenProcessToken
OpenProcessToken.restype = BOOL
OpenProcessToken.argtypes = [ HANDLE, DWORD, LPHANDLE ]

LookupPrivilegeValue = advapi32.LookupPrivilegeValueW
LookupPrivilegeValue.restype = BOOL
LookupPrivilegeValue.argtypes = [ LPWSTR, LPWSTR, POINTER(LUID_AND_ATTRIBUTES) ]

AdjustTokenPrivileges = advapi32.AdjustTokenPrivileges
AdjustTokenPrivileges.restype = BOOL
AdjustTokenPrivileges.argtypes = [ HANDLE, BOOL, LPVOID, DWORD, LPVOID, LPDWORD ]

_DeviceIoControl = kernel32.DeviceIoControl
_DeviceIoControl.restype = BOOL
_DeviceIoControl.argtypes = [ HANDLE, DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD, LPVOID ]

def DeviceIoControl(hDevice, dwCtrlCode, lpIn, szIn, lpOut, szOut, lpOverlapped=None):
	"""
	Wrapper around the real DeviceIoControl to return a tuple containing a bool indicating
	success, and a number containing the size of the bytes returned. (Also, lpOverlapped to
	default to NULL) """
	dwRet = DWORD(0)
	return bool(
		_DeviceIoControl(hDevice, dwCtrlCode, lpIn, szIn, lpOut, szOut, byref(dwRet), lpOverlapped)
	), dwRet.value

def obtain_privileges(privileges):
	"""
	Given a list of SE privilege names (eg: [ SE_CREATE_TOKEN_NAME, SE_BACKUP_NAME ]), lookup
	the privilege values for each and then attempt to acquire them for the current process.
	"""
	global _obtained_privileges
	privileges = filter(lambda priv: priv not in _obtained_privileges, list(set(privileges)))
	privcount = len(privileges)
	if privcount == 0: return

	class TOKEN_PRIVILEGES(ctypes.Structure):
		#noinspection PyTypeChecker
		_fields_ = [
			('PrivilegeCount', DWORD),
			('Privileges', LUID_AND_ATTRIBUTES * privcount),
		]

	with HANDLE() as hToken:
		tp = TOKEN_PRIVILEGES()
		tp.PrivilegeCount = privcount
		hProcess = GetCurrentProcess()
		if not OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES, byref(hToken)):
			raise WinError()

		for i, privilege in enumerate(privileges):
			tp.Privileges[i].Attributes = SE_PRIVILEGE_ENABLED
			if not LookupPrivilegeValue(None, privilege, byref(tp.Privileges[i])):
				raise Exception('LookupPrivilegeValue failed for privilege: {0}'.format(privilege))

		if not AdjustTokenPrivileges(hToken, False, byref(tp), sizeof(TOKEN_PRIVILEGES), None, None):
			raise WinError()

		_obtained_privileges.extend(privileges)

def open_file(filepath, flags=FILE_FLAG_OPEN_REPARSE_POINT, autoclose=False):
	""" Open file for read & write, acquiring the SE_BACKUP & SE_RESTORE privileges. """
	obtain_privileges([ SE_BACKUP_NAME, SE_RESTORE_NAME ])
	if (flags & FILE_FLAG_BACKUP_SEMANTICS) != FILE_FLAG_BACKUP_SEMANTICS:
		flags |= FILE_FLAG_BACKUP_SEMANTICS
	hFile = CreateFile(filepath, GENERIC_RW, FILE_SHARE_ALL, NULL, OPEN_EXISTING, flags, HANDLE.NULL)
	if not hFile: raise WinError()
	if autoclose: hFile.autoclose = True
	return hFile

def get_buffer(filepath, hFile=HANDLE.INVALID):
	""" Get a reparse point buffer. """
	if not hFile:
		hFile = open_file(filepath, autoclose = True)
	
	obj = ReparsePoint()
	result, dwRet = DeviceIoControl(hFile, FSCTL_GET_REPARSE_POINT, None, 0L, byref(obj), MAX_REPARSE_BUFFER)
	return obj if result else None

def delete_reparse_point(filepath):
	""" Remove the reparse point folder at filepath. """
	dwRet = 0
	with open_file(filepath) as hFile:
		# Try to delete it first without the reparse GUID
		info = ReparsePoint()
		info.ReparseTag = 0
		result, dwRet = DeviceIoControl(hFile, FSCTL_DELETE_REPARSE_POINT, byref(info),
										REPARSE_GUID_DATA_BUFFER_HEADER_SIZE , None, 0L)

		if not result:
			# If the first try fails, we'll set the GUID and try again
			buffer = get_buffer(filepath, hFile)
			info.ReparseTag = buffer.ReparseTag
			info.ReparseGuid = info.ReparseGuid
			result, dwRet = DeviceIoControl(hFile, FSCTL_DELETE_REPARSE_POINT, byref(info), 
											REPARSE_GUID_DATA_BUFFER_HEADER_SIZE, None, 0L)
			if not result: raise WinError()
	
	if not RemoveDirectory(filepath):
		raise WinError()
	
	return dwRet

def is_reparse_point(filepath):
	""" Check whether or not filepath refers to a reparse point. """
	return hasflag(GetFileAttributes(filepath), FILE_ATTRIBUTE_REPARSE_POINT)

def rmtree(filepath, ignore_errors=False, onerror=None):
	"""
	Re-implementation of shutil.rmtree that checks for reparse points
	(junctions/symbolic links) before iterating folders.
	"""
	
	def rm(fn, childpath):
		try:
			fn(childpath)
		except:
			if not ignore_errors:
				if onerror is None:
					raise
				else:
					onerror(fn, childpath, sys.exc_info()[0])
	
	def visit_files(root, targets):
		for target in targets:
			rm(os.unlink, os.path.join(root, target))
	
	def visit_dirs(root, targets):
		for target in targets:
			childpath = os.path.join(root, target)
			rmtree(childpath, ignore_errors, onerror)
	
	if is_reparse_point(filepath):
		rm(delete_reparse_point, filepath)
		return
	
	for root, dirs, files in os.walk(filepath):
		visit_files(root, files)
		visit_dirs(root, dirs)
	
	rm(os.rmdir, filepath)


def mklink(target_path, link_path):
    os_symlink = getattr(os, "symlink", None)
    if callable(os_symlink):
        os_symlink(target_path, link_path)
    else:
        # There isn't a simple "CreateJunction" Win32 API we can call using ctypes, 
        # so instead we execute mklink /J in using the shell.
        import subprocess
        result = subprocess.call(['mklink', '/J', link_path, target_path], shell=True)
        if result != 0:
            raise RuntimeError('Could not create junction point.')
            
                                     
