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
    SearchComponent,
    ThumbnailComponent,
    GraphComponent
} from "./component/index";
import { DragulaModule, DragulaService } from 'ng2-dragula/ng2-dragula';
import {
    InnerRouterService,
    ApiGatewayService,    
    DependencyService,
    DependencyGuard

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
        FacetComponent,
        ThumbnailComponent,
        GraphComponent
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
        MetricComponent,
        ThumbnailComponent,
        GraphComponent
    ],
    providers: [
        DragulaService,
        InnerRouterService,
        ApiGatewayService,        
        DependencyService,
        DependencyGuard
    ],
    entryComponents: [
        BodyTreeViewComponent,
        RestApiExplorerComponent,
        CloudWatchLogComponent, 
        MetricComponent,
        GraphComponent            
    ]
})
export class GameSharedModule { }