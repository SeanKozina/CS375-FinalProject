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


// ADiamondSquare::CreateVertices
// ------------------------------
// Purpose: 
//   Generates vertices for the procedural mesh based on a noise map and biome map.
//   Each vertex is assigned a color based on the biome and its height value.
//
// Parameters:
//   - const TArray<TArray<float>>& NoiseMap: A 2D array representing the Perlin noise map.
//
// Functionality:
//   - Iterates over a grid defined by XSize and YSize, using the noise map to determine 
//     the Z-coordinate (height) of each vertex.
//   - Assigns colors to vertices based on biome type and height.
//   - Initializes normals and tangents for the mesh.
//   - Creates a mesh section with the generated vertices and other mesh data.
//
// Assumptions:
//   - 'XSize' and 'YSize' are the dimensions of the 2D grid.
//   - 'Colors', 'Vertices', 'Normals', 'Tangents', and 'UV0' are member arrays of the class.
//   - 'Scale', 'ZMultiplier', and 'UVScale' are scaling factors for the mesh.
//
// Logging:
//   - Logs the size of the Colors and Vertices arrays for debugging purposes.
//   - Checks and logs if the sizes of the Colors and Vertices arrays are mismatched.
void ADiamondSquare::CreateVertices(const TArray<TArray<float>>& NoiseMap)
{
    // Prepare the Colors array for new data
    Colors.Empty();

    // Check if the ProceduralMesh is valid
    if (ProceduralMesh)
    {
        // Iterate over each grid point to create vertices
        for (int X = 0; X < XSize; ++X)
        {
            for (int Y = 0; Y < YSize; ++Y)
            {
                float Z = NoiseMap[X][Y]; // Height value from the noise map
                TCHAR BiomeChar = BiomeMap[Y][X]; // Biome type for this vertex

                // Determine the color based on biome and height
                Color = GetColorBasedOnBiomeAndHeight(Z, BiomeChar);
                Colors.Add(Color.ToFColor(false));

                // Add vertex with calculated position and UV coordinates
                Z *= ZMultiplier;
                Z = pow(Z, ZExpo);
                Vertices.Add(FVector(X * Scale, Y * Scale, Z * ZMultiplier * Scale));
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



// ADiamondSquare::GeneratePerlinNoiseMap
// --------------------------------------
// Purpose: 
//   Generates a 2D Perlin noise map used for creating terrain-like structures.
//   The noise map is influenced by a biome map to create varied terrain features.
//
// Return:
//   - TArray<TArray<float>>: A 2D array representing the Perlin noise map.
//
// Functionality:
//   - Generates a Perlin noise map based on specified parameters like scale, octaves, persistence,
//     and lacunarity, adjusting the noise value based on biome characteristics.
//   - The noise map is used to determine the height of vertices in the mesh.
//
// Assumptions:
//   - 'XSize' and 'YSize' are the dimensions of the noise map grid.
//   - 'Scale', 'Octaves', 'Persistence', and 'Lacunarity' are parameters for Perlin noise generation.
//
// Logging:
//   - Logs a warning if the BiomeMap is empty or malformed.
TArray<TArray<float>> ADiamondSquare::GeneratePerlinNoiseMap()
{
    // Reset and create the BiomeMap
    BiomeMap.Empty();
    BiomeMap = CreateBiomeMap();

    // Check if the BiomeMap is valid
    if (BiomeMap.Num() == 0 || BiomeMap[0].Len() == 0)
    {
        // Log a warning if the BiomeMap is empty or malformed
        UE_LOG(LogTemp, Warning, TEXT("BiomeMap is empty or not correctly formed!"));
        return TArray<TArray<float>>();
    }

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
            TCHAR BiomeChar = BiomeMap[Y][X];
            NoiseHeight = GetInterpolatedHeight(NoiseHeight, BiomeChar);

            // Clamp the noise value to ensure it's within the expected range
            NoiseMap[X][Y] = FMath::Clamp(NoiseHeight, 0.0f, 1.0f);
        }
    }
    if (boolConvolve) {
        NoiseMap = Convolve2(NoiseMap, GetGaussianFilter(5,1.5), GetGaussianFilter(10, 1.5),EdgesMap);
    }
    // Return the generated Perlin noise map
    return NoiseMap;
}


// ADiamondSquare::CreateBiomeMap
// ------------------------------
// Purpose: 
//   Generates a biome map based on Perlin noise values for altitude, temperature, and humidity.
//   This map is used to determine the biome type for each point in the terrain.
//
// Return:
//   - TArray<FString>: A 2D array represented as a list of strings, where each character
//     in a string represents a biome type.
//
// Functionality:
//   - Generates noise maps for temperature and humidity.
//   - Determines the biome type for each point based on altitude, temperature, and humidity.
//   - Post-processes the biome map for smoother transitions and adherence to certain rules (like mountain-ocean adjacency).
//   - Logs the biome map for debugging purposes.
//
// Usage:
//   This function is called during the generation of the procedural terrain to create a
//   detailed biome map, which influences the color and structure of the terrain.
TArray<FString> ADiamondSquare::CreateBiomeMap() {
    BiomeMap.Empty();
    // Generate noise maps for temperature and humidity
    TArray<float> TemperatureMap;
    TArray<float> HumidityMap;
    for (int x = 0; x < XSize; x++) {
        for (int y = 0; y < YSize; y++) {
            // Calculate noise values for temperature and humidity.
            float temperatureNoise = FMath::PerlinNoise2D(FVector2D(x * TemperatureScale * BiomeScale, y * TemperatureScale * BiomeScale));
            float humidityNoise = FMath::PerlinNoise2D(FVector2D(x * HumidityScale * BiomeScale, y * HumidityScale * BiomeScale));

            // Store temperature and humidity values.
            TemperatureMap.Add(temperatureNoise);
            HumidityMap.Add(humidityNoise);
        }
    }

    for (int x = 0; x < XSize; x++) {
        FString row;

        for (int y = 0; y < YSize; y++) {
            // Get altitude using Perlin noise.
            float altitudeNoise = FMath::PerlinNoise2D(FVector2D(x * AltitudeScale * BiomeScale, y * AltitudeScale * BiomeScale));

            // Get temperature and humidity for the current position.
            float temperature = TemperatureMap[y * XSize + x];
            float humidity = HumidityMap[y * XSize + x];

            // Determine biome based on altitude, temperature, and humidity.
            FString biomeSymbol = DetermineBiome(altitudeNoise, temperature, humidity);

            // Add the biome symbol to the current row.
            row += biomeSymbol;
        }

        BiomeMap.Add(row);
    }

    // Post-process the biome map for biome transitions and mountain-ocean adjacency rules.
    PostProcessBiomeMap();

    // Log the biome map for debugging purposes.
    for (const FString& row : BiomeMap) {
        UE_LOG(LogDiamondSquare, Log, TEXT("%s"), *row);
    }
    return BiomeMap;
}


// ADiamondSquare::DetermineBiome
// ------------------------------
// Purpose: 
//   Determines the biome type based on altitude, temperature, and humidity.
//   This function uses various thresholds to classify regions into distinct biomes.
//
// Parameters:
//   - float altitude: The altitude value at a point.
//   - float temperature: The temperature value at a point, normalized between 0 and 1.
//   - float humidity: The humidity value at a point, normalized between 0 and 1.
//
// Return:
//   - FString: A string character representing the biome type.
//
// Functionality:
//   - Clamps temperature and humidity values to ensure they are within a valid range.
//   - Uses a series of if-else statements to determine the biome based on the given altitude, 
//     temperature, and humidity.
//   - Defines various biome types like ocean, beach, desert, plains, forests, highlands, etc.
//
// Usage:
//   This function is used in the process of creating a biome map for procedural terrain generation,
//   influencing the color and structure of the terrain.
FString ADiamondSquare::DetermineBiome(float altitude, float temperature, float humidity) {
    // Altitude thresholds for different terrain types.
    const float SeaLevel = 0.05f;
    const float PlainsLevel = 0.3f;
    const float MountainLevel = 0.7f;
    const float HighlandLevel = 0.5f;

    // Temperature thresholds for different climates.
    const float SnowTemperature = -0.3f;
    const float DesertTemperature = 0.7f;
    const float TundraTemperature = -0.7f;
    const float SavannaTemperature = 0.5f;
    const float TaigaTemperature = -0.2f;

    // Humidity thresholds for determining dry/wet biomes.
    const float DryHumidity = -0.5f;
    const float WetHumidity = 0.5f;
    const float RainforestHumidity = 0.7f;

    // Determine biomes based on altitude, temperature, and humidity.
    if (altitude < SeaLevel) {
        // Ocean-related biomes
        if (temperature > DesertTemperature && humidity < DryHumidity) {
            return TEXT("B"); // Beach
        }
        else {
            return TEXT("O"); // Ocean
        }
    }
    else if (altitude < PlainsLevel) {
        // Plains-related biomes
        if (temperature < TundraTemperature) {
            if (humidity > WetHumidity) {
                return TEXT("K"); // Snowy Forest
            }
            else if (temperature < SnowTemperature) {
                return TEXT("U"); // Tundra
            }
            else {
                return TEXT("I"); // Ice Plains
            }
        }
        else if (temperature > DesertTemperature && humidity < DryHumidity) {
            return TEXT("D"); // Desert
        }
        else if (humidity > DryHumidity && humidity < WetHumidity && temperature > SavannaTemperature) {
            return TEXT("S"); // Steppe
        }
        else if (humidity > WetHumidity) {
            return TEXT("X"); // Swamp
        }
        else {
            return TEXT("P"); // Plains
        }
    }
    else if (altitude < HighlandLevel) {
        // Mountain foothills related biomes
        if (temperature < TaigaTemperature && humidity > DryHumidity && humidity < WetHumidity) {
            return TEXT("T"); // Taiga
        }
        else if (humidity > WetHumidity && temperature > SnowTemperature && temperature < TaigaTemperature) {
            return TEXT("F"); // Forest
        }
        else if (humidity > RainforestHumidity && temperature > TaigaTemperature) {
            return TEXT("L"); // Rainforest
        }
        else if (humidity < DryHumidity && temperature > DesertTemperature) {
            return TEXT("V"); // Savanna
        }
        else if (humidity > DryHumidity && humidity < WetHumidity) {
            return TEXT("W"); // Woodlands
        }
        else {
            return TEXT("H"); // Highlands
        }
    }
    else {
        // For altitudes above HighlandLevel, likely snow-capped peaks
        return TEXT("M"); // Snowy Mountains
    }

    // In theory, the code should never reach this point.
    return TEXT("/"); // Something went wrong
}


// ADiamondSquare::PostProcessBiomeMap
// -----------------------------------
// Purpose: 
//   Applies post-processing techniques to the biome map to enhance realism and visual appeal.
//   This includes adding beach zones near oceans and potentially other modifications like smoothing edges.
//
// Functionality:
//   - Copies the original biome map to reference during changes.
//   - Iterates through each cell in the biome map, modifying cells based on their surroundings.
//   - Adds beaches adjacent to ocean cells within a defined width.
//   - Can be extended to include additional post-processing steps like smoothing biome edges or adding rivers.
//
// Usage:
//   This function is called after the initial biome map generation to refine and improve the map,
//   making the resultant terrain more realistic and visually appealing.
void ADiamondSquare::PostProcessBiomeMap() {
    // Copy the original BiomeMap for reference during changes
    TArray<FString> OriginalBiomeMap = BiomeMap;

    EdgesMap.Init(TArray<bool>(), XSize);
    const int BorderWidth = 3;
    bool isSnowBiome = false;

    for (int x = 0; x < XSize; x++) {
        EdgesMap[x].Init(false, YSize);
        for (int y = 0; y < YSize; y++) {
            TCHAR currentBiome = OriginalBiomeMap[x][y];

            // Check neighboring cells for edges
            for (int offsetX = -1; offsetX <= 1; offsetX++) {
                for (int offsetY = -1; offsetY <= 1; offsetY++) {
                    if (offsetX == 0 && offsetY == 0) continue;
                    int neighborY = y + offsetY;
                    int neighborX = x + offsetX;
                    if (neighborY >= 0 && neighborY < YSize && neighborX >= 0 && neighborX < XSize) {
                        TCHAR neighborBiome = OriginalBiomeMap[neighborX][neighborY];
                        if (neighborBiome != currentBiome) {
                            EdgesMap[x][y] = true;
                        }
                    }
                }
            }
            // Check for ocean cells to add beaches or ice
            if (OriginalBiomeMap[x][y] == TEXT('O')) {
                for (int dx = -BorderWidth; dx <= BorderWidth; dx++) {
                    for (int dy = -BorderWidth; dy <= BorderWidth; dy++) {
                        int adjY = y + dy;
                        int adjX = x + dx;
                        if ((dy == 0 && dx == 0) || adjY < 0 || adjY >= YSize || adjX < 0 || adjX >= XSize) continue;

                        // Determine if the adjacent cell is a snow biome
                        TCHAR adjBiome = OriginalBiomeMap[adjX][adjY];
                        if (adjBiome == TEXT('K') || adjBiome == TEXT('I') || adjBiome == TEXT('T') || adjBiome == TEXT('U')) {
                            isSnowBiome = true;
                        }

                        if (adjBiome != TEXT('O') && adjBiome != TEXT('B')) {
                            int distanceFromOcean = FMath::Max(FMath::Abs(dx), FMath::Abs(dy));
                            if (distanceFromOcean <= BorderWidth) {
                                BiomeMap[adjX][adjY] = isSnowBiome ? TEXT('I') : TEXT('B');
                            }
                        }
                    }
                }
            }
        }
    }
    for (int32 i = 0; i < EdgesMap.Num(); i++)
    {
        FString ArrayRowString;
        for (int32 j = 0; j < EdgesMap[i].Num(); j++)
        {
            ArrayRowString += EdgesMap[i][j] ? TEXT("T") : TEXT("F");
        }
        UE_LOG(LogDiamondSquare, Log, TEXT("Row %d: %s"), i, *ArrayRowString);
    }
    // Further post-processing can be done here, such as smoothing biome edges or adding rivers
    // ...
}



// ADiamondSquare::GetInterpolatedHeight
// -------------------------------------
// Purpose: 
//   Adjusts the height value of a vertex based on its biome type. This function uses
//   different adjustment methods for different biomes to create a more realistic and
//   varied terrain.
//
// Parameters:
//   - float heightValue: The original height value of the vertex.
//   - TCHAR biomeType: The character representing the biome type.
//
// Return:
//   - float: The adjusted height value after considering the biome type.
//
// Functionality:
//   - Uses a switch statement to apply different height adjustments based on biome type.
//   - Each biome type has a specific adjustment function that modifies the height to suit
//     the characteristics of that biome.
//   - For unrecognized biomes, it returns the original height value.
//
// Usage:
//   This function is called during the vertex creation process to refine the terrain height
//   based on the biome, contributing to the procedural generation of varied and realistic terrain.
float ADiamondSquare::GetInterpolatedHeight(float heightValue, TCHAR biomeType)
{
    switch (biomeType)
    {
    case 'O': // Ocean
        return AdjustForOcean(heightValue);
    case 'I': // Snow, Tundra
        return AdjustForSnow(heightValue);
    case 'K': // Snowy Forest
        return AdjustForSnowyForest(heightValue);
    case 'M': // Mountain
        return AdjustForMountain(heightValue);
    case 'P': // Plains
        return AdjustForPlains(heightValue);
    case 'B': // Beach
        return AdjustForBeach(heightValue);
    case 'D': // Desert
        return AdjustForDesert(heightValue);
    case 'R': // River
        return AdjustForRiver(heightValue);
    case 'T': // Taiga
        return AdjustForTaiga(heightValue);
    case 'W': // Woodlands
        return AdjustForWoodlands(heightValue);
    case 'H': // Highlands
        return AdjustForHighlands(heightValue);
    case 'F': // Regular Forest
        return AdjustForForest(heightValue);
    case 'S': // Steppe
        return AdjustForSteppe(heightValue);
    case 'J': // Jungle
        return AdjustForJungle(heightValue);
    case 'X': // Swamp
        return AdjustForSwamp(heightValue);
    case 'C': // Coniferous Forest
        return AdjustForConiferousForest(heightValue);
    case 'U': // Tundra
        return AdjustForTundra(heightValue);
    case 'V': // Savanna
        return AdjustForSavanna(heightValue);
    case 'L': // Rainforest
        return AdjustForRainforest(heightValue);
    default:
        // For unrecognized biomes, return the original height
        return heightValue;
    }
}


// ADiamondSquare::GetColorBasedOnBiomeAndHeight
// ---------------------------------------------
// Purpose: 
//   Determines the color of a vertex based on the biome type and its height (Z value).
//   This function assigns distinct colors to different biomes and adjusts shades to reflect
//   variations in height within each biome.
//
// Parameters:
//   - float Z: The height value of the vertex.
//   - TCHAR biomeType: The character representing the biome type.
//
// Return:
//   - FLinearColor: The color corresponding to the given biome and height.
//
// Functionality:
//   - Uses a switch statement to assign colors based on biome types like ocean, mountain,
//     forest, desert, etc.
//   - Adjusts color shades based on the height value to create more realistic terrain features.
//
// Usage:
//   This function is called for each vertex in the procedural mesh generation process to
//   apply appropriate colors based on the biome and elevation.
FLinearColor ADiamondSquare::GetColorBasedOnBiomeAndHeight(float Z, TCHAR biomeType)
{
    // Assign colors based on biome type. Adjust the shades if needed to reflect different heights within the biomes.
    switch (biomeType)
    {
    case 'O': // Ocean
        if (Z < 0.1f) Color = FLinearColor(0.05f, 0.19f, 0.57f); // Deep Water
        else Color = FLinearColor(0.28f, 0.46f, 0.80f); // Shallow Water
        break;
    case 'I': // Snow, Tundra
        Color = FLinearColor::White;
        break;
    case 'K': // Snowy Forest
        Color = FLinearColor(0.85f, 0.85f, 0.85f); // Lighter shade for snowy overlay on trees
        break;
    case 'M': // Mountain
        if (Z > 0.8f) Color = FLinearColor::White; // Snow capped peaks
        else Color = FLinearColor(0.50f, 0.50f, 0.50f); // Mountain Rock
        break;
    case 'P': // Plains
        Color = FLinearColor(0.24f, 0.70f, 0.44f); // Grass Green
        break;
    case 'B': //Beach
        Color = FLinearColor(0.82f, 0.66f, 0.42f);
        break;
    case 'D': // Desert
        Color = FLinearColor(0.82f, 0.66f, 0.42f); // Sand
        break;
    case 'R': // River
        Color = FLinearColor(0.50f, 0.73f, 0.93f); // Freshwater color
        break;
        // Add additional cases for new biome types here.
    case 'T': // Taiga
        Color = (Z > 0.5f) ? FLinearColor(0.52f, 0.37f, 0.26f) : FLinearColor(0.20f, 0.40f, 0.20f); // Darker Green for dense forestation
        break;
    case 'W': // Woodlands
        Color = FLinearColor(0.30f, 0.60f, 0.30f); // Green with a bit more yellow
        break;
    case 'H': // Highlands
        Color = FLinearColor(0.45f, 0.55f, 0.30f); // Dryer green with hints of brown
        break;
    case 'F': // Regular Forest
        Color = FLinearColor(0.13f, 0.55f, 0.13f); // Same color as before
        break;
    case 'S': // Steppe
        Color = FLinearColor(0.678f, 0.537f, 0.403f);
        break;
    case 'J': // Jungle
        Color = FLinearColor(0.25f, 0.50f, 0.20f);
        break;
    case 'X': // Swamp
        Color = FLinearColor(0.36f, 0.34f, 0.22f);
        break;
    case 'C': // Con Forest
        Color = FLinearColor(0.149f, 0.243f, 0.192f);
        break;
    case 'U': // Tundra
        Color = FLinearColor(0.462f, 0.564f, 0.678f); // Cold and sparse vegetation color
        break;
    case 'V': // Savanna
        Color = FLinearColor(0.85f, 0.87f, 0.45f); // Yellowish grass color
        break;
    case 'L': // Rainforest
        Color = FLinearColor(0.00f, 0.39f, 0.00f); // Deep green
        break;

    default:
        // If the biome type is unrecognized, use a default color (gray).
        Color = FLinearColor::Red;
        break;
    }

    return Color;
}


float ADiamondSquare::AdjustForOcean(float& heightValue) {
    return heightValue = FMath::Lerp(0.0f, 0.01f, 0.05f); // Flatten terrain for ocean regions
}

float ADiamondSquare::AdjustForMountain(float& heightValue) {
    return heightValue = FMath::Lerp(0.7f, 1.0f, heightValue); // Ensure mountains are within a certain range
}

float ADiamondSquare::AdjustForPlains(float& heightValue) {
    return heightValue = FMath::Lerp(0.1f, 0.3f, heightValue); // Slightly raise terrain for plains
}

float ADiamondSquare::AdjustForSnow(float& heightValue) {
    return heightValue = FMath::Lerp(0.1f, 0.3f, heightValue); // Keep snow regions at higher altitudes
}

float ADiamondSquare::AdjustForForest(float& heightValue) {
    return heightValue = FMath::Lerp(0.5f, 0.7f, heightValue); // Slightly raise terrain for forest regions
}

float ADiamondSquare::AdjustForTaiga(float& heightValue) {
    return heightValue = FMath::Lerp(0.5f, 0.7f, heightValue); // Moderate terrain height for taiga
}

float ADiamondSquare::AdjustForWoodlands(float& heightValue) {
    return heightValue = FMath::Lerp(0.5f, 0.7f, heightValue); // Variable terrain for woodlands
}

float ADiamondSquare::AdjustForSnowyMountain(float& heightValue) {
    return heightValue = FMath::Clamp(heightValue, 0.8f, 1.0f); // Ensure snowy mountains are tall
}

float ADiamondSquare::AdjustForHighlands(float& heightValue) {
    return heightValue = FMath::Lerp(0.5f, 0.7f, heightValue); // Higher than plains, lower than mountains
}

// Example of river adjustment (if needed)
float ADiamondSquare::AdjustForRiver(float& heightValue) {
    return heightValue *= 0.8f; // Lower terrain for river
}

// Additional examples (if other biomes are present)
float ADiamondSquare::AdjustForSnowyForest(float& heightValue) {
    return heightValue = FMath::Lerp(0.1f, 0.3f, heightValue); // Elevate for snowy forests
}

float ADiamondSquare::AdjustForOldForest(float& heightValue) {
    return heightValue = FMath::Lerp(0.1f, 0.3f, heightValue); // Adjust height for old forests
}


float ADiamondSquare::AdjustForBeach(float& heightValue) {
    // Logic to adjust terrain for beaches
    return heightValue = FMath::Lerp(0.02f, 0.1f, heightValue); // This is an example
}

float ADiamondSquare::AdjustForJungle(float& heightValue) {
    // Logic to adjust terrain for jungle regions
    return heightValue = FMath::Lerp(0.1f, 0.3f, heightValue); // This is an example
}

float ADiamondSquare::AdjustForConiferousForest(float& heightValue) {
    // Logic to adjust terrain for coniferous forests
    return heightValue = FMath::Lerp(0.1f, 0.3f, heightValue); // This is an example
}

float ADiamondSquare::AdjustForSwamp(float& heightValue) {
    // Logic to adjust terrain for swamps
    return heightValue = FMath::Lerp(0.1f, 0.3f, heightValue); // This is an example
}

float ADiamondSquare::AdjustForDesert(float& heightValue) {
    // Logic to adjust terrain for desert regions
    return heightValue = FMath::Lerp(0.1f, 0.3f, heightValue); // Slightly raise terrain for desert regions
}

float ADiamondSquare::AdjustForSavanna(float& heightValue) {
    // Logic to adjust terrain for savanna regions
    return heightValue = FMath::Lerp(0.5f, 0.7f, heightValue); // Ensure savanna is relatively flat but slightly elevated
}

float ADiamondSquare::AdjustForRainforest(float& heightValue) {
    // Logic to adjust terrain for rainforest regions
    return heightValue = FMath::Lerp(0.5f, 0.7f, heightValue); // Provide varied terrain for rainforest
}

float ADiamondSquare::AdjustForSteppe(float& heightValue) {
    // Logic to adjust terrain for steppe regions
    return heightValue = FMath::Lerp(0.1f, 0.3f, heightValue); // Ensure steppes have a moderate, variable elevation
}

float ADiamondSquare::AdjustForTundra(float& heightValue) {
    // Logic to adjust terrain for tundra regions
    return heightValue = FMath::Lerp(0.1f, 0.3f, heightValue); // Keep tundra flat with slight undulations
}

TArray<TArray<float>> ADiamondSquare::GetGaussianFilter(int32 fSize, float variance) {
    // Clamp variance to be greater than 0
    variance = std::max(variance, 0.0001f);

    TArray<TArray<float>> res;
    int32 halfSize = fSize / 2;
    float sum = 0;
    // Add each
    for (int32 i = -halfSize; i <= halfSize; ++i) {
        TArray<float> row;
        for (int32 j = -halfSize; j <= halfSize; ++j) {
            float val = exp(-1 * (i * i + j * j) / (2 * variance));
            sum += val;
            row.Add(val);
        }
        res.Add(row);
    }

    // Normalize the filter
    for (int32 i = 0; i < fSize; ++i) {
        for (int32 j = 0; j < fSize; ++j) {
            res[i][j] /= sum;
        }
    }

    return res;
}

TArray<TArray<float>> ADiamondSquare::Convolve(const TArray<TArray<float>>& map, const TArray<TArray<float>>& filter) {
    // Get Bounds
    int32 fSize = filter.Num();
    int32 halfFSize = fSize / 2;
    int32 rowStart = fSize / 2;
    int32 rowStop = map.Num() - rowStart;
    int32 colStart = rowStart;
    int32 colStop = map[0].Num() - colStart;

    // Make a copy of the map
    TArray<TArray<float>> res = map;

    // Convolve the map with the filter
    for (int32 row = rowStart; row < rowStop; ++row) {
        for (int32 col = colStart; col < colStop; ++col) {
            // Process a cell with the filter
            float sum = 0.0;
            for (int32 rowOff = -halfFSize; rowOff <= halfFSize; rowOff++) {
                for (int32 colOff = -halfFSize; colOff <= halfFSize; colOff++) {
                    int32 map_row = rowOff + row;
                    int32 map_col = colOff + col;
                    int32 f_row = rowOff + halfFSize;
                    int32 f_col = colOff + halfFSize;

                    sum += map[map_row][map_col] * filter[f_row][f_col];
                }
            }
            // Save convolution result on cell in temp array
            res[row][col] = sum;
        }
    }

    return res;
}


TArray<TArray<float>> ADiamondSquare::Convolve2(const TArray<TArray<float>>& map, const TArray<TArray<float>>& filterF, const TArray<TArray<float>>& filterT, const TArray<TArray<bool>>& boolMap) {
    // Get Bounds
    int32 fSize = filterF.Num();
    int32 halfFSize = fSize / 2;
    int32 rowStart = fSize / 2;
    int32 rowStop = map.Num() - rowStart;
    int32 colStart = rowStart;
    int32 colStop = map[0].Num() - colStart;

    // Make a copy of the map
    TArray<TArray<float>> res = map;

    // Convolve the map with the filters
    for (int32 row = rowStart; row < rowStop; ++row) {
        for (int32 col = colStart; col < colStop; ++col) {
            // Determine which filter to use based on the BoolMap
            const TArray<TArray<float>>& filter = boolMap[row][col] ? filterT : filterF;

            // Process a cell with the filter
            float sum = 0.0;
            for (int32 rowOff = -halfFSize; rowOff <= halfFSize; rowOff++) {
                for (int32 colOff = -halfFSize; colOff <= halfFSize; colOff++) {
                    int32 map_row = rowOff + row;
                    int32 map_col = colOff + col;
                    int32 f_row = rowOff + halfFSize;
                    int32 f_col = colOff + halfFSize;
                    // Check bounds before accessing arrays
                    if (map_row >= 0 && map_row < map.Num() && map_col >= 0 && map_col < map[0].Num() &&
                        f_row >= 0 && f_row < fSize && f_col >= 0 && f_col < fSize) {
                        sum += map[map_row][map_col] * filter[f_row][f_col];
                    }
                }
            }
            // Save convolution result on cell in temp array
            res[row][col] = sum;
        }
    }

    return res;
}

/*
void ADiamondSquare::Convolve2(TArray<TArray<float>>& Map, const TArray<TArray<float>>& FilterF, const TArray<TArray<float>>& FilterT, const TArray<TArray<bool>>& BoolMap)
{
    // Get Bounds
    int32 HalfFSize = FilterF.Num() / 2;

    // Make a copy of the map
    TArray<TArray<float>> Res = Map;

    // Convolve the map with the filter
    for (int32 Row = 0; Row < Map.Num(); ++Row) {
        for (int32 Col = 0; Col < Map[0].Num(); ++Col) {
            // Process a cell with the filter
            float Sum = 0.0f;
            const TArray<TArray<float>>& Filter = (BoolMap[Row][Col] ? FilterT : FilterF);
            for (int32 RowOff = -HalfFSize; RowOff <= HalfFSize; RowOff++) {
                for (int32 ColOff = -HalfFSize; ColOff <= HalfFSize; ColOff++) {
                    // Get coords of cell to look at
                    int32 MapRow = RowOff + Row;
                    int32 MapCol = ColOff + Col;

                    // Handle edge cases
                    if (MapRow < 0) MapRow = 0;
                    else if (MapRow >= Map.Num()) MapRow = Map.Num() - 1;
                    if (MapCol < 0) MapCol = 0;
                    else if (MapCol >= Map[0].Num()) MapCol = Map[0].Num() - 1;

                    // Filter coords
                    int32 FRow = RowOff + HalfFSize;
                    int32 FCol = ColOff + HalfFSize;

                    float MapValue = Map[MapRow][MapCol];
                    float FilterValue = Filter[FRow][FCol];

                    Sum += FilterValue * MapValue;
                }
            }
            // Save convolution result on cell in temp array
            Res[Row][Col] = Sum;
        }
    }
    Map = Res;
}



*/
/*void ADiamondSquare::PostProcessBiomeMap() {
    // Copy the original BiomeMap for reference during changes
    TArray<FString> OriginalBiomeMap = BiomeMap;

    // Define how wide the beach zone should be
    const int BeachWidth = 3;

    // Loop through each cell in the BiomeMap
    for (int y = 0; y < YSize; y++) {
        for (int x = 0; x < XSize; x++) {
            // Check if the current cell is ocean
            if (OriginalBiomeMap[y][x] == TEXT('O')) {
                // Check the surrounding area for land and add beaches if necessary
                for (int dy = -BeachWidth; dy <= BeachWidth; dy++) {
                    for (int dx = -BeachWidth; dx <= BeachWidth; dx++) {
                        int adjX = x + dx;
                        int adjY = y + dy;
                        // Skip the current cell and check bounds
                        if ((dx == 0 && dy == 0) || adjX < 0 || adjX >= XSize || adjY < 0 || adjY >= YSize) {
                            continue;
                        }

                        // If adjacent cell is land, check if it needs to be converted to beach
                        if (OriginalBiomeMap[adjY][adjX] != TEXT('O') && OriginalBiomeMap[adjY][adjX] != TEXT('B')) {
                            // Calculate distance from the ocean to this cell
                            int distanceFromOcean = FMath::Max(FMath::Abs(dx), FMath::Abs(dy));
                            // If the cell is within beach width range, convert it to beach
                            if (distanceFromOcean <= BeachWidth) {
                                BiomeMap[adjY][adjX] = TEXT('B'); // Beach
                            }
                        }
                    }
                }
            }
        }
    }

    // Further post-processing can be done here, such as smoothing biome edges or adding rivers
    // ...
}
*/



/*
TArray<FString> ADiamondSquare::CreateBiomeMap() {
    // Define the size of the map

    // Create a 2D array to store the biome data
    TArray<FString> biomeMap;
    for (int32 i = 0; i < YSize; ++i) {
        biomeMap.Add(FString());
    }

    // Fill the map with biomes, dividing it into quarters
    for (int32 y = 0; y < YSize; ++y) {
        for (int32 x = 0; x < XSize; ++x) {
            if (x < XSize / 2 && y < YSize / 2) {
                biomeMap[y].AppendChar('O'); // Top-left quarter
            }
            else if (x >= XSize / 2 && y < YSize / 2) {
                biomeMap[y].AppendChar('I'); // Top-right quarter
            }
            else if (x < XSize / 2 && y >= YSize / 2) {
                biomeMap[y].AppendChar('|'); // Bottom-left quarter
            }
            else {
                biomeMap[y].AppendChar('+'); // Bottom-right quarter
            }
        }
    }
    // Debug print the entire biome map
    for (int32 i = 0; i < biomeMap.Num(); ++i) {
        UE_LOG(LogDiamondSquare, Warning, TEXT("%s"), *biomeMap[i]);
    }
    return biomeMap;
}

*/




/*
TArray<TArray<float>> ADiamondSquare::GeneratePerlinNoiseMap()
{
    TArray<TArray<float>> NoiseMap;
    NoiseMap.Init(TArray<float>(), XSize + 1);
    for (int X = 0; X <= XSize; ++X)
    {
        NoiseMap[X].Init(0.0f, YSize + 1);
        for (int Y = 0; Y <= YSize; ++Y)
        {
            float Amplitude = 1.0f;
            float Frequency = 1.0f;
            float NoiseHeight = 0.0f;
            for (int Octave = 0; Octave < Octaves; ++Octave)
            {
                float SampleX = X / Scale * Frequency;
                float SampleY = Y / Scale * Frequency;

                float PerlinValue = FMath::PerlinNoise2D(FVector2D(SampleX, SampleY));
                NoiseHeight += PerlinValue * Amplitude;
                NoiseHeight = FMath::Max(0, NoiseHeight);

                Amplitude *= Persistence;
                Frequency *= Lacunarity;
            }

            // Use the biome map to adjust noise height
            switch (BiomeMap[Y][X])
            {
            case 'O':
                AdjustForOcean(NoiseHeight);
                break;
            case 'I':
                AdjustForIsland(NoiseHeight);
                break;
            case '|':
                AdjustForRiver(NoiseHeight);
                break;
            case '+':
                AdjustForMountain(NoiseHeight);
                break;
            case '%':
                AdjustForPlains(NoiseHeight);
                break;
            }

            NoiseMap[X][Y] = NoiseHeight;
        }
    }
    return NoiseMap;
}
*/


/*
TArray<TArray<float>> ADiamondSquare::GeneratePerlinNoiseMap()
{
    TArray<TArray<float>> NoiseMap;
    NoiseMap.Init(TArray<float>(), XSize + 1);
    for (int X = 0; X <= XSize; ++X)
    {
        NoiseMap[X].Init(0.0f, YSize + 1);
        for (int Y = 0; Y <= YSize; ++Y)
        {
            float Amplitude = 1.0f;
            float Frequency = 1.0f;
            float NoiseHeight = 0.0f;
            for (int Octave = 0; Octave < Octaves; ++Octave)
            {
                float SampleX = X / Scale * Frequency;
                float SampleY = Y / Scale * Frequency;

                float PerlinValue = FMath::PerlinNoise2D(FVector2D(SampleX, SampleY));
                NoiseHeight += PerlinValue * Amplitude;
                NoiseHeight = FMath::Max(0,NoiseHeight);

                Amplitude *= Persistence;
                Frequency *= Lacunarity;
            }

            NoiseMap[X][Y] = NoiseHeight;
        }
    }
    return NoiseMap;
}
*/



/*
void ADiamondSquare::CreateTriangles()
{
    int Vertex = 0;

    for (int X = 0; X < XSize; ++X)
    {
        for (int Y = 0; Y < YSize; ++Y)
        {
            Triangles.Add(Vertex);//Bottom left corner
            Triangles.Add(Vertex + 1);//Bottom right corner
            Triangles.Add(Vertex + YSize + 1);//Top left corner
            Triangles.Add(Vertex + 1);//Bottom right corner
            Triangles.Add(Vertex + YSize + 2);//Top right corner
            Triangles.Add(Vertex + YSize + 1);//Top left corner

            ++Vertex;
        }
        ++Vertex;
    }
}

*/







/*

TArray<FString> ADiamondSquare::CreateBiomeMap() {
    BiomeMap.Empty();

    // Use these constants to scale the noise for altitude and latitude.
    const float AltitudeScale = 0.05f;
    const float LatitudeScale = 0.1f;

    for (int y = 0; y < YSize; y++) {
        FString row;

        // Use y-coordinate as a proxy for latitude.
        float latitudeFactor = FMath::Sin(PI * y / YSize);

        for (int x = 0; x < XSize; x++) {
            // Get altitude (height) using Perlin noise.
            float altitudeNoise = FMath::PerlinNoise2D(FVector2D(x * AltitudeScale, y * AltitudeScale));

            // Consider latitude when determining biome.
            float latitudeNoise = FMath::PerlinNoise2D(FVector2D(x * LatitudeScale, y * LatitudeScale)) + latitudeFactor;

            // Biome determination.
            if (altitudeNoise < 0.2) {
                if (latitudeNoise < 0.2 || latitudeNoise > 0.8) {
                    row += TEXT("S"); // Snow (Tundra at low altitudes and northern/southern latitudes)
                }
                else {
                    row += TEXT("O"); // Ocean or lowlands
                }
            }
            else if (altitudeNoise < 0.6) {
                if (latitudeNoise < 0.2) {
                    row += TEXT("%"); // Snowy Forest
                }
                else if (latitudeNoise < 0.6) {
                    row += TEXT("|"); // Forest
                }
                else {
                    row += TEXT("+"); //Mountain
                }
            }
            else {
                row += TEXT("+"); // Mountain
            }
        }

        BiomeMap.Add(row);
    }

    // Post-process to ensure mountains and oceans are not adjacent.
    for (int y = 0; y < YSize; y++) {
        for (int x = 0; x < XSize; x++) {
            if (BiomeMap[y][x] == TEXT('+')) { // Mountain detected
                // Check all adjacent tiles.
                for (int i = -1; i <= 1; i++) {
                    for (int j = -1; j <= 1; j++) {
                        if (i == 0 && j == 0) continue; // Skip the mountain tile itself
                        int adjX = x + i;
                        int adjY = y + j;
                        // Check bounds and adjacent ocean tiles
                        if (adjX >= 0 && adjX < XSize && adjY >= 0 && adjY < YSize &&
                            BiomeMap[adjY][adjX] == TEXT('O')) {
                            // Adjust the adjacent ocean to another biome, e.g., forest "|"
                            BiomeMap[adjY].GetCharArray()[adjX] = TEXT('|');
                        }
                    }
                }
            }
        }
    }

    for (const FString& row : BiomeMap) {
        UE_LOG(LogDiamondSquare, Log, TEXT("%s"), *row);
    }

    return BiomeMap;
}


*/



/*TArray<TArray<float>> ADiamondSquare::GeneratePerlinNoiseMap()
{
    BiomeMap.Empty();
    BiomeMap = CreateBiomeMap();
    if (BiomeMap.Num() == 0 || BiomeMap[0].Len() == 0)
    {
        // Handle the case where the BiomeMap is empty or malformed.
        UE_LOG(LogTemp, Warning, TEXT("BiomeMap is empty or not correctly formed!"));
        return TArray<TArray<float>>();
    }

    float Amplitude = 1.0f;
    float Frequency = 1.0f;
    float NoiseHeight = 0.0f;
    TArray<TArray<float>> NoiseMap;
    NoiseMap.Init(TArray<float>(), XSize);

    for (int X = 0; X < XSize; ++X)
    {
        NoiseMap[X].Init(0.0f, YSize);
        for (int Y = 0; Y < YSize; ++Y)
        {

            Amplitude = 1.0f;
            Frequency = 1.0f;
            NoiseHeight = 0.0f;
            for (int Octave = 0; Octave < Octaves; ++Octave)
            {
                float SampleX = X / Scale * Frequency;
                float SampleY = Y / Scale * Frequency;

                float PerlinValue = FMath::PerlinNoise2D(FVector2D(SampleX, SampleY));
                NoiseHeight += PerlinValue * Amplitude;
                NoiseHeight = FMath::Max(0, NoiseHeight);

                Amplitude *= Persistence;
                Frequency *= Lacunarity;
            }
            // Use the biome map to adjust noise height
            //TCHAR BiomeChar = BiomeMap[X][Y];
            switch (BiomeMap[Y][X])
            {
            case 'O':
                NoiseHeight = AdjustForOcean(NoiseHeight);
                break;
            case 'S':
                NoiseHeight = AdjustForSand(NoiseHeight);
                break;
            case '%':
                NoiseHeight = AdjustForRiver(NoiseHeight);
                break;
            case '+':
                NoiseHeight = AdjustForMountain(NoiseHeight);
                break;
            case '|':
                NoiseHeight = AdjustForPlains(NoiseHeight);
                break;
            }
            //NoiseHeight = FMath::Clamp(NoiseHeight,0.0f,1.0f);
            NoiseMap[X][Y] = GetInterpolatedHeight(NoiseHeight, BiomeMap[Y][X]);
        }
    }
    /*
    for (int X = 0; X < NoiseMap.Num(); ++X)
    {
        FString RowData = FString::Printf(TEXT("Row %d: "), X);
        for (int Y = 0; Y < NoiseMap[X].Num(); ++Y)
        {
            RowData += FString::Printf(TEXT("%f "), NoiseMap[X][Y]);
        }
        UE_LOG(LogTemp, Warning, TEXT("%s"), *RowData);
    }
return NoiseMap;
}*/