// Copyright (c) Jared Taylor

#include "GameplayBehavior_GraspAbility.h"
#include "GraspBehaviorLinkComponent.h"
#include "GraspSmartObjectSlotComponent.h"
#include "GraspStatics.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GraspData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameplayBehavior_GraspAbility)

// Defined in GraspBehaviorLinkComponent.cpp
DECLARE_LOG_CATEGORY_EXTERN(LogGraspSmartObject, Log, All);


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

	// Find the graspable component via slot mapping on the SmartObject actor
	UGraspSmartObjectSlotComponent* SlotMapping = SmartObjectOwner->FindComponentByClass<UGraspSmartObjectSlotComponent>();
	if (!SlotMapping)
	{
		UE_LOG(LogGraspSmartObject, Warning, TEXT("UGameplayBehavior_GraspAbility::Trigger - SmartObjectOwner %s has no UGraspSmartObjectSlotComponent"), *GetNameSafe(SmartObjectOwner));
		return false;
	}

	UPrimitiveComponent* GraspableComponent = SlotMapping->GetGraspableForSlot(GraspConfig->SlotGraspableIndex);
	if (!GraspableComponent)
	{
		UE_LOG(LogGraspSmartObject, Warning, TEXT("UGameplayBehavior_GraspAbility::Trigger - No graspable component found for slot index %d on %s"), GraspConfig->SlotGraspableIndex, *GetNameSafe(SmartObjectOwner));
		return false;
	}

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
	const UGraspData* GraspData = UGraspStatics::GetGraspData(GraspableComponent, GraspConfig->GraspDataIndex);
	if (!GraspData)
	{
		UE_LOG(LogGraspSmartObject, Warning, TEXT("UGameplayBehavior_GraspAbility::Trigger - No GraspData at index %d on %s"), GraspConfig->GraspDataIndex, *GetNameSafe(GraspableComponent));
		return false;
	}

	const TSubclassOf<UGameplayAbility> AbilityClass = GraspData->GetGraspAbility();
	if (!AbilityClass)
	{
		UE_LOG(LogGraspSmartObject, Warning, TEXT("UGameplayBehavior_GraspAbility::Trigger - GraspData has no ability class"));
		return false;
	}

	// Grant the ability if not already present
	FGameplayAbilitySpec* ExistingSpec = UGraspStatics::FindGraspAbilitySpec(ASC, GraspableComponent, GraspConfig->GraspDataIndex);
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
		EGraspAbilityComponentSource::EventData, GraspConfig->GraspDataIndex);

	if (!bActivated)
	{
		UE_LOG(LogGraspSmartObject, Warning, TEXT("UGameplayBehavior_GraspAbility::Trigger - Failed to activate Grasp ability on %s"), *GetNameSafe(&Avatar));
		UGraspStatics::RemoveGraspAbilityLock(&Avatar, GraspableComponent);
		return false;
	}

	// Get the ability spec handle for the link
	FGameplayAbilitySpec* Spec = UGraspStatics::FindGraspAbilitySpec(ASC, GraspableComponent, GraspConfig->GraspDataIndex);
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
