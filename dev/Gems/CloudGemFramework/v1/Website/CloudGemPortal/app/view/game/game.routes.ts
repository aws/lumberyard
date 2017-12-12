import { NgModule } from '@angular/core';
import { RouterModule } from '@angular/router';
import { AuthGuardService } from 'app/shared/service/index';

import { GameComponent } from './component/game.component';
import { AdminComponent } from './module/admin/component/admin.component';
import { AnalyticIndex } from './module/analytic/component/analytic.component';
import { SupportComponent } from './module/support/component/support.component';
import { GemIndexComponent } from './module/cloudgems/component/gem-index.component';

@NgModule({
    imports: [
        RouterModule.forChild([
            {
                path: 'game', 
                component: GameComponent,
                canActivate: [AuthGuardService],
                children: [
                    {
                        path: 'support',
                        canLoad: [AuthGuardService],
                        component: SupportComponent
                    },
                    {
                        path: 'cloudgems',
                        canLoad: [AuthGuardService],
                        component: GemIndexComponent
                    },
                    {
                        path: 'analytics',
                        canLoad: [AuthGuardService],
                        component: AnalyticIndex
                    },
                    {
                        path: 'admin',
                        canLoad: [AuthGuardService],
                        component: AdminComponent
                    }
                ]
            }
        ])
    ],
    exports: [
        RouterModule
    ]
})
export class GameRoutingModule { }
