import { ViewChild, ElementRef, Input, Output, Component, EventEmitter, ViewContainerRef } from '@angular/core';
import { ModalComponent } from 'app/shared/component/index';
import { AbstractCloudGemIndexComponent } from 'app/view/game/module/cloudgems/class/index';
import { LyMetricService } from 'app/shared/service/index';
import { OnInit } from '@angular/core'

@Component({
    selector: 'prompt-configuration',
    templateUrl: 'node_modules/@cloud-gems/cloudgemspeechrecognition/prompt-configuration.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemspeechrecognition/prompt-configuration.component.css']
})

export class SpeechToTextPromptConfigurationComponent {
    @Input() promptContent: Object;
    @Input() title: string = "Prompt";
    @Input() showMaxAttempts: boolean;
    @Input() editable: boolean;
    @Input() context: any;
    @Output() promptContentChange = new EventEmitter<any>();

    private newPromptMessage: string = "";

    constructor(private vcr: ViewContainerRef, private metric: LyMetricService) {
    }

    /**
     * Initialize values
	**/
    ngOnInit() {
        if (this.showMaxAttempts) {
            this.promptContent = this.promptContent ? JSON.parse(JSON.stringify(this.promptContent)) : { 'maxAttempts': 2 };
        }
        else {
            this.promptContent = this.promptContent ? JSON.parse(JSON.stringify(this.promptContent)) : {};
        }
    }

    /**
     * Remove the selected prompt message
     * @param index the index of the prompt message in the list
	**/
    public removePromptMessage(index): void {
        this.promptContent['messages'].splice(index, 1);
        this.promptContentChange.emit(this.promptContent);
    }

    /**
     * Change the prompt message. Delete the entry if it is empty
     * @param $event the new prompt message
     * @param index the index of the prompt message in the list
	**/
    private promptMessageChange($event, index): void {
        if ($event == "") {
            this.removePromptMessage(index);
        }
        this.promptContentChange.emit(this.promptContent);
    }

    /**
     * Add a new prompt message
	**/
    public addNewPromptMessage(): void {
        if (this.newPromptMessage == "") {
            return;
        }

        this.promptContent["messages"] = this.promptContent["messages"] ? this.promptContent["messages"] : [];
        this.promptContent["messages"].push({ content: this.newPromptMessage, contentType: "PlainText" });
        this.promptContentChange.emit(this.promptContent);
        this.newPromptMessage = "";
    }
}