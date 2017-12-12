//View models
export class ValidationModelEntry {
    public isvalid: boolean = true;
    public message: string = null;

    constructor(isvalid: boolean = true, message: string = null) {
        this.isvalid = isvalid;
        this.message = message;
    }

}

export class ValidationModel {
    public username = new ValidationModelEntry(true, "Username is required");
    public playerName = new ValidationModelEntry();
    public gender = new ValidationModelEntry();
    public locale = new ValidationModelEntry();
    public givenName = new ValidationModelEntry();
    public middleName = new ValidationModelEntry();
    public familyName = new ValidationModelEntry();
    public email = new ValidationModelEntry(true, "Email is required");
    public accountId = new ValidationModelEntry();
    public birthdate = new ValidationModelEntry();
    public phone = new ValidationModelEntry();
    public nickName = new ValidationModelEntry();
   }

export class PlayerAccountEditModel {
    public accountId: string;
    public username: string; //immutable
    public lastModified: number;
    public serialization: SerializationModel;
    public validation: ValidationModel;
}

export class SerializationModel {
    public CognitoUsername?: string;
    public PlayerName?: string;
    public IdentityProviders?: IdentityModel;
}

export class PlayerAccountModel {
    public PlayerName: string;
    public CognitoIdentityId: string;
    public IdentityProviders: IdentityModel;
    public CognitoUsername: string;
    public AccountBlacklisted: boolean;
    public Email: string;
    public AccountId: string;
}

export class IdentityModel {
    public Cognito: CognitoModel;
}

export class CognitoModel {
    public username: string;
    public status: string;
    public create_date: number;
    public last_modified_date: number;
    public enabled: boolean;
    public IdentityProviderId: string;
    public email: string;
    public given_name: string;
    public family_name: string;
    public locale: string;
    public nickname: string;
    public gender: string;
}
//end view models