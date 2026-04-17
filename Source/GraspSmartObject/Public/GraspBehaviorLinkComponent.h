// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "GraspBehaviorLinkComponent.generated.h"

GRASPSMARTOBJECT_API DECLARE_LOG_CATEGORY_EXTERN(LogGraspSmartObject, Log, All);

class UGameplayBehavior;
class UAbilitySystemComponent;
struct FAbilityEndedData;

/**
 * Pairing between an active GameplayBehavior and a Grasp ability.
 * When either ends, the other is cleaned up via the link component.
 */
USTRUCT()
struct FGraspBehaviorLink
{
	GENERATED_BODY()

	TWeakObjectPtr<UGameplayBehavior> Behavior;
	FGameplayAbilitySpecHandle AbilityHandle;
	TWeakObjectPtr<UPrimitiveComponent> GraspableComponent;
	uint32 LinkId = 0;

	bool IsValid() const { return Behavior.IsValid() && AbilityHandle.IsValid(); }
};

/**
 * Bidirectional lifecycle bridge between Grasp abilities and GameplayBehaviors.
 * Place on characters that participate in SmartObject-based Grasp interactions.
 *
 * When the ability ends (damage, gameplay event, etc.), the paired behavior is ended.
 * When the behavior ends (AI decision, interruption), the paired ability is cancelled.
 * A re-entrancy guard prevents infinite recursion between the two.
 */
UCLASS(BlueprintType, ClassGroup = "Gameplay", meta = (BlueprintSpawnableComponent))
class GRASPSMARTOBJECT_API UGraspBehaviorLinkComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGraspBehaviorLinkComponent();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * Register a new paired link between a behavior and ability.
	 * @return The LinkId for this pairing, used to end it later.
	 */
	uint32 RegisterLink(UGameplayBehavior* Behavior, FGameplayAbilitySpecHandle AbilityHandle, UPrimitiveComponent* GraspableComponent);

	/**
	 * End a link from either side.
	 * @param bFromBehavior True if the behavior is ending (cancel the ability). False if ability ended (end the behavior).
	 */
	void EndLink(uint32 LinkId, bool bFromBehavior);

	/** Find the LinkId for a given behavior instance. Returns 0 if not found. */
	uint32 FindLinkByBehavior(const UGameplayBehavior* Behavior) const;

	/** Find the LinkId for a given ability spec handle. Returns 0 if not found. */
	uint32 FindLinkByAbilityHandle(FGameplayAbilitySpecHandle Handle) const;

	/** Check if any links are active. */
	bool HasActiveLinks() const { return ActiveLinks.Num() > 0; }

private:
	void OnAbilityEnded(const FAbilityEndedData& AbilityEndedData);

	UAbilitySystemComponent* GetASC() const;

	UPROPERTY()
	TMap<uint32, FGraspBehaviorLink> ActiveLinks;

	uint32 NextLinkId = 1;

	/** Re-entry guard: prevents ability end -> end behavior -> behavior end -> cancel ability loop */
	bool bIsEndingLink = false;

	FDelegateHandle OnAbilityEndedDelegateHandle;
};
