import { GemModule } from 'app/view/game/module/cloudgems/gem.module';
import { SWFConfigurationComponent, FleetManagementComponent, CloudGemComputeFarmIndexComponent, CloudGemComputeFarmThumbnailComponent } from './index'
import { GameSharedModule } from 'app/view/game/module/shared/shared.module'
import { NgModule } from '@angular/core';
import { ProgressSliceComponent } from './progress-slice.component';
import { TableViewRowComponent } from './table-view-row.component';
import { ViewHeaderComponent } from './view-header.component';

@NgModule({
    imports: [
        GameSharedModule,
        GemModule
    ],
    declarations: [
        SWFConfigurationComponent,
        FleetManagementComponent,
        CloudGemComputeFarmIndexComponent,
        CloudGemComputeFarmThumbnailComponent,
        ProgressSliceComponent,
        TableViewRowComponent,
        ViewHeaderComponent
    ],
    providers: [

    ],
    bootstrap: [CloudGemComputeFarmThumbnailComponent, CloudGemComputeFarmIndexComponent]
})
export class CloudGemComputeFarmModule { }
