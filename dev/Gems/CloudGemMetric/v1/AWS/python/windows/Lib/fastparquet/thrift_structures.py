import io
import sys
import types
from thrift.protocol.TCompactProtocol import TCompactProtocolAccelerated as TCompactProtocol
from thrift.protocol.TProtocol import TProtocolException

from .parquet_thrift.parquet import ttypes as parquet_thrift
from .util import ParquetException


def read_thrift(file_obj, ttype):
    """Read a thrift structure from the given fo."""
    from thrift.transport.TTransport import TFileObjectTransport, TBufferedTransport
    starting_pos = file_obj.tell()

    # set up the protocol chain
    ft = TFileObjectTransport(file_obj)
    bufsize = 2 ** 16
    # for accelerated reading ensure that we wrap this so that the CReadable transport can be used.
    bt = TBufferedTransport(ft, bufsize)
    pin = TCompactProtocol(bt)

    # read out type
    obj = ttype()
    obj.read(pin)

    # The read will actually overshoot due to the buffering that thrift does. Seek backwards to the correct spot,.
    buffer_pos = bt.cstringio_buf.tell()
    ending_pos = file_obj.tell()
    blocks = ((ending_pos - starting_pos) // bufsize) - 1
    if blocks < 0:
        blocks = 0
    file_obj.seek(starting_pos + blocks * bufsize + buffer_pos)
    return obj


def write_thrift(fobj, thrift):
    """Write binary compact representation of thiftpy structured object

    Parameters
    ----------
    fobj: open file-like object (binary mode)
    thrift: thriftpy object to write

    Returns
    -------
    Number of bytes written
    """
    t0 = fobj.tell()
    pout = TCompactProtocol(fobj)
    try:
        thrift.write(pout)
        fail = False
    except TProtocolException as e:
        typ, val, tb = sys.exc_info()
        frames = []
        while tb is not None:
            frames.append(tb)
            tb = tb.tb_next
        frame = [tb for tb in frames if 'write_struct' in str(tb.tb_frame.f_code)]
        variables = frame[0].tb_frame.f_locals
        obj = variables['obj']
        name = variables['fname']
        fail = True
    if fail:
        raise ParquetException('Thrift parameter validation failure %s'
                               ' when writing: %s-> Field: %s' % (
            val.args[0], obj, name
        ))
    return fobj.tell() - t0


def is_thrift_item(item):
    return hasattr(item, 'thrift_spec') and hasattr(item, 'read')


def thrift_print(structure, offset=0):
    """
    Handy recursive text ouput for thrift structures
    """
    if not is_thrift_item(structure):
        return str(structure)
    s = str(structure.__class__) + '\n'
    for key in dir(structure):
        if key.startswith('_') or key in ['thrift_spec', 'read', 'write',
                                          'default_spec', 'validate']:
            continue
        s = s + ' ' * offset + key + ': ' + thrift_print(getattr(structure, key)
                                                         , offset+2) + '\n'
    return s


def bind_method(cls, name, func):
    if sys.version_info.major == 2:
        setattr(cls, name, types.MethodType(func, None, cls))
    else:
        setattr(cls, name, func)


for clsname in dir(parquet_thrift):
    if clsname[0].isupper():
        cls = getattr(parquet_thrift, clsname)
        bind_method(cls, '__repr__', thrift_print)


def getstate_method(ob):
    b = io.BytesIO()
    write_thrift(b, ob)
    b.seek(0)
    return b.read()


def copy_method(self):
    cls = type(self)
    out = cls.__new__(cls)
    out.__dict__ = self.__dict__.copy()
    return out


def setstate_method(self, state):
    b = io.BytesIO(state)
    out = read_thrift(b, type(self))
    self.__dict__ = out.__dict__


for cls in [parquet_thrift.FileMetaData,
            parquet_thrift.RowGroup,
            parquet_thrift.ColumnChunk]:
    bind_method(cls, '__getstate__', getstate_method)
    bind_method(cls, '__setstate__', setstate_method)
    bind_method(cls, '__copy__', copy_method)
