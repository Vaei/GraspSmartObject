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

	/** Index into the SmartObject actor's UGraspSmartObjectSlotComponent mappings to find the graspable component for this slot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grasp")
	int32 SlotGraspableIndex = 0;

	/** GraspData index on the graspable component. Usually 0 unless the component has multiple interaction options. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grasp")
	int32 GraspDataIndex = 0;
};

/**
 * GameplayBehavior that activates a Grasp ability when triggered by AI via SmartObjects.
 *
 * Lifecycle:
 * 1. AI claims SmartObject slot, UAITask_UseGameplayBehaviorSmartObject triggers this behavior
 * 2. Trigger() finds the graspable component via UGraspSmartObjectSlotComponent, activates the Grasp ability
 * 3. Registers a bidirectional link via UGraspBehaviorLinkComponent
 * 4. When either the behavior or ability ends, the other is cleaned up automatically
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
