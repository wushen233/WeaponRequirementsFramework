Scriptname WRF_Native Native Hidden
{Weapon Requirements Framework public Papyrus API.}

bool Function IsModEnabled() Global Native

float Function GetEquippedTotalRequirement(Actor akActor) Global Native

float Function GetEquippedBaseRequirement(Actor akActor) Global Native

int Function GetAmmoRequirementValue(Form akAmmo) Global Native

float Function GetActorStrengthDeficit(Actor akActor) Global Native

bool Function IsEquippedHeavyWeapon(Actor akActor) Global Native

; 0=normal, 1=heavy, 2=power armor supported, 3=power armor only
int Function GetEquippedPAWeaponState(Actor akActor) Global Native
