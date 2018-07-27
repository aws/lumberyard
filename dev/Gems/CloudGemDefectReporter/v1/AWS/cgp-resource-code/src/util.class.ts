import { DateTimeUtil } from 'app/shared/class/index';

export class RandomUtil {
// Utilities that use Math.random to generate random values

    static getRandomInt(min, max): number {
    // Quick random number generator (RNG) to test list

        min = Math.ceil(min);
        max = Math.floor(max);
        return Math.floor(Math.random() * (max - min)) + min;
    }
}