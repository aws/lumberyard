export class WorkflowConfig {
    s3_dir: string = "";
    s3_file: string = "";
    max_level: number = 0;

    getProperty(key: string) {
        let type = typeof this[key];

        if (type == "object") {
            return JSON.stringify(this[key]);
        }

        return this[key];
    }

    setProperty(key: string, value: any, save: boolean = true): void {
        let type = typeof this[key];

        if (type == "undefined") {
            return;
        }
        
        if (type == "number") {
            this[key] = parseInt(value);
        }
        else if (type == "string") {
            this[key] = value;
        }
        else {
            if (typeof value == "string") {
                let data = JSON.parse(value);

                if (data) {
                    this[key] = data;
                }
            }
            else if (typeof value == "object") {
                this[key] = value;
            }
        }

        if (save) {
            window.localStorage.setItem("swfConfiguration", JSON.stringify(this));
        }
    }

    isFieldValid(key: string): boolean {
        let type = typeof this[key];

        if (type == "number") {
            return this[key] > 0;
        }
        else if (type == "string") {
            return this[key].length > 0;
        }
        else {
            return this[key] != null;
        }
    }

    isValid(): boolean {
        return this.isFieldValid("s3_file") && this.isFieldValid("max_level");
    }

    constructor() {
        let jsonString: string = localStorage.getItem("swfConfiguration");

        if (jsonString) {
            let data = JSON.parse(jsonString);

            for (let key of Object.keys(data)) {
                this.setProperty(key, data[key], false);
            }
        }
    }
}
