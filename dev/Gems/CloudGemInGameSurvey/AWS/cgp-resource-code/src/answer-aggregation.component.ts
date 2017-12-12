import { Component, Input, OnInit, ViewChild, ViewContainerRef } from '@angular/core'
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { ModalComponent } from 'app/shared/component/index';
import { ToastsManager } from 'ng2-toastr';
import { LyMetricService } from 'app/shared/service/index';

import {
    InGameSurveyApi,
    SurveyAnswerAggregations,
    SurveyAnswerAggregationsUIModel,
    SurveyMetadata,
    Survey,
    InGameSurveyTextAnswerSubmissionComponent,
    Chart,
    QuestionAnswerAggregationsUIModel
} from './index';

@Component({
    selector: 'answer-aggregation',
    templateUrl: 'node_modules/@cloud-gems/cloudgemingamesurvey/answer-aggregation.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemingamesurvey/answer-aggregation.component.css']
})

export class InGameSurveyAnswerAggregationComponent {   
    @Input() survey: Survey;
    @Input() context: any;

    private Chart = Chart;

    private apiHandler: InGameSurveyApi;
    private surveyAnswerAggregations: SurveyAnswerAggregations;
    private surveyAnswerAggregationsUIModel: SurveyAnswerAggregationsUIModel;
    private isLoading: boolean;
    private curViewingTextQuestionId: String;

    @ViewChild(ModalComponent) modalRef: ModalComponent;

    constructor(private http: Http, private aws: AwsService, private toastr: ToastsManager, private vcr: ViewContainerRef, private metric: LyMetricService) {
    }
       
    ngOnInit() {   
        this.apiHandler = new InGameSurveyApi(this.context.ServiceUrl, this.http, this.aws, this.metric, this.context.identifier);
        this.surveyAnswerAggregationsUIModel = new SurveyAnswerAggregationsUIModel();
        this.isLoading = false;

        this.bindMethods();

        this.loadSurveyAnswerAggregations();
    }

    private bindMethods(): void {
        this.onShowTextAnswers = this.onShowTextAnswers.bind(this);
        this.onShowAggregatedResults = this.onShowAggregatedResults.bind(this);
        this.onShowBarChart = this.onShowBarChart.bind(this);
        this.onShowPieChart = this.onShowPieChart.bind(this);
    }

    private onShowBarChart(questionAnswerAggregation: QuestionAnswerAggregationsUIModel): void {
        questionAnswerAggregation.chart = Chart.Bar;
    }

    private onShowPieChart(questionAnswerAggregation: QuestionAnswerAggregationsUIModel): void {
        questionAnswerAggregation.chart = Chart.Pie;
    }

    private onShowTextAnswers(questionId: string): void {
        this.curViewingTextQuestionId = questionId;
    }

    private onShowAggregatedResults(): void {
        this.curViewingTextQuestionId = null;
    }    
    
    private loadSurveyAnswerAggregations() {
        this.isLoading = true;

        this.apiHandler.getSurveyAnswerAggregations(this.survey.surveyMetadata.surveyId).subscribe(answerAggregationsResponse => {
            let answerAggregationsObj = JSON.parse(answerAggregationsResponse.body.text());

            this.surveyAnswerAggregations = new SurveyAnswerAggregations();
            this.surveyAnswerAggregations.fromBackend(answerAggregationsObj.result);            

            this.surveyAnswerAggregationsUIModel.init(this.survey, this.surveyAnswerAggregations.questionIdToAnswerAggregationsMap);

            this.isLoading = false;
        }, err => {
            this.toastr.error("Survey answer aggregations did not refresh properly. " + err);
            this.isLoading = false;
        });
    }
}