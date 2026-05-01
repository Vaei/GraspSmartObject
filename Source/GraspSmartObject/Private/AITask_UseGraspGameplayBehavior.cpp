// Copyright (c) Jared Taylor

#include "AITask_UseGraspGameplayBehavior.h"

#include "AIController.h"
#include "Misc/ScopeExit.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AITask_UseGraspGameplayBehavior)

UAITask_UseGraspGameplayBehavior* UAITask_UseGraspGameplayBehavior::ActiveTriggeringTask = nullptr;

UAITask_UseGraspGameplayBehavior* UAITask_UseGraspGameplayBehavior::UseGraspSmartObjectWithGameplayBehavior(AAIController* Controller, FSmartObjectClaimHandle ClaimHandle, bool bLockAILogic, ESmartObjectClaimPriority ClaimPriority)
{
	if (!Controller || !ClaimHandle.IsValid())
	{
		return nullptr;
	}

	UAITask_UseGraspGameplayBehavior* Task = UAITask::NewAITask<UAITask_UseGraspGameplayBehavior>(*Controller, EAITaskPriority::High);
	if (Task)
	{
		Task->SetClaimHandle(ClaimHandle);
		Task->SetShouldReachSlotLocation(false);
		if (bLockAILogic)
		{
			Task->RequestAILogicLocking();
		}
	}
	return Task;
}

UAITask_UseGraspGameplayBehavior* UAITask_UseGraspGameplayBehavior::MoveToAndUseGraspSmartObjectWithGameplayBehavior(AAIController* Controller, FSmartObjectClaimHandle ClaimHandle, bool bLockAILogic, ESmartObjectClaimPriority ClaimPriority)
{
	if (!Controller || !ClaimHandle.IsValid())
	{
		return nullptr;
	}

	UAITask_UseGraspGameplayBehavior* Task = UAITask::NewAITask<UAITask_UseGraspGameplayBehavior>(*Controller, EAITaskPriority::High);
	if (Task)
	{
		Task->SetClaimHandle(ClaimHandle);
		Task->SetShouldReachSlotLocation(true);
		if (bLockAILogic)
		{
			Task->RequestAILogicLocking();
		}
	}
	return Task;
}

void UAITask_UseGraspGameplayBehavior::Activate()
{
	UAITask_UseGraspGameplayBehavior* PreviousTask = ActiveTriggeringTask;
	ActiveTriggeringTask = this;
	ON_SCOPE_EXIT { ActiveTriggeringTask = PreviousTask; };

	Super::Activate();
}

void UAITask_UseGraspGameplayBehavior::OnGameplayTaskDeactivated(UGameplayTask& Task)
{
	UAITask_UseGraspGameplayBehavior* PreviousTask = ActiveTriggeringTask;
	ActiveTriggeringTask = this;
	ON_SCOPE_EXIT { ActiveTriggeringTask = PreviousTask; };

	Super::OnGameplayTaskDeactivated(Task);
}
