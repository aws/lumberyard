import { NgModule } from '@angular/core';
import { RouterModule } from '@angular/router';

import { AuthenticationComponent } from './component/authentication.component'

@NgModule({
    imports: [
        RouterModule.forChild([      
            {
                path: 'login',
                component: AuthenticationComponent
            }
        ])
    ],
    exports: [
        RouterModule
    ]    
})
export class AuthRoutingModule { }