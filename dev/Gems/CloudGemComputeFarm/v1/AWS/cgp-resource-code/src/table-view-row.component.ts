import { Input, Component } from '@angular/core';
import { CloudGemComputeFarmApi } from './index' 
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
 
@Component({
    selector: 'table-view-row',
    templateUrl: 'node_modules/@cloud-gems/cloudgemcomputefarm/table-view-row.component.html',
    styleUrls: [
        'node_modules/@cloud-gems/cloudgemcomputefarm/table-view-row.component.css',
        'node_modules/@cloud-gems/cloudgemcomputefarm/index.component.css'
    ]
})
export class TableViewRowComponent {
    @Input() row: any;
}
