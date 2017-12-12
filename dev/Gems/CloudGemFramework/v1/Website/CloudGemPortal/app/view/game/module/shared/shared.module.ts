import { NgModule } from '@angular/core';
import { AppSharedModule } from 'app/shared/shared.module';
import { DragableComponent, ActionStubComponent, ActionStubBasicComponent, ActionStubItemsComponent, PaginationComponent, SubNavComponent, SearchComponent } from "./component/index";
import { DragulaModule, DragulaService } from 'ng2-dragula/ng2-dragula';
import { InnerRouterService } from './service/index';

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
        SubNavComponent
    ],
    declarations: [                  
        DragableComponent,        
        ActionStubComponent,
        ActionStubBasicComponent,
        ActionStubItemsComponent,
        PaginationComponent,
        SearchComponent,  
        SubNavComponent
    ],
    providers: [
        DragulaService,
        InnerRouterService,        
    ]
})
export class GameSharedModule { }