// Copyright (c) Jared Taylor

#include "GraspSmartObjectSlotComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GraspSmartObjectSlotComponent)

UPrimitiveComponent* UGraspSmartObjectSlotComponent::GetGraspableForSlot(int32 SlotIndex) const
{
	for (const FGraspSmartObjectSlotMapping& Mapping : SlotMappings)
	{
		if (Mapping.SmartObjectSlotIndex == SlotIndex)
		{
			return Cast<UPrimitiveComponent>(Mapping.GraspableComponentRef.GetComponent(GetOwner()));
		}
	}
	return nullptr;
}

int32 UGraspSmartObjectSlotComponent::GetSlotForGraspable(const UPrimitiveComponent* GraspableComponent) const
{
	for (const FGraspSmartObjectSlotMapping& Mapping : SlotMappings)
	{
		if (Mapping.GraspableComponentRef.GetComponent(GetOwner()) == GraspableComponent)
		{
			return Mapping.SmartObjectSlotIndex;
		}
	}
	return INDEX_NONE;
}
