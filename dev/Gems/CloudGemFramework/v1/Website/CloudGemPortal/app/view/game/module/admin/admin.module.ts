import { NgModule } from '@angular/core';
import { AdminRoutingModule } from './admin.routes'
import { GameSharedModule } from '../shared/shared.module'
import { AdminComponent } from './component/admin.component';

@NgModule({
    imports: [AdminRoutingModule, GameSharedModule],    
    declarations: [AdminComponent]
})
export class AdminModule { }