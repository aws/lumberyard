import { Component, Input, OnInit, ViewChild, ViewContainerRef } from '@angular/core'
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { ModalComponent } from 'app/shared/component/index';
import { ToastsManager } from 'ng2-toastr';
import { Observable } from 'rxjs/rx';
import { LyMetricService } from 'app/shared/service/index';

import {
    InGameSurveyApi,
    Survey,
    SurveyAnswerSubmissions,
    AnswerSubmission,
    AnswerSubmissionUIModel,
    TimeUtil,
    SortOrder
} from './index';

enum EditMode {
    None,
    DeleteSubmission
}

@Component({
    selector: 'answer-submission-list',
    templateUrl: 'node_modules/@cloud-gems/cloudgemingamesurvey/answer-submission.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemingamesurvey/answer-submission.component.css']
})

export class InGameSurveyAnswerSubmissionComponent {   
    @Input() survey: Survey;
    @Input() context: any;

    private epochToString = TimeUtil.epochToString;
    private EditMode = EditMode;    

    private editMode: EditMode;
    private apiHandler: InGameSurveyApi;
    private answerSubmission: AnswerSubmission;
    private answerSubmissionUIModel: AnswerSubmissionUIModel;
    private surveyAnswerSubmissions: SurveyAnswerSubmissions;
    private hasMoreSubmissionToLoad: boolean;
    private isLoading: boolean;
    private sortOrder: SortOrder;
    private loadSubmissionBatchSize = 100;
    private curEditingAnswerSubmission: AnswerSubmission;

    private isExportingCSV: boolean;
    private exportCSVRequestId: string;
    private numResponsesExported: number;
    private checkExportStatusIntervalMilliSeconds = 3000;

    @ViewChild(ModalComponent) modalRef: ModalComponent;

    constructor(private http: Http, private aws: AwsService, private toastr: ToastsManager, private vcr: ViewContainerRef, private metric: LyMetricService) {
    }
       
    ngOnInit() {   
        this.apiHandler = new InGameSurveyApi(this.context.ServiceUrl, this.http, this.aws, this.metric, this.context.identifier);
        this.editMode = EditMode.None;
        this.isLoading = false;
        this.hasMoreSubmissionToLoad = true;
        this.surveyAnswerSubmissions = new SurveyAnswerSubmissions();
        this.sortOrder = SortOrder.DESC;
        this.isExportingCSV = false;

        this.bindMethods();

        this.loadNextPageOfAnswerSubmissions();
    }

    private bindMethods(): void {
        this.loadNextPageOfAnswerSubmissions = this.loadNextPageOfAnswerSubmissions.bind(this);
        this.onReverseSubmissionTimeSortOrder = this.onReverseSubmissionTimeSortOrder.bind(this);
        this.onShowAnswerSubmissionDetail = this.onShowAnswerSubmissionDetail.bind(this);
        this.onShowSumissionList = this.onShowSumissionList.bind(this);
        this.onExportAnswerSubmissionsToCSV = this.onExportAnswerSubmissionsToCSV.bind(this);
        this.onDismissModal = this.onDismissModal.bind(this);        
        this.onShowDeleteSubmissionModal = this.onShowDeleteSubmissionModal.bind(this);
        this.onDeleteAnswerSubmission = this.onDeleteAnswerSubmission.bind(this);
    }

    private closeModal() {
        this.modalRef.close();
    }

    private onDeleteAnswerSubmission(answerSubmission: AnswerSubmission): void {
        this.apiHandler.deleteAnswerSubmission(this.survey.surveyMetadata.surveyId, answerSubmission.submissionId).subscribe(response => {

            let idx = this.surveyAnswerSubmissions.answerSubmissions.findIndex((submission) => {
                return submission.submissionId == answerSubmission.submissionId;
            });

            if (idx != -1) {
                this.surveyAnswerSubmissions.answerSubmissions.splice(idx, 1);
            }            

            if (this.answerSubmission == answerSubmission) {
                this.answerSubmission = null;
                this.answerSubmissionUIModel = null;
            }

            this.closeModal();
            this.editMode = EditMode.None;
        }, err => {
            this.toastr.error("Failed to delete submission. " + err);
        });
    }

    private onShowDeleteSubmissionModal(answerSubmission: AnswerSubmission): void {
        this.curEditingAnswerSubmission = answerSubmission;
        this.editMode = EditMode.DeleteSubmission;
    }

    private onDismissModal(): void {
        this.editMode = EditMode.None;
    }

    private onExportAnswerSubmissionsToCSV() {
        if (this.isExportingCSV) {
            return;
        }       

        this.isExportingCSV = true;
        this.numResponsesExported = 0;

        let surveyId = this.survey.surveyMetadata.surveyId;
        this.apiHandler.exportSurveyAnswerSubmissionsToCSV(surveyId).subscribe(response => {                        
            let responseObj = JSON.parse(response.body.text());
            this.exportCSVRequestId = responseObj.result.request_id;

            let checkExportStatusTimer = Observable.timer(0, this.checkExportStatusIntervalMilliSeconds).subscribe(t => {
                this.apiHandler.getExportSurveyAnswerSubmissionsToCSVStatus(surveyId, this.exportCSVRequestId).subscribe(response => {
                    let statusObj = JSON.parse(response.body.text());
                    this.numResponsesExported = statusObj.result.num_submissions_exported;
                    if (statusObj.result.s3_presigned_url) {                        
                        this.isExportingCSV = false;
                        checkExportStatusTimer.unsubscribe();
                        window.open(statusObj.result.s3_presigned_url);
                    }
                });
            });
        }, err => {
            this.isExportingCSV = false;
            this.toastr.error("Failed to export answer submissions to CSV, please try again. " + err);
        });
    }

    private onShowSumissionList() {
        this.answerSubmission = null;
        this.answerSubmissionUIModel = null;
    }

    private onShowAnswerSubmissionDetail(answerSubmission: AnswerSubmission) {
        this.answerSubmission = answerSubmission;
        this.answerSubmissionUIModel = new AnswerSubmissionUIModel();
        this.answerSubmissionUIModel.init(this.survey, answerSubmission.questionIdToAnswerMap);
    }

    private onReverseSubmissionTimeSortOrder() {
        this.sortOrder ^= 1;

        this.hasMoreSubmissionToLoad = true;
        this.surveyAnswerSubmissions = new SurveyAnswerSubmissions();

        this.loadNextPageOfAnswerSubmissions();
    }

    private loadNextPageOfAnswerSubmissions() {
        if (!this.hasMoreSubmissionToLoad) {
            return;
        }

        this.isLoading = true;

        this.apiHandler.getSurveyAnswerSubmissions(this.survey.surveyMetadata.surveyId,
            this.surveyAnswerSubmissions.paginationToken, this.loadSubmissionBatchSize, SortOrder[this.sortOrder]).subscribe(response => {
                let submissionsObj = JSON.parse(response.body.text());

                this.surveyAnswerSubmissions.answerSubmissions = this.surveyAnswerSubmissions.answerSubmissions.concat(
                    submissionsObj.result.submissions.map((answerSubmission) => {
                        let out = new AnswerSubmission();
                        out.fromBackend(answerSubmission);
                        return out;
                    })
                );

                this.surveyAnswerSubmissions.paginationToken = submissionsObj.result.pagination_token;
                if (!this.surveyAnswerSubmissions.paginationToken) {
                    this.hasMoreSubmissionToLoad = false;
                }

                this.isLoading = false;
            }, err => {
                this.toastr.error("The survey answer submissions did not refresh properly. " + err);
                this.isLoading = false;
            });
    }
}