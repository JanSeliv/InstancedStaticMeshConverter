// Copyright (c) Yevhenii Selivanov

#include "InstancedStaticMeshActor.h"
//---
#include "Components/InstancedStaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "ProceduralMeshConversion.h"
#include "StaticMeshDescription.h"

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

	const FCachedActorMeshInstances* CachedActorMeshInstance = FindOrCreateInstancedMeshes(ActorBlueprint);
	checkf(CachedActorMeshInstance, TEXT("%s: ERROR: 'CachedActorMeshInstance' is null!"), *FString(__FUNCTION__));

	if (UInstancedStaticMeshComponent* InstancedStaticMeshComponent = CachedActorMeshInstance->InstancedStaticMeshComponent)
	{
		constexpr bool bWorldSpace = true;
		InstancedStaticMeshComponent->AddInstance(Transform, bWorldSpace);
	}
}

void AInstancedStaticMeshActor::ResetAllInstances()
{
	// for (const FCachedActorMeshInstances& CachedActorMeshInstance : CachedBlueprintMeshes)
	// {
	// 	for (const FCachedInstancedStaticMeshData& InstancedStaticMeshData : CachedActorMeshInstance.InstancedStaticMeshDataArray)
	// 	{
	// 		if (InstancedStaticMeshData.InstancedStaticMeshComponent)
	// 		{
	// 			InstancedStaticMeshData.InstancedStaticMeshComponent->ClearInstances();
	// 		}
	// 	}
	// }
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
FCachedActorMeshInstances* AInstancedStaticMeshActor::FindOrCreateInstancedMeshes(TSubclassOf<AActor> ActorBlueprint)
{
	FCachedActorMeshInstances* CachedActorMeshInstance = FindCachedActorMeshInstances(ActorBlueprint);
	if (CachedActorMeshInstance)
	{
		// Already cached
		return CachedActorMeshInstance;
	}

	// Obtain meshes from given actor class if it's not cached yet

	FCachedActorMeshInstances& NewActorMeshInstance = CachedBlueprintMeshes.Emplace_GetRef();
	NewActorMeshInstance.ActorBlueprint = ActorBlueprint;

	// We can't obtain components from a blueprint, so we need to spawn an actor from it and cache its components
	AActor* BlueprintActor = GetWorld()->SpawnActor<AActor>(ActorBlueprint);
	checkf(ActorBlueprint, TEXT("%s: ERROR: 'ActorBlueprint' is null!"), *FString(__FUNCTION__));

	TArray<UStaticMeshComponent*> StaticMeshComponents;
	GetVisibleStaticMeshComponents(/*out*/StaticMeshComponents, BlueprintActor);
	SetAllMaterialsInstancedInComponents(StaticMeshComponents);

	NewActorMeshInstance.MergedStaticMesh = MergeStaticMeshes(StaticMeshComponents, this);
	checkf(NewActorMeshInstance.MergedStaticMesh, TEXT("%s: ERROR: 'MergedStaticMesh' is null!"), *FString(__FUNCTION__));

	UInstancedStaticMeshComponent* InstancedStaticMeshComponent = NewObject<UInstancedStaticMeshComponent>(this);
	InstancedStaticMeshComponent->RegisterComponent();
	InstancedStaticMeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	InstancedStaticMeshComponent->SetStaticMesh(NewActorMeshInstance.MergedStaticMesh);
	NewActorMeshInstance.InstancedStaticMeshComponent = InstancedStaticMeshComponent;

	// All components are cached, so we can destroy the actor
	BlueprintActor->Destroy();

	return &NewActorMeshInstance;
}

// Returns all visible static mesh components of the given actor
void AInstancedStaticMeshActor::GetVisibleStaticMeshComponents(TArray<UStaticMeshComponent*>& StaticMeshComponents, const AActor* InActor)
{
	if (!ensureMsgf(InActor, TEXT("%s: 'InActor' is not valid"), *FString(__FUNCTION__)))
	{
		return;
	}

	InActor->GetComponents<UStaticMeshComponent>(StaticMeshComponents);
	for (int32 Index = StaticMeshComponents.Num() - 1; Index >= 0; --Index)
	{
		const UStaticMeshComponent* StaticMeshComponentIt = StaticMeshComponents.IsValidIndex(Index) ? StaticMeshComponents[Index] : nullptr;
		const UStaticMesh* StaticMesh = StaticMeshComponentIt ? StaticMeshComponentIt->GetStaticMesh() : nullptr;
		if (!StaticMesh
			|| !StaticMeshComponentIt->IsVisible()
			|| StaticMeshComponentIt->bHiddenInGame)
		{
			StaticMeshComponents.RemoveAtSwap(Index);
		}
	}
}

// Toggles 'Used With Instanced Static Meshes' flag for all given static mesh components
void AInstancedStaticMeshActor::SetAllMaterialsInstancedInComponents(const TArray<UStaticMeshComponent*>& StaticMeshComponents)
{
	for (const UStaticMeshComponent* StaticMeshComponentIt : StaticMeshComponents)
	{
		if (!StaticMeshComponentIt)
		{
			continue;
		}

		for (int32 MaterialIndex = 0; MaterialIndex < StaticMeshComponentIt->GetNumMaterials(); ++MaterialIndex)
		{
			UMaterial* MaterialIt = Cast<UMaterial>(StaticMeshComponentIt->GetMaterial(MaterialIndex));
			if (!MaterialIt)
			{
				continue;
			}

			MaterialIt->bUsedWithInstancedStaticMeshes = true;
		}
	}
}

// Creates a new Procedural Mesh Component and attach it to the Actor
UStaticMesh* AInstancedStaticMeshActor::MergeStaticMeshes(const TArray<UStaticMeshComponent*>& StaticMeshComponents, AActor* Outer)
{
	if (!ensureMsgf(Outer, TEXT("%s: 'Outer' is not valid"), *FString(__FUNCTION__)))
	{
		return nullptr;
	}

	UProceduralMeshComponent& ProceduralMeshComponent = MergeStaticMeshesIntoProceduralComponent(StaticMeshComponents, *Outer);
	UStaticMesh* MergedMesh = ConvertProceduralMeshToStaticMesh(ProceduralMeshComponent, *Outer);
	ProceduralMeshComponent.DestroyComponent();

	return MergedMesh;
}

UProceduralMeshComponent& AInstancedStaticMeshActor::MergeStaticMeshesIntoProceduralComponent(const TArray<UStaticMeshComponent*>& StaticMeshComponents, AActor& Outer)
{
	UProceduralMeshComponent* ProceduralMeshComponent = NewObject<UProceduralMeshComponent>(&Outer);
	ProceduralMeshComponent->RegisterComponent();
	ProceduralMeshComponent->AttachToComponent(Outer.GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);

	int32 MeshSectionIndex = 0;

	for (UStaticMeshComponent* MeshComponent : StaticMeshComponents)
	{
		if (!MeshComponent)
		{
			continue;
		}

		const FStaticMeshLODResources& LOD = MeshComponent->GetStaticMesh()->GetLODForExport(0);

		int32 NumMaterials = MeshComponent->GetNumMaterials();

		for (int32 MaterialIndex = 0; MaterialIndex < NumMaterials; ++MaterialIndex)
		{
			TArray<FVector> Vertices;
			TArray<int32> Indices;
			TArray<FVector> Normals;
			TArray<FVector2D> UVs;
			TArray<FProcMeshTangent> Tangents;
			TArray<FColor> VertexColors;

			for (int32 i = 0; i < LOD.GetNumVertices(); i++)
			{
				const uint32 VertexIndex = i;

				FVector Vertex = FVector::ZeroVector;
				if (VertexIndex < LOD.VertexBuffers.PositionVertexBuffer.GetNumVertices())
				{
					const FVector3f Vertex3f = LOD.VertexBuffers.PositionVertexBuffer.VertexPosition(VertexIndex);
					Vertex = MeshComponent->GetComponentTransform().TransformPosition(FVector(Vertex3f.X, Vertex3f.Y, Vertex3f.Z));
				}

				FVector Normal = FVector::UpVector;
				if (VertexIndex < LOD.VertexBuffers.StaticMeshVertexBuffer.GetNumVertices())
				{
					const FVector4f Normal4f = LOD.VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(VertexIndex);
					Normal = MeshComponent->GetComponentTransform().TransformVectorNoScale(FVector(Normal4f.X, Normal4f.Y, Normal4f.Z));
				}

				FVector Tangent = FVector::RightVector;
				if (VertexIndex < LOD.VertexBuffers.StaticMeshVertexBuffer.GetNumVertices())
				{
					const FVector4f Tangent4f = LOD.VertexBuffers.StaticMeshVertexBuffer.VertexTangentX(VertexIndex);
					Tangent = MeshComponent->GetComponentTransform().TransformVectorNoScale(FVector(Tangent4f.X, Tangent4f.Y, Tangent4f.Z));
				}

				FVector2D UV = FVector2D::ZeroVector;
				if (VertexIndex < LOD.VertexBuffers.StaticMeshVertexBuffer.GetNumVertices())
				{
					const FVector2f UV2f = LOD.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(VertexIndex, 0);
					UV = FVector2D(UV2f.X, UV2f.Y);
				}

				FColor VertexColor = FColor::White;
				if (VertexIndex < LOD.VertexBuffers.ColorVertexBuffer.GetNumVertices())
				{
					VertexColor = LOD.VertexBuffers.ColorVertexBuffer.VertexColor(VertexIndex);
				}

				Vertices.Emplace(Vertex);
				Normals.Emplace(Normal);
				UVs.Emplace(FVector2D(UV.X, UV.Y));
				Tangents.Emplace(FProcMeshTangent(Tangent, false));
				VertexColors.Emplace(VertexColor);
			}

			for (int32 i = 0; i < LOD.IndexBuffer.GetNumIndices(); i += 3)
			{
				int32 Index0 = LOD.IndexBuffer.GetIndex(i);
				int32 Index1 = LOD.IndexBuffer.GetIndex(i + 1);
				int32 Index2 = LOD.IndexBuffer.GetIndex(i + 2);

				int32 TriangleMaterialIndex = LOD.Sections[0].MaterialIndex;

				if (TriangleMaterialIndex == MaterialIndex)
				{
					Indices.Emplace(Index0);
					Indices.Emplace(Index1);
					Indices.Emplace(Index2);
				}
			}

			if (Indices.Num() > 0)
			{
				ProceduralMeshComponent->CreateMeshSection(MeshSectionIndex, Vertices, Indices, Normals, UVs, VertexColors, Tangents, false);
				ProceduralMeshComponent->SetMaterial(MeshSectionIndex, MeshComponent->GetMaterial(MaterialIndex));
				MeshSectionIndex++;
			}
		}
	}

	return *ProceduralMeshComponent;
}

UStaticMesh* AInstancedStaticMeshActor::ConvertProceduralMeshToStaticMesh(UProceduralMeshComponent& ProcMeshComp, AActor& Outer)
{
	TArray<UMaterialInterface*> MeshMaterials;
	const int32 NumSections = ProcMeshComp.GetNumSections();
	for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
	{
		MeshMaterials.Emplace(ProcMeshComp.GetMaterial(SectionIdx));
	}

	UStaticMesh* StaticMesh = NewObject<UStaticMesh>(&Outer, NAME_None, RF_Transient);
	StaticMesh->bAllowCPUAccess = true;
	StaticMesh->NeverStream = true;
	StaticMesh->InitResources();
	StaticMesh->SetLightingGuid();

	// Copy materials to new mesh
	for (UMaterialInterface* Material : MeshMaterials)
	{
		StaticMesh->GetStaticMaterials().Emplace(FStaticMaterial(Material));
	}

	const FMeshDescription PMC_Description = BuildMeshDescription(&ProcMeshComp);
	UStaticMeshDescription* SM_Description = StaticMesh->CreateStaticMeshDescription();
	SM_Description->SetMeshDescription(PMC_Description);
	StaticMesh->BuildFromStaticMeshDescriptions({SM_Description}, false);

#if WITH_EDITOR
	StaticMesh->PostEditChange();
#endif

	StaticMesh->MarkPackageDirty();

	return StaticMesh;
}
