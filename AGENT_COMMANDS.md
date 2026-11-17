# PowerShell commands

Run these from `C:\Users\garym\Murder-at-Midnight`.

## Git

```powershell
cd C:\Users\garym\Murder-at-Midnight
git status
git diff
git log -5 --oneline
```

## Start the Cursor agent

```powershell
cd C:\Users\garym\Murder-at-Midnight
agent
```

Start it with one instruction:

```powershell
agent "Read GDD.md, ARCHITECTURE.md, and CURSOR_RULES.md before changing systems."
```

Resume the last session:

```powershell
agent --continue
```

## Build the game

Close the Unreal editor first. A build while the editor is open can fail, and a live compile can break a data table.

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" MurderAtMidnightEditor Win64 Development -Project="C:\Users\garym\Murder-at-Midnight\MurderAtMidnight\MurderAtMidnight.uproject" -WaitMutex
```

A small change inside a class that already exists can be compiled in the editor with Ctrl+Alt+F11. Do not use that for a new class, a new replicated field, or a data table row change.
