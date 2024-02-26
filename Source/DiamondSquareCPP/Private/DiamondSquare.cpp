#include "DiamondSquare.h"
#include "Engine/World.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDiamondSquare, Log, All);
DEFINE_LOG_CATEGORY(LogDiamondSquare);


ADiamondSquare::ADiamondSquare()
{
    PrimaryActorTick.bCanEverTick = false;

    ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>("ProceduralMesh");
    ProceduralMesh->SetupAttachment(GetRootComponent());
}


// ADiamondSquare::OnConstruction
// -------------------------------
// Purpose: 
//   Constructs a procedural mesh for the ADiamondSquare class. This function is called
//   when an instance of ADiamondSquare is constructed or modified in the Unreal Editor.
//   It uses Perlin noise for vertex generation and creates a mesh with calculated normals
//   and tangents, applying a specified material.
//
// Parameters:
//   - const FTransform& Transform: The transformation (position, rotation, scale) applied to the object.
//
// Usage:
//   This function is automatically invoked during the construction of an ADiamondSquare object
//   or when its properties are updated in the editor. It is responsible for the procedural
//   generation and updating of the mesh based on the current parameters and noise map.
//
// Note:
//   The function depends on Unreal Engine's procedural mesh capabilities and is designed for use
//   within this game engine.
void ADiamondSquare::OnConstruction(const FTransform& Transform)
{
    // Call the superclass's OnConstruction to handle basic setup
    Super::OnConstruction(Transform);

    // Check if the mesh needs to be recreated
    if (recreateMesh) {
        // Generate a Perlin noise map for terrain-like structure
        //TestIsland();

        auto NoiseMap = GeneratePerlinNoiseMap();

        // Reset mesh data to prepare for new mesh creation
        Vertices.Reset();
        Triangles.Reset();
        UV0.Reset();

        // Create vertices and triangles for the mesh
        CreateVertices(NoiseMap);
        CreateTriangles();

        // Calculate normals and tangents for the mesh
        UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UV0, Normals, Tangents);

        // Create the mesh section with the specified data and apply the material
        ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UV0, Colors, Tangents, true);
        ProceduralMesh->SetMaterial(0, Material);

        // Reset the flag to avoid unnecessary mesh recreation
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


void ADiamondSquare::CreateTriangles()
{
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
}



void ADiamondSquare::CreateVertices(const TArray<TArray<float>>& NoiseMap)
{
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

    // Log the size of the Colors and Vertices arrays
    UE_LOG(LogTemp, Warning, TEXT("Size of Colors array: %d"), Colors.Num());
    UE_LOG(LogTemp, Warning, TEXT("Size of Vertices array: %d"), Vertices.Num());

    // Check and log if Colors and Vertices arrays sizes are mismatched
    if (Colors.Num() != Vertices.Num()) {
        UE_LOG(LogTemp, Error, TEXT("Error: Colors array and Vertices array are not the same size."));
    }
}



TArray<TArray<float>> ADiamondSquare::GeneratePerlinNoiseMap()
{
    // Reset and create the BiomeMap
    BiomeMap.Empty();
    BiomeMap = TestIsland();

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

    // Return the generated Perlin noise map
    return NoiseMap;
}


float ADiamondSquare::GetInterpolatedHeight(float HeightValue, ECell BiomeType)
{
    switch (BiomeType)
    {
    case ECell::Ocean: // Ocean
        return AdjustForOcean(HeightValue);
    case ECell::SnowyForest: // Snowy Forest
        return AdjustForSnowyForest(HeightValue);
    case ECell::Mountain: // Mountain
        return AdjustForMountain(HeightValue);
    case ECell::Plains: // Plains
        return AdjustForPlains(HeightValue);
    case ECell::Beach: // Beach
        return AdjustForBeach(HeightValue);
    case ECell::Desert: // Desert
        return AdjustForDesert(HeightValue);
    case ECell::River: // River
        return AdjustForRiver(HeightValue);
    case ECell::Taiga: // Taiga
        return AdjustForTaiga(HeightValue);
    case ECell::Forest: // Regular Forest
        return AdjustForForest(HeightValue);
    case ECell::Swamp: // Swamp
        return AdjustForSwamp(HeightValue);
    case ECell::Tundra: // Additional Tundra case, if distinct from 'Snow'
        return AdjustForTundra(HeightValue);
    case ECell::Rainforest: // Rainforest
        return AdjustForRainforest(HeightValue);
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
        if (Z < 0.1f) Color = FLinearColor(0.05f, 0.19f, 0.57f); // Deep Water
        else Color = FLinearColor(0.28f, 0.46f, 0.80f); // Shallow Water
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
        Color = FLinearColor(0.65f, 0.50f, 0.39f); // Rocky Terrain
        break;
    case ECell::IcePlains:
        Color = FLinearColor(0.90f, 0.90f, 0.98f); // Very Light Blue, almost white
        break;
    case ECell::Land:
        Color = FLinearColor::Black; // Sandy Beach
        break;
        // Add more biome cases as necessary.

    default:
        // If the biome type is unrecognized, use a default color red
        Color = FLinearColor::Red;
        break;
    }

    return Color;
}





// Example usage within the ADiamondSquare class
TArray<TArray<ADiamondSquare::ECell>> ADiamondSquare::TestIsland()
{
    TArray<TArray<ECell>> Board;
    Island(Board); // Adjust Island function to return the board
    Board = FuzzyZoom(Board);
    Board = AddIsland(Board);
    Board = Zoom(Board);
    Board = AddIsland(Board);
    Board = AddIsland(Board);
    Board = AddIsland(Board);
    Board = RemoveTooMuchOcean(Board);
    Board = AddTemps(Board);
    PrintBoard(Board);
    //Board = AddIsland(Board);
    Board = WarmToTemperate(Board);
    PrintBoard(Board);
    Board = FreezingToCold(Board);
    Board = Zoom(Board);
    Board = Zoom(Board);
    Board = AddIsland(Board);
    Board = TemperatureToBiome(Board);
    Board = Zoom(Board);
    Board = Zoom(Board);
    Board = Zoom(Board);
    Board = AddIsland(Board);
    Board = Zoom(Board);
    //PrintBoard(Board); // Print the resulting board
    return Board;
}


TArray<TArray<ADiamondSquare::ECell>> ADiamondSquare::AddTemps(const TArray<TArray<ADiamondSquare::ECell>>& Board)
{
    const int32 Rows = Board.Num();
    const int32 Cols = Board[0].Num();
    TArray<TArray<ECell>> NewBoard = Board; // Assuming a shallow copy is sufficient for your use case

    FRandomStream Rng;
    Rng.GenerateNewSeed();

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


TArray<TArray<ADiamondSquare::ECell>> ADiamondSquare::Zoom(const TArray<TArray<ADiamondSquare::ECell>>& Board)
{
    // Predefined indexes array simulating a non-uniform distribution
    TArray<int32> Indexes = { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 };
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

    FRandomStream Rng;
    Rng.GenerateNewSeed();

    for (int32 i = 0; i < Rows * 2; ++i)
    {
        for (int32 j = 0; j < Cols * 2; ++j)
        {
            if (IsEdgeCell(ScaledBoard, i, j)) // Assuming IsEdgeCell is defined elsewhere
            {
                // Choose xoff and yoff from the predefined indexes
                int32 xoff = Indexes[Rng.RandRange(0, Indexes.Num() - 1)];
                int32 yoff = Indexes[Rng.RandRange(0, Indexes.Num() - 1)];

                // Update the cell value, ensuring we stay within bounds
                int32 new_i = FMath::Clamp(i + xoff, 0, Rows * 2 - 1);
                int32 new_j = FMath::Clamp(j + yoff, 0, Cols * 2 - 1);
                ScaledBoard[i][j] = ScaledBoard[new_i][new_j];
            }
        }
    }

    return ScaledBoard;
}


TArray<TArray<ADiamondSquare::ECell>> ADiamondSquare::AddIsland(const TArray<TArray<ADiamondSquare::ECell>>& Board)
{
    const float PLand = 0.6f;
    const int32 Rows = Board.Num();
    const int32 Cols = Board[0].Num();
    TArray<TArray<ECell>> NextBoard = Board; // Assuming you have a way to deep copy

    FRandomStream Rng;
    Rng.GenerateNewSeed();

    auto IsEdgeCell = [&](int32 r, int32 c) -> bool {
        if (Board[r][c] != ECell::Ocean) {
            for (const auto& Offset : { FIntPoint(-1, 0), FIntPoint(1, 0), FIntPoint(0, -1), FIntPoint(0, 1) }) {
                int32 nr = r + Offset.X;
                int32 nc = c + Offset.Y;
                if (nr >= 0 && nr < Rows && nc >= 0 && nc < Cols && Board[nr][nc] == ECell::Ocean) {
                    return true;
                }
            }
        }
        else {
            for (const auto& Offset : { FIntPoint(-1, 0), FIntPoint(1, 0), FIntPoint(0, -1), FIntPoint(0, 1) }) {
                int32 nr = r + Offset.X;
                int32 nc = c + Offset.Y;
                if (nr >= 0 && nr < Rows && nc >= 0 && nc < Cols && Board[nr][nc] != ECell::Ocean) {
                    return true;
                }
            }
        }
        return false;
        };

    for (int32 i = 0; i < Rows; ++i) {
        for (int32 j = 0; j < Cols; ++j) {
            if (IsEdgeCell(i, j)) {
                ECell NewState = Rng.FRand() < PLand ? ECell::Land : ECell::Ocean;
                NextBoard[i][j] = NewState;
            }
        }
    }

    return NextBoard;
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

    // Random number generator
    FRandomStream Rng;
    Rng.GenerateNewSeed();

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


bool ADiamondSquare::IsEdgeCell(const TArray<TArray<ADiamondSquare::ECell>>& Board, int32 i, int32 j)
{
    // Example implementation, assuming edge cells are those on the boundary of the board
    int32 Rows = Board.Num();
    int32 Cols = Board[0].Num();
    return i == 0 || j == 0 || i == Rows - 1 || j == Cols - 1;
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

    // Seed random number generator
    FRandomStream Rng;
    Rng.GenerateNewSeed();

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


TArray<TArray<ADiamondSquare::ECell>> ADiamondSquare::RemoveTooMuchOcean(const TArray<TArray<ADiamondSquare::ECell>>& Board)
{
    const float PLand = 0.35f; // Probability of changing an ocean cell to land
    FRandomStream Rng;
    Rng.GenerateNewSeed();

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


// Define the IsSurroundedByOcean helper function


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
    int32 Radius = 1;
    TArray<TArray<ECell>> NextBoard = Board; // Copy board
    int32 Rows = Board.Num();
    int32 Cols = Board[0].Num(); // Assuming all rows are the same length

    TSet<ECell> GroupA = { ECell::Freezing };
    TSet<ECell> GroupB = { ECell::Cold, ECell::Temperate };

    for (int32 i = 0; i < Rows; ++i)
    {
        for (int32 j = 0; j < Cols; ++j)
        {
            if (IsAdjacentToGroup(Board, i, j, GroupA, GroupB))
            {
                SetBoardRegion(NextBoard, i, j, Radius, ECell::Cold);
            }
        }
    }

    return NextBoard;
}


ADiamondSquare::ECell ADiamondSquare::SelectBiome(const TArray<ECell>& Biomes, const TArray<float>& Odds, FRandomStream& Rng)
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
    FRandomStream Rng;
    Rng.GenerateNewSeed();

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
                    TArray<float> Odds = { 0.3f, 0.3f, 0.2f, 0.1f, 0.1f };
                    NewBoard[Row][Col] = SelectBiome(Biomes, Odds, Rng);
                }
                else if (Board[Row][Col] == ECell::Temperate)
                {
                    TArray<ECell> Biomes = { ECell::Woodland, ECell::Forest, ECell::Highland };
                    TArray<float> Odds = { 0.5f, 0.25f, 0.25f };
                    NewBoard[Row][Col] = SelectBiome(Biomes, Odds, Rng);
                }
                else if (Board[Row][Col] == ECell::Cold)
                {
                    TArray<ECell> Biomes = { ECell::Taiga, ECell::SnowyForest };
                    TArray<float> Odds = { 0.5f, 0.5f };
                    NewBoard[Row][Col] = SelectBiome(Biomes, Odds, Rng);
                }
                else if (Board[Row][Col] == ECell::Freezing)
                {
                    TArray<ECell> Biomes = { ECell::Tundra, ECell::IcePlains };
                    TArray<float> Odds = { 0.7f, 0.3f };
                    NewBoard[Row][Col] = SelectBiome(Biomes, Odds, Rng);
                }
            }
        }
    }

    return NewBoard;
}

