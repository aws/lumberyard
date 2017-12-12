import { GemModule } from 'app/view/game/module/cloudgems/gem.module';
import { SpeechToTextIndexComponent, SpeechToTextThumbnailComponent } from './index'
import { GameSharedModule } from 'app/view/game/module/shared/shared.module'
import { NgModule } from '@angular/core';

@NgModule({
    imports: [
        GameSharedModule,
        GemModule
    ],
    declarations: [
        SpeechToTextIndexComponent,
        SpeechToTextThumbnailComponent
    ],
    providers: [

    ],
    bootstrap: [SpeechToTextThumbnailComponent, SpeechToTextIndexComponent]
})
export class CloudGemSpeechToTextModule { }
