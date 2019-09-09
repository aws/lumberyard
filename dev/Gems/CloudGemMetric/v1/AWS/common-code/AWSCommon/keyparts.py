import util
import datetime

class KeyParts(object):

    def __init__(self, key, sep):        
        self.__key = util.get_path_without_leading_seperator(key)
        self.__parts = self.__key.split(sep)

    @property
    def sensitivity_level(self):
        return self.raw_split(self.key_sensitivity)

    @property
    def source(self):
        return self.raw_split(self.key_source)

    @property
    def buildid(self):
        return self.raw_split(self.key_buildid)

    @property
    def datetime(self):
        return self.raw_split(self.key_datetime)

    @property
    def datetime_format(self):
        return self.raw_split(self.key_datetime, 0)

    @property
    def year(self):
        return int(self.raw_split(self.key_year))

    @property
    def month(self):
        return int(self.raw_split(self.key_month))

    @property
    def day(self):
        return int(self.raw_split(self.key_day))

    @property
    def hour(self):
        return int(self.raw_split(self.key_hour))

    @property
    def event(self):
        return self.raw_split(self.key_event)

    @property
    def filename(self):
        return self.__parts[11]

    @property
    def filename_timestamp(self):
        return datetime.datetime.strptime(self.filename.split("_")[0], util.path_date_format())    

    @property
    def extension(self):
        return self.raw_split(self.__parts[11],1,".")

    @property
    def schema(self):
        return self.raw_split(self.key_schema)

    @property
    def key_source(self):
        return self.__parts[7]

    @property
    def key_buildid(self):
        return self.__parts[8]

    @property
    def key_year(self):
        return self.__parts[3]

    @property
    def key_month(self):
        return self.__parts[4]

    @property
    def key_day(self):
        return self.__parts[5]

    @property
    def key_hour(self):
        return self.__parts[6]

    @property
    def key_event(self):
        return self.__parts[1]

    @property
    def key_schema(self):
        return self.__parts[10]

    @property
    def key_datetime(self):
        return self.__parts[2]

    @property
    def key_sensitivity(self):
        return self.__parts[9]

    @property
    def path(self):
        return self.__key.replace(self.filename, "") 

    @property
    def raw(self):
        return self.__parts

    def raw_split(self, value, idx=1, split_on="="):        
        return value.split(split_on)[idx]

    