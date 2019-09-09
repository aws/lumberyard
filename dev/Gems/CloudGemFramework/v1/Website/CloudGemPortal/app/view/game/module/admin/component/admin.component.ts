import { Component, OnInit } from '@angular/core';
import { DependencyService } from '../../shared/service/index'
import { FeatureDefinitions, Location } from 'app/view/game/module/shared/class/feature.class'

@Component({
    selector: 'admin',
    templateUrl: 'app/view/game/module/admin/component/admin.component.html'
})
export class AdminComponent implements OnInit {
    constructor(private dependencyservice: DependencyService) { }

    ngOnInit() {
        this.dependencyservice.subscribeToDeploymentChanges(new FeatureDefinitions());
    }
}