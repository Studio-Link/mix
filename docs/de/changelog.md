# Changelog 

Alle nennenswerten Änderungen an diesem Projekt werden in dieser Datei dokumentiert.

Das Format basiert auf [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
und dieses Projekt hält sich an die [Semantische Versionierung](https://semver.org/spec/v2.0.0.html).

## v1.0.0-beta - 2025-05-29

### 🎥 Verbesserte Video-Layouts

[BILDER]

### 👤 Solo-Button

Ein neuer **Solo-Button** erlaubt es, gezielt einzelne Teilnehmer\*innen hervorzuheben – ideal für Moderation und Live-Schnitt.

![[Pasted image 20250516152940.png]]

### 🪞 Spiegelbild (Selfview)

Die eigene Kameraansicht wird nun mit weniger Verzögerung und gespiegelt angezeigt.

### 🎛️ Kameraeinstellungen für Login-Avatar-Snapshots

Es ist jetzt möglich gezielt eine Kamera für die Avatar Erstellung (Login) auszuwählen.

### 📚 Dokumentation

Unter https://mix.studio.link/hosted/started findet sich nun eine Anleitung für die Hosted Version und auch zum selber Hosten: https://mix.studio.link/self-hosting/install-intro

### 📶 Schlechten Verbindungen - optimiert

Die Stabilität bei schwankender Netzqualität wurde weiter verbessert, insbesondere für Videoverbindungen.

### 🛠️ Weitere Technische Neuerungen & Interne Refactorings

- Upgrade auf **TailwindCSS 4** im Webinterface
- Neue **Content-Security-Policy** für bessere Sicherheit
- Docker Image (Self-Hosting)
- Refactorings in **API**, **WebRTC**, **HTTP-Routing** u. v. m.


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
