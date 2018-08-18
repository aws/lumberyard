#!/usr/local/bin/python

from __future__ import print_function

"""A Python version of the Perforce "p4" client.
    
    This uses the Python type P4API.P4Adapter, which is a wrapper for the
    Perforce ClientApi object.
    
    $Id: //depot/r17.2/p4-python/P4.py#1 $
    
    #*******************************************************************************
    # Copyright (c) 2007-2010, Perforce Software, Inc.  All rights reserved.
    # Portions Copyright (c) 1999, Mike Meyer. All rights reserved.
    # Portions Copyright (c) 2004-2007, Robert Cowham. All rights reserved.
    #
    # Redistribution and use in source and binary forms, with or without
    # modification, are permitted provided that the following conditions are met:
    #
    # 1.  Redistributions of source code must retain the above copyright
    # notice, this list of conditions and the following disclaimer.
    #
    # 2.  Redistributions in binary form must reproduce the above copyright
    # notice, this list of conditions and the following disclaimer in the
    # documentation and/or other materials provided with the distribution.
    #
    # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    # AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    # IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    # ARE DISCLAIMED. IN NO EVENT SHALL PERFORCE SOFTWARE, INC. BE LIABLE FOR ANY
    # DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    # (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    # LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    # ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    # (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    # SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    #*******************************************************************************
    
    Copyright 2007-2010 Perforce Software, Inc. All rights reserved
    
    
    License:
    See accompanying LICENSE.txt including for redistribution permission.
    """

import sys, datetime
import re
import shutil
from contextlib import contextmanager
import uuid, tempfile
import os, os.path, platform
import subprocess

# P4Exception - some sort of error occurred
class P4Exception(Exception):
    """Exception thrown by P4 in case of Perforce errors or warnings"""
    
    def __init__(self, value):
        Exception.__init__(self)
        
        if isinstance(value, (list, tuple)) and len(value) > 2:
            self.value = value[0]
            self.errors = value[1]
            self.warnings = value[2]
        else:
            self.value = value
    def __str__(self):
        return str(self.value)

class Spec(dict):
    """Subclass of dict, representing the fields of a spec definition.
        
        Attributes can be accessed either with the conventional dict format,
        spec['attribute'] or with shorthand spec._attribute.
        
        Instances of this class will preventing any unknown keys.
        """
    def __init__(self, fieldmap=None):
        self.__dict__['_Spec__fields'] = fieldmap
    
    def permitted_fields(self):
        return self.__fields
    
    def __setitem__(self, key, value):
        if not isinstance(value, str) and not isinstance(value, list):
            raise P4Exception("Illegal value of type %s, must be string or list" % value.__class__)
        if key in self or self.__fields == None:
            dict.__setitem__(self, key, value)
        elif str(key).lower() in self.__fields:
            dict.__setitem__(self, self.__fields[key.lower()], value)
        else:
            raise P4Exception("Illegal field '%s'" % str(key))
    
    def __getattr__(self, attr):
        key = str(attr).lower()
        if key[0] != '_':
            raise AttributeError(attr)
        key = key[1:]
        if key in self:
            return self[key]
        elif key in self.__fields:
            return self[self.__fields[key]]
    
    def __setattr__(self, attr, value):
        if attr == 'comment':
            self.__dict__[attr] = value
        else:
            key = str(attr).lower()
            if key[0] != '_':
                raise AttributeError(attr)
            key = key[1:]
            self[key] = value

#
# P4Integration objects hold details about the integrations that have
# been performed on a particular revision. Used primarily with the
# P4Revision class
#
class Integration:
    def __init__( self, how, file, srev, erev ):
        self.how = how
        self.file = file
        self.srev = srev
        self.erev = erev
    
    def __repr__(self):
        return "Integration (how = %s file = %s srev = %s erev = %s)" \
            % (self.how, self.file, self.srev, self.erev)
#
# Each P4Revision object holds details about a particular revision
# of a file. It may also contain the history of any integrations
# to/from the file
#

class Revision:
    def __init__( self, depotFile ):
        self.depotFile = depotFile
        self.integrations = []
        self.rev = None
        self.change = None
        self.action = None
        self.type = None
        self.time = None
        self.user = None
        self.client = None
        self.desc = None
        self.digest = None
        self.fileSize = None
    
    def integration( self, how, file, srev, erev ):
        rec = Integration( how, file, srev, erev )
        self.integrations.append( rec )
        return rec
    
    # iterator over the collection calling a provided function
    # Python's poor man version of the Ruby block
    def each_integration(self):
        for i in self.integrations:
            yield i
    
    def __repr__(self):
        return "Revision (depotFile = %s rev = %s change = %s action = %s type = %s time = %s user = %s client = %s)" % \
            (self.depotFile, self.rev, self.change, self.action, self.type, self.time, self.user, self.client)
#
# Each DepotFile entry contains details about one depot file.
#
class DepotFile:
    def __init__( self, name ):
        self.depotFile = name
        self.revisions = []
    
    def new_revision(self):
        r = Revision( self.depotFile )
        self.revisions.append( r )
        return r
    
    def each_revision(self):
        for r in self.revisions:
            yield r

    def str_revision(self, rev, revFormat, changeFormat):
        result = "... #{rev:<{rf}} change {change:{cf}} {action:9} on {date}  by {user}@{client} ({type}) '{desc}'". \
            format(rev = rev.rev, 
                   rf = revFormat,
                   change = rev.change,
                   cf = changeFormat,
                   action = rev.action, 
                   date = rev.time, 
                   user = rev.user, 
                   client = rev.client, 
                   type = rev.type,
                   desc = rev.desc)
        return result
 
    def str_integration(self, integ):
        result = "... ... {how} {file}#{srev},{erev}". \
            format(how = integ.how, file = integ.file, srev = integ.srev, erev = integ.erev)
        return result
    
    def __str__(self):
        result = "{}".format(self.depotFile)
        
        revFormat = len(str(self.revisions[0].rev))
        changeFormat = len(str(self.revisions[0].change))
        
        for rev in self.revisions:
            result += "\n{}".format(self.str_revision(rev, revFormat, changeFormat))
            for integ in rev.integrations:
                result += "\n{}".format(self.str_integration(integ))
        return result
    
    def __repr__(self):
        return "DepotFile (depotFile = %s, %s revisions)" % ( self.depotFile, len( self.revisions ) )

#
# Resolver class used in p4.run_resolve()
#
# The default simply checks that p4.input is set to sensible value
# This class is meant to be subclassed for a custom resolver and
# Resolver.resolve() overriden
#

class Resolver:
    def __init__(self):
        pass
    
    def resolve(self, mergeInfo):
        if mergeInfo.merge_hint == "e":
            print("Standard resolver encountered merge conflict, skipping resolve")
            return "s"
        else:
            return mergeInfo.merge_hint

    def actionResolve(self, mergeInfo):
        return mergeInfo.merge_hint

#
# OutputHandler base class
#
# Extend this class if you want to use the handler interface
#

class OutputHandler:
    REPORT  = 0
    HANDLED = 1
    CANCEL  = 2
    
    def __init__(self):
        pass
    
    def outputText(self, s):
        return OutputHandler.REPORT
    
    def outputBinary(self, b):
        return OutputHandler.REPORT
    
    def outputStat(self, h):
        return OutputHandler.REPORT
    
    def outputInfo(self, i):
        return OutputHandler.REPORT
    
    def outputMessage(self, e):
        return OutputHandler.REPORT

class ReportHandler( OutputHandler ):
    def __init__(self):
        OutputHandler.__init__(self)
    
    def outputText(self, s):
        print( "text: ", s)
        return OutputHandler.HANDLED
    
    def outputBinary(self, b):
        print( "binary: ", b)
        return OutputHandler.HANDLED
    
    def outputStat(self, h):
        print( "stat:", h)
        return OutputHandler.HANDLED
    
    def outputInfo(self, i):
        print( "info: ", i)
        return OutputHandler.HANDLED
    
    def outputMessage(self, e):
        print( "error:", e)
        return OutputHandler.HANDLED

class Progress:
    TYPE_SENDFILE = 1
    TYPE_RECEIVEFILE = 2
    TYPE_TRANSFER = 3
    TYPE_COMPUTATION = 4
    
    UNIT_PERCENT = 1
    UNIT_FILES = 2
    UNIT_KBYTES = 3
    UNIT_MBYTES = 4
    
    def __init__(self):
        pass
    
    def init(self, type):
        self.type = type
    
    def setDescription( self, description, units ):
        self.description = description
        self.units = units
    
    def setTotal( self, total ):
        self.total = total
    
    def update( self, position ):
        self.position = position
    
    def done( self, fail ):
        pass

class TextProgress(Progress):
    TYPES = [ "Unknown", "Submit", "Sync", "Clone" ]
    UNITS = [ "Unknown", "Percent", "Files", "KBytes", "MBytes" ]
    
    def __init__(self):
        Progress.__init__(self)
    
    def init(self, type):
        Progress.init(self, type)
        print( "Progress.init with '%s'" % self.TYPES[type] )
    
    def setDescription(self, description, units):
        Progress.setDescription(self, description, units)
        print( "Progress.setDescription with '%s' and units '%s'" % (description, self.UNITS[units]) )
    
    def setTotal( self, total ):
        Progress.setTotal(self, total)
        print( "Progress.setTotal with '%s' " % total )
    
    def update( self, position ):
        Progress.update(self, position )
        print( "Progress.update with '%s'" % position )
    
    def done( self, fail ):
        Progress.done(self, fail)
        print( "Progress.done with '%s"'' % fail )

def processFilelog(h):
    if "depotFile" in h:
        df = DepotFile( h[ "depotFile" ] )
        for n, rev in enumerate( h[ "rev" ]):
            # Create a new revision of this file ready for populating
            r = df.new_revision()
            # Populate the base attributes of each revision
            r.rev = int( rev )
            r.change = int( h[ "change" ][ n ] )
            r.action = h[ "action" ][ n ]
            r.type = h[ "type" ][ n ]
            r.time = datetime.datetime.utcfromtimestamp( int( h[ "time" ][ n ]) )
            r.user = h[ "user" ][ n ]
            r.client = h[ "client" ][ n ]
            r.desc = h[ "desc" ][ n ]
            if "digest" in h and n < len(h[ "digest" ]):
                r.digest = h[ "digest" ][ n ]
            if "fileSize" in h and n < len(h[ "fileSize" ]):
                r.fileSize = h[ "fileSize" ][ n ]
            
            # Now if there are any integration records for this revision,
            # add them in too
            
            if (not "how" in h) or (n >= len(h["how"]) or h["how"][n] == None):
                continue
            else:
                for m, how in enumerate( h[ "how" ][ n ] ):
                    file = h[ "file" ][ n ][ m ]
                    srev = h[ "srev" ][ n ][ m ].lstrip('#')
                    erev = h[ "erev" ][ n ][ m ].lstrip('#')
                    
                    if srev == "none":
                        srev = 0
                    else:
                        srev = int( srev )
                
                    if erev == "none":
                        erev = 0
                    else:
                        erev = int( erev )
        
                    r.integration( how, file, srev, erev )
        return df
    else:
        raise Exception("Not a filelog object: " + h)

class FilelogOutputHandler(OutputHandler):
    def __init__(self):
        OutputHandler.__init__(self)
    
    def outputStat(self, h):
        df = processFilelog(h)
        
        return self.outputFilelog(df)
    
    def outputFilelog(self, f):
        return OutputHandler.REPORT

# This is where the C/C++ shared library is loaded
# It has to be in this place because the library needs to access
# the classes defined above. Accessing classes defined below this
# entry would cause an endless loop

import P4API

class P4(P4API.P4Adapter):
    """Use this class to communicate with a Perforce server
        
        Instances of P4 will use the environment settings (including P4CONFIG)
        to determine the connection parameters such as P4CLIENT and P4PORT.
        
        This attributes can also be set separately before connecting.
        
        To run any Perforce commands, users of this class first need to run
        the connect() method.
        
        It is good practice to disconnect() after the program is complete.
        """
    # Constants useful for exception_level
    # RAISE_ALL:     Errors and Warnings are raised as exceptions (default)
    # RAISE_ERROR:   Only Errors are raised as exceptions
    # RAISE_NONE:    No exceptions are raised, instead False is returned
    
    RAISE_ALL    = 2
    RAISE_ERROR  = 1
    RAISE_ERRORS = 1
    RAISE_NONE   = 0
    
    # Named values for generic error codes returned by
    # P4API.Message.generic
    
    EV_NONE      = 0     # misc
    
    # The fault of the user
    
    EV_USAGE     = 0x01  # request not consistent with dox
    EV_UNKNOWN   = 0x02  # using unknown entity
    EV_CONTEXT   = 0x03  # using entity in wrong context
    EV_ILLEGAL   = 0x04  # trying to do something you can't
    EV_NOTYET    = 0x05  # something must be corrected first
    EV_PROTECT   = 0x06  # protections prevented operation
    
    # No fault at all
    
    EV_EMPTY     = 0x11  # action returned empty results
    
    # not the fault of the user
    
    EV_FAULT     = 0x21  # inexplicable program fault
    EV_CLIENT    = 0x22  # client side program errors
    EV_ADMIN     = 0x23  # server administrative action required
    EV_CONFIG    = 0x24  # client configuration inadequate
    EV_UPGRADE   = 0x25  # client or server too old to interact
    EV_COMM      = 0x26  # communications error
    EV_TOOBIG    = 0x27  # not even Perforce can handle this much
    
    # Named values for error severities returned by
    # P4API.Message.severity
    E_EMPTY      = 0  # nothing yet
    E_INFO       = 1  # something good happened
    E_WARN       = 2  # something not good happened
    E_FAILED     = 3  # user did something wrong
    E_FATAL      = 4  # system broken -- nothing can continue
    
    # mappings for __iterate
    # list-of-specs => ( name-of-one-spec, field-name-in-list-of-specs )
    
    specfields = {
        'clients'   :   ('client', 'client'),
        'labels'    :   ('label', 'label'),
        'branches'  :   ('branch', 'branch'),
        'changes'   :   ('change', 'change'),
        'streams'   :   ('stream', 'Stream'),
        'jobs'      :   ('job', 'Job'),
        'users'     :   ('user', 'User'),
        'groups'    :   ('group', 'group'),
        'depots'    :   ('depot', 'name'),
        'servers'   :   ('server', 'Name')
    }
    
    def __init__(self, *args, **kwlist):
        P4API.P4Adapter.__init__(self, *args, **kwlist)
    
    def __del__(self):
        if self.debug > 3:
            print("P4.__del__()", file=sys.stderr)
    
    def __getattr__(self, name):
        if name.startswith("run_"):
            cmd = name[len("run_"):]
            return lambda *args, **kargs: self.run(cmd, *args, **kargs)
        elif name.startswith("delete_"):
            cmd = name[len("delete_"):]
            return lambda *args, **kargs: self.run(cmd, "-d", *args, **kargs)
        elif name.startswith("fetch_"):
            cmd = name[len("fetch_"):]
            return lambda *args, **kargs: self.__fetch(cmd, *args, **kargs)
        elif name.startswith("save_"):
            cmd = name[len("save_"):]
            return lambda *args, **kargs: self.__save(cmd, *args, **kargs)
        elif name.startswith("parse_"):
            cmd = name[len("parse_"):]
            return lambda *args, **kargs: self.__parse_spec(cmd, *args, **kargs)
        elif name.startswith("format_"):
            cmd = name[len("format_"):]
            return lambda *args, **kargs: self.__format_spec(cmd, *args, **kargs)
        elif name.startswith("iterate_"):
            cmd = name[len("iterate_"):]
            return lambda *args, **kargs: self.__iterate(cmd, *args, **kargs)
        else:
            raise AttributeError(name)
    
    def __save(self, cmd, *args, **kargs):
        self.input = args[0]
        return self.run(cmd, "-i", args[1:], **kargs)
    
    def __parse_spec(self, cmd, *args, **kargs):
        form = args[0]
        comments = "\n".join( [ x for x in form.split('\n') if x.startswith('#') ] ) + "\n"
        spec = self.parse_spec(cmd, *args, **kargs)
        spec.__dict__['comment'] = comments
        return spec
    
    def __format_spec(self, cmd, *args, **kargs):
        spec = args[0]
        form = self.format_spec(cmd, *args, **kargs)
        if 'comment' in spec.__dict__:
            form = spec.__dict__['comment'] + "\n" +  form
        return form
    
    def __fetch(self, cmd, *args, **kargs):
        result = self.run(cmd, "-o", *args, **kargs)
        for r in result:
            if isinstance(r, tuple) or isinstance(r,dict):
                return r 
        return result[0]
    
    def __iterate(self, cmd, *args, **kargs):
        
        if cmd in self.specfields:
            specs = self.run(cmd, *args, **kargs)
            spec = self.specfields[cmd][0]
            field = self.specfields[cmd][1]
            
            # Return a generators (Python iterator object)
            # On iteration, this will retrieve one spec at a time
            return ( self.run(spec, '-o', x[field])[0] for x in specs )
        else:
            raise Exception('Unknown spec list command: %s', cmd)
    
    def __repr__(self):
        state = "disconnected"
        if self.connected():
            state = "connected"
        
        return "P4 [%s@%s %s] %s" % \
            (self.user, self.client, self.port, state)
    
    def identify(cls):
        return P4API.identify()
    identify = classmethod(identify)

    def log_messages(self):
        for message in self.messages:
            if message.severity == 3:
                self.logger.error(message)
            elif message.severity == 2:
                self.logger.warning(message)
            elif message.severity == 1:
                self.logger.info(message)
    
        
    def run(self, *args, **kargs):
        "Generic run method"
        context = {}
        resultLogging = True
        
        if "resultLogging" in kargs:
            resultLogging= False
            del kargs["resultLogging"]
        
        for (k,v) in list(kargs.items()):
            context[k] = getattr(self, k)
            setattr(self, k, v)
                
        flatArgs = self.__flatten(args)
        
        # if encoding is set, translate to Bytes
        if hasattr(self,"encoding") and self.encoding and not self.encoding == 'raw':
            result = []
            for s in flatArgs:
                result.append( s.encode(self.encoding) )
            flatArgs = result
        
        if self.logger:
            self.logger.info("p4 " + " ".join(flatArgs))
        
        try:
            result = P4API.P4Adapter.run(self, *flatArgs)
        except P4Exception as e:
            if self.logger:
                self.log_messages()
            for (k,v) in list(context.items()):
                setattr( self, k, v)
            raise e
        
        if self.logger:
            self.log_messages()
        
        if resultLogging and self.logger:
            self.logger.debug(result)

        for (k,v) in list(context.items()):
            setattr( self, k, v)
                    
        return result
    
    def run_submit(self, *args, **kargs):
        "Simplified submit - if any arguments is a dict, assume it to be the changeform"
        nargs = list(args)
        form = None
        for n, arg in enumerate(nargs):
            if isinstance( arg, dict):
                self.input = arg
                nargs.pop(n)
                nargs.append("-i")
                break
        return self.run("submit", *nargs, **kargs)
    
    def run_shelve(self, *args, **kargs):
        "Simplified shelve - if any arguments is a dict, assume it to be the changeform"
        nargs = list(args)
        form = None
        for n, arg in enumerate(nargs):
            if isinstance( arg, dict):
                self.input = arg
                nargs.pop(n)
                nargs.append("-i")
                break
        return self.run("shelve", *nargs, **kargs)
    
    def delete_shelve(self, *args, **kargs):
        "Simplified deletion of shelves - if no -c is passed in, add it to the args"
        nargs = list(args)
        if '-c' not in nargs:
            nargs = ['-c'] + nargs # prepend -c if it is not there
        nargs = ['-d'] + nargs
        return self.run("shelve", *nargs, **kargs)
    
    def run_login(self, *args, **kargs):
        "Simple interface to make login easier"
        
        if "password" in kargs:
            password = kargs["password"]
            self.input = password
            del kargs["password"]
        else:
            self.input = self.password
        return self.run("login", *args, **kargs)
    
    def run_password( self, oldpass, newpass, *args, **kargs ):
        "Simple interface to allow setting of the password"
        if( oldpass and len(oldpass) > 0 ):
            self.input = [ oldpass, newpass, newpass ]
        else:
            self.input = [ newpass, newpass ]
        
        try:
            return self.run( "password" , *args, **kargs)
        except P4Exception as e:
            if self.errors and self.errors[0] == "Passwords don't match.":
                raise P4Exception("Password invalid.")


    #
    # run_filelog: convert "p4 filelog" responses into objects with useful
    #              methods
    #
    # Requires tagged output to be of any real use. If tagged output it not
    # enabled then you just get the raw data back
    #
    def run_filelog( self, *args, **kargs ):
        kargs["resultLogging"] = False
        raw = self.run( 'filelog', args, **kargs )
        if (not self.tagged or not raw):
            # untagged mode returns simple strings, which breaks the code below
            # raw could be None if a handler is used
            return raw
        result = []
        for h in raw:
            df = None
            if isinstance( h, dict ):
                df = processFilelog( h )
            else:
                df = h
            result.append( df )
        
        # if logger is in the kargs, P4.run has changed its context but already reset it
        # so we need to pull out the logger again for this special case
        # run_filelog has its own result debug output because of the post-processing step
        
        logger = self.logger
        if "logger" in kargs:
            logger = kargs["logger"]
        
        if logger:
            output = "\n\n".join([ str(x) for x in result ])
            logger.debug(output)
        
        return result

    def run_print(self, *args, **kargs):
        kargs["resultLogging"] = False
        raw = self.run('print', args, **kargs)

        logger = self.logger
        if "logger" in kargs:
            logger = kargs["logger"]

        result = []
        if raw:
            debugResult = []
            for line in raw:
                if isinstance(line, dict):
                    result.append(line)
                    if logger:
                        debugResult.append(line)
                    result.append("")
                else:
                    # to support encoding for Python 3, we have to do a little dance
                    # we cannot add bytes to the str "", but we expect that all lines
                    # are either str or bytes. So if we encounter bytes, we replace the content
                    try:
                        result[-1] += line
                    except TypeError:
                        if type(line) == bytes and type(result[-1]) == str and result[-1] == "":
                            result[-1] = line
                        else:
                            raise
            if logger:
                logger.debug(debugResult)
            return result
        else:
            return []

    def run_resolve(self, *args, **kargs):
        if self.resolver:
            myResolver = self.resolver
        else:
            myResolver = Resolver()
        if "resolver" in kargs:
            myResolver = kargs["resolver"]
        
        savedResolver = self.resolver
        self.resolver = myResolver
        result = self.run("resolve", args)
        self.resolver = savedResolver
        
        return result

    def run_tickets(self, *args):
        fname = self.ticket_file
        with open(fname) as f:
            tickets_raw = f.readlines()
        pattern = re.compile('([^=]*)=(.*):([^:]*)\n')
        tickets = [ pattern.match(x).groups() for x in tickets_raw ]
        keys = [ "Host", "User", "Ticket" ]
        result = [ dict(zip(keys, x)) for x in tickets ]
        return result
    
    def run_init(self, *args, **kargs):
        raise Exception("Please run P4.init() instead")
        
    def run_clone(self, *args, **kargs):
        raise Exception("Please run P4.clone) instead")
    
    def __flatten(self, args):
        result = []
        if isinstance(args, tuple) or isinstance(args, list):
            for i in args:
                result.extend(self.__flatten(i))
        else:
            result.append(args)
        return tuple(result)

    def __enter__( self ):
        return self
    
    def __exit__( self, exc_type, exc_val, exc_tb ):
        if self.connected():
            self.disconnect()
        return False
    
    def connect( self ):
        P4API.P4Adapter.connect( self )
        return self
    
    def is_ignored( self, path ):
        return P4API.P4Adapter.is_ignored( self, os.path.abspath(path) )
    
    @contextmanager
    def while_tagged( self, t ):
        old = self.tagged
        self.tagged = t
        try:
            yield
        finally:
            self.tagged = old
    
    @contextmanager
    def at_exception_level( self, e ):
        old = self.exception_level
        self.exception_level = e
        try:
            yield
        finally:
            self.exception_level = old
    
    @contextmanager
    def using_handler( self, c ):
        old = self.handler
        self.handler = c
        try:
            yield
        finally:
            self.handler = old
    
    @contextmanager
    def saved_context( self , **kargs):
        """Saves the context of this p4 object and restores it again at the end of the block"""
        
        saved_context = {}
        for attr in self.__members__:
            saved_context[attr] = getattr(self, attr)
        
        for (k,v) in list(kargs.items()):
            setattr( self, k, v)
        
        try:
            yield
        finally:
            # now restore the context again. Ignore AttributeError exception
            # Exception is expected because some attributes only have getters, no setters
            
            for (k,v) in list(saved_context.items()):
                if k not in ("port", "track"):
                    try:
                        setattr( self, k, v )
                    except AttributeError:
                        pass # expected for server_level and p4config_file

    @contextmanager
    def temp_client( self, prefix, template ):
        """Creates a temporary workspace with a temporary root. 
            To be used with the "with" statement. Will clean up the temporary root and client
            workspace after the block has finished.
            The prefix is prepended to the workspace name and should be used in conjunction with
            the SpecMap of a spec depot to avoid creating entries there.
        """

        name = "{prefix}_{id}".format(prefix=prefix, id=str(uuid.uuid1()))
        ws = self.fetch_client('-t', template, name)

        try:
            root = tempfile.mkdtemp(prefix=prefix)
            ws._root = root
            self.save_client(ws)

            oldName = self.client
            self.client = name
            
            oldCwd = self.cwd
            self.cwd = root
            
            yield ws

            self.cwd = oldCwd
            self.client = oldName
            
        finally:
            self.delete_client(name)
            shutil.rmtree(root)

class Map(P4API.P4Map):
    def __init__(self, *args):
        P4API.P4Map.__init__(self, *args)
        if len(args) > 0:
            self.insert( *args )
    
    LEFT2RIGHT = True
    RIGHT2LEFT = False
    
    def __str__( self ):
        result = ""
        
        for a in self.as_array():
            result += a + "\n"
        
        return result
    
    def is_empty(self):
        """Returns True if this map has no entries yet, otherwise False"""
        
        return self.count() == 0
    
    def includes(self, *args):
        return self.translate(*args) != None
    
    def reverse(self):
        return Map(P4API.P4Map.reverse(self).as_array())
    
    def insert(self, *args):
        """Insert an argument to the map. The argument can be:
            
            A String,
            Either of the form "[+-]//lhs/... //rhs/..." or "[+-]//lhs/..."
            for label style maps.
            A List:
            This is a list of strings of one of the single string formats
            described above.
            A pair of Strings:
            P4.Map.insert(lhs, rhs)
            """
        
        if len(args) == 1 :
            arg = args[0]
            if isinstance( arg, str ):
                P4API.P4Map.insert( self, arg )
            elif isinstance( arg, list ):
                for s in arg:
                    P4API.P4Map.insert( self, s )
        
        else: # expecting 2 args in this case: left, right
            left = args[0].strip()
            right = args[1].strip()
            P4API.P4Map.insert(self, left, right )


def init(*args, **kargs):  
    keywords = ("user", "client", "directory", "port", "casesensitive", "unicode")
    
    new_kargs = dict((x,kargs[x]) for x in kargs if x in keywords)

    result = P4API.dvcs_init(*args, **new_kargs)
    
    return __dvcs_post_process(result, *args, **kargs)

def clone(*args, **kargs):
    keywords = ("user", "client", "directory", "depth", "verbose", "port", "remote", "file", "noarchive", "progress")

    new_kargs = dict((x,kargs[x]) for x in kargs if x in keywords)
    
    result = P4API.dvcs_clone(*args, **new_kargs)

    return __dvcs_post_process(result, *args, **kargs)

def __dvcs_post_process(result, *args, **kargs):
    
    excluded = ("directory", "unicode", "casesensitive", "depth", "verbose", "port", "remote", "file", "noarchive", "progress" )
    
    new_kargs = dict((x,kargs[x]) for x in kargs if x not in excluded)
    new_kargs["cwd"] = os.getcwd() # this is a hack for now, the dvcs commands do not set the environment variable CWD
    
    p4 = P4(**new_kargs)
    p4.messages.extend(result)
    
    return p4

def __run_dvcs(cmd, *args, **kargs):
    __check_paths()
    options = []
    
    def add_option(options, name, opt):
        if name in kargs:
            options += [opt, kargs[name]]
    
    add_option(options, "client","-c")
    add_option(options, "directory","-d")
    add_option(options, "user", "-u")
        
    named_args = []
    
    if "unicode" in kargs:
        unicode = kargs["unicode"]
        del kargs["unicode"]
        if unicode:
            named_args.append("-xi")
        else:
            named_args.append("-n")
    
    if "casesensitive" in kargs:
        casesensitive = kargs["casesensitive"]
        del kargs["casesensitive"]
        if casesensitive:
            named_args.append("-C1")
        else:
            named_args.append("-C0")
    
    arguments = [ "p4" ]
    arguments += options
    arguments += [ cmd ]
    arguments += named_args 
    for a in args:
        arguments += a
    
    p = subprocess.Popen(arguments, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (sout,serr) = (p.stdout, p.stderr)
    results = sout.read()
    errors = serr.read()
    sout.close()
    serr.close()
    
    if errors:
        if type(errors) == bytes:
            errors = errors.decode('UTF-8')
        errors = errors.strip()
        
        raise Exception("Cmd '{}' raised\n{}".format(" ".join(arguments), errors))
    
    if type(results) == bytes:
        results = results.decode('UTF-8')
    
    # this is where we analyze the outcome of the command
    # move into the resulting directory (if specified)
    # then create a P4 object and return that (connected??)
    
    results = results.strip()
    
    if "directory" in kargs:
        path = os.path.abspath(kargs["directory"])
        os.chdir(path)
        os.environ["PWD"] = path
        
    # create a copy of kargs without the "directory" key - make it compatible with Python 2.6
    # new_kargs = { x:kargs[x] for x in kargs if x != "directory" }
    new_kargs = dict((x,kargs[x]) for x in kargs if x not in ("directory", "unicode", "casesensitive" ))
    
    return P4(**new_kargs)
    
def __check_paths():
    if not __exec_exists("p4"):
        raise Exception("P4 executable not in path")
    if not __exec_exists("p4d"):
        raise Exception("P4D executable not in path")
    
def __exec_exists(name):
    execName = name
    if platform.system() == "Windows":
        execName += ".exe"
    
    for p in os.environ["PATH"].split(os.pathsep):
        pathToFile = os.path.join(p, execName)
        if os.path.exists(pathToFile) and os.access(pathToFile, os.X_OK):
            return __check_version(pathToFile)
    return False
    
def __check_version(pathToFile):
    p = subprocess.Popen([pathToFile,'-V'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (sout,serr) = (p.stdout, p.stderr)
    
    output = sout.read()
    error = serr.read()

    sout.close()
    serr.close()
    
    if type(output) == bytes: # Python 3 output is bytes, not str
        output = output.decode('UTF-8')
    chunks = output.split(os.linesep)
    
    pattern = re.compile("Rev. (?P<Program>.+)/(?P<Platform>.+)/(?P<Release>.+)/(?P<Patch>\d+) \((\d+/\d+/\d+)\).")
    
    for c in chunks:
        match = pattern.match(c)
        if match:
            version = match.group('Release')
            year = int(version.split('.')[0])
            if year >= 2015:
                return True
            else:
                program = match.group('Program')
                raise Exception("{} must be at least 2015.1, not {}".format(program, version))

    raise Exception("Unknown P4 output : {}".format(output) )

if __name__ == "__main__":
    p4 = P4()
    p4.connect()
    try:
        ret = p4.run(sys.argv[1:])
        for line in ret:
            if isinstance(line, dict):
                print("-----")
                for k in list(line.keys()):
                    print(k, "=", line[k])
            else:
                print(line)
    except:
        for e in p4.errors:
            print(e)
