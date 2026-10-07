#!/usr/bin/env python3
"""Set up a fresh Serpent Isle game with a named Avatar, as the DOS main menu did.

The menu wrote gameargs.dat beside the game: one byte, the portrait chosen, then the name.
Portraits run female, male, female, male, female, male with darker skin each pair, so the
game reads bit 0 as male and the rest as skin colour 0-2. When the game starts and finds the
file, it deletes GAMEDAT, builds a new game from STATIC/INITGAME.DAT and applies the name, sex
and skin.

    python3 "ultima7-portable/Serpent Isle/tools/si_new_game.py" /path/to/serpent --name Jamie --female --skin 1

The current GAMEDAT is copied aside first, since the game removes it.
"""
import argparse
import shutil
import time
from pathlib import Path


def find(folder, name):
    for child in folder.iterdir():
        if child.name.lower() == name.lower():
            return child
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('data', type=Path)
    ap.add_argument('--name', required=True)
    sex = ap.add_mutually_exclusive_group(required=True)
    sex.add_argument('--female', action='store_true')
    sex.add_argument('--male', action='store_true')
    ap.add_argument('--skin', type=int, choices=(0, 1, 2), default=0,
                    help='0 fair, 1 medium, 2 dark (default 0)')
    args = ap.parse_args()

    name = args.name.encode('ascii')
    if not 0 < len(name) <= 14:
        ap.error('the name must be 1 to 14 characters')

    gamedat = find(args.data, 'GAMEDAT')
    if gamedat is not None:
        backup = args.data.parent / f'gamedat-before-new-game-{time.strftime("%Y%m%d-%H%M%S")}'
        shutil.copytree(gamedat, backup)
        print(f'copied the current GAMEDAT to {backup}')

    portrait = args.skin * 2 + (1 if args.male else 0)
    record = bytes([portrait]) + name.ljust(16, b'\0')
    (args.data / 'gameargs.dat').write_bytes(record)
    print(f'new game ready: {args.name}, {"male" if args.male else "female"}, skin {args.skin}; '
          'it starts on the next launch')


if __name__ == '__main__':
    main()
