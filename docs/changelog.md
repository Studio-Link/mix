# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).


## v1.0.0-beta - 2025-05-31

### ⁂ Fediverse Login

It is now possible to log in with a Fediverse login (e.g. Mastodon).

<video width="720" class="aspect-video mt-2" controls>
 <source src="/social_login.mp4" type="video/mp4">
</video>


### 🎥 Improved video layouts

The new video layouts try to make better use of the available space:

![screenshot new video layouts](/vidconv.drawio.png)

<div class="text-center">(N = number of participants)</div>

### 👤 Solo button

A new **solo button** allows you to highlight individual participants - ideal for moderation and live editing.

![screenshot solo button](/solo_button.png)

### 🪞 Mirror image (Selfview)

Your own camera view is now displayed with less delay and mirrored.

### 🎛️ Camera settings for login avatar snapshots

It is now possible to select a specific camera for avatar creation (login).

### Emoticon reactions

There is now a possibility to react with emoticons:

<video width="720" class="aspect-video mt-2" controls>
 <source src="/emoticons.mp4" type="video/mp4">
</video>


### 📚 Documentation

At https://mix.studio.link/hosted/started you will now find instructions for the hosted version and also for self-hosting: https://mix.studio.link/self-hosting/install-intro

### 📶 Poor connections - optimised

Stability with fluctuating network quality has been further improved, especially for video connections.

### 🛠️ Further technical innovations & internal refactorings

- Upgrade to **TailwindCSS 4** in the web interface
- New **Content-Security-Policy** for better security
- Docker image (self-hosting)
- Refactorings in **API**, **WebRTC**, **HTTP routing** and much more.

## v0.6.0-beta - 2024-04-10

### Added/Changed

- New documentation (German and English)
- Basic SIP support
- Improved docker image
- Native ffmpeg audio and video mp4 recording
- Improved video encode/decode handling

## v0.5.3-beta - 2023-08-15

### Added

- Add rtcp statistics
- Basic Dockerfile and entrypoint.sh

### Changed

- Allow re-auth by login token

## v0.5.2-beta - 2023-04-09

### Fixed

- Speaker state initialization

## v0.5.1-beta - 2023-04-09

### Added

- Fullscreen user status
- HTML5 Login validation

### Fixes

- Speaker state after logout

## v0.5.0-beta - 2023-04-07

### Added

- Audio only recording option
- Auto video scaling
- Persistent sessions



