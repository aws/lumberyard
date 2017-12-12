import { Component, Input, OnInit, ViewChild, ViewContainerRef } from '@angular/core'
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { ModalComponent } from 'app/shared/component/index';
import { ToastsManager } from 'ng2-toastr';
import { LyMetricService } from 'app/shared/service/index';

import {
    InGameSurveyApi,
    Survey,
    SurveyAnswerSubmissions,
    AnswerSubmission,
    AnswerSubmissionUIModel,
    TimeUtil,
    TextAnswerSubmissionsUIModel,
    SortOrder
} from './index';

@Component({
    selector: 'text-answer-submission-list',
    templateUrl: 'node_modules/@cloud-gems/cloudgemingamesurvey/text-answer-submission.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemingamesurvey/text-answer-submission.component.css']
})

export class InGameSurveyTextAnswerSubmissionComponent {   
    @Input() survey: Survey;
    @Input() questionId: String;
    @Input() context: any;

    private epochToString = TimeUtil.epochToString;

    private apiHandler: InGameSurveyApi;
    private paginationToken: string;
    private textAnswerSubmissionUIModels: TextAnswerSubmissionsUIModel[];
    private hasMoreSubmissionToLoad: boolean;
    private isLoading: boolean;
    private sortOrder: SortOrder;
    private loadSubmissionBatchSize = 100;

    @ViewChild(ModalComponent) modalRef: ModalComponent;

    constructor(private http: Http, private aws: AwsService, private toastr: ToastsManager, private vcr: ViewContainerRef, private metric: LyMetricService) {
    }
       
    ngOnInit() {   
        this.apiHandler = new InGameSurveyApi(this.context.ServiceUrl, this.http, this.aws, this.metric, this.context.identifier);
        this.isLoading = false;
        this.hasMoreSubmissionToLoad = true;
        this.paginationToken = null;
        this.sortOrder = SortOrder.DESC;
        this.textAnswerSubmissionUIModels = [];

        this.bindMethods();

        this.loadNextPageOfAnswerSubmissions();
    }

    private bindMethods(): void {
        this.loadNextPageOfAnswerSubmissions = this.loadNextPageOfAnswerSubmissions.bind(this);
        this.onReverseSubmissionTimeSortOrder = this.onReverseSubmissionTimeSortOrder.bind(this);
    }

    private onReverseSubmissionTimeSortOrder() {
        this.sortOrder ^= 1;

        this.hasMoreSubmissionToLoad = true;
        this.textAnswerSubmissionUIModels = [];
        this.paginationToken = null;

        this.loadNextPageOfAnswerSubmissions();
    }

    private loadNextPageOfAnswerSubmissions() {
        if (!this.hasMoreSubmissionToLoad) {
            return;
        }

        this.isLoading = true;

        this.apiHandler.getSurveyAnswerSubmissions(this.survey.surveyMetadata.surveyId,
            this.paginationToken, this.loadSubmissionBatchSize, SortOrder[this.sortOrder]).subscribe(response => {
                let submissionsObj = JSON.parse(response.body.text());

                let answerSubmissions = submissionsObj.result.submissions.map( (answerSubmission) => {
                    let out = new AnswerSubmission();
                    out.fromBackend(answerSubmission);
                    return out;
                });

                answerSubmissions.forEach( (answerSubmission) => {
                    let uiModel = new TextAnswerSubmissionsUIModel();
                    if (uiModel.init(this.questionId, answerSubmission)) {
                        this.textAnswerSubmissionUIModels.push(uiModel);
                    }
                })

                this.paginationToken = submissionsObj.result.pagination_token;
                if (!this.paginationToken) {
                    this.hasMoreSubmissionToLoad = false;
                }

                this.isLoading = false;
            }, err => {
                this.toastr.error("The survey answer submissions did not refresh properly. " + err);
                this.isLoading = false;
            });
    }
}