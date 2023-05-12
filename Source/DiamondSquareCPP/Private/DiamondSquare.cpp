#include "DiamondSquare.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"

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
/*
void ADiamondSquare::CreateVertices(const TArray<TArray<float>>& NoiseMap)
{
	for (int X = 0; X <= XSize; ++X)
	{
		for (int Y = 0; Y <= YSize; ++Y)
		{
			float Z = NoiseMap[X][Y];
			Vertices.Add(FVector(X * Scale, Y * Scale, Z));
			UV0.Add(FVector2D(X * UVScale, Y * UVScale));
		}
	}
}

*/





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

                Amplitude *= Persistence;
                Frequency *= Lacunarity;
            }

            NoiseMap[X][Y] = NoiseHeight * ZMultiplier;
        }
    }
    return NoiseMap;
}
