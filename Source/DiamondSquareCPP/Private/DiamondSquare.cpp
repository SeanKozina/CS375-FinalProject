#include "DiamondSquare.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "Math/Color.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "Components/InstancedStaticMeshComponent.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDiamondSquare, Log, All);
DEFINE_LOG_CATEGORY(LogDiamondSquare);


ADiamondSquare::ADiamondSquare()
{
    // Always set a RootComponent first
    ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
    RootComponent = ProceduralMesh; // Set ProceduralMesh as the RootComponent

    // Initialize and attach the tree mesh component to the ProceduralMesh
    TreeMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TreeMeshComponent"));
    TreeMeshComponent->SetupAttachment(ProceduralMesh); // Attach to the root component

    // Assign the static mesh asset to the TreeMeshComponent
    /*static ConstructorHelpers::FObjectFinder<UStaticMesh> TreeMeshAsset(TEXT("/Game/Fantastic_Village_Pack/meshes/environment/SM_ENV_TREE_village_LOD0"));
    if (TreeMeshAsset.Succeeded())
    {
        TreeMeshComponent->SetStaticMesh(TreeMeshAsset.Object);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load tree mesh."));
    }*/

    // PrimaryActorTick.bCanEverTick is set to false by default, but you can enable it if needed.
    PrimaryActorTick.bCanEverTick = false;
}


void ADiamondSquare::OnConstruction(const FTransform& Transform)
{
    // Call the superclass's OnConstruction to handle basic setup
    Super::OnConstruction(Transform);

    // Check if the mesh needs to be recreated
    if (recreateMesh) {
        if (!ProceduralMesh || !TreeMeshComponent)
        {
            UE_LOG(LogTemp, Error, TEXT("Mesh components are not initialized properly."));
            return;
        }

        // Reset mesh data to prepare for new mesh creation
        Normals.Reset();
        Tangents.Reset();
        UV0.Reset();
        Colors.Reset();
        Vertices.Reset();
        Triangles.Reset();
        UV0.Reset();
        BiomeMap.Reset();

        if (TreeMeshComponent)
        {
            TreeMeshComponent->ClearInstances();
        }
        double StartTimeOC = FPlatformTime::Seconds();
        auto NoiseMap = GeneratePerlinNoiseMap();

        


        // Create vertices and triangles for the mesh
        CreateVertices(NoiseMap);
        CreateTriangles();


        // Calculate normals and tangents for the mesh
        if (CalculateTangents) {
            UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UV0, Normals, Tangents);
        }

        // Create the mesh section with the specified data and apply the material
        ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UV0, Colors, Tangents, true);
        ProceduralMesh->SetMaterial(0, Material);

        if (addProceduralObjects) {
            PlaceEnvironmentObjects(NoiseMap);
        }

        double EndTimeOC = FPlatformTime::Seconds();
        double ElapsedTimeOC = EndTimeOC - StartTimeOC;
        UE_LOG(LogTemp, Warning, TEXT("Construction took %f seconds"), ElapsedTimeOC);

        // Reset mesh data to prepare for new mesh creation
        Normals.Reset();
        Tangents.Reset();
        UV0.Reset();
        Colors.Reset();
        Vertices.Reset();
        Triangles.Reset();
        UV0.Reset();
        BiomeMap.Reset();

        // Reset the flag to avoid unnecessary mesh recreation
        CalculateTangents = false;
        addProceduralObjects = false;
        recreateMesh = false;
    }
}


void ADiamondSquare::BeginPlay()
{
    Super::BeginPlay();

}


void ADiamondSquare::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

}

void ADiamondSquare::PlaceEnvironmentObjects(const TArray<TArray<float>>& NoiseMap)
{
    for (int X = 0; X < XSize; ++X)
    {
        for (int Y = 0; Y < YSize; ++Y)
        {
            float Z = NoiseMap[X][Y] * ZMultiplier;
            FVector Location(X * Scale, Y * Scale, Z);
            FRotator Rotation = FRotator(0, FMath::RandRange(0, 360), 0); // Random rotation for variation
            FVector VectorScale(5.0f, 5.0f, 5.0f); // Scale can be adjusted based on the object and biome

            ECell Biome = BiomeMap[X][Y];

            // Determine what object to place based on the biome
            switch (Biome)
            {
            case ECell::Forest:
                if (FMath::RandRange(0.0f, 1.0f) < 0.01f) // Low probability for buildings
                {
                    TreeMeshComponent->AddInstance(FTransform(Rotation, Location, VectorScale));
                }
            case ECell::Taiga:
                // Add a tree instance
                
                break;
            case ECell::Mountain:
                if (FMath::RandRange(0.0f, 1.0f) < 0.01f) // Low probability for buildings
                {
                    TreeMeshComponent->AddInstance(FTransform(Rotation, Location, VectorScale));
                }
            case ECell::Highland:
                if (FMath::RandRange(0.0f, 1.0f) < 0.01f) // Low probability for buildings
                {
                    TreeMeshComponent->AddInstance(FTransform(Rotation, Location, VectorScale));
                }
                // Add a rock instance
                //RockMeshComponent->AddInstance(FTransform(Rotation, Location, Scale));
                break;
            case ECell::Plains:
                if (FMath::RandRange(0.0f, 1.0f) < 0.01f) // Low probability for buildings
                {
                    TreeMeshComponent->AddInstance(FTransform(Rotation, Location, VectorScale));
                }
            case ECell::Savannah:
                if (FMath::RandRange(0.0f, 1.0f) < 0.01f) // Low probability for buildings
                {
                    TreeMeshComponent->AddInstance(FTransform(Rotation, Location, VectorScale));
                }
                // Add a building instance with some probability
                if (FMath::RandRange(0.0f, 1.0f) < 0.01f) // Low probability for buildings
                {
                    //BuildingMeshComponent->AddInstance(FTransform(Rotation, Location, Scale));
                }
                break;
            }
        }
    }
}


void ADiamondSquare::CreateTriangles()
{
    double StartTime = FPlatformTime::Seconds();
    for (int X = 0; X < XSize - 1; ++X)
    {
        for (int Y = 0; Y < YSize - 1; ++Y)
        {
            int VertexIndex = X * YSize + Y;

            // First triangle (clockwise winding order)
            Triangles.Add(VertexIndex);                  // Bottom left
            Triangles.Add(VertexIndex + YSize + 1);      // Top right
            Triangles.Add(VertexIndex + YSize);          // Top left

            // Second triangle (clockwise winding order)
            Triangles.Add(VertexIndex);                  // Bottom left
            Triangles.Add(VertexIndex + 1);              // Bottom right
            Triangles.Add(VertexIndex + YSize + 1);      // Top right
        }
    }
    double EndTime = FPlatformTime::Seconds();
    double ElapsedTime = EndTime - StartTime;
    UE_LOG(LogTemp, Warning, TEXT("CreateTriangles took %f seconds"), ElapsedTime);
}



void ADiamondSquare::CreateVertices(const TArray<TArray<float>>& NoiseMap)
{
    double StartTimeCV = FPlatformTime::Seconds();
    // Prepare the Colors array for new data
    Colors.Empty();
    FLinearColor Color;
    // Check if the ProceduralMesh is valid
    if (ProceduralMesh)
    {
        // Iterate over each grid point to create vertices
        for (int X = 0; X < XSize; ++X)
        {
            for (int Y = 0; Y < YSize; ++Y)
            {
                float Z = NoiseMap[X][Y]; // Height value from the noise map
                ECell BiomeChar = BiomeMap[X][Y];
                Color = GetColorBasedOnBiomeAndHeight(Z, BiomeChar);
                Colors.Add(Color.ToFColor(false));
                // Determine the color based on biome and heigh

                // Add vertex with calculated position and UV coordinates
                Z *= ZMultiplier;
                Z = pow(Z, ZExpo);
                Vertices.Add(FVector(X * Scale, Y * Scale, Z * Scale));
                UV0.Add(FVector2D(X * UVScale, Y * UVScale));
            }
        }

        // Initialize normals and tangents for the vertices
        Normals.Init(FVector(0.0f, 0.0f, 1.0f), Vertices.Num());
        Tangents.Init(FProcMeshTangent(1.0f, 0.0f, 0.0f), Vertices.Num());

        // Create the mesh section with the generated data
        ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UV0, Colors, Tangents, true);
    }
    double EndTimeCV = FPlatformTime::Seconds();
    double ElapsedTimeCV = EndTimeCV - StartTimeCV;
    UE_LOG(LogTemp, Warning, TEXT("CreateVertices took %f seconds"), ElapsedTimeCV);
}


TArray<TArray<float>> ADiamondSquare::GeneratePerlinNoiseMap()
{

    // Reset and create the BiomeMap
    BiomeMap.Empty();
    BiomeMap = TestIsland();
    double StartTimeGP = FPlatformTime::Seconds();
    // Initialize the NoiseMap array
    TArray<TArray<float>> NoiseMap;
    NoiseMap.Init(TArray<float>(), XSize);

    // Iterate over the grid to generate noise values
    for (int X = 0; X < XSize; ++X)
    {
        NoiseMap[X].Init(0.0f, YSize);
        for (int Y = 0; Y < YSize; ++Y)
        {
            // Initialize variables for noise calculation
            float Amplitude = 1.0f;
            float Frequency = 1.0f;
            float NoiseHeight = 0.0f;

            // Calculate noise value across multiple octaves
            for (int Octave = 0; Octave < Octaves; ++Octave)
            {
                // Determine sample coordinates based on frequency and scale
                float SampleX = X / Scale * Frequency;
                float SampleY = Y / Scale * Frequency;

                // Generate Perlin noise value
                float PerlinValue = FMath::PerlinNoise2D(FVector2D(SampleX, SampleY));
                NoiseHeight += PerlinValue * Amplitude;

                // Adjust amplitude and frequency for next octave
                Amplitude *= Persistence;
                Frequency *= Lacunarity;
            }

            // Adjust noise height based on biome
            ECell BiomeChar = BiomeMap[X][Y];
            NoiseHeight = GetInterpolatedHeight(NoiseHeight, BiomeChar);

            // Clamp the noise value to ensure it's within the expected range
            NoiseMap[X][Y] = FMath::Clamp(NoiseHeight, 0.0f, 1.0f);
        }
    }
    double EndTimeGP = FPlatformTime::Seconds();
    double ElapsedTimeGP = EndTimeGP - StartTimeGP;
    UE_LOG(LogTemp, Warning, TEXT("GeneratePerlinNoiseMap took %f seconds"), ElapsedTimeGP);

    // Return the generated Perlin noise map
    return NoiseMap;
}


float ADiamondSquare::GetInterpolatedHeight(float HeightValue, ECell BiomeType)
{
    switch (BiomeType)
    {
    case ECell::Ocean: // Ocean
        return FMath::Clamp(HeightValue, 0.0f, 0.0f);
    case ECell::DeepOcean: // DeepOcean
        return FMath::Lerp(0.0f, 0.0f, HeightValue);
    case ECell::SnowyForest: // SnowyForest
        return FMath::Lerp(0.2f, 0.7f, HeightValue);
    case ECell::Mountain: // Mountain
        return FMath::Lerp(0.7f, 1.0f, HeightValue);
    case ECell::Plains: // Plains
        return FMath::Lerp(0.2f, 0.5f, HeightValue);
    case ECell::Beach: // Beach
        return FMath::Lerp(0.03f, 0.3f, HeightValue);
    case ECell::ColdBeach: // Beach
        return FMath::Lerp(0.03f, 0.3f, HeightValue);
    case ECell::Desert: // Desert
        return FMath::Lerp(0.2f, 0.6f, HeightValue);
    case ECell::River: // River
        return FMath::Lerp(0.1f, 0.4f, HeightValue);
    case ECell::Taiga: // Taiga
        return FMath::Lerp(0.25f, 0.65f, HeightValue);
    case ECell::Forest: // Forest
        return FMath::Lerp(0.2f, 0.7f, HeightValue);
    case ECell::Swamp: // Swamp
        return FMath::Lerp(0.05f, 0.2f, HeightValue);
    case ECell::Tundra: // Tundra
        return FMath::Lerp(0.25f, 0.65f, HeightValue);
    case ECell::Rainforest: // Rainforest
        return FMath::Lerp(0.2f, 0.55f, HeightValue);
    case ECell::Woodland: // Woodland
        return FMath::Lerp(0.3f, 0.5f, HeightValue);
    case ECell::Savannah: // Savannah
        return FMath::Lerp(0.2f, 0.5f, HeightValue);
    case ECell::Highland: // Highland
        return FMath::Lerp(0.3f, 0.99f, HeightValue);
    case ECell::IcePlains: // IcePlains
        return FMath::Lerp(0.1f, 0.5f, HeightValue);
    case ECell::Ice: // Ice
        return FMath::Lerp(0.2f, 0.9f, HeightValue);
    case ECell::SwampShore: // SwampShore
        return FMath::Lerp(0.05f, 0.25f, HeightValue);

    default:
        // For unrecognized biomes, return the original height
        return HeightValue;
    }
}



FLinearColor ADiamondSquare::GetColorBasedOnBiomeAndHeight(float Z, ECell BiomeType)
{
    FLinearColor Color; // Declare the color variable

    // Assign colors based on biome type. Adjust the shades if needed to reflect different heights within the biomes.
    switch (BiomeType)
    {
    case ECell::Ocean:
        Color = FLinearColor(0.0f, 0.2509f, 0.501f); // Shallow Water
        break;
    case ECell::DeepOcean:
        Color = FLinearColor(0.05f, 0.19f, 0.57f); // Deep Water
        break;
    case ECell::Tundra:
        Color = FLinearColor::White;
        break;
    case ECell::SnowyForest:
        Color = FLinearColor(0.85f, 0.85f, 0.85f); // Lighter shade for snowy overlay on trees
        break;
    case ECell::Mountain:
        if (Z > 0.8f) Color = FLinearColor::White; // Snow capped peaks
        else Color = FLinearColor(0.50f, 0.50f, 0.50f); // Mountain Rock
        break;
    case ECell::Plains:
        Color = FLinearColor(0.24f, 0.70f, 0.44f); // Grass Green
        break;
    case ECell::Beach:
        Color = FLinearColor(0.82f, 0.66f, 0.42f); // Sandy Beach
        break;
    case ECell::ColdBeach:
        Color = FLinearColor(0.627f, 0.706f, 0.784f); // Cold Beach
        break;
    case ECell::Desert:
        Color = FLinearColor(0.82f, 0.66f, 0.42f); // Sand
        break;
    case ECell::River:
        Color = FLinearColor(0.50f, 0.73f, 0.93f); // Freshwater color
        break;
    case ECell::Taiga:
        Color = (Z > 0.5f) ? FLinearColor(0.52f, 0.37f, 0.26f) : FLinearColor(0.20f, 0.40f, 0.20f); // Darker Green for dense forestation
        break;
    case ECell::Rainforest:
        Color = FLinearColor(0.13f, 0.55f, 0.13f); // Lush Green
        break;
    case ECell::Savannah:
        Color = FLinearColor(0.85f, 0.75f, 0.45f); // Dry Grass
        break;
    case ECell::Swamp:
        Color = FLinearColor(0.47f, 0.60f, 0.33f); // Murky Green
        break;
    case ECell::Woodland:
        Color = FLinearColor(0.30f, 0.50f, 0.28f); // Forest Green
        break;
    case ECell::Forest:
        Color = FLinearColor(0.25f, 0.40f, 0.18f); // Deep Forest Green
        break;
    case ECell::Highland:
        Color = (Z > 0.75f) ? FLinearColor::White : FLinearColor(0.502f, 0.502f, 0.502f); // Rocky Terrain
        break;
    case ECell::IcePlains:
        Color = FLinearColor(0.90f, 0.90f, 0.98f); // Very Light Blue, almost white
        break;
    case ECell::SwampShore:
        Color = FLinearColor(0.306f,0.369f,0.224f);
        break;
    case ECell::Land:
        Color = FLinearColor::Black; 
        break;
    case ECell::Ice:
        Color = FLinearColor(191,199,214);
        break;
        // Add more biome cases as necessary.

    default:
        // If the biome type is unrecognized, use a default color red
        Color = FLinearColor::Red;
    }

    // Generate random variations in the RGB components
    float variation = 0.05f; // Adjust this value for more or less variation
    Color.R = FMath::Clamp(Color.R + Rng.FRandRange(-variation, variation), 0.0f, 1.0f);
    Color.G = FMath::Clamp(Color.G + Rng.FRandRange(-variation, variation), 0.0f, 1.0f);
    Color.B = FMath::Clamp(Color.B + Rng.FRandRange(-variation, variation), 0.0f, 1.0f);

    return Color;
}



// Example usage within the ADiamondSquare class
TArray<TArray<ADiamondSquare::ECell>> ADiamondSquare::TestIsland()
{
    double StartTimeTI = FPlatformTime::Seconds();
    TArray<TArray<ECell>> Board;
    InitializeSeed();
    Island(Board); // Adjust Island function to return the board
    Board = FuzzyZoom(Board);;
    Board = AddIsland(Board);
    Board = Zoom(Board);
    Board = AddIsland(Board);
    Board = AddIsland(Board);
    Board = AddIsland(Board);
    Board = RemoveTooMuchOcean(Board);
    Board = AddTemps(Board);
    Board = AddIsland2(Board);
    Board = WarmToTemperate(Board);
    Board = FreezingToCold(Board);
    Board = Zoom(Board);
    Board = AddIsland2(Board);
    if (SurroundMapWithOcean) {
        Board = SurroundWithOcean(Board);
    }
    Board = Zoom(Board);
    Board = TemperatureToBiome(Board);
    Board = DeepOcean(Board);
    Board = Zoom(Board);
    Board = Zoom(Board);
    Board = Zoom(Board);
    //Board = AddIsland2(Board);
    Board = Zoom(Board);
    Board = Shore(Board);
    Board = Zoom(Board);
    //Board = Zoom(Board);
    //Board = Zoom(Board);

    if (Board.Num() > 0)
    {
        int NumRows = Board.Num();
        int NumColumns = Board[0].Num();  // Assuming all rows are of the same length
        UE_LOG(LogTemp, Warning, TEXT("Board Size: %d x %d"), NumRows, NumColumns);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Board is empty"));
    }
    //PrintBoard(Board); // Print the resulting board
    double EndTimeTI = FPlatformTime::Seconds();
    double ElapsedTimeTI = EndTimeTI - StartTimeTI;
    UE_LOG(LogTemp, Warning, TEXT("BiomeMap took %f seconds"), ElapsedTimeTI);
    return Board;
}

void ADiamondSquare::InitializeSeed()
{
    Rng.Initialize(Seed);
    UE_LOG(LogTemp, Warning, TEXT("Random Number Generator Seeded with: %d"), Seed);
}


TArray<TArray<ADiamondSquare::ECell>> ADiamondSquare::DeepOcean(const TArray<TArray<ADiamondSquare::ECell>>& Board) {
    // Make a copy of the board to modify and return.
    TArray<TArray<ECell>> ModifiedBoard = Board;

    // Helper lambda to check if a cell is within bounds and is an Ocean.
    auto IsOcean = [&](int32 Row, int32 Col) -> bool {
        return Row >= 0 && Row < Board.Num() && Col >= 0 && Col < Board[Row].Num() && Board[Row][Col] == ECell::Ocean;
        };

    // Iterate over each cell in the board, this time considering the extended radius for checking.
    for (int32 Row = 0; Row < Board.Num(); ++Row) {
        for (int32 Col = 0; Col < Board[Row].Num(); ++Col) {
            // Check if the current cell is Ocean.
            if (Board[Row][Col] == ECell::Ocean) {
                bool bIsSurroundedByOcean = true;

                // Check cells within a radius of 2 around the current cell.
                for (int32 dRow = -1; dRow <= 1 && bIsSurroundedByOcean; ++dRow) {
                    for (int32 dCol = -1; dCol <= 1; ++dCol) {
                        // Skip the check for the current cell itself.
                        if (dRow == 0 && dCol == 0) continue;

                        // If any cell within the radius is not ocean, mark as not surrounded.
                        if (!IsOcean(Row + dRow, Col + dCol)) {
                            bIsSurroundedByOcean = false;
                            break; // Break inner loop
                        }
                    }
                }

                // If the cell is surrounded by ocean within a radius of 2, change it to DeepOcean.
                if (bIsSurroundedByOcean) {
                    ModifiedBoard[Row][Col] = ECell::DeepOcean;
                }
            }
        }
    }

    // Return the modified board.
    return ModifiedBoard;
}


TArray<TArray<ADiamondSquare::ECell>> ADiamondSquare::AddTemps(const TArray<TArray<ECell>>& Board)
{
    const int32 Rows = Board.Num();
    const int32 Cols = Board[0].Num();
    TArray<TArray<ECell>> NewBoard = Board;

    for (int32 i = 0; i < Rows; ++i)
    {
        for (int32 j = 0; j < Cols; ++j)
        {
            if (NewBoard[i][j] == ECell::Ocean) continue;

            // Generate a random number to determine the temperature
            int32 Temp = Rng.RandRange(1, 6); // Generates a number between 1 and 6

            if (Temp <= 4) // 1-4 are warm
            {
                NewBoard[i][j] = ECell::Warm;
            }
            else if (Temp == 5) // 5 is cold
            {
                NewBoard[i][j] = ECell::Cold;
            }
            else // 6 is freezing
            {
                NewBoard[i][j] = ECell::Freezing;
            }
        }
    }

    return NewBoard;
}

TArray<TArray<ADiamondSquare::ECell>> ADiamondSquare::Zoom(const TArray<TArray<ECell>>& Board)
{
    int32 Rows = Board.Num();
    int32 Cols = Board[0].Num();
    TArray<TArray<ECell>> ScaledBoard;

    // Scale the board by a factor of 2
    ScaledBoard.Reserve(Rows * 2);
    for (int32 i = 0; i < Rows; ++i)
    {
        TArray<ECell> NewRow;
        NewRow.Reserve(Cols * 2);
        for (int32 j = 0; j < Cols; ++j)
        {
            NewRow.Add(Board[i][j]);
            NewRow.Add(Board[i][j]);
        }
        ScaledBoard.Add(NewRow);
        ScaledBoard.Add(NewRow);
    }

    int32 ScaledRows = ScaledBoard.Num();
    int32 ScaledCols = ScaledBoard[0].Num();
    TArray<int32> Indexes = { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 };

    for (int32 i = 0; i < ScaledRows; ++i)
    {
        for (int32 j = 0; j < ScaledCols; ++j)
        {
            if (IsEdgeCell(ScaledBoard, i, j))
            {
                // Introduce more randomness in how we choose to modify the cell
                int32 RandIndex = FMath::RandRange(0, Indexes.Num() - 1);
                int32 xoff = Indexes[RandIndex];
                RandIndex = FMath::RandRange(0, Indexes.Num() - 1);
                int32 yoff = Indexes[RandIndex];

                int32 new_i = FMath::Clamp(i + xoff, 0, ScaledRows - 1);
                int32 new_j = FMath::Clamp(j + yoff, 0, ScaledCols - 1);

                ScaledBoard[i][j] = ScaledBoard[new_i][new_j];
            }
        }
    }

    return ScaledBoard;
}


TArray<TArray<ADiamondSquare::ECell>> ADiamondSquare::AddIsland(const TArray<TArray<ADiamondSquare::ECell>>& Board)
{
    const int32 Rows = Board.Num();
    const int32 Cols = Board[0].Num();
    TArray<TArray<ECell>> NextBoard = Board; // Copy the original board to modify

    for (int32 i = 0; i < Rows; ++i) {
        for (int32 j = 0; j < Cols; ++j) {
            if (IsEdgeCell(Board, i, j) && CanTransform(Board[i][j])) {
                ECell NewState = Rng.FRand() < ProbabilityOfLand ? ECell::Land : ECell::Ocean;
                NextBoard[i][j] = NewState;
            }
        }
    }

    return NextBoard;
}

TArray<TArray<ADiamondSquare::ECell>> ADiamondSquare::AddIsland2(const TArray<TArray<ADiamondSquare::ECell>>& Board)
{
    const int32 Rows = Board.Num();
    const int32 Cols = Board[0].Num(); // Assuming the board is at least 1x1
    TArray<TArray<ECell>> NextBoard = Board; // Assuming you have a way to deep copy

    // Directions to check: Up, Down, Left, Right
    TArray<FIntPoint> Directions = {
        FIntPoint(-1, 0), // Up
        FIntPoint(1, 0),  // Down
        FIntPoint(0, -1), // Left
        FIntPoint(0, 1)   // Right
    };

    for (int32 i = 0; i < Rows; ++i) {
        for (int32 j = 0; j < Cols; ++j) {
            // Use the IsEdgeCell function
            if (IsEdgeCell(Board, i, j) && CanTransform(Board[i][j])) {
                // Map to count the occurrences of each ECell type, excluding Ocean
                TMap<ECell, int32> CellTypeCounts;

                for (const FIntPoint& Dir : Directions) {
                    int32 NR = i + Dir.X;
                    int32 NC = j + Dir.Y;

                    // Ensure the neighbor is within bounds
                    if (NR >= 0 && NR < Rows && NC >= 0 && NC < Cols) {
                        ECell NeighborCell = Board[NR][NC];
                        // Increment count if not Ocean
                        if (NeighborCell != ECell::Ocean) {
                            CellTypeCounts.FindOrAdd(NeighborCell)++;
                        }
                    }
                }

                // Determine the majority cell type, excluding Ocean
                ECell MajorityType = ECell::Ocean; // Default to Ocean if no majority found
                int32 MaxCount = 0;
                for (const auto& Kvp : CellTypeCounts) {
                    if (Kvp.Value > MaxCount) {
                        MajorityType = Kvp.Key;
                        MaxCount = Kvp.Value;
                    }
                }

                // If a majority type is found, update the cell
                if (MajorityType != ECell::Ocean && Rng.FRand() <= ProbabilityOfLand) {
                    NextBoard[i][j] = MajorityType;
                }

            }
        }
    }

    return NextBoard;
}




// CanTransform checks if a cell of a given type is eligible for transformation.
// It returns true if the cell can be transformed, and false otherwise.
bool ADiamondSquare::CanTransform(ECell CellType) const {
    // Define which cell types should not be transformed.
    // Assuming TempCold, TempWarm, etc., represent temperature states that should be preserved.
    TArray<ECell> NonTransformableTypes = { ECell::Temperate, ECell::Warm, ECell::Cold, ECell::Freezing, ECell::Temperate, };

    // Check if the cell type is in the list of types that should not be transformed.
    return !NonTransformableTypes.Contains(CellType);
}


TArray<TArray<ADiamondSquare::ECell>> ADiamondSquare::FuzzyZoom(const TArray<TArray<ADiamondSquare::ECell>>& Board)
{
    const int32 Rows = Board.Num();
    const int32 Cols = Board[0].Num();
    TArray<TArray<ECell>> ScaledBoard;

    // Scale the board by a factor of 2
    ScaledBoard.SetNum(Rows * 2);
    for (int32 i = 0; i < Rows * 2; ++i)
    {
        ScaledBoard[i].SetNum(Cols * 2);
        for (int32 j = 0; j < Cols * 2; ++j)
        {
            ScaledBoard[i][j] = Board[i / 2][j / 2];
        }
    }

    for (int32 i = 0; i < Rows * 2; ++i)
    {
        for (int32 j = 0; j < Cols * 2; ++j)
        {
            if (IsEdgeCell(ScaledBoard, i, j))
            {
                // Generate xoff and yoff uniformly from [-1, 0, 1]
                int32 xoff = Rng.RandRange(-1, 1);
                int32 yoff = Rng.RandRange(-1, 1);

                // Update the cell value, ensuring we stay within bounds
                int32 new_i = FMath::Clamp(i + xoff, 0, Rows * 2 - 1);
                int32 new_j = FMath::Clamp(j + yoff, 0, Cols * 2 - 1);
                ScaledBoard[i][j] = ScaledBoard[new_i][new_j];
            }
        }
    }

    return ScaledBoard;
}


bool ADiamondSquare::IsEdgeCell(const TArray<TArray<ECell>>& Board, int32 R, int32 C)
{
    // Assuming Board is a valid 2D array with dimensions already checked elsewhere
    if (Board.Num() == 0 || Board[0].Num() == 0)
    {
        return false; // Early exit if the board is empty or not properly initialized
    }

    int32 Dims = Board.Num(); // Assuming square board for simplicity
    ECell Key = Board[R][C];

    // Directions to check: Up, Down, Left, Right
    TArray<FIntPoint> Directions = {
        FIntPoint(-1, 0), // Up
        FIntPoint(1, 0),  // Down
        FIntPoint(0, -1), // Left
        FIntPoint(0, 1)   // Right
    };

    for (FIntPoint Dir : Directions)
    {
        int32 NR = R + Dir.X;
        int32 NC = C + Dir.Y;

        // Check if the new row and column are within the board bounds
        if (NR >= 0 && NR < Dims && NC >= 0 && NC < Dims)
        {
            // Check if the adjacent cell is different from the current cell
            if (Board[NR][NC] != Key)
            {
                return true; // This is an edge cell
            }
        }
    }

    return false; // Not an edge cell
}


TArray<TArray<ADiamondSquare::ECell>> ADiamondSquare::Island(TArray<TArray<ECell>>& Board)
{
    // Assuming ECell is the enum with Land and Ocean
    const float ProbLand = 0.1f;
    Board.SetNum(4);

    // Initialize the board with ocean cells
    for (int32 i = 0; i < 4; ++i)
    {
        Board[i].SetNum(4);
        for (int32 j = 0; j < 4; ++j)
        {
            Board[i][j] = ECell::Ocean;
        }
    }

    // Populate the board with land cells based on ProbLand
    for (int32 i = 0; i < 4; ++i)
    {
        for (int32 j = 0; j < 4; ++j)
        {
            if (Rng.FRand() <= ProbLand) // FRand() returns a float between 0.0 and 1.0
            {
                Board[i][j] = ECell::Land;
            }
        }
    }
    return Board;

    // At this point, Board is filled with cells. You can return it or use it as needed.
    // Note: Adjustments might be needed based on how you plan to use the board.
}



TArray<TArray<ADiamondSquare::ECell>> ADiamondSquare::RemoveTooMuchOcean(const TArray<TArray<ADiamondSquare::ECell>>& Board)
{
    const float PLand = 0.35f; // Probability of changing an ocean cell to land

    TArray<TArray<ECell>> NewBoard = Board; // Assuming deep copy isn't needed

    for (int32 i = 0; i < Board.Num(); ++i)
    {
        for (int32 j = 0; j < Board[i].Num(); ++j)
        {
            if (Board[i][j] == ECell::Ocean && IsSurroundedByOcean(Board, i, j))
            {
                if (Rng.FRand() < PLand) // Chance to convert to land
                {
                    NewBoard[i][j] = ECell::Land;
                }
            }
        }
    }

    return NewBoard;
}


bool ADiamondSquare::IsSurroundedByOcean(const TArray<TArray<ADiamondSquare::ECell>>& Board, int32 i, int32 j)
{
    const int32 Rows = Board.Num();
    const int32 Cols = Board[0].Num();

    if (i > 0 && Board[i - 1][j] != ECell::Ocean) return false; // Check up
    if (i < Rows - 1 && Board[i + 1][j] != ECell::Ocean) return false; // Check down
    if (j > 0 && Board[i][j - 1] != ECell::Ocean) return false; // Check left
    if (j < Cols - 1 && Board[i][j + 1] != ECell::Ocean) return false; // Check right

    return true; // Surrounded by ocean
}


bool ADiamondSquare::IsAdjacentToGroup(const TArray<TArray<ECell>>& Board, int32 X, int32 Y, const TSet<ECell>& GroupA, const TSet<ECell>& GroupB)
{
    const int32 Rows = Board.Num();
    const int32 Cols = Board[0].Num();

    for (int32 i = FMath::Max(0, X - 1); i <= FMath::Min(X + 1, Rows - 1); ++i)
    {
        for (int32 j = FMath::Max(0, Y - 1); j <= FMath::Min(Y + 1, Cols - 1); ++j)
        {
            if (i == X && j == Y) continue; // Skip the cell itself
            if (GroupA.Contains(Board[X][Y]) && GroupB.Contains(Board[i][j]))
                return true;
        }
    }

    return false;
}


TArray<TArray<ADiamondSquare::ECell>> ADiamondSquare::WarmToTemperate(const TArray<TArray<ECell>>& Board)
{
    // First, create a copy of the Board to modify and return
    TArray<TArray<ECell>> ModifiedBoard = Board;

    // Get the number of rows and columns
    int32 Rows = Board.Num();
    int32 Columns = Board[0].Num();

    // Iterate over each cell in the Board
    for (int32 Row = 0; Row < Rows; ++Row)
    {
        for (int32 Column = 0; Column < Columns; ++Column)
        {
            // Check if the current cell is Warm
            if (Board[Row][Column] == ECell::Warm)
            {
                // Check adjacent cells for Cold or Freezing
                bool AdjacentToCooler = false;

                // Check above
                if (Row > 0 && (Board[Row - 1][Column] == ECell::Cold || Board[Row - 1][Column] == ECell::Freezing))
                {
                    AdjacentToCooler = true;
                }

                // Check below
                if (Row < Rows - 1 && (Board[Row + 1][Column] == ECell::Cold || Board[Row + 1][Column] == ECell::Freezing))
                {
                    AdjacentToCooler = true;
                }

                // Check left
                if (Column > 0 && (Board[Row][Column - 1] == ECell::Cold || Board[Row][Column - 1] == ECell::Freezing))
                {
                    AdjacentToCooler = true;
                }

                // Check right
                if (Column < Columns - 1 && (Board[Row][Column + 1] == ECell::Cold || Board[Row][Column + 1] == ECell::Freezing))
                {
                    AdjacentToCooler = true;
                }

                // If adjacent to cooler cell, change to Temperate
                if (AdjacentToCooler)
                {
                    ModifiedBoard[Row][Column] = ECell::Temperate;
                }
            }
        }
    }

    // Return the modified board
    return ModifiedBoard;
}


void ADiamondSquare::SetBoardRegion(TArray<TArray<ECell>>& Board, int32 CenterX, int32 CenterY, int32 Radius, ECell NewState)
{
    const int32 Rows = Board.Num();
    const int32 Cols = Board[0].Num();

    for (int32 i = FMath::Max(0, CenterX - Radius); i <= FMath::Min(CenterX + Radius, Rows - 1); ++i)
    {
        for (int32 j = FMath::Max(0, CenterY - Radius); j <= FMath::Min(CenterY + Radius, Cols - 1); ++j)
        {
            Board[i][j] = NewState;
        }
    }
}


// Main function to convert freezing land adjacent to warm or temperate regions to cold
TArray<TArray<ADiamondSquare::ECell>> ADiamondSquare::FreezingToCold(const TArray<TArray<ECell>>& Board)
{
    TArray<TArray<ECell>> NextBoard = Board; // Copy board
    int32 Rows = Board.Num();
    int32 Cols = Board[0].Num(); // Assuming all rows are the same length

    for (int32 Row = 0; Row < Rows; ++Row)
    {
        for (int32 Column = 0; Column < Cols; ++Column)
        {
            // Check if the current cell is Freezing
            if (Board[Row][Column] == ECell::Freezing)
            {
                // Check adjacent cells for Warm or Temperate
                bool AdjacentToWarmer = false;

                // Directions: Up, Down, Left, Right
                TArray<FIntPoint> Directions = {
                    FIntPoint(-1, 0), // Up
                    FIntPoint(1, 0),  // Down
                    FIntPoint(0, -1), // Left
                    FIntPoint(0, 1)   // Right
                };

                for (const FIntPoint& Dir : Directions)
                {
                    int32 AdjRow = Row + Dir.X;
                    int32 AdjCol = Column + Dir.Y;

                    // Check bounds and then check for Warm or Temperate
                    if (AdjRow >= 0 && AdjRow < Rows && AdjCol >= 0 && AdjCol < Cols &&
                        (Board[AdjRow][AdjCol] == ECell::Warm || Board[AdjRow][AdjCol] == ECell::Temperate))
                    {
                        AdjacentToWarmer = true;
                        break; // Break as we only need one match
                    }
                }

                // If adjacent to a warmer cell, change to Cold
                if (AdjacentToWarmer)
                {
                    NextBoard[Row][Column] = ECell::Cold;
                }
            }
        }
    }

    return NextBoard;
}

ADiamondSquare::ECell ADiamondSquare::SelectBiome(const TArray<ECell>& Biomes, const TArray<float>& Odds)
{
    float Roll = Rng.FRand(); // Roll a random number between 0.0 and 1.0
    float Cumulative = 0.0f;
    for (int32 Index = 0; Index < Biomes.Num(); ++Index)
    {
        Cumulative += Odds[Index];
        if (Roll < Cumulative)
        {
            return Biomes[Index];
        }
    }
    return Biomes.Last(); // Default to the last biome if none selected
}


TArray<TArray<ADiamondSquare::ECell>> ADiamondSquare::TemperatureToBiome(const TArray<TArray<ECell>>& Board)
{
    TArray<TArray<ECell>> NewBoard = Board;

    for (int32 Row = 0; Row < Board.Num(); ++Row)
    {
        for (int32 Col = 0; Col < Board[Row].Num(); ++Col)
        {
            if (Board[Row][Col] != ECell::Ocean)
            {
                // Example mapping for Warm temperature to biomes
                if (Board[Row][Col] == ECell::Warm)
                {
                    TArray<ECell> Biomes = { ECell::Desert, ECell::Plains, ECell::Rainforest, ECell::Savannah, ECell::Swamp };
                    TArray<float> Odds = { 0.2f, 0.4f, 0.18f, 0.2f, 0.02f };
                    NewBoard[Row][Col] = SelectBiome(Biomes, Odds);
                }
                else if (Board[Row][Col] == ECell::Temperate)
                {
                    TArray<ECell> Biomes = { ECell::Woodland, ECell::Forest, ECell::Highland };
                    TArray<float> Odds = { 0.2f, 0.55f, 0.25f };
                    NewBoard[Row][Col] = SelectBiome(Biomes, Odds);
                }
                else if (Board[Row][Col] == ECell::Cold)
                {
                    TArray<ECell> Biomes = { ECell::Taiga, ECell::SnowyForest };
                    TArray<float> Odds = { 0.5f, 0.5f };
                    NewBoard[Row][Col] = SelectBiome(Biomes, Odds);
                }
                else if (Board[Row][Col] == ECell::Freezing)
                {
                    TArray<ECell> Biomes = { ECell::Tundra, ECell::IcePlains };
                    TArray<float> Odds = { 0.7f, 0.3f };
                    NewBoard[Row][Col] = SelectBiome(Biomes, Odds);
                }
            }
        }
    }

    return NewBoard;
}


TArray<TArray<ADiamondSquare::ECell>> ADiamondSquare::SurroundWithOcean(TArray<TArray<ECell>>& Board)
{
    int32 Rows = Board.Num();
    if (Rows == 0) return Board;

    int32 Cols = Board[0].Num();

    // Set the top and bottom rows to Ocean
    for (int32 Col = 0; Col < Cols; ++Col)
    {
        Board[0][Col] = ECell::Ocean;
        Board[Rows - 1][Col] = ECell::Ocean;
    }

    // Set the first and last columns to Ocean
    for (int32 Row = 0; Row < Rows; ++Row)
    {
        Board[Row][0] = ECell::Ocean;
        Board[Row][Cols - 1] = ECell::Ocean;
    }

    return Board;
}


void ADiamondSquare::PrintBoard(const TArray<TArray<ECell>>& Board)
{
    FString BoardString;

    for (int32 i = 0; i < Board.Num(); ++i)
    {
        for (int32 j = 0; j < Board[i].Num(); ++j)
        {
            switch (Board[i][j])
            {
            case ECell::Land:
                BoardString += TEXT("L "); // Land
                break;
            case ECell::Ocean:
                BoardString += TEXT("O "); // Ocean
                break;
            case ECell::Warm:
                BoardString += TEXT("W "); // Warm
                break;
            case ECell::Temperate:
                BoardString += TEXT("T "); // Temperate
                break;
            case ECell::Cold:
                BoardString += TEXT("C "); // Cold
                break;
            case ECell::Freezing:
                BoardString += TEXT("F "); // Freezing
                break;
                // Adding new biome types
            case ECell::Desert:
                BoardString += TEXT("D "); // Desert
                break;
            case ECell::Plains:
                BoardString += TEXT("P "); // Plains
                break;
            case ECell::Rainforest:
                BoardString += TEXT("R "); // Rainforest
                break;
            case ECell::Savannah:
                BoardString += TEXT("S "); // Savannah
                break;
            case ECell::Swamp:
                BoardString += TEXT("M "); // Swamp
                break;
            case ECell::Woodland:
                BoardString += TEXT("w "); // Woodland
                break;
            case ECell::Forest:
                BoardString += TEXT("f "); // Forest
                break;
            case ECell::Highland:
                BoardString += TEXT("h "); // Highland
                break;
            case ECell::Taiga:
                BoardString += TEXT("t "); // Taiga
                break;
            case ECell::SnowyForest:
                BoardString += TEXT("s "); // Snowy Forest
                break;
            case ECell::Tundra:
                BoardString += TEXT("t "); // Tundra
                break;
            case ECell::IcePlains:
                BoardString += TEXT("i "); // Ice Plains
                break;
            default:
                BoardString += TEXT("? "); // Unknown cell type
                break;
            }
        }
        BoardString += TEXT("\n");
    }

    // Print the board to the output log
    UE_LOG(LogTemp, Warning, TEXT("%s"), *BoardString);
}

TArray<TArray<ADiamondSquare::ECell>> ADiamondSquare::Shore(const TArray<TArray<ADiamondSquare::ECell>>& Board) {
    TArray<TArray<ECell>> ModifiedBoard = Board; // Make a copy of the board to modify and return.

    int ShoreDepth = 0; // Assuming we want beaches to be 1 cell wide

    TSet<ECell> IgnoreSet = { ECell::Tundra, ECell::IcePlains, ECell::Taiga, ECell::SnowyForest, ECell::DeepOcean };
    TSet<ECell> OceanSet = { ECell::Ocean }; 
    TSet<ECell> DeepOceanSet = { ECell::DeepOcean };

    for (int32 Row = 0; Row < Board.Num(); ++Row) {
        for (int32 Col = 0; Col < Board[Row].Num(); ++Col) {
            ECell CurrentCell = Board[Row][Col];
            // Check adjacency to Ocean and ensure it is not adjacent to Deep Ocean
            if (CurrentCell != ECell::Ocean && IsAdjacentToGroup(Board, Row, Col, { CurrentCell }, OceanSet) &&
                !IsAdjacentToGroup(Board, Row, Col, { CurrentCell }, DeepOceanSet)) {
                if (IgnoreSet.Contains(CurrentCell)) {
                    // Set cells from IgnoreSet that are adjacent to ocean but not adjacent to deep ocean to ColdBeach
                    SetBoardRegion(ModifiedBoard, Row, Col, ShoreDepth, ECell::ColdBeach);
                }
                else if (CurrentCell == ECell::Swamp) {
                    // Special treatment for swamp cells adjacent to ocean
                    SetBoardRegion(ModifiedBoard, Row, Col, ShoreDepth, ECell::SwampShore);
                }
                else {
                    // Standard treatment for other land cells adjacent to ocean
                    SetBoardRegion(ModifiedBoard, Row, Col, ShoreDepth, ECell::Beach);
                }
            }
        }
    }


    return ModifiedBoard;
}


