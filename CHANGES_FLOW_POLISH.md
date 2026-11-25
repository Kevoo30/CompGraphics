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
