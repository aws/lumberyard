import { ApiGatewayAction, ActionContext, EnumApiGatewayState } from '../apigateway.service'
import { BehaviorSubject } from 'rxjs/BehaviorSubject';
import { AwsContext } from 'app/aws/context.class'

declare var AWS: any;

export class ReadSwaggerAction implements ApiGatewayAction {

    constructor(private context: AwsContext) {

    }

    public handle(subject: BehaviorSubject<ActionContext>, ...args: any[]): void {
        let apiId = 'qyqyhyjdb0'; //args[0];
        if (apiId === undefined || apiId === null || apiId === '') {
            subject.next(<ActionContext>{
                state: EnumApiGatewayState.READ_SWAGGER_FAILURE,
                output: ["Cognito user was undefined."]
            });
            return;
        }

        var params = {
            exportType: 'swagger', /* required */
            restApiId: apiId, /* required */
            stageName: 'api', /* required */
            accepts: 'application/json',
            parameters: {
                'extensions': 'integrations, authorizers, documentation, validators, gatewayresponses',
                'Accept': 'application/json'
            }
        };

        this.context.apiGateway.getExport(params, function (err, data) {
            if (err) {
                subject.next(<ActionContext>{
                    state: EnumApiGatewayState.READ_SWAGGER_FAILURE,
                    output: [err]
                })
                return
            }            
            subject.next(<ActionContext>{
                state: EnumApiGatewayState.READ_SWAGGER_FAILURE,
                output: [JSON.parse(data.body)]
            })
        });
    }

}