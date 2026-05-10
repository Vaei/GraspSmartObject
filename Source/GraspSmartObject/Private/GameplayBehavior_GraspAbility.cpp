// Copyright (c) Jared Taylor

#include "GameplayBehavior_GraspAbility.h"
#include "AITask_UseGraspGameplayBehavior.h"
#include "GraspBehaviorLinkComponent.h"
#include "GraspStatics.h"
#include "GraspTags.h"
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

	const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(&Avatar);
	if (!ASI || !ASI->GetAbilitySystemComponent())
	{
		UE_LOG(LogGraspSmartObject, Warning, TEXT("UGameplayBehavior_GraspAbility::Trigger - Avatar %s has no AbilitySystemComponent"), *GetNameSafe(&Avatar));
		return false;
	}

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();

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

	// Grant the ability if it isn't already on this NPC's ASC. Player-side scanner-driven grant /
	// remove (UGraspComponent) is irrelevant here: AIControllers never have a UGraspComponent, so
	// no scanner ever grants or yanks abilities on NPC ASCs. We grant directly and let the spec
	// persist on the ASC for reuse on subsequent interactions.
	if (!UGraspStatics::FindGraspAbilitySpec(ASC, GraspableComponent, GraspDataIndex))
	{
		FGameplayAbilitySpec NewSpec(AbilityClass, 1, INDEX_NONE, &Avatar);
		NewSpec.SourceObject = GraspableComponent;
		ASC->GiveAbility(NewSpec);
	}

	FGameplayAbilitySpec* Spec = UGraspStatics::FindGraspAbilitySpec(ASC, GraspableComponent, GraspDataIndex);
	if (!Spec)
	{
		UE_LOG(LogGraspSmartObject, Warning, TEXT("UGameplayBehavior_GraspAbility::Trigger - Could not find ability spec after grant on %s"), *GetNameSafe(&Avatar));
		return false;
	}

	// Activate the ability via the ASC directly. We deliberately do NOT route through
	// UGraspStatics::TryActivateGraspAbility because that path requires a UGraspComponent on the
	// SourceActor's controller (FindGraspComponentForActor walks Pawn -> PlayerState -> Controller
	// and ensure-fails if missing). UGraspComponent is the player-side scanner; AIControllers
	// don't have one. Bypassing it skips the player-only Pre/PostActivate notifications and
	// ability-lock tracking, neither of which apply to NPCs.
	FGameplayAbilityActorInfo* ActorInfo = ASC->AbilityActorInfo.Get();
	FGameplayEventData Payload;
	bool bActivated = false;

	if (UGraspStatics::PrepareGraspAbilityDataPayload(GraspableComponent, Payload, &Avatar, ActorInfo,
		EGraspAbilityComponentSource::EventData, GraspDataIndex))
	{
		bActivated = ASC->TriggerAbilityFromGameplayEvent(Spec->Handle, ActorInfo,
			FGraspTags::Grasp_Interact_Activate, &Payload, *ASC);
	}
	else
	{
		bActivated = ASC->TryActivateAbility(Spec->Handle, /*bAllowRemoteActivation*/ true);
	}

	if (!bActivated)
	{
		UE_LOG(LogGraspSmartObject, Warning, TEXT("UGameplayBehavior_GraspAbility::Trigger - Failed to activate Grasp ability on %s"), *GetNameSafe(&Avatar));
		return false;
	}

	// Register the bidirectional link so behavior end -> ability cancel and ability end -> behavior
	// end stay in sync. The link component lives on the pawn and is the only on-pawn glue needed.
	if (UGraspBehaviorLinkComponent* LinkComp = Avatar.FindComponentByClass<UGraspBehaviorLinkComponent>())
	{
		ActiveLinkId = LinkComp->RegisterLink(this, Spec->Handle, GraspableComponent);
	}
	else
	{
		UE_LOG(LogGraspSmartObject, Warning, TEXT("UGameplayBehavior_GraspAbility::Trigger - Avatar %s has no UGraspBehaviorLinkComponent; ability will run but interruption won't propagate"), *GetNameSafe(&Avatar));
	}

	// Skip Super::Trigger: the base implementation only dispatches K2_OnTriggered* BP events and
	// returns bTransientIsActive, which stays false unless one of the bTrigger* flags is set
	// (those auto-flip only for BP subclasses that implement the corresponding event). Stamp the
	// transient state ourselves so callers see the trigger as successful.
	TransientAvatar = &Avatar;
	TransientSmartObjectOwner = SmartObjectOwner;
	bTransientIsActive = true;
	return true;
}

void UGameplayBehavior_GraspAbility::EndBehavior(AActor& Avatar, const bool bInterrupted)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UGameplayBehavior_GraspAbility::EndBehavior);

	if (ActiveLinkId != 0)
	{
		if (UGraspBehaviorLinkComponent* LinkComp = Avatar.FindComponentByClass<UGraspBehaviorLinkComponent>())
		{
			LinkComp->EndLink(ActiveLinkId, true);
		}
		ActiveLinkId = 0;
	}

	Super::EndBehavior(Avatar, bInterrupted);
}
