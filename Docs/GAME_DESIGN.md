# Game Design – ustalenia

Stan: ustalenia z rozmowy projektowej (2026-10-17). Nic z tego nie jest jeszcze zaimplementowane.

## Gatunek i cel
- Hack and slash z elementami platformówki, w stylu roguelike (inspiracja: Megabonk).
- Mniej przeciwników niż w Megabonku, więcej myślenia i planowania.
- Gra przechodzona kilkaset razy różnymi buildami, więc pojedynczy run musi być krótki.

## Struktura runu
- Seria map. Między mapami jest mapa ze sklepem (zakup ulepszeń), potem kolejna mapa.
- Śmierć oznacza nowy run. Wyjątek: przedmiot dający drugie życie.
- Propozycja do weryfikacji: 4–5 map po 4–6 min, cały run 20–30 min.
- Sklep może pokazywać podgląd następnej mapy (biom, modyfikator), żeby było pod co planować zakupy (propozycja).

## Mapy
- Układ terenu robiony ręcznie, zawartość losowana w ręcznie postawionych punktach (skrzynie, złoża, grupy wrogów, wyjście, eventy).
- Kilka map na etap, z których losuje się jedną, plus modyfikatory mapy (propozycja).
- Kilka poziomów w pionie: pod mostem (np. skrzynia), na moście (parkour po zawalonych elementach), ukryte miejsca wysoko (np. 100 m nad mostem).
- Dostęp do trudnych miejsc ogranicza trudność dotarcia (np. wysokość skoku z modyfikatorów), a nie brak konkretnego przedmiotu.
- Skala map: do ustalenia. Obecny Lvl_Landscape (2 km) służy tylko do testów. Docelową skalę wyznaczy lista zawartości mapy (struktury, zasoby, punkty).

## Wyjście z mapy
- Nieoznaczone na starcie, losowane z kilku możliwych miejsc.
- Do wyjścia potrzebny klucz albo zapłata równowartości w walucie (wyjście awaryjne kosztem zakupów w sklepie).
- Klucz zawsze w ryzykownym miejscu: u elity, w trudno dostępnym miejscu albo w strefie, którą mgła zajmuje wcześnie.

## Mgła (presja czasu)
- Granica mapy zawęża się z czasem i jest oznaczona mgłą.
- Mgła nie zabija sama. Za nią pojawia się coraz więcej coraz silniejszych przeciwników.
- Kilka poziomów mgły. Na ostatnim przeciwnicy są śmiertelni, ale na tyle trudni, że zostaje tylko ucieczka albo śmierć.
- Mgła zawęża się w stronę wyjścia. To, że zdradza jego położenie, jest zamierzone (nagroda za uważność).
- Część obszarów jest spod mgły wyłączona: okolica wyjścia, struktury ponad mgłą.
- Gracz może cofnąć się w mgłę do niezbadanej części mapy na własne ryzyko.
- Do rozważenia: lepsze nagrody w obszarach, które mgła już zajęła; zagrożenie rosnące z głębokością i czasem spędzonym w mgle; ograniczenie przeczekiwania w miejscach ponad mgłą (latający lub strzelający wrogowie w późnych poziomach).
- Krawędź mgły musi być czytelna, czyli osobny efekt, a nie mgła odległości (ExponentialHeightFog).

## Przeciwnicy
- Mogą wychodzić poza mgłę, a z czasem robią to coraz częściej.
- Nie wszyscy idą na ślepo w stronę gracza. Typy zachowań:
  - **Patrol:** chodzi po wyznaczonej trasie, reaguje po zauważeniu gracza.
  - **Strażnik:** pilnuje miejsca (skrzyni, klucza) i nie oddala się od niego.
  - **Zasadzka:** pojawia się, gdy gracz wejdzie w obszar albo otworzy skrzynię.
  - **Łowca z mgły:** pojawia się w mgle i ściga gracza. Liczba i zasięg rosną z poziomem mgły.
- Warunki pojawiania się (czas, poziom mgły, wejście w obszar, zdarzenie) ustawiane w ręcznie postawionych punktach spawnu.
- Mała liczba wrogów, więc każdy potrzebuje czytelnych ataków z zapowiedzią i roli w grupie, a elity własnej mechaniki.

## Zasoby
- Skrzynie z przedmiotami, obiekty z walutą (np. beczki).
- Zasoby z niszczenia elementów otoczenia (drzewa, kamienie). Niszczenie ma być szybkie (2–3 uderzenia).
- Różne zasoby z różnym zastosowaniem w sklepie, żeby wybór, co zbierać, zależał od planowanego buildu (propozycja).

## Kamera i ruch
- Kamera trzecioosobowa z możliwością obrotu dookoła postaci i lekkiej zmiany kąta.
- Kamera musi pokazywać gracza i teren wokół niego przez zasłaniające obiekty (np. przenikanie lub wycinanie geometrii).
- Potrzebne stałe parametry ruchu (wysokość skoku, podwójny skok, zasięg dasha, progi modyfikatorów), żeby projektować pod nie miejsca trudno dostępne.

## Na później
- Questy brane w sklepie (np. zabij konkretną elitę lub bossa).
- Meta-progresja między runami: do ustalenia na końcu.

## Otwarte kwestie
- Odległość i zakres kąta kamery (zza pleców czy wyżej, nad postacią).
- Parametry ruchu i progi modyfikatorów skoku.
- Lista zawartości mapy, a z niej docelowa skala map.
- Długość runu i liczba map.

## Proponowana kolejność prac
1. Kamera (obrót, kąt, widoczność przez obiekty) i parametry ruchu, testowane na małej scenie (most, platformy, punkt wysoko).
2. Lista zawartości mapy i skala.
3. Pętla jednej mapy: wejście, eksploracja, klucz, poziomy mgły, wyjście.
4. Sklep i przedmioty, potem questy i meta-progresja.
