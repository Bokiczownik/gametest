# Coin combo (archived)

Removed from the game on 2026-10-18; kept for a later return (combo for fast looting or for combat).

- `CoinComboComponent` was on BP_TopDownCharacter as "Combo": each coin picked up while the bar was active added +0.1 to the multiplier (max x5), the bar lasted 1.5 s, reward = Round(coin value * multiplier) into UCurrencyComponent.
- `ComboWidget` was created in NewHUD BeginPlay (top-centre bar).
- AGoldCoin called `Combo->RegisterCoin(Value)` and fell back to the wallet without the component.

To restore: move the four files back to `Source/Nowaproba/Collectibles/`, add the component to BP_TopDownCharacter as "Combo", create UComboWidget in NewHUD BeginPlay and call RegisterCoin from AGoldCoin again.
