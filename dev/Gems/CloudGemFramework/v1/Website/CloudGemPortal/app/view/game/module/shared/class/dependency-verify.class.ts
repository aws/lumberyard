import { AwsService } from 'app/aws/aws.service'
import { FeatureDefinitions } from 'app/view/game/module/shared/class/feature.class'

export class Verify {

    static dependency(name: string, aws: AwsService, featureDefinitions: FeatureDefinitions): boolean {
        for (let idx in featureDefinitions.defined) {
            let feature = featureDefinitions.defined[idx]
            if (name == feature.component.name) {
                let all_present = true
                for (let i = 0; i < feature.dependencies.length; i++) {
                    let dep = feature.dependencies[i]
                    let found = false
                    for (let idx in aws.context.project.activeDeployment.resourceGroupList) {
                        let rg = aws.context.project.activeDeployment.resourceGroupList[idx]
                        if (rg.logicalResourceId == dep)
                            found = true
                    }
                    all_present = all_present && found
                }
                return all_present
            }
        }
        return false
    }
}