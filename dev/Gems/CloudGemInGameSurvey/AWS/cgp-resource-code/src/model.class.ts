export enum SurveyStatus {
    Active = 1,
    Scheduled = 1<<1,
    Closed = 1<<2,
    Draft = 1<<3
}

// UI Model for activation period 
export class SurveyActivationPeriodUIModel {
    start: any;
    end: any;
    hasStart: boolean;
    hasEnd: boolean;

    init(surveyMetadata: SurveyMetadata): void {
        let startDate = null;
        let endDate = null;

        if (surveyMetadata.activationStartTime) {
            startDate = new Date(surveyMetadata.activationStartTime * 1000);
            this.hasStart = true;
        } else {
            startDate = new Date();
            this.hasStart = false;
        }

        if (surveyMetadata.activationEndTime) {
            endDate = new Date(surveyMetadata.activationEndTime * 1000);
            this.hasEnd = true;
        } else {
            endDate = new Date();
            this.hasEnd = false;
        }

        this.start = {
            date: {
                year: startDate.getFullYear(),
                month: startDate.getMonth() + 1,
                day: startDate.getDate(),
            },
            time: (this.hasStart) ? { hour: startDate.getHours(), minute: startDate.getMinutes() } : { hour: 12, minute: 0 }
        };

        this.end = {
            date: {
                year: endDate.getFullYear(),
                month: endDate.getMonth() + 1,
                day: endDate.getDate(),
            },
            time: (this.hasEnd) ? { hour: endDate.getHours(), minute: endDate.getMinutes() } : { hour: 12, minute: 0 }
        };
    }
}

export class SurveyMetadata {
    surveyId: string;
    surveyName: string;
    creationTime: number;
    published: boolean;
    activationStartTime: number;
    activationEndTime: number;
    status: SurveyStatus;
    numResponses: number;

    constructor() {
        this.published = false;
        this.activationStartTime = 0;
        this.status = SurveyStatus.Draft;
        this.numResponses = 0;
    }

    determineSurveyStatus(currentEpochTime: number): void {
        if (!this.published) {
            this.status = SurveyStatus.Draft;
            return;
        }

        if (currentEpochTime < this.activationStartTime) {
            this.status = SurveyStatus.Scheduled;
            return;
        }

        if ((this.activationEndTime != null && this.activationEndTime != undefined) && currentEpochTime > this.activationEndTime) {
            this.status = SurveyStatus.Closed;
            return;
        }

        this.status = SurveyStatus.Active;
    }

    fromBackend(surveyMetadata: any) {
        this.surveyId = surveyMetadata['survey_id'];
        this.surveyName = surveyMetadata['survey_name'];
        this.creationTime = surveyMetadata['creation_time'];
        this.published = surveyMetadata['published'];
        this.activationStartTime = surveyMetadata['activation_start_time'];
        this.activationEndTime = surveyMetadata['activation_end_time'];
        this.status = surveyMetadata['status'];
        this.numResponses = surveyMetadata['num_responses'];
    }    
}

export class Survey {
    surveyMetadata: SurveyMetadata;
    questions: Question[];

    constructor() {
        this.surveyMetadata = new SurveyMetadata();
        this.questions = [];
    }

    init(survey: Object, surveyMetadata: SurveyMetadata) {
        this.surveyMetadata = surveyMetadata;

        this.questions = survey["questions"].map( (question) => {
            let out = new Question();
            out.fromBackend(question);
            return out;
        });
    }
}

export class Question {
    questionId: string;
    title: string;
    type: string;
    maxChars: number;
    min: number;
    max: number;
    minLabel: string;
    maxLabel: string;
    predefines: string[];
    multipleSelect: boolean;
    enabled: boolean;

    constructor() {
        this.enabled = true;
    }

    deepCopy(question: Question): void {
        this.questionId = question.questionId;
        this.title = question.title;
        this.type = question.type;
        this.maxChars = question.maxChars;
        this.min = question.min;
        this.max = question.max;
        this.minLabel = question.minLabel;
        this.maxLabel = question.maxLabel;
        this.predefines = question.predefines ? question.predefines.slice() : question.predefines;
        this.multipleSelect = question.multipleSelect;
        this.enabled = question.enabled;
    }

    fromBackend(question: Object) {
        this.questionId = question['question_id'];
        this.title = question['title'];
        this.type = question['type'];
        this.maxChars = question['max_chars'];
        this.min = question['min'];
        this.max = question['max'];
        this.minLabel = question['min_label'];
        this.maxLabel = question['max_label'];
        this.predefines = question['predefines'];
        this.multipleSelect = question['multiple_select'];
        this.enabled = question['enabled'];
    }

    toBackend(): Object {
        let question = {
            "title": this.title,
            "type": this.type
        };

        switch (question.type) {
            case "text":
                question["max_chars"] = this.maxChars;
                break;

            case "scale":
                question["min"] = this.min;
                question["max"] = this.max;
                question["min_label"] = this.minLabel && this.minLabel.length > 0 ? this.minLabel: null;
                question["max_label"] = this.maxLabel && this.maxLabel.length > 0 ? this.maxLabel : null;
                break;

            case "predefined":
                question["predefines"] = this.predefines;
                question["multiple_select"] = this.multipleSelect;
                break;
        }

        return question;
    }

    private getQuestionTypeDisplayText(): string {
        switch (this.type) {
            case "text":
                return 'Text';
            case "scale":
                return 'Slider';
            case "predefined":
                if (this.multipleSelect) {
                    return "Multiple Choice (Checkboxes)";
                } else {
                    return "Multiple Choice (Radio Buttons)";
                }
            default:
                return "unknown";
        }
    }
}

export enum Chart {
    Bar,
    Pie
}

// UI model for question aggregated answer
export class QuestionAnswerAggregationsUIModel {
    questionTitle: string;
    questionType: string;
    questionId: string;
    answerAggregations: Object[];
    totalResponses: number;
    chartData: Object[];
    chart: Chart;

    constructor() {
        this.answerAggregations = [];
        this.chartData = [];
        this.chart = Chart.Bar;
    }
}

// UI model for survey aggregated answer
export class SurveyAnswerAggregationsUIModel {
    questionAnswerAggregationsUIModels: QuestionAnswerAggregationsUIModel[];

    init(survey: Survey, questionIdToAnswerAggregationsMap: Map<string, QuestionAnswerAggregations>): void {
        this.questionAnswerAggregationsUIModels = [];
        if (!survey.questions) {
            return;
        }

        for (let i = 0; i < survey.questions.length; i++) {
            let totalResponses = 0;
            let question = survey.questions[i];
            let questionAnswerAggregationsUIModel = new QuestionAnswerAggregationsUIModel();
            questionAnswerAggregationsUIModel.questionTitle = question.title;
            questionAnswerAggregationsUIModel.questionType = question.type;
            questionAnswerAggregationsUIModel.questionId = question.questionId;

            let answerAggregations = questionIdToAnswerAggregationsMap.get(question.questionId);

            if (question.type == "predefined") {
                for (let k = 0; k < question.predefines.length; k++) {
                    let option = question.predefines[k];
                    let count = 0;
                    if (answerAggregations) {
                        count = answerAggregations.questionIndexToAnswerCountMap.get(String(k));
                        if (count == undefined) {
                            count = 0;
                        }
                    }

                    questionAnswerAggregationsUIModel.answerAggregations.push({
                        "answer": option,
                        "count": count
                    });
                    questionAnswerAggregationsUIModel.chartData.push({
                        "label": option,
                        "value": count
                    });

                    totalResponses += count;
                }
            } else if (question.type == "scale") {
                for (let k = question.min; k <= question.max; k++) {
                    let answer = String(k);

                    let count = 0;
                    if (answerAggregations) {
                        let answerCount = answerAggregations.questionIndexToAnswerCountMap.get(answer);
                        if (answerCount) {
                            count = answerCount;
                        }
                    }

                    questionAnswerAggregationsUIModel.answerAggregations.push({
                        "answer": answer,
                        "count": count
                    });
                    questionAnswerAggregationsUIModel.chartData.push({
                        "label": answer,
                        "value": count
                    });

                    totalResponses += count;
                }
            }

            questionAnswerAggregationsUIModel.totalResponses = totalResponses;

            this.questionAnswerAggregationsUIModels.push(questionAnswerAggregationsUIModel);
        }

        for (let questionAnswerAggregationsUIModel of this.questionAnswerAggregationsUIModels) {
            if (questionAnswerAggregationsUIModel.questionType == "text") {
                continue;
            }

            questionAnswerAggregationsUIModel.answerAggregations.sort(function (x, y) {
                if (x["count"] > y["count"]) {
                    return -1;
                } else if (x["count"] < y["count"]) {
                    return 1;
                } else {
                    return 0;
                }
            });            

            questionAnswerAggregationsUIModel.chartData.sort(function (x, y) {
                if (x["value"] > y["value"]) {
                    return -1;
                } else if (x["value"] < y["value"]) {
                    return 1;
                } else {
                    return 0;
                }
            });

            if (questionAnswerAggregationsUIModel.questionType == "predefined") {
                for (let i = 0; i < questionAnswerAggregationsUIModel.answerAggregations.length; i++) {
                    let answerAggregation = questionAnswerAggregationsUIModel.answerAggregations[i];
                    answerAggregation['answer'] = String(i+1) + '. ' + answerAggregation['answer'];
                }

                for (let i = 0; i < questionAnswerAggregationsUIModel.chartData.length; i++) {
                    let chartData = questionAnswerAggregationsUIModel.chartData[i];
                    chartData['label'] = String(i+1);
                }
            }

            if (questionAnswerAggregationsUIModel.questionType == "predefined" || questionAnswerAggregationsUIModel.questionType == "scale") {
                if (questionAnswerAggregationsUIModel.totalResponses > 0) {
                    for (let answerAggregation of questionAnswerAggregationsUIModel.answerAggregations) {
                        answerAggregation['count'] += " (" + Math.floor(answerAggregation["count"] / questionAnswerAggregationsUIModel.totalResponses * 100) + "%)";
                    }
                    for (let chartData of questionAnswerAggregationsUIModel.chartData) {
                        chartData['label'] += " (" + Math.floor(chartData["value"] / questionAnswerAggregationsUIModel.totalResponses * 100) + "%)";
                    }
                }
            }
        }
    }
}


export class SurveyAnswerAggregations {
    questionIdToAnswerAggregationsMap: Map<string, QuestionAnswerAggregations>;
 
    fromBackend(surveyAnswerAggregations: Object) {        
        this.questionIdToAnswerAggregationsMap = new Map<string, QuestionAnswerAggregations>();

        for (let answerAggregations of surveyAnswerAggregations["question_answer_aggregations"]) {
            let questionAnswerAggregations = new QuestionAnswerAggregations();
            questionAnswerAggregations.fromBackend(answerAggregations);
            this.questionIdToAnswerAggregationsMap.set(questionAnswerAggregations.questionId, questionAnswerAggregations);
        }
    }
}

export class QuestionAnswerAggregations {
    questionId: string;
    questionIndexToAnswerCountMap: Map<string, number>;

    fromBackend(questionAnswerAggregations: any) {
        this.questionId = questionAnswerAggregations['question_id'];
        this.questionIndexToAnswerCountMap = new Map<string, number>();

        for (let answerAggregation of questionAnswerAggregations['answer_aggregations']) {
            this.questionIndexToAnswerCountMap.set(answerAggregation['answer'], answerAggregation['count']);
        }
    }
}

export class SurveyAnswerSubmissions {
    paginationToken: string;
    answerSubmissions: AnswerSubmission[];

    constructor() {
        this.paginationToken = null;
        this.answerSubmissions = [];
    }
}

export class AnswerSubmissionUIModel {
    answers: Object[];

    constructor() {
        this.answers = [];
    }
    
    init(survey: Survey, questionIdToAnswerMap: Map<string, Answer>): void {
        this.answers = [];

        for (let i = 0; i < survey.questions.length; i++) {
            let question = survey.questions[i];
            let answer = questionIdToAnswerMap.get(question.questionId);
            let answerString = "";
            if (answer) {
                if (question.type == "predefined") {
                    let answers = [];

                    for (let idxStr of answer.answer) {
                        let questionIndex = Number(idxStr);
                        if (questionIndex >= 0 && questionIndex < question.predefines.length) {
                            answers.push(question.predefines[questionIndex]);
                        }
                    }

                    answerString = answers.join(", ");
                } else if (question.type == "scale") {
                    if (answer.answer.length > 0) {
                        answerString = answer.answer[0];
                    }
                } else { // text type question
                    if (answer.answer.length > 0) {
                        answerString = answer.answer[0];
                    }
                }
            }

            this.answers.push({
                "questionTitle": question.title,
                "answer": answerString
            });
        }
    }
}

export class TextAnswerSubmissionsUIModel {
    creationTime: number;
    answer: string;

    init(questionId, answerSubmission: AnswerSubmission): boolean {
        let answer = answerSubmission.questionIdToAnswerMap.get(questionId);
        if (!answer || answer.answer.length < 1) {
            return false;
        }

        this.creationTime = answerSubmission.creationTime;
        this.answer = answer.answer[0];

        return true;
    }
}

export class AnswerSubmission {
    submissionId: string;
    creationTime: number;
    questionIdToAnswerMap: Map<string, Answer>;

    fromBackend(answerSubmission: Object) {
        this.submissionId = answerSubmission['submission_id'];
        this.creationTime = answerSubmission['creation_time'];

        this.questionIdToAnswerMap = new Map<string, Answer>();

        for (let ans of answerSubmission['answers']) {
            let answer = new Answer();
            answer.fromBackend(ans);
            this.questionIdToAnswerMap.set(answer.questionId, answer);
        }
    }
}

export class Answer {
    questionId: string;
    answer: string[];

    fromBackend(answer: Object) {
        this.questionId = answer['question_id'];
        this.answer = answer['answer'];
    }
}

export enum SortOrder {
    DESC = 0,
    ASC = 1
}