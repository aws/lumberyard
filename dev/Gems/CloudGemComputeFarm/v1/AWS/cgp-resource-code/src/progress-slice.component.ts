import { Input, Component } from '@angular/core';
import { CloudGemComputeFarmApi } from './index' 
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
 
@Component({
    selector: 'progress-slice',
    templateUrl: 'node_modules/@cloud-gems/cloudgemcomputefarm/progress-slice.component.html',
    styleUrls: [
        'node_modules/@cloud-gems/cloudgemcomputefarm/progress-slice.component.css',
        'node_modules/@cloud-gems/cloudgemcomputefarm/index.component.css'
    ]
})
export class ProgressSliceComponent {
    @Input() sliceInfo: any;
    
    private getLeadWidth(): number {
        return this.sliceInfo.initialized ? (this.sliceInfo.leadEndPos - this.sliceInfo.fixedParent.childLeadPos) : 0;
    }

    private entered(): void {
        this.sliceInfo.entered = true;
    }
}