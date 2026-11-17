# Hold and drop

Picking an object up puts it on a fixed point in front of the camera. Looking and walking move that point, and the object stays there. It does not swing. Collision stays on, so it stops against walls and pushes other loose objects.

Letting go leaves it where it is. It does not keep your walking speed. Walking over a dropped key or weapon does not push it through the floor. Those objects do not collide with players. They still collide with the floor, walls, and other loose objects. Doors and cabinets still stop a player.

Weight does not change the hold. It changes how long the pickup takes, and how fast you walk while carrying it. An object with no mass set picks up immediately and does not slow you.

F picks up. G lets go.

## Where to change the numbers

On the object Blueprint, select the mesh. That is the physical shape.

- Collision Enabled: Query and Physics
- Collision Preset: PhysicsActor
- Simulate Physics: on
- Mass (kg): the weight, with the box beside it ticked

Open the mesh asset and use Collision → Auto Convex Collision so the shape fits the mesh. A box or capsule is enough when the auto hull looks wrong.

PickupVolume is only the look target. Collision Enabled is Query Only. Simulate Physics is off.

On the object's Interactable component, under Hold:

- Hold Distance: 80
- Hold Height: -15
- Bump Force: 20000

On BP_FirstPersonCharacter, Class Defaults, under Carry:

- Heavy Mass Kg: 20
- Slowest Walk Scale: 0.5
- Light Pickup Seconds: 0.05
- Heavy Pickup Seconds: 1.2

A 10 kg object takes about 0.6 seconds to pick up and slows you to about three quarters speed. A 20 kg object takes 1.2 seconds and slows you to half speed. Heavier than that stays at half speed.

## What owns it

`UMurderAtMidnightInteractableComponent` places, drops, and keeps the object off the player. `AMurderAtMidnightKey` and `AMurderAtMidnightWeapon` are the test objects. The mesh is the root. The carry numbers are on `AMurderAtMidnightCharacter`.

The server decides the pickup and the drop. Clients ask.

## Leave this alone unless asked

Do not put simulation back on while the object is in the hand. Do not let a player collision push a dropped key or weapon. Do not store who last held an object on the object.
