import { NgModule } from '@angular/core';
import { GameRoutingModule } from './game.routes';
import { AppSharedModule } from 'app/shared/shared.module';
import { GemModule } from './module/cloudgems/gem.module';
import { AnalyticModule } from './module/analytic/analytic.module';
import { SupportModule } from './module/support/support.module';
import { AdminModule } from "./module/admin/admin.module";
import { GameComponent } from "./component/game.component";
import { NavComponent } from './component/nav.component';
import { SidebarComponent } from './component/sidebar.component';

@NgModule({
    imports: [                
        GameRoutingModule,
        AppSharedModule,                
        SupportModule,
        AdminModule,
        AnalyticModule,
        GemModule       
    ],    
    declarations: [
        NavComponent,
        SidebarComponent,
        GameComponent
    ]
})

export class GameModule { }
