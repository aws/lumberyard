import { GemModule } from 'app/view/game/module/cloudgems/gem.module';
import { GameSharedModule } from 'app/view/game/module/shared/shared.module';
import { NgModule } from '@angular/core';
import { 
    CloudGemDefectReporterIndexComponent, 
    CloudGemDefectReporterThumbnailComponent,
    CloudGemDefectReporterDefectDetailsPageComponent,
    CloudGemDefectReporterDefectListPageComponent,
    CloudGemDefectReproterDefectDatatableComponent,
    CloudGemDefectReporterReportInformationComponent,
    CloudGemDefectReporterPlayerInformationComponent,
    CloudGemDefectReporterSystemInformationComponent,
    CloudGemDefectReporterClientConfigurationComponent,
    CloudGemDefectReporterReportConfigurationComponent,
    CloudGemDefectReporterJiraIntegrationComponent,
    CloudGemDefectReporterCreateJiraIssueWindowComponent,
    CloudGemDefectReporterDefectDetailOverviewTabComponent,
    CloudGemDefectReporterRawDataTabComponent,
    CloudGemDefectReporterRecentSearchesTabComponent,
    CloudGemDefectReporterDefectListOverviewTabComponent,
    CloudGemDefectReporterDashboardTabComponent,
    CloudGemDefectReporterCustomFieldValueComponent,
    CloudGemDefectReporterClientConfigurationFieldComponent,
    CloudGemDefectReporterFieldMappingComponent
 } from './index';

@NgModule({
    imports: [
        GameSharedModule,
        GemModule
    ],
    declarations: [
        CloudGemDefectReporterIndexComponent,
        CloudGemDefectReporterThumbnailComponent,
        CloudGemDefectReporterDefectDetailsPageComponent,
        CloudGemDefectReporterDefectListPageComponent,
        CloudGemDefectReproterDefectDatatableComponent,
        CloudGemDefectReporterReportInformationComponent,
        CloudGemDefectReporterPlayerInformationComponent,
        CloudGemDefectReporterSystemInformationComponent,
        CloudGemDefectReporterClientConfigurationComponent,
        CloudGemDefectReporterJiraIntegrationComponent,
        CloudGemDefectReporterReportConfigurationComponent,
        CloudGemDefectReporterCreateJiraIssueWindowComponent,
        CloudGemDefectReporterDefectDetailOverviewTabComponent,
        CloudGemDefectReporterRawDataTabComponent,
        CloudGemDefectReporterRecentSearchesTabComponent,
        CloudGemDefectReporterDefectListOverviewTabComponent,
        CloudGemDefectReporterDefectListOverviewTabComponent,
        CloudGemDefectReporterDashboardTabComponent,
        CloudGemDefectReporterCustomFieldValueComponent,
        CloudGemDefectReporterClientConfigurationFieldComponent,
        CloudGemDefectReporterFieldMappingComponent
    ],
    providers: [

    ],
    bootstrap: [CloudGemDefectReporterThumbnailComponent, CloudGemDefectReporterIndexComponent]
})
export class CloudGemDefectReporterModule { }
