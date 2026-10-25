#pragma once

#include "CoreMinimal.h"
#include "EnemyAIController.h"
#include "EnemyRangedAIController.generated.h"

/**
 * Brain for ranged enemies (e.g. the skeleton archer): same detection and idle behaviour as AEnemyAIController, but in
 * combat it stands still while attacking and between moves faces the player. When an attack is off cooldown, in range
 * and has a clear shot (world static only: shots fly through allies) it stops and attacks. Otherwise it repositions:
 * every RepositionInterval (or at once when the player gets closer than MinDistance, or when it is ready to shoot but
 * cannot) it scores the current spot and CandidateCount short, straight, walkable moves around itself (navmesh
 * raycasts, so it never runs into walls) and walks to the best one. Score: distance to the player near a preferred
 * distance re-rolled each pick within [MinDistance, MaxDistance] (no fixed ring), clear shot, a melee ally between the
 * spot and the player (cover), how much free navmesh lies behind and beside the spot (corners and wall hugging score
 * low), no path past the player, short moves.
 */
UCLASS()
class NOWAPROBA_API AEnemyRangedAIController : public AEnemyAIController
{
	GENERATED_BODY()

protected:
	virtual void UpdateCombat(APawn* Player, float Distance) override;
	virtual void OnChaseChanged(bool bNowChasing) override;

	/** Closer than this the player is too close: back off (2D). */
	UPROPERTY(EditAnywhere, Category = "Ranged AI", meta = (ClampMin = "0"))
	float MinDistance = 600.f;

	/** Preferred distance band upper end (keep below the main attack's range). */
	UPROPERTY(EditAnywhere, Category = "Ranged AI", meta = (ClampMin = "0"))
	float MaxDistance = 1200.f;

	/** Length range of one repositioning move. */
	UPROPERTY(EditAnywhere, Category = "Ranged AI", meta = (ClampMin = "50"))
	float MinStep = 250.f;

	UPROPERTY(EditAnywhere, Category = "Ranged AI", meta = (ClampMin = "50"))
	float MaxStep = 600.f;

	/** Average time between picks while not forced (randomised +-30%); after arriving it waits out the rest. */
	UPROPERTY(EditAnywhere, Category = "Ranged AI", meta = (ClampMin = "0.2"))
	float RepositionInterval = 1.4f;

	/** Moves sampled per pick (evenly spread directions, random lengths). */
	UPROPERTY(EditAnywhere, Category = "Ranged AI", meta = (ClampMin = "4", ClampMax = "32"))
	int32 CandidateCount = 12;

	/** An ally this close to the line from a spot to the player counts as cover. */
	UPROPERTY(EditAnywhere, Category = "Ranged AI", meta = (ClampMin = "0"))
	float CoverWidth = 150.f;

	/** Navmesh checked this far behind, diagonally behind and beside a spot; the nearer an edge, the lower its score. */
	UPROPERTY(EditAnywhere, Category = "Ranged AI", meta = (ClampMin = "50"))
	float OpenSpaceCheckDistance = 500.f;

	/** Score weights. */
	UPROPERTY(EditAnywhere, Category = "Ranged AI|Score")
	float CoverBonus = 400.f;

	UPROPERTY(EditAnywhere, Category = "Ranged AI|Score")
	float NoShotPenalty = 500.f;

	/** Per direction at full closeness (behind counts 1.5x, beside 0.75x). */
	UPROPERTY(EditAnywhere, Category = "Ranged AI|Score")
	float BlockedSidePenalty = 250.f;

	UPROPERTY(EditAnywhere, Category = "Ranged AI|Score")
	float StayBonus = 100.f;

private:
	FVector MoveGoal = FVector::ZeroVector;
	double NextPickTime = 0.0;
	double LastPickTime = -1.0;

	void SetFacePlayer(bool bFace);
	bool HasClearShot(const FVector& From, const APawn* Player) const;
	void Reposition(const APawn* Player);
};
