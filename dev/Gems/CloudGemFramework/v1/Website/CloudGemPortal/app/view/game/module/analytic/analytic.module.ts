import { NgModule } from '@angular/core';
import { AnalyticIndex } from './component/analytic.component';
import { AnalyticRoutingModule } from './analytic.routes'
import { GameSharedModule } from '../shared/shared.module'

@NgModule({
    imports: [AnalyticRoutingModule, GameSharedModule],
    declarations: [                
        AnalyticIndex        
    ]
})
export class AnalyticModule { }