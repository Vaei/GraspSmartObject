// Copyright (c) Jared Taylor

#pragma once

#include "CoreMinimal.h"
#include "AI/AITask_UseGameplayBehaviorSmartObject.h"
#include "AITask_UseGraspGameplayBehavior.generated.h"

class AAIController;

/**
 * AI task for triggering Grasp interactions through SmartObjects.
 *
 * Subclass of UAITask_UseGameplayBehaviorSmartObject that exposes the active claim handle
 * to UGameplayBehavior_GraspAbility::Trigger via a thread-safe stash. The behavior reads
 * the claim handle to resolve the SmartObjectComponent (and thus the graspable, which is
 * the SO's attachment parent) and the slot index that maps to the GraspData index.
 *
 * Use the static factories on this class instead of the engine ones for grasp interactions.
 */
UCLASS()
class GRASPSMARTOBJECT_API UAITask_UseGraspGameplayBehavior : public UAITask_UseGameplayBehaviorSmartObject
{
	GENERATED_BODY()

public:
	/** Reaches and uses an already-claimed grasp slot. */
	UFUNCTION(BlueprintCallable, Category = "AI|Tasks", meta = (DefaultToSelf = "Controller", BlueprintInternalUseOnly = "true"))
	static UAITask_UseGraspGameplayBehavior* UseGraspSmartObjectWithGameplayBehavior(AAIController* Controller, FSmartObjectClaimHandle ClaimHandle, bool bLockAILogic = true, ESmartObjectClaimPriority ClaimPriority = ESmartObjectClaimPriority::Normal);

	/** Moves to and uses an already-claimed grasp slot. */
	UFUNCTION(BlueprintCallable, Category = "AI|Tasks", meta = (DefaultToSelf = "Controller", BlueprintInternalUseOnly = "true"))
	static UAITask_UseGraspGameplayBehavior* MoveToAndUseGraspSmartObjectWithGameplayBehavior(AAIController* Controller, FSmartObjectClaimHandle ClaimHandle, bool bLockAILogic = true, ESmartObjectClaimPriority ClaimPriority = ESmartObjectClaimPriority::Normal);

	/** The claim handle this task is operating on. Valid once Activate has been called. */
	const FSmartObjectClaimHandle& GetClaimHandle() const { return ClaimedHandle; }

	/** The task currently invoking TriggerBehavior (synchronous reentrant lookup for the behavior). */
	static UAITask_UseGraspGameplayBehavior* GetTriggeringTask() { return ActiveTriggeringTask; }

protected:
	virtual void Activate() override;
	virtual void OnGameplayTaskDeactivated(UGameplayTask& Task) override;

private:
	static UAITask_UseGraspGameplayBehavior* ActiveTriggeringTask;
};
