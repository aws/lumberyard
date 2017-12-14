import { Component, OnInit, ChangeDetectorRef, ViewChild } from '@angular/core';
import { AwsService } from 'app/aws/aws.service'
declare var bootstrap: any

@Component({
    selector: 'project-logs',
    templateUrl: 'app/view/game/module/admin/component/log.component.html'    
})
export class ProjectLogComponent implements OnInit {    
    private context = {}
    private refresh = false

    constructor(
        private aws: AwsService     
    ) {               
    }

    ngOnInit() {
        this.context = {
            cloudwatchPhysicalId: bootstrap.projectPhysicalId
        } 
    }
}
