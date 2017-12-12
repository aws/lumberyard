import { Component, Input, OnInit, ViewChild, ViewContainerRef } from '@angular/core'
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { ModalComponent } from 'app/shared/component/index';
import { ToastsManager } from 'ng2-toastr';
import { LyMetricService } from 'app/shared/service/index';

import {
    InGameSurveyApi,
    InGameSurveyAnswerAggregationComponent,
    InGameSurveyAnswerSubmissionComponent,
    SurveyMetadata,
    Survey
} from './index';

@Component({
    selector: 'survey-details-page',
    templateUrl: 'node_modules/@cloud-gems/cloudgemingamesurvey/survey-details-page.component.html'
})

export class InGameSurveySurveyDetailsPageComponent {   
    @Input() context: any;
    @Input() surveyMetadata: SurveyMetadata;    

    private apiHandler: InGameSurveyApi;
    private survey: Survey;
    private isLoading: boolean;    
    private tabIndex: number;

    constructor(private http: Http, private aws: AwsService, private toastr: ToastsManager, private vcr: ViewContainerRef, private metric: LyMetricService) {
    }
       
    ngOnInit() {   
        this.apiHandler = new InGameSurveyApi(this.context.ServiceUrl, this.http, this.aws, this.metric, this.context.identifier);
        this.isLoading = false;
        this.tabIndex = 0;

        this.bindMethods();

        this.loadSurvey(this.surveyMetadata);
    }

    private bindMethods(): void {
        this.onTabIndexUpdated = this.onTabIndexUpdated.bind(this);
    }

    private onTabIndexUpdated(activeIndex: number): void {
        this.tabIndex = activeIndex;
    }

    private loadSurvey(surveyMetadata: SurveyMetadata) {
        this.isLoading = true;

        this.apiHandler.getSurvey(surveyMetadata.surveyId).subscribe(surveyResponse => {
            let surveyObj = JSON.parse(surveyResponse.body.text());

            this.survey = new Survey();
            this.survey.init(surveyObj.result, this.surveyMetadata);

            this.isLoading = false;
        }, err => {
            this.toastr.error("Failed to load survey, please try again. " + err);
            this.isLoading = false;
        });
    }
}