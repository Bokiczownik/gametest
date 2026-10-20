# Lekcje: animacje postaci z Blendera do UE (szkielet Quaternius, 2026-10-21)

Notatka robocza dla agenta. Przeczytaj przed kolejną pracą nad animacjami postaci, rigami z pobranych paczek
(Quaternius, Poly Pizza, glTF) albo importem skeletal mesh do UE.
Pliki: `Tools/Blender/skeleton_export.py`, `SourceArt/Characters/Skeleton/`, `/Game/Characters/Skeleton/`.

## Problemy i rozwiązania

### 1. Rozciągające się buty, biodra bez ruchu (wszystkie animacje w UE)
- **Objaw:** w Blenderze bieg wyglądał dobrze, w UE stopy stały na ziemi, a buty rozciągały się do kolan.
- **Przyczyna:** obiekt armatury z importu glTF miał skalę 100 i obrót 90°. W FBX węzeł armatury dostał skalę
  10000, a przesunięcia kości ok. 0,001. Kompresja ACL w UE wyzerowała te przesunięcia, a rotacje zostały.
  Stopy Quaterniusa są dziećmi `Root` (nie podudzia), więc bez przesunięć zostają w pozie spoczynkowej.
- **Wykrycie:** w FBX węzły `Model` mają `Lcl Scaling` (parsowanie przez `io_scene_fbx.parse_fbx`). Zrzut z UE:
  tymczasowy SkeletalMeshActor z `AnimationSingleNode` i `AnimationData.SavedPosition`.
- **Rozwiązanie:** eksport z kopii armatury z zastosowanym obrotem i skalą (`transform_apply`), a klucze
  `location` pomnożone przez tę skalę. Robi to `skeleton_export.py`.
- **Na przyszłość:** przed eksportem dowolnego rigu sprawdź `matrix_world` armatury. Skala różna od 1 oznacza,
  że trzeba ją zastosować i przeskalować klucze.

### 2. Niekluczowane kości w animacjach Quaterniusa
- **Objaw:** palce prostowały się przy zmianie animacji, ręce w Idle ustawiały się dziwnie.
- **Przyczyna:** każda animacja kluczuje tylko część kości (np. Idle nie ma UpperArm ani palców). W glTF
  takie kości biorą domyślną pozę węzła. Blender zostawia w nich to, co było ustawione wcześniej, i tak
  trafia to do bake'u FBX.
- **Rozwiązanie:** przed każdą klatką bake'u ustaw wszystkie kości na pozę domyślną glTF. Pozę czyta się
  z kopii .glb z wyciętym `animations` (import i odczyt basis kości), potem nakłada się akcję.
- **Na przyszłość:** nie zakładaj, że akcja kluczuje wszystkie kości (sprawdź fcurves). Nie resetuj pozy do
  identity, bo to T-pose, a nie poza domyślna.

### 3. Broń przypięta do kości (parent bone) rozjeżdża się w UE
- **Objaw:** miecz odstawał od dłoni. Import pokazywał ostrzeżenie „brakuje kości Weapon_Dagger w pozie
  powiązania”.
- **Rozwiązanie:** przy eksporcie kopia broni dołączana do mesha ciała z grupą wierzchołków kości dłoni
  (waga 1). Druga opcja to osobny static mesh na sockecie.
- **Inny błąd przy okazji:** relacja broń–kość z oryginału miała przesunięcie 0,46 m. Broń przypiąłem od nowa,
  licząc macierz z pozycji dłoni (środek rękojeści = origin broni, ostrze = lokalne -Z).

### 4. Nienaturalny kąt miecza
- **Przyczyna:** ostrze celowane minimalnym obrotem dłoni wychodziło prawie w osi przedramienia, czego
  nadgarstek nie potrafi.
- **Rozwiązanie (funkcja `grip`):**
  1. ustaw kierunek przedramienia;
  2. dłoń neutralna (kości `Index1/Middle1/Pinky1/Thumb1.R` w rest);
  3. obróć przedramię wokół jego osi, żeby ostrze leżało w płaszczyźnie (przedramię, cel);
  4. odchyl nadgarstek, z kątem ostrze–przedramię w zakresie 50–110°.
- **Szczegóły rigu:** „dłoń” to 4 korzenie palców obracane razem wokół nadgarstka (nie ma kości dłoni).
  Pięść to stałe lokalne rotacje segmentów 2–3 palców i Thumb2, policzone raz z rest.

### 5. Przeskok miecza przy przejściu bieg → atak
- **Przyczyna:** atak zaczynał się od pozy Idle (miecz nisko z przodu), a bieg miał miecz w górze.
- **Rozwiązanie:** wspólna poza gotowości (lokalne wartości ramienia względem tułowia) nałożona na Idle i Run
  (Sword_IdleReady, Sword_RunReady). Atak zaczyna się i kończy w tej pozie.
- **Na przyszłość:** pierwsza i ostatnia klatka każdego montażu muszą pasować do pozy lokomocji, z której
  i do której on blenduje.

### 6. Skala postaci
- **Pomyłka:** dobrałem skalę 1,35 pod kapsułę gracza (1,8 m), a szkielet miał być niższy od gracza.
- **Rozwiązanie:** mesh 0,7 (ok. 0,95 m), kapsuła 30/50.
- **Na przyszłość:** zapytaj o docelowy wzrost względem gracza albo pokaż porównanie obok gracza przed
  dalszą pracą.

### 7. AI nie atakowało po pierwszym trafieniu
- **Przyczyna:** `MoveToActor` zatrzymuje się w odległości 0,6 × zasięg + promienie obu kapsuł. Przy krótkim
  zasięgu stanie poza zasięgiem ataku.
- **Warunek:** 0,4 × (Reach+Radius) > promień własnej kapsuły + 42.
- **Druga przyczyna:** postawiony aktor miał własną kopię starej tablicy `Attacks`. Zmiana wartości w BP nie
  nadpisuje instancji: trzeba ustawić instancję albo postawić aktora od nowa.

### 8. Pułapki MCP i edytora
- Crash edytora zniszczył niezapisany mesh, bo zmieniałem nazwy zasobów przed zapisem mesha. Po każdym
  imporcie od razu `save_assets`.
- ABP przez MCP:
  - `BlueprintTools.create(asset_type=/Script/Engine.AnimInstance)` tworzy ABP bez TargetSkeleton;
  - działa ogólny sequence player `Animacja|Sekwencje|Odtwarzaczsekwencji.` + `Node.sequence`;
  - TwoWayBlend to `Animacja|Mieszania|Mieszaniedwukierunkowe`.
- Usunięcie zasobu zeruje referencje do niego (BP, instancje). `HealthComponent.DeathAnimation` nie da się
  ustawić na instancji, więc aktora trzeba postawić od nowa.
- Tablicę struktur skraca się dwoma wywołaniami `set_properties`: najpierw zmiana elementów przy tym samym
  rozmiarze, potem krótsza tablica.
- Okno Message Log po imporcie FBX zasłania PIE w `CaptureEditorImage`, trzeba poprosić użytkownika o
  zamknięcie. Edytor w tle chodzi z ok. 3 fps, więc szybkich klatek ataku nie da się złapać zrzutami.
  Pozy sprawdza się w edytorze (punkt 1).
- Blender: importer FBX przesuwa akcje o +1 klatkę. Przy porównaniu reimportu porównuj klatkę f z f+1.

## Kolejność pracy na przyszłość
1. Sprawdź transformację armatury, hierarchię (np. stopy pod Root), kluczowane kości w akcjach i sposób
   przypięcia broni.
2. Buduj pozy na pozie domyślnej glTF. Montaże zaczynaj i kończ w pozie lokomocji.
3. Eksportuj przez `skeleton_export.py` (albo jego kopię dla innej postaci).
4. Sprawdź FBX: `Lcl Scaling` armatury ok. 1 i reimport do Blendera zgodny ze źródłem.
5. Import do UE, `save_assets` od razu. Kontrola póz przez SingleNode + SavedPosition, potem PIE.
