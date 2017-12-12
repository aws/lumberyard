import { NgModule } from '@angular/core';
import { RouterModule } from '@angular/router';

import { AdminComponent } from 'app/view/game/module/admin/component/admin.component';

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