#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Interaction/Interactable.h"
#include "ShopNPC.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class USphereComponent;
class UTextRenderComponent;
class UUserWidget;

/**
 * Shop keeper: while the player is inside InteractionRange it becomes the player's interaction target and shows
 * the "Press E" prompt; interacting toggles the shop window. The window closes when the player walks away.
 * Mesh / animation are set in the Blueprint subclass (BP_ShopNPC).
 */
UCLASS()
class NOWAPROBA_API AShopNPC : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AShopNPC();

	virtual void Interact(APawn* InstigatorPawn) override;

	UFUNCTION(BlueprintPure, Category = "Shop")
	bool IsShopOpen() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Blocks the player like a character would. */
	UPROPERTY(VisibleAnywhere, Category = "Shop")
	TObjectPtr<UCapsuleComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "Shop")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	/** Player inside this sphere can interact. */
	UPROPERTY(VisibleAnywhere, Category = "Shop")
	TObjectPtr<USphereComponent> InteractionRange;

	UPROPERTY(VisibleAnywhere, Category = "Shop")
	TObjectPtr<UTextRenderComponent> NameLabel;

	/** Shown only while the player can interact. */
	UPROPERTY(VisibleAnywhere, Category = "Shop")
	TObjectPtr<UTextRenderComponent> PromptLabel;

	/** Window opened on interact; defaults to the placeholder UShopWidget. */
	UPROPERTY(EditAnywhere, Category = "Shop")
	TSubclassOf<UUserWidget> ShopWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<UUserWidget> ShopWidget;

	UFUNCTION()
	void HandleRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	void OpenShop(APawn* InstigatorPawn);
	void CloseShop();
};
