// Copyright (c) Yevhenii Selivanov

#include "InstancedStaticMeshActor.h"
//---
#include "Components/InstancedStaticMeshComponent.h"

// Sets default values
AInstancedStaticMeshActor::AInstancedStaticMeshActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

void AInstancedStaticMeshActor::SpawnInstance(const FTransform& Transform, TSubclassOf<AActor> ActorBlueprint)
{
	if (!ensureMsgf(ActorBlueprint, TEXT("%s: 'ActorBlueprint' is not valid"), *FString(__FUNCTION__)))
	{
		return;
	}

	const AActor* BlueprintCDO = ActorBlueprint->GetDefaultObject<AActor>();
	checkf(BlueprintCDO, TEXT("%s: ERROR: 'BlueprintCDO' is null!"), *FString(__FUNCTION__));

	TArray<UStaticMeshComponent*> StaticMeshComponents;
	BlueprintCDO->GetComponents<UStaticMeshComponent>(StaticMeshComponents);

	for (const UStaticMeshComponent* StaticMeshComponent : StaticMeshComponents)
	{
		UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
		UInstancedStaticMeshComponent* InstancedStaticMeshComponent = nullptr;

		if (InstancedStaticMeshComponents.Contains(StaticMesh))
		{
			InstancedStaticMeshComponent = InstancedStaticMeshComponents[StaticMesh];
		}
		else
		{
			InstancedStaticMeshComponent = NewObject<UInstancedStaticMeshComponent>(this);
			InstancedStaticMeshComponent->RegisterComponent();
			InstancedStaticMeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
			InstancedStaticMeshComponent->SetStaticMesh(StaticMesh);
			InstancedStaticMeshComponent->SetMaterial(0, StaticMeshComponent->GetMaterial(0));
			InstancedStaticMeshComponents.Emplace(StaticMesh, InstancedStaticMeshComponent);
		}

		InstancedStaticMeshComponent->AddInstance(Transform);
	}
}

void AInstancedStaticMeshActor::ResetAllInstances()
{
	for (const TTuple<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>>& Entry : InstancedStaticMeshComponents)
	{
		UInstancedStaticMeshComponent* InstancedStaticMeshComponent = Entry.Value;
		InstancedStaticMeshComponent->ClearInstances();
	}
}

void AInstancedStaticMeshActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyAllInstances();

	Super::EndPlay(EndPlayReason);
}

// Completely destroys all instances of all instanced static mesh components
void AInstancedStaticMeshActor::DestroyAllInstances()
{
	TArray<UInstancedStaticMeshComponent*> MapValues;
	for (int32 Index = MapValues.Num() - 1; Index >= 0; --Index)
	{
		UInstancedStaticMeshComponent* ComponentIt = MapValues.IsValidIndex(Index) ? MapValues[Index] : nullptr;
		if (ComponentIt)
		{
			ComponentIt->ClearInstances();
			ComponentIt->DestroyComponent();
		}
	}

	MapValues.Empty();
}
