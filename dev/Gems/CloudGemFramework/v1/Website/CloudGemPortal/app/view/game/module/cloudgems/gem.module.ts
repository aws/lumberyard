import { NgModule } from '@angular/core';
import { GemRoutingModule } from './gem.routes';
import { GemIndexComponent } from './component/gem-index.component';
import { GemThumbnailComponent } from './component/gem-thumbnail.component';
import { GemFactory } from './directive/gem-factory.directive';
import { GameSharedModule } from '../shared/shared.module'
import { CacheHandlerService } from './service/cachehandler.service'

@NgModule({
    imports: [GemRoutingModule, GameSharedModule],
    exports: [GemThumbnailComponent],
    declarations: [GemFactory, GemIndexComponent, GemThumbnailComponent],
    providers: [
        CacheHandlerService
    ]
})
export class GemModule { }
