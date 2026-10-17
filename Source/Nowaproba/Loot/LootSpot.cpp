#include "LootSpot.h"

#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"

ALootSpot::ALootSpot()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

#if WITH_EDITORONLY_DATA
	Sprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (Sprite)
	{
		Sprite->SetupAttachment(GetRootComponent());
	}

	Arrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	if (Arrow)
	{
		Arrow->SetupAttachment(GetRootComponent());
		Arrow->ArrowSize = 2.f;
	}
#endif
}

void ALootSpot::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

#if WITH_EDITORONLY_DATA
	if (Arrow)
	{
		switch (SpotType)
		{
		case ELootSpotType::Chest: Arrow->SetArrowColor(FLinearColor(1.f, 0.8f, 0.1f)); break;
		case ELootSpotType::FogChest: Arrow->SetArrowColor(FLinearColor(0.7f, 0.5f, 1.f)); break;
		case ELootSpotType::Barrel: Arrow->SetArrowColor(FLinearColor(0.5f, 0.3f, 0.1f)); break;
		case ELootSpotType::ResourceNode: Arrow->SetArrowColor(FLinearColor(0.1f, 1.f, 0.4f)); break;
		}
	}
#endif
}
