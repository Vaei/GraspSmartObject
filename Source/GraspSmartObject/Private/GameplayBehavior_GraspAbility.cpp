// Copyright (c) Jared Taylor

#include "GameplayBehavior_GraspAbility.h"
#include "AITask_UseGraspGameplayBehavior.h"
#include "GraspBehaviorLinkComponent.h"
#include "GraspStatics.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GraspData.h"
#include "SmartObjectComponent.h"
#include "SmartObjectSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameplayBehavior_GraspAbility)

UGameplayBehaviorConfig_GraspAbility::UGameplayBehaviorConfig_GraspAbility()
{
	BehaviorClass = UGameplayBehavior_GraspAbility::StaticClass();
}

bool UGameplayBehavior_GraspAbility::Trigger(AActor& Avatar, const UGameplayBehaviorConfig* Config, AActor* SmartObjectOwner)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UGameplayBehavior_GraspAbility::Trigger);

	const UGameplayBehaviorConfig_GraspAbility* GraspConfig = Cast<UGameplayBehaviorConfig_GraspAbility>(Config);
	if (!GraspConfig)
	{
		UE_LOG(LogGraspSmartObject, Warning, TEXT("UGameplayBehavior_GraspAbility::Trigger - Missing or wrong config type for %s"), *GetNameSafe(&Avatar));
		return false;
	}

	if (!SmartObjectOwner)
	{
		UE_LOG(LogGraspSmartObject, Warning, TEXT("UGameplayBehavior_GraspAbility::Trigger - No SmartObjectOwner for %s"), *GetNameSafe(&Avatar));
		return false;
	}

	// Resolve the SO instance the AI claimed via the active task's claim handle.
	const UAITask_UseGraspGameplayBehavior* GraspTask = UAITask_UseGraspGameplayBehavior::GetTriggeringTask();
	if (!GraspTask)
	{
		UE_LOG(LogGraspSmartObject, Warning, TEXT("UGameplayBehavior_GraspAbility::Trigger - No active UAITask_UseGraspGameplayBehavior; AI must use that task class to trigger grasp behaviors"));
		return false;
	}

	const FSmartObjectClaimHandle& ClaimHandle = GraspTask->GetClaimHandle();
	USmartObjectSubsystem* SOSubsystem = USmartObjectSubsystem::GetCurrent(SmartObjectOwner->GetWorld());
	const USmartObjectComponent* SOComponent = SOSubsystem ? SOSubsystem->GetSmartObjectComponent(ClaimHandle) : nullptr;
	if (!SOComponent)
	{
		UE_LOG(LogGraspSmartObject, Warning, TEXT("UGameplayBehavior_GraspAbility::Trigger - Could not resolve SmartObjectComponent from claim handle on %s"), *GetNameSafe(SmartObjectOwner));
		return false;
	}

	UPrimitiveComponent* GraspableComponent = Cast<UPrimitiveComponent>(SOComponent->GetAttachParent());
	if (!GraspableComponent)
	{
		UE_LOG(LogGraspSmartObject, Warning, TEXT("UGameplayBehavior_GraspAbility::Trigger - SmartObjectComponent %s is not attached beneath a graspable component"), *GetNameSafe(SOComponent));
		return false;
	}

	const int32 GraspDataIndex = ClaimHandle.SlotHandle.GetSlotIndex();

	CachedGraspableComponent = GraspableComponent;

	// The ASC should be on the Avatar (character)
	const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(&Avatar);
	if (!ASI || !ASI->GetAbilitySystemComponent())
	{
		UE_LOG(LogGraspSmartObject, Warning, TEXT("UGameplayBehavior_GraspAbility::Trigger - Avatar %s has no AbilitySystemComponent"), *GetNameSafe(&Avatar));
		return false;
	}

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();

	// Check if the ability is already granted via Grasp scanning.
	// If not, grant it directly so AI can use it regardless of scanning.
	const UGraspData* GraspData = UGraspStatics::GetGraspData(GraspableComponent, GraspDataIndex);
	if (!GraspData)
	{
		UE_LOG(LogGraspSmartObject, Warning, TEXT("UGameplayBehavior_GraspAbility::Trigger - No GraspData at index %d on %s"), GraspDataIndex, *GetNameSafe(GraspableComponent));
		return false;
	}

	const TSubclassOf<UGameplayAbility> AbilityClass = GraspData->GetGraspAbility();
	if (!AbilityClass)
	{
		UE_LOG(LogGraspSmartObject, Warning, TEXT("UGameplayBehavior_GraspAbility::Trigger - GraspData has no ability class"));
		return false;
	}

	// Grant the ability if not already present
	FGameplayAbilitySpec* ExistingSpec = UGraspStatics::FindGraspAbilitySpec(ASC, GraspableComponent, GraspDataIndex);
	if (!ExistingSpec)
	{
		FGameplayAbilitySpec NewSpec(AbilityClass, 1, INDEX_NONE, &Avatar);
		NewSpec.SourceObject = GraspableComponent;
		ASC->GiveAbility(NewSpec);
	}

	// Lock the ability so Grasp scanning doesn't remove it mid-interaction
	UGraspStatics::AddGraspAbilityLock(&Avatar, GraspableComponent);

	// Activate the ability
	const bool bActivated = UGraspStatics::TryActivateGraspAbility(&Avatar, GraspableComponent,
		EGraspAbilityComponentSource::EventData, GraspDataIndex);

	if (!bActivated)
	{
		UE_LOG(LogGraspSmartObject, Warning, TEXT("UGameplayBehavior_GraspAbility::Trigger - Failed to activate Grasp ability on %s"), *GetNameSafe(&Avatar));
		UGraspStatics::RemoveGraspAbilityLock(&Avatar, GraspableComponent);
		return false;
	}

	// Get the ability spec handle for the link
	FGameplayAbilitySpec* Spec = UGraspStatics::FindGraspAbilitySpec(ASC, GraspableComponent, GraspDataIndex);
	if (!Spec)
	{
		UE_LOG(LogGraspSmartObject, Warning, TEXT("UGameplayBehavior_GraspAbility::Trigger - Could not find ability spec after activation"));
		UGraspStatics::RemoveGraspAbilityLock(&Avatar, GraspableComponent);
		return false;
	}

	// Register the bidirectional link
	UGraspBehaviorLinkComponent* LinkComp = Avatar.FindComponentByClass<UGraspBehaviorLinkComponent>();
	if (!LinkComp)
	{
		UE_LOG(LogGraspSmartObject, Warning, TEXT("UGameplayBehavior_GraspAbility::Trigger - Avatar %s has no UGraspBehaviorLinkComponent"), *GetNameSafe(&Avatar));
		// Still works, just no bidirectional cleanup
	}
	else
	{
		ActiveLinkId = LinkComp->RegisterLink(this, Spec->Handle, GraspableComponent);
	}

	// Call parent to set up transient state and fire BP events
	return Super::Trigger(Avatar, Config, SmartObjectOwner);
}

void UGameplayBehavior_GraspAbility::EndBehavior(AActor& Avatar, const bool bInterrupted)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UGameplayBehavior_GraspAbility::EndBehavior);

	// End the link (which may cancel the ability if it's still running)
	if (ActiveLinkId != 0)
	{
		if (UGraspBehaviorLinkComponent* LinkComp = Avatar.FindComponentByClass<UGraspBehaviorLinkComponent>())
		{
			LinkComp->EndLink(ActiveLinkId, true);
		}
		ActiveLinkId = 0;
	}

	// Remove the ability lock
	if (UPrimitiveComponent* Graspable = CachedGraspableComponent.Get())
	{
		UGraspStatics::RemoveGraspAbilityLock(&Avatar, Graspable);
	}
	CachedGraspableComponent.Reset();

	Super::EndBehavior(Avatar, bInterrupted);
}
