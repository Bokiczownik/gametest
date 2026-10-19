#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

namespace CombatAim
{
	/**
	 * Horizontal direction from Origin to the point under the mouse cursor, found by intersecting the cursor ray with the
	 * horizontal plane at Origin's height (not a scene trace, so walls / props / slopes under the cursor do not skew it).
	 * Returns false when there is no cursor or the ray does not reach the plane.
	 */
	inline bool GetCursorDirection(const APlayerController* PC, const FVector& Origin, FVector& OutDirection)
	{
		FVector RayOrigin, RayDirection;
		if (!PC || !PC->DeprojectMousePositionToWorld(RayOrigin, RayDirection) || RayDirection.Z > -KINDA_SMALL_NUMBER)
		{
			return false;
		}

		const FVector Point = RayOrigin + RayDirection * ((Origin.Z - RayOrigin.Z) / RayDirection.Z);
		OutDirection = (Point - Origin).GetSafeNormal2D();
		return !OutDirection.IsNearlyZero();
	}
}
