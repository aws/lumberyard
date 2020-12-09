import sqlite3
import uuid


class AssetDBConnection:
    def __init__(self, databasePath):
        sqlite3.register_converter('BLOB', lambda b: uuid.UUID(bytes=b))
        sqlite3.register_adapter(uuid.UUID, lambda u: buffer(u.bytes))
        self.databasePath = databasePath.replace('\\', '/')

    def __enter__(self):
        self.open()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()

    def open(self):
        self.connection = None
        if not self.databasePath:
            raise ValueError("Invalid database path set")
        try:
            self.connection = sqlite3.connect(self.databasePath, detect_types=sqlite3.PARSE_DECLTYPES)
        except Exception, e:
            raise RuntimeError('Error occored while trying to open db - %s\nFull error:%s' % (self.databasePath, e))

        if self.connection is None:
            raise ValueError('Could not connect to db - %s' % self.databasePath)

    def close(self):
        if self.connection:
            self.connection.close()

    def getAssetId(self, relativeProductPath, relativeSourcePath):
        if not self.connection:
            return None

        cursor = self.connection.cursor()

        dbProductQuery = "SELECT * from products where productname = '{0}'".format(relativeProductPath).lower()
        cursor.execute(dbProductQuery)
        productRecord = cursor.fetchone()

        dbSourceQuery = "SELECT * from Sources where SourceName = '{0}'".format(relativeSourcePath).lower()
        cursor.execute(dbSourceQuery)
        sourceRecord = cursor.fetchone()

        if productRecord is None or sourceRecord is None:
            return None

        return 'id={{{0}}}:{1:2X},type={{{2}}},hint={{{3}}}'.format(sourceRecord[3], productRecord[3], productRecord[4], productRecord[2])
