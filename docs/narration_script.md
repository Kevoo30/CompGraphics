# Narration script (22 clips)

The loader plays `assets/narration/01.wav` … `22.wav`, one per shot, in the same order as the film timeline.
**Auto-fit:** each shot lasts at least its table length and grows to fit its clip plus about 2 seconds, so a longer recording never gets cut off. A missing file just means that shot is silent and keeps its table length.

The word counts below are sized for a calm documentary pace (about 2.3 words per second) against each shot's minimum length. Going over is fine; the shot simply stretches.

| Clip | Shot (film order) | Min. length | Narration |
|---:|---|---:|---|
| 01 | Title | 10 s | Kenya, 2022 to 2026. A country, its leaders, and the people who kept watching. |
| 02 | 9 Aug 2022, Kenyans vote | 15 s | On 9 August 2022, Kenyans queued at polling stations across the country to choose a president, along with governors, legislators and county representatives, in a general election. |
| 03 | 15 Aug 2022, result declared | 15 s | Six days later, at the Bomas of Kenya in Nairobi, the electoral commission declared William Ruto the winner of the presidential race, though the commission itself was divided over the result. |
| 04 | 5 Sep 2022, Supreme Court | 14 s | The result was challenged. On 5 September, the Supreme Court heard the petitions and unanimously upheld the election result. |
| 05 | 13 Sep 2022, swearing-in, Kasarani | 15 s | On 13 September, at the Moi International Sports Centre in Kasarani, before a packed stadium, William Ruto was sworn in as Kenya's fifth president. |
| 06 | 2023, cost of living | 14 s | Within months, the cost of living became the dominant story. Higher taxes on fuel and food, and rising prices, put real pressure on household budgets. |
| 07 | 7 Jul 2023, Saba Saba | 15 s | On 7 July 2023, Saba Saba day, opposition-led protesters took to the streets over the Finance Act and the rising cost of living. |
| 08 | Nov 2023, housing levy ruling | 14 s | In November 2023, the High Court issued a ruling on the housing levy, a reminder that the courts were now part of the argument over tax. |
| 09 | May 2024, Finance Bill introduced | 14 s | In 2024, the government brought a new Finance Bill to Parliament, with proposals to raise taxes. Many Kenyans, especially young people, were watching closely. |
| 10 | 20 Jun 2024, protests | 15 s | By 20 June, people were out across Nairobi and beyond, protesting the proposed tax rises, organising online and gathering in the streets. |
| 11 | 25 Jun 2024, bill passes | 18 s | On 25 June, as lawmakers voted to pass the Finance Bill, protesters reached Parliament itself. The day turned deadly, and the country was shaken by what it saw. |
| 12 | 26 Jun 2024, State House | 14 s | The next day, from State House, the president said he would not sign the bill, and the proposed tax rises were withdrawn. |
| 13 | 11 Jul 2024, cabinet dismissed | 14 s | On 11 July, the president dismissed most of his cabinet, in the next stage of the government's response to the protests. |
| 14 | 17 Oct 2024, deputy president removed | 15 s | In October, the Senate voted on the removal of the deputy president, an unusual moment for a government only two years into its term. |
| 15 | Jun 2025, death in custody | 16 s | In June 2025, the death of a man in police custody sparked fresh anger. Candles were lit, and people gathered again in Nairobi to demand answers. |
| 16 | 25 Jun and 7 Jul 2025, anniversary | 15 s | On 25 June and again on Saba Saba, 7 July, people marked the anniversary of the 2024 protests, linking the new demonstrations to the movement that came before. |
| 17 | 17 Oct 2025, state funeral | 16 s | In October 2025, Kenya mourned Raila Odinga, the long-time opposition leader. A state funeral was held at Nyayo National Stadium in Nairobi. |
| 18 | 19 Oct 2025, Bondo | 14 s | Two days later, he was laid to rest at his home in Bondo, in Siaya County, with military honours. |
| 19 | 18 Jun 2026, Finance Bill 2026 | 14 s | In June 2026, Parliament passed the Finance Bill 2026 after amendments, a debate once again watched closely by the public. |
| 20 | 25 Jun 2026, anniversary marches | 15 s | On 25 June, two years on, marches again marked the anniversary of the 2024 protests, and the memory of those who did not come home. |
| 21 | 10 Aug 2027, next election | 13 s | Kenya's next general election is expected in August 2027. Once more, the country will choose, and once more, the people will be watching. |
| 22 | Outro | 10 s | The people are watching. |

## Please verify before recording

These were written conservatively, but check each against a reliable source (news archive, Parliament record, Supreme Court judgment) before presenting:

- **03:** the commission being "divided" (some commissioners disowned the result) and the six-day gap (9 to 15 Aug).
- **04:** "unanimously" for the 5 September Supreme Court decision.
- **05:** "fifth president" (Kenyatta was the fourth).
- **08:** the exact November 2023 ruling. Say what the court actually decided if you want it more specific.
- **09:** the "May 2024" date for the Finance Bill 2024 being tabled; the lower third on screen uses this too. Change the date in `film.cpp` if it is off.
- **11:** the wording about deaths on 25 June 2024. Keep or soften according to your sources and your lecturer's guidance.
- **12:** "tax rises were withdrawn" (the bill was withdrawn after the president declined to sign).
- **14:** the 17 October 2024 date for the Senate vote.
- **15:** left unnamed on purpose. Name the person only if your sources are solid.
- **19, 20, 21:** these are on or after the dates of your own timeline. Confirm 18 June 2026 and the 2027 election date (the election is held on the second Tuesday of August).

## Recording notes

- Keep it dry and calm; add music and ambience separately (the film already plays `assets/music.wav` and generated ambience under the voice).
- Leave about half a second of silence at the start of each clip.
- Export as WAV, mono or stereo (any sample rate the loader accepts); name files `01.wav` … `22.wav`.
- If the timeline changes, renumber every clip after the changed shot.
