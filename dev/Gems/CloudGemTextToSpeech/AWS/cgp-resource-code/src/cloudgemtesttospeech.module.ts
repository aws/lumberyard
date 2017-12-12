import { GemModule } from 'app/view/game/module/cloudgems/gem.module';
import { TextToSpeechIndexComponent, TextToSpeechThumbnailComponent } from './index'
import { GameSharedModule } from 'app/view/game/module/shared/shared.module'
import { NgModule } from '@angular/core';

@NgModule({
    imports: [
        GameSharedModule,
        GemModule
    ],
    declarations: [
        TextToSpeechIndexComponent,
        TextToSpeechThumbnailComponent
    ],
    providers: [

    ],
    bootstrap: [TextToSpeechThumbnailComponent, TextToSpeechIndexComponent]
})
export class CloudGemTextToSpeechModule { }
