import { NgModule } from '@angular/core';
import { AdminRoutingModule } from './admin.routes'
import { GameSharedModule } from '../shared/shared.module'
import { AdminComponent } from './component/admin.component';
import { ProjectLogComponent } from './component/log.component';
import { UserAdminComponent } from './component/user-admin/user-admin.component';

@NgModule({
    imports: [AdminRoutingModule, GameSharedModule],    
    declarations: [AdminComponent, ProjectLogComponent, UserAdminComponent]
})
export class AdminModule { }