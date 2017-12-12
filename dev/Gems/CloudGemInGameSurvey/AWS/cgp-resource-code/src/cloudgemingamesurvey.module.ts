import { NgModule } from '@angular/core';
import { GemModule } from 'app/view/game/module/cloudgems/gem.module';
import { GameSharedModule } from 'app/view/game/module/shared/shared.module';
import {
    InGameSurveyThumbnailComponent,
    InGameSurveyIndexComponent,
    InGameSurveyEditSurveyComponent,
    InGameSurveyAnswerAggregationComponent,
    InGameSurveyAnswerSubmissionComponent,
    InGameSurveySurveyDetailsPageComponent,
    InGameSurveySurveyListPageComponent,
    InGameSurveyTextAnswerSubmissionComponent
} from './index';

@NgModule({
    imports: [
        GameSharedModule,
        GemModule
    ],
    declarations: [
        InGameSurveyIndexComponent,
        InGameSurveyThumbnailComponent,
        InGameSurveyEditSurveyComponent,
        InGameSurveyAnswerAggregationComponent,
        InGameSurveyAnswerSubmissionComponent,
        InGameSurveySurveyDetailsPageComponent,
        InGameSurveySurveyListPageComponent,
        InGameSurveyTextAnswerSubmissionComponent
    ],
    providers: [

    ],
    bootstrap: [InGameSurveyThumbnailComponent, InGameSurveyIndexComponent]
})
export class CloudGemInGameSurveyModule { }