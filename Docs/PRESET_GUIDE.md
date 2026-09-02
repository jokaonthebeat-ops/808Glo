# Preset System

## Factory bank

`Resources/FactoryPresets.json` contains 128 full-state factory presets. CMake embeds the JSON through `juce_add_binary_data`; installation does not require a separate factory-content folder.

Every preset includes:

- `schemaVersion`
- unique `name`
- separate `category`
- purpose-driven `description`
- searchable `tags`
- `author`
- every one of the 31 raw parameter values

Categories are kept separate from names in the browser. A sound appears as `Pure Current` inside `Clean & Sub`, not `Clean & Sub - Pure Current`.

## Regenerating and validating

The generator is deterministic:

```bash
python3 tools/generate_factory_presets.py
python3 tools/validate_presets.py Resources/FactoryPresets.json
```

The validator rejects:

- any count other than 128
- missing or extra parameter IDs
- invalid ranges, enums, booleans, names, or categories
- duplicate names and descriptions
- category behavior that contradicts its label
- presets that are only output/tuning variants
- insufficient distance between neighboring sounds

## User presets

The editor can save and load `.808glo` files. They are readable JSON and contain raw APVTS values plus metadata. The default location is:

- macOS: the user's Application Support folder under `Diamond Loopz/808Glo Pro/Presets`
- Windows: the user's AppData folder under `Diamond Loopz/808Glo Pro/Presets`

Factory presets remain locked. Saving a changed factory sound creates a user preset rather than overwriting factory content.

## Curation pass before release

The numerical bank is validated, but final release curation must still be done by ear in the native plugin build:

1. Audition notes 24–60 at velocities 64, 100, and 127.
2. Loudness-match neighboring presets.
3. Check mono, monitors, headphones, phone simulation, and a 100 Hz high-pass.
4. Verify Detroit presets remain short/mid-forward and Drill presets slide correctly.
5. Confirm Mix Ready presets translate without excessive limiting.
6. Adjust only the JSON source, regenerate, validate, and rebuild—never hand-edit compiled binary data.

