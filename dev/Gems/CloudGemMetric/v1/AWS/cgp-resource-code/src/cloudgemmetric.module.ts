import { GemModule } from 'app/view/game/module/cloudgems/gem.module';
import { CloudGemMetricIndexComponent } from './index.component';
import { CloudGemMetricThumbnailComponent } from './thumbnail.component';
import { GameSharedModule } from 'app/view/game/module/shared/shared.module'
import { NgModule } from '@angular/core';
import { MetricOverviewComponent } from './metric-overview.component';
import { MetricPartitionsComponent } from './metric-partitions.component';
import { MetricFilteringComponent } from './metric-filtering.component';
import { MetricSettingsComponent } from './metric-settings.component';

@NgModule({
    imports: [
        GameSharedModule,
        GemModule
    ],
    declarations: [
        CloudGemMetricIndexComponent,
        CloudGemMetricThumbnailComponent,
        MetricOverviewComponent,
        MetricPartitionsComponent,
        MetricFilteringComponent,
        MetricSettingsComponent
    ],
    bootstrap: [CloudGemMetricThumbnailComponent, CloudGemMetricIndexComponent]
})
export class CloudGemMetricModule { }
