import { ViewChild, ElementRef, Input, Output, Component, EventEmitter, ViewContainerRef } from '@angular/core';
import { ModalComponent } from 'app/shared/component/index';
import { AbstractCloudGemIndexComponent } from 'app/view/game/module/cloudgems/class/index';
import { LyMetricService } from 'app/shared/service/index';
import { OnInit } from '@angular/core'

enum EditPromptMode {
    EditPrompt,
    List
}

@Component({
    selector: 'prompt',
    templateUrl: 'node_modules/@cloud-gems/cloudgemspeechrecognition/prompt.component.html',
    styleUrls: ['node_modules/@cloud-gems/cloudgemspeechrecognition/prompt.component.css']
})

export class SpeechToTextPromptComponent {
    @Input() promptContent: Object;
    @Input() showMaxAttempts: boolean;
    @Input() context: any;
    @Input() editable: boolean;
    @Output() promptContentChange = new EventEmitter<any>();

    @ViewChild(ModalComponent) modalRef: ModalComponent;

    private mode: EditPromptMode;
    private sttModes: any;
    
    private currentPrompt: Object;
    private newPromptMessage: string = "";
    private promptType: string;
    
    private sortDir: string;

    constructor(private vcr: ViewContainerRef, private metric: LyMetricService) {
    }

    /**
     * Initialize values
	**/
    ngOnInit() {
        // bind proper scopes for callbacks that have a this reference
        this.dismissModal = this.dismissModal.bind(this);
        this.editPromptModal = this.editPromptModal.bind(this);
        // end of bind scopes

        this.sttModes = EditPromptMode;
        this.mode = EditPromptMode.List;
    }

    /**
     * Change the prompt content. Delete the entry if it is empty
     * @param $event the new content of the prompt
     * @param index the index of the prompt in the list
	**/
    private promptChange($event, index): void {
        if (this.promptContent && this.promptContent['messages']) {
            this.promptContent['messages'][index]['content'] = $event;
            if ($event == "") {
                this.promptContent['messages'].splice(index, 1);
                if (this.promptContent['messages'].length == 0) {
                    delete this.promptContent;
                }
            }
        }
        else if (!this.promptContent) {
            this.promptContent = this.showMaxAttempts ? { "messages": [], 'maxAttempts': 2 } : { "messages": [] };
            this.promptContent["messages"].push({ content: $event, contentType: "PlainText" });
            this.newPromptMessage = "";
        }

        this.promptContentChange.emit(this.promptContent);
    }

    /**
     * Save the current prompt
    **/
    public savePrompt(): void {       
        this.modalRef.close();
        this.mode = EditPromptMode.List;
        this.promptContent = this.currentPrompt;
        if (this.promptContent['messages'].length == 0) {
            delete this.promptContent;
        }
    }

    /**
     * Define the EditPrompt modal
    **/
    public editPromptModal(): void {
        this.mode = EditPromptMode.EditPrompt;
        if (this.showMaxAttempts) {
            this.currentPrompt = this.promptContent ? JSON.parse(JSON.stringify(this.promptContent)) : { 'maxAttempts': 2 };
        }
        else {
            this.currentPrompt = this.promptContent ? JSON.parse(JSON.stringify(this.promptContent)) : {};
        }
    }

    /**
     * Define the Dismiss modal
	**/
    public dismissModal(bot): void {
        this.mode = EditPromptMode.List;
    }
}