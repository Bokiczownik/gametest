#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MapFogVisualComponent.generated.h"

class UMaterialParameterCollection;

/** How the fog looks at one fog level. */
USTRUCT(BlueprintType)
struct NOWAPROBA_API FFogLevelLook
{
	GENERATED_BODY()

	/** Mist density multiplier (0..1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog", meta = (ClampMin = "0", ClampMax = "1"))
	float Opacity = 0.5f;

	/** Height above BaseZ (cm) the ground mist reaches in deep fog (halved at the safe circle's edge); mist layers end at 45 cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog", meta = (ClampMin = "1"))
	float MaxHeight = 50.f;
};

/**
 * Ground mist look for BP_MapFog. The owner's Blueprint calls SetFogState every tick with the safe circle's radius
 * and the current fog level; the owner's location is the circle centre. Writes everything to MPC_MapFog
 * (read by the MistLayer planes via MI_MapGroundMist and by the camera post-process; the grass material reads GrassCollection),
 * blends between level looks, fades the "player in fog" camera effect, and from GrassHideLevel on hides grass areas lying
 * completely in the fog.
 */
UCLASS(ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class NOWAPROBA_API UMapFogVisualComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMapFogVisualComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** HalfExtent = radius of the safe circle (the Blueprint keeps its LevelHalfExtents naming). */
	UFUNCTION(BlueprintCallable, Category = "Fog")
	void SetFogState(float HalfExtent, int32 FogLevel);

	/** True when Location lies outside the safe circle; false until the first SetFogState. */
	UFUNCTION(BlueprintPure, Category = "Fog")
	bool IsInFog(const FVector& Location) const;

	/** 2D distance (cm) from the safe circle's edge into the fog: positive in the fog, negative inside the circle; -MAX_flt until the first SetFogState. */
	UFUNCTION(BlueprintPure, Category = "Fog")
	float GetDistanceIntoFog(const FVector& Location) const;

	/** Fog level from the last SetFogState. */
	UFUNCTION(BlueprintPure, Category = "Fog")
	int32 GetFogLevel() const { return CurrentLevel; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Fog")
	TObjectPtr<UMaterialParameterCollection> Collection;

	/** One entry per fog level (index = fog level); the last entry is used for higher levels. */
	UPROPERTY(EditAnywhere, Category = "Fog")
	TArray<FFogLevelLook> LevelLooks;

	UPROPERTY(EditAnywhere, Category = "Fog")
	FLinearColor FogColor = FLinearColor(0.85f, 0.55f, 1.f);

	/** Ground height the fog heights are measured from. */
	UPROPERTY(EditAnywhere, Category = "Fog")
	float BaseZ = 0.f;

	/** Width (cm) of the thinner, lower mist band behind the safe circle's edge. */
	UPROPERTY(EditAnywhere, Category = "Fog", meta = (ClampMin = "1"))
	float EdgeWidth = 1500.f;

	/** Mist density right at the safe circle's edge, relative to deep fog. */
	UPROPERTY(EditAnywhere, Category = "Fog", meta = (ClampMin = "0", ClampMax = "1"))
	float EdgeDensity = 0.35f;

	/** World size (cm) of one tile of the mist cloud texture. */
	UPROPERTY(EditAnywhere, Category = "Fog", meta = (ClampMin = "1"))
	float MistScale = 600.f;

	/** Mist drift speed multiplier. */
	UPROPERTY(EditAnywhere, Category = "Fog", meta = (ClampMin = "0"))
	float MistSpeed = 1.f;

	/** Seconds to blend to a new level's look. */
	UPROPERTY(EditAnywhere, Category = "Fog", meta = (ClampMin = "0.01"))
	float LevelBlendTime = 3.f;

	/** Strength of the screen-edge tint while the player stands in the fog. */
	UPROPERTY(EditAnywhere, Category = "Fog", meta = (ClampMin = "0", ClampMax = "1"))
	float VignetteStrength = 0.5f;

	/** How far things beyond ~8 m from the player fade toward the fog colour while the player stands in the fog. */
	UPROPERTY(EditAnywhere, Category = "Fog", meta = (ClampMin = "0", ClampMax = "1"))
	float HazeStrength = 0.2f;

	/** From this fog level on, grass in the fog is hidden (areas fully in fog) or flattened by the grass material. */
	UPROPERTY(EditAnywhere, Category = "Fog|Grass")
	int32 GrassHideLevel = 4;

	/** Collection read by the grass material (MPC_GrassWind; a material may use only 2 collections): gets MapFogArea and MapFogState. */
	UPROPERTY(EditAnywhere, Category = "Fog|Grass")
	TObjectPtr<UMaterialParameterCollection> GrassCollection;

	/** Grass area actors (BP_GrassArea) to hide when they lie completely in the fog. */
	UPROPERTY(EditAnywhere, Category = "Fog|Grass")
	TSubclassOf<AActor> GrassAreaClass;

private:
	FFogLevelLook CurrentLook;
	FFogLevelLook FromLook;
	float BlendAlpha = 1.f;
	float CurrentRadius = 0.f;
	int32 CurrentLevel = 0;
	float PlayerInFog = 0.f;
	FVector PlayerLocation = FVector::ZeroVector;
	bool bLookInitialized = false;
	FTimerHandle GrassTimer;

	const FFogLevelLook& LookForLevel(int32 Level) const;
	void PushToCollection() const;
	void HideGrassInFog();
};
