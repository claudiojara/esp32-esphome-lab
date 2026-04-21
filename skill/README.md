# esp32-homeassistant skill (mirror)

Mirror of the global agent skill at `~/.claude/skills/esp32-homeassistant/`.

Committed to this repo so the knowledge travels with the code — anyone cloning
this repository (or Claude on a different machine) gets the same guidance that
was used to build it.

## Source of truth

The **global skill** (`~/.claude/skills/esp32-homeassistant/`) is the authoritative
version. This mirror is a snapshot, captured when the repo is updated.

## Keeping in sync

To refresh this mirror from the global skill:

```bash
cp -R ~/.claude/skills/esp32-homeassistant/. ./skill/
```

To push local improvements back to the global skill:

```bash
cp -R ./skill/. ~/.claude/skills/esp32-homeassistant/
```

Drift is possible — if you edit both sides, reconcile manually.
