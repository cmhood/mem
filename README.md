# `mem` \-  A spaced repetition flashcard program that runs in the terminal

`mem` is a program that helps you to memorize arbitrary pieces of textual
information. It can be used to learn vocabulary in a foreign language, for
example. The program runs in the terminal, and each deck of flashcards is just a
plaintext file which you can create yourself.

Flashcards are reviewed using a [spaced
repetition](https://en.wikipedia.org/wiki/Spaced_repetition) system, where cards
you know well are reviewed less frequently, and cards you are struggling with
are reviewed more frequently.

Each day, some cards will be selected for review, while others are skipped.
Once all selected cards are correctly recalled (i.e. scored at least a 3) on a
given day, the program will update your deck(s) based on your responses, and no
further review will be possible for that day.

## Usage

### Command line

```
mem deck[...]
```

### Creating flashcards

Create a deck of flashcards as a plaintext file. Separate individual flashcardsi
with a line containing only a `%` character. Within each flashcard, use the `|`
character to break up a flashcard into different parts, and `mem` will pause
after each part. (Press _space_ to progress each flashcard.) The `\` character
is handled as an escape character.

Example deck:

```
to know
|saber
%
to think
|pensar
```

### Reviewing flashcards

After viewing each flashcard, the program will ask you how well the flashcard
was recognized by prompting you to provide keyboard input:

  * `5` - Perfect, quick recollection
  * `4` - Recalled after some thought
  * `3` - Recalled with serious difficulty
  * `2` - Failed to recall but very easily recognized
  * `1` - Failed to recall but recognized
  * `0` - Not recognized

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
