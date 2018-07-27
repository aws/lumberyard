import { BehaviorSubject } from 'rxjs/BehaviorSubject';
import { Observable } from 'rxjs/Observable'
import { Targetable, Outputable } from './aws.service'
import { Schema } from './schema.class'
import { AwsContext } from './context.class'
import { LyMetricService } from 'app/shared/service/index'

export enum ResourceOutputEnum {
    ServiceUrl
}

export class ResourceOutput {
    outputKey: string;
    outputValue: string;
    description: string;

    constructor(key: string, val: string, desc: string) {
        this.outputKey = key;
        this.outputValue = val;
        this.description = desc;
    }
}

export abstract class ResourceGroup implements Targetable, Outputable {
    abstract outputs: Map<string, ResourceOutput>;
    abstract logicalResourceId: string;
    abstract physicalResourceId: string;
    abstract resourceType: string;
    abstract resourceStatus: string;
    abstract stackId: string;
    abstract stackName: string;
}

export class AwsResourceGroup extends ResourceGroup {

    public static get RESOURCE_TEMPLATE_FILE(): string { return "resource-template.json" }

    private _logicalResourceId: string;
    private _physicalResourceId: string;
    private _resourceType: string;
    private _resourceStatus: string;
    private _stackId: string;
    private _stackName: string;
    private _outputs: Map<string, ResourceOutput>;
    private _updating: BehaviorSubject<boolean>;

    get logicalResourceId() {
        return this._logicalResourceId;
    }

    get physicalResourceId() {
        return this._physicalResourceId;
    }

    get resourceType() {
        return this._resourceType;
    }

    get resourceStatus() {
        return this._resourceStatus;
    }

    get stackId() {
        return this._stackId;
    }

    get stackName() {
        return this._stackName;
    } 
        
    get outputs(): Map<string, ResourceOutput> {
        return this._outputs;
    }

    get updating(): Observable<boolean> {
        return this._updating.asObservable();
    }

    constructor(private context: AwsContext, spec: any) {         
        super();
        this.setResource(spec);
        this._outputs = new Map<string, ResourceOutput>();
        this._updating = new BehaviorSubject<boolean>(true);
    }

    public setOutputs(): void {
        //desribe the resource group stack resources
        this.context.cloudFormation.describeStacks({
            StackName: this.physicalResourceId
        }, (err: any, data: any) => {
            if (err) {
                let message = err.message
                if (err.statusCode == 403) {
                    this.context.authentication.refreshSessionOrLogout();
                }
            }
            var stackOutputs = data.Stacks[0].Outputs
            for (var i = 0; i < stackOutputs.length; i++) {
                let o = stackOutputs[i];                
                this.outputs[o.OutputKey] = o.OutputValue;
            }
            this._updating.next(false);
        });
    }

    public setResource(specification: any): void {
        this._logicalResourceId = specification[Schema.CloudFormation.LOGICAL_RESOURCE_ID];
        this._physicalResourceId = specification[Schema.CloudFormation.PHYSICAL_RESOURCE_ID];
        this._resourceStatus = specification[Schema.CloudFormation.RESOURCE_STATUS];
        this._resourceType = specification[Schema.CloudFormation.RESOURCE_TYPE.NAME];
        this._stackId = specification[Schema.CloudFormation.STACK_ID];
        this._stackName = specification[Schema.CloudFormation.STACK_NAME];
    }

}
