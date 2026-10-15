# Plan: reakcja trawy na ruch postaci i ataki

Status: plan zatwierdzony kierunkowo, implementacja NIE rozpoczęta. Następny krok: 1 (WPO + wiatr).
Obowiązują zasady z CLAUDE.md (Blueprinty, bez nowych pluginów, commit po każdym kroku).

## Stan wyjściowy (gotowe)
- Meshe: /Game/Environment/Ground/Grass/SM_GrassBlade_A..C, SM_GrassClump_A..D (+ osobne SM_GrassClump_X_LOD1, nieprzypisane jako LOD).
- Dane wierzchołków: Vertex Color R = wysokość 0 (nasada) -> 1 (czubek), G = losowe per źdźbło (faza wiatru),
  B = losowe per kępka, A = 1. UV0: U w poprzek, V = wysokość. Normalne (0,0,1). Źdźbło wygięte w -Y (UE).
- Generator: Tools/Blender/grass_generator.py (Blender, kolekcja GRASS, eksport FBX do SourceArt/Ground/Grass/Export).
- Materiał: /Game/Environment/Ground/Grass/M_Grass (dwustronny, normal world-space (0,0,1), kolor z R, wariacja z B + PerInstanceRandom).
- Rozmieszczanie: /Game/Blueprints/BP_GrassArea (Box "Area" + HISM Grass_A..D, Construction Script, Density/Seed/ScaleMin/ScaleMax/MaxInstances).
- Test: Lvl_TopDown, folder GrassTest, GrassArea_Test przy PlayerStart.
- Szczegóły techniczne i pułapki MCP: Docs/PROJECT_MAP.txt.

## Decyzje użytkownika
- Bez ścinania trawy (nic nie jest trwałe, wszystko zanika).
- Dziesiątki postaci jednocześnie (nie setki).
- Prędkość postaci, siła ataku i prędkość upadku mają skalować zasięg (promień) i siłę interakcji.
- Obszar interakcji: założone ~40x40 m wokół gracza (parametr, do zmiany).

## Architektura
[Postacie / ataki] -> pędzle -> [RT_GrassInteraction] -> M_Grass (World Position Offset)

1. RT_GrassInteraction (~512x512, ~40x40 m wokół gracza, ~8 cm/teksel)
   - RG = kierunek odgięcia * siła (zakodowane z biasem 0.5), B = spłaszczenie.
   - Zanikanie: dwa RT na zmianę (ping-pong), co klatkę poprzedni * współczynnik < 1 (~1-2 s powrotu).
2. BP_GrassInteractionManager (jeden aktor na poziom, jedyny Tick)
   - Co klatkę: zanikanie, narysowanie pędzli wszystkich zarejestrowanych interaktorów jednym przebiegiem canvas,
     przesunięcie środka RT za graczem skokowo co teksel (bez "pływania" śladu).
   - Przekazuje środek/rozmiar RT do MPC_GrassInteraction.
   - Pomija interaktory poza obszarem RT.
   - Funkcja AddGrassImpulse(Location, Radius, Direction, Strength, Shape) dla zdarzeń; brak managera = nic się nie dzieje.
3. GrassInteractorComponent (Actor Component, bez Ticka; manager czyta jego stan)
   - Na graczu, NPC, wrogach, pociskach. Rejestruje się w managerze na BeginPlay, wyrejestrowuje na EndPlay.
   - Ślad ciągły z prędkości poziomej (CharacterMovement) i stanu z MovementStateComponent (slide itd.), bez twardego powiązania.
   - Lądowanie: event Landed, siła z prędkości spadania w chwili uderzenia.
   - Profil: DA_GrassInteractionProfile (per typ postaci).
4. M_Grass (WPO)
   - Czyta RT w world XY (pozycja/rozmiar z MPC), odgina czubek proporcjonalnie do R^2, obniża czubek (zginanie zamiast rozciągania).
   - Wiatr jako osobny składnik (faza z G), sumowany z interakcją.
   - HISM w BP_GrassArea: powiększyć bounds (positive/negative bounds extension), żeby wygięta trawa nie znikała.

## Skalowanie (MapRangeClamped, zakresy w DA_GrassInteractionProfile)
| Źródło | Wejście | Promień | Siła | Uwagi |
|---|---|---|---|---|
| Ruch ciągły (chód/bieg/slide/dash) | prędkość pozioma, np. 300->1500 cm/s | 40->120 cm | 0.3->1.0 | pędzel-kapsuła wydłużony wzdłuż prędkości; slide mocniej spłaszcza, dash mocniej odgina (inne profile) |
| Stanie | - | mały | mały docisk | |
| W powietrzu | - | brak | brak | trawa się prostuje |
| Lądowanie | prędkość spadania, np. 400->2000 cm/s | 60->300 cm | 0.4->1.0 | pierścień na zewnątrz; poniżej progu tylko lekki docisk |
| Atak / umiejętność | Strength z AddGrassImpulse | BaseRadius * krzywa(Strength) | Strength znormalizowany | kształt koło/łuk/stożek, kierunek z rotacji postaci |

Wartości liczbowe są startowe, do strojenia w PIE.

## Kroki (każdy = osobne zadanie + commit)
1. WPO + wiatr w M_Grass (parametry: siła/prędkość/kierunek wiatru, np. przez MPC). Przy okazji weryfikacja,
   że R dociera do UE liniowo (R ~ 0.5 w połowie źdźbła; kontrola gamma niewykonana w zadaniu importu).
2. RT (2x ping-pong) + MPC_GrassInteraction + BP_GrassInteractionManager + GrassInteractorComponent na graczu;
   ślad zależny od prędkości. Test w PIE (Lvl_TopDown, GrassArea_Test).
3. Lądowanie zależne od prędkości spadania; DA_GrassInteractionProfile z profilami chód/bieg/slide/dash.
4. AddGrassImpulse + kształty (koło/łuk/stożek) dla ataków; komponent na NPC/wrogach.

## Otwarte / do pamiętania
- Dash jeszcze nie istnieje w grze: dostanie profil, gdy powstanie.
- Rozmiar obszaru RT (40x40 m) do potwierdzenia przy kamerze z gry.
- LOD1 kępek trzeba przypisać ręcznie (Static Mesh Editor -> LOD Import), MCP tego nie umie.
- Aktorzy postawieni przez MCP wymagają zapisu Ctrl+S w edytorze (MCP nie zapisuje nowych aktorów OFPA).
- Grafy Blueprint w polskim edytorze: budować create_node + connect_pins (write_graph_dsl nie działa) - patrz PROJECT_MAP.
