import os, json
try:
    import cPickle
except ImportError:
    import pickle as cPickle
from waflib import Build, Logs, ConfigSet, Context, Errors, Utils, Node, Task
from az_code_generator import az_code_gen

Build.SAVED_ATTRS.append('cached_engine_path')
Build.SAVED_ATTRS.append('cached_tp_root_path')


class ArtifactsCacheMetrics:
    def __init__(self):
        self.tasks_processed = set()
        self.tasks_missed = set()
        self.tasks_failed_to_retrieve = set()
        self.time_spent = 0


def options(opt):
    opt.add_option('--artifacts-cache', default='', dest='artifacts_cache')
    opt.add_option('--artifacts-cache-upload', default='False', dest='artifacts_cache_upload')
    opt.add_option('--artifacts-cache-restore', default='False', dest='artifacts_cache_restore')
    opt.add_option('--artifacts-cache-days-to-keep', default=3, dest='artifacts_cache_days_to_keep')
    opt.add_option('--artifacts-cache-wipeout', default='False', dest='artifacts_cache_wipeout')


def restore(self):
    """
    Load the data from a previous run, sets the attributes listed in :py:const:`waflib.Build.SAVED_ATTRS`
    """
    try:
        env = ConfigSet.ConfigSet(os.path.join(self.cache_dir, 'build.config.py'))
    except (IOError, OSError):
        pass
    else:
        if env['version'] < Context.HEXVERSION:
            raise Errors.WafError('Version mismatch! reconfigure the project')
        for t in env['tools']:
            self.setup(**t)

    dbfn = os.path.join(self.variant_dir, Context.DBFILE)
    Node.Nod3 = self.node_class
    local_data = None
    cache_data = None
    data = None
    try:
        local_data_str = Utils.readf(dbfn, 'rb')
        try:
            local_data = cPickle.loads(local_data_str)
        except cPickle.UnpicklingError:
            Logs.debug('build: Could not unpickle the data from local build cache {}'.format(dbfn))
    except (IOError, EOFError):
        # handle missing file/empty file
        Logs.debug('build: Could not load the local build cache {} (missing)'.format(dbfn))

    if local_data:
        data = local_data

    # If artifacts cache is enabled, try to load the artifacts cache, this ensures that the task's include dependencies can be known in advance in a clean build
    if self.artifacts_cache and self.is_option_true('artifacts_cache_restore'):
        try:
            dbfn = os.path.join(self.artifacts_cache, 'wafpickle', self.cmd, Context.DBFILE)
            cache_data_str = Utils.readf(dbfn, 'rb')
            try:
                cache_data = cPickle.loads(cache_data_str)
            except cPickle.UnpicklingError:
                Logs.debug('build: Could not unpickle the data from global build cache {}'.format(dbfn))
        except (IOError, EOFError):
            # handle missing file/empty file
            Logs.debug('build: Could not load the global build cache {} (missing)'.format(dbfn))
        if cache_data:
            if not local_data:
                data = cache_data
            else:
                merged_data = {}
                for x in local_data:
                    if x not in cache_data:
                        merged_data[x] = local_data

                for x in cache_data:
                    if x not in local_data:
                        merged_data[x] = cache_data[x]
                    else:
                        if isinstance(local_data[x], dict):
                            cache_data[x].update(local_data[x])
                            merged_data[x] = cache_data[x]
                        else:
                            merged_data[x] = local_data[x]
                data = merged_data
                data['cached_engine_path'] = cache_data['cached_engine_path']
                data['cached_tp_root_path'] = cache_data['cached_tp_root_path']
    if data:
        try:
            Node.pickle_lock.acquire()
            for x in Build.SAVED_ATTRS:
                if x in data:
                    setattr(self, x, data[x])
                else:
                    Logs.debug("build: SAVED_ATTRS key {} missing from cache".format(x))
        finally:
            Node.pickle_lock.release()

    self.init_dirs()
Build.BuildContext.restore = Utils.nogc(restore)


def store(self):
    """
    Store the data for next runs, sets the attributes listed in :py:const:`waflib.Build.SAVED_ATTRS`. Uses a temporary
    file to avoid problems on ctrl+c.
    """

    # Write ArtifactsCacheMetrics to file

    if self.artifacts_cache and self.is_option_true('artifacts_cache_restore') and getattr(self, 'artifacts_cache_metrics', False):
        json_data = {}
        json_data['tasks_processed_num'] = len(self.artifacts_cache_metrics.tasks_processed)
        json_data['tasks_missed_num'] = len(self.artifacts_cache_metrics.tasks_missed)
        json_data['tasks_failed_to_retrieve_num'] = len(self.artifacts_cache_metrics.tasks_failed_to_retrieve)
        f = os.path.join(self.variant_dir, 'ArtifactsCacheMetrics.json')
        with open(f, 'w') as output:
            json.dump(json_data, output)

        Logs.info("Total number of tasks processed by waf artifacts cache: {}\n".format(len(self.artifacts_cache_metrics.tasks_processed)) +
                   "Cache miss: {}".format(len(self.artifacts_cache_metrics.tasks_missed)))

    data = {}
    for x in Build.SAVED_ATTRS:
        data[x] = getattr(self, x, None)

    try:
        Node.pickle_lock.acquire()
        Node.Nod3 = self.node_class
        x = cPickle.dumps(data, -1)
    finally:
        Node.pickle_lock.release()

    def write_to_db(db, contents):
        Utils.writef(db + '.tmp', contents, m='wb')

        try:
            st = os.stat(db)
            os.remove(db)
            if not Utils.is_win32: # win32 has no chown but we're paranoid
                os.chown(db + '.tmp', st.st_uid, st.st_gid)
        except (AttributeError, OSError):
            pass

        # do not use shutil.move (copy is not thread-safe)
        os.rename(db + '.tmp', db)

    write_to_db(os.path.join(self.variant_dir, Context.DBFILE), x)
    # Save to artifacts cache if artifacts cache is enabled
    if self.artifacts_cache and self.is_option_true('artifacts_cache_upload'):
        x = cPickle.dumps(data, -1)
        wafpickle_dir = os.path.join(self.artifacts_cache, 'wafpickle', self.cmd)
        if not os.path.exists(wafpickle_dir):
            os.makedirs(wafpickle_dir)
        try:
            write_to_db(os.path.join(wafpickle_dir, Context.DBFILE), x)
        except Exception:
            pass
Build.BuildContext.store = Utils.nogc(store)


def replace_engine_path_and_tp_root_in_string(bld, s):
    if not hasattr(bld, 'engine_path_replaced'):
        bld.engine_path_replaced = bld.engine_path.translate(None, r'\/').lower()
    if not hasattr(bld, 'tp_root_replaced'):
        bld.tp_root_replaced = bld.tp.content.get('3rdPartyRoot').translate(None, r'\/').lower()
    s = s.translate(None, r'\/').lower()
    s = s.replace(bld.engine_path_replaced, '')
    s = s.replace(bld.tp_root_replaced, '')
    return s


def hash_env_vars(self, env, vars_lst):
    """
    Override env signature computation, and make it to be engine path and 3rdParty path independent.
    """
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

    lst = [env[x] for x in vars_lst if x not in ['INCLUDES', 'INCPATHS', 'LIBPATH', 'STLIBPATH']]
    env_str = replace_engine_path_and_tp_root_in_string(self, str(lst))
    m = Utils.md5()
    m.update(env_str.encode())
    ret = m.digest()
    Logs.debug('envhash: %s %r', Utils.to_hex(ret), lst)

    cache[idx] = ret

    return ret


def uid(self):
    """
    Override uid computation, and make it to be engine path and 3rdParty path independent
    """
    try:
        return self.uid_
    except AttributeError:
        # this is not a real hot zone, but we want to avoid surprises here
        m = Utils.md5()
        up = m.update
        up(self.__class__.__name__.encode())
        for k in self.inputs + self.outputs:
            s = replace_engine_path_and_tp_root_in_string(self.generator.bld, k.abspath().encode())
            up(s)
        self.uid_ = m.digest()
        return self.uid_


def azcg_uid(self):
    """
    Override uid computation for AzCodeGenerator, and make it to be engine path and 3rdParty path independent
    """
    try:
        return self.uid_
    except AttributeError:
        m = Utils.md5()
        up = m.update

        # Be sure to add any items here that change how code gen runs, this needs to be unique!
        # Ensure anything here will not change over the life of the task
        up(replace_engine_path_and_tp_root_in_string(self.bld, str(self.path.abspath().encode())))
        up(replace_engine_path_and_tp_root_in_string(self.bld, str(self.input_dir.abspath().encode())))
        up(replace_engine_path_and_tp_root_in_string(self.bld, str(self.output_dir.abspath().encode())))
        up(replace_engine_path_and_tp_root_in_string(self.bld, str(self.inputs)))
        #up(replace_engine_path_and_tp_root_in_string(self.bld, str(self.script_nodes)))
        up(replace_engine_path_and_tp_root_in_string(self.bld, str(self.defines)))
        up(replace_engine_path_and_tp_root_in_string(self.bld, str(self.argument_list)))
        up(replace_engine_path_and_tp_root_in_string(self.bld, str(self.azcg_deps)))

        self.uid_ = m.digest()
        return self.uid_


def can_retrieve_cache(self):
    """
    Used by :py:meth:`waflib.Task.cache_outputs`

    Retrieve build nodes from the cache
    update the file timestamps to help cleaning the least used entries from the cache
    additionally, set an attribute 'cached' to avoid re-creating the same cache files

    Suppose there are files in `cache/dir1/file1` and `cache/dir2/file2`:

    #. read the timestamp of dir1
    #. try to copy the files
    #. look at the timestamp again, if it has changed, the data may have been corrupt (cache update by another process)
    #. should an exception occur, ignore the data
    """
    bld = self.generator.bld
    if not getattr(self, 'outputs', None):
        return False

    if not hasattr(self, 'can_retrieve_cache_checked'):
        self.can_retrieve_cache_checked = True
    else:
        return False

    bld.artifacts_cache_metrics.tasks_processed.add(self)

    sig = self.signature()
    ssig = Utils.to_hex(self.uid()) + Utils.to_hex(sig)

    # first try to access the cache folder for the task
    dname = os.path.join(bld.artifacts_cache, ssig)
    if not os.path.exists(dname):
        bld.artifacts_cache_metrics.tasks_missed.add(self)
        return False

    for node in self.outputs:
        orig = os.path.join(dname, node.name)
        # Maximum Path Length Limitation on Windows is 260 characters, starting from Windows 10, we can enable long path to remove this limitation.
        # In case long path is not enabled, extended-length path to bypass this limitation.
        orig = Utils.extended_path(orig)
        try:
            t1 = os.stat(orig).st_mtime
        except OSError:
            bld.artifacts_cache_metrics.tasks_missed.add(self)
            return False
        dir_name = os.path.dirname(node.abspath())
        try:
            os.makedirs(dir_name)
        except Exception:
            pass

        try:
            # Do not use shutil.copy2(orig, node.abspath()), otherwise, it will cause threading issue with compiler and linker.
            # shutil.copy2() first calls shutil.copyfile() to copy the file contents, and then calls os.copystat() to copy the file stats, after the file contents are copied, waf is able to get the node's signature and might think the runnable status of a task is ready to run, but the copied file is then opened by os.copystat(), and compiler or linker who use the copied file as input file will fail.
            if Utils.is_win32:
                os.system('copy {} {} /Y>nul'.format(orig, node.abspath()))
            else:
                os.system('cp {} {}'.format(orig, node.abspath()))
            # is it the same file?
            try:
                t2 = os.stat(orig).st_mtime
                if t1 != t2:
                    bld.artifacts_cache_metrics.tasks_failed_to_retrieve.add(self)
                    return False
            except OSError:
                bld.artifacts_cache_metrics.tasks_failed_to_retrieve.add(self)
                return False
        except Exception as e:
            Logs.warn('[WARN] task: failed retrieving file {} due to exception\n{}\n'.format(node.abspath(), e))
            bld.artifacts_cache_metrics.tasks_failed_to_retrieve.add(self)
            return False

    for node in self.outputs:
        node.sig = sig
        if bld.progress_bar < 1:
            bld.to_log('restoring from cache %r\n' % node.abspath())

    # mark the cache file folder as used recently (modified)
    os.utime(dname, None)

    self.cached = True
    return True


def put_files_cache(self):
    """
    Used by :py:func:`waflib.Task.cache_outputs` to store the build files in the cache
    """

    # file caching, if possible
    # try to avoid data corruption as much as possible
    if hasattr(self, 'cached'):
        return

    sig = self.signature()
    ssig = Utils.to_hex(self.uid()) + Utils.to_hex(sig)
    dname = os.path.join(self.generator.bld.artifacts_cache, ssig)
    dname_tmp = dname + '_tmp'

    if os.path.exists(dname) or os.path.exists(dname_tmp):
        return
    else:
        try:
            os.makedirs(dname_tmp)
        except OSError:
            return

    for node in self.outputs:
        dest = os.path.join(dname_tmp, node.name)
        dest = Utils.extended_path(dest)
        try:
            if Utils.is_win32:
                os.system('copy {} {} /Y>nul'.format(node.abspath(), dest))
            else:
                os.system('cp -f {} {}'.format(node.abspath(), dest))
        except Exception as e:
            Logs.warn('[WARN] task: failed caching file {} due to exception\n {}\n'.format(dest, e))
            return

    try:
        os.utime(dname_tmp, None)
        os.chmod(dname_tmp, Utils.O755)
        if Utils.is_win32:
            # For rename in Windows, target path cannot be full path, it will remain in the same folder with source path
            os.system('rename {} {}'.format(dname_tmp, ssig))
        else:
            os.system('mv {} {}'.format(dname_tmp, dname))
    except Exception as e:
        Logs.warn('[WARN] task: failed updating timestamp/permission for cached outputs folder for task signature {} due to exception\n {}\n'.format(dname, e))
        pass

    self.cached = True


def replace_nodes(ctx, data):
    """
    Replace nodes stored in wafpickle file with new nodes created in current build directory.
    """
    cached_engine_path = getattr(ctx, 'cached_engine_path', None)
    cached_tp_root_path = getattr(ctx, 'cached_tp_root_path', None)
    if cached_engine_path and cached_tp_root_path:
        cached_engine_path = cached_engine_path.lower()
        cached_tp_root_path = cached_tp_root_path.lower()
        if isinstance(data, list):
            new_list = [replace_nodes(ctx, x) for x in data]
            return new_list
        elif isinstance(data, dict):
            new_dict = {k:replace_nodes(ctx, v) for (k, v) in data.items()}
            return new_dict
        elif isinstance(data, Node.Node):
            cache_abspath = data.abspath()
            if cache_abspath:
                cache_abspath = os.path.normpath(cache_abspath)
                cached_engine_path_len = len(cached_engine_path)
                cached_tp_root_path_len = len(cached_tp_root_path)
                if cache_abspath[:cached_engine_path_len].lower() == cached_engine_path:
                    relpath = cache_abspath[cached_engine_path_len + 1:]
                    new_node = ctx.engine_node.make_node(relpath)
                    try:
                        new_node.sig = data.sig
                    except AttributeError:
                        pass
                    return new_node
                elif cache_abspath[:cached_tp_root_path_len].lower() == cached_tp_root_path:
                    relpath = cache_abspath[cached_tp_root_path_len + 1:]
                    new_node = ctx.tp.third_party_root.make_node(relpath)
                    try:
                        new_node.sig = data.sig
                    except AttributeError:
                        pass
                    return new_node
                else:
                    return data
        elif isinstance(data, str):
            d = os.path.normpath(data)
            cached_engine_path_len = len(cached_engine_path)
            cached_tp_root_path_len = len(cached_tp_root_path)
            if d[:cached_engine_path_len].lower() == cached_engine_path:
                return os.path.normpath(os.path.join(ctx.engine_path, d[cached_engine_path_len + 1:]))
            elif d[:cached_tp_root_path_len].lower() == cached_tp_root_path:
                return os.path.normpath(os.path.join(ctx.tp.content.get('3rdPartyRoot'), d[cached_tp_root_path_len + 1:]))
    return data


def build(ctx):
    if ctx.artifacts_cache:
        if not os.path.isdir(ctx.artifacts_cache):
            try:
                os.makedirs(ctx.artifacts_cache)
            except Exception as e:
                ctx.warn_once('Cannot create directory {} for artifacts cache. Artifacts cache will not be used.\n{}'.format(ctx.artifacts_cache, e))
                ctx.artifacts_cache = ""
                return

        if ctx.is_option_true('artifacts_cache_restore'):
            if getattr(ctx, 'cached_engine_path', '') != ctx.engine_path or getattr(ctx, 'cached_tp_root_path', '') != ctx.tp.content.get('3rdPartyRoot'):
                ctx.tp.third_party_root = ctx.root.make_node(ctx.tp.content.get('3rdPartyRoot'))
                for x in Build.SAVED_ATTRS:
                    data = getattr(ctx, x, None)
                    if data:
                        d = replace_nodes(ctx, data)
                        setattr(ctx, x, d)
            Task.Task.can_retrieve_cache = can_retrieve_cache
            ctx.artifacts_cache_metrics = ArtifactsCacheMetrics()
            Task.Task.uid = uid
            az_code_gen.uid = azcg_uid
            Build.BuildContext.hash_env_vars = hash_env_vars

        if ctx.is_option_true('artifacts_cache_upload'):
            ctx.cached_engine_path = ctx.engine_path
            ctx.cached_tp_root_path = ctx.tp.content.get('3rdPartyRoot')
            Task.Task.put_files_cache = put_files_cache
            Task.Task.uid = uid
            az_code_gen.uid = azcg_uid
            Build.BuildContext.hash_env_vars = hash_env_vars

