import { GemModule } from 'app/view/game/module/cloudgems/gem.module';
import { $-GEM-NAME-$IndexComponent, $-GEM-NAME-$ThumbnailComponent } from './index'
import { GameSharedModule } from 'app/view/game/module/shared/shared.module'
import { NgModule } from '@angular/core';

@NgModule({
    imports: [
        GameSharedModule,
        GemModule
    ],
    declarations: [
        $-GEM-NAME-$IndexComponent,
        $-GEM-NAME-$ThumbnailComponent
    ],
    providers: [

    ],
    bootstrap: [$-GEM-NAME-$ThumbnailComponent, $-GEM-NAME-$IndexComponent]
})
export class $-GEM-NAME-$Module { }
