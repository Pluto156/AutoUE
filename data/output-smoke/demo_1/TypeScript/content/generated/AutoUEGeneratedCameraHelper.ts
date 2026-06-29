import UE = require('ue');

export interface AutoUEGeneratedCameraOptions {
  orthoWidth?: number;
  armLength?: number;
}

const DEFAULT_SIDE_CAMERA_DISTANCE = 1650;
const SIDE_CAMERA_HEIGHT = 260;
const SIDE_CAMERA_YAW = 90;

function placeSideCamera(actor: UE.Character, boom: UE.SpringArmComponent, shakeZ: number = 0): void {
  const self = actor as any;
  const sideDistance = self._autoueGeneratedCameraSideDistance || DEFAULT_SIDE_CAMERA_DISTANCE;
  const base = actor.K2_GetActorLocation();
  try { (boom as any).SetAbsolute(true, true, false); } catch (_) {}
  try { (boom as any).K2_SetWorldLocation(new UE.Vector(base.X, base.Y - sideDistance, base.Z + SIDE_CAMERA_HEIGHT + shakeZ), false, undefined, false); } catch (_) {}
  try { (boom as any).K2_SetWorldRotation(new UE.Rotator(0, SIDE_CAMERA_YAW, 0), false); } catch (_) {}
}

export function updateAutoUEGeneratedSideCamera(actor: UE.Character, shakeZ: number = 0): void {
  const self = actor as any;
  const boom = self._autoueGeneratedCameraArm as UE.SpringArmComponent | undefined;
  if (boom) placeSideCamera(actor, boom, shakeZ);
}

export function setupAutoUEGeneratedCamera(actor: UE.Character, options: AutoUEGeneratedCameraOptions = {}): UE.CameraComponent | undefined {
  try {
    const self = actor as any;
    if (self._autoueGeneratedCamera) return self._autoueGeneratedCamera as UE.CameraComponent;
    const sideDistance = options.armLength || DEFAULT_SIDE_CAMERA_DISTANCE;
    self._autoueGeneratedCameraSideDistance = sideDistance;
    const boom = new UE.SpringArmComponent(actor, 'AutoUEGenerated_SideCameraArm');
    boom.SetupAttachment(actor.RootComponent, '');
    boom.TargetArmLength = 0;
    (boom as any).bDoCollisionTest = false;
    boom.RegisterComponent();
    placeSideCamera(actor, boom, 0);
    const cam = new UE.CameraComponent(actor, 'AutoUEGenerated_SideCamera');
    cam.SetupAttachment(boom, '');
    (cam as any).K2_SetRelativeLocation(new UE.Vector(0, 0, 0), false, undefined, false);
    (cam as any).K2_SetRelativeRotation(new UE.Rotator(0, 0, 0), false);
    cam.SetProjectionMode(UE.ECameraProjectionMode.Orthographic);
    cam.SetOrthoWidth(options.orthoWidth || 1450);
    cam.RegisterComponent();
    try { cam.Activate(true); } catch (_) {}
    self._autoueGeneratedCameraArm = boom;
    self._autoueGeneratedCamera = cam;
    const pc = UE.GameplayStatics.GetPlayerController(actor, 0);
    if (pc) {
      try { (pc as any).bAutoManageActiveCameraTarget = false; } catch (_) {}
      try { actor.EnableInput(pc); } catch (_) {}
      try { pc.SetViewTargetWithBlend(actor, 0, UE.EViewTargetBlendFunction.VTBlend_Linear, 0, false); } catch (_) {}
    }
    console.warn(`[AUTOUE_GENERATED] CameraReady viewTarget=${!!pc} sideOffsetY=${sideDistance} yaw=${SIDE_CAMERA_YAW} flow=${FLOW_ID}`);
    return cam;
  } catch (e) {
    console.warn(`[AUTOUE_GENERATED] CameraSetupFailed error=${e}`);
    return undefined;
  }
}

const FLOW_ID = "flow_player_combat_attack";
