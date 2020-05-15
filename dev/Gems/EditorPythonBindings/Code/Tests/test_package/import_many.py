#
# testing mulitple test modules with azlmbr
# 
import azlmbr
import azlmbrtest
import do_work

def test_many_entity_id():
    do_work.print_entity_id(azlmbrtest.EntityId(101))
