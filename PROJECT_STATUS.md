# Project status

Last updated: 11 November 2026.

## Where the project is

This is a design and architecture stage. Gameplay implementation has not started.

The game is a 5 to 8 player first-person murder mystery in Unreal Engine 5. The rules are in [GDD.md](GDD.md), version 0.4. The proposed code layout is in [ARCHITECTURE.md](ARCHITECTURE.md). Agent constraints are in [CURSOR_RULES.md](CURSOR_RULES.md).

## What exists

- `GDD.md` — locked design, including the vote, the hunt, agendas, rats, and disconnects.
- `ARCHITECTURE.md` — proposed Unreal class split, networking, and build order. Not approved yet.
- `PROJECT_STATUS.md` — this file.
- `CURSOR_RULES.md` — instructions for Cursor agents working in this repo.
- `MurderAtMidnight/MurderAtMidnight.uproject` — Unreal Engine 5.8, module `MurderAtMidnight`.
- The first-person template classes: `AMurderAtMidnightGameMode`, `AMurderAtMidnightPlayerController`, and `AMurderAtMidnightCharacter`.
- The template Horror and Shooter variants, including weapons and AI. These are samples, not the match.
- A default map, `Lvl_FirstPerson`, and a Blueprint game mode, `BP_FirstPersonGameMode`.
- A git repository on `main` with no commits yet.

## What does not exist

- No custom GameInstance, GameState, or PlayerState.
- No networked session. The build file leaves the online subsystem commented out.
- No interaction, inventory, agendas, roles, vote, hunt, rats, or voice.
- No manor map. The open level is the template range.

The class names in the architecture document are the match classes still to add. The first-person character, controller, and game mode are already in the project.

## Decisions already made

- The server decides murders, agendas, votes, the clock, and who is alive.
- Secrets go to the owning player only. Public actors do not carry the killer id, the target list, or agenda text.
- The GameMode holds match rules and secrets. The GameState holds public match facts. The PlayerController is the private mailbox.
- C++ holds the rules. Blueprint and data assets hold tuning, lines, meshes, and agenda content.
- There is no Mischief meter and no reputation score.
- A listen server is acceptable for early tests. It is not a real privacy boundary for the hosting player. A dedicated server is the later fix.

## Waiting on approval

The architecture in `ARCHITECTURE.md` needs a yes before any Unreal project or classes are created.

After that yes, the first implementation step stays inside the existing project. Add the missing match classes onto the first-person base. Do not replace the project, and do not build killer assignment, murder, agendas, the vote, the hunt, or the rat in that step.

## Known risks

- A listen-server host can inspect server memory and see every secret.
- A replicated variable on PlayerState or an actor will be sent to every client if its condition is left at the default.
- Blueprint UI that reads the GameMode will fail on a real client, and can leak secrets in the editor.
- Proximity voice is not built into Unreal. It needs a plugin choice before that part of the first playable test.

## Next step

Approve or correct `ARCHITECTURE.md`. Then add the missing match classes inside the existing Unreal project. Do not create a second project.
