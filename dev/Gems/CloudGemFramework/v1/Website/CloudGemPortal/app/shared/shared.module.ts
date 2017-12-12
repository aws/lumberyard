import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { RouterModule } from '@angular/router';
import { JsonpModule, HttpModule } from '@angular/http';
import { NgbModule } from '@ng-bootstrap/ng-bootstrap';
import { FormsModule, ReactiveFormsModule } from '@angular/forms';
import { ModelDebugPipe } from "./pipe/index";
import { AwsService } from "app/aws/aws.service";
import { ApiService, UrlService, DefinitionService, AuthGuardService, PreloadingService } from './service/index';
import {
    DateTimeRangePickerComponent,
    LoadingSpinnerComponent,
    AutoFocusComponent,
    FooterComponent,
    ModalComponent
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
        AutoFocusComponent,
        FooterComponent        
    ],
    declarations: [
        ModalComponent,            
        DateTimeRangePickerComponent,
        LoadingSpinnerComponent,
        ModelDebugPipe,      
        AutoFocusComponent,        
        FooterComponent        
    ],
    providers: [                                            
        DefinitionService,
        UrlService,
        AwsService,
        ApiService,
        AuthGuardService,
        PreloadingService
    ]    
})
export class AppSharedModule { }
