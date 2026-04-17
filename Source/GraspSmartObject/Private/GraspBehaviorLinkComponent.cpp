// Copyright (c) Jared Taylor

#include "GraspBehaviorLinkComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayBehavior.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GraspBehaviorLinkComponent)

DECLARE_LOG_CATEGORY_EXTERN(LogGraspSmartObject, Log, All);
DEFINE_LOG_CATEGORY(LogGraspSmartObject);

UGraspBehaviorLinkComponent::UGraspBehaviorLinkComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGraspBehaviorLinkComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Tear down all active links
	if (ActiveLinks.Num() > 0)
	{
		// Copy keys to avoid modifying map during iteration
		TArray<uint32> LinkIds;
		ActiveLinks.GetKeys(LinkIds);

		for (const uint32 LinkId : LinkIds)
		{
			EndLink(LinkId, false);
		}
	}

	// Unbind from ASC
	if (OnAbilityEndedDelegateHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = GetASC())
		{
			ASC->OnAbilityEnded.Remove(OnAbilityEndedDelegateHandle);
		}
		OnAbilityEndedDelegateHandle.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

UAbilitySystemComponent* UGraspBehaviorLinkComponent::GetASC() const
{
	if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		return ASI->GetAbilitySystemComponent();
	}
	return nullptr;
}

uint32 UGraspBehaviorLinkComponent::RegisterLink(UGameplayBehavior* Behavior, FGameplayAbilitySpecHandle AbilityHandle, UPrimitiveComponent* GraspableComponent)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UGraspBehaviorLinkComponent::RegisterLink);

	if (!Behavior || !AbilityHandle.IsValid())
	{
		UE_LOG(LogGraspSmartObject, Warning, TEXT("RegisterLink: Invalid behavior or ability handle"));
		return 0;
	}

	// Bind to ASC ability ended if not already bound
	if (!OnAbilityEndedDelegateHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = GetASC())
		{
			OnAbilityEndedDelegateHandle = ASC->OnAbilityEnded.AddUObject(this, &UGraspBehaviorLinkComponent::OnAbilityEnded);
		}
		else
		{
			UE_LOG(LogGraspSmartObject, Warning, TEXT("RegisterLink: No AbilitySystemComponent on owner %s"), *GetNameSafe(GetOwner()));
			return 0;
		}
	}

	const uint32 LinkId = NextLinkId++;

	FGraspBehaviorLink& Link = ActiveLinks.Add(LinkId);
	Link.Behavior = Behavior;
	Link.AbilityHandle = AbilityHandle;
	Link.GraspableComponent = GraspableComponent;
	Link.LinkId = LinkId;

	UE_LOG(LogGraspSmartObject, Verbose, TEXT("RegisterLink: Created link %u (Behavior: %s, AbilityHandle: %s)"),
		LinkId, *GetNameSafe(Behavior), *AbilityHandle.ToString());

	return LinkId;
}

void UGraspBehaviorLinkComponent::EndLink(uint32 LinkId, bool bFromBehavior)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UGraspBehaviorLinkComponent::EndLink);

	if (bIsEndingLink)
	{
		// Re-entrancy: we're already tearing down a link, don't recurse
		return;
	}

	FGraspBehaviorLink* Link = ActiveLinks.Find(LinkId);
	if (!Link)
	{
		return;
	}

	// Copy data before removing from map
	const FGraspBehaviorLink LinkCopy = *Link;
	ActiveLinks.Remove(LinkId);

	bIsEndingLink = true;

	if (bFromBehavior)
	{
		// Behavior is ending -> cancel the ability
		if (LinkCopy.AbilityHandle.IsValid())
		{
			if (UAbilitySystemComponent* ASC = GetASC())
			{
				UE_LOG(LogGraspSmartObject, Verbose, TEXT("EndLink %u: Behavior ended, cancelling ability %s"),
					LinkId, *LinkCopy.AbilityHandle.ToString());
				ASC->CancelAbilityHandle(LinkCopy.AbilityHandle);
			}
		}
	}
	else
	{
		// Ability ended -> end the behavior
		if (UGameplayBehavior* Behavior = LinkCopy.Behavior.Get())
		{
			if (AActor* Avatar = GetOwner())
			{
				UE_LOG(LogGraspSmartObject, Verbose, TEXT("EndLink %u: Ability ended, ending behavior %s"),
					LinkId, *GetNameSafe(Behavior));
				Behavior->EndBehavior(*Avatar, true);
			}
		}
	}

	bIsEndingLink = false;

	// If no more links, we could unbind from ASC, but keeping it bound is cheap
}

void UGraspBehaviorLinkComponent::OnAbilityEnded(const FAbilityEndedData& AbilityEndedData)
{
	if (bIsEndingLink)
	{
		return;
	}

	const uint32 LinkId = FindLinkByAbilityHandle(AbilityEndedData.AbilitySpecHandle);
	if (LinkId != 0)
	{
		UE_LOG(LogGraspSmartObject, Verbose, TEXT("OnAbilityEnded: Ability %s ended (cancelled: %s), ending link %u"),
			*AbilityEndedData.AbilitySpecHandle.ToString(),
			AbilityEndedData.bWasCancelled ? TEXT("true") : TEXT("false"),
			LinkId);
		EndLink(LinkId, false);
	}
}

uint32 UGraspBehaviorLinkComponent::FindLinkByBehavior(const UGameplayBehavior* Behavior) const
{
	for (const auto& Pair : ActiveLinks)
	{
		if (Pair.Value.Behavior.Get() == Behavior)
		{
			return Pair.Key;
		}
	}
	return 0;
}

uint32 UGraspBehaviorLinkComponent::FindLinkByAbilityHandle(FGameplayAbilitySpecHandle Handle) const
{
	for (const auto& Pair : ActiveLinks)
	{
		if (Pair.Value.AbilityHandle == Handle)
		{
			return Pair.Key;
		}
	}
	return 0;
}
