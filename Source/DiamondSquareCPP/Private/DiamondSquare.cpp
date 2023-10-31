#include "DiamondSquare.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"

void AdjustForOcean(float& heightValue);
void AdjustForPlains(float& heightValue);
void AdjustForMountain(float& heightValue);
void AdjustForRiver(float& heightValue);
void AdjustForIsland(float& heightValue);

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
    for (int X = 0; X <= XSize; ++X)
    {
        for (int Y = 0; Y <= YSize; ++Y)
        {
            float Z = NoiseMap[X][Y];
            FLinearColor Color;
            if (Z >= 0.9f && Z <= 5000.0f)
            {
                Color = FLinearColor::Black;
            }
            else if (Z >= 0.7f && Z < 0.9f)
            {
                Color = FLinearColor::LerpUsingHSV(FLinearColor(1.0f, 1.0f, 0.5f), FLinearColor::White, (Z - 0.7f) / 0.2f);
            }
            else if (Z >= 0.5f && Z < 0.7f)
            {
                Color = FLinearColor::LerpUsingHSV(FLinearColor(0.5f, 0.5f, 0.5f), FLinearColor(1.0f, 1.0f, 0.5f), (Z - 0.5f) / 0.2f);
            }
            else if (Z >= 0.0f && Z < 0.5f)
            {
                Color = FLinearColor::LerpUsingHSV(FLinearColor::Black, FLinearColor(0.5f, 0.5f, 0.5f), Z / 0.5f);
            }
            Colors.Add(Color.ToFColor(false));
            Vertices.Add(FVector(X * Scale, Y * Scale, Z * ZMultiplier));
            UV0.Add(FVector2D(X * UVScale, Y * UVScale));
        }
    }
    Normals.Init(FVector(0.0f, 0.0f, 1.0f), Vertices.Num());
    Tangents.Init(FProcMeshTangent(1.0f, 0.0f, 0.0f), Vertices.Num());
    ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UV0, Colors, Tangents, true);
}



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



TArray<TArray<float>> ADiamondSquare::GeneratePerlinNoiseMap()
{
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

                float PerlinValue = FMath::PerlinNoise2D(FVector2D(SampleX, SampleY));
                NoiseHeight += PerlinValue * Amplitude;
                NoiseHeight = FMath::Max(0, NoiseHeight);

                Amplitude *= Persistence;
                Frequency *= Lacunarity;
            }

            // Use the biome map to adjust noise height
            TCHAR BiomeChar = BiomeMap[Y][X];
            switch (BiomeChar)
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
void AdjustForOcean(float& heightValue)
{
    heightValue *= 0.5f; // Lower the terrain for oceanic regions
}

void AdjustForIsland(float& heightValue)
{
    heightValue *= 1.5f; // Increase the terrain height for island regions
}

void AdjustForRiver(float& heightValue)
{
    heightValue *= 0.8f; // Lower the terrain slightly for rivers
}

void AdjustForMountain(float& heightValue)
{
    heightValue *= 2.0f; // Double the terrain height for mountains
}

void AdjustForPlains(float& heightValue)
{
    heightValue *= 1.2f; // Slightly increase the terrain height for plains
}




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




TArray<FString> ADiamondSquare::CreateBiomeMap() {
    // Define some simple proportions for each biome based on ySize.
    int32 ForestEnd = YSize * 0.3;
    int32 SnowEnd = YSize * 0.6;
    int32 DesertEnd = YSize * 0.9;

    for (int32 y = 0; y < YSize; y++) {
        FString row;

        for (int32 x = 0; x < XSize; x++) {
            if (y < ForestEnd) {
                row += TEXT("% ");
            }
            else if (y < SnowEnd) {
                row += TEXT("I ");
            }
            else if (y < DesertEnd) {
                row += TEXT("O ");
            }
            else {
                // Adding the river and mountain based on the x-coordinate.
                if (x == XSize / 2) {
                    row += TEXT("| ");
                }
                else if (x > XSize * 0.75) {
                    row += TEXT("+ ");
                }
                else {
                    row += TEXT("I ");
                }
            }
        }

        BiomeMap.Add(row);
    }

    return BiomeMap;
}
