# Project status

Last updated: 11 November 2026.

## Where the project is

Step 5 is in the Unreal project. The server assigns one killer and a private target list, and it is the only machine that can accept a kill. A kill does not choose a winner, call a vote, or create a rat.

The game is a 5 to 8 player first-person murder mystery in Unreal Engine 5. The rules are in [GDD.md](GDD.md), version 0.4. The proposed code layout is in [ARCHITECTURE.md](ARCHITECTURE.md). Agent constraints are in [CURSOR_RULES.md](CURSOR_RULES.md).

## What exists

- `GDD.md` — locked design, including the vote, the hunt, agendas, rats, and disconnects.
- `ARCHITECTURE.md` — Unreal class split, networking, and build order. Step 1 is approved and added.
- `PROJECT_STATUS.md` — this file.
- `CURSOR_RULES.md` — instructions for Cursor agents working in this repo.
- `MurderAtMidnight/MurderAtMidnight.uproject` — Unreal Engine 5.8, module `MurderAtMidnight`.
- Match shells: `AMurderAtMidnightGameMode`, `AMurderAtMidnightGameState`, `AMurderAtMidnightPlayerState`, `UMurderAtMidnightGameInstance`, `AMurderAtMidnightPlayerController`, and `AMurderAtMidnightCharacter`.
- `UMurderAtMidnightInventoryComponent` on the character. The carried item id replicates to every player. Only the server writes it.
- `UMurderAtMidnightInteractableComponent` on `AMurderAtMidnightKey` and `AMurderAtMidnightCabinet`. Whether the key is carried, who holds it, and whether the cabinet is open replicate to every player. The server owns those actors. Picking one up does not give the client ownership.
- The server spawns one gold key and one cabinet in front of the player start when the map does not already have them. F asks to use the thing you are looking at. G asks to drop the key. A refusal is a client message to the player who asked.
- Each guest has one private agenda: find the key, unlock the cabinet, then lock it again. The server records that sequence for that guest only. When it is done, that guest's agenda line ends with " - Completed". The killer has no agenda. The cabinet state shown on screen is public: isopen or isclosed. Opening and locking both require the key.
- One player is the killer. The target list is stored on the GameMode and sent only to the killer. A lobby of 2 to 4 players has one target. Five or six players have two. Seven or eight have three. Guests are not told whether they are targets. Public names are Guest 1, Guest 2, and so on.
- K asks the server to kill the player in front of you, within about two metres. The server accepts it only from the living killer, and only against a living player. A target is removed from the killer's private list. Anyone else can be killed, and the list does not change. Every client sees that the person is dead. The body does not store who did it. The victim is told they are dead, not who attacked them.
- The template Horror and Shooter variants, including weapons and AI. These are samples, not the match.
- A default map, `Lvl_FirstPerson`, and a Blueprint game mode, `BP_FirstPersonGameMode`.
- A git repository on `main` with no commits yet.

## What does not exist

- No online subsystem session. Joining uses a direct IP address.
- No vote, hunt, rats, or voice. Agendas are delivered, and they are not checked off. A kill does not end the match.
- No code that writes a match result.
- No manor map. The open level is the template range.

## Decisions already made

- The server decides murders, agendas, votes, the clock, and who is alive.
- Secrets go to the owning player only. Public actors do not carry the killer id, the target list, or agenda text.
- The GameMode holds match rules and secrets. The GameState holds public match facts. The PlayerController is the private mailbox.
- C++ holds the rules. Blueprint and data assets hold tuning, lines, meshes, and agenda content.
- There is no Mischief meter and no reputation score.
- A listen server is acceptable for early tests. It is not a real privacy boundary for the hosting player. A dedicated server is the later fix.

## Step 2

`AMurderAtMidnightGameMode` starts at Arrival and moves through Dinner, Garden, and Resolution on a timer. Each phase length is a property on the game mode. The ending phase does not pick a winner. The phase widget draws the phase and the seconds left from the replicated clock. A ninth player is refused. `LocalMapOptions=?listen` makes a standalone launch a listen server. The console commands are `HostMatch` and `JoinMatch 127.0.0.1`.

## Known risks

- A listen-server host can inspect server memory and see every secret.
- A replicated variable on PlayerState or an actor will be sent to every client if its condition is left at the default.
- Blueprint UI that reads the GameMode will fail on a real client, and can leak secrets in the editor.
- Proximity voice is not built into Unreal. It needs a plugin choice before that part of the first playable test.

## Step 3

F looks at a gold key, another player, or the dark cabinet. G drops the key. The server trace is about three metres. Empty hands, a locked cabinet, a second unlock, and a give with nothing in hand are refused. The key stays in the inventory after the cabinet opens. Opening it does not remove the key.

The refusal text is sent only to the player who pressed the key. The carried item, the holder, and the open cabinet are public.

## Next step

A door and one clue. The door uses the same lock check as the cabinet. The clue stores what changed, not who did it.
