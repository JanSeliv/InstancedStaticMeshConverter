// Copyright (c) Yevhenii Selivanov

#pragma once

#include "GameFramework/Actor.h"
//---
#include "InstancedDataTypes.h"
//---
#include "InstancedStaticMeshActor.generated.h"

class UInstancedStaticMeshComponent;

/**
 * Converts actors with static meshes to instanced static meshes.
 */
UCLASS(BlueprintType, Blueprintable)
class INSTANCEDSTATICMESHCONVERTER_API AInstancedStaticMeshActor : public AActor
{
	GENERATED_BODY()

public:
	AInstancedStaticMeshActor();

	/** Creates all instanced static meshes for each static mesh component contained in given actor class. */
	UFUNCTION(BlueprintCallable, Category = "InstancedStaticMesh")
	void SpawnInstanceByActor(const FTransform& Transform, TSubclassOf<AActor> ActorBlueprint);

	/** Spawns an instance of the specified static mesh with the given transform. */
	UFUNCTION(BlueprintCallable, Category = "InstancedStaticMesh")
	void SpawnInstanceByMesh(const FTransform& Transform, const UStaticMesh* Mesh);

	/** Removes all mesh instances created by this actor. */
	UFUNCTION(BlueprintCallable, Category = "InstancedStaticMesh", meta = (Keywords = "Clear,Empty,Remove"))
	void ResetAllInstances();

protected:
	/** Cached data about all created instanced static meshes of specific actor class. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Transient, Category = "InstancedStaticMesh", meta = (BlueprintProtected))
	TArray<FCachedActorMeshInstances> CachedBlueprintMeshes;

	/** Called when this actor is explicitly being destroyed during gameplay or in the editor, not called during level streaming or gameplay ending */
	virtual void Destroyed() override;

	/** Tries to find cached data about given actor class if was spawned before. */
	FCachedActorMeshInstances* FindCachedActorMeshInstances(TSubclassOf<AActor> ActorBlueprint);

	/** If not cached yet, tries to obtain the static meshes by spawning actor. */
	FCachedActorMeshInstances* FindOrCreateInstancedMeshes(TSubclassOf<AActor> ActorClass);
};
