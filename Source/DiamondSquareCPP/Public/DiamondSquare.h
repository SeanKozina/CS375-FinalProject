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
    enum class ECell
    {
        Land,
        Ocean,
        Warm,
        Cold,
        Freezing,
        Temperate,
        // Biome types
        DeepOcean,
        Desert,
        SandDunes,
        Plains,
        Grassland,
        Rainforest,
        Savannah,
        Swamp,
        Marsh,
        Woodland,
        Forest,
        Highland,
        Taiga,
        SnowyForest,
        Tundra,
        IcePlains,
        Mountain,
        Volcanic,
        Beach,
        River,
        SwampShore,
        Ice,
        ColdBeach,
        Oasis,
        Steppe,
        Mesa
    };

    ADiamondSquare();

    UPROPERTY(EditAnywhere)
    bool recreateMesh = false;

    UPROPERTY(EditAnywhere, Meta = (ClampMin = 0), Meta = (ClampMax = 2048))
    int XSize = 200;

    UPROPERTY(EditAnywhere, Meta = (ClampMin = 0), Meta = (ClampMax = 2048))
    int YSize = 200;

    UPROPERTY(EditAnywhere, Meta = (ClampMin = 0))
    float ZMultiplier = 8.0f;

    UPROPERTY(EditAnywhere, Meta = (ClampMin = 0))
    float ZExpo = 2.1;

    UPROPERTY(EditAnywhere, Meta = (ClampMin = 0))
    float Scale = 500.0f;

    UPROPERTY(EditAnywhere, Meta = (ClampMin = 0))
    float UVScale = 0.0f;

    UPROPERTY(EditAnywhere, Meta = (ClampMin = 0))
    int Octaves = 12;

    UPROPERTY(EditAnywhere, Meta = (ClampMin = 0))
    float Lacunarity = 7.0f;

    UPROPERTY(EditAnywhere, Meta = (ClampMin = 0))
    float Persistence = 0.7f;

    UPROPERTY(EditAnywhere)
    bool SurroundMapWithOcean = false;

    UPROPERTY(EditAnywhere)
    bool CalculateTangents = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Map Parameters")
    int32 Seed = 0;

    UPROPERTY(EditAnywhere, Meta = (ClampMin = 0.0f, ClampMax = 1.0f), Category = "Biome Map Parameters")
    float ProbabilityOfLand = 0.5f;
   
    //UPROPERTY(EditAnywhere, Category = "Procedural Generation")
    bool addProceduralObjects = false;

    // Make the mesh component editable in the Unreal Editor
    //UPROPERTY(EditAnywhere, Category = "Procedural Generation")
    UInstancedStaticMeshComponent* TreeMeshComponent;
   

    void PlaceEnvironmentObjects(const TArray<TArray<float>>& NoiseMap);

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

    TArray<FColor> Colors;

    void CreateVertices(const TArray<TArray<float>>& NoiseMap);
    void CreateTriangles();

    TArray<TArray<float>> GeneratePerlinNoiseMap();

    TArray<TArray<ADiamondSquare::ECell>> BiomeMap;

    FLinearColor GetColorBasedOnBiomeAndHeight(float Z, ECell BiomeType);
    float GetInterpolatedHeight(float heightValue, ECell BiomeType);

    FRandomStream Rng;

    //Schostaic Automata Stack to Create Biome Map
    TArray<TArray<ECell>> Island(TArray<TArray<ECell>>& Board);
    TArray<TArray<ECell>> FuzzyZoom(const TArray<TArray<ECell>>& Board);
    TArray<TArray<ECell>> AddIsland(const TArray<TArray<ECell>>& Board);
    TArray<TArray<ECell>> AddIsland2(const TArray<TArray<ECell>>& Board);
    TArray<TArray<ECell>> Zoom(const TArray<TArray<ECell>>& Board);
    TArray<TArray<ECell>> RemoveTooMuchOcean(const TArray<TArray<ECell>>& Board);
    TArray<TArray<ECell>> AddTemps(const TArray<TArray<ECell>>& Board);
    TArray<TArray<ECell>> WarmToTemperate(const TArray<TArray<ECell>>& Board);
    TArray<TArray<ECell>> FreezingToCold(const TArray<TArray<ECell>>& Board);
    TArray<TArray<ECell>> TemperatureToBiome(const TArray<TArray<ECell>>& Board);
    TArray<TArray<ECell>> DeepOcean(const TArray<TArray<ECell>>& Board);
    TArray<TArray<ECell>> Shore(const TArray<TArray<ECell>>& Board);
    TArray<TArray<ECell>> SurroundWithOcean(TArray<TArray<ECell>>& Board);



    //helper functions
    ECell SelectBiome(const TArray<ECell>& Biomes, const TArray<float>& Odds);
    void SetBoardRegion(TArray<TArray<ECell>>& Board, int32 CenterX, int32 CenterY, int32 Radius, ECell NewState);
    bool IsAdjacentToGroup(const TArray<TArray<ECell>>& Board, int32 X, int32 Y, const TSet<ECell>& GroupA, const TSet<ECell>& GroupB);
    bool IsSurroundedByOcean(const TArray<TArray<ECell>>& Board, int32 i, int32 j);
    bool IsEdgeCell(const TArray<TArray<ECell>>& Board, int32 i, int32 j);
    void PrintBoard(const TArray<TArray<ECell>>& Board);
    TArray<TArray<ECell>> TestIsland();
    bool CanTransform(ECell CellType) const; 
    void InitializeSeed();
};