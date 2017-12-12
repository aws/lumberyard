import { NgModule } from '@angular/core';
import { GameRoutingModule } from './game.routes';
import { AppSharedModule } from 'app/shared/shared.module';
import { GemModule } from './module/cloudgems/gem.module';
import { AnalyticModule } from './module/analytic/analytic.module';
import { SupportModule } from './module/support/support.module';
import { AdminModule } from "./module/admin/admin.module";
import { GameComponent } from "./component/game.component";
import { NavComponent } from './component/nav.component';

@NgModule({
    imports: [        
        GameRoutingModule,
        AppSharedModule,        
        GemModule,
        SupportModule,
        AdminModule,
        AnalyticModule        
    ],
    declarations: [
        NavComponent,
        GameComponent
    ]
})

export class GameModule { }
