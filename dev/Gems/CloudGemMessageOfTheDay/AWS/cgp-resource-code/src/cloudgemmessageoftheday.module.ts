import { GemModule } from 'app/view/game/module/cloudgems/gem.module';
import { MessageOfTheDayIndexComponent, MessageOfTheDayThumbnailComponent } from './index'
import { GameSharedModule } from 'app/view/game/module/shared/shared.module'
import { NgModule } from '@angular/core';

@NgModule({
    imports: [
        GameSharedModule,
        GemModule
    ],
    declarations: [
        MessageOfTheDayIndexComponent,
        MessageOfTheDayThumbnailComponent
    ],
    providers: [

    ],
    bootstrap: [MessageOfTheDayThumbnailComponent, MessageOfTheDayIndexComponent]
})
export class CloudGemMessageOfTheDayModule { }
