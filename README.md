# Grasp Smart Object

> [!IMPORTANT]
> Bridges Grasp interaction abilities with SmartObject GameplayBehaviors
> <br>AI and players share the same Grasp abilities for interactions like sitting, opening doors, etc.
> <br>Depends on: Grasp, SmartObjects, GameplayBehaviors, GameplayAbilities

> [!TIP]
> Players interact via Grasp directly (scan, grant, input, activate)
> <br>AI interacts via SmartObject -> GameplayBehavior -> Grasp ability
> <br>The same ability and montage is used by both paths

## How It Works

### Player Path
1. Grasp scans for nearby graspable components
2. Ability is granted when in range
3. Player presses input to activate
4. Ability plays the interaction montage

No GraspSmartObject involvement. The existing Grasp pipeline handles everything.

### AI Path
1. AI decision system (StateTree, BehaviorTree) claims a SmartObject slot
2. `UAITask_UseGameplayBehaviorSmartObject` triggers `UGameplayBehavior_GraspAbility`
3. The behavior finds the graspable component via `UGraspSmartObjectSlotComponent`
4. The behavior activates the Grasp ability (same one the player would use)
5. `UGraspBehaviorLinkComponent` pairs the behavior and ability for bidirectional cleanup

### Bidirectional Lifecycle

Either the ability or the behavior can end first, and the other is cleaned up automatically:

- **Ability ends** (damage, gameplay event, etc.) -> link component ends the behavior -> SmartObject slot freed
- **Behavior ends** (AI decision, interruption) -> link component cancels the ability -> SmartObject slot freed
- A re-entrancy guard prevents infinite recursion between the two

## Setup

### On the SmartObject Actor (e.g., Bench)

1. Add graspable components (Box, Sphere, etc.) at each interaction point, each with a `UGraspData` asset
2. Add a `USmartObjectComponent` with a definition containing matching slots
3. Add a `UGraspSmartObjectSlotComponent` to map slot indices to graspable components

Example for a 3-seat bench:

```
BenchActor
  StaticMeshComponent (bench mesh)
  SmartObjectComponent (definition with 3 slots)
  GraspSmartObjectSlotComponent
    SlotMappings:
      [0] SlotIndex=0 -> GraspableBox_Left
      [1] SlotIndex=1 -> GraspableBox_Center
      [2] SlotIndex=2 -> GraspableBox_Right
  GraspableBoxComponent "Left"   @ (-100, 0, 45) with DA_Sit
  GraspableBoxComponent "Center" @ (0, 0, 45)    with DA_Sit
  GraspableBoxComponent "Right"  @ (100, 0, 45)  with DA_Sit
```

> [!NOTE]
> Graspable component positions should match their corresponding SmartObject slot offsets
> <br>This ensures the player interaction range aligns with where AI will sit

### On the SmartObject Definition

Each slot's behavior definition should use `UGameplayBehaviorSmartObjectBehaviorDefinition` with a `UGameplayBehaviorConfig_GraspAbility`:

| Config Property | Purpose |
|-----------------|---------|
| `SlotGraspableIndex` | Which entry in `UGraspSmartObjectSlotComponent::SlotMappings` to use |
| `GraspDataIndex` | Which GraspData entry on the graspable component (usually 0) |

### On the Character

Add `UGraspBehaviorLinkComponent` to characters that will use AI-driven SmartObject interactions.

> [!CAUTION]
> Without the link component, the bidirectional cleanup will not work
> <br>The ability will still activate, but interrupting one side won't clean up the other

## Components

### UGraspSmartObjectSlotComponent

Placed on SmartObject actors. Maps slot indices to graspable components via `FGraspSmartObjectSlotMapping`.

| Method | Purpose |
|--------|---------|
| `GetGraspableForSlot(int32 SlotIndex)` | Find the graspable component for a slot |
| `GetSlotForGraspable(UPrimitiveComponent*)` | Find which slot a graspable belongs to |

### UGraspBehaviorLinkComponent

Placed on characters. Manages paired behavior+ability lifecycles.

| Method | Purpose |
|--------|---------|
| `RegisterLink(Behavior, AbilityHandle, GraspableComponent)` | Create a paired link, returns LinkId |
| `EndLink(LinkId, bFromBehavior)` | Tear down a link from either side |
| `FindLinkByBehavior(Behavior)` | Look up LinkId by behavior instance |
| `FindLinkByAbilityHandle(Handle)` | Look up LinkId by ability spec handle |

### UGameplayBehavior_GraspAbility

The behavior subclass triggered by the SmartObject system. You do not call this directly.

On trigger:
1. Finds the graspable component via slot mapping
2. Grants the ability if not already granted
3. Locks the ability (prevents Grasp scan from removing it)
4. Activates the ability
5. Registers the bidirectional link

On end:
1. Ends the link (cancels ability if still active)
2. Removes the ability lock

## Changelog

### 1.0.0
* Initial release
* `UGraspBehaviorLinkComponent` for bidirectional ability/behavior lifecycle
* `UGameplayBehavior_GraspAbility` + `UGameplayBehaviorConfig_GraspAbility`
* `UGraspSmartObjectSlotComponent` for slot-to-graspable mapping
