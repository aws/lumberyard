#!/usr/bin/env python
# encoding: utf-8
# Thomas Nagy, 2005-2010 (ita)

"""
logging, colors, terminal width and pretty-print
"""

import os, re, traceback, sys

class IWafLogHandler(object):		
	def write_msg(self, msg):
		pass
		
	def write_err(self,msg):
		pass

external_log_handlers = {}

def set_external_log_handler(name, handler):
	global external_log_handlers
	external_log_handlers[name] = handler

def remove_external_log_handler(name):
	global external_log_handlers
	external_log_handlers.pop(name, None)
	
_nocolor = os.environ.get('NOCOLOR', 'no') not in ('no', '0', 'false')
try:
	if not _nocolor:
		import waflib.ansiterm
except ImportError:
	pass

try:
	import threading
except ImportError:
	if not 'JOBS' in os.environ:
		# no threading :-(
		os.environ['JOBS'] = '1'
else:
	wlock = threading.Lock()

	class sync_stream(object):
		def __init__(self, stream):
			self.stream = stream
			self.encoding = self.stream.encoding

		def write(self, txt):
			try:
				wlock.acquire()
				self.stream.write(txt)
				self.stream.flush()
				
				for key, value in external_log_handlers.iteritems():
					value.write_msg(txt)
					
			finally:
				wlock.release()

		def fileno(self):
			return self.stream.fileno()

		def flush(self):
			self.stream.flush()

		def isatty(self):
			return self.stream.isatty()

	if not os.environ.get('NOSYNC', False):
		if id(sys.stdout) == id(sys.__stdout__):
			sys.stdout = sync_stream(sys.stdout)
			sys.stderr = sync_stream(sys.stderr)

import logging # import other modules only after

LOG_FORMAT = "%(asctime)s %(c1)s%(zone)s%(c2)s %(message)s"
HOUR_FORMAT = "%H:%M:%S"

zones = ''
verbose = 0
sig_delta = False

colors_lst = {
'USE' : True,
'BOLD'  :'\x1b[01;1m',
'RED'   :'\x1b[01;31m',
'GREEN' :'\x1b[32m',
'YELLOW':'\x1b[33m',
'PINK'  :'\x1b[35m',
'BLUE'  :'\x1b[01;34m',
'CYAN'  :'\x1b[36m',
'NORMAL':'\x1b[0m',
'cursor_on'  :'\x1b[?25h',
'cursor_off' :'\x1b[?25l',
}

got_tty = not os.environ.get('TERM', 'dumb') in ['dumb', 'emacs']
if got_tty:
	try:
		got_tty = sys.stderr.isatty() and sys.stdout.isatty()
	except AttributeError:
		got_tty = False

if (not got_tty and os.environ.get('TERM', 'dumb') != 'msys') or _nocolor:
	colors_lst['USE'] = False

def get_term_cols():
	return 80

# If console packages are available, replace the dummy function with a real
# implementation
try:
	import struct, fcntl, termios
except ImportError:
	pass
else:
	if got_tty:
		def get_term_cols_real():
			"""
			Private use only.
			"""

			dummy_lines, cols = struct.unpack("HHHH", \
			fcntl.ioctl(sys.stderr.fileno(),termios.TIOCGWINSZ , \
			struct.pack("HHHH", 0, 0, 0, 0)))[:2]
			return cols
		# try the function once to see if it really works
		try:
			get_term_cols_real()
		except Exception:
			pass
		else:
			get_term_cols = get_term_cols_real

get_term_cols.__doc__ = """
	Get the console width in characters.

	:return: the number of characters per line
	:rtype: int
	"""

def get_color(cl):
	if not colors_lst['USE']: return ''
	return colors_lst.get(cl, '')

class color_dict(object):
	"""attribute-based color access, eg: colors.PINK"""
	def __getattr__(self, a):
		return get_color(a)
	def __call__(self, a):
		return get_color(a)

colors = color_dict()

re_log = re.compile(r'(\w+): (.*)', re.M)
class log_filter(logging.Filter):
	"""
	The waf logs are of the form 'name: message', and can be filtered by 'waf --zones=name'.
	For example, the following::

		from waflib import Logs
		Logs.debug('test: here is a message')

	Will be displayed only when executing::

		$ waf --zones=test
	"""
	def __init__(self, name=None):
		pass

	def filter(self, rec):
		"""
		filter a record, adding the colors automatically

		* error: red
		* warning: yellow

		:param rec: message to record
		"""

		rec.c1 = colors.PINK
		rec.c2 = colors.NORMAL
		rec.zone = rec.module
		if rec.levelno >= logging.INFO:
			if rec.levelno >= logging.ERROR:
				rec.c1 = colors.RED
			elif rec.levelno >= logging.WARNING:
				rec.c1 = colors.YELLOW
			else:
				rec.c1 = colors.GREEN
			return True

		m = re_log.match(rec.msg)
		if m:
			rec.zone = m.group(1)
			rec.msg = m.group(2)

		if zones:
			return getattr(rec, 'zone', '') in zones or '*' in zones
		elif not verbose > 2:
			return False
		return True

class formatter(logging.Formatter):
	"""Simple log formatter which handles colors"""
	def __init__(self):
		logging.Formatter.__init__(self, LOG_FORMAT, HOUR_FORMAT)

	def format(self, rec):
		"""Messages in warning, error or info mode are displayed in color by default"""
		if rec.levelno >= logging.WARNING or rec.levelno == logging.INFO:
			try:
				msg = rec.msg.decode('utf-8')
			except Exception:
				msg = rec.msg
			return '%s%s%s' % (rec.c1, msg, rec.c2)
		return logging.Formatter.format(self, rec)

log = None
"""global logger for Logs.debug, Logs.error, etc"""

def debug(*k, **kw):
	"""
	Wrap logging.debug, the output is filtered for performance reasons
	"""
	if verbose:
		k = list(k)
		k[0] = k[0].replace('\n', ' ')
		global log
		log.debug(*k, **kw)

def error(*k, **kw):
	"""
	Wrap logging.errors, display the origin of the message when '-vv' is set
	"""
	global log
	log.error(*k, **kw)
	if verbose > 2:
		st = traceback.extract_stack()
		if st:
			st = st[:-1]
			buf = []
			for filename, lineno, name, line in st:
				buf.append('  File "%s", line %d, in %s' % (filename, lineno, name))
				if line:
					buf.append('	%s' % line.strip())
			if buf: log.error("\n".join(buf))

def warn(*k, **kw):
	"""
	Wrap logging.warn
	"""
	global log
	log.warn(*k, **kw)

def info(*k, **kw):
	"""
	Wrap logging.info
	"""
	global log
	log.info(*k, **kw)

def init_log():
	"""
	Initialize the loggers globally
	"""
	global log
	log = logging.getLogger('waflib')
	log.handlers = []
	log.filters = []
	hdlr = logging.StreamHandler()
	hdlr.setFormatter(formatter())
	log.addHandler(hdlr)
	log.addFilter(log_filter())
	log.setLevel(logging.DEBUG)

def make_logger(path, name):
	"""
	Create a simple logger, which is often used to redirect the context command output::

		from waflib import Logs
		bld.logger = Logs.make_logger('test.log', 'build')
		bld.check(header_name='sadlib.h', features='cxx cprogram', mandatory=False)
		bld.logger = None

	:param path: file name to write the log output to
	:type path: string
	:param name: logger name (loggers are reused)
	:type name: string
	"""
	logger = logging.getLogger(name)
	hdlr = logging.FileHandler(path, 'w')
	formatter = logging.Formatter('%(message)s')
	hdlr.setFormatter(formatter)
	logger.addHandler(hdlr)
	logger.setLevel(logging.DEBUG)
	return logger

def make_mem_logger(name, to_log, size=10000):
	"""
	Create a memory logger to avoid writing concurrently to the main logger
	"""
	from logging.handlers import MemoryHandler
	logger = logging.getLogger(name)
	hdlr = MemoryHandler(size, target=to_log)
	formatter = logging.Formatter('%(message)s')
	hdlr.setFormatter(formatter)
	logger.addHandler(hdlr)
	logger.memhandler = hdlr
	logger.setLevel(logging.DEBUG)
	return logger

def pprint(col, str, label='', sep='\n'):
	"""
	Print messages in color immediately on stderr::

		from waflib import Logs
		Logs.pprint('RED', 'Something bad just happened')

	:param col: color name to use in :py:const:`Logs.colors_lst`
	:type col: string
	:param str: message to display
	:type str: string or a value that can be printed by %s
	:param label: a message to add after the colored output
	:type label: string
	:param sep: a string to append at the end (line separator)
	:type sep: string
	"""
	sys.stderr.write("%s%s%s %s%s" % (colors(col), str, colors.NORMAL, label, sep))
	
	for key, value in external_log_handlers.iteritems():
		value.write_err("%s%s%s %s%s" % (colors(col), str, colors.NORMAL, label, sep))


WARNED_MESSAGES = set()


def warn_once(msg):
	if msg not in WARNED_MESSAGES:
		WARNED_MESSAGES.add(msg)
		warn(msg)
	
	
INFO_MESSAGES = set()


def info_once(msg):
	if msg not in INFO_MESSAGES:
		INFO_MESSAGES.add(msg)
		info(msg)
