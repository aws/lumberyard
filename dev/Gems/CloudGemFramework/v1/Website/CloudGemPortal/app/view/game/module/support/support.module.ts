import { NgModule } from '@angular/core';
import { GameSharedModule } from '../shared/shared.module'
import { SupportRoutingModule } from './support.routes'
import { SupportComponent } from './component/support.component';


@NgModule({
    imports: [SupportRoutingModule, GameSharedModule],    
    declarations: [SupportComponent]
})
export class SupportModule { }