#include "DiamondSquare.h"
#include "Engine/World.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDiamondSquare, Log, All);
DEFINE_LOG_CATEGORY(LogDiamondSquare);

float AdjustForOcean(float& heightValue);
float AdjustForPlains(float& heightValue);
float AdjustForMountain(float& heightValue);
float AdjustForRiver(float& heightValue);
float AdjustForIce(float& heightValue);

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

        ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UV0, TArray<FColor>(), Tangents, true);
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


void ADiamondSquare::CreateVertices(const TArray<TArray<float>>& NoiseMap)
{

    if (ProceduralMesh)
    {

        for (int X = 0; X < XSize; ++X)
        {
            for (int Y = 0; Y < YSize; ++Y)
            {
                float Z = NoiseMap[X][Y];
                UE_LOG(LogDiamondSquare, Log, TEXT("Value of Z at [%d][%d]: %f"), X, Y, Z);
                FLinearColor Color;

                if (Z >= 0.9f)
                {
                    Color = FLinearColor::Blue;        
                }
                else if (Z >= 0.7f && Z < 0.9f)
                {
                    Color = FLinearColor::Blue;
                }
                else if (Z >= 0.5f && Z < 0.7f)
                {
                    Color = FLinearColor::Blue;
                }
                else if (Z >= 0.0f && Z < 0.5f)
                {
                    Color = FLinearColor::Blue;
                }
                Colors.Add(Color.ToFColor(false));
                UE_LOG(LogTemp, Warning, TEXT("R: %f, G: %f, B: %f, A: %f"), Color.R, Color.G, Color.B, Color.A);

                Vertices.Add(FVector(X * Scale, Y * Scale, Z * ZMultiplier));
                UV0.Add(FVector2D(X * UVScale, Y * UVScale));
            }
        }
        Normals.Init(FVector(0.0f, 0.0f, 1.0f), Vertices.Num());
        Tangents.Init(FProcMeshTangent(1.0f, 0.0f, 0.0f), Vertices.Num());
        ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UV0, Colors, Tangents, true);
    }
}



void ADiamondSquare::CreateTriangles()
{
    int Vertex = 0;
    for (int X = 0; X < XSize - 1; ++X)
    {
        for (int Y = 0; Y < YSize - 1; ++Y)
        {
            //int Vertex = X * YSize + Y;

            // First triangle (swapping the winding order)
            Triangles.Add(Vertex); // Bottom left corner
            Triangles.Add(Vertex + 1); // Bottom right corner
            Triangles.Add(Vertex + YSize + 1); // Top left corner

            // Second triangle (swapping the winding order)
            Triangles.Add(Vertex + 1); // Bottom right corner
            Triangles.Add(Vertex + YSize + 2); // Top right corner
            Triangles.Add(Vertex + YSize + 1); // Top left corner
            Vertex++;
        }
        Vertex++;
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
            case 'I':
                NoiseHeight = AdjustForIce(NoiseHeight);
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
            NoiseMap[X][Y] = NoiseHeight;
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
    */
    return NoiseMap;
}


float AdjustForOcean(float& heightValue)
{
    return heightValue = FMath::Lerp(0.0f,0.2f,0.1f); // Lower the terrain for oceanic regions
}

float AdjustForIce(float& heightValue)
{
    return heightValue = FMath::Lerp(0.2f, 0.3f, heightValue);
}

float AdjustForRiver(float& heightValue)
{
    //heightValue *= 0.8f; // Lower the terrain slightly for rivers
    return 0.0f;
}

float AdjustForMountain(float& heightValue)
{
    heightValue = FMath::Lerp(0.4f, 1.0f,heightValue);
    return heightValue; // Double the terrain height for mountains
}

float AdjustForPlains(float& heightValue)
{
    heightValue = FMath::Lerp(0.2f, 0.4f, heightValue);
    return heightValue; // Slightly increase the terrain height for plains
}



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
                    row += TEXT("I"); // Snow (Tundra at low altitudes and northern/southern latitudes)
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
                    row += TEXT("+"); // Desert
                }
            }
            else {
                row += TEXT("+"); // Mountain
            }
        }

        BiomeMap.Add(row);
    }
    for (const FString& row : BiomeMap) {
        UE_LOG(LogDiamondSquare, Log, TEXT("%s"), *row);
    }

    return BiomeMap;
}


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
