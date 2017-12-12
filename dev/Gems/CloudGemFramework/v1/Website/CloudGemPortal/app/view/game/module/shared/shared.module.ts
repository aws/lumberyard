import { NgModule } from '@angular/core';
import { AppSharedModule } from 'app/shared/shared.module';
import {
    CloudWatchLogComponent,
    RestApiExplorerComponent,
    MetricComponent,
    BodyTreeViewComponent,
    DragableComponent,
    ActionStubComponent,
    ActionStubBasicComponent,
    ActionStubItemsComponent,
    PaginationComponent,
    FacetComponent,
    SearchComponent    
} from "./component/index";
import { DragulaModule, DragulaService } from 'ng2-dragula/ng2-dragula';
import {
    InnerRouterService,
    ApiGatewayService,
    CloudGemDirectoryService
} from './service/index';
import { FacetDirective } from './directive/facet.directive'

@NgModule({
    imports: [AppSharedModule, DragulaModule],
    exports: [
        AppSharedModule,        
        DragableComponent,
        ActionStubComponent,
        ActionStubBasicComponent,
        ActionStubItemsComponent,
        PaginationComponent,
        SearchComponent,
        FacetComponent          
    ],
    declarations: [                  
        DragableComponent,        
        ActionStubComponent,
        ActionStubBasicComponent,
        ActionStubItemsComponent,
        PaginationComponent,
        SearchComponent,  
        FacetComponent,                        
        FacetDirective,
        BodyTreeViewComponent,
        RestApiExplorerComponent,
        CloudWatchLogComponent,
        MetricComponent       
    ],
    providers: [
        DragulaService,
        InnerRouterService,
        ApiGatewayService,
        CloudGemDirectoryService
    ],
    entryComponents: [
        BodyTreeViewComponent,
        RestApiExplorerComponent,
        CloudWatchLogComponent, 
        MetricComponent             
    ]
})
export class GameSharedModule { }