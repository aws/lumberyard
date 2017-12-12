import { NgModule } from '@angular/core';
import { AuthRoutingModule } from './auth.routes';
import { AuthenticationComponent } from './component/authentication.component'
import { AppSharedModule } from 'app/shared/shared.module';
import { LoginContainerComponent } from 'app/view/authentication/component/login.component';

@NgModule({
    imports: [AuthRoutingModule, AppSharedModule],    
    declarations: [        
        AuthenticationComponent,
        LoginContainerComponent
    ]    
})

export class AuthModule { }