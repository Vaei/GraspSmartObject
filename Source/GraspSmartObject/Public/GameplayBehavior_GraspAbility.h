// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "GameplayBehavior.h"
#include "GameplayBehaviorConfig.h"
#include "GameplayBehavior_GraspAbility.generated.h"

class UGameplayAbility;

/**
 * Config for UGameplayBehavior_GraspAbility.
 * Set on a UGameplayBehaviorSmartObjectBehaviorDefinition for SmartObject slots
 * that should activate Grasp abilities on use.
 */
UCLASS(BlueprintType, EditInlineNew)
class GRASPSMARTOBJECT_API UGameplayBehaviorConfig_GraspAbility : public UGameplayBehaviorConfig
{
	GENERATED_BODY()

public:
	UGameplayBehaviorConfig_GraspAbility();
};

/**
 * GameplayBehavior that activates a Grasp ability when triggered by AI via SmartObjects.
 *
 * Lifecycle:
 * 1. AI claims a SmartObject slot via UAITask_UseGraspGameplayBehavior (subclass of the engine task).
 * 2. The task triggers this behavior; Trigger reads the active task's claim handle to resolve the
 *    SmartObjectComponent. The graspable is the SO's attachment parent. The slot index from the
 *    claim handle is the GraspData index (positional mapping enforced by AhoyGraspableValidator).
 * 3. Registers a bidirectional link via UGraspBehaviorLinkComponent.
 * 4. When either the behavior or ability ends, the other is cleaned up automatically.
 */
UCLASS(BlueprintType, Blueprintable)
class GRASPSMARTOBJECT_API UGameplayBehavior_GraspAbility : public UGameplayBehavior
{
	GENERATED_BODY()

public:
	virtual bool Trigger(AActor& Avatar, const UGameplayBehaviorConfig* Config = nullptr, AActor* SmartObjectOwner = nullptr) override;
	virtual void EndBehavior(AActor& Avatar, const bool bInterrupted = false) override;

private:
	/** The link ID registered with UGraspBehaviorLinkComponent, for cleanup on EndBehavior. */
	uint32 ActiveLinkId = 0;

	/** Cached graspable component for ability lock cleanup. */
	TWeakObjectPtr<UPrimitiveComponent> CachedGraspableComponent;
};
