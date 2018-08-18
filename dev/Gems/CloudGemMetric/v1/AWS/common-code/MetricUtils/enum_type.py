
def create(**enums):
    return type('Enum', (), enums)