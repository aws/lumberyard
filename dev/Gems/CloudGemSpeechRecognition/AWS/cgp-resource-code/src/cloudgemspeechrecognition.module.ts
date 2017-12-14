import { GemModule } from 'app/view/game/module/cloudgems/gem.module';
import { SpeechToTextIndexComponent, SpeechToTextThumbnailComponent, SpeechToTextEditIntentComponent, SpeechToTextEditSlotTypeComponent, SpeechToTextPromptComponent, SpeechToTextPromptConfigurationComponent } from './index';
import { GameSharedModule } from 'app/view/game/module/shared/shared.module'
import { NgModule } from '@angular/core';


@NgModule({
    imports: [
        GameSharedModule,
        GemModule
    ],
    declarations: [
        SpeechToTextIndexComponent,
        SpeechToTextThumbnailComponent,
        SpeechToTextEditIntentComponent,
        SpeechToTextEditSlotTypeComponent,
        SpeechToTextPromptComponent,
        SpeechToTextPromptConfigurationComponent
    ],
    providers: [
    ],
    bootstrap: [SpeechToTextThumbnailComponent, SpeechToTextIndexComponent]
})
export class CloudGemSpeechRecognitionModule { }
