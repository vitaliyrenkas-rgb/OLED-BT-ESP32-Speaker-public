# Public Release Cleanup Commands

This overlay is intentionally provided as full replacement files, not a patch, so old private values are not repeated in diff hunks.

Recommended workflow from the repository root:

```bash
unzip -o oled_bt_speaker_public_release_overlay.zip

git rm -f *Speaker_HARD*_LoLin32_MicroPython.ino
git rm -f archive/original/*Speaker_HARD*_LoLin32_MicroPython.ino

git add -A README.md CHANGELOG.md RELEASE_NOTES.md PUBLIC_RELEASE_CLEANUP_COMMANDS.md \
  OLED_BT_Speaker_LoLin.ino archive/original/OLED_BT_Speaker_original_monolithic.ino \
  docs src

git grep -n "YOUR_WIFI_PASSWORD\|YOUR_OPENWEATHER_API_KEY\|OLED-SETUP" -- src README.md RELEASE_NOTES.md

git status --short
git commit -m "Prepare public release docs"
git push origin feature/v3.5-config-portal
```

If this repository is going public with full Git history, remember that removing secrets from HEAD is not the same as removing them from old commits. Rotate any credential that has ever been committed, or publish a clean exported repository without private history.
