class Error(object):
    
    @staticmethod
    def exceeded_maximum_metric_capacity():
        return "ErrorExceededMaximumMetricCapacity"

    @staticmethod
    def missing_attribute():
        return "ErrorMissingAttributes"

    @staticmethod
    def is_not_lower():
        return "ErrorNotLowerCase"

    @staticmethod
    def out_of_order():
        return "ErrorOutOfOrder"

    @staticmethod
    def unable_to_sort():
        return "ErrorNotSorted"

    @staticmethod
    def is_null():
        return "ErrorNullValue"

    @staticmethod   
    def empty_dataframe():
        return "ErrorEmptyDataFrame"
    