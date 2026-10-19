#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponData.generated.h"

class UAnimSequenceBase;
class UStaticMesh;

/** One attack of a weapon moveset: a time range of an animation plus its hit window and tuning. Times are in animation seconds. */
USTRUCT(BlueprintType)
struct FWeaponAttack
{
	GENERATED_BODY()

	/** Played on the owner's DefaultSlot as a dynamic montage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	TObjectPtr<UAnimSequenceBase> Animation;

	/** Animation time where the attack starts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0"))
	float StartTime = 0.f;

	/** Animation time where the attack ends (<= StartTime = end of the animation). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0"))
	float EndTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.1"))
	float PlayRate = 1.f;

	/** Animation time range in which the weapon deals damage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Hit", meta = (ClampMin = "0"))
	float HitStartTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Hit", meta = (ClampMin = "0"))
	float HitEndTime = 0.f;

	/** Animation time from which the next attack (buffered input) may start. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0"))
	float ChainTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Hit", meta = (ClampMin = "0"))
	float Damage = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Hit", meta = (ClampMin = "0"))
	float PoiseDamage = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0"))
	float StaminaCost = 4.f;

	/** Forward step at the start of the attack (cm, ground only). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Lunge", meta = (ClampMin = "0"))
	float LungeDistance = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack|Lunge", meta = (ClampMin = "0.05"))
	float LungeDuration = 0.15f;
};

/**
 * Weapon definition: mesh held in the right hand, blade line used for hit sweeps and the universal moveset
 * (light combo, heavy, sprint and jump attack). Used by UWeaponComponent.
 */
UCLASS(BlueprintType)
class NOWAPROBA_API UWeaponData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	FName AttachSocket = TEXT("hand_r");

	/** Mesh transform relative to AttachSocket. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
	FTransform AttachTransform;

	/** Added to the cursor direction when an attack turns the owner (deg); e.g. 90 for a sideways spear stance that strikes towards the character's left. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Moveset")
	float AttackFacingYawOffset = 0.f;

	/** Damaging part of the weapon along the mesh's local X axis (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit")
	float BladeStart = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit")
	float BladeEnd = 148.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit", meta = (ClampMin = "1"))
	float HitRadius = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Moveset")
	TArray<FWeaponAttack> LightCombo;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Moveset")
	FWeaponAttack HeavyAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Moveset")
	FWeaponAttack SprintAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Moveset")
	FWeaponAttack JumpAttack;

	/** Holding attack this long (s) performs the heavy attack. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Moveset", meta = (ClampMin = "0.05"))
	float HeavyHoldTime = 0.35f;

	/** After an attack ends, no new attack can start for this long (s). Chaining inside an attack (ChainTime) is not affected. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Moveset", meta = (ClampMin = "0"))
	float AttackCooldown = 0.5f;

	/** The light combo restarts if the next light attack comes later than this after the previous one ended (s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Moveset", meta = (ClampMin = "0"))
	float ComboResetTime = 0.6f;
};
