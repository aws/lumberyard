#! /usr/bin/env python
# encoding: utf-8
# Thomas Nagy, 2011 (ita)

"""
A client for the network cache (playground/netcache/). Launch the server with:
./netcache_server, then use it for the builds by adding the following:

	def options(opt):
		opt.load('netcache_client')

The parameters should be present in the environment in the form:
	NETCACHE=host:port@mode waf configure build

where:
	mode: PUSH, PULL, PUSH_PULL
	host: host where the server resides, for example 127.0.0.1
	port: by default the server runs on port 51200

The cache can be enabled for the build only:
	def options(opt):
		opt.load('netcache_client', funs=[])
	def build(bld):
		bld.setup_netcache('localhost', 51200, 'PUSH_PULL')
"""

import os, socket, time, atexit
from waflib import Task, Logs, Utils, Build, Options, Runner
from waflib.Configure import conf

BUF = 8192 * 16
HEADER_SIZE = 128
MODES = ['PUSH', 'PULL', 'PUSH_PULL']
STALE_TIME = 30 # seconds

GET = 'GET'
PUT = 'PUT'
LST = 'LST'
BYE = 'BYE'

all_sigs_in_cache = (0.0, [])

active_connections = Runner.Queue(0)
def get_connection():
	# return a new connection... do not forget to release it!
	try:
		ret = active_connections.get(block=False)
	except Exception:
		ret = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		ret.connect(Task.net_cache[:2])
	return ret

def release_connection(conn, msg=''):
	if conn:
		active_connections.put(conn)

def close_connection(conn, msg=''):
	if conn:
		data = '%s,%s' % (BYE, msg)
		try:
			conn.send(data.ljust(HEADER_SIZE))
		except:
			pass
		try:
			conn.close()
		except:
			pass

def close_all():
	while active_connections.qsize():
		conn = active_connections.get()
		try:
			close_connection(conn)
		except:
			pass
atexit.register(close_all)

def read_header(conn):
	cnt = 0
	buf = []
	while cnt < HEADER_SIZE:
		data = conn.recv(HEADER_SIZE - cnt)
		if not data:
			#import traceback
			#traceback.print_stack()
			raise ValueError('connection ended when reading a header %r' % buf)
		buf.append(data)
		cnt += len(data)
	return ''.join(buf)

def check_cache(conn, ssig):
	"""
	List the files on the server, this is an optimization because it assumes that
	concurrent builds are rare
	"""
	global all_sigs_in_cache
	if not STALE_TIME:
		return
	if time.time() - all_sigs_in_cache[0] > STALE_TIME:

		params = (LST,'')
		conn.send(','.join(params).ljust(HEADER_SIZE))

		# read what is coming back
		ret = read_header(conn)
		size = int(ret.split(',')[0])

		buf = []
		cnt = 0
		while cnt < size:
			data = conn.recv(min(BUF, size-cnt))
			if not data:
				raise ValueError('connection ended %r %r' % (cnt, size))
			buf.append(data)
			cnt += len(data)
		all_sigs_in_cache = (time.time(), ''.join(buf).split('\n'))
		Logs.debug('netcache: server cache has %r entries' % len(all_sigs_in_cache[1]))

	if not ssig in all_sigs_in_cache[1]:
		raise ValueError('no file %s in cache' % ssig)

class MissingFile(Exception):
	pass

def recv_file(conn, ssig, count, p):
	check_cache(conn, ssig)

	params = (GET, ssig, str(count))
	conn.send(','.join(params).ljust(HEADER_SIZE))
	data = read_header(conn)

	size = int(data.split(',')[0])

	if size == -1:
		raise MissingFile('no file %s - %s in cache' % (ssig, count))

	# get the file, writing immediately
	# TODO a tmp file would be better
	f = open(p, 'wb')
	cnt = 0
	while cnt < size:
		data = conn.recv(min(BUF, size-cnt))
		if not data:
			raise ValueError('connection ended %r %r' % (cnt, size))
		f.write(data)
		cnt += len(data)
	f.close()

def put_data(conn, ssig, cnt, p):
	#print "pushing %r %r %r" % (ssig, cnt, p)
	size = os.stat(p).st_size
	params = (PUT, ssig, str(cnt), str(size))
	conn.send(','.join(params).ljust(HEADER_SIZE))
	f = open(p, 'rb')
	cnt = 0
	while cnt < size:
		r = f.read(min(BUF, size-cnt))
		while r:
			k = conn.send(r)
			if not k:
				raise ValueError('connection ended')
			cnt += k
			r = r[k:]

#def put_data(conn, ssig, cnt, p):
#	size = os.stat(p).st_size
#	params = (PUT, ssig, str(cnt), str(size))
#	conn.send(','.join(params).ljust(HEADER_SIZE))
#	conn.send(','*size)
#	params = (BYE, 'he')
#	conn.send(','.join(params).ljust(HEADER_SIZE))

def can_retrieve_cache(self):
	if not Task.net_cache:
		return False
	if not self.outputs:
		return False
	if Task.net_cache[-1] == 'PUSH':
		return
	self.cached = False

	cnt = 0
	sig = self.signature()
	ssig = self.uid().encode('hex') + sig.encode('hex')

	conn = None
	err = False
	try:
		try:
			conn = get_connection()
			for node in self.outputs:
				p = node.abspath()
				recv_file(conn, ssig, cnt, p)
				cnt += 1
		except MissingFile as e:
			Logs.debug('netcache: file is not in the cache %r' % e)
			err = True

		except Exception as e:
			Logs.debug('netcache: could not get the files %r' % e)
			err = True

			# broken connection? remove this one
			close_connection(conn)
			conn = None
	finally:
		release_connection(conn)
	if err:
		return False

	for node in self.outputs:
		node.sig = sig
		#if self.generator.bld.progress_bar < 1:
		#	self.generator.bld.to_log('restoring from cache %r\n' % node.abspath())

	self.cached = True
	return True

@Utils.run_once
def put_files_cache(self):
	if not Task.net_cache:
		return
	if not self.outputs:
		return
	if Task.net_cache[-1] == 'PULL':
		return
	if getattr(self, 'cached', None):
		return

	#print "called put_files_cache", id(self)
	bld = self.generator.bld
	sig = self.signature()
	ssig = self.uid().encode('hex') + sig.encode('hex')

	conn = None
	cnt = 0
	try:
		for node in self.outputs:
			# We could re-create the signature of the task with the signature of the outputs
			# in practice, this means hashing the output files
			# this is unnecessary
			try:
				if not conn:
					conn = get_connection()
				put_data(conn, ssig, cnt, node.abspath())
			except Exception as e:
				Logs.debug("netcache: could not push the files %r" % e)

				# broken connection? remove this one
				close_connection(conn)
				conn = None
			cnt += 1
	finally:
		release_connection(conn)

	bld.task_sigs[self.uid()] = self.cache_sig

def hash_env_vars(self, env, vars_lst):
	if not env.table:
		env = env.parent
		if not env:
			return Utils.SIG_NIL

	idx = str(id(env)) + str(vars_lst)
	try:
		cache = self.cache_env
	except AttributeError:
		cache = self.cache_env = {}
	else:
		try:
			return self.cache_env[idx]
		except KeyError:
			pass

	v = str([env[a] for a in vars_lst])
	v = v.replace(self.srcnode.abspath().__repr__()[:-1], '')
	m = Utils.md5()
	m.update(v.encode())
	ret = m.digest()

	Logs.debug('envhash: %r %r', ret, v)

	cache[idx] = ret

	return ret

def uid(self):
	try:
		return self.uid_
	except AttributeError:
		m = Utils.md5()
		src = self.generator.bld.srcnode
		up = m.update
		up(self.__class__.__name__.encode())
		for x in self.inputs + self.outputs:
			up(x.path_from(src).encode())
		self.uid_ = m.digest()
		return self.uid_

@conf
def setup_netcache(ctx, host, port, mode):
	Logs.warn('Using the network cache %s, %s, %s' % (host, port, mode))
	Task.net_cache = (host, port, mode)
	Task.Task.can_retrieve_cache = can_retrieve_cache
	Task.Task.put_files_cache = put_files_cache
	Task.Task.uid = uid
	Build.BuildContext.hash_env_vars = hash_env_vars
	ctx.cache_global = Options.cache_global = True

def options(opt):
	if not 'NETCACHE' in os.environ:
		Logs.warn('the network cache is disabled, set NETCACHE=host:port@mode to enable')
	else:
		v = os.environ['NETCACHE']
		if v in MODES:
			host = socket.gethostname()
			port = 51200
			mode = v
		else:
			mode = 'PUSH_PULL'
			host, port = v.split(':')
			if port.find('@'):
				port, mode = port.split('@')
			port = int(port)
			if not mode in MODES:
				opt.fatal('Invalid mode %s not in %r' % (mode, MODES))
		setup_netcache(opt, host, port, mode)

