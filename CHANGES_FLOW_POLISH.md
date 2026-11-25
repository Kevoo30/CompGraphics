# What changed (flow polish + realism)

## Compared with Midnight-Facet
Midnight-Facet is true 3D (perspective cameras, lit interiors, textured floors, articulated characters). Our film was flat 2D
backdrops, so nothing could be shown *inside* a court or chamber. This update adds a 3D layer in the same OpenGL/SDL2 setup.

## New: src/interior3d.h / interior3d.cpp  (3D scenes, no new dependencies)
- COURT: Supreme Court bench with seven robed judges, clerks, advocates at lecterns, public gallery, coat of arms, flags, window light.
- CHAMBER: Parliament floor with facing green benches, Speaker's chair, the Mace, public galleries; members stand to vote (Params.vote).
- POLLING: four streams of voters queueing through classroom doors, clerks with tablets and registers, booths, six ballot boxes
  with papers dropping in, party agents, an inked finger being shown.
- INAUGURATION: Kasarani stands with spectators, the dais, Chief Justice and president-elect (right hand raised, left on the book),
  honour guard marching, gun-salute smoke. With `mourning` it becomes the funeral: flag-draped coffin, black-clad crowd, half-mast flag.
- PROTEST_STREET / PROTEST_PARLIAMENT: marching crowd with placards, fists and phones, a banner, burning-tyre barricades,
  police line with shields and an armoured vehicle, tear-gas canisters and drifting smoke, vigil with candles at night. Handheld camera.
- STATE_HOUSE: podium and press pen with flashes (variant 1 = results declaration at the Bomas).
Everyone is a generic stylised figure; no real person is depicted. Violence is not shown.

## film.cpp
- Each shot opens on its 2D exterior, then cuts inside (see `buildInside()`; fractions say when). A short dip to black hides each cut.
- `trans` column: 0 whip-pan, 1 dip to black. Lower thirds slide in, vignette, film grain, handheld shake from `energy`.
- Voting shots show an "illustrative turnout" panel. Crowd sizes, stations and barricades come from assets/events.json (illustrative values).

## locations.cpp (2D exteriors)
- Articulated 2D people (walk cycle, arms, hair, shadows), marching crowds, cloth banners, police line, tear-gas haze, drifting clouds and birds.

## Debug
`people_are_watching --s3d <scene 1-7> <u 0..1> [seconds] [crowd] [smoke police night candles mourning vote results]` saves s3d.bmp.

# Update: consistency + glitch pass

## Coat of arms / guards
- One shared `kenyaShield()` (black top, red middle, green bottom, white fimbriation, cockerel with axe) now draws the wall coat of arms
  AND the chest badge of every guard / police officer, so it is identical in every scene. Wall emblem also has spears, two lions, scroll, Mount Kenya.
- Flicker while the camera turned was z-fighting. Fixes: explicit 24-bit depth buffer (render.cpp), near plane 0.1 -> 0.25, every emblem layer is a
  flat plate with a real gap + polygon offset, the flag is one continuous cloth mesh (no overlapping boxes), floor textures are mipmapped.

## President and faces
- President is one fixed look (`presidentLook()`): dark skin, navy suit, short black hair, in every scene. Chief Justice and Speaker are fixed looks too.
- All skin tones (3D, 2D, crowd) are now one dark-brown family; the light tones that made people jump between scenes are gone.
- Every face has the same layout (eyes, brows, nose, mouth, ears) drawn on top of the hair.

## Time of day
- `Params::sky` is filled from each shot's `Mood::sky` in `film.cpp`. `tod()` in interior3d.cpp drives sky, sun, ambient, window light, light shafts,
  street lamps, floodlights, ceiling lamps and building brightness in all 3D scenes (dawn, day, midday, dusk, night, pre-dawn).
- Debug: `--s3d <scene> <u> <t> <crowd> sky=4` previews a time of day.

## Flags + Parliament (follow-up)
- Inside flags (court, chamber, state house, stadium) are now an exact copy of the outside flag: same 74:46 proportions, same band heights, red shield with white
  outline, crossed white spears, gold ball on the pole, same colours.
- Parliament chamber is a full house: every seat on both sides is filled (5 rows x 18 per side = 180 members) regardless of the crowd value.
