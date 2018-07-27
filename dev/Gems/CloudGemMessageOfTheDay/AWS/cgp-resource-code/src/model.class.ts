//View models
export class MotdForm {
    UniqueMsgID: string;
    message: string;
    priority: number;
    start?: any;
    end?: any;
    hasStart?: boolean;
    hasEnd?: boolean;
    datetime?: any;

    constructor(motdInfo: any) {
        this.UniqueMsgID = motdInfo.UniqueMsgID;
        this.start = motdInfo.start;
        this.end = motdInfo.end;
        this.message = motdInfo.message;        
        this.priority = motdInfo.priority;
        this.hasStart = motdInfo.hasStart;
        this.hasEnd = motdInfo.hasEnd;
    }

    public clearStartDate() {
        this.start = {
            date: {}
        };
    }

}
//end view models