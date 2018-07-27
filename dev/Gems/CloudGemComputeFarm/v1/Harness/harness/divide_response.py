import uuid


class Task(object):
    def __init__(self, task_input, path_leaf):
        self.task_id = str(uuid.uuid4())
        self.task_input = task_input
        self.path_leaf = path_leaf


class DivideResponse(object):
    def __init__(self, path):
        self.merge_task = Task([], path)
        self.divide_tasks = []

    def as_dict(self):
        return {
            'merge_task_id': self.merge_task.task_id,
            'merge_input': self.merge_task.task_input,
            'merge_path': self.merge_task.path_leaf,
            'divide_task_ids': [task.task_id for task in self.divide_tasks],
            'divide_inputs': [task.task_input for task in self.divide_tasks],
            'divide_paths': [task.path_leaf for task in self.divide_tasks]
        }

    def set_leaf_task(self, task_input):
        self.merge_task.task_input = task_input

    def add_divide_task(self, task_input, path_leaf):
        self.divide_tasks.append(Task(task_input, path_leaf))
