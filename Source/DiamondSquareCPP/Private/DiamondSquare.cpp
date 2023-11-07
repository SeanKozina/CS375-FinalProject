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


void ADiamondSquare::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    if (recreateMesh) {
        auto NoiseMap = GeneratePerlinNoiseMap();

        Vertices.Reset();
        Triangles.Reset();
        UV0.Reset();

        CreateVertices(NoiseMap);
        CreateTriangles();

        UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UV0, Normals, Tangents);

        ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UV0, Colors, Tangents, true);
        ProceduralMesh->SetMaterial(0, Material);
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
    Colors.Empty();
    if (ProceduralMesh)
    {
        for (int X = 0; X < XSize; ++X)
        {
            for (int Y = 0; Y < YSize; ++Y)
            {
                float Z = NoiseMap[X][Y];
                TCHAR BiomeChar = BiomeMap[Y][X];
                //UE_LOG(LogDiamondSquare, Log, TEXT("Value of Z at [%d][%d]: %f"), X, Y, Z);

                Color = GetColorBasedOnBiomeAndHeight(Z, BiomeChar);

                Colors.Add(Color.ToFColor(false));
                //UE_LOG(LogTemp, Warning, TEXT("R: %f, G: %f, B: %f, A: %f"), Color.R, Color.G, Color.B, Color.A);

                Vertices.Add(FVector(X * Scale, Y * Scale, Z * ZMultiplier * Scale));
                UV0.Add(FVector2D(X * UVScale, Y * UVScale));
            }
        }
        Normals.Init(FVector(0.0f, 0.0f, 1.0f), Vertices.Num());
        Tangents.Init(FProcMeshTangent(1.0f, 0.0f, 0.0f), Vertices.Num());
        ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UV0, Colors, Tangents, true);
    }
    // Assume the existence of the arrays 'Colors' and 'Vertices'
    // Assume xsize and ysize are provided as the dimensions of a 2D grid

    // Log the size of the Colors array
    UE_LOG(LogTemp, Warning, TEXT("Size of Colors array: %d"), Colors.Num());

    // Log the size of the Vertices array
    UE_LOG(LogTemp, Warning, TEXT("Size of Vertices array: %d"), Vertices.Num());
    if (Colors.Num() != Vertices.Num()) {
        UE_LOG(LogTemp, Error, TEXT("Error: Colors array and Vertices array are not the same size."));
    }
}





TArray<TArray<float>> ADiamondSquare::GeneratePerlinNoiseMap()
{
    BiomeMap.Empty();
    BiomeMap = CreateBiomeMap();
    if (BiomeMap.Num() == 0 || BiomeMap[0].Len() == 0)
    {
        // Handle the case where the BiomeMap is empty or malformed.
        UE_LOG(LogTemp, Warning, TEXT("BiomeMap is empty or not correctly formed!"));
        return TArray<TArray<float>>();
    }

    TArray<TArray<float>> NoiseMap;
    NoiseMap.Init(TArray<float>(), XSize);

    for (int X = 0; X < XSize; ++X)
    {
        NoiseMap[X].Init(0.0f, YSize);
        for (int Y = 0; Y < YSize; ++Y)
        {
            float Amplitude = 1.0f;
            float Frequency = 1.0f;
            float NoiseHeight = 0.0f;
            for (int Octave = 0; Octave < Octaves; ++Octave)
            {
                float SampleX = X / Scale * Frequency;
                float SampleY = Y / Scale * Frequency;

                float PerlinValue = FMath::PerlinNoise2D(FVector2D(SampleX, SampleY)); // Adjust to range [-1, 1]
                NoiseHeight += PerlinValue * Amplitude;

                Amplitude *= Persistence;
                Frequency *= Lacunarity;
            }

            // Use the biome map to adjust noise height
            TCHAR BiomeChar = BiomeMap[Y][X];
            NoiseHeight = GetInterpolatedHeight(NoiseHeight, BiomeChar); // Adjust height based on biome

            NoiseMap[X][Y] = FMath::Clamp(NoiseHeight, 0.0f, 1.0f); // Clamp final value just in case
        }
    }

    return NoiseMap;
}





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
    case '%': // Snowy Forest
        Color = FLinearColor(0.85f, 0.85f, 0.85f); // Lighter shade for snowy overlay on trees
        break;
    case '+': // Mountain
        if (Z > 0.8f) Color = FLinearColor::White; // Snow capped peaks
        else Color = FLinearColor(0.50f, 0.50f, 0.50f); // Mountain Rock
        break;
    case '|': // Forest
        Color = FLinearColor(0.13f, 0.55f, 0.13f); // Forest Green
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
        Color = FLinearColor(0.75f, 0.75f, 0.50f); // Dry grass color
        break;
    case 'M': // Snowy Mountains
        Color = FLinearColor::White; // Snow capped at any Z
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

float ADiamondSquare::AdjustForSand(float& heightValue) {
    return heightValue = FMath::Lerp(0.15f, 0.2f, 0.4f); // Gently raise terrain for sandy regions
}

float ADiamondSquare::AdjustForMountain(float& heightValue) {
    return heightValue = FMath::Clamp(heightValue, 1.0f, 0.5f); // Ensure mountains are within a certain range
}

float ADiamondSquare::AdjustForPlains(float& heightValue) {
    return heightValue = FMath::Lerp(0.15f, 0.25f, 0.6f); // Slightly raise terrain for plains
}

float ADiamondSquare::AdjustForSnow(float& heightValue) {
    return heightValue = FMath::Lerp(heightValue, 0.85f, 0.2f); // Keep snow regions at higher altitudes
}

float ADiamondSquare::AdjustForForest(float& heightValue) {
    return heightValue = FMath::Lerp(0.3f, 0.45f, heightValue); // Slightly raise terrain for forest regions
}

float ADiamondSquare::AdjustForTaiga(float& heightValue) {
    return heightValue = FMath::Lerp(heightValue, 0.3f, 0.25f); // Moderate terrain height for taiga
}

float ADiamondSquare::AdjustForWoodlands(float& heightValue) {
    return heightValue = FMath::Lerp(0.32f, 0.55f, heightValue); // Variable terrain for woodlands
}

float ADiamondSquare::AdjustForSnowyMountain(float& heightValue) {
    return heightValue = FMath::Clamp(heightValue, 0.6f, 1.0f); // Ensure snowy mountains are tall
}

float ADiamondSquare::AdjustForHighlands(float& heightValue) {
    return heightValue = FMath::Lerp(0.4f, 0.7f, heightValue); // Higher than plains, lower than mountains
}

// Example of river adjustment (if needed)
float ADiamondSquare::AdjustForRiver(float& heightValue) {
    return heightValue *= 0.8f; // Lower terrain for river
}

// Additional examples (if other biomes are present)
float ADiamondSquare::AdjustForSnowyForest(float& heightValue) {
    return heightValue = FMath::Lerp(0.3f, 0.45f, heightValue); // Elevate for snowy forests
}

float ADiamondSquare::AdjustForOldForest(float& heightValue) {
    return heightValue = FMath::Lerp(0.3f, 0.45f, heightValue); // Adjust height for old forests
}


float ADiamondSquare::AdjustForBeach(float& heightValue) {
    // Logic to adjust terrain for beaches
    return heightValue = FMath::Lerp(0.05f, 0.33f, heightValue); // This is an example
}

float ADiamondSquare::AdjustForJungle(float& heightValue) {
    // Logic to adjust terrain for jungle regions
    return heightValue = FMath::Lerp(0.3f, 0.45f, heightValue); // This is an example
}

float ADiamondSquare::AdjustForConiferousForest(float& heightValue) {
    // Logic to adjust terrain for coniferous forests
    return heightValue = FMath::Lerp(0.3f, 0.45f, heightValue); // This is an example
}

float ADiamondSquare::AdjustForSwamp(float& heightValue) {
    // Logic to adjust terrain for swamps
    return heightValue = FMath::Lerp(0.3f, 0.4f, heightValue); // This is an example
}



TArray<FString> ADiamondSquare::CreateBiomeMap() {
    BiomeMap.Empty();

    // Constants to scale the noise functions for different parameters.
    const float AltitudeScale = 0.03f;
    const float TemperatureScale = 0.1f;
    const float HumidityScale = 0.1f;

    // Generate noise maps for temperature and humidity
    TArray<float> TemperatureMap;
    TArray<float> HumidityMap;
    for (int y = 0; y < YSize; y++) {
        for (int x = 0; x < XSize; x++) {
            // Calculate noise values for temperature and humidity.
            float temperatureNoise = FMath::PerlinNoise2D(FVector2D(x * TemperatureScale, y * TemperatureScale));
            float humidityNoise = FMath::PerlinNoise2D(FVector2D(x * HumidityScale, y * HumidityScale));

            // Store temperature and humidity values.
            TemperatureMap.Add(temperatureNoise);
            HumidityMap.Add(humidityNoise);
        }
    }

    for (int y = 0; y < YSize; y++) {
        FString row;

        for (int x = 0; x < XSize; x++) {
            // Get altitude using Perlin noise.
            float altitudeNoise = FMath::PerlinNoise2D(FVector2D(x * AltitudeScale, y * AltitudeScale));

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



// Biome determination 
FString ADiamondSquare::DetermineBiome(float altitude, float temperature, float humidity) {
    // Normalize temperature and humidity between 0 and 1 if not already.
    temperature = FMath::Clamp((temperature + 1) / 2, 0.0f, 1.0f);
    humidity = FMath::Clamp((humidity + 1) / 2, 0.0f, 1.0f);

    // Altitude thresholds for different terrain types.
    const float SeaLevel = 0.3f;
    const float PlainsLevel = 0.4f;
    const float MountainLevel = 0.6f;

    // Temperature thresholds for different climates.
    const float SnowTemperature = 0.2f;
    const float DesertTemperature = 0.8f;

    // Humidity thresholds for determining dry/wet biomes.
    const float DryHumidity = 0.3f;
    const float WetHumidity = 0.7f;

    // Determine biomes based on altitude, temperature, and humidity.
    if (altitude < SeaLevel) {
        if (temperature > DesertTemperature && humidity < DryHumidity) {
            return TEXT("B"); // Beach
        }
        else {
            return TEXT("O"); // Ocean
        }
    }
    else if (altitude < PlainsLevel) {
        if (temperature < SnowTemperature) {
            return TEXT("I"); // Snow
        }
        else if (temperature > DesertTemperature) {
            return TEXT("D"); // Desert
        }
        else if (humidity < DryHumidity) {
            return TEXT("P"); // Plains
        }
        else if (humidity > WetHumidity) {
            return TEXT("J"); // Jungle
        }
        else {
            return TEXT("F"); // Forest
        }
    }
    else if (altitude < MountainLevel) {
        if (humidity < DryHumidity) {
            if (temperature < SnowTemperature) {
                return TEXT("T"); // Taiga
            }
            else {
                return TEXT("S"); // Steppe
            }
        }
        else if (humidity > WetHumidity) {
            return TEXT("X"); // Swamp
        }
        else {
            return TEXT("W"); // Woodlands
        }
    }
    else {
        if (temperature < SnowTemperature) {
            return TEXT("M"); // Snowy Mountains
        }
        else if (humidity < DryHumidity) {
            return TEXT("C"); // Coniferous Forest
        }
        else {
            return TEXT("H"); // Highlands
        }
    }
}



void ADiamondSquare::PostProcessBiomeMap() {
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




float ADiamondSquare::GetInterpolatedHeight(float heightValue, TCHAR biomeType)
{
    // Biomes: Add or revise as necessary for the complete set of biomes in the simulation.
    // O: Ocean
    // I: Ice/Snow or Tundra
    // D: Desert
    // P: Plains
    // F: Forest
    // T: Taiga
    // W: Woodlands
    // M: Snowy Mountains
    // H: Highlands
    // '%': Snowy Forest
    // '|': Forest
    // '+': Mountain
    // Additional biomes not covered by the previous code:
    // R: River
    // B: Beach
    // J: Jungle
    // C: Coniferous Forest (if different from 'F' and '|')
    // X: Swamp

    switch (biomeType)
    {
    case 'O': // Ocean
        return FMath::Lerp(heightValue, AdjustForOcean(heightValue), 0.1f);
    case 'I': // Ice/Snow or Tundra
        return FMath::Lerp(heightValue, AdjustForSnow(heightValue), 0.1f);
    case 'D': // Desert
        return FMath::Lerp(heightValue, AdjustForSand(heightValue), 0.5f);
    case 'P': // Plains
        return FMath::Lerp(heightValue, AdjustForPlains(heightValue), 0.1f);
    case 'F': // Forest
        return FMath::Lerp(heightValue, AdjustForForest(heightValue), 0.1f);
    case 'T': // Taiga
        return FMath::Lerp(heightValue, AdjustForTaiga(heightValue), 0.1f);
    case 'W': // Woodlands
        return FMath::Lerp(heightValue, AdjustForWoodlands(heightValue), 0.1f);
    case 'M': // Snowy Mountains
        return FMath::Lerp(heightValue, AdjustForSnowyMountain(heightValue), 0.1f);
    case 'H': // Highlands
        return FMath::Lerp(heightValue, AdjustForHighlands(heightValue), 0.1f);
    case 'R': // River
        return FMath::Lerp(heightValue, AdjustForRiver(heightValue), 0.1f);
    case 'B': // Beach
        return FMath::Lerp(heightValue, AdjustForBeach(heightValue), 0.2f);
    case 'J': // Jungle
        return FMath::Lerp(heightValue, AdjustForJungle(heightValue), 0.1f);
    case 'C': // Coniferous Forest
        return FMath::Lerp(heightValue, AdjustForConiferousForest(heightValue), 0.1f);
    case 'X': // Swamp
        return FMath::Lerp(heightValue, AdjustForSwamp(heightValue), 0.2f);
    case '%': // Snowy Forest (from old code)
        return FMath::Lerp(heightValue, AdjustForSnowyForest(heightValue), 0.5f);
    case '|': // Forest (from old code)
        return FMath::Lerp(heightValue, AdjustForOldForest(heightValue), 0.1f);
    case '+': // Mountain
        return FMath::Lerp(heightValue, AdjustForMountain(heightValue), 0.1f);
    default:
        // For unrecognized biomes, we could return the original height, 
        // or handle them appropriately if they have special requirements.
        return heightValue;
    }
}





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