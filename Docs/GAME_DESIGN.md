# Game Design – ustalenia

Stan: ustalenia z rozmowy projektowej (2026-10-17). Nic z tego nie jest jeszcze zaimplementowane.

## Gatunek i cel
- Hack and slash z elementami platformówki, w stylu roguelike (inspiracja: Megabonk).
- Mniej przeciwników niż w Megabonku, więcej myślenia i planowania.
- Gra przechodzona kilkaset razy różnymi buildami. Długość runu jak w innych roguelike'ach (Isaac, późniejsze etapy Hadesa): w wolnym tempie 1–1,5 h, przy najszybszym przejściu 15–20 min (klucz, wyjście, 4 mapy, boss).

## Struktura runu
- Seria map. Między mapami jest mapa ze sklepem (zakup ulepszeń), potem kolejna mapa.
- Na mapie sklepu kilku NPC, każdy z własnym sklepem (szczegóły później).
- Śmierć oznacza nowy run. Wyjątek: przedmiot dający drugie życie.
- Jedna mapa: 10–15 min, dłużej, jeśli gracz zostaje po rozprzestrzenieniu się mgły i poluje na trudniejszych przeciwników.
- Run to 4 mapy (do ok. 15 min każda) i główny boss na końcu. Na razie gra ma jeden etap.
- Na później, po ukończeniu pierwszego etapu: drugi etap i zapis postaci na jego początku, żeby po śmierci zaczynać od stanu z początku drugiego etapu (pomysł).
- Zapis automatyczny i bardzo częsty, nie tylko przy zmianie mapy czy instancji. Implementacja na sam koniec.
- Sklep może pokazywać podgląd następnej mapy (biom, modyfikator), żeby było pod co planować zakupy (propozycja).

## Mapy
- Układ terenu robiony ręcznie, zawartość losowana w ręcznie postawionych punktach (skrzynie, złoża, grupy wrogów, wyjście, eventy).
- Kilka map na etap, z których losuje się jedną, plus modyfikatory mapy (propozycja).
- Kilka poziomów w pionie: pod mostem (np. skrzynia), na moście (parkour po zawalonych elementach), ukryte miejsca wysoko (np. 100 m nad mostem).
- Dostęp do trudnych miejsc ogranicza trudność dotarcia (np. wysokość skoku z modyfikatorów), a nie brak konkretnego przedmiotu.
- Obecny Lvl_Landscape (2 km) służy tylko do testów.
- Skala mapy: obszar ze strukturami 550×550 m (sprawdzony na Lvl_ScaleTest, wystarczający).
- Wokół niego pas 200 m z każdej strony (cała mapa ok. 950×950 m) zajęty przez mgłę od początku: bez struktur i zawartości, tylko dekoracje (np. drzewa) i przeciwnicy pojawiający się, gdy gracz się zbliży.
  - Prędkość: chód 6 m/s, sprint 10 m/s, ale stamina (30, zużycie 25/s, regeneracja po 3 s, 20/s) daje średnio ok. 7 m/s w dłuższym biegu.
  - Szybkie przejście (ok. 3,5 min na mapę, połowa czasu na ruch): ok. 700 m drogi start → klucz w głównej strukturze → wyjście przy krawędzi; przy krętej trasie daje to promień ok. 250–300 m od środka.
  - Wolne przejście (15 min, połowa czasu na ruch, reszta na walkę i zbieranie): ok. 3 km drogi, co przy widoczności ok. 30 m w każdą stronę obejmuje mniej więcej połowę mapy. Gracz nie zbierze wszystkiego, co pasuje do założenia planowania.
  - Poziomy w pionie (pod mostem, na moście, wysoko) zwiększają powierzchnię do eksploracji bez wydłużania odległości. Jeśli test pokaże, że mapa jest za ciasna, powiększamy ją.
  - Gęstość przy ok. 0,3 km²: skrzynia średnio co ok. 130 m, beczki co ok. 60 m (w praktyce w grupach). Główna struktura ok. 100–150 m szerokości.

### Zawartość jednej mapy
- **Główna struktura:** zawsze jedna bardzo duża (np. zamek na środku mapy).
  - W jej centrum opcjonalny boss strzegący głównego skarbu.
  - 2–3 mniejsze instancje, każda z elitą i mniejszym skarbem (np. wieża, hall, loch).
- **Mniejsze struktury i znajdźki** rozłożone po reszcie mapy.
- **Złoża surowców:** losowo 1–3 złoża każdego z 3 rodzajów.
- **Skrzynie (osobne pule):** ok. 10 losowych, 2 gwarantowane rzadsze, 5 mgielnych.
- **Beczki i podobne obiekty:** 70–90; nie z każdej coś wypada.
- **Miejsca na klucz:** 5–6, zawsze w głównej strukturze lub w jej pobliżu.

## Wyjście z mapy
- Nieoznaczone na starcie, losowane z kilku możliwych miejsc.
- Jedno wyjście i jeden klucz na mapę.
- Do wyjścia potrzebny klucz albo zapłata równowartości w walucie (wyjście awaryjne kosztem zakupów w sklepie).
- Klucz nie może być szczególnie trudny do zdobycia. Losowany z 5–6 ręcznie postawionych miejsc w głównej strukturze lub w jej pobliżu.

## Mgła (presja czasu)
- Granica mapy zawęża się z czasem i jest oznaczona mgłą.
- Mgła nie zabija sama. Za nią pojawia się coraz więcej coraz silniejszych przeciwników.
- Kilka poziomów mgły. Na ostatnim przeciwnicy są śmiertelni, ale na tyle trudni, że zostaje tylko ucieczka albo śmierć.
- Mgła zawęża się w stronę wyjścia. To, że zdradza jego położenie, jest zamierzone (nagroda za uważność).
- Część obszarów jest spod mgły wyłączona: okolica wyjścia, struktury ponad mgłą.
- Gracz może cofnąć się w mgłę do niezbadanej części mapy na własne ryzyko.
- Do rozważenia: lepsze nagrody w obszarach, które mgła już zajęła; zagrożenie rosnące z głębokością i czasem spędzonym w mgle; ograniczenie przeczekiwania w miejscach ponad mgłą (latający lub strzelający wrogowie w późnych poziomach).
- Krawędź mgły musi być czytelna, czyli osobny efekt, a nie mgła odległości (ExponentialHeightFog).
- Wygląd: pastelowa, cukierkowa mgła przy ziemi (nie realistyczna). Pierwszy poziom delikatny, z każdym poziomem gęstsza; na ostatnim pełna, nieprzezroczysta do kolan, zakrywa podłoże, a trawa pod nią jest wyłączana. Gdy gracz stoi w mgle, brzegi ekranu lekko się zabarwiają.

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
- Obiekty z walutą (np. beczki); waluta też z przeciwników i skrzyń.
- Drzewa i kamienie są tylko dekoracją (bez niszczenia i zbierania), żeby gracz nie mylił się, co da się zebrać.
- Oprócz waluty są surowce. Mają dawać różnorodność i zmuszać do planowania runu: czasu na mapie nie starcza na zebranie wszystkiego, więc gracz wybiera, co zbiera, pod planowany build.
- Surowce muszą być wyraźnie odróżnialne od dekoracji.
- Pasuje do 3 surowców w UResourceComponent:
  - **Waluta (złoto):** beczki, wrogowie, skrzynie; jedyna cena przedmiotów w sklepie, zapłata za wyjście bez klucza.
  - **3 surowce jako świecące złoża w różnych kolorach** (rozbijane atakiem, wyraźnie inne niż kamienie), każdy przypisany do gałęzi ulepszeń umiejętności: czerwony: atak, niebieski: przetrwanie, zielony: ruch.
  - Przedmioty w sklepie nie wymagają surowców. Surowce służą tylko do ulepszeń umiejętności swojej gałęzi.
  - Przykłady ulepszeń: ruch: podwójny skok, dash; przetrwanie: blok tarczą.
  - Złoża w ręcznie postawionych punktach o różnej trudności dostępu (wysoko, pod mostem, w mgle); rzadsze surowce częściej w trudnych miejscach.
  - Podgląd następnej mapy w sklepie pokazuje, których surowców będzie więcej, a przedmiot zwiększający ilość surowca pozwala to zmienić.

## Skrzynie
- Ok. 99% skrzyń jest darmowych do otwarcia.
- Ok. 1% to skrzynie legendarne lub z questa: otwierają się dopiero po spełnieniu warunku (pokonanie przeciwnika, wykonanie zadania).
- Rarity: zwykła, rzadka, epicka, legendarna.
- Rodzaje:
  - **zwykła:** na mapie od początku,
  - **mgielna:** pojawia się dopiero, gdy dotrze do niej mgła; wyższe rarity jako nagroda za ryzyko,
  - **strzeżona:** pilnowana przez elitę lub bossa; co najmniej rzadka.

## Wpływ gracza na zawartość mapy
- Ulepszenie (w sklepie) zwiększające szansę na pojawienie się rzadszych skrzyń.
- Jednorazowy przedmiot zwiększający ilość wybranego surowca na następnej mapie kosztem pozostałych.

## Kamera i ruch
- Kamera trzecioosobowa z możliwością obrotu dookoła postaci i lekkiej zmiany kąta.
- Kamera musi pokazywać gracza i teren wokół niego przez zasłaniające obiekty (np. przenikanie lub wycinanie geometrii).
- Podstawowy ruch: sprint, slide (już w grze), skok.
- Podwójny skok i dash: ulepszenia ruchu kupowane za surowiec ruchu (zielony).
- Dash zastępuje slide.
- Wysokości struktur i trudno dostępnych miejsc nie są teraz priorytetem; na końcu można je dopasować, dodając półki i inne elementy otoczenia do wskakiwania.

## Na później
- Questy brane w sklepie (np. zabij konkretną elitę lub bossa).
- Meta-progresja między runami: do ustalenia na końcu.

## Otwarte kwestie
- Odległość i zakres kąta kamery (zza pleców czy wyżej, nad postacią).
- Sklep i ulepszenia umiejętności (na później): pełna lista ulepszeń w każdej gałęzi, podział między NPC.

## Proponowana kolejność prac
1. Kamera (obrót, kąt, widoczność przez obiekty): zrobione na Lvl_CameraTest.
2. Lista zawartości mapy i skala: ustalone.
3. Pętla jednej mapy: wejście, eksploracja, klucz, poziomy mgły, wyjście. W toku: klucz, wyjście (klucz lub opłata) i kurczenie mgły do wyjścia działają na Lvl_ScaleTest; efekt mgły zrobiony (do oceny w grze); brak jeszcze wrogów w mgle.
4. Sklep i przedmioty, potem questy i meta-progresja.
