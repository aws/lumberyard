
import gzip
from .thrift_structures import parquet_thrift
from .util import PY2

# TODO: use stream/direct-to-buffer conversions instead of memcopy

compressions = {
    'UNCOMPRESSED': lambda x: x
}
decompressions = {
    'UNCOMPRESSED': lambda x, y: x
}

# Gzip is present regardless
COMPRESSION_LEVEL = 6
if PY2:
    def gzip_compress_v2(data, compresslevel=COMPRESSION_LEVEL):
        from io import BytesIO
        bio = BytesIO()
        f = gzip.GzipFile(mode='wb',
                          compresslevel=compresslevel,
                          fileobj=bio)
        f.write(data)
        f.close()
        return bio.getvalue()
    def gzip_decompress_v2(data, uncompressed_size):
        import zlib
        return zlib.decompress(data,
                               16+15)
    compressions['GZIP'] = gzip_compress_v2
    decompressions['GZIP'] = gzip_decompress_v2
else:
    def gzip_compress_v3(data, compresslevel=COMPRESSION_LEVEL):
        return gzip.compress(data, compresslevel=compresslevel)
    def gzip_decompress(data, uncompressed_size):
        return gzip.decompress(data)
    compressions['GZIP'] = gzip_compress_v3
    decompressions['GZIP'] = gzip_decompress

try:
    import snappy
    def snappy_decompress(data, uncompressed_size):
        return snappy.decompress(data)
    compressions['SNAPPY'] = snappy.compress
    decompressions['SNAPPY'] = snappy_decompress
except ImportError:
    pass
try:
    import lzo
    def lzo_decompress(data, uncompressed_size):
        return lzo.decompress(data)
    compressions['LZO'] = lzo.compress
    decompressions['LZO'] = lzo_decompress
except ImportError:
    pass
try:
    import brotli
    def brotli_decompress(data, uncompressed_size):
        return brotli.decompress(data)
    compressions['BROTLI'] = brotli.compress
    decompressions['BROTLI'] = brotli_decompress
except ImportError:
    pass
try:
    import lz4.block
    def lz4_compress(data, **kwargs):
        kwargs['store_size'] = False
        return lz4.block.compress(data, **kwargs)
    def lz4_decompress(data, uncompressed_size):
        return lz4.block.decompress(data, uncompressed_size=uncompressed_size)
    compressions['LZ4'] = lz4_compress
    decompressions['LZ4'] = lz4_decompress
except ImportError:
    pass
try:
    import zstandard
    def zstd_compress(data, **kwargs):
        kwargs['write_content_size'] = False
        cctx = zstandard.ZstdCompressor(**kwargs)
        return cctx.compress(data)
    def zstd_decompress(data, uncompressed_size):
        dctx = zstandard.ZstdDecompressor()
        return dctx.decompress(data, max_output_size=uncompressed_size)
    compressions['ZSTD'] = zstd_compress
    decompressions['ZSTD'] = zstd_decompress
except ImportError:
    pass
if 'ZSTD' not in compressions:
    try:
        import zstd
        def zstd_compress(data, **kwargs):
            kwargs['write_content_size'] = False
            cctx = zstd.ZstdCompressor(**kwargs)
            try:
                return cctx.compress(data, allow_empty=True)
            except TypeError:
                # zstandard-0.9 removed allow_empy and made it the default.
                return cctx.compress(data)
        def zstd_decompress(data, uncompressed_size):
            dctx = zstd.ZstdDecompressor()
            return dctx.decompress(data, max_output_size=uncompressed_size)
        compressions['ZSTD'] = zstd_compress
        decompressions['ZSTD'] = zstd_decompress
    except ImportError:
        pass

compressions = {k.upper(): v for k, v in compressions.items()}
decompressions = {k.upper(): v for k, v in decompressions.items()}

rev_map = {getattr(parquet_thrift.CompressionCodec, key): key for key in
           dir(parquet_thrift.CompressionCodec) if key in
           ['UNCOMPRESSED', 'SNAPPY', 'GZIP', 'LZO', 'BROTLI', 'LZ4', 'ZSTD']}


def compress_data(data, compression='gzip'):
    if isinstance(compression, dict):
        algorithm = compression.get('type', 'gzip')
        if isinstance(algorithm, int):
            algorithm = rev_map[compression]
        args = compression.get('args', None)
    else:
        algorithm = compression
        args = None

    if isinstance(algorithm, int):
        algorithm = rev_map[compression]

    if algorithm.upper() not in compressions:
        raise RuntimeError("Compression '%s' not available.  Options: %s" %
                (algorithm, sorted(compressions)))
    if args is None:
        return compressions[algorithm.upper()](data)
    else:
        if not isinstance(args, dict):
            raise ValueError("args dict entry is not a dict")
        return compressions[algorithm.upper()](data, **args)

def decompress_data(data, uncompressed_size, algorithm='gzip'):
    if isinstance(algorithm, int):
        algorithm = rev_map[algorithm]
    if algorithm.upper() not in decompressions:
        raise RuntimeError("Decompression '%s' not available.  Options: %s" %
                (algorithm.upper(), sorted(decompressions)))
    return decompressions[algorithm.upper()](data, uncompressed_size)
