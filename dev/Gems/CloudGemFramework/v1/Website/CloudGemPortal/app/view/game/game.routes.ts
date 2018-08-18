import { NgModule } from '@angular/core';
import { RouterModule } from '@angular/router';
import { AuthGuardService } from 'app/shared/service/index';
import { DependencyGuard } from './module/shared/service/index'
import { GameComponent } from './component/game.component';
import { AdminComponent } from './module/admin/component/admin.component';
import { ProjectLogComponent } from './module/admin/component/log.component';
import { UserAdminComponent } from './module/admin/component/user-admin/user-admin.component';
import { AnalyticIndex } from './module/analytic/component/analytic.component';
import { HeatmapComponent } from './module/analytic/component/heatmap/heatmap.component';
import { DashboardComponent } from './module/analytic/component/dashboard/dashboard.component';
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
                        canActivate: [AuthGuardService],
                        component: SupportComponent
                    },
                    {
                        path: 'cloudgems',
                        canActivate: [AuthGuardService],
                        component: GemIndexComponent
                    },
                    {
                        path: 'cloudgems/:id',
                        canActivate: [AuthGuardService],
                        component: GemIndexComponent
                    },
                    {
                        path: 'analytics',                        
                        canActivate: [AuthGuardService, DependencyGuard],
                        component: AnalyticIndex
                    },
                    {
                        path: 'analytics/dashboard',
                        canActivate: [AuthGuardService, DependencyGuard],
                        component: DashboardComponent
                    },
                    {
                       path: 'analytics/heatmap',
                       canActivate: [AuthGuardService, DependencyGuard],
                       component: HeatmapComponent
                    },
                    {
                        path: 'admin',
                        canActivate: [AuthGuardService],
                        component: AdminComponent,
                    },
                    {
                        path: 'admin/users',
                        canActivate: [AuthGuardService],
                        component: UserAdminComponent
                    },
                    {
                        path: 'admin/logs',
                        canActivate: [AuthGuardService],
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
