import { NgModule } from '@angular/core';
import { GemModule } from 'app/view/game/module/cloudgems/gem.module';
import { DynamicContentThumbnailComponent, DynamicContentIndexComponent } from './index'
import { GameSharedModule } from 'app/view/game/module/shared/shared.module'

@NgModule({
    imports: [
        GameSharedModule,
        GemModule
    ],
    declarations: [
        DynamicContentIndexComponent,
        DynamicContentThumbnailComponent
    ],
    providers: [

    ],
    bootstrap: [DynamicContentThumbnailComponent, DynamicContentIndexComponent]
})
export class CloudGemDynamicContentModule { }