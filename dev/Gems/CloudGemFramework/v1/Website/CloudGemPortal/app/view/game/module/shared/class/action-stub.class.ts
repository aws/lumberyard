export class ActionItem {
    public name: string;
    public onClick: Function;

    constructor(displayname: string, onclick: Function) {
        this.name = displayname;
        this.onClick = onclick;
    }
}
