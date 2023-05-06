// Copyright (c) Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
//---
#include "InstancedDataTypes.generated.h"

/**
 * Contains all the static meshes of specific actor class.
 * It will be reused for all actors of the same class.
 */
USTRUCT(BlueprintType)
struct FCachedActorMeshInstances
{
	GENERATED_BODY()

	/** The actor class that contains static meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++")
	TSubclassOf<class AActor> ActorBlueprint = nullptr;

	/** Created and merged mesh obtained from multiple static mesh components. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++")
	TObjectPtr<class UStaticMesh> MergedStaticMesh = nullptr;

	/** Created instanced static mesh component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++")
	TObjectPtr<class UInstancedStaticMeshComponent> InstancedStaticMeshComponent = nullptr;
};
