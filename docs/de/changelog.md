# Changelog 

Alle nennenswerten Änderungen an diesem Projekt werden in dieser Datei dokumentiert.

Das Format basiert auf [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
und dieses Projekt hält sich an die [Semantische Versionierung](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Hinzugefügt

- Neue Dokumentation (Deutsch und Englisch)
- Grundlegende SIP-Unterstützung
- Video Source Previews (derzeit nicht stabil)
- RTMP Live Streaming Unterstützung

### Geändert

- Verbessertes Docker-Image
- Native ffmpeg Audio- und Video MP4-Aufnahme
- Verbessertes Video Encode/Decode Handling


## v0.5.3-beta - 2023-08-15

### Hinzugefügt

- RTCP Statistiken
- Grundlegende Dockerfile und entrypoint.sh

### Geändert

- Neuer Login durch bestehendes Login-Token zulassen

## v0.5.2-beta - 2023-04-09

### Behoben

- Anzeigeverbesserung wer gerade spricht

## v0.5.1-beta - 2023-04-09

### Hinzugefügt

- Benutzerstatus im Vollbild ist nun sichtbar
- HTML5 Login-Validierung

### Behebt

- Sprecher-Status nach Logout

## v0.5.0-beta - 2023-04-07

### Hinzugefügt

- Nur Audio Aufnahme (ohne Video)
- Automatische Video-Skalierung
- Dauerhafte Sitzungen
