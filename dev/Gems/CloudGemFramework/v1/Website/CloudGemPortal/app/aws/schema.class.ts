
export class Schema {

    public static CloudFormation: {
        LOGICAL_RESOURCE_ID: string,
        PHYSICAL_RESOURCE_ID: string,
        RESOURCE_STATUS: string,
        RESOURCE_TYPE: {
            NAME: string,
            TYPES: {
                S3: string,
                CLOUD_FORMATION: string,
                LAMBDA: string
            }
        },
        STACK_ID: string,
        STACK_NAME: string,
    } = {
        LOGICAL_RESOURCE_ID: "LogicalResourceId",
        PHYSICAL_RESOURCE_ID: "PhysicalResourceId",
        RESOURCE_STATUS: "ResourceStatus",
        RESOURCE_TYPE: {
            NAME: "ResourceType",
            TYPES: {                
                S3: "AWS::S3::Bucket",
                CLOUD_FORMATION: "AWS::CloudFormation::Stack",
                LAMBDA: "AWS::Lambda::Function"
            }
        },
        STACK_ID: "StackId",
        STACK_NAME: "StackName",
    }

    public static Files: {
        INDEX_HTML : string
    } = {
        INDEX_HTML: "index.html",
    }

    public static Folders: {
        CGP_RESOURCES: string
    } = {
        CGP_RESOURCES: "cgp-resource-code",
    }
}
