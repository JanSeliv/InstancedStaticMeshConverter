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

	FCachedActorMeshInstances* CachedActorMeshInstance = FindOrCreateInstancedMeshes(ActorBlueprint);
	checkf(CachedActorMeshInstance, TEXT("%s: ERROR: 'CachedActorMeshInstance' is null!"), *FString(__FUNCTION__));

	for (const FCachedInstancedStaticMeshData& It : CachedActorMeshInstance->InstancedStaticMeshDataArray)
	{
		if (UInstancedStaticMeshComponent* InstancedComponent = It.InstancedStaticMeshComponent)
		{
			constexpr bool bWorldSpace = true;
			const FTransform CombinedTransform = It.RelativeTransform * Transform;
			InstancedComponent->AddInstance(CombinedTransform, bWorldSpace);
		}
	}
}

void AInstancedStaticMeshActor::ResetAllInstances()
{
	for (const FCachedActorMeshInstances& CachedActorMeshInstance : CachedBlueprintMeshes)
	{
		for (const FCachedInstancedStaticMeshData& InstancedStaticMeshData : CachedActorMeshInstance.InstancedStaticMeshDataArray)
		{
			if (InstancedStaticMeshData.InstancedStaticMeshComponent)
			{
				InstancedStaticMeshData.InstancedStaticMeshComponent->ClearInstances();
			}
		}
	}
}

void AInstancedStaticMeshActor::Destroyed()
{
	if (IsValid(this))
	{
		ResetAllInstances();
		CachedBlueprintMeshes.Empty();
	}

	Super::Destroyed();
}

// Tries to find cached data about given actor class if was spawned before
FCachedActorMeshInstances* AInstancedStaticMeshActor::FindCachedActorMeshInstances(TSubclassOf<AActor> ActorBlueprint)
{
	for (FCachedActorMeshInstances& CachedActorMeshInstance : CachedBlueprintMeshes)
	{
		if (CachedActorMeshInstance.ActorBlueprint == ActorBlueprint)
		{
			return &CachedActorMeshInstance;
		}
	}

	return nullptr;
}

// If not cached yet, tries to obtain the static meshes by spawning actor
FCachedActorMeshInstances* AInstancedStaticMeshActor::FindOrCreateInstancedMeshes(TSubclassOf<AActor> ActorClass)
{
	FCachedActorMeshInstances* CachedActorMeshInstance = FindCachedActorMeshInstances(ActorClass);
	if (CachedActorMeshInstance)
	{
		// Already cached
		return CachedActorMeshInstance;
	}

	// Obtain meshes from given actor class if it's not cached yet

	FCachedActorMeshInstances& NewActorMeshInstance = CachedBlueprintMeshes.Emplace_GetRef();
	NewActorMeshInstance.ActorBlueprint = ActorClass;

	// We can't obtain components from a blueprint, so we need to spawn an actor from it and cache its components
	AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(ActorClass);
	checkf(ActorClass, TEXT("%s: ERROR: 'ActorClass' is null!"), *FString(__FUNCTION__));

	TArray<UStaticMeshComponent*> StaticMeshComponents;
	SpawnedActor->GetComponents<UStaticMeshComponent>(StaticMeshComponents);

	for (const UStaticMeshComponent* StaticMeshComponent : StaticMeshComponents)
	{
		UStaticMesh* StaticMesh = StaticMeshComponent ? StaticMeshComponent->GetStaticMesh() : nullptr;
		if (!StaticMesh
			|| !StaticMeshComponent->IsVisible()
			|| StaticMeshComponent->bHiddenInGame)
		{
			// Static mesh is not chosen or hidden
			continue;
		}

		UInstancedStaticMeshComponent* InstancedStaticMeshComponent = NewObject<UInstancedStaticMeshComponent>(this);
		InstancedStaticMeshComponent->RegisterComponent();
		InstancedStaticMeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		InstancedStaticMeshComponent->SetStaticMesh(StaticMesh);

		// Prepare materials: copy from static mesh component to instanced static mesh component
		const int32 NumMaterials = StaticMeshComponent->GetNumMaterials();
		for (int32 MaterialIndex = 0; MaterialIndex < NumMaterials; ++MaterialIndex)
		{
			UMaterialInterface* MaterialIt = StaticMeshComponent->GetMaterial(MaterialIndex);
			if (!MaterialIt)
			{
				continue;
			}

			InstancedStaticMeshComponent->SetMaterial(MaterialIndex, MaterialIt);

			// Mark material as used with instanced static meshes
			if (const UMaterialInstanceDynamic* MaterialInstance = Cast<UMaterialInstanceDynamic>(MaterialIt))
			{
				MaterialIt = MaterialInstance->Parent;
			}
			if (UMaterial* Material = Cast<UMaterial>(MaterialIt))
			{
				Material->bUsedWithInstancedStaticMeshes = true;
			}
		}

		FCachedInstancedStaticMeshData CachedStaticMeshData;
		CachedStaticMeshData.StaticMesh = StaticMesh;
		CachedStaticMeshData.RelativeTransform = StaticMeshComponent->GetComponentTransform().GetRelativeTransform(SpawnedActor->GetActorTransform());
		CachedStaticMeshData.InstancedStaticMeshComponent = InstancedStaticMeshComponent;

		NewActorMeshInstance.InstancedStaticMeshDataArray.Emplace(CachedStaticMeshData);
	}

	// All components are cached, so we can destroy the actor
	SpawnedActor->Destroy();

	return &NewActorMeshInstance;
}
