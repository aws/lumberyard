import { Component, OnInit } from '@angular/core';
import { DependencyService } from '../../shared/service/index'
import { FeatureDefinitions } from 'app/view/game/module/shared/class/feature.class'

@Component({
    selector: 'game-analytic-index',
    templateUrl: 'app/view/game/module/analytic/component/analytic.component.html',
    styleUrls: ['app/view/game/module/analytic/component/analytic.component.css']
})
export class AnalyticIndex implements OnInit {
    constructor(private dependencyservice: DependencyService ) { }

    ngOnInit() {
        this.dependencyservice.subscribeToDeploymentChanges(new FeatureDefinitions())        
    }
}
