# Phase A Publish Checklist (Desktop)

## Release Artifact

- Archive: AURA-ESISA.zip
- Size: ~18.23 MB
- SHA256: 33318C54FBC85ADBFBE5ECAA9462D208D262985119A2A612064785ED63371BEB

## Recommended GitHub Release Naming

- Tag: v1.0.0
- Release title: AURA ESISA Desktop v1.0.0

## Where to Publish

- GitHub Releases (recommended)
- Itch.io (optional mirror)

## GitHub Release Steps

1. Open your repository on GitHub.
2. Go to Releases -> Draft a new release.
3. Set tag to v1.0.0.
4. Set title to AURA ESISA Desktop v1.0.0.
5. Upload AURA-ESISA.zip.
6. Paste the release notes from the next section.
7. Publish release.

## Release Notes (Copy/Paste)

AURA ESISA Desktop v1.0.0

Windows desktop release of the ESISA interview simulator.

### Included
- Desktop application (AURA.exe)
- Launcher scripts (AURA.bat, Lancer_AURA.bat)
- Runtime dependencies (DLL files + cacert.pem)
- Assets, config, cache, and data folders

### Quick Start
1. Download and extract AURA-ESISA.zip.
2. Open the extracted folder.
3. Double-click AURA.bat.
4. Configure API key in config/aura.cfg:
   - groq_api_key=YOUR_KEY

### Debug
- Run AURA.bat /debug to show startup errors in console.

### Integrity
- SHA256: 33318C54FBC85ADBFBE5ECAA9462D208D262985119A2A612064785ED63371BEB

## Post-Publish Validation

1. Download the release from GitHub as a user.
2. Extract to a clean folder.
3. Run AURA.bat.
4. Confirm login window appears.
5. Confirm config/aura.cfg can be edited for API key setup.
