# Android Amp (mobile-test)

Testbau: Gitarre über USB-Audio-Interface ins Handy, ein NAM-Modell als Verstärker, Gain und Lautstärke am Touchscreen.

## Stand

- [x] Gradle-/CMake-Grundgerüst
- [x] Oboe-Anbindung (Duplex, Input → Verarbeitung → Output) als Stub
- [x] Kotlin-UI mit Start/Stop, Gain, Lautstärke
- [ ] NAM-Engine einbinden (`third_party/nam-core`, Lizenz vorher prüfen)
- [ ] Modellwahl und Laden von `.nam`-Dateien
- [ ] Latenz und CPU-Last messen
- [ ] Build auf echtem Gerät testen

## Bauen

Android Studio öffnen, Gradle-Wrapper wird beim ersten Sync erzeugt. Benötigt NDK und CMake über den SDK-Manager.

## Hinweise

- Oboe wird per CMake `FetchContent` geladen. Version in `app/src/main/cpp/CMakeLists.txt` prüfen.
- Nur `arm64-v8a`, damit der Build klein bleibt.
- Der Code ist noch nicht kompiliert worden.
