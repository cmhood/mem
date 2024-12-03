/*
 * Copyright (c) 2024 Charles Hood <chood@chood.net>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define E_FACTOR_FIXED_POINT 4096
#define E_FACTOR_MIN ((uint32_t) (1.3f * E_FACTOR_FIXED_POINT))

struct flashcard {
	uint32_t e_factor;
	uint32_t repetition_interval;
	time_t review_timestamp;
	size_t body_size;
	char *body;
};

struct deck {
	FILE *fp;
	char *buf;
	size_t buf_size;
	size_t flashcard_count;
	size_t flashcard_limit;
	struct flashcard *flashcards;
};

static struct {
	size_t deck_count;
	size_t deck_limit;
	struct deck *decks;
} deck_book = {
	.deck_limit = 1,
};
static size_t due_flashcard_count;
static size_t next_due_flashcard_count;
static struct flashcard **due_flashcards;

static struct termios original_termios;
static struct termios raw_termios;
static bool raw_mode_enabled = false;

static time_t current_time;
static struct tm current_day_tm;
static time_t current_day;

static void show_usage(void);
static void die(const char *);
static void *xmalloc(size_t);
static void *xrealloc(void *, size_t);
static void enable_raw_mode(void);
static void disable_raw_mode(void);
static void set_termios(struct termios *);
static time_t get_day(time_t, struct tm *);

static void load_deck(const char *);
static void parse_deck(struct deck *);
static void parse_header(char **, char *, struct flashcard *);
static uint32_t parse_uint32(char **, char *);
static uint64_t parse_uint64(char **, char *);

static void get_due_flashcards(void);
static void shuffle_flashcards(struct flashcard **, size_t);
static void review_flashcard(struct flashcard *, bool);
static char get_raw_char(void);

static void write_deck(struct deck *);

int
main(const int argc, const char **argv)
{
	current_time = time(NULL);
	current_day = get_day(current_time, &current_day_tm);

	if (argc < 2)
		show_usage();

	deck_book.decks = xmalloc(deck_book.deck_limit * sizeof(struct deck));
	for (int i = 1; i < argc; i++)
		load_deck(argv[i]);

	for (size_t i = 0; i < deck_book.deck_count; i++)
		parse_deck(&deck_book.decks[i]);

	srand(current_time);
	get_due_flashcards();
	bool is_repeat = false;
	while (due_flashcard_count > 0) {
		shuffle_flashcards(due_flashcards, due_flashcard_count);

		printf("\x1b[1;1H\x1b[2J");
		for (size_t i = 0; i < due_flashcard_count; i++)
			review_flashcard(due_flashcards[i], is_repeat);

		is_repeat = true;
		due_flashcard_count = next_due_flashcard_count;
		next_due_flashcard_count = 0;
	}
	free(due_flashcards);
	if (!is_repeat)
		printf("No flashcards due for review\n");

	for (size_t i = 0; i < deck_book.deck_count; i++)
		write_deck(&deck_book.decks[i]);

	free(deck_book.decks);

	return 0;
}

static void
show_usage(void)
{
	fprintf(stderr, "Usage: mem deck...\n");
	exit(EXIT_FAILURE);
}

static void
die(const char *str)
{
	fprintf(stderr, "mem: %s: %s\n", str, strerror(errno));
	exit(EXIT_FAILURE);
}

static void *
xmalloc(size_t size)
{
	void *ptr = malloc(size);
	if (ptr == NULL && size != 0)
		die("malloc");
	return ptr;
}

static void *
xrealloc(void *ptr, size_t size)
{
	ptr = realloc(ptr, size);
	if (ptr == NULL && size != 0)
		die("realloc");
	return ptr;
}

static void
enable_raw_mode(void)
{
	static bool first_run = true;
	if (first_run) {
		first_run = false;

		if (tcgetattr(STDIN_FILENO, &original_termios) == -1)
			die("tcgetattr");

		raw_termios = original_termios;
		raw_termios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
		raw_termios.c_oflag &= ~(OPOST);
		raw_termios.c_cflag |= CS8;
		raw_termios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
		raw_termios.c_cc[VMIN] = 0;
		raw_termios.c_cc[VTIME] = 1;
	}

	set_termios(&raw_termios);
	raw_mode_enabled = true;
}

static void
disable_raw_mode(void)
{
	if (raw_mode_enabled) {
		set_termios(&original_termios);
		raw_mode_enabled = false;
	}
}

static void
set_termios(struct termios *termios)
{
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, termios) == -1)
		die("tcsetattr");
}

static time_t
get_day(time_t time, struct tm *date)
{
	localtime_r(&time, date);
	date->tm_sec = 0;
	date->tm_min = 0;
	date->tm_hour = 0;
	date->tm_isdst = 0;
	return mktime(date);
}

static void
load_deck(const char *path)
{
	if (deck_book.deck_count == deck_book.deck_limit) {
		deck_book.deck_limit *= 2;
		deck_book.decks = xrealloc(deck_book.decks, deck_book.deck_limit * sizeof(struct deck));
	}
	struct deck *deck = &deck_book.decks[deck_book.deck_count];
	deck_book.deck_count++;

	deck->fp = fopen(path, "r+");
	if (deck->fp == NULL)
		goto fail;

	if (fseek(deck->fp, 0L, SEEK_END) == -1)
		goto fail;

	long file_size = ftell(deck->fp);
	if (file_size == -1)
		goto fail;

	rewind(deck->fp);

	deck->buf = xmalloc(file_size + 1);
	deck->buf_size = fread(deck->buf, 1, file_size, deck->fp);
	if (ferror(deck->fp) != 0)
		goto fail;

	if (deck->buf[deck->buf_size - 1] != '\n') {
		deck->buf[deck->buf_size] = '\n';
		deck->buf_size++;
	}

	rewind(deck->fp);
	return;

fail:
	die(path);
}

static void
parse_deck(struct deck *deck)
{
	char *ptr = deck->buf;
	char *buf_end = deck->buf + deck->buf_size;

	deck->flashcard_count = 0;
	deck->flashcard_limit = 1;
	deck->flashcards = xmalloc(deck->flashcard_limit * sizeof(struct flashcard));

	while (ptr != buf_end) {
		if (deck->flashcard_count == deck->flashcard_limit) {
			deck->flashcard_limit *= 2;
			deck->flashcards = xrealloc(deck->flashcards, deck->flashcard_limit * sizeof(struct flashcard));
		}
		struct flashcard *flashcard = &deck->flashcards[deck->flashcard_count];
		deck->flashcard_count++;

		parse_header(&ptr, buf_end, flashcard);

		flashcard->body = ptr;
		while (ptr != buf_end && !(ptr > deck->buf && ptr[-1] == '\n' && *ptr == '%'))
			ptr++;
		flashcard->body_size = ptr - flashcard->body;
	}
}

static void
parse_header(char **ptr, char *end, struct flashcard *flashcard)
{
	if (**ptr != '%')
		goto new_header_without_seek;
	(*ptr)++;
	flashcard->e_factor = parse_uint32(ptr, end);

	if (**ptr != '%')
		goto new_header;
	(*ptr)++;
	flashcard->repetition_interval = parse_uint32(ptr, end);

	if (**ptr != '%')
		goto new_header;
	(*ptr)++;
	flashcard->review_timestamp = parse_uint64(ptr, end);

	if (**ptr != '\n')
		goto new_header;
	(*ptr)++;
	return;

new_header:
	while (**ptr != '\n')
		(*ptr)++;
	(*ptr)++;
new_header_without_seek:
	flashcard->e_factor = 2.5f * E_FACTOR_FIXED_POINT;
	flashcard->repetition_interval = 0;
	flashcard->review_timestamp = current_time;
}

static uint32_t
parse_uint32(char **ptr, char *end)
{
	uint32_t n = 0;
	while (*ptr != end && '0' <= **ptr && **ptr <= '9') {
		n *= 10;
		n += (**ptr - '0');
		(*ptr)++;
	}
	return n;
}

static uint64_t
parse_uint64(char **ptr, char *end)
{
	uint64_t n = 0;
	while (*ptr != end && '0' <= **ptr && **ptr <= '9') {
		n *= 10;
		n += (**ptr - '0');
		(*ptr)++;
	}
	return n;
}

static void
get_due_flashcards(void)
{
	size_t due_flashcard_limit = 1;
	due_flashcards = xmalloc(due_flashcard_limit * sizeof(struct flashcard *));
	for (size_t i = 0; i < deck_book.deck_count; i++) {
		struct deck *deck = &deck_book.decks[i];
		size_t limit = 8;
		for (size_t j = 0; j < deck->flashcard_count; j++) {
			struct flashcard *flashcard = &deck->flashcards[j];
			struct tm tm;
			time_t review_day = get_day(flashcard->review_timestamp, &tm);
			if (review_day > current_day)
				continue;

			if (flashcard->repetition_interval == 0) {
				if (limit == 0) {
					struct tm review_timestamp_tm = current_day_tm;
					review_timestamp_tm.tm_mday++;
					flashcard->review_timestamp = mktime(&review_timestamp_tm);
					continue;
				}
				limit--;
			}

			if (due_flashcard_count == due_flashcard_limit) {
				due_flashcard_limit *= 2;
				due_flashcards = xrealloc(due_flashcards, due_flashcard_limit * sizeof(struct flashcard *));
			}
			due_flashcards[due_flashcard_count] = flashcard;
			due_flashcard_count++;
		}
	}
}

static void
shuffle_flashcards(struct flashcard **flashcards, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		/*
		 * rand() will only produce numbers up to RAND_MAX, which is
		 * probably less than SIZE_MAX, but no one will ever reach
		 * numbers that high, and the only consequence is that the
		 * flashcards aren't shuffled as well as they could be.
		 */
		size_t r = (size_t) rand() % (count - i) + i;
		struct flashcard *tmp = flashcards[i];
		flashcards[i] = flashcards[r];
		flashcards[r] = tmp;
	}
}

static void
review_flashcard(struct flashcard *flashcard, bool is_repeat)
{
	for (size_t i = 0; i < flashcard->body_size; i++) {
		char c = flashcard->body[i];
		switch (c) {
		case '|':
			while (get_raw_char() != ' ')
				continue;
			break;
		case '\\':
			i++;
			c = flashcard->body[i];
			// FALLTHROUGH
		default:
			putchar(c);
		}
	}
	printf("\x1b[1mScore: \x1b[0m");
	fflush(stdout);
	int score;
	for (;;) {
		char c = get_raw_char();
		if (c == '`')
			c = '0';
		if ('0' <= c && c <= '5') {
			score = c - '0';
			break;
		}
	}
	printf("\x1b[1m%d\x1b[0m\n", score);

	if (score < 4) {
		due_flashcards[next_due_flashcard_count] = flashcard;
		next_due_flashcard_count++;
	}

	if (!is_repeat) {
		float q = 5.0f - score;
		flashcard->e_factor += (0.1f - q * (0.08f + 0.02f * q)) * E_FACTOR_FIXED_POINT;
		if (flashcard->e_factor < E_FACTOR_MIN)
			flashcard->e_factor = E_FACTOR_MIN;
	}

	if (score < 3 || flashcard->repetition_interval == 0) {
		flashcard->repetition_interval = 1;
	} else if (!is_repeat) {
		if (flashcard->repetition_interval == 1)
			flashcard->repetition_interval = 6;
		else
			flashcard->repetition_interval = (flashcard->repetition_interval * flashcard->e_factor + (E_FACTOR_FIXED_POINT - 1)) / E_FACTOR_FIXED_POINT;
	} else {
		return;
	}

	struct tm review_timestamp_tm = current_day_tm;
	review_timestamp_tm.tm_mday += flashcard->repetition_interval;
	flashcard->review_timestamp = mktime(&review_timestamp_tm);
}

static char
get_raw_char(void)
{
	fflush(stdout);
	enable_raw_mode();
	char c = '\0';
	while (c == '\0') {
		if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) {
			disable_raw_mode();
			die("read");
		}
	}
	disable_raw_mode();
	return c;
}

static void
write_deck(struct deck *deck)
{
	for (size_t i = 0; i < deck->flashcard_count; i++) {
		struct flashcard *flashcard = &deck->flashcards[i];
		if (fprintf(deck->fp, "%%%u%%%u%%%lu\n", flashcard->e_factor, flashcard->repetition_interval, flashcard->review_timestamp) < 0)
			perror("fprintf");
		if (fwrite(flashcard->body, 1, flashcard->body_size, deck->fp) != flashcard->body_size)
			perror("fwrite");
	}
	free(deck->flashcards);
	free(deck->buf);
	fclose(deck->fp);
}
