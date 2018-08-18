import { NgModule, ModuleWithProviders } from '@angular/core';
import { Router } from '@angular/router';
import { AwsService } from "app/aws/aws.service";
import { AnalyticIndex } from './component/analytic.component';
import { AnalyticRoutingModule } from './analytic.routes'
import { GameSharedModule } from '../shared/shared.module'
import { DashboardComponent } from './component/dashboard/dashboard.component';
import { CreateHeatmapComponent } from './component/heatmap/create-heatmap.component';
import { ListHeatmapComponent } from './component/heatmap/list-heatmap.component';
import { HeatmapComponent } from './component/heatmap/heatmap.component';
import { DashboardGraphComponent } from './component/dashboard/dashboard-graph.component';

@NgModule({
    imports: [AnalyticRoutingModule, GameSharedModule],
    declarations: [
        AnalyticIndex,
        HeatmapComponent,
        CreateHeatmapComponent,
        ListHeatmapComponent,
        DashboardComponent,
        DashboardGraphComponent        
    ]
})
export class AnalyticModule {

}