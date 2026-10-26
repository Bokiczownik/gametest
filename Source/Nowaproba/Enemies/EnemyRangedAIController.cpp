#include "EnemyRangedAIController.h"

#include "CollisionQueryParams.h"
#include "EnemyAttackComponent.h"
#include "EnemyCharacter.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"

namespace
{
	/** Forced picks (player too close, no shot) are not repeated faster than this (s). */
	constexpr double RangedMinPickGap = 0.4;

	/** Moves end this far short of a navmesh edge hit. */
	constexpr float RangedWallMargin = 40.f;

	/** Firing spots sampled around the player when no nearby move has a shot, and how many of the nearest get a path query. */
	constexpr int32 RangedFiringSpotSamples = 12;
	constexpr int32 RangedFiringSpotPathChecks = 4;

	float DistToSegment2D(const FVector& Point, const FVector& A, const FVector& B)
	{
		return FMath::PointDistToSegment(FVector(Point.X, Point.Y, 0.f), FVector(A.X, A.Y, 0.f), FVector(B.X, B.Y, 0.f));
	}
}

void AEnemyRangedAIController::UpdateCombat(APawn* Player, float Distance)
{
	AEnemyCharacter* Self = GetEnemy();
	UEnemyAttackComponent* Attack = Self->GetAttack();
	Self->SetInCombat(true);
	if (GetFocusActor() != Player)
	{
		SetFocus(Player);
	}

	const FVector ShotStart = Self->GetActorLocation() + FVector(0.f, 0.f, Attack->GetProjectileSpawnOffset().Z);
	if (Attack->CanStartAttack(Player) && HasClearShot(ShotStart, Player))
	{
		StopMovement();
		SetFacePlayer(true);
		Attack->StartAttack(Player);
		// Move again as soon as the attack is over.
		NextPickTime = 0.0;
		return;
	}

	const bool bMoving = GetMoveStatus() == EPathFollowingStatus::Moving;
	SetFacePlayer(!bMoving);

	const double Now = GetWorld()->GetTimeSeconds();
	bool bPick = !bMoving && Now >= NextPickTime;
	if (Now - LastPickTime >= RangedMinPickGap)
	{
		// Player closing in: leave now unless already heading away from it.
		const bool bHeadingAway = bMoving && FVector::Dist2D(MoveGoal, Player->GetActorLocation()) > Distance + 100.f;
		bPick |= Distance < MinDistance && !bHeadingAway;
		// Ready to shoot but not from here (out of range or no line of fire).
		bPick |= !bMoving && Attack->IsOffCooldown();
	}
	if (bPick)
	{
		Reposition(Player);
	}
}

void AEnemyRangedAIController::OnChaseChanged(bool bNowChasing)
{
	if (!bNowChasing)
	{
		ClearFocus(EAIFocusPriority::Gameplay);
		SetFacePlayer(false);
		NextPickTime = 0.0;
	}
}

void AEnemyRangedAIController::SetFacePlayer(bool bFace)
{
	// Standing: turn toward the focus (player); walking: face the walk direction.
	if (AEnemyCharacter* Self = GetEnemy())
	{
		UCharacterMovementComponent* Movement = Self->GetCharacterMovement();
		Movement->bOrientRotationToMovement = !bFace;
		Movement->bUseControllerDesiredRotation = bFace;
	}
}

bool AEnemyRangedAIController::HasClearShot(const FVector& From, const APawn* Player) const
{
	// Shots fly through pawns, so only level geometry blocks them.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(RangedEnemyShot), false, GetPawn());
	FHitResult Hit;
	return !GetWorld()->LineTraceSingleByObjectType(Hit, From, Player->GetActorLocation(), FCollisionObjectQueryParams(ECC_WorldStatic), Params);
}

void AEnemyRangedAIController::Reposition(const APawn* Player)
{
	AEnemyCharacter* Self = GetEnemy();
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation SelfNav;
	if (!NavSys || !NavSys->ProjectPointToNavigation(Self->GetNavAgentLocation(), SelfNav))
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	LastPickTime = Now;
	NextPickTime = Now + RepositionInterval * FMath::FRandRange(0.7f, 1.3f);

	const FVector Start = SelfNav.Location;
	const FVector PlayerLocation = Player->GetActorLocation();
	const float ShotHeight = Self->GetActorLocation().Z - Self->GetNavAgentLocation().Z + Self->GetAttack()->GetProjectileSpawnOffset().Z;
	const float PreferredDistance = FMath::FRandRange(MinDistance, FMath::Max(MinDistance, MaxDistance));

	// Melee allies near the fight can screen this enemy from the player.
	TArray<FVector, TInlineAllocator<8>> Screens;
	for (TActorIterator<AEnemyCharacter> It(GetWorld()); It; ++It)
	{
		const AEnemyCharacter* Ally = *It;
		if (Ally != Self && !Ally->IsDead() && !Cast<AEnemyRangedAIController>(Ally->GetController())
			&& FVector::Dist2D(Ally->GetActorLocation(), PlayerLocation) <= MaxDistance)
		{
			Screens.Add(Ally->GetActorLocation());
		}
	}

	// 0 = open navmesh for the whole check distance in that direction, 1 = blocked right at the spot.
	const auto Closeness = [this](const FVector& From, const FVector& Direction) -> float
	{
		FVector HitLocation;
		if (!UNavigationSystemV1::NavigationRaycast(this, From, From + Direction * OpenSpaceCheckDistance, HitLocation))
		{
			return 0.f;
		}
		return 1.f - FMath::Clamp(FVector::Dist2D(From, HitLocation) / OpenSpaceCheckDistance, 0.f, 1.f);
	};

	const auto Score = [&](const FVector& Spot)
	{
		const float Dist = FVector::Dist2D(Spot, PlayerLocation);
		float Result = -FMath::Abs(Dist - PreferredDistance) * 0.5f;
		Result -= FMath::Max(0.f, MinDistance - Dist) * 2.f;
		Result -= FMath::Max(0.f, Dist - MaxDistance);

		if (!HasClearShot(Spot + FVector(0.f, 0.f, ShotHeight), Player))
		{
			Result -= NoShotPenalty;
		}

		const FVector ToPlayer = PlayerLocation - Spot;
		const float ToPlayerSq = ToPlayer.SizeSquared2D();
		for (const FVector& Screen : Screens)
		{
			const FVector ToScreen = Screen - Spot;
			const float Along = ToPlayerSq > 1.f ? (ToScreen.X * ToPlayer.X + ToScreen.Y * ToPlayer.Y) / ToPlayerSq : 0.f;
			if (Along > 0.1f && Along < 0.9f && DistToSegment2D(Screen, Spot, PlayerLocation) <= CoverWidth)
			{
				Result += CoverBonus;
				break;
			}
		}

		// Room to back off and sidestep later: the nearer a wall or edge behind or beside the spot, the lower its score,
		// so corners score lowest and a cornered enemy prefers moving out along the wall.
		const FVector Away = (-ToPlayer).GetSafeNormal2D();
		const FVector Up = FVector::UpVector;
		float Cramped = Closeness(Spot, Away) * 1.5f;
		Cramped += Closeness(Spot, Away.RotateAngleAxis(45.f, Up)) + Closeness(Spot, Away.RotateAngleAxis(-45.f, Up));
		Cramped += (Closeness(Spot, Away.RotateAngleAxis(90.f, Up)) + Closeness(Spot, Away.RotateAngleAxis(-90.f, Up))) * 0.75f;
		Result -= Cramped * BlockedSidePenalty;

		return Result + FMath::FRandRange(0.f, 50.f);
	};

	FVector Best = Start;
	float BestScore = Score(Start) + StayBonus;
	const float AngleOffset = FMath::FRandRange(0.f, 360.f);
	for (int32 i = 0; i < CandidateCount; ++i)
	{
		const FVector Direction = FVector::ForwardVector.RotateAngleAxis(AngleOffset + 360.f * i / CandidateCount, FVector::UpVector);
		FVector End = Start + Direction * FMath::FRandRange(MinStep, FMath::Max(MinStep, MaxStep));

		// Straight walkable moves only: cut short before a wall or ledge.
		FVector HitLocation;
		if (UNavigationSystemV1::NavigationRaycast(this, Start, End, HitLocation))
		{
			End = HitLocation - Direction * RangedWallMargin;
		}
		FNavLocation EndNav;
		if (FVector::Dist2D(Start, End) < MinStep * 0.5f || !NavSys->ProjectPointToNavigation(End, EndNav))
		{
			continue;
		}

		float SpotScore = Score(EndNav.Location) - FVector::Dist2D(Start, EndNav.Location) * 0.25f;
		// Never run past the player.
		const float PassDistance = DistToSegment2D(PlayerLocation, Start, EndNav.Location);
		if (PassDistance < MinDistance * 0.5f && PassDistance < FVector::Dist2D(Start, PlayerLocation) - 50.f)
		{
			SpotScore -= 500.f;
		}
		if (SpotScore > BestScore)
		{
			BestScore = SpotScore;
			Best = EndNav.Location;
		}
	}

	// No shot from anywhere nearby (e.g. behind a long wall): walk the navmesh path to a firing spot around the player.
	FVector FiringSpot;
	if (!HasClearShot(Best + FVector(0.f, 0.f, ShotHeight), Player) && FindFiringSpot(Player, Start, ShotHeight, FiringSpot))
	{
		Best = FiringSpot;
	}

	if (Best != Start)
	{
		MoveGoal = Best;
		SetFacePlayer(false);
		MoveToLocation(Best, 30.f);
	}
}

bool AEnemyRangedAIController::FindFiringSpot(const APawn* Player, const FVector& Start, float ShotHeight, FVector& OutSpot) const
{
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys)
	{
		return false;
	}

	// Spots in the preferred band around the player that have a shot, nearest (straight line) first.
	const FVector PlayerFeet = Player->GetNavAgentLocation();
	TArray<FVector, TInlineAllocator<RangedFiringSpotSamples>> Spots;
	const float AngleOffset = FMath::FRandRange(0.f, 360.f);
	for (int32 i = 0; i < RangedFiringSpotSamples; ++i)
	{
		const FVector Direction = FVector::ForwardVector.RotateAngleAxis(AngleOffset + 360.f * i / RangedFiringSpotSamples, FVector::UpVector);
		FNavLocation SpotNav;
		if (NavSys->ProjectPointToNavigation(PlayerFeet + Direction * FMath::FRandRange(MinDistance, FMath::Max(MinDistance, MaxDistance)), SpotNav)
			&& HasClearShot(SpotNav.Location + FVector(0.f, 0.f, ShotHeight), Player))
		{
			Spots.Add(SpotNav.Location);
		}
	}
	Spots.Sort([&Start](const FVector& A, const FVector& B) { return FVector::DistSquared2D(A, Start) < FVector::DistSquared2D(B, Start); });

	// Of the nearest few, take the one with the shortest complete path.
	float BestLength = TNumericLimits<float>::Max();
	for (int32 i = 0; i < FMath::Min(Spots.Num(), RangedFiringSpotPathChecks); ++i)
	{
		const UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), Start, Spots[i], GetPawn());
		if (Path && Path->IsValid() && !Path->IsPartial() && Path->GetPathLength() < BestLength)
		{
			BestLength = Path->GetPathLength();
			OutSpot = Spots[i];
		}
	}
	return BestLength < TNumericLimits<float>::Max();
}
