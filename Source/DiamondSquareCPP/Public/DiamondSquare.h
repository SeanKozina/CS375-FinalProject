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


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Map Parameters")
        float AltitudeScale = 0.03f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Map Parameters")
        float TemperatureScale = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Map Parameters")
        float HumidityScale = 0.1f;



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

    FLinearColor GetColorBasedOnBiomeAndHeight(float Z, TCHAR biomeType);

    void CreateVertices(const TArray<TArray<float>>& NoiseMap);
    void CreateTriangles();
    TArray<TArray<float>> GeneratePerlinNoiseMap();

    TArray<FString> CreateBiomeMap();

    void PostProcessBiomeMap();
    FString DetermineBiome(float altitude, float temperature, float humidity);

    float GetInterpolatedHeight(float heightValue, TCHAR biomeType);

    TArray<FString> BiomeMap;

    // Height adjustment functions for biomes
    float AdjustForOcean(float& heightValue);
    float AdjustForMountain(float& heightValue);
    float AdjustForPlains(float& heightValue);
    float AdjustForSnow(float& heightValue);
    float AdjustForForest(float& heightValue);
    float AdjustForTaiga(float& heightValue);
    float AdjustForWoodlands(float& heightValue);
    float AdjustForSnowyMountain(float& heightValue);
    float AdjustForHighlands(float& heightValue);
    float AdjustForRiver(float& heightValue);  // If needed
    float AdjustForSnowyForest(float& heightValue); // If needed
    float AdjustForOldForest(float& heightValue); // If needed
    float AdjustForBeach(float& heightValue);
    float AdjustForJungle(float& heightValue);
    float AdjustForConiferousForest(float& heightValue);
    float AdjustForSwamp(float& heightValue);
    float AdjustForDesert(float& heightValue);
    float AdjustForSavanna(float& heightValue);
    float AdjustForRainforest(float& heightValue);
    float AdjustForSteppe(float& heightValue);
    float AdjustForTundra(float& heightValue);




};