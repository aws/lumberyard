from multiprocessing import Process, Queue
# import traceback


def _wrapper(target, codec, queue, args, kwargs):
    exc = None
    try:
        if args is None:
            args = []
        if kwargs is None:
            kwargs = {}
        r = target(*args, **kwargs)
    except Exception as e:
#         traceback.print_exc()
        r = None
        exc = e

    if codec:
        r = codec[0](r)

    queue.put((exc, r))


def run_in_childprocess(target, codec=None, *args, **kwargs):
    assert codec is None or len(codec) == 2, codec
    queue = Queue()
    p = Process(target=_wrapper, args=(target, codec, queue,  args, kwargs))
    p.start()
    e, r = queue.get()
    p.join()

    if e:
        raise e

    if codec:
        r = codec[1](r)

    return r
