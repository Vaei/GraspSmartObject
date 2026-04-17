// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GraspSmartObjectSlotComponent.generated.h"

/**
 * Maps a SmartObject slot index to a graspable component on the same actor.
 */
USTRUCT(BlueprintType)
struct GRASPSMARTOBJECT_API FGraspSmartObjectSlotMapping
{
	GENERATED_BODY()

	/** Index into the SmartObjectDefinition's slot array. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grasp")
	int32 SmartObjectSlotIndex = 0;

	/** Reference to the graspable component (IGraspableComponent) on this actor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grasp", meta = (UseComponentPicker))
	FComponentReference GraspableComponentRef;
};

/**
 * Maps SmartObject slot indices to graspable components on the same actor.
 * Place on actors that have both a USmartObjectComponent and graspable components (e.g., a bench).
 *
 * Each slot mapping associates a SmartObject slot index with a specific graspable component,
 * so the AI behavior knows which graspable to activate when using a particular slot.
 */
UCLASS(BlueprintType, ClassGroup = "Gameplay", meta = (BlueprintSpawnableComponent))
class GRASPSMARTOBJECT_API UGraspSmartObjectSlotComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grasp")
	TArray<FGraspSmartObjectSlotMapping> SlotMappings;

	/** Find the graspable component for a given slot index. */
	UFUNCTION(BlueprintCallable, Category = "Grasp")
	UPrimitiveComponent* GetGraspableForSlot(int32 SlotIndex) const;

	/** Find the slot index for a given graspable component. Returns INDEX_NONE if not found. */
	UFUNCTION(BlueprintCallable, Category = "Grasp")
	int32 GetSlotForGraspable(const UPrimitiveComponent* GraspableComponent) const;
};
