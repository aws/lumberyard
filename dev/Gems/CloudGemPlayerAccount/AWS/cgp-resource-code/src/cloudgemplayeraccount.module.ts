import { NgModule } from '@angular/core';
import { GemModule } from 'app/view/game/module/cloudgems/gem.module';
import { PlayerAccountThumbnailComponent, PlayerAccountIndexComponent } from './index'
import { GameSharedModule } from 'app/view/game/module/shared/shared.module'

@NgModule({
    imports: [
        GameSharedModule,
        GemModule
    ],
    declarations: [
        PlayerAccountIndexComponent,
        PlayerAccountThumbnailComponent
    ],
    providers: [

    ],
    bootstrap: [PlayerAccountThumbnailComponent, PlayerAccountIndexComponent]
})
export class CloudGemPlayerAccountModule { }