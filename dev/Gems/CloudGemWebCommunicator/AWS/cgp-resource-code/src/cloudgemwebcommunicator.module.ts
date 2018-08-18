import { GemModule } from 'app/view/game/module/cloudgems/gem.module';
import { WebCommunicatorIndexComponent, WebCommunicatorThumbnailComponent } from './index';
import { GameSharedModule } from 'app/view/game/module/shared/shared.module'
import { NgModule } from '@angular/core';

@NgModule({
    imports: [
        GameSharedModule,
        GemModule
    ],
    declarations: [
        WebCommunicatorIndexComponent,
        WebCommunicatorThumbnailComponent
    ],
    providers: [
    ],
    bootstrap: [WebCommunicatorThumbnailComponent, WebCommunicatorIndexComponent]
})
export class CloudGemWebCommunicatorModule { }
