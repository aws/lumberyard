import { NgModule } from '@angular/core';
import { GemModule } from 'app/view/game/module/cloudgems/gem.module';
import { LeaderboardIndexComponent, LeaderboardThumbnailComponent } from './index'
import { GameSharedModule } from 'app/view/game/module/shared/shared.module'

@NgModule({
    imports: [
        GameSharedModule,
        GemModule
    ],
    declarations: [
        LeaderboardIndexComponent,
        LeaderboardThumbnailComponent
    ],
    providers: [

    ],
    bootstrap: [LeaderboardThumbnailComponent, LeaderboardIndexComponent]
})
export class CloudGemLeaderboardModule { }