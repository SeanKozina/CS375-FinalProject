#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "DiamondSquare.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;

UCLASS()
class DIAMONDSQUARECPP_API ADiamondSquare : public AActor
{
    GENERATED_BODY()

public:
    ADiamondSquare();

    UPROPERTY(EditAnywhere)
        bool recreateMesh = true;

    UPROPERTY(EditAnywhere, Meta = (ClampMin = 0))
        int XSize = 200;

    UPROPERTY(EditAnywhere, Meta = (ClampMin = 0))
        int YSize = 200;

    UPROPERTY(EditAnywhere, Meta = (ClampMin = 0))
        float ZMultiplier = 1000.0f;

    UPROPERTY(EditAnywhere, Meta = (ClampMin = 0))
        float Scale = 10.0f;

    UPROPERTY(EditAnywhere, Meta = (ClampMin = 0))
        float UVScale = 0.0f;

    UPROPERTY(EditAnywhere, Meta = (ClampMin = 0))
        int Octaves = 1;

    UPROPERTY(EditAnywhere, Meta = (ClampMin = 0))
        float Lacunarity = 2.0f;

    UPROPERTY(EditAnywhere, Meta = (ClampMin = 0))
        float Persistence = 0.5f;

protected:
    virtual void BeginPlay() override;
    virtual void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(EditAnywhere)
        UMaterialInterface* Material;

public:
    virtual void Tick(float DeltaTime) override;

private:
    UProceduralMeshComponent* ProceduralMesh;
    TArray<FVector> Vertices;
    TArray<int> Triangles;
    TArray<FVector2D> UV0;
    TArray<FVector> Normals;
    TArray<struct FProcMeshTangent> Tangents;

    FLinearColor Color;
    TArray<FColor> Colors;
        
    FLinearColor GetColorForBiome(FLinearColor BaseColor, TCHAR BiomeChar, float Z);
    FLinearColor GetColorBasedOnHeight(float Z);


    void CreateVertices(const TArray<TArray<float>>& NoiseMap);
    void CreateTriangles();
    TArray<TArray<float>> GeneratePerlinNoiseMap();
    TArray<FString> CreateBiomeMap();
    float GetInterpolatedHeight(float heightValue, char biomeType);


    TArray<FString> BiomeMap;
};