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

	/** 0..1, 1 = ground fully covered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog", meta = (ClampMin = "0", ClampMax = "1"))
	float Opacity = 0.25f;

	/** Height above BaseZ (cm) with full Opacity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog", meta = (ClampMin = "0"))
	float FullHeight = 0.f;

	/** Height above BaseZ (cm) where the fog has faded out. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog", meta = (ClampMin = "1"))
	float FadeHeight = 300.f;

	/** 0..1 strength of the moving cloud blobs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fog", meta = (ClampMin = "0", ClampMax = "1"))
	float CloudStrength = 1.f;
};

/**
 * Stylised fog look for BP_MapFog. The owner's Blueprint calls SetFogState every tick with the safe square's half
 * extent and the current fog level; the owner's location is the square centre. Writes everything to MPC_MapFog
 * (read by the fog post-process and the fog walls; the grass material reads GrassCollection), blends between level looks, fades the
 * "player in fog" vignette, and from GrassHideLevel on hides grass areas lying completely in the fog.
 */
UCLASS(ClassGroup = (Gameplay), meta = (BlueprintSpawnableComponent))
class NOWAPROBA_API UMapFogVisualComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMapFogVisualComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Fog")
	void SetFogState(float HalfExtent, int32 FogLevel);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Fog")
	TObjectPtr<UMaterialParameterCollection> Collection;

	/** One entry per fog level (index = fog level); the last entry is used for higher levels. */
	UPROPERTY(EditAnywhere, Category = "Fog")
	TArray<FFogLevelLook> LevelLooks;

	UPROPERTY(EditAnywhere, Category = "Fog")
	FLinearColor FogColor = FLinearColor(0.95f, 0.78f, 0.98f);

	/** Ground height the fog heights are measured from. */
	UPROPERTY(EditAnywhere, Category = "Fog")
	float BaseZ = 0.f;

	/** Width (cm) of the soft border where the fog builds up behind the safe square's edge. */
	UPROPERTY(EditAnywhere, Category = "Fog", meta = (ClampMin = "1"))
	float EdgeWidth = 400.f;

	/** Number of toon bands; 0 = smooth. */
	UPROPERTY(EditAnywhere, Category = "Fog", meta = (ClampMin = "0"))
	float Steps = 4.f;

	/** Size (cm) of the cloud blobs. */
	UPROPERTY(EditAnywhere, Category = "Fog", meta = (ClampMin = "1"))
	float CloudScale = 600.f;

	UPROPERTY(EditAnywhere, Category = "Fog", meta = (ClampMin = "0"))
	float CloudSpeed = 0.3f;

	/** Seconds to blend to a new level's look. */
	UPROPERTY(EditAnywhere, Category = "Fog", meta = (ClampMin = "0.01"))
	float LevelBlendTime = 3.f;

	/** Strength of the screen-edge tint while the player stands in the fog. */
	UPROPERTY(EditAnywhere, Category = "Fog", meta = (ClampMin = "0", ClampMax = "1"))
	float VignetteStrength = 0.5f;

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
	float CurrentHalfExtent = 0.f;
	int32 CurrentLevel = 0;
	float PlayerInFog = 0.f;
	bool bLookInitialized = false;
	FTimerHandle GrassTimer;

	const FFogLevelLook& LookForLevel(int32 Level) const;
	void PushToCollection() const;
	void HideGrassInFog();
};
