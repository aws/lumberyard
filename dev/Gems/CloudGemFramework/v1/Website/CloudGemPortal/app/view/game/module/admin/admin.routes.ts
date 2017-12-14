import { NgModule } from '@angular/core';
import { RouterModule } from '@angular/router';

import { AdminComponent } from 'app/view/game/module/admin/component/admin.component';
import { UserAdminComponent } from 'app/view/game/module/admin/component/user-admin/user-admin.component';
import { ProjectLogComponent } from 'app/view/game/module/admin/component/log.component';

@NgModule({
    imports: [
        RouterModule.forChild([           
         
        ])
    ],    
    exports: [
        RouterModule
    ]
})
export class AdminRoutingModule { }