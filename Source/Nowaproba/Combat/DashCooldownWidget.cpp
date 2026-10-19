#include "DashCooldownWidget.h"

#include "AbilitySystemGlobals.h"
#include "DashAbility.h"
#include "Rendering/DrawElements.h"

namespace
{
	/** Pie of [0, Fraction] clockwise from 12 o'clock, drawn as a line of thickness Radius along the circle at Radius / 2. */
	void DrawPie(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& Geometry, const FVector2D& Center,
		float Radius, float Fraction, const FLinearColor& Color)
	{
		constexpr int32 FullSegments = 48;
		const int32 Segments = FMath::Max(1, FMath::CeilToInt(FullSegments * Fraction));
		TArray<FVector2D> Points;
		Points.Reserve(Segments + 1);
		for (int32 i = 0; i <= Segments; ++i)
		{
			const float Angle = -HALF_PI + TWO_PI * Fraction * i / Segments;
			Points.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius * 0.5f);
		}
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color, true, Radius);
	}
}

void UDashCooldownWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UDashCooldownWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const float NewProgress = UDashAbility::GetDashCooldownProgress(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwningPlayerPawn()));
	if (NewProgress != Progress)
	{
		// The widget has no child widgets that change, so Slate would keep the cached paint without this.
		Progress = NewProgress;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

int32 UDashCooldownWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const FVector2D Center(Size.X * 0.5f, Size.Y - BottomOffset - Radius);
	const bool bReady = Progress >= 1.f;

	DrawPie(OutDrawElements, ++LayerId, AllottedGeometry, Center, Radius, 1.f, bReady ? ReadyColor : BackgroundColor);
	if (!bReady && Progress > 0.f)
	{
		DrawPie(OutDrawElements, ++LayerId, AllottedGeometry, Center, Radius, Progress, ChargingColor);
	}
	return LayerId;
}
