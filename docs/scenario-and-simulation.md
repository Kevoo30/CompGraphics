# Scenario and crowd simulation

The film reads `assets/scenario.json` when it starts. Its `events` array is the event timeline: array order controls shot order, and each row controls its event identifier, location, on-screen place/date/headline, event type, modeled crowd size, minimum shot duration, random seed, and optional polling or barrier settings. The narration clip can extend a shot beyond its configured duration.

All turnout and voter counts currently in the sample file are **illustrative simulation values**. They are not an official count or a claim about any district. Replace them with verified, sourced figures before presenting them as data.

## Event fields

- `id`: stable name for the event.
- `location`: `court`, `bomas`, `street`, `parliament`, `state_house`, `stadium`, `homestead`, or `ballot`.
- `place`, `date`, `headline`, `year_card`: documentary lower-third and year card text.
- `type`: `voting`, `protest`, or `audience`.
- `crowd_size`: number of simulated agents.
- `duration`: minimum event duration in seconds.
- `seed`: deterministic variation seed.
- `stations`: number of parallel polling queues for voting events.
- `turnout`: fallback turnout from 0 to 1.
- `districts`: optional voting districts with `name`, `turnout`, `expected_voters`, and normalized map coordinates (`map_x`, `map_y`).
- `barriers`: optional protest obstacles with world coordinates `x`, `y`, and a positive `radius`.

Voting agents approach a station, wait in a line, cast a ballot, then leave. Long waits can exceed an agent's patience. Their simulated casts feed the on-screen tally and district schematic; the displayed figures are model output and should not be interpreted as real-time election results.

Audience agents are present at the start of a hearing, announcement, or ceremony. They stand facing the scene and make small idle movements. Protest agents arrive over the first few seconds, gather around a rally point, maintain personal space through steering forces, and disperse later in the event. Barriers push agents away from restricted areas. Demonstrations are peaceful and use fictional crowd agents.

## Rendering approach and limits

The project uses legacy fixed-function OpenGL, so the new scenes keep that renderer rather than introducing a shader pipeline. Directional lighting, fog, layered translucent ground shadows, warm lamp glows, grain, and camera motion add atmosphere. These are stylized approximations: the renderer does not provide physically based materials or depth-of-field post-processing.

## Editing the sample

Edit an event row in `assets/scenario.json`; no rebuild is needed. Event order controls the narration and shot sequence, so keep the number of rows aligned with the narration files you want to use. Values are validated when loaded; malformed JSON or out-of-range turnout values cause the film to fall back to its built-in crowd settings.

## Tests

The CMake test targets cover the existing transform checks and the agent, queue, and scenario loader behavior. Build the CMake project and run its discovered CTest tests from VS Code.

## Video export

Install an FFmpeg build with H.264 encoding available on `PATH`, then run:

```text
people_are_watching.exe --export-video out.mp4
```

The exporter renders a hidden SDL/OpenGL context at 1920x1080 and a fixed 30 frames per second, then sends RGB frames to FFmpeg. Narration WAVs are delayed to their timeline positions and mixed with `assets/music.wav` when present. The CMake `export_video` target writes `people_are_watching.mp4` in the build directory. This uses a hidden window and still needs an OpenGL driver; it is not a GPU independent software renderer.
