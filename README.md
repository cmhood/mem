# `mem` \-  A spaced repetition flashcard program that runs in the terminal

`mem` is a program that helps you to memorize arbitrary pieces of textual
information. It can be used to learn vocabulary in a foreign language, for
example. The program runs in the terminal, and each deck of flashcards is just a
plaintext file which you can create yourself.

Flashcards are reviewed using a [spaced
repetition](https://en.wikipedia.org/wiki/Spaced_repetition) system, where cards
you know well are reviewed less frequently, and cards you are struggling with
are reviewed more frequently.

Each day, some cards will be selected for review, while others will be skipped.
Once all selected cards are successfully recalled, the program will update your
deck(s) based on your responses, and no further review will be possible for that
day.

## Usage

### Command line

```
mem deck[...]
```

You can review multiple decks at a time. The flashcards from each deck will be
interspersed.

### Creating flashcards

Create a deck of flashcards as a plaintext file. Separate individual flashcards
with a line containing only a `%` character. Within each flashcard, use the `|`
character to break up a flashcard into different parts, and `mem` will pause
after each part. (Press the space key to advance.) The `\` character is handled
as an escape character.

Example deck:

```
to know
|saber
%
to think
|pensar
```

### Reviewing flashcards

After each flashcard is shown, the program will prompt you for keyboard input to
rate your recollection of it:

  * `5` - Perfect, quick recollection
  * `4` - Recalled after some thought
  * `3` - Recalled with serious difficulty
  * `2` - Failed to recall but very easily recognized
  * `1` - Failed to recall but recognized
  * `0` - Not recognized

The backtick/tilde key is also aliased to `0` due to its location on the US
keyboard layout.

## Installation

Run `make install` with sufficient privileges.

## License

Copyright (c) 2020-2024 Charles Hood <chood@chood.net>

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
