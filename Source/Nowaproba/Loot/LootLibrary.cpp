#include "LootLibrary.h"

#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "../Currency/CurrencyComponent.h"
#include "../Resources/ResourceComponent.h"

namespace
{
	int32 RandRangeSorted(int32 A, int32 B)
	{
		return FMath::RandRange(FMath::Min(A, B), FMath::Max(A, B));
	}

	EResourceType ToResourceType(ELootResource Resource)
	{
		switch (Resource)
		{
		case ELootResource::Resource1: return EResourceType::Resource1;
		case ELootResource::Resource2: return EResourceType::Resource2;
		case ELootResource::Resource3: return EResourceType::Resource3;
		default: return static_cast<EResourceType>(FMath::RandRange(0, UResourceComponent::NumResources - 1));
		}
	}
}

FLootResult ULootLibrary::RollLoot(const UDataTable* Table, FName RowName)
{
	FLootResult Result;
	const FLootTableRow* Row = Table ? Table->FindRow<FLootTableRow>(RowName, TEXT("RollLoot")) : nullptr;
	if (!Row || FMath::FRand() >= Row->DropChance)
	{
		return Result;
	}

	Result.Coins = FMath::Max(0, RandRangeSorted(Row->CoinsMin, Row->CoinsMax));

	if (FMath::FRand() < Row->ResourceChance)
	{
		Result.ResourceAmount = FMath::Max(0, RandRangeSorted(Row->ResourceMin, Row->ResourceMax));
		Result.bHasResource = Result.ResourceAmount > 0;
		Result.Resource = ToResourceType(Row->ResourceType);
	}

	if (Row->ItemRarityMax != ERarity::None && FMath::FRand() < Row->ItemChance)
	{
		const int32 Min = FMath::Max(static_cast<int32>(Row->ItemRarityMin), static_cast<int32>(ERarity::Common));
		Result.ItemRarity = static_cast<ERarity>(RandRangeSorted(Min, static_cast<int32>(Row->ItemRarityMax)));
	}

	return Result;
}

FName ULootLibrary::PickWeightedRow(const UDataTable* Table, const TArray<FName>& OnlyRows)
{
	if (!Table)
	{
		return NAME_None;
	}

	auto WeightOf = [&OnlyRows](const FName& Name, const FLootTableRow& Row)
	{
		return OnlyRows.Num() == 0 || OnlyRows.Contains(Name) ? FMath::Max(0.f, Row.SpawnWeight) : 0.f;
	};

	float Total = 0.f;
	Table->ForeachRow<FLootTableRow>(TEXT("PickWeightedRow"), [&Total, &WeightOf](const FName& Name, const FLootTableRow& Row)
	{
		Total += WeightOf(Name, Row);
	});
	if (Total <= 0.f)
	{
		return NAME_None;
	}

	float Pick = FMath::FRand() * Total;
	FName Picked = NAME_None;
	Table->ForeachRow<FLootTableRow>(TEXT("PickWeightedRow"), [&Pick, &Picked, &WeightOf](const FName& Name, const FLootTableRow& Row)
	{
		const float Weight = WeightOf(Name, Row);
		if (Picked.IsNone() && Weight > 0.f)
		{
			Pick -= Weight;
			if (Pick < 0.f)
			{
				Picked = Name;
			}
		}
	});
	return Picked;
}

void ULootLibrary::GrantLoot(APawn* Pawn, const FLootResult& Loot)
{
	if (!Pawn)
	{
		return;
	}

	if (Loot.Coins > 0)
	{
		if (UCurrencyComponent* Currency = Pawn->FindComponentByClass<UCurrencyComponent>())
		{
			Currency->AddCurrency(Loot.Coins);
		}
	}

	if (Loot.bHasResource)
	{
		if (UResourceComponent* Resources = Pawn->FindComponentByClass<UResourceComponent>())
		{
			Resources->AddResource(Loot.Resource, Loot.ResourceAmount);
		}
	}

	// Placeholder until items exist: show what would have dropped.
	if (Loot.ItemRarity != ERarity::None && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan,
			FString::Printf(TEXT("Item: %s"), *StaticEnum<ERarity>()->GetDisplayNameTextByValue(static_cast<int64>(Loot.ItemRarity)).ToString()));
	}
}

FLootResult ULootLibrary::RollAndGrantLoot(const UDataTable* Table, FName RowName, APawn* Pawn)
{
	const FLootResult Loot = RollLoot(Table, RowName);
	GrantLoot(Pawn, Loot);
	return Loot;
}
