import { NgModule, ModuleWithProviders } from '@angular/core';
import { CommonModule } from '@angular/common';
import { RouterModule, Router } from '@angular/router';
import { JsonpModule, HttpModule, Http } from '@angular/http';
import { NgbModule } from '@ng-bootstrap/ng-bootstrap';
import { FormsModule, ReactiveFormsModule } from '@angular/forms';
import { ModelDebugPipe, ObjectKeysPipe, FromEpochPipe, FilterArrayPipe } from "./pipe/index";
import { AwsService } from "app/aws/aws.service";
import { ApiService, UrlService, DefinitionService, AuthGuardService, PreloadingService, LyMetricService, GemService } from './service/index';
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
    SwitchButtonComponent
} from "./component/index";

@NgModule({
    imports: [CommonModule,
        JsonpModule,
        HttpModule,
        FormsModule,
        NgbModule,
        ReactiveFormsModule,
        RouterModule
    ],
    exports: [
        CommonModule,
        JsonpModule,
        HttpModule,
        FormsModule,
        NgbModule,
        ReactiveFormsModule,        
        RouterModule,             
        ModalComponent,
        DateTimeRangePickerComponent,
        LoadingSpinnerComponent,        
        ModelDebugPipe,   
        ObjectKeysPipe,  
        FromEpochPipe,      
        FilterArrayPipe,     
        AutoFocusComponent,
        FooterComponent,   
        PieChartComponent,
        BarChartComponent,   
        TagComponent,
        InlineEditingComponent,
        SwitchButtonComponent       
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
        PieChartComponent,
        BarChartComponent,   
        TagComponent,
        InlineEditingComponent,
        SwitchButtonComponent 
    ]
})
export class AppSharedModule {
    static forRoot(): ModuleWithProviders {
        return {
            ngModule: AppSharedModule,
            providers: [
                DefinitionService,
                GemService,
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
