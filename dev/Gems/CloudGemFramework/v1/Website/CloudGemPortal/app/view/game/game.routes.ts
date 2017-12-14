import { NgModule } from '@angular/core';
import { RouterModule } from '@angular/router';
import { AuthGuardService } from 'app/shared/service/index';

import { GameComponent } from './component/game.component';
import { AdminComponent } from './module/admin/component/admin.component';
import { ProjectLogComponent } from './module/admin/component/log.component';
import { UserAdminComponent } from './module/admin/component/user-admin/user-admin.component';
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
                        path: 'cloudgems/:id',
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
                        component: AdminComponent,
                    },
                    {
                        path: 'admin/users',
                        canLoad: [AuthGuardService],
                        component: UserAdminComponent
                    },
                    {
                        path: 'admin/logs',
                        canLoad: [AuthGuardService],
                        component: ProjectLogComponent
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
