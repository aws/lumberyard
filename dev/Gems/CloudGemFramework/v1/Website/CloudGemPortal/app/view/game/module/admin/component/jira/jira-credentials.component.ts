import { Component, OnInit, AfterViewInit, ViewChild } from '@angular/core';
import { FormBuilder, FormGroup, Validators } from '@angular/forms';
import { Http } from '@angular/http';
import { ToastsManager } from 'ng2-toastr/ng2-toastr';
import { DefinitionService, BreadcrumbService } from 'app/shared/service/index';
import { MetricGraph, MetricQuery } from 'app/view/game/module/shared/class/index';
import { AwsService } from 'app/aws/aws.service';
import { Router } from '@angular/router';

@Component({
    selector: 'jira-credentials',
    templateUrl: 'app/view/game/module/admin/component/jira/jira-credentials.component.html',
    styles: [`
        .jira-credentials-container {
            margin-top: 25px;
        }
        .stored-credentials {
            font-family: "AmazonEmber-Bold";
        }
        .credential-property {
            padding-left: 0;
        }
    `]
})

export class JiraCredentialsComponent implements OnInit {
    private defectReporterApiHandler: any;
    private currentCredentials: any;
    private jiraIntegrationEnabled: boolean;
    private isLoadingJiraCredentials: boolean;
    private jiraCredentialsExist: boolean;

    constructor(private fb: FormBuilder,
        private http: Http,
        private aws: AwsService,
        private definition: DefinitionService,
        private breadcrumbs: BreadcrumbService,
        private router: Router,
        private toastr: ToastsManager) {


    }

    ngOnInit() {
        let defectReporterGemService = this.definition.getService("CloudGemDefectReporter");
        // Initialize the defect reporter api handler
        this.defectReporterApiHandler = new defectReporterGemService.constructor(defectReporterGemService.serviceUrl, this.http, this.aws);
        this.jiraIntegrationEnabled = defectReporterGemService.context.JiraIntegrationEnabled === 'enabled';

        if (this.jiraIntegrationEnabled) {
            this.currentCredentials = this.default();
            this.getJiraCredentials();
        }

        this.initializeBreadcrumbs();
    }

    public getLocation() {
        return 1;
    }

    initializeBreadcrumbs = () => {
        this.breadcrumbs.removeAllBreadcrumbs();
        this.breadcrumbs.addBreadcrumb("Administration", () => {
            this.router.navigateByUrl('/game/analytics');
        });
        this.breadcrumbs.addBreadcrumb("Jira Credentials", () => {
            this.initializeBreadcrumbs();
        });
    }

    updateJiraCredentials = () => {
        if (this.validate(this.currentCredentials)) {
            let body = {};
            for (let property of Object.keys(this.currentCredentials))
                body[property] = this.currentCredentials[property]['value'];

            this.defectReporterApiHandler.updateJiraCredentials(body).subscribe(response => {
                this.jiraCredentialsExist = true;
                this.toastr.success("Jira credentials were stored successfully.");
            }, err => {
                this.toastr.error("Failed to store Jira credentials. " + err.message);
            });
        }
    }

    getJiraCredentials = () => {
        this.isLoadingJiraCredentials = true;
        this.defectReporterApiHandler.getJiraCredentialsStatus().subscribe(response => {
            let obj = JSON.parse(response.body.text());
            this.jiraCredentialsExist = obj.result.exist;
            this.isLoadingJiraCredentials = false;
        }, err => {
            this.toastr.error("Failed to store Jira credentials. " + err.message);
            this.jiraCredentialsExist = false;
            this.isLoadingJiraCredentials = false;
        });
    }

    clearCredentials = () => {
        let body = {};
        for (let property of Object.keys(this.currentCredentials))
            body[property] = "";

        this.defectReporterApiHandler.updateJiraCredentials(body).subscribe(response => {
            this.currentCredentials = this.default();
            this.jiraCredentialsExist = false;
            this.toastr.success("The existing credentials were cleared successfully.");
        }, err => {
            this.toastr.error("Failed to clear the existing credentials. " + err.message);
        });
    }

    getPropertyName = (property) => {
        if (property === "userName") {
            return "User Name";
        }
        else if (property === "password") {
            return "Password";
        }
        else if (property === "server") {
            return "Server";
        }

        return property;
    }

    protected validate = (model) => {
        let isValid = true;
        for (let property of Object.keys(model)) {
            model[property]['valid'] = model[property]['value'] != '' ? true : false;
            isValid = isValid && model[property]['valid'];
        }
        return isValid;
    }

    private default(): any {
        return {
            userName: {
                valid: true,
                value: '',
                message: 'User name cannot be empty'
            },
            password: {
                valid: true,
                value: '',
                message: 'Password cannot be empty'
            },
            server: {
                valid: true,
                value: '',
                message: 'Server cannot be empty'
            }
        };
    }
}