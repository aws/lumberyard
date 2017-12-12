import { Injectable } from '@angular/core';

declare const CryptoJS: any;

@Injectable()
export class CacheHandlerService {
    private cacheMap = new Map<String, any>();

	constructor() { }

    public set(key: string, value: any, options?: any): void {
        var encryptedKeyString = CryptoJS.MD5(key).toString();
        this.cacheMap.set(encryptedKeyString, value);
}
    public setObject(key: any, value: any, options?: any): void {
        this.set(JSON.stringify(key), value, options);
	}

	public get(key: string): any {
        var encryptedKeyString = CryptoJS.MD5(key).toString();
        var cacheResult = this.cacheMap.get(encryptedKeyString);
        return cacheResult;  
	}
    public getObject(key: any): any {
        return this.get(JSON.stringify(key));
	}
	public remove(key: string): void {
        var encryptedKeyString = CryptoJS.MD5(key).toString();
        this.cacheMap.delete(encryptedKeyString);
	}
	public removeAll(): void {
        this.cacheMap = new Map<String, any>();
	}
    public exists(key: string): boolean {
        var encryptedKeyString = CryptoJS.MD5(key).toString();
        var valueExists = this.cacheMap.has(encryptedKeyString);
        if (valueExists) {
            return true; 
        }
        return false;
    }
    public objectExists(key: any): boolean {
        return this.exists(JSON.stringify(key));
    }
}