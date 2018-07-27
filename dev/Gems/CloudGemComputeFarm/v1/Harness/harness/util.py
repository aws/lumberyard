def s3_key_join(*args):
    return "/".join([arg for arg in args if len(arg)])
