import errors
import validation_utils


def validate_question(question):
    __validate_question_title(question.get('title'))
    __validate_question_type(question.get('type'))
    __validate_type_specific_attributes(question)

def __validate_question_title(title):
    validation_utils.validate_param(title, 'title', validation_utils.is_not_blank_str)

def __validate_question_type(question_type):
    if question_type is None or question_type not in ['text', 'scale', 'predefined']:
        raise errors.ClientError('Question type is invalid: [{}]'.format(question_type))

def __validate_type_specific_attributes(question):
    question_type = question.get('type')
    if question_type == 'text':
        validation_utils.validate_param(question.get('max_chars'), 'max_chars', validation_utils.is_positive)
    elif question_type == 'scale':
        validate_min_max(question.get('min'), question.get('max'))
        min_label = question.get('min_label')
        if validation_utils.is_not_none(min_label):
            validation_utils.validate_param(min_label, 'min_label', validation_utils.is_not_blank_str)
        max_label = question.get('max_label')
        if validation_utils.is_not_none(max_label):
            validation_utils.validate_param(max_label, 'max_label', validation_utils.is_not_blank_str)
    elif question_type == 'predefined':
        validation_utils.validate_param(question.get('multiple_select'), 'multiple_select', validation_utils.is_not_none)
        predefines = question.get('predefines')
        validation_utils.validate_param(predefines, 'predefines', validation_utils.is_not_empty)
        for predefine in predefines:
            validation_utils.validate_param(predefine, 'predefine', validation_utils.is_not_blank_str)

def validate_min_max(min, max):
    validation_utils.validate_param(min, 'min', validation_utils.is_not_none)
    validation_utils.validate_param(max, 'max', validation_utils.is_not_none)
    if min > max:
        raise errors.ClientError('range is invalid, min: [{}], max: [{}]'.format(min, max))

def validate_activation_period(activation_start_time, activation_end_time):
    activation_start_time = activation_start_time if activation_start_time is not None else 0
    validation_utils.validate_param(activation_start_time, 'activation_start_time', validation_utils.is_non_negative)
    validation_utils.validate_param(activation_end_time, 'activation_end_time', validation_utils.is_non_negative_or_none)
    if activation_end_time is not None and activation_end_time < activation_start_time:
        raise errors.ClientError(
            'range is invalid, activation_start_time: [{}], activation_end_time: [{}]'.format(activation_start_time, activation_end_time))

    return activation_start_time, activation_end_time

def validate_answers(answers):
    validation_utils.validate_param(answers, 'answers', validation_utils.is_not_empty)
    if __has_duplicate_answer(answers):
        raise errors.ClientError('Answers has duplicate elements')
    for answer in answers:
        validation_utils.validate_param(answer.get('question_id'), 'question_id', validation_utils.is_not_blank_str)
        validation_utils.validate_param(answer.get('answer'), 'answer', validation_utils.is_not_empty)

def __has_duplicate_answer(answers):
    for answer in answers:
        answer_list = answer['answer']
        if len(answer_list) != len(set(answer_list)):
            return True
    return False

def validate_cognito_identity_id(cognito_identity_id):
    validation_utils.validate_param(cognito_identity_id, 'cognito_identity_id', validation_utils.is_not_blank_str)

def validate_survey_id(survey_id):
    validation_utils.validate_param(survey_id, 'survey_id', validation_utils.is_not_blank_str)

def validate_question_id(question_id):
    validation_utils.validate_param(question_id, 'question_id', validation_utils.is_not_blank_str)

def validate_submission_id(submission_id):
    validation_utils.validate_param(submission_id, 'submission_id', validation_utils.is_not_blank_str)

def validate_limit(limit, max_limit):
    if validation_utils.is_none(limit):
        return max_limit

    if not validation_utils.is_positive(limit):
        raise errors.ClientError('Limit is invalid: [{}]'.format(limit))

    return limit if limit < max_limit else max_limit

def validate_question_range_param(question_index, question_count):
    if validation_utils.is_non_negative_or_none(question_index) == False or validation_utils.is_non_negative_or_none(question_count) == False:
        raise errors.ClientError('Question range param is invalid, question_index: [{}], question_count: [{}]'.format(question_index, question_count))

    end_index = None
    if question_index is not None or question_count is not None:
        end_index = None if question_count is None else (question_count if question_index is None else question_index + question_count)

    return question_index, end_index

def validate_sort_order(sort):
    if validation_utils.is_empty(sort):
        return 'desc'

    sort = sort.lower()
    if sort not in ['asc', 'desc']:
        raise errors.ClientError('Sort order is invalid: [{}]'.format(sort))

    return sort
