// Copyright (c) Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"
//---
#include "InstancedDataTypes.generated.h"

/**
 *Data obtained from specific static mesh component of actor class
 */
USTRUCT(BlueprintType)
struct FCachedInstancedStaticMeshData
{
	GENERATED_BODY()

	/** Obtained static mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++")
	TObjectPtr<class UStaticMesh> StaticMesh = nullptr;

	/** Original relative transform of the component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++")
	FTransform RelativeTransform = FTransform::Identity;

	/** Created instanced static mesh component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++")
	TObjectPtr<class UInstancedStaticMeshComponent> InstancedStaticMeshComponent = nullptr;
};

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

	/** All data about obtained static meshes from given actor class . */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++")
	TArray<FCachedInstancedStaticMeshData> InstancedStaticMeshDataArray;
};
