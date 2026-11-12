# Game Design Document

Version 0.5 — Manor Murder Mystery

This version keeps the locked rules from 0.4 and adds the host's compulsory assignment. Meters, reputation scores, and the old open questions stay out.

## 1. Game Overview

### 1.1 Concept

A first-person multiplayer murder mystery, set during an evening gathering at an eccentric, wealthy manor.

Players arrive as invited guests. They expect entertainment, conversation, and activities hosted by the owner. One guest is secretly the killer. The killer has a private list of targets and must eliminate those people before the evening ends.

The other guests have their own private goals. They also watch for suspicious behaviour, protect people who seem to be in danger, and try to work out who the killer is.

The evening is built from a few simple activities:

- Talking, lying, and making deals.
- Exploring the manor and handling objects.
- Private personal agendas.
- Scheduled gatherings.
- Murders that take time and can be seen.
- A rat, for each guest who dies.

The manor is part of the game. Its rooms, objects, secrets, and gatherings create the situations. It is not only a backdrop.

### 1.2 Design Philosophy

Memorable evenings should come from simple rules bumping into each other.

Players make the stories by:

- Hiding and sharing information.
- Lying about what they are doing.
- Stealing, hiding, and moving objects.
- Making promises, and breaking them.
- Watching what other people do.
- Chasing their own goals.
- Helping the killer, or getting in the way.

Each match should feel like its own evening, the kind of night people retell afterwards.

### 1.3 Tone

Dark comedy. Suspense. British eccentricity. The absurdity of a very rich house.

The game should be entertaining, and sometimes ridiculous. The murder mystery should still feel tense.

### 1.4 Player Count

Minimum: 5 players.

Maximum: 8 players.

One player is the killer. Everyone else is a guest.

## 2. The Evening

### 2.1 Schedule

The match runs from 6 PM to midnight, in four periods.

| Time | Event | Purpose |
| --- | --- | --- |
| 6 PM | Arrival | Introductions, exploration, first objectives, conversation. |
| 8 PM | Dinner | The host brings people together. Guests may talk about their agendas. |
| 10 PM | Garden Party | Outdoor space, exploration, and chances to be alone. |
| 12 AM | End of Night | The deadline, the vote, and the result. |

These are gatherings, not separate rounds. Players can walk between rooms the whole time. Missing dinner is allowed. A guest who skips it misses what was said there.

One event is different. Once per match the host calls a compulsory assignment, described in section 2.3. A living guest who misses it, or fails its task, is expelled.

### 2.2 The Host

The host is an NPC. The host welcomes the guests, hands out agendas in private at the start, announces the gatherings, prompts dinner conversation, calls people together for a vote, and once per match calls the living players to the hall for the compulsory assignment.

The host does not judge who is guilty, invent new objectives, or play the match for the players. The assignment notes are written in advance. The host's lines can be written in advance.

When only two living people remain, the host announces that fact through the manor. The announcement is described in section 5.

### 2.3 The Host's Compulsory Assignment

Once each match, the host summons the living players to the main hall over the intercom.

A table in the hall holds one private note for each living player, including the killer. Each person takes one note. The server gives that person their own note. It does not show the note to anyone else.

A guest's note is a simple task and a place in the manor. It is not their personal agenda. The agenda stays as it was. A guest may still ignore the agenda. A living guest may not ignore this task.

The server checks that the assigned task was completed. The time limit is a few minutes. It should be long enough to walk there and do the task, and short enough that people are spread through the house together.

A living guest who has not finished when the time ends is expelled. They leave the match. They do not become a rat. Their agenda leaves the set. If they were a target, they come off the hit list. That is not a kill, and nobody is added in their place. If the expulsion leaves the killer and one guest, the hunt in section 5 starts. If it leaves only the killer, the killer wins.

The killer does not get a task, and cannot be expelled by this event. The killer's note is a clue to a tool or a weapon. The item is in a box with a lock timer. The server allows the box to be opened only during this assignment. When the time ends, the server refuses it. The clue is private to the killer. The box does not store the killer's id.

In those few minutes the killer can get a kill, fetch the item, or try to do both. The assignment does not pause murder.

Rats are not called to the table. This event does not expel them.

The assignment sends people to different rooms at the same time. They meet where they did not plan to. The task competes with a guest's agenda and with the killer's own plan.

## 3. Roles and Victory

### 3.1 The Killer

One player is secretly the killer.

The killer receives a private list of targets. The other players never see this list. They can only guess who is in danger from what they see and hear.

The killer has no personal agenda. Part of the role is inventing a believable reason to walk around, talk to people, and visit certain rooms.

The killer wins only by finishing the target list and also surviving the social side of the evening. A finished list is not, by itself, a win. Section 4 and section 5 describe the exact win checks.

### 3.2 Guests

Every other player is a guest. Each guest receives a private personal agenda.

Guests can win in more than one way. Any one of these wins ends the match for the guests:

- They vote the killer out, while three or more players are alive.
- At midnight, with three or more players alive, at least one target is still alive.
- Every personal agenda still in the match has been completed.
- The final hunt ends with the last guest still alive. Section 5 describes the hunt.

Guests do not have to chase all of these at once. A guest may drop their own agenda to watch someone, protect a person, or help someone else.

### 3.3 The Hit List

The hit list is private to the killer.

The number of targets depends on the lobby size.

| Players | Targets |
| --- | --- |
| 5–6 | 2 |
| 7–8 | 3 |

There is no rule that exactly three players must be alive at the end.

The killer may also kill a guest who is not on the list. That death does not count as a target. It still takes time, makes noise, and can leave evidence. It can also remove a witness. The cost is the final hunt, described in section 5. Killing extra people can throw away a vote the killer might have survived by talking.

Targets are assigned for the match. When a target leaves the game, the list gets shorter. It does not gain a new name. See section 9.

### 3.4 The Strongest Killer Plan

In a five-player match the cast is the killer, two targets, and two other guests.

The strongest plan is to kill the two targets and leave the other two guests alive. Midnight is then a vote among those three living people. The killer wins that vote if the two guests do not both name the killer.

Killing a witness is allowed. It does not end the killer's chance to win. It can lead to the hunt in section 5.

## 4. Voting

Voting happens when the host calls the living players together. That is during a scheduled gathering, or at midnight. A few people in a corridor cannot vote someone out.

Every living player votes, including the killer. Rats do not vote.

The killer is out only if a majority of the living players name the killer. The guests then win immediately.

Any other result changes nothing. The accused stays in the match. Voting then locks until the next gathering.

With three living people, both guests have to name the killer. The killer will name someone else. If the guests split, the killer stays.

At midnight, the match checks the result in this order:

1. If every remaining agenda is already complete, the guests have already won.
2. If a target is still alive, the guests win.
3. If every target is dead, the living players vote. The guests win if the vote names the killer. Otherwise the killer wins.

Players may also force the issue earlier, at a gathering. A successful vote ends the match then. A failed vote only locks voting until the next gathering.

## 5. The Final Hunt

The hunt starts when a death or a disconnect leaves exactly two living people. Those two are the killer and one guest.

The host announces, throughout the manor, that only two people are alive: the killer and one guest. The announcement does not say who is who.

Voting closes. With only one guest left, a vote would have only one possible answer.

The clock speeds up. The hunt lasts a few minutes, even if the deaths happened early in the evening. The exact length can be tuned in playtests. It should feel like a climax, not a second full match.

From here:

- If the last guest is alive when that clock ends, the guests win. This is true even if that guest was never a target, and even if every target is already dead.
- If the killer kills the last guest before the clock ends, the killer wins. If that guest was a remaining target, the kill also completes the list.
- Agendas stay active during the hunt. Rats, and the guest who is hiding, can still finish them. If every remaining agenda is completed before the catch, the guests win. The catch itself ends the match, so the agendas have to be finished first.

If the killer is the only living person left, the killer wins. There is no guest left to hide, vote, or survive.

## 6. Personal Agendas

### 6.1 Purpose

Agendas are private goals. The host gives them to guests at arrival. The killer does not get one.

An agenda gives a guest a reason to visit a place, handle an object, or learn something. It is also one of the ways the guests can win, if every remaining agenda is finished.

Players may ignore their agendas. The vote and the clock do not depend on them.

### 6.2 What an Agenda Contains

Agendas are data, checked by the server. A player cannot win one by saying it is done.

An agenda can include:

- A name.
- A private description.
- The condition that completes it.
- The objects, rooms, or actions involved.
- An optional hint.
- A short way for a rat to finish it.
- Whether it is complete.

The completion check should allow more than one approach where that makes sense. The first agendas should be about objects, rooms, and information. A guest might need to place an item, open a cabinet, or read a document.

Agendas that need a score, an endorsement, or a formal social system are out. The game has no reputation meter.

Two guests can want the same object for different reasons. One guest can need a paper that another guest wants to hide. Those overlaps are where deals, theft, and lies come from.

### 6.3 Collective Victory

Every guest agenda still in the match must be finished. When the last one is finished, the guests win immediately.

The game tells each player about their own agenda in private. It does not show a public list of who is done. A public list would make the killer obvious, because the killer has no agenda. Players may claim they are finished. The game does not confirm that claim out loud.

Living guests do not need to know whether the rats are going to finish. They can play for the vote and the clock. If the agendas all get done, the match ends on its own.

### 6.4 Rats and Agendas

Death does not delete an agenda. A guest who becomes a rat still has to finish theirs, if they are still playing, for the collective win to happen.

Each agenda needs a rat version: one short action in the world, with a private marker that only that rat can see. A rat might move a small object, reach a hidden spot, or trip a mechanism. The action should be the same goal, done in a smaller way.

A rat who is still playing may ignore the agenda and spend the time helping the other guests, or bothering the killer. That choice can stop the agenda victory. It does not stop the vote, midnight, or the hunt.

### 6.5 Leaving and Going Idle

If a player disconnects, they are removed. Their agenda leaves the set. A quitter cannot freeze the match.

If a rat does nothing for a couple of minutes, they get a private warning. If they still do nothing, their agenda leaves the set. Walking around counts as being present.

A rat who is present and simply refuses to finish stays in the set. That is a real choice. It blocks the agenda win only.

## 7. Dinner and Talking

Dinner is the gathering where the host asks the guests what they are doing at the manor.

A guest may tell the truth, tell part of the truth, refuse to say, or lie. Innocent guests are allowed to lie. Some real agendas involve theft, secrets, or embarrassing facts. If only the killer lied, dinner would be too easy to read.

The game does not grade what anyone says. It does not compare a speech to the real agenda. There is no truth score.

Players can compare what was said with what they later see. A person can claim they are looking for a portrait, and someone may remember seeing them in the gallery. That memory is a reason to trust them, or a reason to doubt them. It does not prove they are innocent.

The killer has no real agenda, so the killer has to invent one. Other players have to decide whether the story fits the behaviour.

Promises are talk. The game does not enforce a deal. A player may break one.

There is no reputation bar and no endorsement score. Trust is something the players do by talking.

## 8. Murder, Witnesses, and Evidence

### 8.1 Murder

The killer does not have an instant, silent button that always works.

A murder takes a deliberate action. It has a place, a method, a sound, and a result. Nearby players can see it, hear it, and talk about it afterwards. The target, or someone close by, should have a chance to react.

A kill in front of other people can succeed. The game does not cancel it because someone was looking. Those people can then say what they saw.

Possible methods include poison and a direct attack. Poison can be prepared earlier and take effect later. A direct attack is faster and easier to see. The methods should leave different traces. They are tools for the scene, not a list of stats.

Killing a target advances the list. Killing anyone else does not.

### 8.2 What the Victim Knows

After the murder, the victim becomes a rat. They may be sure who did it, unsure, or have no useful memory. That depends on the method and the situation. An attack from the front is easier to remember than poison or an attack from behind.

That memory stays with the dead player. The game does not print the killer's name on a living player's screen. The victim cannot say the name out loud, because a rat cannot speak. They can try to point the others toward it by moving objects or making noise.

### 8.3 Evidence

Players will collect four kinds of impression:

- Social: relationships, secrets, grudges, stories that do not match.
- Time: where someone claimed to be, who saw them, which rooms they used.
- Physical: keys, papers, weapons, where an object was left, traces of a murder.
- Behaviour: someone knowing a thing they should not know, or pushing the group toward the wrong person.

Objects and traces should be useful and uncertain. A stain, a moved glass, or a missing key should give the players something to argue about. The game should not attach the killer's name to the clue.

Players may lie about where they were. The game does not decide which alibi is true.

The killer can move objects, hide things, and tell a false story. The killer cannot wipe the evening clean. A murder should leave something behind.

## 9. Disconnects

Two living people means the killer and one guest.

If a target disconnects:

- They are removed from the match.
- They are removed from the hit list.
- This is not a kill.
- Nobody is added to the list in their place.
- The number of targets left simply falls by one.
- Their agenda leaves the set.

If a guest who is not a target disconnects, they are removed, and their agenda leaves the set. The hit list does not change.

If a disconnect leaves only the killer and one guest, the hunt in section 5 starts.

If the killer disconnects, the guests win.

## 10. Rats

When a guest dies, they come back as a rat. They stay in the match.

A rat can do two useful things:

- Finish their personal agenda.
- Help the living guests, or get in the killer's way, by doing things in the world.

Help might mean moving a small object, knocking something over, opening an unlocked door, or making a noise. The living players have to notice and interpret it. A rat cannot speak. A rat cannot vote. A rat has no nameplate, no tracking of players through walls, and no way to point a label at a person.

There is no Mischief meter, no shared pool, and no cost bar. Rat actions are ordinary interactions. If a particular action needs a limit so it cannot be spammed, that limit can be a simple cooldown on that action. It is not a currency.

A rat does not speak.

A rat shares the guests' win and the guests' loss. There is no separate rat victory. A rat cannot win by helping the killer. Cheese, stolen trinkets, and general trouble are something to do. They are not a score.

Rats should be able to reach places people cannot, such as a crawlspace or a gap under the floor. That is how some agendas stay possible after death. A shared nest can wait. It is not required for the first version.

During the hunt, rats can still finish agendas or try to interfere. They still cannot tell the hiding guest who the killer is in words. The host's announcement has already told the manor that one of the two living people is the killer.

## 11. The Manor and Objects

The manor should give people reasons to move. Agendas send them to different rooms. Gatherings pull them back together. A found object or a locked door can change the plan.

Useful places include the library, dining room, billiards room, wine cellar, study, conservatory, garden, and bedrooms. The first playable version does not need all of them. Section 13 describes the first slice.

Objects use a small set of shared actions: pick up, drop, give, hide, inspect, and steal. A key can open a room, be stolen, become a clue, and later be traded or hidden. The same actions should cover most objects. Individual items should not each invent a new rule.

Doors and keys decide who can reach a room, who could have found an object, and who could have been present for a murder.

## 12. Activities

Scheduled activities should change the match. A good activity moves people, objects, access, or information.

The first evening does not need a stack of minigames. Charades, croquet, poker, chess, and a quiz can wait. Each of those is a separate piece of work.

Dinner, seating, and passing objects around are enough at the start, because they use talking and the object rules we already need. An auction or a treasure hunt can come later, and only if it hands out keys, letters, or access that the match already understands.

Refusing to join an ordinary activity does not make someone guilty. The compulsory assignment in section 2.3 is the exception. A living guest who refuses it, or fails its task, is expelled from the manor. The expulsion does not name them as the killer.

## 13. First Playable Version

The first test is small. The goal is to see whether players naturally negotiate, steal, hide things, lie, and investigate.

Include:

- Five players, with host and join.
- First-person movement.
- One library and an adjoining corridor.
- Pick up, drop, and give.
- A locked cabinet and a key.
- Papers and a few valuable objects that can be moved.
- A handful of basic agendas, checked by the server.
- Proximity voice chat.
- Private delivery of the killer role, the target list, and each agenda. Other clients must not receive that information.

Murder, the vote, the hunt, and rats can be added on top of that, in that spirit: one complete evening, not a pile of unfinished systems.

A sensible build order is:

1. Movement, joining, voice, and object interaction.
2. Private roles, the target list, and a server-checked murder.
3. Doors, keys, and clues.
4. Agendas, including the short rat version.
5. The schedule, dinner, the vote, and the hunt.
6. The rat itself.

## 14. Technical Rules

The server decides murders, agenda completion, votes, the clock, and who is alive.

Each client may know its own role, its own agenda, and, for the killer, the target list. Clients must not be able to read anyone else's private information from the network.

The host's announcements, the living player count, the vote, and an expulsion are public.

Agenda progress is private to the player who owns it. A compulsory-assignment note is private to the player who took it. The killer's tool clue is private to the killer.

## 15. Tuning, Not Open Rules

These numbers can change after playtests. They are not undecided rules.

- The sped-up hunt lasts a few minutes.
- The compulsory assignment lasts a few minutes.
- A rat is warned after a couple of minutes with no input.
- A five- or six-player match has two targets. A seven- or eight-player match has three.
- Poison's delay, and how hard a witnessed murder is to land, will need to be felt in a real session.

The rules those numbers serve are already set. They are in sections 2.3, 3, 4, 5, 6, and 10.
