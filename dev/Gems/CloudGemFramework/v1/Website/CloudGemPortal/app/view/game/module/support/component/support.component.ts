import { Component } from '@angular/core';
import { AwsService } from 'app/aws/aws.service';
import { ApiService } from 'app/shared/service/index';

@Component({
    selector: 'game-welcome',
    templateUrl: 'app/view/game/module/support/component/support.component.html',
    styleUrls: ['app/view/game/module/support/component/support.component.css']

})
export class SupportComponent {

    public projectName: string;
    public window: any;

    public constructor(aws: AwsService, api: ApiService) {
        this.projectName = aws.context.name;
        this.window = window;
    }
}
     