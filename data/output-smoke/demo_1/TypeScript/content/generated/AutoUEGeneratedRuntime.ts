import UE = require('ue');
import { setupAutoUEGeneratedCamera, updateAutoUEGeneratedSideCamera } from './AutoUEGeneratedCameraHelper';
import { getAutoUEGeneratedSceneManifest } from './AutoUEGeneratedSceneManifest';

const LOG_PREFIX = '[AUTOUE_GENERATED]';
const MOVE_SPEED = 620;
const DASH_SPEED = 1100;
const ENEMY_X = 360;
const TRAP_X = 160;
const EXIT_X = 820;
const ROOM_Y = 0;
const FLOOR_Z = 92;
const MELEE_RADIUS = 185;
const TRAP_RADIUS = 95;
const TRAP_REARM_RADIUS = 135;
const EXIT_RADIUS = 165;
const FREEZE_SECONDS = 1.25;
const CAMERA_SHAKE_SECONDS = 0.35;
const INPUT_PULSE_SECONDS = 0.25;

const INPUT_LEFT = 'AUTOUE_INPUT_LEFT';
const INPUT_RIGHT = 'AUTOUE_INPUT_RIGHT';
const INPUT_RIGHT_1S = 'AUTOUE_INPUT_RIGHT_1S';
const INPUT_RIGHT_3S = 'AUTOUE_INPUT_RIGHT_3S';
const INPUT_DASH = 'AUTOUE_INPUT_DASH';
const INPUT_ATTACK = 'AUTOUE_INPUT_ATTACK';
const INPUT_RESET = 'AUTOUE_INPUT_RESET';

type GeneratedState = {
  enemyHealth: number;
  enemyDefeated: boolean;
  frozenUntil: number;
  trapArmed: boolean;
  trapTriggerCount: number;
  cameraShakeUntil: number;
  exitUnlocked: boolean;
  exitReached: boolean;
  roomComplete: boolean;
  elapsed: number;
};

type HarnessInput = {
  axis: number;
  axisUntil: number;
  dashUntil: number;
  attackPulseUntil: number;
};

function log(message: string): void { console.warn(`${LOG_PREFIX} ${message}`); }
function loc(actor: UE.Actor): UE.Vector { try { return actor.K2_GetActorLocation(); } catch (_) { return new UE.Vector(0, 0, FLOOR_Z); } }
function keyDown(pc: UE.PlayerController | undefined, key: string): boolean { if (!pc) return false; try { return pc.IsInputKeyDown(new UE.Key(key)); } catch (_) { return false; } }
function addTag(actor: UE.Actor, tag: string): void { try { actor.Tags.Add(tag); } catch (_) {} }
function hasTag(actor: UE.Actor, tag: string): boolean { try { return actor.ActorHasTag(tag); } catch (_) { return false; } }
function clearTags(actor: UE.Actor): void { try { (actor.Tags as any).Empty(); } catch (_) {} }
function refreshTags(actor: UE.Actor, state: GeneratedState): void {
  clearTags(actor);
  addTag(actor, 'AUTOUE_GENERATED_ACTIVE');
  if (state.enemyDefeated) addTag(actor, 'AUTOUE_GENERATED_ENEMY_DEFEATED');
  if (state.elapsed < state.frozenUntil) addTag(actor, 'AUTOUE_GENERATED_PLAYER_FROZEN');
  if (!state.trapArmed) addTag(actor, 'AUTOUE_GENERATED_TRAP_SPENT');
  if (state.exitUnlocked) addTag(actor, 'AUTOUE_GENERATED_EXIT_UNLOCKED');
  if (state.roomComplete) addTag(actor, 'AUTOUE_GENERATED_ROOM_COMPLETE');
}
function makeMaterial(owner: UE.Object, r: number, g: number, b: number): UE.MaterialInstanceDynamic | undefined {
  try {
    const base = UE.Material.Load('/Engine/BasicShapes/BasicShapeMaterial');
    if (!base) return undefined;
    const mat = UE.KismetMaterialLibrary.CreateDynamicMaterialInstance(owner, base);
    mat.SetVectorParameterValue('Color', new UE.LinearColor(r, g, b, 1));
    return mat;
  } catch (_) { return undefined; }
}
function configureMesh(c: UE.StaticMeshComponent, owner: UE.Object, scale: UE.Vector, color: UE.LinearColor): void {
  const mesh = UE.StaticMesh.Load('/Engine/BasicShapes/Cube');
  if (mesh) c.SetStaticMesh(mesh);
  c.SetRelativeScale3D(scale);
  const mat = makeMaterial(owner, color.R, color.G, color.B);
  if (mat) c.SetMaterial(0, mat);
  c.SetCollisionEnabled(UE.ECollisionEnabled.QueryOnly);
  try { c.SetGenerateOverlapEvents(true); } catch (_) {}
}
function makeWorldMesh(owner: UE.Character, name: string, world: UE.Vector, scale: UE.Vector, color: UE.LinearColor): UE.StaticMeshComponent | undefined {
  try {
    const c = new UE.StaticMeshComponent(owner, name);
    configureMesh(c, owner, scale, color);
    c.SetAbsolute(true, false, false);
    c.RegisterComponent();
    (c as any).K2_SetWorldLocation(world, false, undefined, true);
    try { c.ComponentTags.Add(name); } catch (_) {}
    return c;
  } catch (e) { log(`WorldMeshCreateFailed name=${name} error=${e}`); return undefined; }
}
function makeAttachedMesh(owner: UE.Character, name: string, relative: UE.Vector, scale: UE.Vector, color: UE.LinearColor): UE.StaticMeshComponent | undefined {
  try {
    const c = new UE.StaticMeshComponent(owner, name);
    configureMesh(c, owner, scale, color);
    c.SetupAttachment(owner.RootComponent, '');
    c.RegisterComponent();
    (c as any).K2_SetRelativeLocation(relative, false, undefined, true);
    c.SetVisibility(false, true);
    try { c.ComponentTags.Add(name); } catch (_) {}
    return c;
  } catch (e) { log(`AttachedMeshCreateFailed name=${name} error=${e}`); return undefined; }
}

export function configureAutoUEGeneratedCharacter(actor: UE.Character): void {
  actor.PrimaryActorTick.bCanEverTick = true;
  actor.PrimaryActorTick.bStartWithTickEnabled = true;
  try { actor.CharacterMovement.MaxWalkSpeed = MOVE_SPEED; } catch (_) {}
  try { (actor.CharacterMovement as any).bRunPhysicsWithNoController = true; } catch (_) {}
  try { (actor.CharacterMovement as any).bConstrainToPlane = true; } catch (_) {}
  try { (actor.CharacterMovement as any).SetPlaneConstraintNormal(new UE.Vector(0, 1, 0)); } catch (_) {}
  try { actor.CapsuleComponent.SetCapsuleSize(48, 96, true); } catch (_) {}
}

function initialState(): GeneratedState {
  return { enemyHealth: 2, enemyDefeated: false, frozenUntil: 0, trapArmed: true, trapTriggerCount: 0, cameraShakeUntil: 0, exitUnlocked: false, exitReached: false, roomComplete: false, elapsed: 0 };
}
function resetGeneratedRuntime(actor: UE.Character): void {
  const self = actor as any;
  self._autoueGeneratedState = initialState();
  self._autoueGeneratedHarnessInput = { axis: 0, axisUntil: 0, dashUntil: 0, attackPulseUntil: 0 } as HarnessInput;
  try { (actor as any).K2_SetActorLocation(new UE.Vector(0, ROOM_Y, FLOOR_Z + 40), false, undefined, true); } catch (_) {}
  try { if (self._autoueGeneratedEnemy) self._autoueGeneratedEnemy.SetVisibility(true, true); } catch (_) {}
  try { if (self._autoueGeneratedFreezeVFX) self._autoueGeneratedFreezeVFX.SetVisibility(false, true); } catch (_) {}
  refreshTags(actor, self._autoueGeneratedState);
  log('RuntimeReset=1');
}

export function beginAutoUEGeneratedCharacter(actor: UE.Character): void {
  const self = actor as any;
  configureAutoUEGeneratedCharacter(actor);
  self._autoueGeneratedEnemyLocation = new UE.Vector(ENEMY_X, ROOM_Y, FLOOR_Z);
  self._autoueGeneratedTrapLocation = new UE.Vector(TRAP_X, ROOM_Y, FLOOR_Z);
  self._autoueGeneratedExitLocation = new UE.Vector(EXIT_X, ROOM_Y, FLOOR_Z);
  self._autoueGeneratedFloor = makeWorldMesh(actor, 'AutoUEGenerated_RoomFloor', new UE.Vector(EXIT_X * 0.5, ROOM_Y, 25), new UE.Vector(10, 2.0, 0.08), new UE.LinearColor(0.18, 0.18, 0.18, 1));
  self._autoueGeneratedEnemy = makeWorldMesh(actor, 'AutoUEGenerated_Enemy', self._autoueGeneratedEnemyLocation, new UE.Vector(0.65, 0.65, 1.15), new UE.LinearColor(1, 0.12, 0.08, 1));
  self._autoueGeneratedTrap = makeWorldMesh(actor, 'AutoUEGenerated_IceTrap', self._autoueGeneratedTrapLocation, new UE.Vector(0.65, 0.65, 0.2), new UE.LinearColor(0.2, 0.8, 1, 1));
  self._autoueGeneratedExit = makeWorldMesh(actor, 'AutoUEGenerated_Exit', self._autoueGeneratedExitLocation, new UE.Vector(0.35, 1.0, 1.65), new UE.LinearColor(0.05, 0.9, 0.15, 1));
  self._autoueGeneratedFreezeVFX = makeAttachedMesh(actor, 'AutoUEGenerated_FreezeVFX', new UE.Vector(0, 0, 80), new UE.Vector(1.4, 1.4, 1.4), new UE.LinearColor(0.25, 0.9, 1, 1));
  setupAutoUEGeneratedCamera(actor, { orthoWidth: 1450, armLength: 1650 });
  resetGeneratedRuntime(actor);
  const manifest = getAutoUEGeneratedSceneManifest({});
  log(`BeginPlay prompt=complex_side_scroller behaviors=${BEHAVIOR_ID} manifestActors=${(manifest.actors as unknown[]).length}`);
  log('HarnessReady tags=AUTOUE_INPUT_RIGHT,AUTOUE_INPUT_RIGHT_1S,AUTOUE_INPUT_RIGHT_3S,AUTOUE_INPUT_LEFT,AUTOUE_INPUT_DASH,AUTOUE_INPUT_ATTACK,AUTOUE_INPUT_RESET');
}

function harness(actor: UE.Character, state: GeneratedState): HarnessInput {
  const self = actor as any;
  let h = self._autoueGeneratedHarnessInput as HarnessInput | undefined;
  if (!h) h = { axis: 0, axisUntil: 0, dashUntil: 0, attackPulseUntil: 0 };
  if (hasTag(actor, INPUT_RESET)) resetGeneratedRuntime(actor);
  if (hasTag(actor, INPUT_LEFT)) { h.axis = -1; h.axisUntil = state.elapsed + INPUT_PULSE_SECONDS; }
  if (hasTag(actor, INPUT_RIGHT)) { h.axis = 1; h.axisUntil = state.elapsed + INPUT_PULSE_SECONDS; }
  if (hasTag(actor, INPUT_RIGHT_1S)) { h.axis = 1; h.axisUntil = state.elapsed + 1.0; }
  if (hasTag(actor, INPUT_RIGHT_3S)) { h.axis = 1; h.axisUntil = state.elapsed + 3.0; }
  if (hasTag(actor, INPUT_DASH)) h.dashUntil = state.elapsed + 0.35;
  if (hasTag(actor, INPUT_ATTACK)) h.attackPulseUntil = state.elapsed + 0.12;
  self._autoueGeneratedHarnessInput = h;
  return h;
}
function readAxis(actor: UE.Character, pc: UE.PlayerController | undefined, state: GeneratedState): number {
  const left = keyDown(pc, 'A') || keyDown(pc, 'Left');
  const right = keyDown(pc, 'D') || keyDown(pc, 'Right');
  const keyAxis = (right ? 1 : 0) - (left ? 1 : 0);
  if (keyAxis) return keyAxis;
  const h = harness(actor, state);
  return state.elapsed <= h.axisUntil ? h.axis : 0;
}
function dashActive(actor: UE.Character, pc: UE.PlayerController | undefined, state: GeneratedState): boolean {
  const h = harness(actor, state);
  return keyDown(pc, 'LeftShift') || keyDown(pc, 'RightShift') || state.elapsed <= h.dashUntil;
}
function attackPressed(actor: UE.Character, pc: UE.PlayerController | undefined, state: GeneratedState): boolean {
  const self = actor as any;
  const h = harness(actor, state);
  const down = keyDown(pc, 'J') || keyDown(pc, 'LeftMouseButton') || keyDown(pc, 'SpaceBar') || state.elapsed <= h.attackPulseUntil;
  const pressed = down && !self._autoueGeneratedAttackDown;
  self._autoueGeneratedAttackDown = down;
  return pressed;
}
function applyMovement(actor: UE.Character, pc: UE.PlayerController | undefined, dt: number, state: GeneratedState): void {
  const axis = readAxis(actor, pc, state);
  if (!axis) return;
  if (state.elapsed < state.frozenUntil) { log(`MoveBlocked reason=frozen remainingMs=${Math.round((state.frozenUntil-state.elapsed)*1000)}`); return; }
  const dash = dashActive(actor, pc, state);
  const speed = dash ? DASH_SPEED : MOVE_SPEED;
  try { (actor as any).K2_AddActorWorldOffset(new UE.Vector(axis * speed * dt, 0, 0), true, undefined, true); } catch (e) { log(`MoveFailed error=${e}`); }
  log(`MoveInput axis=${axis} dash=${dash ? 1 : 0} x=${Math.round(loc(actor).X)}`);
}
function applyTrap(actor: UE.Character, state: GeneratedState): void {
  const distance = Math.abs(loc(actor).X - TRAP_X);
  if (distance > TRAP_REARM_RADIUS) {
    if (!state.trapArmed) log(`IceTrapRearmed distance=${Math.round(distance)}`);
    state.trapArmed = true;
    return;
  }
  if (distance <= TRAP_RADIUS && state.trapArmed && state.elapsed >= state.frozenUntil) {
    state.trapArmed = false;
    state.trapTriggerCount += 1;
    state.frozenUntil = state.elapsed + FREEZE_SECONDS;
    state.cameraShakeUntil = state.elapsed + CAMERA_SHAKE_SECONDS;
    log(`IceTrapTriggered frozen=1 duration=${FREEZE_SECONDS} distance=${Math.round(distance)} triggerCount=${state.trapTriggerCount}`);
    log('CameraShakeTriggered=1');
  }
}
function applyAttack(actor: UE.Character, pc: UE.PlayerController | undefined, state: GeneratedState): void {
  const self = actor as any;
  const pressed = attackPressed(actor, pc, state);
  if (!pressed || state.enemyDefeated) return;
  const distance = Math.abs(loc(actor).X - ENEMY_X);
  log(`MeleeAttack distance=${Math.round(distance)} radius=${MELEE_RADIUS}`);
  if (distance > MELEE_RADIUS) return;
  state.enemyHealth -= 1;
  if (state.enemyHealth <= 0) {
    state.enemyDefeated = true;
    state.exitUnlocked = true;
    try { if (self._autoueGeneratedEnemy) self._autoueGeneratedEnemy.SetVisibility(false, true); } catch (_) {}
    log('EnemyDefeated=1 ExitUnlocked=1');
  } else {
    log(`EnemyHit enemyHealth=${state.enemyHealth}`);
  }
}
function updateExit(actor: UE.Character, state: GeneratedState): void {
  if (!state.exitUnlocked || state.roomComplete) return;
  const distance = Math.abs(loc(actor).X - EXIT_X);
  if (distance <= EXIT_RADIUS) {
    state.exitReached = true;
    state.roomComplete = true;
    log(`ExitReached=1 RoomComplete=1 distance=${Math.round(distance)}`);
  }
}
function updateFeedback(actor: UE.Character, state: GeneratedState): void {
  const self = actor as any;
  const frozen = state.elapsed < state.frozenUntil;
  try { if (self._autoueGeneratedFreezeVFX) self._autoueGeneratedFreezeVFX.SetVisibility(frozen, true); } catch (_) {}
  try {
    const shake = state.elapsed < state.cameraShakeUntil ? Math.sin(state.elapsed * 95) * 18 : 0;
    updateAutoUEGeneratedSideCamera(actor, shake);
  } catch (_) {}
}
export function tickAutoUEGeneratedCharacter(actor: UE.Character, dt: number): void {
  const self = actor as any;
  if (!self._autoueGeneratedState) beginAutoUEGeneratedCharacter(actor);
  const state = self._autoueGeneratedState as GeneratedState;
  state.elapsed += dt;
  const pc = UE.GameplayStatics.GetPlayerController(actor, 0);
  harness(actor, state);
  applyMovement(actor, pc, dt, state);
  applyTrap(actor, state);
  applyAttack(actor, pc, state);
  updateExit(actor, state);
  updateFeedback(actor, state);
  refreshTags(actor, state);
}
export function latestAutoUEGeneratedSnapshot(actor: UE.Actor): Record<string, unknown> | undefined {
  const self = actor as any;
  const state = self._autoueGeneratedState as GeneratedState | undefined;
  if (!state) return undefined;
  const h = self._autoueGeneratedHarnessInput as HarnessInput | undefined;
  let freezeVfxVisible = false;
  try { if (self._autoueGeneratedFreezeVFX) freezeVfxVisible = !!self._autoueGeneratedFreezeVFX.GetVisibleFlag(); } catch (_) {}
  return { ...state, pawnX: loc(actor).X, enemyX: ENEMY_X, trapX: TRAP_X, exitX: EXIT_X, freezeVfxVisible, harnessAxis: h ? h.axis : 0, harnessAxisUntil: h ? h.axisUntil : 0, cameraShakeActive: state.elapsed < state.cameraShakeUntil };
}

const BEHAVIOR_ID = "player.combat.attack";
