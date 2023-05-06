// Copyright (c) Yevhenii Selivanov

#pragma once

#include "GameFramework/Actor.h"
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

	UFUNCTION(BlueprintCallable, Category = "InstancedStaticMesh")
	void SpawnInstance(const FTransform& Transform, TSubclassOf<AActor> ActorBlueprint);

	UFUNCTION(BlueprintCallable, Category = "InstancedStaticMesh", meta = (Keywords = "Clear"))
	void ResetAllInstances();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Transient, Category = "InstancedStaticMesh", meta = (BlueprintProtected))
	TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>> InstancedStaticMeshComponents;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Completely destroys all instances of all instanced static mesh components. */
	UFUNCTION(BlueprintCallable, Category = "C++", meta = (BlueprintProtected))
	void DestroyAllInstances();
};
