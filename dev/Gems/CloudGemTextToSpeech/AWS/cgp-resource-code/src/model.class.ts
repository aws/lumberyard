//View models
export class SpeechEntry {
    character: string;
    line: string;
    tags: string[];
    isSelected: Object;
    previewLabelText: string;
    isEditing: boolean;
    isUploaded: boolean;

    constructor(speechInfo: any) {
        this.character = speechInfo.character;
        this.line = speechInfo.line;
        this.tags = speechInfo.tags;
        this.isSelected = speechInfo.isSelected;
        this.isEditing = false;
        this.isUploaded = false;
        this.previewLabelText = speechInfo.previewLabelText;
    }
}

export class CharacterEntry {
    name: string;
    voice: string;
    language: string;
    speechMarks: string;
    speechMarksStatus: Object[];
    tags: string[];
    voiceList: string[];
    isEditing: boolean;
    isUploaded: boolean;
    ssmlTags: string[];
    ssmlTagMappings: Object;
    previewLabelText: string;

    constructor(characterInfo: any) {
        this.name = characterInfo.name;
        this.voice = characterInfo.voice;
        this.language = characterInfo.language;
        this.speechMarksStatus = characterInfo.speechMarksStatus;
        this.speechMarks = characterInfo.speechMarks;
        this.voiceList = characterInfo.voiceList;
        this.ssmlTags = characterInfo.ssmlTags;
        this.ssmlTagMappings = characterInfo.ssmlTagMappings;
        this.isEditing = false;
        this.isUploaded = false;
        this.previewLabelText = characterInfo.previewLabelText;
    }
}

export class GeneratedPackageEntry {
    name: string;
    lastModified: string;
    size: number;
    isSelected: boolean;

    constructor(generatedPackageInfo: any) {
        this.name = generatedPackageInfo.name;
        this.lastModified = generatedPackageInfo.lastModified;
        this.size = generatedPackageInfo.size;
        this.isSelected = false;
    }
}
//end view models