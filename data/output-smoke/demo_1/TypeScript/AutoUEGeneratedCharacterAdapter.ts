import UE = require('ue');
import { beginAutoUEGeneratedCharacter, configureAutoUEGeneratedCharacter, tickAutoUEGeneratedCharacter } from './content/generated/AutoUEGeneratedRuntime';

class AutoUEGeneratedCharacterAdapter extends UE.Character {
    Constructor() { configureAutoUEGeneratedCharacter(this); }
    ReceiveBeginPlay(): void { beginAutoUEGeneratedCharacter(this); }
    ReceiveTick(dt: number): void { tickAutoUEGeneratedCharacter(this, dt); }
}

export = AutoUEGeneratedCharacterAdapter;
