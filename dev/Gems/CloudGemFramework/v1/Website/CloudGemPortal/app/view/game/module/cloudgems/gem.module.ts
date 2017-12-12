import { NgModule } from '@angular/core';
import { GemRoutingModule } from './gem.routes';
import { GemIndexComponent } from './component/gem-index.component';
import { MediumTackableGem } from './component/thumbnail.component';
import { CloudGemService } from './service/cloud-gem.service';
import { GemFactory } from './directive/gem-factory.directive';
import { GameSharedModule } from '../shared/shared.module'
import { CacheHandlerService } from './service/cachehandler.service'

@NgModule({
    imports: [GemRoutingModule, GameSharedModule],
    exports: [],
    declarations: [GemFactory, GemIndexComponent, MediumTackableGem],
    providers: [
        CloudGemService,
        CacheHandlerService
    ]
})
export class GemModule { }
