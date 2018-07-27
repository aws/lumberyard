import { NgModule, ModuleWithProviders } from '@angular/core';
import { CommonModule } from '@angular/common';
import { RouterModule, Router } from '@angular/router';
import { JsonpModule, HttpModule, Http } from '@angular/http';
import { NgbModule } from '@ng-bootstrap/ng-bootstrap';
import { FormsModule, ReactiveFormsModule } from '@angular/forms';
import { ModelDebugPipe, ObjectKeysPipe, FromEpochPipe, FilterArrayPipe } from "./pipe/index";
import { AwsService } from "app/aws/aws.service";
import { ApiService, UrlService, DefinitionService, AuthGuardService, PreloadingService,
     LyMetricService, GemService, BreadcrumbService, PaginationService } from './service/index';
import { NgxChartsModule } from '@swimlane/ngx-charts';
import { NouisliderModule } from 'ng2-nouislider';
import { AnchorDirective } from './directive/anchor.directive';
import { NgxDatatableModule } from '@swimlane/ngx-datatable';

import {
    DateTimeRangePickerComponent,
    LoadingSpinnerComponent,
    AutoFocusComponent,
    FooterComponent,
    ModalComponent,
    PieChartComponent,
    BarChartComponent,
    TagComponent,
    InlineEditingComponent,
    SwitchButtonComponent,
    BreadcrumbComponent,
    DropdownComponent,
    ProgressBarComponent,
    LineChartComponent,
    TooltipComponent    
} from "./component/index";

@NgModule({
    imports: [CommonModule,
        JsonpModule,
        HttpModule,
        FormsModule,
        NgbModule,
        ReactiveFormsModule,
        RouterModule,
        NgxChartsModule,
        NouisliderModule,
        NgxDatatableModule
    ],
    exports: [
        CommonModule,
        JsonpModule,
        HttpModule,
        FormsModule,
        NgbModule,
        ReactiveFormsModule,
        RouterModule,
        NgxChartsModule,
        NgxDatatableModule,
        ModalComponent,
        DateTimeRangePickerComponent,
        LoadingSpinnerComponent,
        ModelDebugPipe,
        ObjectKeysPipe,
        FromEpochPipe,
        FilterArrayPipe,
        AutoFocusComponent,
        FooterComponent,
        TooltipComponent,
        PieChartComponent,
        BarChartComponent,        
        TagComponent,
        InlineEditingComponent,
        SwitchButtonComponent,
        BreadcrumbComponent,
        DropdownComponent,
        ProgressBarComponent,
        LineChartComponent,
        NouisliderModule,
        AnchorDirective
    ],
    declarations: [
        ModalComponent,
        DateTimeRangePickerComponent,
        LoadingSpinnerComponent,
        ModelDebugPipe,
        ObjectKeysPipe,
        FromEpochPipe,
        FilterArrayPipe,
        AutoFocusComponent,
        FooterComponent,
        TooltipComponent,
        PieChartComponent,
        BarChartComponent,        
        TagComponent,
        InlineEditingComponent,
        SwitchButtonComponent,
        BreadcrumbComponent,
        DropdownComponent,
        ProgressBarComponent,
        LineChartComponent,
        AnchorDirective
    ]
})
export class AppSharedModule {

    static forRoot(): ModuleWithProviders {
        return {
            ngModule: AppSharedModule,
            providers: [
                DefinitionService,
                BreadcrumbService,
                GemService,
                PaginationService,
                LyMetricService,
                {
                    provide: AwsService,
                    useClass: AwsService,
                    deps: [Router, Http, LyMetricService]
                },
                {
                    provide: UrlService,
                    useClass: UrlService,
                    deps: [AwsService]
                },
                {
                    provide: ApiService,
                    useClass: ApiService,
                    deps: [AwsService, Http, Router]
                },
                {
                    provide: AuthGuardService,
                    useClass: AuthGuardService,
                    deps: [Router, AwsService]
                },
                {
                    provide: PreloadingService,
                    useClass: PreloadingService,
                    deps: [DefinitionService]
                },

            ]
        }
    }
}
