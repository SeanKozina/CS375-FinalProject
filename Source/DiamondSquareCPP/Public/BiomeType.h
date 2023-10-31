// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BiomeType.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class EBiomeEnum : uint8
{
    Mountains UMETA(DisplayName = "Mountains"),
    Plains UMETA(DisplayName = "Plains"),
    Forest UMETA(DisplayName = "Forest"),
    Desert UMETA(DisplayName = "Desert"),
    //... Add other biomes as needed
};

UCLASS(Blueprintable)
class DIAMONDSQUARECPP_API UBiomeType : public UObject
{
    GENERATED_BODY()

public:
    UBiomeType();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
    EBiomeEnum Biome;

    // Additional properties for the biome, such as moisture levels, fauna, flora, etc.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
    float AverageTemperature;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
    float AverageRainfall;

    // Virtual function for any biome-specific behavior/logic
    virtual void HandleBiomeSpecificBehavior();
};


UCLASS(Blueprintable)
class DIAMONDSQUARECPP_API UDesertBiome : public UBiomeType
{
    GENERATED_BODY()

public:
    UDesertBiome();

    // Desert specific properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Desert")
    float SandDuneHeight;

    virtual void HandleBiomeSpecificBehavior() override;
};
