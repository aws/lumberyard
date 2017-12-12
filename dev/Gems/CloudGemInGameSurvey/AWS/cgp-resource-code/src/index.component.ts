import { Component, ViewChild, SecurityContext, Input } from '@angular/core';

import { AbstractCloudGemIndexComponent } from 'app/view/game/module/cloudgems/class/index';
import {
    InGameSurveyApi,
    InGameSurveySurveyDetailsPageComponent,
    InGameSurveySurveyListPageComponent,
    SurveyMetadata
} from './index';

export enum InGameSurveyPages {
    SurveyListPage,    
    SurveyDetailsPage,
}

@Component({
    selector: 'in-game-survey-index',
    templateUrl: 'node_modules/@cloud-gems/cloudgemingamesurvey/index.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemingamesurvey/index.component.css']
})

export class InGameSurveyIndexComponent extends AbstractCloudGemIndexComponent {
    @Input() context: any;
    private curPage: InGameSurveyPages;

    private InGameSurveyPages = InGameSurveyPages;
    private curSurveyMetadata: SurveyMetadata;

    constructor() {
        super()
    }

    ngOnInit() {
        this.curPage = InGameSurveyPages.SurveyListPage;

        this.bindMethods();
    }

    private bindMethods(): void {
        this.toSurveyDetailsPage = this.toSurveyDetailsPage.bind(this);
        this.toSurveyListPage = this.toSurveyListPage.bind(this);
    }

    private toSurveyDetailsPage(surveyMetadata: SurveyMetadata): void {
        this.curSurveyMetadata = surveyMetadata;
        this.curPage = InGameSurveyPages.SurveyDetailsPage;
    }

    private toSurveyListPage(): void {
        this.curPage = InGameSurveyPages.SurveyListPage;
    }
}