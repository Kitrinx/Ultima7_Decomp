#!/usr/bin/env python3
"""Set up a fresh Black Gate game with a named Avatar, as the DOS main menu did.

The menu wrote gameargs.dat beside the game: one byte for the Avatar's sex (nonzero female) and
15 bytes of name. When the game starts and finds it, it deletes GAMEDAT, builds a new game from
STATIC/INITGAME.DAT and applies the name and sex. The Avatar's portrait follows the sex.

    python3 agents/tools/u7_new_game.py build-portable/data --name Jamie --female

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
    args = ap.parse_args()

    name = args.name.encode('ascii')
    if not 0 < len(name) <= 14:
        ap.error('the name must be 1 to 14 characters')

    gamedat = find(args.data, 'GAMEDAT')
    if gamedat is not None:
        backup = args.data.parent / f'gamedat-before-new-game-{time.strftime("%Y%m%d-%H%M%S")}'
        shutil.copytree(gamedat, backup)
        print(f'copied the current GAMEDAT to {backup}')

    record = bytes([1 if args.female else 0]) + name.ljust(15, b'\0')
    (args.data / 'gameargs.dat').write_bytes(record)
    print(f'new game ready: {args.name}, {"female" if args.female else "male"}; it starts on the next launch')


if __name__ == '__main__':
    main()
