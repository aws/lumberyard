import { Component, Input, OnInit, ViewChild, ViewContainerRef } from '@angular/core'
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { DateTimeUtil } from 'app/shared/class/index';
import { ModalComponent } from 'app/shared/component/index';
import { ToastsManager } from 'ng2-toastr';
import { ActionItem } from "app/view/game/module/shared/class/index";
import { LyMetricService } from 'app/shared/service/index';
import { FormBuilder, FormGroup, FormArray, Validators } from '@angular/forms';

import {
    InGameSurveyApi,
    SurveyActivationPeriodUIModel,
    SurveyStatus,
    SurveyMetadata,
    Survey,
    Question,
    TimeUtil,
    ValidationUtil
} from './index';

enum EditMode {
    None,
    CreateQuestion,
    EditQuestion,
    PublishSurvey,
    EditActivationPeriod,
    DeleteQuestion,
    DisableQuestion,
    EnableQuestion,
    ConfirmEditActiveSurvey,
    CloneSurvey,
    CloseSurvey
}

@Component({
    selector: 'edit-survey',
    templateUrl: 'node_modules/@cloud-gems/cloudgemingamesurvey/edit-survey.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemingamesurvey/edit-survey.component.css']
})

export class InGameSurveyEditSurveyComponent {   
    @Input() survey: Survey;
    @Input() context: any;

    private EditMode = EditMode;
    private SurveyStatus = SurveyStatus;
    private epochToString = TimeUtil.epochToString;    
    private isFormFieldRequiredEmpty = ValidationUtil.isFormFieldRequiredEmpty;
    private isFormFieldNotNonNegativeNum = ValidationUtil.isFormFieldNotNonNegativeNum;
    private isFormFieldNotValid = ValidationUtil.isFormFieldNotValid;

    private questionTypes = [
        { 'typeinfo': { 'type': 'predefined', 'multipleSelect': true }, 'displayText': 'Multiple Choice (Checkboxes)' },
        { 'typeinfo': { 'type': 'predefined', 'multipleSelect': false }, 'displayText': 'Multiple Choice (Radio Buttons)' },
        { 'typeinfo': { 'type': 'scale' }, 'displayText': 'Slider' },
        { 'typeinfo': { 'type': 'text' }, 'displayText': 'Text' }
    ];

    private apiHandler: InGameSurveyApi; 
    private curEditingQuestion: Question;
    private activationPeriod: SurveyActivationPeriodUIModel;
    private editMode: EditMode;
    private canEdit: boolean;

    private disableQuestionAction = new ActionItem("Disable Question", this.onShowDisableQuestionModal);
    private enableQuestionAction = new ActionItem("Enable Question", this.onShowEnableQuestionModal);

    private cloneSurveyAction = new ActionItem("Clone", this.onShowCloneSurveyModal);
    private closeSurveyAction = new ActionItem("End", this.onShowCloseSurveyModal);
    private editSurveySchedulingAction = new ActionItem("Edit Scheduling", this.onShowEditSchedulingModal);
    private reopenSurveyAction = new ActionItem("Reopen", this.onShowEditSchedulingModal);

    private questionTitleForm: FormGroup;
    private predefinedQuestionForm: FormGroup;
    private scaleQuestionForm: FormGroup;
    private textQuestionForm: FormGroup;
    private surveyNameForm: FormGroup;

    private dummyRadioButtonForm: FormGroup;
    private dummyTextAreaModels: string[];

    @ViewChild(ModalComponent) modalRef: ModalComponent;

    constructor(private fb: FormBuilder, private http: Http, private aws: AwsService, private toastr: ToastsManager, private vcr: ViewContainerRef, private metric: LyMetricService) {
        // dummy radio button form group for the radio buttons to work in preview
        this.dummyRadioButtonForm = fb.group({
            'dummy': 'dummy'
        })
    }
       
    ngOnInit() {   
        this.apiHandler = new InGameSurveyApi(this.context.ServiceUrl, this.http, this.aws, this.metric, this.context.identifier);
        this.editMode = EditMode.None;
        this.activationPeriod = new SurveyActivationPeriodUIModel();
        this.activationPeriod.init(this.survey.surveyMetadata);
        this.determineCanEditSurvey();

        this.dummyTextAreaModels = [];
        if (this.survey.questions) {
            for (let i = 0; i < this.survey.questions.length; i++) {
                this.dummyTextAreaModels.push("");
            }
        }

        this.bindMethods();
    }

    private bindMethods(): void {
        this.cloneSurveyAction.onClick = this.cloneSurveyAction.onClick.bind(this);
        this.closeSurveyAction.onClick = this.closeSurveyAction.onClick.bind(this);
        this.editSurveySchedulingAction.onClick = this.editSurveySchedulingAction.onClick.bind(this);        
        this.reopenSurveyAction.onClick = this.reopenSurveyAction.onClick.bind(this);        
        this.disableQuestionAction.onClick = this.disableQuestionAction.onClick.bind(this);
        this.enableQuestionAction.onClick = this.enableQuestionAction.onClick.bind(this);
        this.onQuestoinOrderChanged = this.onQuestoinOrderChanged.bind(this);
        this.onShowAddQuestionModal = this.onShowAddQuestionModal.bind(this);
        this.onDismissModal = this.onDismissModal.bind(this);
        this.onModifyQuestion = this.onModifyQuestion.bind(this);
        this.onAddQuestion = this.onAddQuestion.bind(this);
        this.onShowModifyQuestionModal = this.onShowModifyQuestionModal.bind(this);
        this.onShowPublishSurveyModal = this.onShowPublishSurveyModal.bind(this);
        this.onShowEditSchedulingModal = this.onShowEditSchedulingModal.bind(this);
        this.onShowDeleteQuestionModal = this.onShowDeleteQuestionModal.bind(this);
        this.onShowDisableQuestionModal = this.onShowDisableQuestionModal.bind(this);
        this.onShowEnableQuestionModal = this.onShowEnableQuestionModal.bind(this);
        this.onDeleteOption = this.onDeleteOption.bind(this);
        this.onAddOption = this.onAddOption.bind(this);
        this.onDeleteQuestion = this.onDeleteQuestion.bind(this);
        this.onDisableQuestion = this.onDisableQuestion.bind(this);
        this.onEnableQuestion = this.onEnableQuestion.bind(this);
        this.onPublishSurvey = this.onPublishSurvey.bind(this);
        this.onModifyActivationPeriod = this.onModifyActivationPeriod.bind(this);
        this.onUpdateSurveyActivationPeriod = this.onUpdateSurveyActivationPeriod.bind(this);
        this.onConfirmEditActiveSurvey = this.onConfirmEditActiveSurvey.bind(this);        
        this.onShowConfirmEditActiveSurveyModal = this.onShowConfirmEditActiveSurveyModal.bind(this);         
        this.onCloneSurvey = this.onCloneSurvey.bind(this);
        this.onCloseSurvey = this.onCloseSurvey.bind(this);
        this.onShowCloneSurveyModal = this.onShowCloneSurveyModal.bind(this);
        this.onShowCloseSurveyModal = this.onShowCloseSurveyModal.bind(this);
    }    

    private onShowCloneSurveyModal(): void {
        this.createSurveyNameForm();
        this.editMode = EditMode.CloneSurvey;
    }

    private onShowCloseSurveyModal(): void {
        this.editMode = EditMode.CloseSurvey;
    }

    private onCloneSurvey(): void {
        if (!this.validateSurveyNameForm()) {
            return;
        }

        this.apiHandler.cloneSurvey(this.surveyNameForm.value.surveyName, this.survey.surveyMetadata.surveyId).subscribe(response => {
            this.toastr.success("The survey has been cloned.");

            this.closeModal();
            this.editMode = EditMode.None;
        }, err => {
            this.toastr.error("Failed to update activation period. " + err);
            this.closeModal();
            this.editMode = EditMode.None;
        });
    }

    private onCloseSurvey(): void {
        let surveyEndTime = Math.floor(Date.now() / 1000) - 1;
        let surveyMetadata = this.survey.surveyMetadata;
        this.apiHandler.setSurveyActivationPeriod(surveyMetadata.surveyId,
            surveyMetadata.activationStartTime, surveyEndTime).subscribe(response => {
                this.toastr.success("Survey has been closed");

                surveyMetadata.activationEndTime = surveyEndTime;
                surveyMetadata.status = SurveyStatus.Closed;

                this.activationPeriod.init(surveyMetadata);

                this.closeModal();
                this.editMode = EditMode.None;
            }, err => {
                this.toastr.error("Failed to update activation period. " + err);
            });
    }

    private getActions(): ActionItem[] {
        if (this.survey.surveyMetadata.status == SurveyStatus.Draft) {
            return [this.cloneSurveyAction];
        } else if (this.survey.surveyMetadata.status == SurveyStatus.Active) {
            return [this.editSurveySchedulingAction, this.cloneSurveyAction, this.closeSurveyAction];
        } else if (this.survey.surveyMetadata.status == SurveyStatus.Closed) {
            return [this.reopenSurveyAction, this.cloneSurveyAction];
        } else {
            return [this.editSurveySchedulingAction, this.cloneSurveyAction];
        }
    }

    private onConfirmEditActiveSurvey(): void {
        this.canEdit = true;
        this.closeModal();
        this.editMode = EditMode.None;
    }

    private onUpdateSurveyActivationPeriod(updatedPeriod: SurveyActivationPeriodUIModel): void {
        this.activationPeriod.hasStart = updatedPeriod.hasStart;
        this.activationPeriod.hasEnd = updatedPeriod.hasEnd;

        this.activationPeriod.start = updatedPeriod.start;
        this.activationPeriod.end = updatedPeriod.end;
    }

    private determineCanEditSurvey(): void {
        this.canEdit = this.survey.surveyMetadata.status == SurveyStatus.Active ? false : true;
    }

    private validDate(): boolean {
        let start_date = DateTimeUtil.toObjDate(this.activationPeriod.start);
        let end_date = DateTimeUtil.toObjDate(this.activationPeriod.end);

        if (this.activationPeriod.hasStart && !start_date) {
            this.activationPeriod.start.valid = false;
            this.activationPeriod.start.message = "The start date is not a valid date.  A date must have a date, hour and minute."
            return false
        }

        if (this.activationPeriod.hasEnd && !end_date) {
            this.activationPeriod.end.valid = false;
            this.activationPeriod.end.message = "The end date is not a valid date.  A date must have a date, hour and minute."
            return false
        }
        return true
    }

    private onPublishSurvey(): void {
        if (!this.validDate()) {            
            return
        }
        
        let start = this.activationPeriod.hasStart ? DateTimeUtil.toObjDate(this.activationPeriod.start).getTime() / 1000 : 0;
        let end = this.activationPeriod.hasEnd ? DateTimeUtil.toObjDate(this.activationPeriod.end).getTime() / 1000 : null;        

        let surveyId = this.survey.surveyMetadata.surveyId;
        this.apiHandler.setSurveyActivationPeriod(surveyId, start, end).subscribe(response => {
            this.survey.surveyMetadata.activationStartTime = start;
            this.survey.surveyMetadata.activationEndTime = end;

            this.apiHandler.publishSurvey(surveyId).subscribe(response => {
                this.toastr.success("The survey is now published.");

                this.survey.surveyMetadata.published = true;
                this.survey.surveyMetadata.determineSurveyStatus(new Date().getTime());

                this.determineCanEditSurvey();
                
                this.closeModal();
                this.editMode = EditMode.None;
            }, err => {
                this.toastr.error("Failed to publish survey, please try again. " + err);
            });
        }, err => {
            this.toastr.error("Failed to set activation period, please try again. " + err);
        });
    }

    private onModifyActivationPeriod(): void {
        if (!this.validDate()) {
            return
        }

        let start = this.activationPeriod.hasStart ? DateTimeUtil.toObjDate(this.activationPeriod.start).getTime() / 1000 : 0;
        let end = this.activationPeriod.hasEnd ? DateTimeUtil.toObjDate(this.activationPeriod.end).getTime() / 1000 : null;           

        let surveyId = this.survey.surveyMetadata.surveyId;
        this.apiHandler.setSurveyActivationPeriod(surveyId, start, end).subscribe(response => {
            this.survey.surveyMetadata.activationStartTime = start;
            this.survey.surveyMetadata.activationEndTime = end;
            this.survey.surveyMetadata.determineSurveyStatus(new Date().getTime() / 1000);
            this.determineCanEditSurvey();

            this.closeModal();
            this.editMode = EditMode.None;
        }, err => {
            this.toastr.error("Failed to modify activation period, please try again. " + err);
        });
    }

    private onDeleteQuestion(questionId: string): void {
        this.apiHandler.deleteQuestion(this.survey.surveyMetadata.surveyId, questionId).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            let questionIdx = this.survey.questions.findIndex((question) => question.questionId == questionId);
            if (questionIdx != -1) {
                this.survey.questions.splice(questionIdx, 1);
                this.dummyTextAreaModels.splice(questionIdx, 1);
            }
            this.curEditingQuestion = null;
            this.closeModal();
        }, err => {
            this.toastr.error("Failed to delete question, please try again. " + err);
        });
    }

    private onDisableQuestion(questionId: string): void {
        this.apiHandler.enableQuestion(this.survey.surveyMetadata.surveyId, questionId, false).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            let questionIdx = this.survey.questions.findIndex( (question) => question.questionId == questionId);
            if (questionIdx != -1) {
                this.survey.questions[questionIdx].enabled = false;
            }

            this.closeModal();
        }, err => {
            this.toastr.error("Failed to disable question, please try again. " + err);
        });
    }

    private onEnableQuestion(questionId: string): void {
        this.apiHandler.enableQuestion(this.survey.surveyMetadata.surveyId, questionId, true).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            let questionIdx = this.survey.questions.findIndex((question) => question.questionId == questionId);
            if (questionIdx != -1) {
                this.survey.questions[questionIdx].enabled = true;
            }

            this.closeModal();
        }, err => {
            this.toastr.error("Failed to enable question, please try again. " + err);
        });
    }

    private trackByFn(index: any, item: any) {
        return index;
    }

    private onAddOption(): void {
        let options = <FormArray>this.predefinedQuestionForm.controls["options"];
        options.push(
            this.fb.group({
                'value': [null, Validators.compose([Validators.required])]
            })
        );
    }

    private onDeleteOption(optionIndex: number): void {
        let options = <FormArray>this.predefinedQuestionForm.controls["options"];
        options.removeAt(optionIndex);
    }

    private onShowModifyQuestionModal(question: Question): void {
        this.curEditingQuestion = new Question();
        this.curEditingQuestion.deepCopy(question);
        if (!this.curEditingQuestion.predefines || this.curEditingQuestion.predefines.length == 0) {
            this.curEditingQuestion.predefines = [''];
        }

        this.createQuestionForms();
        this.editMode = EditMode.EditQuestion;
    }

    private onShowPublishSurveyModal(): void {
        this.editMode = EditMode.PublishSurvey;        
    }

    private onShowEditSchedulingModal(): void {
        this.editMode = EditMode.EditActivationPeriod;
    }

    private onShowDeleteQuestionModal(question: Question): void {
        this.curEditingQuestion = question;
        this.editMode = EditMode.DeleteQuestion;
    }

    private onShowDisableQuestionModal(question: Question): void {
        this.curEditingQuestion = question;
        this.editMode = EditMode.DisableQuestion;
    }

    private onShowEnableQuestionModal(question: Question): void {
        this.curEditingQuestion = question;
        this.editMode = EditMode.EnableQuestion;
    }

    private onShowConfirmEditActiveSurveyModal(): void {
        this.editMode = EditMode.ConfirmEditActiveSurvey;
    }

    private closeModal(): void {
        this.modalRef.close();
    }

    private extractQuestionForm(): void {
        this.curEditingQuestion.title = this.questionTitleForm.value.title;

        switch (this.curEditingQuestion.type) {
            case "predefined":
                let predefines = [];
                let options = <FormArray>this.predefinedQuestionForm.controls["options"];
                for (let optionForm of options.controls) {
                    predefines.push(optionForm.value.value);
                }

                this.curEditingQuestion.predefines = predefines;
                break;
            case "scale":
                this.curEditingQuestion.min = Number(this.scaleQuestionForm.value.min);
                this.curEditingQuestion.max = Number(this.scaleQuestionForm.value.max);
                this.curEditingQuestion.minLabel = this.scaleQuestionForm.value.minLabel;
                this.curEditingQuestion.maxLabel = this.scaleQuestionForm.value.maxLabel;
                break;
            case "text":
                this.curEditingQuestion.maxChars = this.textQuestionForm.value.maxChars;
                break;
        }
    }

    private validateQuestionForms(): boolean {
        let success = true;

        success = this.validateQuestionTitleForm() && success;

        switch (this.curEditingQuestion.type) {
            case "predefined":
                success = this.validatePredefinedQuestionForm() && success;
                break;
            case "scale":
                success = this.validateScaleQuestionForm() && success;
                break;
            case "text":
                success = this.validateTextQuestionForm() && success;
                break;
        }

        return success;
    }

    private validateQuestionTitleForm(): boolean {
        this.questionTitleForm.controls["title"].markAsTouched();
        return this.questionTitleForm.controls["title"].valid;
    }

    private validateTextQuestionForm(): boolean {
        this.textQuestionForm.controls["maxChars"].markAsTouched();
        return this.textQuestionForm.controls["maxChars"].valid;
    }

    private validateScaleQuestionForm(): boolean {
        this.scaleQuestionForm.controls["min"].markAsTouched();
        this.scaleQuestionForm.controls["max"].markAsTouched();
        return this.scaleQuestionForm.controls["min"].valid &&
               this.scaleQuestionForm.controls["max"].valid;
    }

    private validatePredefinedQuestionForm(): boolean {
        let success = true;

        let options = <FormArray>this.predefinedQuestionForm.controls["options"];
        for (let option of options.controls) {
            let optionForm = <FormGroup>option;
            optionForm.controls["value"].markAsTouched();

            success = success && optionForm.controls["value"].valid;
        }
       
        return success;
    }

    private createSurveyNameForm(): void {
        this.surveyNameForm = this.fb.group({
            'surveyName': [null, Validators.compose([Validators.required])]
        });
    }

    private validateSurveyNameForm(): boolean {
        this.surveyNameForm.controls["surveyName"].markAsTouched();
        return this.surveyNameForm.controls["surveyName"].valid;
    }

    private onAddQuestion(): void {
        if (!this.validateQuestionForms()) {
            return;
        }

        this.extractQuestionForm();

        this.apiHandler.createQuestion(this.survey.surveyMetadata.surveyId, this.curEditingQuestion.toBackend()).subscribe(response => {
            let obj = JSON.parse(response.body.text());
            this.curEditingQuestion.questionId = obj.result.question_id;
            this.survey.questions.push(this.curEditingQuestion);
            this.dummyTextAreaModels.push("");
            this.curEditingQuestion = null;
            this.closeModal();
        }, err => {
            this.toastr.error("Failed to create question, please try again. " + err);
        });
    }

    private onModifyQuestion(): void {
        if (!this.validateQuestionForms()) {
            return;
        }

        this.extractQuestionForm();

        this.apiHandler.modifyQuestion(this.survey.surveyMetadata.surveyId,
            this.curEditingQuestion.questionId, this.curEditingQuestion.toBackend()).subscribe(response => {
                let questionIdx = this.survey.questions.findIndex((question) => question.questionId == this.curEditingQuestion.questionId);
                if (questionIdx != -1) {
                    this.survey.questions[questionIdx] = this.curEditingQuestion;
                }
                this.curEditingQuestion = null;
                this.closeModal();
            }, err => {
                this.toastr.error("Failed to modify question, please try again. " + err);
            });
    }

    private onDismissModal(): void {
        this.editMode = EditMode.None;
    }

    private onChangeQuestionType(questionType: any): void {
        this.curEditingQuestion.type = questionType.typeinfo.type;
        if (this.curEditingQuestion.type == 'predefined') {
            this.curEditingQuestion.multipleSelect = questionType.typeinfo.multipleSelect;
        }
    }

    private createDefaultQuestion(): Question {
        let defaultQuestion = new Question();
        defaultQuestion.type = "predefined";
        defaultQuestion.multipleSelect = true;
        defaultQuestion.predefines = [''];
        return defaultQuestion;
    }

    private createQuestionTitleForm(): void {
        this.questionTitleForm = this.fb.group({
            'title': [this.curEditingQuestion.title, Validators.compose([Validators.required])]
        });
    }

    private createPredefinedQuestionForm(): void {
        this.predefinedQuestionForm = this.fb.group({
            'options': this.fb.array([])
        });

        if (this.curEditingQuestion.predefines) {
            let options = <FormArray>this.predefinedQuestionForm.controls["options"];
            for (let option of this.curEditingQuestion.predefines) {
                let optionForm = this.fb.group({
                    'value': [option, Validators.compose([Validators.required])]
                });

                options.push(optionForm);
            }
        }
    }

    private createScaleQuestionForm(): void {
        this.scaleQuestionForm = this.fb.group({
            'min': [this.curEditingQuestion.min, Validators.compose([Validators.required, ValidationUtil.nonNegativeNumberValidator])],
            'max': [this.curEditingQuestion.max, Validators.compose([Validators.required, ValidationUtil.nonNegativeNumberValidator])],
            'minLabel': [this.curEditingQuestion.minLabel],
            'maxLabel': [this.curEditingQuestion.maxLabel]
        });
    }

    private createTextQuestionForm(): void {
        this.textQuestionForm = this.fb.group({
            'maxChars': [this.curEditingQuestion.maxChars, Validators.compose([Validators.required])]
        });
    }

    private createQuestionForms(): void {
        this.createQuestionTitleForm();
        this.createPredefinedQuestionForm();
        this.createScaleQuestionForm();
        this.createTextQuestionForm();
    }

    private onShowAddQuestionModal(): void {       
        this.curEditingQuestion = this.createDefaultQuestion();
        this.createQuestionForms();

        this.editMode = EditMode.CreateQuestion;
    }
    
    private onQuestoinOrderChanged(element, targetContainer, sourceContainer): void {
        let questionIdList = [];
        for (let questionElement of targetContainer.children) {
            questionIdList.push(questionElement["id"]);
        }
        
        let questionOrder = {
            "question_id_list": questionIdList
        };

        this.apiHandler.setSurveyQuestionOrder(this.survey.surveyMetadata.surveyId, questionOrder).subscribe(response => {
            this.toastr.success("Question order updated.");

            
            let questionIdToQuestionMap = {};
            for (let question of this.survey.questions) {
                questionIdToQuestionMap[question.questionId] = question;
            }

            this.survey.questions = questionIdList.map( (questionId) => {
                return questionIdToQuestionMap[questionId]
            });
        }, err => {           
            this.toastr.error("Failed to set question order, please try again. " + err);

            // restore questions to original order
            let container = document.getElementById(targetContainer["id"]);

            let questionIdToQuestionElementMap = {};

            for (let questionElement of targetContainer.children) {
                questionIdToQuestionElementMap[questionElement["id"]] = questionElement;
            }

            let questionElementList = this.survey.questions.map( (question) => {
                return questionIdToQuestionElementMap[question.questionId];
            });

            container.innerHTML = "";

            for (let questionElement of questionElementList) {
                container.appendChild(questionElement);
            }
        });
    }
}