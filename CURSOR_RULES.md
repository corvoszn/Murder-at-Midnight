# Cursor rules

These rules apply to every Cursor agent working in this project.

## Read first

Before changing systems, read [GDD.md](GDD.md) and [ARCHITECTURE.md](ARCHITECTURE.md). The GDD is the game. The architecture file is the Unreal layout. If they disagree, stop and ask.

## Do not get ahead of approval

Steps 1 to 4 are in the project. Do not start a later build step until the user asks for it.

Do not create a second Unreal project. Do not implement murder, roles, agendas, votes, the hunt, rats, or voice unless that step was requested.

After approval, build only the step the user names. The Unreal project already exists at `MurderAtMidnight/`. The default first step adds the missing match classes onto the first-person base. Do not create a second project. Do not fill those classes with murder, roles, or agendas in the same step. Leave the Horror and Shooter sample variants in place unless the user asks to remove them.

## Authority

The server decides every match change: phase, role, targets, murder, inventory, doors, agendas, votes, life, and wins.

A client message is a request. Validate it on the server. Ignore requests that try to set a role, complete an agenda, or declare a winner.

## Privacy

The killer id, target list, agenda text, agenda progress, and a victim's memory are private.

Send them with a client RPC to that player's PlayerController, or keep them on the server-only GameMode.

Do not put them on the GameState, on a normally replicated PlayerState field, or on a world actor. Do not print them in a log that other players receive.

After adding a replicated property, say who is meant to receive it.

The player who hosts a listen server can still read server memory. Do not pretend replication hides secrets from that host.

## Where code goes

C++ owns rules and replication.

Blueprint and data assets own durations, meshes, host lines, rooms, and agenda content. Blueprint may play a reaction to a phase or a result. It does not decide the result.

Prefer a component or a data asset over a new character subclass. Do not add a manager or subsystem beside the GameMode unless the user asks. The GameMode is the match authority.

## Game rules that are easy to break in code

- The target list is private to the killer.
- Killing a guest who is not a target does not count as a target.
- A target who disconnects is removed from the list. They are not replaced. It is not a kill.
- Two living people means the killer and one guest. That starts the hunt.
- Rats cannot speak and cannot vote. They keep their agenda unless they disconnect or go idle.
- There is no Mischief meter and no reputation score.
- Game clues do not store the killer's player id.

## In-engine work

Do not spend repeated attempts on something the user can fix in the Unreal editor. Ask them to do that step. They can reparent a Blueprint, assign a class, change a default, compile, or press Play.

Do not hand them hard Blueprint node work. Do not ask them to write C++ or to reason about C++. Explain the editor click in plain language, then continue from what they report.

## How to work

Explain an architecture change before making it. Do not rewrite a system the user did not ask to replace.

Keep explanations in plain sentences. Name the class that owns a fact, and name who receives it.

Do not commit unless the user asks.
