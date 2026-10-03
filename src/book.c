/**
 * @file book.c
 *
 * File for opening book management.
 *
 * Edax book is a set of positions. Each position is unique concerning all possible symetries.
 * A position is made of an othello board, a set of moves leading to other positions in the book,
 * called here "link", and the best remaining move, as evaluated by a search at fixed depth, called
 * here leaf. It also contains win/draw/loss statistics, which is actually useless, and a score
 * with two bounds from retropropagated error.
 * Several algorithms are present to add positions in the book, in the most usefull way.
 *
 * @date 1998 - 2020
 * @author Richard Delorme
 * @version 4.5
 */

#include "book.h"
#include "base.h"
#include "search.h"
#include "const.h"
#include "bit.h"
#include "options.h"
#include "util.h"

#include <assert.h>
#include <time.h>
#include <stdarg.h>
#include <limits.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#include <sched.h>
#endif

#define BOOK_DEBUG 0
static const int BOOK_INFO_RESOLUTION = 100000;

#define clear_line() bprint("                                                                                \r")

bool book_verbose = false;

/**
 * @brief print a message on stdout.
 * @param format Format string.
 * @param ... variable arguments.
 */
#if defined(__GNUC__)
static void bprint(const char *format, ...) __attribute__((format(printf, 1, 2)));
#endif
static void bprint(const char *format, ...)
{
	if (book_verbose) {
		va_list args;

		va_start(args, format);
		vprintf(format, args);
		va_end(args);
		fflush (stdout);

	}
}

/**
 * @brief print a message about the book on stdout (as the functions of this file do).
 * @param format Format string.
 * @param ... variable arguments.
 */
void book_print(const char *format, ...)
{
	if (book_verbose) {
		va_list args;

		va_start(args, format);
		vprintf(format, args);
		va_end(args);
		fflush (stdout);
	}
}

/**
 * @brief Run the threads of a book function (negamax, fix, link, sort, deviate walks).
 *
 * The calling thread runs the first worker, the others get their own threads (see thread_run_workers).
 * With the cpu option, each of them gets its own cpu, as the threads of a search do: where the option
 * binds the threads (linux), a new thread would else stay on the cpu of the main thread, and all the
 * threads of the book functions would share cpu 0.
 *
 * @param function Function to run on each worker.
 * @param worker Array of workers.
 * @param size Size of a worker.
 * @param n Number of workers.
 */
static void book_run_workers(void* (*function)(void*), void *worker, const size_t size, const int n)
{
	thread_run_workers(function, worker, size, n, true, options.cpu_affinity);
}

/**
 * @brief Tell if the searches of the book functions must be done one at a time.
 *
 * Where the cpu option binds the threads (linux), the threads of every search are bound to the cpus
 * 0 to n-1: searches done at the same time (book-store-tasks, book-expand-tasks) would all run on
 * the same cpus. They are done one after the other, with the main search, as Edax always did.
 */
static bool book_one_search_at_a_time(void)
{
	return options.cpu_affinity && thread_cpu_bound();
}

/** struct Link
 * @brief a move (with its score) linking to another Position.
 */
typedef struct Link {
	signed char score; /**< move score */
	unsigned char move; /**< move coordinate */
} Link;

const Link BAD_LINK = {-SCORE_INF, NOMOVE};

/**
 * @brief check if a link is unvalid.
 *
 * @param link checked link.
 * @return true if link is unvalid, false otherwise.
 */
static inline bool link_is_bad(const Link *link)
{
	return link->score == -SCORE_INF;
}

/** Number of links stored inside the Position itself (99% of positions of a large book). */
#define POSITION_INLINE_LINKS 4

/**
 * struct Position
 * @brief A position stored in the book.
 */
typedef struct Position {
	Board board;               /**< (unique) board */
	union {
		Link *array;           /**< heap array, used when n_link > POSITION_INLINE_LINKS */
		Link in[POSITION_INLINE_LINKS]; /**< in-place storage for up to POSITION_INLINE_LINKS links */
	} links;                   /**< linking moves (use position_links()) */
	unsigned int n_wins;       /**< game win count */
	unsigned int n_draws;      /**< game draw count */
	unsigned int n_losses;     /**< game loss count */
	unsigned int n_lines;      /**< unterminated line count */
	struct {
		signed char value, lower, upper;
	} score;                   /**< Position value & bounds (saved as 16-bit values; see score_char()) */
	Link leaf;                 /**< best remaining move */
	unsigned char n_link;      /**< linking moves number */
	unsigned char level;       /**< search level */
	unsigned char state;       /**< book epoch and done/todo flags (see below) */
} Position;                    /* 48 bytes on 64-bit targets (56 in v4.5.5-nikque.4, 64 before) */

typedef char position_size_check[(sizeof (void*) != 8 || sizeof (Position) == 48) ? 1 : -1];

/*
 * The state of a position holds the book epoch at which its done/todo flags were set, so that
 * book_clean() only has to change the epoch instead of rewriting every position (a full scan is
 * still done once every POSITION_EPOCH_MAX epochs). The flags of an older epoch are all clear.
 * POSITION_BUSY marks a position being computed by the parallel negamax (always run just after
 * book_clean(), when no position has a todo flag of the current epoch).
 */
#define POSITION_EPOCH_MASK 0x1f
#define POSITION_EPOCH_MAX 31
#define POSITION_BUSY 0x20
#define POSITION_TODO 0x40
#define POSITION_DONE 0x80

/** @return state with a flag set at epoch (the flags of an older epoch are cleared). */
static inline unsigned char position_state_set(const unsigned char state, const unsigned char epoch, const unsigned char flag)
{
	return (unsigned char) (((state & POSITION_EPOCH_MASK) == epoch ? state : epoch) | flag);
}

#define position_state_is(state, book, flag) (((state) & (POSITION_EPOCH_MASK | (flag))) == ((book)->epoch | (flag)))
#define position_is_done(p, book) position_state_is((p)->state, book, POSITION_DONE)
#define position_set_done(p, book) ((p)->state = position_state_set((p)->state, (book)->epoch, POSITION_DONE))
#define position_is_todo(p, book) position_state_is((p)->state, book, POSITION_TODO)
#define position_set_todo(p, book) ((p)->state = position_state_set((p)->state, (book)->epoch, POSITION_TODO))
#define position_clear_todo(p) ((p)->state &= (unsigned char) ~POSITION_TODO)

/**
 * @brief Score stored in a position (one byte): scores are within [-SCORE_INF, SCORE_INF], except
 * bounds widened by book enhance errors above 63, which are saturated.
 */
static inline signed char score_char(const int score)
{
	return (signed char) (score < -SCORE_INF ? -SCORE_INF : (score > SCORE_INF ? SCORE_INF : score));
}

/** @brief linking moves of a position, wherever they are stored. */
#define position_links(p) ((p)->n_link > POSITION_INLINE_LINKS ? (p)->links.array : (p)->links.in)

static Position* book_probe(const Book*, const Board*);
static int book_add(Book*, const Position*);
static void book_mark_todo(Book*, Position*);
static void position_print(const Position*, const Board*, FILE*);
static bool book_game_boards(Book*, const Game*, const bool);

#define foreach_link(l, p)  \
	for ((l) = position_links(p); (l) < position_links(p) + (p)->n_link; ++(l))

/**
 * @brief Replace the links of a position.
 *
 * @param position Position (its previous links are released).
 * @param links New links (may not alias the position's storage).
 * @param n Number of links.
 * @return false if memory is exhausted (the position is left without links).
 */
static bool position_set_links(Position *position, const Link *links, const int n)
{
	if (position->n_link > POSITION_INLINE_LINKS) free(position->links.array);
	position->links.array = NULL;
	position->n_link = 0;
	if (n > POSITION_INLINE_LINKS) {
		Link *array = (Link*) malloc(sizeof (Link) * n);
		if (array == NULL) return false;
		memcpy(array, links, sizeof (Link) * n);
		position->links.array = array;
	} else if (n > 0) {
		memcpy(position->links.in, links, sizeof (Link) * n);
	}
	position->n_link = (unsigned char) n;
	return true;
}

/**
 * @brief return the number of plies from where the search is solving.
 *
 * @param depth search depth.
 * @return book depth.
 */
static int get_book_depth(const int depth)
{
	if (depth <= 10) return 60 - 2 * depth;
	else if (depth <= 18) return 39;
	else if (depth <= 24) return 36;
	else if (depth < 30) return 33;
	else if (depth < 36) return 30;
	else if (depth < 42) return 66 - depth;
	else return 24;
}


/**
 * @brief Check if position is ok or need fixing.
 *
 * Note: All positions should always be OK! A wrong position means a BUG!
 *
 * @param position Position.
 * @param verbose Explain what is wrong (false: only check; used by the threads of book_fix).
 * @return true if ok, false if it needs fixing.
 */
static bool position_check(const Position *position, const bool verbose)
{
	Board board;
	Move move;
	const Link *l;
	int i, j;
	char s[4];

	// board is legal ?
	if (position->board.player & position->board.opponent) {
		if (verbose) warn("Board is illegal: Two discs on the same square?\n");
		if (verbose) board_print(&position->board, BLACK, stderr);
		return false;
	}
	if (((position->board.player | position->board.opponent) & 0x0000001818000000ULL) != 0x0000001818000000ULL) {
		if (verbose) warn("Board is illegal: Empty center?\n");
		if (verbose) board_print(&position->board, BLACK, stderr);
		return false;
	}

	// is board unique
	board_unique(&position->board, &board);
	if (!board_equal(&position->board, &board)) {
		if (verbose) warn("board is not unique\n");
		if (verbose) position_print(position, &position->board, stdout);
		return false;
	}

	// are moves legal ?
	foreach_link(l, position) {
		if (l->move == PASS) {
			if (position->n_link > 1
			 || can_move(board.player, board.opponent)
			 || !can_move(board.opponent, board.player)) {
				if (verbose) warn("passing move is wrong\n");
				if (verbose) position_print(position, &position->board, stdout);
				return false;
			}
		} else {
			if (/*l->move < A1 ||*/ l->move > H8
			 || board_is_occupied(&board, l->move)
			 || board_get_move_flip(&board, l->move, &move) == 0) {
				if (verbose) warn("link %s is wrong\n", move_to_string(l->move, WHITE, s));
				if (verbose) position_print(position, &position->board, stdout);
				return false;
			}
		}
	}

	l = &position->leaf;
	if (l->move == PASS) {
		if (position->n_link > 0
		 || can_move(board.player, board.opponent)
		 || !can_move(board.opponent, board.player)) {
			if (verbose) warn("passing move is wrong\n");
			if (verbose) position_print(position, &position->board, stdout);
			return false;
		}
	} else if (l->move == NOMOVE) {
		if (get_mobility(position->board.player, position->board.opponent) != position->n_link && !(position->n_link == 1 && position_links(position)[0].move == PASS)) {
			if (verbose) warn("nomove is wrong\n");
			if (verbose) position_print(position, &position->board, stdout);
			return false;
		}
	} else if (/*l->move < A1 ||*/ l->move > H8
		 || board_is_occupied(&board, l->move)
		 || board_get_move_flip(&board, l->move, &move) == 0) {
			if (verbose) warn("leaf %s is wrong\n", move_to_string(l->move, WHITE, s));
			if (verbose) position_print(position, &position->board, stdout);
			return false;
	}

	// doublons ?
	l = position_links(position);
	for (i = 0; i < position->n_link; ++i) {
		for (j = i + 1; j < position->n_link; ++j) {
			if (l[j].move == l[i].move) {
				if (verbose) warn("doublon found in links\n");
				if (verbose) position_print(position, &position->board, stdout);
				return false;
			}
		}
		if (position->leaf.move == l[i].move) {
			if (verbose) warn("doublon found in links/leaf\n");
			if (verbose) position_print(position, &position->board, stdout);
			return false;
		}
	}
	return true;
}

/** @brief Check if position is ok or need fixing, and explain what is wrong. */
static bool position_is_ok(const Position *position)
{
	return position_check(position, true);
}

/**
 * @brief Initialize a position.
 *
 * @param position Position.
 */
static void position_init(Position *position)
{
	position->board.player = position->board.opponent = 0;

	position->leaf = BAD_LINK;
	position->links.array = NULL;

	position->n_wins = position->n_draws = position->n_losses = position->n_lines = 0;
	position->score.value = position->score.lower = -SCORE_INF;
	position->score.upper = +SCORE_INF;

	position->n_link = 0;
	position->level = 0;
	position->state = 1 | POSITION_DONE; // done at epoch 1 (as before)
}

/**
 * @brief Merge a position with another one.
 *
 * A position is merged if its level is > to the destination position; or == and
 * its leaf move is not contains into the destination link moves.
 * 
 * Note: link moves are not copied. This can be done later with position_link().
 *
 * @param dest Destination position.
 * @param src Source position.
 */
static void position_merge(Position *dest, const Position *src)
{
	Link *l;

	position_init(dest);		//??? dest->n_link = 0,
	dest->board = src->board;
	if (dest->level == src->level) { 
		foreach_link(l, dest) {	// so this does nothing
			if (l->move == src->leaf.move) return;
		}
		dest->leaf = src->leaf;
	} else if (dest->level > src->level) {
		return;
	} else {
		dest->leaf = src->leaf;
		dest->level = src->level;
	}
	// start from the leaf score (as position_search() does), not from -SCORE_INF:
	// positions that negamax cannot reach from the root would keep +/-127 values
	if (dest->leaf.move != NOMOVE && dest->leaf.score > dest->score.value) dest->score.value = dest->leaf.score;
}

/**
 * @brief Free resources used by a position.
 *
 * @param position Position.
 */
static void position_free(Position *position)
{
	if (position->n_link > POSITION_INLINE_LINKS) free(position->links.array);
}

/**
 * @brief Buffered sequential access to a binary book file.
 *
 * The binary format stores each position as 40 bytes of fixed fields,
 * n_link links of 2 bytes and a 2-byte leaf. Reading or writing it field by
 * field costs ~15 stdio calls per position (about 10 billion calls for a
 * 650 million position book), so records are packed into a large buffer.
 */
typedef struct BookStream {
	FILE *f;
	unsigned char *buffer;
	size_t size, n, pos;
} BookStream;

#define BOOK_STREAM_SIZE (16u << 20)
#define POSITION_FIXED_SIZE 40

static bool book_stream_open(BookStream *s, FILE *f)
{
	s->f = f;
	s->size = BOOK_STREAM_SIZE;
	s->n = s->pos = 0;
	s->buffer = (unsigned char*) malloc(s->size);
	return s->buffer != NULL;
}

static void book_stream_close(BookStream *s)
{
	free(s->buffer);
	s->buffer = NULL;
}

static bool book_stream_read(BookStream *s, void *data, size_t len)
{
	unsigned char *d = (unsigned char*) data;
	while (len) {
		size_t k;
		if (s->pos == s->n) {
			s->n = fread(s->buffer, 1, s->size, s->f);
			s->pos = 0;
			if (s->n == 0) return false;
		}
		k = s->n - s->pos; if (k > len) k = len;
		memcpy(d, s->buffer + s->pos, k);
		s->pos += k; d += k; len -= k;
	}
	return true;
}

/** @return true if no unread byte remains in the stream. */
static bool book_stream_at_end(BookStream *s)
{
	return s->pos == s->n && fgetc(s->f) == EOF && !ferror(s->f);
}

static bool book_stream_write(BookStream *s, const void *data, size_t len)
{
	const unsigned char *d = (const unsigned char*) data;
	while (len) {
		size_t k;
		if (s->n == s->size) {
			if (fwrite(s->buffer, 1, s->n, s->f) != s->n) return false;
			s->n = 0;
		}
		k = s->size - s->n; if (k > len) k = len;
		memcpy(s->buffer + s->n, d, k);
		s->n += k; d += k; len -= k;
	}
	return true;
}

static bool book_stream_flush(BookStream *s)
{
	bool ok = (s->n == 0 || fwrite(s->buffer, 1, s->n, s->f) == s->n);
	s->n = 0;
	return ok;
}

/**
 * @brief Read a position.
 *
 * @param position Position to read in.
 * @param s Input stream.
 */
static bool position_read(Position *position, BookStream *s)
{
	unsigned char h[POSITION_FIXED_SIZE];
	Link links[256];

	if (!book_stream_read(s, h, sizeof h)) return false;
	memcpy(&position->board.player, h, 8);
	memcpy(&position->board.opponent, h + 8, 8);
	memcpy(&position->n_wins, h + 16, 4);
	memcpy(&position->n_draws, h + 20, 4);
	memcpy(&position->n_losses, h + 24, 4);
	memcpy(&position->n_lines, h + 28, 4);
	{
		short value, lower, upper;
		memcpy(&value, h + 32, 2);
		memcpy(&lower, h + 34, 2);
		memcpy(&upper, h + 36, 2);
		position->score.value = score_char(value);
		position->score.lower = score_char(lower);
		position->score.upper = score_char(upper);
	}
	position->level = h[39];

	position->state = 0;

	if (!book_stream_read(s, links, sizeof (Link) * h[38])) return false;
	if (!book_stream_read(s, &position->leaf, sizeof (Link))) return false;

	position->n_link = 0;
	if (!position_set_links(position, links, h[38])) {
		error("cannot allocate opening book position's moves\n");
		return false;
	}

	return true;
}

/**
 * @brief Read a position.
 *
 * @param position Position to read in.
 * @param f Input stream.
 */
static bool position_import(Position *position, FILE *f)
{
	char *line, *s, *old;
	int value;
	Move move;
	bool ok = false;

	if ((line = string_read_line(f)) != NULL) {
		position_init(position);
		s = parse_board(line, &position->board, &value);
		if (s != line) {
			s = parse_find(s, ',');
			if (*s == ',') {
				value = -1;	s = parse_int(old = s + 1, &value); BOUND(value, -1, 60, "level");
				if (s != old && value != -1) {
					position->level = value;
					s = parse_find(s, ',');
					if (*s == ',') {
						s = parse_move(old = s + 1, &position->board, &move);
						if (s != old) {
							s = parse_find(s, ',');
							if (*s == ',') {
								s = parse_int(old = s + 1, &value);
								if (s != old) {
									position->leaf.move = move.x;
									position->leaf.score = value;
								}
							}
						}
					}
					ok = true;
				} else {
					warn("wrong level: %s\n", line);
				}
			} else {
				warn("missing ',' after board setting\n");
			}
		} else {
			warn("wrong board: %s\n", line);
		}
	}

	if (!ok) warn("=> wrong position\n");

	free(line);
	return ok;
}

/**
 * @brief Write a position.
 *
 * @param position position to write out.
 * @param f output stream.
 */
static bool position_write(const Position *position, BookStream *s)
{
	unsigned char h[POSITION_FIXED_SIZE + 2 * 257];
	const int n = position->n_link;

	memcpy(h, &position->board.player, 8);
	memcpy(h + 8, &position->board.opponent, 8);
	memcpy(h + 16, &position->n_wins, 4);
	memcpy(h + 20, &position->n_draws, 4);
	memcpy(h + 24, &position->n_losses, 4);
	memcpy(h + 28, &position->n_lines, 4);
	{
		const short value = position->score.value, lower = position->score.lower, upper = position->score.upper;
		memcpy(h + 32, &value, 2);
		memcpy(h + 34, &lower, 2);
		memcpy(h + 36, &upper, 2);
	}
	h[38] = position->n_link;
	h[39] = position->level;
	if (n) memcpy(h + POSITION_FIXED_SIZE, position_links(position), sizeof (Link) * n);
	memcpy(h + POSITION_FIXED_SIZE + sizeof (Link) * n, &position->leaf, sizeof (Link));

	return book_stream_write(s, h, POSITION_FIXED_SIZE + sizeof (Link) * (n + 1));
}

/**
 * @brief write a position.
 *
 * @param p position to write out.
 * @param f output stream.
 */
static bool position_export(const Position *p, FILE* f)
{
	char b[80], m[4];

	board_to_string(&p->board, BLACK, b);
	move_to_string(p->leaf.move, BLACK, m);
	return (fprintf(f, "%s,%d,%s,%d\n", b, p->level, m, p->leaf.score) > 0);
}

/**
 * @brief Make position unique, regarding symetries.
 *
 * @param position position.
 */
static void position_unique(Position *position)
{
	Board board;
	int i, s;

	board = position->board;
	if ((s = board_unique(&board, &position->board)) != 0) {
		for (i = 0; i < position->n_link; ++i) {
			position_links(position)[i].move = symetry(position_links(position)[i].move, s);
		}
		position->leaf.move = symetry(position->leaf.move, s);
	}
}

/**
 * @brief Get moves from a position.
 *
 * @param position position to get moves from.
 * @param board board.
 * @param movelist movelist.
 */
static int position_get_moves(const Position *position, const Board *board, MoveList *movelist)
{
	Move *previous = movelist->move;
	Move *move = movelist->move + 1;
	Board sym;
	int i, x, s;

	for (s = 0; s < 8; ++s) {
		board_symetry(&position->board, s, &sym);

		if (board_equal(&sym, board)) {
			for (i = 0; i < position->n_link; ++i) {
				x = symetry(position_links(position)[i].move, s);
				board_get_move_flip(board, x, move);
				move->score = position_links(position)[i].score;
				previous = previous->next = move;
				++move;
			}
			x = symetry(position->leaf.move, s);
			if (x != NOMOVE) {
				board_get_move_flip(board, x, move);
				move->score = position->leaf.score;
				previous = previous->next = move;
				++move;
			}
			previous->next = NULL;
			movelist->n_moves = move - movelist->move - 1;
			movelist_sort(movelist);
			return s;
		}
	}

	fatal_error("unreachable code\n");
	return -1;
}

/**
 * @brief print a position in a readable format.
 *
 * @param position position to print out.
 * @param board Symetrical board to use.
 * @param f output stream.
 */
static void position_show(const Position *position, const Board *board, FILE *f)
{
	MoveList movelist;
	Move *move;
	const int n_empties = board_count_empties(board);
	const int color = n_empties & 1;
	int sym;
	char s[4];

	board_print(board, color, f);

	fprintf(f, "\nLevel: %d\n", position->level);
	fprintf(f, "Best score: %+02d [%+02d, %+02d]\n", position->score.value, position->score.lower, position->score.upper);
	fprintf(f, "Moves:");
	sym = position_get_moves(position, board, &movelist);
	foreach_move(move, movelist) {
		move_to_string(move->x, color, s);
		if (symetry(position->leaf.move, sym) == move->x) {
			fprintf(f, " <%s:%+02d>", s, move->score);
		} else {
			fprintf(f, " [%s:%+02d]", s, move->score);
		}
	}
}

/**
 * @brief print a position in a compact but readable format.
 *
 * @param position position to print out.
 * @param board Symetrical board to use.
 * @param f output stream.
 */
static void position_print(const Position *position, const Board *board, FILE *f)
{
	MoveList movelist;
	Move *move;
	int color = board_count_empties(board) & 1, sym;
	char b[80], m[4];

	board_to_string(board, color, b);
	fprintf(f, "{board:%s; ", b);
	fprintf(f, "level:%d; ", position->level);
	fprintf(f, "best: %+02d [%+02d, %+02d];", position->score.value, position->score.lower, position->score.upper);
	fprintf(f, "moves:");
	sym = position_get_moves(position, board, &movelist);
	foreach_move(move, movelist) {
		move_to_string(move->x, color, m);
		if (symetry(position->leaf.move, sym) == move->x) {
			fprintf(f, " <%s:%+02d>", m, move->score);
		} else {
			fprintf(f, " [%s:%+02d]", m, move->score);
		}
	}
	fprintf(f, "}\n");
}

/**
 * @brief Chose a move at random from the position.
 *
 * @param position Position to chose a move from.
 * @param board Correctly rotated/mirrored board.
 * @param move Chosen move.
 * @param r Random data.
 * @param randomness Randomness intensity (randomness = 0 means no randomness).
 */
static void position_get_random_move(const Position *position, const Board *board, Move *move, Random *r, const int randomness)
{
	MoveList movelist;
	Move *m;
	int i, n;

	position_get_moves(position, board, &movelist);

	n = 0;
	foreach_best_move(m, movelist) {
		if (position->score.value <= m->score + randomness) {
			++n;
		} else break;
	}

	if (n == 0) { // no move
		move->x = NOMOVE;
		move->flipped = 0;
		return;
	} else {
		i = (random_get(r) % n); // is good enough here
	}

	foreach_best_move(m, movelist) {
		if (i-- == 0) break;
	}

	*move = *m;
}

/**
 * @brief Add a link to this position.
 *
 * @param position Position to chose a move from.
 * @param link Link to add.
 * @return true if the link has been added, false if it was already present.
 */
static bool position_add_link(Position *position, const Link *link)
{
	Link *l;
	int last = position->n_link;

	foreach_link (l, position) {
		if (l->move == link->move) {
			l->score = link->score; // update the link ?
			return false;
		}
	}

	if (last < POSITION_INLINE_LINKS) {
		position->links.in[last] = *link;
	} else {
		if (last == POSITION_INLINE_LINKS) { // move the in-place links to the heap
			l = (Link*) malloc(sizeof (Link) * (last + 1));
			if (l) memcpy(l, position->links.in, sizeof (Link) * last);
		} else {
			l = (Link*) realloc(position->links.array, sizeof (Link) * (last + 1));
		}
		if (l == NULL) {
			error("cannot allocate opening book position's moves\n");
			return false;
		}
		l[last] = *link;
		position->links.array = l;
	}
	++position->n_link;

	if (link->score > position->score.value) position->score.value = link->score;

	if (link->move == position->leaf.move) position->leaf = BAD_LINK;

	return true;
}

/**
 * @brief Sort the link moves.
 *
 * @param position Position to sort.
 */
static void position_sort(Position *position)
{
	Link *i, *j, *best;

	if (position->n_link > 1) {
		Link *links = position_links(position);
		for (i = links; i < links + position->n_link - 1; ++i) {
			best = i;
			for (j = i + 1; j < links + position->n_link; ++j) {
				if (j->score > best->score) best = j;
			}
			if (best > i) {
				Link tmp = *best;
				*best = *i;
				*i = tmp;
			}
		}
	}
}

/**
 * @brief Evaluate a position.
 *
 * If needed, find the best remaining move, after link moves are excluded.
 *
 * @param position Position to search.
 * @param book Opening book.
 */
#ifdef BOOK_TEST_NODES
/* test builds: count the searches of the book functions and their nodes (printed after each negamax) */
static long long book_test_nodes = 0, book_test_searches = 0, book_test_play_nodes = 0;
void book_test_count_play(const unsigned long long n)
{
#if defined(_MSC_VER)
	_InterlockedExchangeAdd64(&book_test_play_nodes, (long long) n);
#else
	__atomic_add_fetch(&book_test_play_nodes, (long long) n, __ATOMIC_RELAXED);
#endif
}
static void book_test_count(const unsigned long long n)
{
#if defined(_MSC_VER)
	_InterlockedExchangeAdd64(&book_test_nodes, (long long) n);
	_InterlockedExchangeAdd64(&book_test_searches, 1);
#else
	__atomic_add_fetch(&book_test_nodes, (long long) n, __ATOMIC_RELAXED);
	__atomic_add_fetch(&book_test_searches, 1, __ATOMIC_RELAXED);
#endif
}
#endif

static int position_search_with(Position *position, Search *search);
static int position_search_planned(Position *position, Book *book);
static struct BookPlan *book_plan = NULL; /**< searches done ahead of book_add_board() (book-store-tasks); NULL: none */

static void position_search(Position *position, Book *book)
{
	const int r = book_plan ? position_search_planned(position, book) : position_search_with(position, book->search);

	if (r & 1) ++book->stats.n_links;
	if (r) book->need_saving = true;
}

/**
 * @brief Evaluate a position with a given search (no book access).
 *
 * @param position Position to search.
 * @param search Search to use.
 * @return 1 if the leaf became a link, | 2 if a search was done.
 */
static int position_search_with(Position *position, Search *search)
{
	Link *l;
	const int n_moves = get_mobility(position->board.player, position->board.opponent);
	long long time;
	bool time_per_move;
	int r = 0;

#ifdef BOOK_TEST_ISOLATE
	search_cleanup(search); // test builds: every search starts with empty hash tables, as the planned searches do
#endif
	if (position->leaf.move != NOMOVE && position_add_link(position, &position->leaf)) {
		r = 1;
	}

	if (position->n_link < n_moves || (position->n_link == 0 && n_moves == 0 && position->score.value == -SCORE_INF)) {
		search_set_board(search, &position->board, BLACK);
		search_set_level(search, position->level, search->eval.n_empties);

		foreach_link (l, position) {
			movelist_exclude(&search->movelist, l->move);
		}

		if (search->options.verbosity >= 2) {
			board_print(&search->board, search->player, stdout);
			puts(search->options.header);
			puts(search->options.separator);
		}

		time = search->options.time;
		time_per_move = search->options.time_per_move;
		search->options.time = TIME_MAX;
		search->options.time_per_move = true;

		search_run(search);
#ifdef BOOK_TEST_NODES
		book_test_count(search_count_nodes(search)); // test builds: nodes of the book searches
#endif

		search->options.time = time;
		search->options.time_per_move = time_per_move;

		position->leaf.score = search->result->score;
		position->leaf.move = search->result->move;
		if (position->leaf.score > position->score.value) {
			position->score.value = position->leaf.score;
		}
		r |= 2;
	}
	return r;
}

/**
 * @brief Link a position.
 *
 * Find moves that lead to other positions in the book.
 *
 * @param position Position to link.
 * @param book Opening book.
 */
static void position_link(Position *position, Book *book)
{
	int x;
	unsigned long long moves = board_get_moves(&position->board);
	Board next;
	Link link;
	Position *child;

	if (moves) {
		foreach_bit(x, moves) {
			board_next(&position->board, x, &next);
			child = book_probe(book, &next);
			if (child) {
				link.score = -child->score.value;
				link.move = x;
				book->stats.n_links += position_add_link(position, &link);
			}
		}
	} else if (can_move(position->board.opponent, position->board.player)) {// pass ?
		next.player = position->board.opponent;
		next.opponent = position->board.player;
		child = book_probe(book, &next);
		if (child) {
			link.score = -child->score.value;
			link.move = PASS;
			book->stats.n_links += position_add_link(position, &link);
		}
	}
}

/**
 * @brief Expand a position.
 *
 * Expand the best yet unlink move. This will add a new position to the book.
 * Two new moves will also be analyzed, one for the new position, the other for
 * the actual position as a new best unlink move.
 *
 * @param position Position to expand.
 * @param book Opening book.
 */
static void position_expand(Position *position, Book *book)
{
	Position child;

	if (position->leaf.move != NOMOVE) {
		position_init(&child);

		board_next(&position->board, position->leaf.move, &child.board);

		child.level = position->level;
		position_link(&child, book);
		search_cleanup(book->search);
		position_search(&child, book);
		position->leaf.score = -child.score.value;
		position_search(position, book);
		position_unique(&child);
		if (book_add(book, &child) <= 0) position_free(&child); // already in the book, or not added
	}
}

/**
 * @brief Negamax a position.
 *
 * Go through the book sub-tree following the current position & negamax the best scores back to this position.
 *
 * @param position Position to expand.
 * @param book Opening book.
 */
static int position_negamax(Position *position, Book *book)
{
	Link *l;
	Board target;
	Position *child;

	if (!position_is_done(position, book)) {
		GameStats stat = {0,0,0,0};
		const int n_empties = board_count_empties(&position->board);
		const int search_depth = LEVEL[position->level][n_empties].depth;
		const int bias = (search_depth & 1) - (n_empties & 1);

		position_set_done(position, book);

		position->score.value = position->score.lower = position->score.upper = -SCORE_INF;

		if (position->leaf.score > -SCORE_INF) {
			position->score.value = position->leaf.score;
			// is solving
			if (search_depth == n_empties && LEVEL[position->level][n_empties].selectivity == NO_SELECTIVITY) {
				position->score.lower = position->score.upper = position->score.value;
				if (position->leaf.score > 0) ++stat.n_wins;
				else if (position->leaf.score < 0) ++stat.n_losses;
				else ++stat.n_draws;
			// is pre-solving
			} else if (search_depth == n_empties) {
				position->score.lower = score_char(position->score.value - book->options.endcut_error);
				position->score.upper = score_char(position->score.value + book->options.endcut_error);
			} else { // midgame
				position->score.lower = score_char(position->score.value - book->options.midgame_error - bias);
				position->score.upper = score_char(position->score.value + book->options.midgame_error - bias);
			}
			++stat.n_lines;
		}

		foreach_link(l, position) {
			board_next(&position->board, l->move, &target);
			child = book_probe(book, &target);
			if (child == NULL) continue; // link to a missing position (corrupted book, see book_fix)
			position_negamax(child, book);
			if (l->score != -child->score.value) {
				l->score = -child->score.value;
				book->need_saving = true;
			}
			if (l->score > position->score.value) position->score.value = l->score;
			if (-child->score.upper > position->score.lower) position->score.lower = -child->score.upper;
			if (-child->score.lower > position->score.upper) position->score.upper = -child->score.lower;

			stat.n_wins += child->n_losses;
			stat.n_draws += child->n_draws;
			stat.n_losses += child->n_wins;
			stat.n_lines += child->n_lines;
		}

		position->n_wins = (unsigned int) MIN(UINT_MAX, stat.n_wins);
		position->n_draws = (unsigned int) MIN(UINT_MAX, stat.n_draws);
		position->n_losses = (unsigned int) MIN(UINT_MAX, stat.n_losses);
		position->n_lines = (unsigned int) MIN(UINT_MAX, stat.n_lines);
	}

	return position->score.value;
}

/*
 * Parallel negamax.
 *
 * The negamaxed values of a position only depend on its leaf and on the final
 * values of its children (max and sums), so the result does not depend on the
 * order in which positions are computed. Several threads walk the book from the
 * same root; a thread computes a position after claiming it (done = epoch|0x80),
 * the others help by walking its children and then wait for it. Links always go
 * to a position with fewer empties (or to the passed position, that cannot pass
 * back), so waiting cannot deadlock (the links of a damaged book that do not are
 * not followed: see negamax_link_target).
 */
#define NEGAMAX_MAX_LINKS 64

#if defined(_MSC_VER)
#include <intrin.h>
static inline bool atomic_cas_uchar(unsigned char *p, const unsigned char from, const unsigned char to)
{
	return (unsigned char) _InterlockedCompareExchange8((volatile char*) p, (char) to, (char) from) == from;
}
#if defined(_M_X64) || defined(_M_IX86)
// x86 loads/stores already have acquire/release ordering: only stop the compiler
static inline unsigned char atomic_load_uchar(const unsigned char *p)
{
	const unsigned char v = *(volatile const unsigned char*) p;
	_ReadWriteBarrier();
	return v;
}
static inline void atomic_store_uchar(unsigned char *p, const unsigned char v)
{
	_ReadWriteBarrier();
	*(volatile unsigned char*) p = v;
}
#else // ARM64: use interlocked (full barrier) operations
static inline unsigned char atomic_load_uchar(const unsigned char *p)
{
	return (unsigned char) _InterlockedOr8((volatile char*) p, 0);
}
static inline void atomic_store_uchar(unsigned char *p, const unsigned char v)
{
	_InterlockedExchange8((volatile char*) p, (char) v);
}
#endif
#else
static inline bool atomic_cas_uchar(unsigned char *p, unsigned char from, const unsigned char to)
{
	return __atomic_compare_exchange_n(p, &from, to, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
}
static inline unsigned char atomic_load_uchar(const unsigned char *p) { return __atomic_load_n(p, __ATOMIC_ACQUIRE); }
static inline void atomic_store_uchar(unsigned char *p, const unsigned char v) { __atomic_store_n(p, v, __ATOMIC_RELEASE); }
#endif

/**
 * @brief Compute the negamaxed values of a position from its (already negamaxed) children.
 * Same computation as position_negamax().
 */
static void position_negamax_compute(Position *position, Book *book, Position **children)
{
	GameStats stat = {0,0,0,0};
	const int n_empties = board_count_empties(&position->board);
	const int search_depth = LEVEL[position->level][n_empties].depth;
	const int bias = (search_depth & 1) - (n_empties & 1);
	Link *l = position_links(position);
	int i;

	position->score.value = position->score.lower = position->score.upper = -SCORE_INF;

	if (position->leaf.score > -SCORE_INF) {
		position->score.value = position->leaf.score;
		if (search_depth == n_empties && LEVEL[position->level][n_empties].selectivity == NO_SELECTIVITY) {
			position->score.lower = position->score.upper = position->score.value;
			if (position->leaf.score > 0) ++stat.n_wins;
			else if (position->leaf.score < 0) ++stat.n_losses;
			else ++stat.n_draws;
		} else if (search_depth == n_empties) {
			position->score.lower = score_char(position->score.value - book->options.endcut_error);
			position->score.upper = score_char(position->score.value + book->options.endcut_error);
		} else {
			position->score.lower = score_char(position->score.value - book->options.midgame_error - bias);
			position->score.upper = score_char(position->score.value + book->options.midgame_error - bias);
		}
		++stat.n_lines;
	}

	for (i = 0; i < position->n_link; ++i) {
		const Position *child = children[i];
		if (child == NULL) continue; // dangling link (corrupted book)
		if (l[i].score != -child->score.value) {
			l[i].score = -child->score.value;
			book->need_saving = true;
		}
		if (l[i].score > position->score.value) position->score.value = l[i].score;
		if (-child->score.upper > position->score.lower) position->score.lower = -child->score.upper;
		if (-child->score.lower > position->score.upper) position->score.upper = -child->score.lower;

		stat.n_wins += child->n_losses;
		stat.n_draws += child->n_draws;
		stat.n_losses += child->n_wins;
		stat.n_lines += child->n_lines;
	}

	position->n_wins = (unsigned int) MIN(UINT_MAX, stat.n_wins);
	position->n_draws = (unsigned int) MIN(UINT_MAX, stat.n_draws);
	position->n_losses = (unsigned int) MIN(UINT_MAX, stat.n_losses);
	position->n_lines = (unsigned int) MIN(UINT_MAX, stat.n_lines);
}

/**
 * @brief Position that a link leads to, for the parallel negamax.
 *
 * The threads walk the links without marking where they are, and wait for each other: a link of a
 * damaged book that leads back to its own position, or to a position above it (a pass that is not
 * one, a move on an occupied square), would be walked without end (stack overflow), where
 * position_negamax() stops at the positions that it has already seen. So only the links that make the
 * game progress are followed: a move that adds a disc, or the pass of a player who cannot move to a
 * player who can. Every link of a valid book is one of them.
 *
 * @param position Position.
 * @param link Link of the position.
 * @param book Opening book.
 * @return the position, or NULL if it is not in the book or if the link is not followed.
 */
static Position* negamax_link_target(const Position *position, const Link *link, const Book *book)
{
	const Board *board = &position->board;
	Board target;

	if (link->move <= H8) {
		board_next(board, link->move, &target);
		if (bit_count(target.player | target.opponent) <= bit_count(board->player | board->opponent)) return NULL;
	} else if (link->move == PASS) {
		if (can_move(board->player, board->opponent) || !can_move(board->opponent, board->player)) return NULL;
		target.player = board->opponent;
		target.opponent = board->player;
	} else {
		return NULL;
	}
	return book_probe(book, &target);
}

static void position_negamax_parallel(Position *position, Book *book, const int id)
{
	const unsigned char done = book->epoch | POSITION_DONE, busy = book->epoch | POSITION_BUSY;
	const unsigned char d = atomic_load_uchar(&position->state);
	Position *children[NEGAMAX_MAX_LINKS];
	const Link *l;
	int i, n, first;
	bool own;

	if (d == done) return;
	own = (d != busy && atomic_cas_uchar(&position->state, d, busy));

	n = position->n_link;
	if (n > NEGAMAX_MAX_LINKS) fatal_error("too many links\n");
	l = position_links(position);
	for (i = 0; i < n; ++i) children[i] = negamax_link_target(position, l + i, book);
	// threads start with different children to spread the work
	first = n ? (id * 7 + board_count_empties(&position->board)) % n : 0;
	for (i = 0; i < n; ++i) {
		Position *child = children[(first + i) % n];
		if (child && atomic_load_uchar(&child->state) != done) position_negamax_parallel(child, book, id);
	}

	if (own) {
		position_negamax_compute(position, book, children);
		atomic_store_uchar(&position->state, done);
	} else {
		int spin = 0;
		while (atomic_load_uchar(&position->state) != done) {
			if (++spin < 64) {
#if defined(_M_X64) || defined(_M_IX86)
				_mm_pause();
#elif defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
				__builtin_ia32_pause(); // same as _mm_pause(), also without SSE headers (-march=i386)
#endif
			} else {
				spin = 0;
#ifdef _WIN32
				Sleep(0);
#else
				sched_yield();
#endif
			}
		}
	}
}

typedef struct NegamaxWorker {
	Book *book;
	Position *root;
	int id;
} NegamaxWorker;

static void* negamax_worker(void *v)
{
	NegamaxWorker *w = (NegamaxWorker*) v;
	position_negamax_parallel(w->root, w->book, w->id);
	return NULL;
}

/**
 * @brief Negamax the book sub-tree of a position with options.n_task threads.
 *
 * @param root Position to negamax (book_clean() must have been called).
 * @param book Opening book.
 */
static void book_negamax_position(Position *root, Book *book)
{
	NegamaxWorker w[MAX_THREADS];
	int i, n = options.n_task;

	if (n > MAX_THREADS) n = MAX_THREADS;
	if (n <= 1) {
		position_negamax(root, book);
		return;
	}
	for (i = 0; i < n; ++i) {
		w[i].book = book; w[i].root = root; w[i].id = i;
	}
	book_run_workers(negamax_worker, w, sizeof *w, n);
}


/**
 * @brief Prune a position.
 *
 * @param position Position to prune.
 * @param book Opening book.
 * @param player_deviation Player's error.
 * @param opponent_deviation Opponent's error.
 * @param lower Error lower bound.
 * @param upper Error upper bound.
 */
static void position_prune(Position *position, Book *book, const int player_deviation, const int opponent_deviation, const int lower, const int upper)
{
	Link *l;
	Board target;
	Position *child;

	// if position is not done yet & good enough & inside the book height limit
	if (lower <= position->score.value && position->score.value <= upper && board_count_empties(&position->board) >= book->options.n_empties - 1) {
		position_set_done(position, book); book->stats.n_todo++;

		// prune all children close to the best move
		foreach_link(l, position) {
			if (position->score.value - l->score <= player_deviation && lower <= l->score && l->score <= upper) {
				board_next(&position->board, l->move, &target);
				child = book_probe(book, &target);
				if (child) position_prune(child, book, opponent_deviation, player_deviation, -upper, -lower);
			}
		}
		if (book->stats.n_todo % BOOK_INFO_RESOLUTION == 0) {
			bprint("Book prune %lld to keep\r", book->stats.n_todo);
			
		}
	}
}

/**
 * @brief Remove bad links after book pruning.
 *
 * @param position Position to fix.
 * @param book Opening book.
 */
static void position_remove_links(Position *position, Book *book)
{
	int i, n = 0;
	const Link *l = position_links(position);
	Link kept[256];
	Board target;

	for (i = 0; i < position->n_link; ++i) {
		board_next(&position->board, l[i].move, &target);
		if (!book_probe(book, &target)) {
			if (l[i].score > position->leaf.score) position->leaf = l[i];
		} else {
			kept[n++] = l[i];
		}
	}
	if (n != position->n_link && !position_set_links(position, kept, n)) {
		error("cannot allocate opening book position's moves\n");
	}
}

/**
 * @brief Check if a position links to a position missing from the book.
 *
 * @param position Position.
 * @param book Opening book.
 * @return true if a link leads to a missing position.
 */
static bool position_has_missing_link(const Position *position, const Book *book)
{
	const Link *l;
	Board target;

	foreach_link(l, position) {
		board_next(&position->board, l->move, &target);
		if (!book_probe(book, &target)) return true;
	}
	return false;
}

/**
 * @brief Deviate a position.
 *
 * This is the important part of the opening book code, where it finds the best moves to add to the book.
 * Considering the current position, it will deviate a child position or expand a move if:
 * - move_score < best_score - player_deviation
 * - lower <= move_score <= upper.
 * So a good candidate move for expansion need to have a score close to the best move, and not too far
 * from the root score from where we deviates, in order to not derive to very bad positions.
 *
 * @param position Position to expand.
 * @param book Opening book.
 * @param player_deviation Player's error.
 * @param opponent_deviation Opponent's error.
 * @param lower Error lower bound.
 * @param upper Error upper bound.
 */
static void position_deviate(Position *position, Book *book, const int player_deviation, const int opponent_deviation, const int lower, const int upper)
{
	Link *l;
	Board target;
	Position *child;

	// if position is not done yet & good enough & inside the book height limit
	if (!position_is_done(position, book) && lower <= position->score.value && position->score.value <= upper && board_count_empties(&position->board) >= book->options.n_empties && !board_is_game_over(&position->board)) {
		position_set_done(position, book);

		// deviate all children close to the best move
		foreach_link(l, position) {
			if (position->score.value - l->score <= player_deviation && lower <= l->score && l->score <= upper) {
				board_next(&position->board, l->move, &target);
				child = book_probe(book, &target);
				if (child) position_deviate(child, book, opponent_deviation, player_deviation, -upper, -lower);
			}
		}

		// expand the best remaining move
		if (position->score.value - position->leaf.score <= player_deviation && lower <= position->leaf.score && position->leaf.score <= upper) {
			book_mark_todo(book, position); book->stats.n_todo++;
			if (book->stats.n_todo % 10 == 0) bprint("Book deviate %lld todo\r", book->stats.n_todo);
		}
	}
}

/**
 * @brief Deviate a position with a cumulative move-loss limit.
 *
 * Each move loss is the difference between the best score at the position
 * and the score of the selected move. The path is followed only while every
 * move loss is within move_loss and their sum is within total_loss.
 *
 * @param position Position to expand.
 * @param book Opening book.
 * @param move_loss Maximum loss for one move.
 * @param total_loss Maximum cumulative loss for both players.
 * @param loss Accumulated loss from the root to this position.
 */
/*
 * Visit marks of the deviate/deviate2/deviate3 selection walks: one byte per position, allocated
 * for a walk only (up to v4.5.5-nikque.4, a 4-byte field of every position kept them):
 * - deviate2/3: smallest accumulated loss + 1 (the loss is at most VISIT_LOSS_MAX),
 * - deviate: ply parity + 1,
 * - 0: not visited.
 * No position is added or moved during a walk.
 */
#define VISIT_LOSS_MAX 254

static inline unsigned char* book_visit(const Book*, const Position*);

static void position_deviate_total(Position *position, Book *book, const int move_loss, const int total_loss, const int loss, const bool skip_solved)
{
	Link *l;
	Board target;
	Position *child;
	int move_error;

	if (loss > total_loss || board_count_empties(&position->board) < book->options.n_empties || board_is_game_over(&position->board)) return;

	// A transposed position can be reached by different lines. Revisit it only
	// when this path has a smaller accumulated loss, which leaves more budget
	// available to every continuation from the same board.
	{
		unsigned char *v = book_visit(book, position);
		if (*v && loss >= *v - 1) return;
		position_set_done(position, book);
		*v = (unsigned char) (loss + 1);
	}

	foreach_link(l, position) {
		move_error = position->score.value - l->score;
		if (0 <= move_error && move_error <= move_loss && loss + move_error <= total_loss) {
			board_next(&position->board, l->move, &target);
			child = book_probe(book, &target);
			if (child) position_deviate_total(child, book, move_loss, total_loss, loss + move_error, skip_solved);
		}
	}

	// A solved leaf already has an exact score. Keep following existing links,
	// since descendants may have been searched at a lower level.
	const int n_empties = board_count_empties(&position->board);
	if (skip_solved && LEVEL[position->level][n_empties].depth == n_empties
		&& LEVEL[position->level][n_empties].selectivity == NO_SELECTIVITY) return;

	move_error = position->score.value - position->leaf.score;
	if (position->leaf.move != NOMOVE && 0 <= move_error && move_error <= move_loss && loss + move_error <= total_loss && !position_is_todo(position, book)) {
		book_mark_todo(book, position);
		book->stats.n_todo++;
		if (book->stats.n_todo % 10 == 0) bprint("Book deviate%d %lld todo\r", skip_solved ? 2 : 3, book->stats.n_todo);
	}
}

/**
 * @brief Enhance a position.
 *
 * This is the other important part of the opening book code, where it finds the best moves to add to the book by
 * another methods.
 * Here we use score negamaxed bounds to decide if a move is a good candidate for further enhancement or for expansion.
 * The purpose of this algorithm is to find more moves.
 *
 *
 * @param position Position to expand.
 * @param book Opening book.
 */
static void position_enhance(Position *position, Book *book)
{
	Link *l;
	Board target;
	Position *child;

	if (!position_is_done(position, book) && board_count_empties(&position->board) >= book->options.n_empties && !board_is_game_over(&position->board)) {
		position_set_done(position, book);

		foreach_link(l, position) {
			board_next(&position->board, l->move, &target);
			child = book_probe(book, &target);
			if (child && (-child->score.upper >= position->score.lower || -child->score.lower >= position->score.upper)) {
				position_enhance(child, book);
			}
		}

		if (position->leaf.score > -SCORE_INF) {
			const int n_empties = board_count_empties(&position->board);
			const int search_depth = LEVEL[position->level][n_empties].depth;
			const int bias = (search_depth & 1) - (n_empties & 1);
			int lower, upper;
			// is solving
			if (search_depth == n_empties && LEVEL[position->level][n_empties].selectivity == NO_SELECTIVITY) {
				lower = upper = position->leaf.score;
			// is pre-solving
			} else if (search_depth == n_empties) {
				lower = position->leaf.score - book->options.endcut_error;
				upper = position->leaf.score + book->options.endcut_error;
			} else { // midgame
				lower = position->leaf.score - book->options.midgame_error - bias;
				upper = position->leaf.score + book->options.midgame_error - bias;
			}

			if (lower >= position->score.lower || upper >= position->score.upper) {
				book_mark_todo(book, position);
			}
		}
	}
}

/**
 * @brief Feed hash from a position.
 *
 * Go through the book sub-tree following the current position & feed the hash table from this position.
 *
 * @param board Position to expand.
 * @param book Opening book.
 * @param search Hashtables container.
 * @param is_pv Flag to tell if the position is from the principal variation.
 */
static void board_feed_hash(Board *board, const Book *book, Search *search, const bool is_pv)
{
	Position *position;
	const unsigned long long hash_code = board_get_hash_code(board);
	MoveList movelist;
	Move *m;
	HashStoreData hash_data;

	position = book_probe(book, board);
	if (position) {
		const int n_empties = board_count_empties(&position->board);
		const int score = position->score.value;
		int move = NOMOVE;

		hash_data.data.wl.c.depth = LEVEL[position->level][n_empties].depth;
		hash_data.data.wl.c.selectivity = LEVEL[position->level][n_empties].selectivity;

		position_get_moves(position, board, &movelist);
		foreach_move(m, movelist) {
			if (move == NOMOVE) move = m->x;
			board_update(board, m);
				board_feed_hash(board, book, search, is_pv && m->score == score);
			board_restore(board, m);
		}

		hash_data.data.lower = hash_data.data.upper = score;
		hash_data.data.move[0] = move;
		hash_feed(&search->hash_table, board, hash_code, &hash_data);
		if (is_pv) hash_feed(&search->pv_table, board, hash_code, &hash_data);
	}
}

/**
 * @brief Fill the opening book.
 *
 * Add positions to link existing positions.
 *
 * @param board Candidate position.
 * @param book Opening book.
 * @param depth Depth at which to search a link.
 * @return true if the board is in the book, possibly just after having been added to it.
 */
static bool board_fill(Board *board, Book *book, int depth)
{
	if (depth > 0) {
		MoveList movelist;
		Move *m;
		bool filled = false;

		movelist_get_moves(&movelist, board);
		if (movelist.n_moves == 0 && can_move(board->opponent, board->player)) {
			board_pass(board);
			if (board_fill(board, book, depth - 1)) {
				book_add_board(book, board);
				filled = true;
			}
			board_pass(board);							
		} else {
			foreach_move(m, movelist) {
				board_update(board, m);
				if (board_fill(board, book, depth - 1)) {
					book_add_board(book, board);
					filled = true;
				}
				board_restore(board, m);					
			}
		}
		return filled;
	}
	return book_probe(book, board) != NULL;
}

/**
 * @brief Fix a position.
 *
 * Recompute all elements of a buggy position
 *
 * @param position Position to fix.
 * @param book Opening book.
 */
static void position_fix(Position *position, Book *book)
{
	Board board;

	if ((position->board.player & position->board.opponent) || 
	    ((position->board.player | position->board.opponent) & 0x0000001818000000ULL) != 0x0000001818000000ULL) {
		position_free(position);
		position_init(position);
		return;
	}
	board_unique(&position->board, &board);
	position_free(position);
	position_init(position);
	position->board = board;
	position->level = book->options.level;
	position_link(position, book);
	position_search(position, book);
}

/**
 * @brief An array with positions.
 *
 * size < 0: the positions are stored in the book pool filled at load time
 * (capacity == n); the array moves to its own heap block when it grows.
 */
typedef struct PositionArray {
	Position *positions;
	int n;
	int size;
} PositionArray;

/**
 * @brief Allocate the visit marks (all 0) of a walk.
 * @param book Opening book.
 * @return true on success.
 */
static bool book_visit_init(Book *book)
{
	unsigned int i, n = 0;

	book->visit_first = (unsigned int*) malloc(book->n * sizeof *book->visit_first);
	if (book->visit_first) {
		for (i = 0; i < (unsigned int) book->n; ++i) {
			book->visit_first[i] = n;
			n += (unsigned int) book->array[i].n;
		}
		book->visit = (unsigned char*) calloc(n ? n : 1, 1);
	}
	if (book->visit == NULL) {
		free(book->visit_first); book->visit_first = NULL;
		error("cannot allocate the marks of the book walk");
		book->failed = true;
		return false;
	}
	return true;
}

/** @brief Clear the visit marks (for a new walk). */
static void book_visit_clear(Book *book)
{
	const PositionArray *a = book->array + book->n - 1;
	memset(book->visit, 0, book->visit_first[book->n - 1] + (size_t) a->n);
}

/** @brief Free the visit marks. */
static void book_visit_free(Book *book)
{
	free(book->visit); book->visit = NULL;
	free(book->visit_first); book->visit_first = NULL;
}

/** @return the visit mark of a position. */
static inline unsigned char* book_visit(const Book *book, const Position *p)
{
	const unsigned long long i = board_get_hash_code(&p->board) & (book->n - 1);
	return book->visit + book->visit_first[i] + (p - book->array[i].positions);
}



/**
 * @brief Initialize the array.
 *
 * @param a Positions' array.
 */
static void position_array_init(PositionArray *a)
{
	a->size = a->n = 0;
	a->positions = NULL;
}

/**
 * @brief Add a position to the array.
 *
 * @param a Positions' array.
 * @param p Position to add.
 * @param epoch Current book epoch.
 * @return 1 if the position was added, 0 if it is already there, -1 if memory is exhausted.
 */
static int position_array_add(PositionArray *a, const Position *p, const unsigned char epoch)
{
	int i;

	board_check(&p->board);
	assert(position_is_ok(p));

	for (i = 0; i < a->n; ++i) if (board_equal(&a->positions[i].board, &p->board)) return 0;
	if (a->size < 0 || a->n == a->size) {
		Position *n;
		const int size = a->n + a->n / 2 + 1;
		if (a->size < 0) { // the array lives in the book pool: move it to its own block
			n = (Position*) malloc(size * sizeof (Position));
			if (n) memcpy(n, a->positions, a->n * sizeof (Position));
		} else {
			n = (Position*) realloc(a->positions, size * sizeof (Position));
		}
		if (n == NULL) {
			error("cannot add a position to the book\n");
			return -1;
		}
		a->positions = n;
		a->size = size;
	}
	a->positions[a->n] = *p;
	a->positions[a->n].state = epoch | POSITION_DONE; // a new position is 'done' until the next book_clean()
	++a->n;
	return 1;
}

/**
 * @brief Remove a position from an array.
 *
 * @param a Positions' array.
 * @param p Position to add.
 * @return true in case of success.
 */
static bool position_array_remove(PositionArray *a, const Position *p)
{
	int i, j;

	for (i = 0; i < a->n; ++i) {
		if (board_equal(&a->positions[i].board, &p->board)) {
			position_free(a->positions + i);
			for (j = i + 1; j < a->n; ++j) {
				a->positions[j - 1] = a->positions[j];
			}
			--a->n;
			return true;
		}
	}
	return false;
}

/**
 * @brief Free resources used by a position array.
 *
 * @param a Positions' array.
 */
static void position_array_free(PositionArray *a)
{
	int i;
	for (i = 0; i < a->n; ++i) position_free(a->positions + i);
	if (a->size >= 0) free(a->positions); // else: part of the book pool
}

/**
 * @brief Find a position in the array.
 *
 * @param a Positions' array.
 * @param board Board to find in the array.
 * @return a position containg the board (or a symetry) or NULL is no position is found.
 */
static Position* position_array_probe(PositionArray *a, const Board *board)
{
	int i;
	for (i = 0; i < a->n; ++i) if (board_equal(&a->positions[i].board, board)) return a->positions + i;
	return NULL;
}

#define foreach_position(p, a, b) \
	for (a = b->array; a < b->array + b->n; ++a) \
	for (p = a->positions; p < a->positions + a->n; ++p)

/**
 * @brief Set book date.
 *
 * @param book Opening book.
 */
static void book_set_date(Book *book)
{
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);

	memset(&book->date, 0, sizeof book->date); // the padding byte is saved too
	book->date.year = tm->tm_year + 1900;
	book->date.month = tm->tm_mon + 1;
	book->date.day = tm->tm_mday;
	book->date.hour = tm->tm_hour;
	book->date.minute = tm->tm_min;
	book->date.second = tm->tm_sec;
}

/**
 * @brief Get book age, in seconds.
 *
 * @param book Opening book.
 * @return book age.
 */
static double book_get_age(Book *book)
{
	struct tm tm;
	double t;

	tm.tm_year = book->date.year - 1900;
	tm.tm_mon = book->date.month - 1;
	tm.tm_mday = book->date.day;
	tm.tm_hour = book->date.hour;
	tm.tm_min = book->date.minute;
	tm.tm_sec = book->date.second;
	tm.tm_isdst = -1;

	t = difftime(time(NULL), mktime(&tm));

	return t;
}



/**
 * @brief Find a position in the book.
 *
 * @param book Opening book.
 * @param board Board to find in the array.
 * @return a position containg the board (or a symetry) or NULL is no position is found.
 */
static Position* book_probe(const Book *book, const Board *board)
{
	Board unique;
	board_unique(board, &unique);
	return position_array_probe(book->array + (board_get_hash_code(&unique) & (book->n - 1)), &unique);
}

/**
 * @brief Add a position to the book.
 *
 * A failure (memory exhausted, or more positions than the file format allows)
 * sets book->failed, which stops the learning loops.
 *
 * @param book Opening book.
 * @param p Position to add.
 * @return 1 if the position was added, 0 if it is already in the book, -1 on failure.
 *         On 0 or -1 the caller keeps the ownership of the position's links.
 */
static int book_add(Book *book, const Position *p)
{
	const unsigned long long i = board_get_hash_code(&p->board) & (book->n - 1);
	int r;

	if (book->n_nodes == UINT_MAX) {
		error("the book cannot hold more than %u positions\n", UINT_MAX);
		book->failed = true;
		return -1;
	}
	r = position_array_add(book->array + i, p, book->epoch);
	if (r > 0) {
		++book->n_nodes;
		++book->stats.n_nodes;
	} else if (r < 0) {
		book->failed = true;
	}
	return r;
}

/**
 * @brief Remove a position from the book.
 *
 * @param book Opening book.
 * @param p Position to add.
 */
static void book_remove(Book *book, const Position *p)
{
	const unsigned long long i = board_get_hash_code(&p->board) & (book->n - 1);

	if (position_array_remove(book->array + i, p)) {
		--book->n_nodes;
		--book->stats.n_nodes;
	}
}

/**
 * @brief Mark a position to be expanded and remember where it is.
 *
 * Positions never move inside their bucket while a book is being deviated
 * (new positions are appended), so book_expand() can visit the marked
 * positions in the original bucket order without scanning the whole book.
 *
 * @param book Opening book.
 * @param p Position (must belong to the book).
 */
static void book_mark_todo(Book *book, Position *p)
{
	position_set_todo(p, book);
	if (book->todo_list.valid) {
		const unsigned long long i = board_get_hash_code(&p->board) & (book->n - 1);
		if (book->todo_list.n == book->todo_list.size) {
			const long long size = book->todo_list.size + book->todo_list.size / 2 + 1024;
			unsigned long long *item = (unsigned long long*) realloc(book->todo_list.item, size * sizeof *item);
			if (item == NULL) { book->todo_list.valid = false; return; } // fall back to a full scan
			book->todo_list.item = item;
			book->todo_list.size = size;
		}
		book->todo_list.item[book->todo_list.n++] = (i << 32) | (unsigned long long) (p - book->array[i].positions);
	}
}

static int todo_item_cmp(const void *a, const void *b)
{
	const unsigned long long x = *(const unsigned long long*) a, y = *(const unsigned long long*) b;
	return (x > y) - (x < y);
}
/**
 * @brief Set all positions as undone.
 *
 * @param book Opening book.
 */
static void book_clean(Book *book)
{
	PositionArray *a;
	Position *p;
	book->stats.n_nodes = book->stats.n_links = book->stats.n_todo = 0;
	book->todo_list.n = 0;
	book->todo_list.valid = true;
	if (++book->epoch > POSITION_EPOCH_MAX) {
		foreach_position(p, a, book) p->state = 0;
		book->epoch = 1;
	}
}

/**
 * @brief Find the initial position in the book.
 *
 * Attention: when a position is added to the book, the pointer
 * returned by this position may be wrong. If the root position is updated
 * the contents of the pointed structure may be wrong. So it is needed to
 * recall this function each time as necessary.
 *
 * @param book Opening book.
 * @return the inital position.
 */
static Position *book_root(Book *book)
{
	Board board;

	board_init(&board);
	return book_probe(book, &board);
}

/**
 * @brief Initialize the opening book.
 *
 * Create an empty opening book.
 *
 * @param book Opening book.
 */
void book_init(Book *book)
{
	int i;

	book_set_date(book);

	book->options.level = 21;
	book->options.n_empties = 24;
	book->options.midgame_error = 2;
	book->options.endcut_error = 1;

	book->n = 65536;
	book->array = (PositionArray*) malloc(book->n * sizeof *book->array);
	if (book->array == NULL) fatal_error("cannot allocate space to store the positions");
	for (i = 0; i < book->n; ++i) position_array_init(book->array + i);

	book->n_nodes = 0;
	book->failed = false;
	book->epoch = 1;
	book->todo_list.item = NULL;
	book->todo_list.n = book->todo_list.size = 0;
	book->todo_list.valid = false;
	book->pool = NULL;
	book->visit = NULL;
	book->visit_first = NULL;
	random_seed(&book->random, real_clock());
	book->need_saving = false;
}

/**
 * @brief Number of buckets for a book of n positions.
 *
 * About 16 positions per bucket or less (65536 buckets at least). The upper
 * limit used to be 2^26 buckets (1.07 billion positions); books of any size
 * that the file format can hold now keep short buckets.
 *
 * @param n Number of positions.
 * @return Number of buckets (a power of 2).
 */
static int book_bucket_count(const long long n)
{
	int b = 65536;
	while (b < (1 << 28) && (long long) b * 16 < n) b <<= 1;
	return b;
}

/**
 * @brief Increase the number of buckets of a book.
 *
 * Used before adding many positions to a book with too few buckets (e.g. a
 * new book merged with a large one): every book search scans a bucket, so the
 * time grows with the number of positions per bucket. Positions keep their
 * relative order (bucket order, then position order).
 *
 * @param book Opening book.
 * @param n_positions Expected number of positions.
 */
static void book_grow_buckets(Book *book, const long long n_positions)
{
	const int n = book_bucket_count(n_positions);
	PositionArray *array, *a;
	Position *p;
	int i;

	if (n_positions <= (long long) book->n * 32 || n <= book->n) return; // not worth moving the book
	array = (PositionArray*) malloc(n * sizeof *array);
	if (array == NULL) return; // keep the current buckets
	for (i = 0; i < n; ++i) position_array_init(array + i);
	foreach_position(p, a, book) {
		PositionArray *b = array + (board_get_hash_code(&p->board) & (n - 1));
		if (b->n == b->size) {
			const int size = b->size + b->size / 2 + 1;
			Position *q = (Position*) realloc(b->positions, size * sizeof (Position));
			if (q == NULL) { // keep the current buckets
				for (i = 0; i < n; ++i) free(array[i].positions);
				free(array);
				return;
			}
			b->positions = q; b->size = size;
		}
		b->positions[b->n++] = *p; // the links move with the position
	}
	for (a = book->array; a < book->array + book->n; ++a) if (a->size >= 0) free(a->positions);
	free(book->array);
	free(book->pool);
	book->pool = NULL;
	book->array = array;
	book->n = n;
	book->todo_list.valid = false; // positions moved
	book->todo_list.n = 0;
}

/**
 * @brief Free resources used by the opening book.
 *
 * @param book Opening book.
 */
void book_free(Book *book)
{
	int i;
	for (i = 0; i < book->n; ++i) {
		position_array_free(book->array + i);
	}
	free(book->array);
	free(book->pool);
	book->pool = NULL;
	free(book->todo_list.item);
	book->todo_list.item = NULL;
	book->todo_list.n = book->todo_list.size = 0;
	book->todo_list.valid = false;
	book_store_release();
}

/**
 * @brief Create a new opening book.
 *
 * Create an opening book with the initial position & a single non link move.
 *
 * @param book Opening book.
 * @param level search level to evaluate positions.
 * @param n_empties number of empty positions up to which to evaluate positions.
 */
void book_new(Book *book, int level, int n_empties)
{
	Board board;

	bprint("New book %d %d...", level, n_empties);
	book_init(book);
	book->options.level = level;
	book->options.n_empties = n_empties;

	board_init(&board);
	book_add_board(book, &board);
	bprint("...done>\n");
	book->need_saving = true;
}

/**
 * @brief Number of bytes left in a file.
 *
 * @param f File (seekable).
 * @return bytes from the current position to the end, -1 on error.
 */
static long long book_file_remaining(FILE *f)
{
	long long pos, end;
#ifdef _WIN32
	pos = _ftelli64(f);
	if (pos < 0 || _fseeki64(f, 0, SEEK_END) != 0) return -1;
	end = _ftelli64(f);
	if (_fseeki64(f, pos, SEEK_SET) != 0) return -1;
#else
	pos = (long long) ftello(f);
	if (pos < 0 || fseeko(f, 0, SEEK_END) != 0) return -1;
	end = (long long) ftello(f);
	if (fseeko(f, (off_t) pos, SEEK_SET) != 0) return -1;
#endif
	return end - pos;
}

/**
 * @brief Apply the book depth of the settings (book-depth) to the book loaded at startup.
 *
 * With book-depth = auto (0), the depth saved in the book file is kept. Otherwise the book
 * depth is set as with the "book depth" command (and saved with the book).
 *
 * @param book Opening book.
 */
void book_set_startup_depth(Book *book)
{
	if (options.book_depth > 0) book->options.n_empties = 61 - options.book_depth;
}

/**
 * @brief Load the opening book.
 *
 * @param book Opening book.
 * @param file File name.
 */
bool book_load(Book *book, const char *file)
{
	FILE *f = fopen(file, "rb");
	if (f) {
		Book loaded = {0};
		BookStream stream = {0};
		Position p, *pool = NULL;
		unsigned int used = 0;
		int last_bucket = -1;
		bool pooling;
		unsigned int header_edax, header_book;
		unsigned char header_version, header_release;
		unsigned int i, expected;	// the position count is saved as a 32-bit unsigned int
		int r, j;
		loaded.search = book->search;
		loaded.epoch = 1;

		info("Loading book from %s...", file);
		r = fread(&header_edax, sizeof (unsigned int), 1, f);
		r += fread(&header_book, sizeof (unsigned int), 1, f);
		if (r != 2 || header_edax != EDAX || header_book != BOOK) {
			error("%s is not an edax opening book", file);
			goto book_load_failed;
		}

		r = fread(&header_version, 1, 1, f);
		r += fread(&header_release, 1, 1, f);
		if (r != 2 || header_version != VERSION) {
			error("%s is not a compatible version", file);
			goto book_load_failed;
		}

		r = fread(&loaded.date, sizeof loaded.date, 1, f);
		r += fread(&loaded.options, sizeof loaded.options, 1, f);
		r += fread(&expected, sizeof expected, 1, f);
		if (r != 3) {
			error("Cannot read book settings from %s", file);
			goto book_load_failed;
		}
		{	// every position takes at least POSITION_FIXED_SIZE bytes and its leaf: reject a count the file cannot hold
			const long long remaining = book_file_remaining(f);
			if (remaining < 0 || (long long) expected > remaining / (long long) (POSITION_FIXED_SIZE + sizeof (Link))) {
				error("Invalid position count in %s", file);
				goto book_load_failed;
			}
		}

		loaded.n = book_bucket_count(expected);

		loaded.array = (PositionArray*) malloc(loaded.n * sizeof (PositionArray));
		if (loaded.array == NULL) {
			error("cannot allocate space to store the positions");
			goto book_load_failed;
		}
		for (j = 0; j < loaded.n; ++j) position_array_init(loaded.array + j);

		if (!book_stream_open(&stream, f)) {
			error("cannot allocate the book read buffer");
			goto book_load_failed;
		}
		// A saved book lists its positions bucket by bucket: store them contiguously
		// in one pool, exactly sized (no per-bucket block, no spare capacity).
		// Positions out of bucket order fall back to the usual growing arrays.
		pool = (expected > 0) ? (Position*) malloc((size_t) expected * sizeof (Position)) : NULL;
		loaded.pool = pool;
		pooling = (pool != NULL);
		for (i = 0; i < expected; ++i) {
			if (!position_read(&p, &stream)) {
				error("Truncated opening book %s at position %u/%u", file, i, expected);
				goto book_load_failed;
			}
			if (pooling) {
				const int b = (int) (board_get_hash_code(&p.board) & (loaded.n - 1));
				PositionArray *a = loaded.array + b;
				if (b < last_bucket) { // not saved in this bucket order: stop using the pool
					pooling = false;
					if (used < expected / 2) { // mostly unused: move what it holds and release it now
						PositionArray *c;
						for (c = loaded.array; c < loaded.array + loaded.n; ++c) {
							if (c->size < 0) {
								Position *q = (Position*) malloc(c->n * sizeof (Position));
								if (q == NULL) { error("cannot allocate space to store the positions"); goto book_load_failed; }
								memcpy(q, c->positions, c->n * sizeof (Position));
								c->positions = q; c->size = c->n;
							}
						}
						free(pool);
						loaded.pool = pool = NULL;
						used = 0;
					}
				}
				if (pooling && (a->n == 0 || (a->size < 0 && a->positions + a->n == pool + used))) {
					int k;
					for (k = 0; k < a->n; ++k) if (board_equal(&a->positions[k].board, &p.board)) break;
					if (k == a->n) {
						if (a->n == 0) { a->positions = pool + used; a->size = -1; }
						pool[used] = p;
						pool[used].state = loaded.epoch | POSITION_DONE;
						++used; ++a->n; ++loaded.n_nodes; ++loaded.stats.n_nodes;
					} else {
						position_free(&p); // duplicated position: the count check below fails
					}
					last_bucket = b;
					continue;
				}
			}
			if (book_add(&loaded, &p) <= 0) position_free(&p); // duplicated position: the count check below fails
		}

		if (pool && used < expected) { // release the unused part of the pool (in place only)
			if (used == 0) { free(pool); loaded.pool = pool = NULL; }
#ifdef _MSC_VER
			else _expand(pool, (size_t) used * sizeof (Position));
#endif
		}
		if (ferror(f) || !book_stream_at_end(&stream) || loaded.n_nodes != expected) {
			error("Invalid opening book size or position count in %s", file);
			goto book_load_failed;
		}
		book_stream_close(&stream);

		random_seed(&loaded.random, real_clock());
		loaded.need_saving = false;

		info("done\n");
		fclose(f);
		*book = loaded;
		return true;

book_load_failed:
		book_stream_close(&stream);
		fclose(f);
		if (loaded.array) book_free(&loaded);
		book->array = NULL;
		book->n = book->n_nodes = 0;
		book->todo_list.item = NULL;
		book->todo_list.n = book->todo_list.size = 0;
		book->todo_list.valid = false;
		book->need_saving = false; // never overwrite a damaged source automatically
		return false;
	} else {
		book_new(book, options.level, 60 - get_book_depth(options.level));
		return false;
	}
}

/**
 * @brief Import an opening book.
 *
 * Read the opening book from a portable text format.
 * After the book is imported, it is needed to
 * relink & negamax it.
 *
 * @param book Opening book.
 * @param file File name.
 */
void book_import(Book *book, const char *file)
{
	FILE *f = fopen(file, "r");
	if (f) {
		PositionArray *a;
		Position *p, position;
		int n_empties;

		book_init(book);
		while (position_import(&position, f)) {
			book_add(book, &position);
			if (book->n_nodes % BOOK_INFO_RESOLUTION == 0) bprint("importing book from %s... %u positions\r", file, book->n_nodes);
		}
		bprint("importing book from %s... %u positions", file, book->n_nodes);

		book->options.n_empties = 60;
		book->options.level = 0;
		foreach_position(p, a, book) {
			n_empties = board_count_empties(&p->board);
			if (p->level > book->options.level) book->options.level = p->level;
			if (n_empties < book->options.n_empties) book->options.n_empties = n_empties;
		}

		random_seed(&book->random, real_clock());
		book->need_saving = true;

		bprint("...done\n");
		fclose(f);
	} else {
		error("cannot open \"%s\" to import the opening book\n", file);
		book_new(book, options.level, 61 - get_book_depth(options.level));
	}
}

/**
 * @brief Export an opening book.
 *
 * Save the book in a portable text format.
 *
 * @param book Opening book.
 * @param file File name.
 */
void book_export(Book *book, const char *file)
{
	FILE *f;
	PositionArray *a;
	Position *p;

	f = fopen(file, "w");
	if (f == NULL) {
		error("cannot open file %s", file);
		return;
	}
	
	info("Exporting book to %s...", file);
	foreach_position(p, a, book) {
		if (!position_export(p, f)) {
			error("cannot export book to %s", file);
			goto book_export_end;
		}
	}
	info("done\n");

book_export_end:
	fclose(f);
}

/**
 * @brief Save an opening book.
 *
 * Save the book in a fast binary format.
 *
 * @param book Opening book.
 * @param file File name.
 */
bool book_save(Book *book, const char *file)
{
	unsigned int header_edax = EDAX, header_book = BOOK;
	unsigned char header_version = VERSION, header_release = RELEASE;
	char *tmp_file;
	FILE *f;
	int r;
	PositionArray *a;
	Position *p;

	tmp_file = (char*) malloc(strlen(file) + 32);
	if (tmp_file == NULL) { error("Cannot allocate save path for %s", file); return false; }
#ifdef _WIN32
	sprintf(tmp_file, "%s.tmp.%lu", file, (unsigned long) GetCurrentProcessId());
#else
	sprintf(tmp_file, "%s.tmp.%lu", file, (unsigned long) getpid());
#endif
	f = fopen(tmp_file, "wb");
	if (f == NULL) { error("Cannot open temporary book %s", tmp_file); free(tmp_file); return false; }
	info("Saving book to %s...", file);
	book_set_date(book);

	r = fwrite(&header_edax, sizeof (unsigned int), 1, f);
	r += fwrite(&header_book, sizeof (unsigned int), 1, f);
	r += fwrite(&header_version, 1, 1, f);
	r += fwrite(&header_release, 1, 1, f);
	r += fwrite(&book->date, sizeof book->date, 1, f);
	r += fwrite(&book->options, sizeof book->options, 1, f);
	r += fwrite(&book->n_nodes, sizeof book->n_nodes, 1, f);

	if (r == 7) {
		BookStream stream;
		if (!book_stream_open(&stream, f)) r = 0;
		else {
			foreach_position(p, a, book) {
				if (!position_write(p, &stream)) {
					r = 0;
					break;
				}
			}
			if (!book_stream_flush(&stream)) r = 0;
			book_stream_close(&stream);
		}
	}
	// the data must be on disk before the new file replaces the book
	if (r == 7 && fflush(f) != 0) r = 0;
#ifdef _WIN32
	if (r == 7 && _commit(_fileno(f)) != 0) r = 0;
#else
	if (r == 7 && fsync(fileno(f)) != 0) r = 0;
#endif
	if (fclose(f) != 0) r = 0;
	if (r != 7) {
		error("\nCannot write complete book to %s; existing book was not replaced", file);
		remove(tmp_file);
		free(tmp_file);
		return false;
	}
#ifdef _WIN32
	if (!MoveFileExA(tmp_file, file, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
#else
	if (rename(tmp_file, file) != 0) {
#endif
		error("\nCannot save book to %s; existing book was not replaced", file);
		remove(tmp_file);
		free(tmp_file);
		return false;
	}
	free(tmp_file);
	book->need_saving = false;
	info("done\n");
	return true;
}

/**
 * @brief Save the book being learned to a side file (.store, .dev, .enh, ...).
 *
 * Unlike a save asked by the user (book_save), the book file itself is not up to date:
 * if the book needed saving, it still does (it is saved to the book file on exit).
 *
 * @param book Opening book.
 * @param file File name.
 * @return true if the book was saved.
 */
bool book_save_progress(Book *book, const char *file)
{
	const bool need_saving = book->need_saving;
	const bool ok = book_save(book, file);

	book->need_saving = need_saving;
	return ok;
}

/**
 * @brief Merge two opening books.
 *
 * It is needed to relink & negamax the destination book
 * after merging.
 *
 * @param dest Destination opening book.
 * @param src Source opening book.
 */
void book_merge(Book *dest, const Book *src)
{
	PositionArray *a;
	const Position *p_src;
	Position p_dest;

	foreach_position(p_src, a, src) {
		if (!book_probe(dest, &p_src->board)) {
			position_merge(&p_dest, p_src);
			book_add(dest, &p_dest);
		}
	}
}

/*
 * Bucket-range parallelism.
 * Each task gets a contiguous range of buckets; tasks only read the book
 * structure (no position is added or removed while they run).
 */
#ifndef BOOK_TEST_TASKS
#define book_n_task() options.n_task
#else
#define book_n_task() BOOK_TEST_TASKS /* test builds: threads of the book functions, whatever the search uses */
#endif

struct ChangedSet;

typedef struct BookTask {
	Book *book;
	int first, last;           /**< bucket range [first, last) */
	const struct ChangedSet *changed; /**< book_link: positions whose score changed while linking */
	unsigned long long *item;  /**< collected (bucket << 32 | index << 8 | move) items, in bucket order */
	long long n, size;
	bool oom;
	void (*run)(struct BookTask*);
	volatile long long done;     /**< positions scanned so far (progress display only) */
	volatile bool finished;      /**< the task is over (progress display only) */
} BookTask;

static void book_task_push(BookTask *task, const unsigned long long item)
{
	if (task->n == task->size) {
		const long long size = task->size + task->size / 2 + 4096;
		unsigned long long *p = (unsigned long long*) realloc(task->item, size * sizeof *p);
		if (p == NULL) { task->oom = true; return; }
		task->item = p; task->size = size;
	}
	task->item[task->n++] = item;
}

static void* book_task_main(void *v)
{
	BookTask *task = (BookTask*) v;
	task->run(task);
	task->finished = true;
	return NULL;
}

/** @return true (at most once per second) when a progress line is due. */
static bool book_progress_due(long long *next)
{
	const long long t = real_clock();
	if (t < *next) return false;
	*next = t + 1000;
	return true;
}

/**
 * @brief Run a function on every bucket range with options.n_task threads.
 *
 * With a progress label, all the ranges run in worker threads and this thread prints
 * "<label>...<positions scanned>/<positions> positions checked" once per second.
 *
 * @param changed Given to every task (see book_link_tasks; NULL otherwise).
 * @return number of tasks (task[0..n-1] hold the results, in bucket order).
 */
static int book_parallel_with(Book *book, void (*run)(BookTask*), BookTask *task, const char *progress, const struct ChangedSet *changed)
{
	int i, n = book_n_task();
	if (n > MAX_THREADS) n = MAX_THREADS;
	if (n < 1) n = 1;
	for (i = 0; i < n; ++i) {
		task[i].book = book;
		task[i].changed = changed;
		task[i].first = (int) ((long long) book->n * i / n);
		task[i].last = (int) ((long long) book->n * (i + 1) / n);
		task[i].item = NULL; task[i].n = task[i].size = 0; task[i].oom = false;
		task[i].run = run;
		task[i].done = 0; task[i].finished = false;
	}
	if (progress && book_verbose) {
		Thread thread[MAX_THREADS];
		bool created[MAX_THREADS];
		long long next = real_clock() + 1000;
		bool finished = false;
		int n_wait = 0;
		for (i = 0; i < n; ++i) {
			created[i] = thread_create(thread + i, book_task_main, task + i);
			if (!created[i]) book_task_main(task + i); // no thread (memory exhausted): this one does the task
			else if (options.cpu_affinity) thread_set_cpu(thread[i], i); // (see book_run_workers)
		}
		while (!finished) {
			long long done = 0;
			relax(++n_wait <= 20 ? 1 : 50); // the scan of a small book is over at once
			finished = true;
			for (i = 0; i < n; ++i) { done += task[i].done; if (!task[i].finished) finished = false; }
			if (!finished && book_progress_due(&next)) bprint("%s...%lld/%u positions checked\r", progress, done, book->n_nodes);
		}
		for (i = 0; i < n; ++i) if (created[i]) thread_join(thread[i]);
	} else {
		book_run_workers(book_task_main, task, sizeof *task, n);
	}
	return n;
}

static int book_parallel(Book *book, void (*run)(BookTask*), BookTask *task, const char *progress)
{
	return book_parallel_with(book, run, task, progress, NULL);
}

static void book_tasks_free(BookTask *task, const int n)
{
	int i;
	for (i = 0; i < n; ++i) free(task[i].item);
}

#define TASK_ITEM(b, k, x) (((unsigned long long) (b) << 32) | ((unsigned long long) (k) << 8) | (unsigned long long) (x))
#define TASK_BUCKET(item) ((int) ((item) >> 32))
#define TASK_INDEX(item) ((int) (((item) >> 8) & 0xffffff))
#define TASK_MOVE(item) ((int) ((item) & 0xff))
#define TASK_NO_LINK 0xff /* the position has no new link but its leaf must be searched */

static bool position_has_link(const Position *position, const int x)
{
	const Link *l;
	foreach_link(l, position) if (l->move == x) return true;
	return false;
}

/*
 * Positions whose score changed while book_link added links (book_link with threads).
 *
 * The one by one book_link() gives to a link the score that its position has at that time:
 * a position linked after one of these gets its new score, a position linked before it keeps
 * the old one. The threads of the first phase read the old scores, so the links to these
 * positions from the positions that come after them are set again in a last phase.
 */
typedef struct ChangedSet {
	unsigned long long *list;  /**< bucket << 32 | index of the positions, in book order */
	long long n;
	unsigned long long *code;  /**< hash table: hash code of the board | 1 (0: free slot); NULL: not available */
	unsigned long long *key;   /**< hash table: bucket << 32 | index */
	unsigned long long mask;
} ChangedSet;

#define changed_set_slot(set, c) (((c) >> 24) & (set)->mask) /* the low bits of the code choose the bucket of the book */

/** Build the hash table of the set (if it cannot be allocated, the list is searched instead). */
static void changed_set_index(ChangedSet *set, const Book *book)
{
	unsigned long long size = 1024, j;
	long long i;

	while (size < (unsigned long long) set->n * 2) size <<= 1;
	set->code = (unsigned long long*) calloc(size, sizeof *set->code);
	set->key = (unsigned long long*) malloc(size * sizeof *set->key);
	if (set->code == NULL || set->key == NULL) {
		free(set->code); free(set->key);
		set->code = set->key = NULL;
		return;
	}
	set->mask = size - 1;
	for (i = 0; i < set->n; ++i) {
		const Position *p = book->array[set->list[i] >> 32].positions + (int) (set->list[i] & 0xffffffffu);
		const unsigned long long code = board_get_hash_code(&p->board) | 1;
		for (j = changed_set_slot(set, code); set->code[j]; j = (j + 1) & set->mask) ;
		set->code[j] = code; set->key[j] = set->list[i];
	}
}

static void changed_set_free(ChangedSet *set)
{
	free(set->code); free(set->key); free(set->list);
}

/** Last phase of book_link: set again the links to the positions whose score changed before their parent was linked. */
static void book_link_refresh(BookTask *task)
{
	Book *book = task->book;
	const ChangedSet *set = task->changed;
	int b, k;
	Board next, unique;
	Link *l;

	for (b = task->first; b < task->last; ++b) {
		const PositionArray *a = book->array + b;
		for (k = 0; k < a->n; ++k) {
			Position *p = a->positions + k;
			const unsigned long long key = ((unsigned long long) b << 32) | (unsigned long long) k;
			foreach_link(l, p) {
				board_next(&p->board, l->move, &next);
				board_unique(&next, &unique);
				if (set->code) {
					const unsigned long long code = board_get_hash_code(&unique) | 1;
					unsigned long long j;
					for (j = changed_set_slot(set, code); set->code[j]; j = (j + 1) & set->mask) {
						if (set->code[j] == code && set->key[j] < key) {
							const Position *child = book->array[set->key[j] >> 32].positions + (int) (set->key[j] & 0xffffffffu);
							if (board_equal(&child->board, &unique)) { l->score = -child->score.value; break; }
						}
					}
				} else { // no hash table: look for the position in the book, then in the list
					const unsigned long long i = board_get_hash_code(&unique) & (book->n - 1);
					const Position *child = position_array_probe(book->array + i, &unique);
					if (child) {
						const unsigned long long child_key = (i << 32) | (unsigned long long) (child - book->array[i].positions);
						long long lo = 0, hi = set->n - 1;
						while (child_key < key && lo <= hi) {
							const long long mid = (lo + hi) / 2;
							if (set->list[mid] == child_key) { l->score = -child->score.value; break; }
							if (set->list[mid] < child_key) lo = mid + 1; else hi = mid - 1;
						}
					}
				}
			}
		}
	}
}

/**
 * Phase 1 of book_link: find the missing links.
 * As position_link() does, it also gives to the existing links the score of their position
 * (only this thread writes to the links of a position, and no thread reads the links of
 * another position).
 */
static void book_link_find(BookTask *task)
{
	Book *book = task->book;
	int b, k, x;
	Board next;

	for (b = task->first; b < task->last; ++b) {
		const PositionArray *a = book->array + b;
		for (k = 0; k < a->n; ++k) {
			Position *p = a->positions + k;
			unsigned long long moves = board_get_moves(&p->board);
			bool found = false;
			const Position *child;
			Link *l;
			if (moves) {
				foreach_bit(x, moves) {
					board_next(&p->board, x, &next);
					child = book_probe(book, &next);
					if (child) {
						foreach_link(l, p) if (l->move == x) break;
						if (l < position_links(p) + p->n_link) l->score = -child->score.value;
						else { book_task_push(task, TASK_ITEM(b, k, x)); found = true; }
					}
				}
			} else if (can_move(p->board.opponent, p->board.player)) {
				next.player = p->board.opponent;
				next.opponent = p->board.player;
				child = book_probe(book, &next);
				if (child) {
					foreach_link(l, p) if (l->move == PASS) break;
					if (l < position_links(p) + p->n_link) l->score = -child->score.value;
					else { book_task_push(task, TASK_ITEM(b, k, PASS)); found = true; }
				}
			}
			if (!found && p->leaf.move == NOMOVE) book_task_push(task, TASK_ITEM(b, k, TASK_NO_LINK));
		}
		task->done += a->n;
	}
}

/**
 * Phase 1 of the link of book merge (as from v4.5.5-nikque.3): find the missing links (read only).
 * The moves that are already links are not probed and keep their score: book merge negamaxes the
 * book afterwards, which sets again the scores of the links of every position reachable from the root.
 */
static void book_link_find_missing(BookTask *task)
{
	Book *book = task->book;
	int b, k, x;
	Board next;

	for (b = task->first; b < task->last; ++b) {
		const PositionArray *a = book->array + b;
		for (k = 0; k < a->n; ++k) {
			const Position *p = a->positions + k;
			unsigned long long moves = board_get_moves(&p->board);
			bool found = false;
			if (moves) {
				foreach_bit(x, moves) {
					if (!position_has_link(p, x)) {
						board_next(&p->board, x, &next);
						if (book_probe(book, &next)) { book_task_push(task, TASK_ITEM(b, k, x)); found = true; }
					}
				}
			} else if (can_move(p->board.opponent, p->board.player) && !position_has_link(p, PASS)) {
				next.player = p->board.opponent;
				next.opponent = p->board.player;
				if (book_probe(book, &next)) { book_task_push(task, TASK_ITEM(b, k, PASS)); found = true; }
			}
			if (!found && p->leaf.move == NOMOVE) book_task_push(task, TASK_ITEM(b, k, TASK_NO_LINK));
		}
		task->done += a->n;
	}
}

/*
 * Leaves of the merge source for positions that are in both books.
 * When relinking clears the leaf of such a position, the source leaf can be
 * used instead of a new search if it is still not a link: after relinking, the
 * destination links include every link of the source position, so the source
 * best non-link move is also the best non-link move of the destination.
 */
typedef struct MergeHint {
	unsigned long long key;    /**< bucket << 32 | index in the destination book */
	Link leaf;
	unsigned char level;
} MergeHint;

static MergeHint *merge_hint = NULL;
static long long merge_hint_n = 0, merge_hint_size = 0;

static void merge_hint_free(void)
{
	free(merge_hint); merge_hint = NULL;
	merge_hint_n = merge_hint_size = 0;
}

static bool merge_hint_add(const unsigned long long key, const Link *leaf, const int level)
{
	if (merge_hint_n == merge_hint_size) {
		const long long size = merge_hint_size + merge_hint_size / 2 + 4096;
		MergeHint *h = (MergeHint*) realloc(merge_hint, size * sizeof *h);
		if (h == NULL) return false;
		merge_hint = h; merge_hint_size = size;
	}
	merge_hint[merge_hint_n].key = key;
	merge_hint[merge_hint_n].leaf = *leaf;
	merge_hint[merge_hint_n].level = (unsigned char) level;
	++merge_hint_n;
	return true;
}

static int merge_hint_cmp(const void *a, const void *b)
{
	const unsigned long long x = ((const MergeHint*) a)->key, y = ((const MergeHint*) b)->key;
	return (x > y) - (x < y);
}

static const MergeHint* merge_hint_find(const unsigned long long key)
{
	long long lo = 0, hi = merge_hint_n - 1;
	while (lo <= hi) {
		const long long mid = (lo + hi) / 2;
		if (merge_hint[mid].key == key) return merge_hint + mid;
		if (merge_hint[mid].key < key) lo = mid + 1; else hi = mid - 1;
	}
	return NULL;
}

static void book_link_one_by_one(Book*);

static void plan_job_add(struct BookPlan*, const Board*, const unsigned long long, const bool, const int);

/**
 * @brief Plan the leaf searches that linking is going to do (book-store-tasks > 1).
 *
 * The positions that get new links, and their links, are known from the items: a position whose
 * leaf becomes a link (or that has no leaf) is searched again without its links, unless a leaf of
 * the merged book can be used. These searches are done at the same time (see book_plan_begin).
 *
 * @param book Opening book (not changed).
 * @param task Items found by the threads.
 * @param n Number of tasks.
 */
static void book_link_plan(Book *book, const BookTask *task, const int n)
{
	int i;
	long long j;

	for (i = 0; i < n; ++i) {
		for (j = 0; j < task[i].n; ) {
			const unsigned long long key = task[i].item[j] >> 8;
			const Position *p = book->array[TASK_BUCKET(task[i].item[j])].positions + TASK_INDEX(task[i].item[j]);
			const MergeHint *hint = merge_hint_find(((unsigned long long) TASK_BUCKET(task[i].item[j]) << 32) | (unsigned long long) TASK_INDEX(task[i].item[j]));
			const int n_moves = get_mobility(p->board.player, p->board.opponent);
			unsigned long long links = 0;
			bool pass = false, no_leaf = (p->leaf.move == NOMOVE);
			int value = p->score.value, n_link;
			const Link *l;

			foreach_link(l, p) {
				if (l->move == PASS) pass = true;
				else if (l->move <= H8) links |= x_to_bit(l->move);
			}
			for (; j < task[i].n && (task[i].item[j] >> 8) == key; ++j) { // the new links of this position
				const int x = TASK_MOVE(task[i].item[j]);
				if (x == TASK_NO_LINK) continue;
				if (x == PASS) pass = true; else links |= x_to_bit(x);
				if (x == p->leaf.move) no_leaf = true;
				value = SCORE_INF; // a link was added: the score is not -SCORE_INF any more
			}
			if (!no_leaf) continue;
			if (hint && hint->level == p->level && hint->leaf.move != NOMOVE
			 && !(hint->leaf.move == PASS ? pass : (hint->leaf.move <= H8 && (links & x_to_bit(hint->leaf.move)) != 0))) continue;
			n_link = bit_count(links) + pass;
			if (n_link < n_moves || (n_link == 0 && n_moves == 0 && value == -SCORE_INF)) plan_job_add(book_plan, &p->board, links, pass, p->level);
		}
	}
}

/**
 * @brief Link a book using several threads.
 *
 * Same links and leaf searches as the one by one book_link(), in the same order.
 * - exact (book_link): the book is the same as with the one by one book_link().
 * - not exact (book merge, as from v4.5.5-nikque.3 to 6): the scores of the links that already
 *   existed are not refreshed here (book_negamax() recomputes them for every position reachable
 *   from the root), and the leaves of the merged book are used (merge hints).
 * With book-store-tasks > 1, the leaf searches are done at the same time before (each one with
 * empty hash tables: see book_plan_begin); with book-store-tasks = 1 they are done one by one.
 *
 * @param book Opening book.
 * @param exact Same result as the one by one book_link().
 */
static void book_link_tasks(Book *book, const bool exact)
{
	BookTask task[MAX_THREADS];
	ChangedSet changed = {0};
	int i, n, value = 0;
	long long j, n_items = 0, i_item = 0, next = real_clock() + 1000;
	bool first = true, oom = false, plan = false;

	bprint("Linking book...\r");
	n = book_parallel(book, exact ? book_link_find : book_link_find_missing, task, "Linking book");
	for (i = 0; i < n; ++i) { n_items += task[i].n; oom = oom || task[i].oom; }
	if (exact && !oom) {
		changed.list = (unsigned long long*) malloc((n_items + 1) * sizeof *changed.list); // enough for every position of the items
		oom = (changed.list == NULL);
	}
	if (oom) {
		// nothing was done yet, but the scores of some existing links (exact), that the one by one link sets again
		book_tasks_free(task, n);
		merge_hint_free();
		error("cannot allocate link list; using sequential link\n");
		book_link_one_by_one(book);
		return;
	}

	if (n_items > 0 && book_plan == NULL && book_store_task_count() > 1 && book_plan_begin(book)) {
		plan = true;
		book_link_plan(book, task, n);
		book_plan_search(book);
	}

	for (i = 0; i < n; ++i) {
		for (j = 0; j < task[i].n; ++j) {
			if ((++i_item & 15) == 0 && book_progress_due(&next)) bprint("Linking book...%lld/%lld positions linked\r", i_item, n_items);
			const unsigned long long item = task[i].item[j];
			Position *p = book->array[TASK_BUCKET(item)].positions + TASK_INDEX(item);
			const int x = TASK_MOVE(item);
			const bool last = (j + 1 == task[i].n || (task[i].item[j + 1] >> 8) != (item >> 8));
			if (first) value = p->score.value; // score before the first item of this position
			first = last;
			if (x != TASK_NO_LINK) {
				Board next;
				Position *child;
				Link link;
				if (x == PASS) { next.player = p->board.opponent; next.opponent = p->board.player; }
				else board_next(&p->board, x, &next);
				child = book_probe(book, &next);
				link.score = -child->score.value;
				link.move = x;
				if (position_add_link(p, &link)) ++book->stats.n_links;
			}
			if (last && p->leaf.move == NOMOVE) {
				const MergeHint *hint = merge_hint_find(((unsigned long long) TASK_BUCKET(item) << 32) | (unsigned long long) TASK_INDEX(item));
				const int n_moves = get_mobility(p->board.player, p->board.opponent);
				if (hint && hint->level == p->level && hint->leaf.move != NOMOVE && !position_has_link(p, hint->leaf.move)) {
					// the source book already searched this position with a subset of these links
					p->leaf = hint->leaf;
					if (p->leaf.score > p->score.value) p->score.value = p->leaf.score;
					book->need_saving = true;
				} else if (p->n_link < n_moves || (p->n_link == 0 && n_moves == 0 && p->score.value == -SCORE_INF)) {
					position_search(p, book);
				}
			}
			if (last && exact && p->score.value != value) changed.list[changed.n++] = ((unsigned long long) TASK_BUCKET(item) << 32) | (unsigned long long) TASK_INDEX(item);
		}
	}
	if (plan) book_plan_end(book);
	book_tasks_free(task, n);
	merge_hint_free();
	if (changed.n) {
		changed_set_index(&changed, book);
		book_tasks_free(task, book_parallel_with(book, book_link_refresh, task, NULL, &changed));
	}
	changed_set_free(&changed);
	bprint("Linking book...%u done\n", book->n_nodes);
}

/**
 * @brief Link a book after book merge.
 *
 * With the leaves of the merged book (merge hints), even with one thread.
 *
 * @param book opening book.
 */
void book_link_parallel(Book *book)
{
	book_link_tasks(book, false);
}

/**
 * Phase 1 of book_fix: find wrong positions (read only; what is wrong is explained when they are fixed).
 */
static void book_fix_find(BookTask *task)
{
	int b, k;
	for (b = task->first; b < task->last; ++b) {
		const PositionArray *a = task->book->array + b;
		for (k = 0; k < a->n; ++k) {
			if (!position_check(a->positions + k, false)) book_task_push(task, TASK_ITEM(b, k, 0));
			else if (position_has_missing_link(a->positions + k, task->book)) book_task_push(task, TASK_ITEM(b, k, 1));
		}
		task->done += a->n;
	}
}

static void book_fix_one_by_one(Book*, int, int, int*, int*);

static void book_sort_range(BookTask *task)
{
	int b, k;
	for (b = task->first; b < task->last; ++b) {
		PositionArray *a = task->book->array + b;
		for (k = 0; k < a->n; ++k) position_sort(a->positions + k);
	}
}

void book_sort_parallel(Book *book)
{
	BookTask task[MAX_THREADS];
	bprint("Sorting book...");
	book_tasks_free(task, book_parallel(book, book_sort_range, task, NULL));
	bprint("done>\n");
}

/**
 * @brief Merge a book file into the current book without loading it.
 *
 * The file is read twice: first to check that it is a complete book, then to
 * add the positions missing from the destination (same rules as book_merge()).
 * On any error the destination book is left unchanged.
 *
 * The positions that the merge adds are the ones 'done' at the current epoch (as every new
 * position: see position_array_add). The merge starts a new epoch, so that no other position is.
 * (Up to v4.5.5-nikque.8 the added positions kept a mark of their own until negamax reached them:
 * after a merge that added positions out of reach from the root, the next merge of a book holding
 * one of them failed with "duplicated position", and removed all of them if it had added something.)
 *
 * @param dest Destination opening book.
 * @param file Source book file.
 * @return true on success.
 */
bool book_merge_file(Book *dest, const char *file)
{
	FILE *f = fopen(file, "rb");
	BookStream stream = {0};
	Position p, merged;
	unsigned int header_edax = 0, header_book = 0;
	unsigned char header[10];
	unsigned int i, expected = 0;	// the position count is saved as a 32-bit unsigned int
	long long n_added = 0, next;
	int pass, k;
	bool ok = false, use_hints = true;

	if (f == NULL) { error("cannot open %s", file); return false; }
	if (fread(&header_edax, 4, 1, f) != 1 || fread(&header_book, 4, 1, f) != 1 || header_edax != EDAX || header_book != BOOK
	 || fread(header, 2, 1, f) != 1 || header[0] != VERSION) {
		error("%s is not a compatible edax opening book", file);
		fclose(f); return false;
	}
	{ Book h; if (fread(&h.date, sizeof h.date, 1, f) != 1 || fread(&h.options, sizeof h.options, 1, f) != 1 || fread(&expected, sizeof expected, 1, f) != 1) {
		error("Cannot read book settings from %s", file);
		fclose(f); return false;
	} }
	if (!book_stream_open(&stream, f)) { fclose(f); return false; }

	book_clean(dest); // no position of the destination is 'done' at the new epoch
	merge_hint_free(); // (hints of a merge that was not followed by book_link_parallel)

	for (pass = 0; pass < 2; ++pass) {
		const long long start = 8 + 2 + sizeof dest->date + sizeof dest->options + sizeof expected;
		if (pass) book_grow_buckets(dest, (long long) dest->n_nodes + expected); // the source is valid: prepare the destination
#ifdef _WIN32
		if (pass && _fseeki64(f, start, SEEK_SET) != 0) goto merge_end;
#else
		if (pass && fseeko(f, start, SEEK_SET) != 0) goto merge_end;
#endif
		stream.n = stream.pos = 0;
		bprint("%s book %s...\r", pass ? "Merging" : "Checking", file);
		next = real_clock() + 1000;
		for (i = 0; i < expected; ++i) {
			if ((i & 0xffff) == 0 && i && book_progress_due(&next)) bprint("%s book %s...%u/%u positions\r", pass ? "Merging" : "Checking", file, i, expected);
			if (!position_read(&p, &stream)) {
				error("Truncated opening book %s at position %u/%u", file, i, expected);
				goto merge_end;
			}
			if (pass) {
				Position *q = book_probe(dest, &p.board);
				if (q == NULL) {
					const unsigned long long b = board_get_hash_code(&p.board) & (dest->n - 1);
					PositionArray *a = dest->array + b;
					position_merge(&merged, &p);
					if (dest->n_nodes == UINT_MAX || position_array_add(a, &merged, dest->epoch) <= 0) { position_free(&p); error("cannot add a position to the book"); goto merge_end; }
					++dest->n_nodes; ++dest->stats.n_nodes; ++n_added;
				} else if (!position_is_done(q, dest)) { // a position of the destination: remember the source leaf (see book_link_parallel)
					// (a board of the file that is not the unique one has its leaf in another orientation: no hint)
					if (use_hints && p.leaf.move != NOMOVE && board_equal(&p.board, &q->board) && (p.leaf.move != q->leaf.move || p.n_link != q->n_link)) {
						const unsigned long long b = board_get_hash_code(&q->board) & (dest->n - 1);
						if (!merge_hint_add((b << 32) | (unsigned long long) (q - dest->array[b].positions), &p.leaf, p.level)) { merge_hint_free(); use_hints = false; }
					}
				} else {
					position_free(&p);
					error("duplicated position in %s", file);
					goto merge_end;
				}
			}
			position_free(&p);
		}
		if (!pass && (ferror(f) || !book_stream_at_end(&stream))) {
			error("Invalid opening book size or position count in %s", file);
			goto merge_end;
		}
	}
	ok = true;
	if (merge_hint_n) qsort(merge_hint, merge_hint_n, sizeof *merge_hint, merge_hint_cmp);

merge_end:
	book_stream_close(&stream);
	fclose(f);
	if (!ok) merge_hint_free();
	if (!ok && n_added) { // remove what was added: the destination is left unchanged
		PositionArray *a;
		for (a = dest->array; a < dest->array + dest->n; ++a)
		for (k = 0; k < a->n; ++k) if (position_is_done(a->positions + k, dest)) { book_remove(dest, a->positions + k); --k; }
	}
	if (ok) dest->need_saving = dest->need_saving || n_added > 0;
	bprint("Merging book %s...%lld positions added\n", file, n_added);
	return ok;
}

/**
 * @brief Negamax a book.
 *
 * @param book opening book.
 */
void book_negamax(Book *book)
{
	Position *root = book_root(book);

	if (root) {
		bprint("Negamaxing book...");
		book_clean(book);
		book_negamax_position(root, book);
		bprint("done\n");
	}
#ifdef BOOK_TEST_NODES
	fprintf(stderr, "<play nodes: %lld>", book_test_play_nodes);
	fprintf(stderr, "<book searches: %lld, nodes: %lld>\n", book_test_searches, book_test_nodes);
#endif
}

/**
 * @brief Link a book.
 *
 * @param book opening book.
 */
void book_link(Book *book)
{
	// with several threads: the same book, found faster (the threads look for the missing links)
	if (book_n_task() > 1) book_link_tasks(book, true);
	else book_link_one_by_one(book);
}

/**
 * @brief Link a book, a position after the other.
 *
 * @param book opening book.
 */
static void book_link_one_by_one(Book *book)
{
	PositionArray *a;
	Position *p;
	int i = 0;

	bprint("Linking book...\r");
	foreach_position(p, a, book) {
		position_link(p, book);
		if (p->leaf.move == NOMOVE) {
			position_search(p, book);
		}
		if (++i % BOOK_INFO_RESOLUTION == 0) bprint("Linking book...%d\r", i);
	}
	bprint("Linking book...%d done\n", i);
}

/**
 * @brief Fix the positions of a book, a position after the other, from a position to the end.
 *
 * @param book opening book.
 * @param b Bucket of the first position to check.
 * @param k Index of the first position to check in its bucket.
 * @param n_fix Number of fixed positions (updated).
 * @param n_missing Number of positions with links to missing positions (updated).
 */
static void book_fix_one_by_one(Book *book, int b, int k, int *n_fix, int *n_missing)
{
	unsigned int n_checked = (unsigned int) k;
	long long next = real_clock() + 1000;
	bool progress = false;
	int i;

	for (i = 0; i < b; ++i) n_checked += (unsigned int) book->array[i].n;
	for (; b < book->n; ++b, k = 0) {
		PositionArray *a = book->array + b;
		for (; k < a->n; ++k) {
			Position *p = a->positions + k;
			// most positions need no fix: show the checked positions (once per second)
			if ((++n_checked & 0xfff) == 0 && book_progress_due(&next)) {
				bprint("Fixing book...%u/%u positions checked\r", n_checked, book->n_nodes);
				progress = true;
			}
			if (!position_is_ok(p)) {
				position_fix(p, book);
				++*n_fix;
			} else if (position_has_missing_link(p, book)) {
				position_remove_links(p, book);
				++*n_missing;
				++*n_fix;
			}
		}
	}
	if (progress) clear_line();
}

/**
 * @brief Fix a book.
 *
 * Wrong positions are recomputed; links to positions missing from the book
 * are removed (their best score may become the leaf, as book prune does).
 *
 * With several threads, the threads check the positions, then the positions found are fixed in
 * the order of the book: the result is the same as with one thread. Removing links does not
 * change what the next positions link to, but recomputing a wrong position can (its board
 * changes): from the first wrong position, the positions are checked again, one after the other.
 *
 * @param book opening book.
 */
void book_fix(Book *book)
{
	int n_fix = 0, n_missing = 0;

	bprint("Fixing book...\r");
	if (book_n_task() > 1) {
		BookTask task[MAX_THREADS];
		const int n = book_parallel(book, book_fix_find, task, "Fixing book");
		long long j;
		int i;
		bool oom = false;

		for (i = 0; i < n; ++i) oom = oom || task[i].oom;
		if (oom) {
			book_fix_one_by_one(book, 0, 0, &n_fix, &n_missing);
		} else {
			if (book_verbose) clear_line();
			for (i = 0; i < n; ++i) for (j = 0; j < task[i].n; ++j) {
				const unsigned long long item = task[i].item[j];
				if (TASK_MOVE(item) == 0) {
					book_fix_one_by_one(book, TASK_BUCKET(item), TASK_INDEX(item), &n_fix, &n_missing);
					i = n; break;
				}
				position_remove_links(book->array[TASK_BUCKET(item)].positions + TASK_INDEX(item), book);
				++n_missing;
				++n_fix;
			}
		}
		book_tasks_free(task, n);
	} else {
		book_fix_one_by_one(book, 0, 0, &n_fix, &n_missing);
	}
	if (n_missing) warn("links to missing positions removed from %d positions\n", n_missing);
	bprint("Fixing book...%d done\n", n_fix);
}

/**
 * @brief Check whether the configured interval for a timed book save elapsed.
 *
 * @param start Time of the last timed save in milliseconds.
 * @return true when a timed save is due.
 */
static bool book_save_interval_elapsed(const long long start)
{
	return options.book_save_interval > 0
		&& real_clock() - start >= (long long)options.book_save_interval * 60000LL;
}

/** Save completed deviate rounds at the configured cadence and at convergence.
 * Timed saves inside book_expand are independent: they can capture a partial
 * round, so they do not reset this completed-round counter.
 */
static void book_deviate_save_progress(Book *book, const char *file, const long long n_diffs, int *rounds_since_save)
{
	if (n_diffs > 0 && *rounds_since_save < INT_MAX) ++*rounds_since_save;
	if (*rounds_since_save > 0
	 && (n_diffs == 0 || (options.book_deviate_save_rounds > 0
	     && *rounds_since_save >= options.book_deviate_save_rounds))) {
		if (book_save_progress(book, file)) *rounds_since_save = 0;
	}
}

/**
 * @brief Deepen a book.
 *
 * Research all non link best move.
 *
 * @param book opening book.
 */
void book_deepen(Book *book)
{
	PositionArray *a;
	Position *p;
	int i = 0;
	unsigned long long t = real_clock();
	char file[FILENAME_MAX + 1];
	
	file_add_ext(options.book_file, ".dep", file);

	bprint("Deepening book...\r"); 
	foreach_position(p, a, book) {
		int n_empties = board_count_empties(&p->board);
		if (LEVEL[p->level][n_empties].depth != LEVEL[book->options.level][n_empties].depth
		 || LEVEL[p->level][n_empties].selectivity != LEVEL[book->options.level][n_empties].selectivity) { // No! compare depth & selectivity;
			p->leaf = BAD_LINK;
			position_search(p, book);
			if (++i % 10 == 0) {
				bprint("Deepening book...%d\r", i); 
			}
			if (book_save_interval_elapsed((long long)t)) {
				book_save_progress(book, file); // timed progress save
				t = real_clock();
			}
		}
	}
	bprint("Deepening book...%d done\n", i);
}

/**
 * @brief Correct wrong solved score in the book.
 *
 * Correct erroneous solved positions. Edax may be unstable and introduce bugs from time to time...
 *
 * @param book opening book.
 */
void book_correct_solved(Book *book)
{
	PositionArray *a;
	Position *p;
	int i = 0;
	unsigned long long t = real_clock();
	char file[FILENAME_MAX + 1];
	Link old_leaf;
	int n_error = 0;
	char s[4];
	
	file_add_ext(options.book_file, ".err", file);

	bprint("Correcting solved positions...\r"); 
	foreach_position(p, a, book) {
		int n_empties = board_count_empties(&p->board);
		if (LEVEL[p->level][n_empties].depth == n_empties && LEVEL[p->level][n_empties].selectivity == NO_SELECTIVITY) { // No! compare depth & selectivity;
			old_leaf = p->leaf;
			p->leaf = BAD_LINK;
			position_search(p, book);
			if (p->leaf.score != old_leaf.score) {
				++n_error;
				bprint("\nError found:\n");
				position_print(p, &p->board, stdout);
				move_to_string(old_leaf.move, n_empties & 1, s);
				bprint("instead of <%s:%d>\n\n", s, old_leaf.score);
			}
			if (++i % 10 == 0 || p->leaf.score != old_leaf.score) {
				bprint("Correcting solved positions...%d (%d error found)\r", i, n_error); 
			}
			if (book_save_interval_elapsed((long long)t)) {
				book_save_progress(book, file); // timed progress save
				t = real_clock();
			}
		}
	}
	bprint("Correcting solved positions...%d done (%d error found)\n", i, n_error);
}

/** state shared by the threads of a concurrent book_expand */
typedef struct ExpandShared {
	Book *book;
	Lock lock;                  /**< guards the book, the counters and the output */
	long long next;             /**< next todo_list item to take */
	int n_done;                 /**< expanded positions (progress) */
	const char *action, *tmp_file;
	unsigned long long t;       /**< time of the last timed save */
	bool stop;
} ExpandShared;

/** one thread of a concurrent book_expand, with its own search (and hash tables) */
typedef struct ExpandWorker {
	ExpandShared *shared;
	Search *search;
} ExpandWorker;

/**
 * @brief Copy a position with its own copy of the links.
 *
 * @param dest Copy (to release with position_free()).
 * @param src Position.
 * @return false if the links cannot be allocated.
 */
static bool position_copy(Position *dest, const Position *src)
{
	*dest = *src;
	if (src->n_link > POSITION_INLINE_LINKS) {
		dest->links.array = (Link*) malloc(src->n_link * sizeof (Link));
		if (dest->links.array == NULL) { dest->n_link = 0; return false; }
		memcpy(dest->links.array, src->links.array, src->n_link * sizeof (Link));
	}
	return true;
}

/**
 * @brief Expand todo positions in a thread.
 *
 * The book is only read (links of the new position) and written (parent update,
 * new position) under the shared lock; the two searches of an expansion run
 * outside of it with the thread's own search. The parent is updated with the same
 * steps as position_expand() (it is not changed by the other expansions).
 *
 * @param v Worker.
 * @return NULL.
 */
static void* book_expand_worker(void *v)
{
	ExpandWorker *w = (ExpandWorker*) v;
	ExpandShared *s = w->shared;
	Book *book = s->book;

	for (;;) {
		Position parent, child, *q = NULL;
		Link leaf;
		int r;

		lock(s);
		while (!s->stop && s->next < book->todo_list.n) {
			const unsigned long long item = book->todo_list.item[s->next++];
			Position *p = book->array[item >> 32].positions + (int) (item & 0xffffffffu);
			if (!position_is_todo(p, book)) continue;
			if (p->leaf.move != NOMOVE) { q = p; break; }
			bprint("%s...%d/%lld done: %lld positions, %lld links\r", s->action, ++s->n_done, book->stats.n_todo, book->stats.n_nodes, book->stats.n_links);
		}
		if (q == NULL) { unlock(s); break; }
		if (!position_copy(&parent, q)) { error("cannot copy a position"); book->failed = s->stop = true; unlock(s); break; }
		position_init(&child);
		board_next(&parent.board, parent.leaf.move, &child.board);
		child.level = parent.level;
		position_link(&child, book);
		unlock(s);

		search_cleanup(w->search);
		position_search_with(&child, w->search);
		parent.leaf.score = -child.score.value;
		leaf = parent.leaf;
		r = position_search_with(&parent, w->search);

		lock(s);
		q = book_probe(book, &parent.board); // the parent may have moved: probe it again
		if (q) {
			q->leaf = leaf;
			if (position_add_link(q, &q->leaf)) ++book->stats.n_links;
			if (r & 2) {
				q->leaf = parent.leaf;
				if (q->leaf.score > q->score.value) q->score.value = q->leaf.score;
			}
		}
		book->need_saving = true;
		position_free(&parent);
		position_unique(&child);
		if (book_add(book, &child) <= 0) position_free(&child); // already in the book, or not added
		if (book->failed) s->stop = true; // a position could not be added: stop learning
		bprint("%s...%d/%lld done: %lld positions, %lld links\r", s->action, ++s->n_done, book->stats.n_todo, book->stats.n_nodes, book->stats.n_links);
		if (book_save_interval_elapsed((long long) s->t)) {
			book_save_progress(book, s->tmp_file); // timed progress save (the other threads wait for the lock)
			s->t = real_clock();
		}
		unlock(s);
	}
	return NULL;
}

#ifdef BOOK_TEST_POOL_MAX
static int book_test_pool_searches = 0; /* test builds: searches created by the book functions and not released yet */
#endif

/** memory that a 32-bit program keeps free when it creates a search for the book functions (see below) */
#define BOOK_SEARCH_MEMORY_MARGIN ((size_t) 128 << 20)

/**
 * @brief Check that a search with hash tables of this size can be created.
 *
 * search_init_with() ends the program when the memory is exhausted, and the book in memory is lost.
 * The searches that the book functions create to search several positions at the same time are not
 * needed to go on: the memory of their hash tables is asked first, and without it the caller works
 * with fewer searches, or with the main search only (a 32-bit program cannot hold as many searches
 * as threads at a high level).
 *
 * A 32-bit program must also keep room for what is allocated later without such a check: the stacks of
 * the threads given to the searches and to the workers (1 MB each), their small tables, the positions
 * added to the book. (Without it, the searches of the positions filled the address space, the threads of
 * a search could not be created, and book learn waited for them for ever.)
 *
 * @param hash_bits Size of the main hash table (in number of bits).
 * @return true if the memory is available.
 */
static bool book_search_memory_available(const int hash_bits)
{
	const size_t n_main = (size_t) 1 << hash_bits, n_other = (n_main > 16 ? n_main >> 4 : 1);
	void *main_table, *other_tables, *margin = NULL;
	bool ok;

#ifdef BOOK_TEST_POOL_MAX
	if (book_test_pool_searches >= BOOK_TEST_POOL_MAX) return false; // test builds: as if the memory was exhausted
#endif
	main_table = malloc((n_main + 8) * sizeof (Hash));
	other_tables = malloc(2 * (n_other + 8) * sizeof (Hash)); // pv and shallow tables
	ok = (main_table != NULL && other_tables != NULL);
	if (ok && sizeof (void*) < 8) { // 32-bit program: the address space is what runs out
		margin = malloc(BOOK_SEARCH_MEMORY_MARGIN);
		ok = (margin != NULL);
	}
	free(main_table); free(other_tables); free(margin);
	return ok;
}

/**
 * @brief Create a search for the book functions.
 *
 * @param n_tasks Threads of the search.
 * @param hash_bits Size of its main hash table (in number of bits).
 * @return the search, NULL if the memory is not available.
 */
static Search* book_search_create(const int n_tasks, const int hash_bits)
{
	Search *search;

	if (!book_search_memory_available(hash_bits)) return NULL;
	search = (Search*) mm_malloc(sizeof (Search));
	if (search == NULL) return NULL;
	search_init_with(search, n_tasks, hash_bits);
#ifdef BOOK_TEST_POOL_MAX
	++book_test_pool_searches;
#endif
	return search;
}

/** @brief Release a search created by book_search_create(). */
static void book_search_release(Search *search)
{
	search_free(search);
	mm_free(search);
#ifdef BOOK_TEST_POOL_MAX
	--book_test_pool_searches;
#endif
}

/** searches that the book functions use to search several positions at the same time, kept from a call to the next */
typedef struct SearchPool {
	Search **search;
	int n, n_tasks, hash_bits; /**< number of searches; threads and hash table size of each one */
} SearchPool;

static SearchPool expand_pool;   /**< concurrent expansion: kept from one book_expand to the next of a learning command */
static SearchPool store_pool[2]; /**< to learn games (see book_store_release). [0]: one thread each, for the positions; [1]: for the games played at the same time */

/** @brief Release the searches of a pool. */
static void search_pool_release(SearchPool *pool)
{
	int i;
	for (i = 0; i < pool->n; ++i) book_search_release(pool->search[i]);
	free(pool->search);
	pool->search = NULL;
	pool->n = 0;
}

/**
 * @brief Get searches of a pool.
 *
 * @param pool Pool.
 * @param n Number of searches.
 * @param n_tasks Threads of each search.
 * @param hash_bits Hash table size of each search.
 * @return the number of searches available in pool->search: n, or fewer if the memory is exhausted.
 */
static int search_pool_get(SearchPool *pool, const int n, const int n_tasks, const int hash_bits)
{
	if (pool->n && (pool->n_tasks != n_tasks || pool->hash_bits != hash_bits)) search_pool_release(pool);
	if (pool->n < n) {
		Search **s = (Search**) realloc(pool->search, n * sizeof *s);
		if (s == NULL) return pool->n;
		pool->search = s;
		pool->n_tasks = n_tasks;
		pool->hash_bits = hash_bits;
		for (; pool->n < n; ++pool->n) {
			Search *search = book_search_create(n_tasks, hash_bits);
			if (search == NULL) break;
			search->options.verbosity = 0; // (the concurrent expansion sets its own)
			pool->search[pool->n] = search;
		}
	}
	return MIN(n, pool->n);
}

/**
 * @brief Number of book positions expanded at the same time.
 *
 * book-expand-tasks = n, or auto (0): each search gets 2 threads at level 18 and below
 * (the fastest in a level 18 test on a large book), 4 up to level 24 and 8 above.
 * Always 1 with the cpu option where it binds the threads (see book_one_search_at_a_time).
 *
 * @param book Opening book.
 * @return the number of concurrent expansions (1 = one by one).
 */
static int book_expand_task_count(const Book *book)
{
	int n = options.book_expand_tasks;

	if (book_one_search_at_a_time()) return 1;
	if (n <= 0) {
		const int level = book->options.level;
		n = options.n_task / (level <= 18 ? 2 : level <= 24 ? 4 : 8);
	}
	return MAX(1, MIN(n, options.n_task));
}

/**
 * @brief Expand the todo positions on several threads (book-expand-tasks > 1).
 *
 * @param book opening book.
 * @param action String with a description of current action.
 * @param tmp_file Temporary file name.
 * @param n_workers Number of positions expanded at the same time.
 * @return false if the memory for at least two searches is not available (nothing was done:
 * the positions are then expanded one after the other, with the main search).
 */
static bool book_expand_concurrent(Book *book, const char *action, const char *tmp_file, int n_workers)
{
	ExpandShared shared;
	ExpandWorker *w;
	const int n_tasks = MAX(1, options.n_task / book_expand_task_count(book));
	int i;

	n_workers = search_pool_get(&expand_pool, n_workers, n_tasks, options.hash_table_auto ? hash_table_size_auto(n_tasks) : options.hash_table_size);
	if (n_workers < 2) return false;
	w = (ExpandWorker*) calloc(n_workers, sizeof *w);
	if (w == NULL) return false;
	shared.book = book; shared.next = 0; shared.n_done = 0; shared.action = action; shared.tmp_file = tmp_file;
	shared.t = real_clock(); shared.stop = false;
	lock_init(&shared);

	for (i = 0; i < n_workers; ++i) {
		w[i].shared = &shared;
		w[i].search = expand_pool.search[i];
		w[i].search->options.verbosity = book->search->options.verbosity;
		w[i].search->options.header = book->search->options.header;
		w[i].search->options.separator = book->search->options.separator;
	}
	thread_run_workers(book_expand_worker, w, sizeof *w, n_workers, false, false); // (each worker in its own thread, as before)
	lock_free(&shared);
	free(w);
	bprint("%s...%d/%lld done: %lld positions, %lld links\n", action, shared.n_done, book->stats.n_todo, book->stats.n_nodes, book->stats.n_links);
	return true;
}

/**
 * @brief Expand a book.
 *
 * Research all non link best move.
 *
 * @param book opening book.
 * @param action String with a description of current action.
 * @param tmp_file Temporary file name.
 */
static void book_expand(Book *book, const char *action, const char *tmp_file)
{
	PositionArray *a;
	Position *p;
	int i = 0, j, k;
	unsigned long long t = real_clock();

	bprint("%s...\r", action);

	if (book_expand_task_count(book) > 1 && book->todo_list.valid && book->todo_list.n > 1) {
		qsort(book->todo_list.item, book->todo_list.n, sizeof *book->todo_list.item, todo_item_cmp);
		if (book_expand_concurrent(book, action, tmp_file, (int) MIN(book_expand_task_count(book), book->todo_list.n))) return;
		// (not enough memory for the searches: one position after the other, below)
	}

	// Visit the todo positions in bucket order: either from the list recorded
	// while marking them, or by scanning the whole book.
	if (book->todo_list.valid) qsort(book->todo_list.item, book->todo_list.n, sizeof *book->todo_list.item, todo_item_cmp);
	a = book->array; k = 0;
	for (j = 0; ; ++j) {
		if (book->todo_list.valid) {
			unsigned long long item;
			if (j == book->todo_list.n) break;
			item = book->todo_list.item[j];
			p = book->array[item >> 32].positions + (int) (item & 0xffffffffu); // do not keep p across expansions: a->positions may change!
		} else {
			while (a < book->array + book->n && k == a->n) { ++a; k = 0; }
			if (a == book->array + book->n) break;
			p = a->positions + k++;
		}
		if (position_is_todo(p, book)) {
			position_expand(p, book);
			if (book->failed) break; // a position could not be added: stop learning
			bprint("%s...%d/%lld done: %lld positions, %lld links\r", action, ++i, book->stats.n_todo, book->stats.n_nodes, book->stats.n_links);
			if (book->search->options.verbosity >= 2) putchar('\n'); else putchar('\r');
			
			if (book_save_interval_elapsed((long long)t)) {
				book_save_progress(book, tmp_file); // timed progress save
				t = real_clock();
			}
		}
	}
	bprint("%s...%d/%lld done: %lld positions, %lld links\n", action, i, book->stats.n_todo, book->stats.n_nodes, book->stats.n_links);
}

/**
 * @brief Sort a book.
 *
 * @param book opening book.
 */
void book_sort(Book *book)
{
	PositionArray *a;
	Position *p;

	if (book_n_task() > 1) { // each position is sorted on its own: the result is the same
		book_sort_parallel(book);
		return;
	}
	bprint("Sorting book...");
	foreach_position(p, a, book) {
		position_sort(p);
	}
	bprint("done>\n");
}

/**
 * @brief Play.
 *
 * Add positions to the opening book by adding links
 * to position with no links.
 *
 * @param book opening book.
 */
void book_play(Book *book)
{
	PositionArray *a;
	Position *p;
	long long n_diffs;
	char file[FILENAME_MAX + 1];

	file_add_ext(options.book_file, ".play", file);
	do {
		n_diffs = 0;
		book->stats.n_nodes = book->stats.n_links = book->stats.n_todo = 0;
		foreach_position(p, a, book) {
			if (p->n_link == 0 && board_count_empties(&p->board) >= book->options.n_empties && !board_is_game_over(&p->board)) {
				position_set_todo(p, book); ++book->stats.n_todo;
			} else {
				position_clear_todo(p);
			}
			if (book->stats.n_todo && book->stats.n_todo % BOOK_INFO_RESOLUTION == 0) bprint("Book play...%lld todo\r", book->stats.n_todo);
		}
		bprint("Book play...%lld todo\n", book->stats.n_todo);
		book->todo_list.valid = false; // todo flags were set by a full scan

		book_expand(book, "Book play", file);

		n_diffs = book->stats.n_nodes + book->stats.n_links;
		if (n_diffs) {
			book_negamax(book);
			book_save_progress(book, file);
		}
	} while (n_diffs && !book->failed); // stop if a position cannot be added
	bprint("Book play... finished\n");
	search_pool_release(&expand_pool);
}

/**
 * @brief Fill a book.
 *
 * @param book opening book.
 * @param depth Distance to fill between two positions.
 */
void book_fill(Book *book, const int depth)
{
	PositionArray *a;
	Position *p;
	long long n_diffs;
	int n_empties, k;
	char file[FILENAME_MAX + 1];

	file_add_ext(options.book_file, ".fill", file);

	do {
		n_diffs = 0;
		book->stats.n_nodes = book->stats.n_links = 0;
		for (a = book->array; a < book->array + book->n; ++a)
		for (k = 0; k < a->n; ++k) { // do not use foreach_positions here! a->positions may change!
			p = a->positions + k;
			n_empties = board_count_empties(&p->board);
			if (n_empties >= book->options.n_empties) {
				Board board = p->board; // board_fill() changes its board and adding positions may move p
				board_fill(&board, book, depth);
				if (n_diffs < book->stats.n_nodes + book->stats.n_links) {
					n_diffs = book->stats.n_nodes + book->stats.n_links;
					bprint("Book fill...%lld %lld done\r", book->stats.n_nodes, book->stats.n_links); 
				}
			}
		}
		bprint("Book fill...%lld %lld done\n", book->stats.n_nodes, book->stats.n_links);
		if (n_diffs) {
			book_negamax(book);
			book_save_progress(book, file);
		}
	} while (n_diffs && !book->failed); // stop if a position cannot be added
	bprint("Book fill... finished\n");
}

/*
 * Parallel selection of the positions to expand (book deviate, deviate2, deviate3).
 *
 * The per-position visit state is kept in the visit marks of the walk (see book_visit_init):
 * - deviate2/3: smallest accumulated loss + 1 (see book_deviate_total_by_loss).
 * - deviate: ply parity + 1 (see book_deviate_by_depth).
 * Positions to expand are collected per thread, then appended to the todo list.
 */

typedef struct DeviateWorker {
	Book *book;
	int id;
	int mode;                  /**< 0: deviate, 2: deviate2, 3: deviate3 */
	int a, b, lower, upper;    /**< deviate: player & opponent deviations, window. deviate2/3: move & total loss */
	unsigned long long *item;  /**< todo positions (bucket << 32 | index) */
	long long n, size, n_todo;
	bool oom;
	volatile bool *conflict;
} DeviateWorker;

static void deviate_worker_todo(DeviateWorker *w, Position *p)
{
	Book *book = w->book;
	const unsigned char s = atomic_load_uchar(&p->state);
	if (position_state_is(s, book, POSITION_TODO) || !atomic_cas_uchar(&p->state, s, position_state_set(s, book->epoch, POSITION_TODO))) return; // already marked
	++w->n_todo;
	if (w->n == w->size) {
		const long long size = w->size + w->size / 2 + 1024;
		unsigned long long *item = (unsigned long long*) realloc(w->item, size * sizeof *item);
		if (item == NULL) { w->oom = true; return; }
		w->item = item; w->size = size;
	}
	{
		const unsigned long long i = board_get_hash_code(&p->board) & (book->n - 1);
		w->item[w->n++] = (i << 32) | (unsigned long long) (p - book->array[i].positions);
	}
}

/*
 * deviate2/3 selection by increasing accumulated loss.
 *
 * Positions are processed level by level (level = accumulated loss), so each
 * position is walked once, with its smallest loss, instead of being re-walked
 * every time a path with a smaller loss is found. The todo set is the same as
 * the one of the recursive walk (the todo test only depends on the smallest
 * loss). Each level is processed by options.n_task threads.
 */
typedef struct PositionList {
	Position **item;
	long long n, size;
} PositionList;

static bool position_list_push(PositionList *l, Position *p)
{
	if (l->n == l->size) {
		const long long size = l->size + l->size / 2 + 1024;
		Position **item = (Position**) realloc(l->item, size * sizeof *item);
		if (item == NULL) return false;
		l->item = item; l->size = size;
	}
	l->item[l->n++] = p;
	return true;
}

typedef struct LossWorker {
	DeviateWorker w;           /**< todo list, counters, parameters */
	PositionList *next;        /**< positions reached with a larger loss, per level (total_loss + 1 lists) */
	PositionList same;         /**< positions reached with the same loss */
	Position **cur;            /**< positions to process */
	long long first, last;
	int level;
} LossWorker;

/** @return true if the position can be walked (same tests as position_deviate_total). */
static inline bool deviate_total_walkable(const Book *book, const Position *p)
{
	return board_count_empties(&p->board) >= book->options.n_empties && !board_is_game_over(&p->board);
}

/** @return true if loss is the new smallest loss of the position. */
static inline bool deviate_total_relax(Book *book, Position *p, const int loss)
{
	unsigned char *v = book_visit(book, p);
	const unsigned char mark = (unsigned char) (loss + 1);
	unsigned char old;
	do {
		old = atomic_load_uchar(v);
		if (old && old <= mark) return false;
	} while (!atomic_cas_uchar(v, old, mark));
	return true;
}

static void* loss_worker_run(void *v)
{
	LossWorker *lw = (LossWorker*) v;
	DeviateWorker *w = &lw->w;
	Book *book = w->book;
	const int move_loss = w->a, total_loss = w->b, loss = lw->level;
	long long k;
	Board target;

	for (k = lw->first; k < lw->last; ++k) {
		Position *position = lw->cur[k];
		const int n_empties = board_count_empties(&position->board);
		const Link *l;
		int move_error;

		if (atomic_load_uchar(book_visit(book, position)) != loss + 1) continue; // reached later with a smaller loss

		foreach_link(l, position) {
			move_error = position->score.value - l->score;
			if (0 <= move_error && move_error <= move_loss && loss + move_error <= total_loss) {
				Position *child;
				board_next(&position->board, l->move, &target);
				child = book_probe(book, &target);
				if (child && deviate_total_walkable(book, child) && deviate_total_relax(book, child, loss + move_error)) {
					if (!position_list_push(move_error ? lw->next + loss + move_error : &lw->same, child)) w->oom = true;
				}
			}
		}

		if (w->mode == 2 && LEVEL[position->level][n_empties].depth == n_empties
			&& LEVEL[position->level][n_empties].selectivity == NO_SELECTIVITY) continue;

		move_error = position->score.value - position->leaf.score;
		if (position->leaf.move != NOMOVE && 0 <= move_error && move_error <= move_loss && loss + move_error <= total_loss) {
			deviate_worker_todo(w, position);
		}
	}
	return NULL;
}

static bool book_deviate_total_by_loss(Book *book, Position *root, const int mode, const int move_loss, const int total_loss)
{
	LossWorker lw[MAX_THREADS];
	PositionList *level, cur = {0};
	int i, j, L, n = options.n_task;
	bool ok = true;

	if (n > MAX_THREADS) n = MAX_THREADS;
	if (n < 1) n = 1;
	if (!book->todo_list.valid) return false;
	level = (PositionList*) calloc(total_loss + 1, sizeof *level);
	if (level == NULL) return false;
	for (i = 0; i < n; ++i) {
		memset(lw + i, 0, sizeof lw[i]);
		lw[i].w.book = book; lw[i].w.id = i; lw[i].w.mode = mode; lw[i].w.a = move_loss; lw[i].w.b = total_loss;
		lw[i].next = (PositionList*) calloc(total_loss + 1, sizeof (PositionList));
		if (lw[i].next == NULL) ok = false;
	}

	if (ok && deviate_total_walkable(book, root) && deviate_total_relax(book, root, 0)) ok = position_list_push(level + 0, root);

	for (L = 0; ok && L <= total_loss; ++L) {
		// take the level list; positions reached with the same loss are processed in extra rounds
		cur = level[L]; memset(level + L, 0, sizeof level[L]);
		while (ok && cur.n > 0) {
			const int m = (cur.n < 4096) ? 1 : n; // small levels are not worth threads
			for (i = 0; i < m; ++i) {
				lw[i].cur = cur.item; lw[i].level = L;
				lw[i].first = cur.n * i / m; lw[i].last = cur.n * (i + 1) / m;
				lw[i].same.n = 0;
			}
			book_run_workers(loss_worker_run, lw, sizeof *lw, m);
			cur.n = 0;
			for (i = 0; i < m && ok; ++i) {
				if (lw[i].w.oom) ok = false;
				for (j = 0; j < lw[i].same.n && ok; ++j) ok = position_list_push(&cur, lw[i].same.item[j]);
			}
		}
		free(cur.item); cur.item = NULL; cur.size = 0;
		// move the positions reached with larger losses to the global levels
		for (i = 0; i < n && ok; ++i) {
			for (j = L + 1; j <= total_loss && j <= L + move_loss; ++j) {
				long long k;
				for (k = 0; k < lw[i].next[j].n && ok; ++k) ok = position_list_push(level + j, lw[i].next[j].item[k]);
				lw[i].next[j].n = 0;
			}
		}
	}

	if (ok) {
		for (i = 0; i < n && ok; ++i) {
			book->stats.n_todo += lw[i].w.n_todo;
			for (j = 0; j < lw[i].w.n; ++j) {
				if (book->todo_list.n == book->todo_list.size) {
					const long long size = book->todo_list.size + book->todo_list.size / 2 + 1024;
					unsigned long long *item = (unsigned long long*) realloc(book->todo_list.item, size * sizeof *item);
					if (item == NULL) { book->todo_list.valid = false; break; }
					book->todo_list.item = item; book->todo_list.size = size;
				}
				book->todo_list.item[book->todo_list.n++] = lw[i].w.item[j];
			}
		}
	}

	for (i = 0; i < n; ++i) {
		if (lw[i].next) { for (j = 0; j <= total_loss; ++j) free(lw[i].next[j].item); free(lw[i].next); }
		free(lw[i].same.item); free(lw[i].w.item);
	}
	for (L = 0; L <= total_loss; ++L) free(level[L].item);
	free(level);
	return ok;
}

/*
 * deviate selection ply by ply (breadth first), each ply with options.n_task threads.
 * Deviations and window only depend on the ply parity, so the visited set is the
 * one of the recursive walk unless a position is reached with both parities
 * (then the caller redoes the recursive walk).
 */
static void* depth_worker_run(void *v)
{
	LossWorker *lw = (LossWorker*) v;
	DeviateWorker *w = &lw->w;
	Book *book = w->book;
	const int parity = lw->level & 1;
	const int player_deviation = parity ? w->b : w->a; // the child ply uses the other deviation
	const int lower = parity ? -w->upper : w->lower, upper = parity ? -w->lower : w->upper;
	const unsigned char mark = (unsigned char) (parity + 1), child_mark = (unsigned char) ((parity ^ 1) + 1);
	long long k;
	Board target;

	for (k = lw->first; k < lw->last && !*w->conflict; ++k) {
		Position *position = lw->cur[k];
		const Link *l;
		unsigned char *v;

		if (!(lower <= position->score.value && position->score.value <= upper && board_count_empties(&position->board) >= book->options.n_empties && !board_is_game_over(&position->board))) continue;
		v = book_visit(book, position);
		if (atomic_load_uchar(v) || !atomic_cas_uchar(v, 0, mark)) {
			if (atomic_load_uchar(v) != mark) *w->conflict = true;
			continue;
		}
		foreach_link(l, position) {
			if (position->score.value - l->score <= player_deviation && lower <= l->score && l->score <= upper) {
				Position *child;
				board_next(&position->board, l->move, &target);
				child = book_probe(book, &target);
				if (child && atomic_load_uchar(book_visit(book, child)) != child_mark && !position_list_push(&lw->same, child)) w->oom = true;
			}
		}
		if (position->score.value - position->leaf.score <= player_deviation && lower <= position->leaf.score && position->leaf.score <= upper) {
			deviate_worker_todo(w, position);
		}
	}
	return NULL;
}

static bool book_deviate_by_depth(Book *book, Position *root, const int player_deviation, const int opponent_deviation, const int lower, const int upper)
{
	LossWorker lw[MAX_THREADS];
	PositionList cur = {0}, next = {0};
	volatile bool conflict = false;
	int i, depth, n = options.n_task;
	long long j;
	bool ok = true;

	if (n > MAX_THREADS) n = MAX_THREADS;
	if (n < 1) n = 1;
	if (!book->todo_list.valid) return false;
	for (i = 0; i < n; ++i) {
		memset(lw + i, 0, sizeof lw[i]);
		lw[i].w.book = book; lw[i].w.id = i; lw[i].w.conflict = &conflict;
		lw[i].w.a = player_deviation; lw[i].w.b = opponent_deviation; lw[i].w.lower = lower; lw[i].w.upper = upper;
	}
	ok = position_list_push(&cur, root);
	for (depth = 0; ok && !conflict && cur.n > 0; ++depth) {
		const int m = (cur.n < 4096) ? 1 : n;
		for (i = 0; i < m; ++i) {
			lw[i].cur = cur.item; lw[i].level = depth;
			lw[i].first = cur.n * i / m; lw[i].last = cur.n * (i + 1) / m;
			lw[i].same.n = 0;
		}
		book_run_workers(depth_worker_run, lw, sizeof *lw, m);
		next.n = 0;
		for (i = 0; i < m && ok; ++i) {
			if (lw[i].w.oom) ok = false;
			for (j = 0; j < lw[i].same.n && ok; ++j) ok = position_list_push(&next, lw[i].same.item[j]);
		}
		{ PositionList t = cur; cur = next; next = t; }
	}
	if (conflict || !ok) {
		ok = false;
	} else {
		for (i = 0; i < n && ok; ++i) {
			book->stats.n_todo += lw[i].w.n_todo;
			for (j = 0; j < lw[i].w.n; ++j) {
				if (book->todo_list.n == book->todo_list.size) {
					const long long size = book->todo_list.size + book->todo_list.size / 2 + 1024;
					unsigned long long *item = (unsigned long long*) realloc(book->todo_list.item, size * sizeof *item);
					if (item == NULL) { book->todo_list.valid = false; break; }
					book->todo_list.item = item; book->todo_list.size = size;
				}
				book->todo_list.item[book->todo_list.n++] = lw[i].w.item[j];
			}
		}
	}
	for (i = 0; i < n; ++i) { free(lw[i].same.item); free(lw[i].w.item); }
	free(cur.item); free(next.item);
	return ok;
}

/** walk wrappers: parallel when possible, else the original sequential walk (after a new book_clean) */
static void book_select_deviate(Book *book, Position *root, const int player_deviation, const int opponent_deviation, const int lower, const int upper)
{
	if (!book_visit_init(book)) return;
	if (!book_deviate_by_depth(book, root, player_deviation, opponent_deviation, lower, upper)) {
		book_clean(book);
		position_deviate(root, book, player_deviation, opponent_deviation, lower, upper);
	}
	book_visit_free(book);
}

static void book_select_deviate_total(Book *book, Position *root, const int move_loss, int total_loss, const bool skip_solved)
{
	if (total_loss > VISIT_LOSS_MAX) {
		warn("total loss %d reduced to %d\n", total_loss, VISIT_LOSS_MAX);
		total_loss = VISIT_LOSS_MAX;
	}
	if (!book_visit_init(book)) return;
	if (!book_deviate_total_by_loss(book, root, skip_solved ? 2 : 3, move_loss, total_loss)) {
		book_clean(book);
		book_visit_clear(book);
		position_deviate_total(root, book, move_loss, total_loss, 0, skip_solved);
	}
	book_visit_free(book);
}

/**
 * @brief Deviate a book.
 *
 * @param book opening book.
 * @param board Position to start from.
 * @param relative_error Error relative to the current position's score.
 * @param absolute_error Error relative to the root position's score.
 */
void book_deviate(Book *book, Board *board, const int relative_error, const int absolute_error)
{
	Position *root = book_probe(book, board);
	if (root) {
		int score;
		long long n_diffs;
		int rounds_since_save = 0;
		char file[FILENAME_MAX + 1];

		file_add_ext(options.book_file, ".dev", file);
		book_clean(book);
		book_negamax_position(root, book);

		do {
			score = root->score.value;

			bprint("Book deviate %d %d:\n", relative_error, absolute_error);
			book_clean(book);
			book_select_deviate(book, root, relative_error, 0, score - absolute_error, score + absolute_error);
			bprint("Book deviate %lld todo\n", book->stats.n_todo);

			book_expand(book, "Book deviate", file);
			n_diffs = book->stats.n_nodes + book->stats.n_links;

			// Adding positions may relocate the array containing root.
			root = book_probe(book, board);
			bprint("Book deviate %d %d:\n", relative_error, absolute_error);
			book_clean(book);
			book_select_deviate(book, root, 0, relative_error, score - absolute_error, score + absolute_error);
			bprint("Book deviate %lld todo\n", book->stats.n_todo);

			book_expand(book, "Book deviate", file);
			n_diffs += book->stats.n_nodes + book->stats.n_links;

			root = book_probe(book, board);
			book_clean(book);
			book_negamax_position(root, book);
			book_deviate_save_progress(book, file, n_diffs, &rounds_since_save);
		} while (n_diffs && !book->failed); // stop if a position cannot be added
		bprint("Book deviate %d %d...finished\n", relative_error, absolute_error);
	}
	search_pool_release(&expand_pool);
}

/**
 * @brief Deviate a book while limiting cumulative move losses.
 *
 * @param book opening book.
 * @param board Position to start from.
 * @param move_loss Maximum loss for one move.
 * @param total_loss Maximum cumulative loss for both players.
 */
void book_deviate2(Book *book, Board *board, const int move_loss, const int total_loss)
{
	Position *root = book_probe(book, board);
	if (root) {
		long long n_diffs;
		int rounds_since_save = 0;
		char file[FILENAME_MAX + 1];

		file_add_ext(options.book_file, ".dev2", file);
		book_clean(book);
		book_negamax_position(root, book);

		do {
			bprint("Book deviate2 %d %d:\n", move_loss, total_loss);
			book_clean(book);
			book_select_deviate_total(book, root, move_loss, total_loss, true);
			bprint("Book deviate2 %lld todo\n", book->stats.n_todo);

			book_expand(book, "Book deviate2", file);
			n_diffs = book->stats.n_nodes + book->stats.n_links;

			root = book_probe(book, board);
			book_clean(book);
			book_negamax_position(root, book);
			book_deviate_save_progress(book, file, n_diffs, &rounds_since_save);
		} while (n_diffs && !book->failed); // stop if a position cannot be added
		bprint("Book deviate2 %d %d...finished\n", move_loss, total_loss);
	}
	search_pool_release(&expand_pool);
}

void book_deviate3(Book *book, Board *board, const int move_loss, const int total_loss)
{
	Position *root = book_probe(book, board);
	if (root) {
		long long n_diffs;
		int rounds_since_save = 0;
		char file[FILENAME_MAX + 1];

		file_add_ext(options.book_file, ".dev3", file);
		book_clean(book);
		book_negamax_position(root, book);

		do {
			bprint("Book deviate3 %d %d:\n", move_loss, total_loss);
			book_clean(book);
			book_select_deviate_total(book, root, move_loss, total_loss, false);
			bprint("Book deviate3 %lld todo\n", book->stats.n_todo);

			book_expand(book, "Book deviate3", file);
			n_diffs = book->stats.n_nodes + book->stats.n_links;

			root = book_probe(book, board);
			book_clean(book);
			book_negamax_position(root, book);
			book_deviate_save_progress(book, file, n_diffs, &rounds_since_save);
		} while (n_diffs && !book->failed); // stop if a position cannot be added
		bprint("Book deviate3 %d %d...finished\n", move_loss, total_loss);
	}
	search_pool_release(&expand_pool);
}

/**
 * @brief Prune a book.
 *
 * Remove positions Edax cannot reach.
 *
 * @param book opening book.
 */
void book_prune(Book *book)
{
	PositionArray *a;
	Position *p;
	Position *root = book_root(book);
	int i;

	if (root) {
		book_clean(book);
		position_negamax(root, book);

		book_clean(book);
		position_prune(root, book, 2*SCORE_INF, 0, -SCORE_INF, SCORE_INF);
		position_print(root, &root->board, stdout);
		bprint("Book prune %lld... done\n", book->stats.n_todo);

		position_prune(root, book, 0, 2*SCORE_INF, -SCORE_INF, SCORE_INF);
		bprint("Book prune %lld... done\n", book->stats.n_todo);
		for (a = book->array; a < book->array + book->n; ++a)
		for (i = 0; i < a->n; ++i) if (!position_is_done(a->positions + i, book)) {book_remove(book, a->positions + i); --i;}
		foreach_position(p, a, book) position_remove_links(p, book);
		bprint("done\n");
	}
}

/**
 * @brief Prune a book.
 *
 * Remove positions Edax cannot reach.
 *
 * @param book opening book.
 */
void book_subtree(Book *book, const Board *board)
{
	PositionArray *a;
	Position *p;
	Position *root = book_probe(book, board);
	int i;

	if (root) {
		book_clean(book);
		position_negamax(root, book);

		book_clean(book);
		position_prune(root, book, 2*SCORE_INF, 2*SCORE_INF, -SCORE_INF, SCORE_INF);
		position_print(root, &root->board, stdout);
		bprint("Book subtree %lld... done\n", book->stats.n_todo);
		for (a = book->array; a < book->array + book->n; ++a)
		for (i = 0; i < a->n; ++i) if (!position_is_done(a->positions + i, book)) {book_remove(book, a->positions + i); --i;}
		foreach_position(p, a, book) position_remove_links(p, book);
		bprint("done\n");
	}
}


/**
 * @brief Enhance a book.
 *
 * @param book opening book.
 * @param board Position to start from.
 * @param midgame_error Error in midgame search.
 * @param endcut_error Error in endgame search.
 */
void book_enhance(Book *book, Board *board, const int midgame_error, const int endcut_error)
{
	Position *root = book_probe(book, board);
	if (root) {
		long long n_diffs;
		char file[FILENAME_MAX + 1];

		file_add_ext(options.book_file, ".enh", file);

		book->options.midgame_error = midgame_error;
		book->options.endcut_error = endcut_error;

		book_clean(book);
		position_negamax(root, book);

		do {
			bprint("Book enhance %d %d...%lld %lld:\n", midgame_error, endcut_error, book->stats.n_nodes, book->stats.n_links);
			book_clean(book);
			position_enhance(root, book);
			book_expand(book, "Book enhance", file);
			n_diffs = book->stats.n_nodes + book->stats.n_links;

			root = book_probe(book, board);
			book_clean(book);
			position_negamax(root, book);
			if (n_diffs) book_save_progress(book, file);
		} while (n_diffs && !book->failed); // stop if a position cannot be added
		bprint("Book enhance %d %d...finished\n", midgame_error, endcut_error);
	}
	search_pool_release(&expand_pool);
}

/**
 * @brief display some book's informations.
 *
 * @param book opening book.
 */
void book_info(Book *book)
{
	PositionArray *a;
	Position *p;
	unsigned long long n_links = 0;
	unsigned long long n_leaves = 0;
	unsigned long long n_level[61] = {0};
	int min_array = INT_MAX, max_array = 0;
	int i;

	foreach_position(p, a, book) {
		n_links += p->n_link;
		if (p->leaf.move != NOMOVE) ++n_leaves;
		++n_level[p->level];
		if (p->level != book->options.level) {
			position_print(p, &p->board, stdout);
		}
	}

	for (a = book->array; a < book->array + book->n; ++a) {
		if (a->n > max_array) max_array = a->n;
		if (a->n < min_array) min_array = a->n;
	}

	bprint("Edax Book %d.%d; ", VERSION, RELEASE);
	bprint("%d-%d-%d ", book->date.year, book->date.month, book->date.day);
	bprint("%d:%02d:%02d;\n", book->date.hour, book->date.minute, book->date.second);
	bprint("Positions: %u (moves = %lld links + %lld leaves);\n", book->n_nodes, n_links, n_leaves);
	for (i = 0; i < 61; ++i) {
		if (n_level[i]) {
			bprint("Level %d : %lld nodes\n", i, n_level[i]);
		}
	}
	bprint("Depth: %d\n", 61 - book->options.n_empties);
	bprint("Memory occupation: %lld\n", (long long) ((size_t) book->n_nodes * sizeof (Position) + book->n * sizeof (PositionArray) + n_links * sizeof (Link)));
	bprint("Hash balance: %d < %d < %d\n", min_array, (int) (book->n_nodes / book->n), max_array);
}

/**
 * @brief Display a position from the book.
 *
 * @param book opening book.
 * @param board position to display.
 */
void book_show(Book *book, Board *board)
{
	GameStats stat = {0,0,0,0};
	Position *position = book_probe(book, board);
	unsigned long long n_games;

	if (position) {
		position_show(position, board, stdout);
		book_get_game_stats(book, board, &stat);
		n_games = stat.n_wins + stat.n_draws + stat.n_losses;
		if (n_games) {
			bprint("\nLines: %lld full games", n_games);
			bprint(" with %.2f%% win, %.2f%% draw, %.2f%% loss",
				100.0 * stat.n_wins / n_games, 100.0 * stat.n_draws / n_games, 100.0 * stat.n_losses / n_games);
		}
		bprint("\n       %lld incomplete lines.\n\n", stat.n_lines - n_games);
	}
}

/**
 * @brief Get a list of moves from the book.
 *
 * @param book Opening book.
 * @param board Position to display.
 * @param movelist List of moves.
 */
bool book_get_moves(Book *book, const Board *board, MoveList *movelist)
{
	Position *position = book_probe(book, board);
	if (position) {
		position_get_moves(position, board, movelist);
		return true;
	}

	return false;
}

/**
 * @brief Get a variation from the book.
 *
 * @param book Opening book.
 * @param board Position.
 * @param move First move;
 * @param line Bariation.
 */
void book_get_line(Book *book, const Board *board, const Move *move, Line *line)
{
	Position *position;
	Board b;
	Move m;

	line_push(line, move->x);
	board_next(board, move->x, &b);

	while ((position = book_probe(book, &b)) != NULL && !board_is_game_over(&position->board)) {
		position_get_random_move(position, &b, &m, &book->random, 0);
		line_push(line, m.x);
		board_update(&b, &m);
	}
}


/**
 * @brief Get a move at random from the opening book.
 *
 * @param book Opening book.
 * @param board Position to find a move from.
 * @param move Chosen move.
 * @param randomness Randomness.
 */
#if 0
#include "srbook.c"
#else
bool book_get_random_move(Book *book, const Board *board, Move *move, const int randomness)
{
	return book_get_random_move_with(book, board, move, randomness, &book->random);
}

/**
 * @brief Get a move at random from the opening book, with a given random generator.
 *
 * The book is only read: several threads can call it at the same time, each one with its generator.
 *
 * @param book Opening book.
 * @param board Position to find a move from.
 * @param move Chosen move.
 * @param randomness Randomness.
 * @param random Random generator.
 */
bool book_get_random_move_with(Book *book, const Board *board, Move *move, const int randomness, Random *random)
{
	Position *position = book_probe(book, board);
	if (position) {
		position_get_random_move(position, board, move, random, randomness);
		return true;
	}

	return false;
}
#endif

/**
 * @brief Get game statistics from a position.
 *
 * @param book Opening book.
 * @param board Position to find a move from.
 * @param stat Game statistics output.
 */
void book_get_game_stats(Book *book, const Board *board, GameStats *stat)
{
	Position *position;

	assert(book != NULL);
	assert(board !=NULL);
	assert(stat != NULL);
	
	stat->n_wins = stat->n_losses = stat->n_draws = stat->n_lines = 0;

	position = book_probe(book, board);
	if (position) {
		if (position->n_wins == UINT_MAX || position->n_losses == UINT_MAX || position->n_draws == UINT_MAX || position->n_lines == UINT_MAX) {
			Board target;
			Link *l;
			GameStats child;
			
			foreach_link(l, position) {
				board_next(&position->board, l->move, &target);
				book_get_game_stats(book, &target, &child);
				stat->n_wins += child.n_losses;
				stat->n_draws += child.n_draws;
				stat->n_losses += child.n_wins;
				stat->n_lines += child.n_lines;
			}
		} else {
			stat->n_wins = position->n_wins;
			stat->n_draws = position->n_draws;
			stat->n_losses = position->n_losses;
			stat->n_lines = position->n_lines;
		}
	}
}


/*
 * Learning games with several threads (book-store-tasks > 1).
 *
 * book_add_board() searches the positions of a game one after the other. Which positions it is
 * going to search, and which moves of each one are links (so excluded from its search), only
 * depend on the positions that are in the book and on the ones that the games add. So:
 * 1. plan: go through the boards as book_add_board() will, without changing the book, and note
 *    each search (book_plan_board);
 * 2. search: do all these searches at the same time, each one with its own search and with
 *    empty hash tables (book_plan_search);
 * 3. add: call book_add_board() as usual. position_search() takes the result of the planned
 *    search of the same board with the same links, or searches as usual if there is none.
 * The book is changed by the third step only, in the same order as without a plan.
 */

/**
 * @brief Release the searches used to learn games (store_pool).
 */
void book_store_release(void)
{
	search_pool_release(store_pool);
	search_pool_release(store_pool + 1);
}

/**
 * @brief Size of the hash tables of a search used to learn games.
 *
 * hash-table-size = n: this size. auto: the size for the threads of the search.
 * A one-thread search starts with empty tables and is short at a low level, and there are as
 * many of them as threads: it never gets more than 19 bits (14 MB) up to level 18, 20 bits up
 * to level 21, 21 bits above, whatever hash-table-size is (32 searches with the tables of
 * hash-table-size = 24 would take 14 GB). At level 18, the searches of 30 games visited 0.7%
 * more nodes with 19 bits than with 21 bits (1.5% more with 18 bits).
 *
 * @param book Opening book.
 * @param n_tasks Threads of the search.
 * @return size (in number of bits).
 */
static int book_store_hash_bits(const Book *book, const int n_tasks)
{
	const int level = MAX(book->options.level, options.level);
	int bits;

#ifdef BOOK_TEST_HASH_BITS
	if (n_tasks == 1) return BOOK_TEST_HASH_BITS; // test builds: to choose the size
#endif
	bits = options.hash_table_auto ? hash_table_size_auto(n_tasks) : options.hash_table_size;
	if (n_tasks == 1) bits = MIN(bits, level <= 18 ? 19 : (level <= 21 ? 20 : 21));
	return bits;
}

/**
 * @brief Get searches to play games at the same time (see play_learn_games).
 *
 * @param book Opening book.
 * @param n Number of searches.
 * @param n_tasks Threads of each search.
 * @return the searches, NULL if they cannot be allocated.
 */
Search** book_store_searches(const Book *book, const int n, const int n_tasks)
{
	// with one thread each, they are the searches of the positions
	SearchPool *pool = store_pool + (n_tasks > 1);

	if (search_pool_get(pool, n, n_tasks, book_store_hash_bits(book, n_tasks)) < n) {
		// not all of them: the games are played one after the other, without these searches. The ones that were
		// created are released (they kept their memory, that the searches of the positions then lacked)
		search_pool_release(pool);
		return NULL;
	}
	return pool->search;
}

/**
 * @brief Number of games learned at the same time.
 *
 * book-store-tasks = n, or auto (0): each game gets 1 thread.
 * Always 1 with the cpu option where it binds the threads (see book_one_search_at_a_time).
 *
 * @return the number of games learned at the same time; 1 = one position after the other, as
 * Edax always did.
 */
int book_store_task_count(void)
{
#ifdef BOOK_TEST_STORE
	return BOOK_TEST_STORE; // test builds: whatever the search uses
#else
	int n = options.book_store_tasks;

	if (book_one_search_at_a_time()) return 1;
	if (n <= 0) n = options.n_task;
	return MAX(1, MIN(n, options.n_task));
#endif
}

/**
 * @brief Number of threads used to learn games (book-store-tasks > 1).
 * @return number of threads.
 */
int book_store_thread_count(void)
{
	return MAX(1, MIN(book_n_task(), MAX_THREADS - 1));
}

#define PLAN_LEAF_UNKNOWN 0xfe /**< leaf of a position whose search is planned */

/** a planned search */
typedef struct PlanJob {
	Board board;               /**< board to search */
	unsigned long long links;  /**< moves that are links (excluded from the search) */
	Link leaf;                 /**< result of the search */
	unsigned char level;       /**< level of the search */
	bool pass;                 /**< the pass is a link */
	bool done;                 /**< the search was done */
} PlanJob;

/** a position as it will be after the boards already planned (in the book before the plan, or added by it) */
typedef struct PlanNode {
	Board board;               /**< unique board */
	unsigned long long links;  /**< moves that are links */
	unsigned char leaf;        /**< leaf move (NOMOVE: none, PLAN_LEAF_UNKNOWN: given by a planned search) */
	unsigned char level;       /**< level of the position */
	bool pass;                 /**< the pass is a link */
	bool no_score;             /**< its score is -SCORE_INF */
} PlanNode;

struct PlanWorker;

typedef struct BookPlan {
	Book *book;
	PlanJob *job;
	PlanNode *node;
	int n_job, job_size, n_node, node_size;
	int *job_index, *node_index;   /**< hash tables of the jobs and of the nodes: index + 1 (0: free slot) */
	unsigned int job_mask, node_mask;
	bool failed;                   /**< out of memory: nothing more is planned */
	bool usual;                    /**< a single search: it is done as usual, when the position is added */
	int n_used, n_missed;          /**< searches taken from the plan, searches that were not planned */
	Lock lock;                     /**< guards next, n_done, and the workers */
	int next, n_done;
	struct PlanWorker *worker;     /**< threads doing the searches (book_plan_search) */
	int n_worker, n_threads;       /**< workers started, threads that they share */
	int n_continued;               /**< searches stopped and continued with more threads */
} BookPlan;

/** a thread doing planned searches */
typedef struct PlanWorker {
	BookPlan *plan;
	Search *search;
	Thread thread;
	bool progress;                 /**< this one shows the progress */
	int n_tasks;                   /**< threads of its search */
	/* guarded by the lock of the plan: */
	int want;                      /**< threads that its search should have */
	int run_tasks;                 /**< threads of the search that it is running (0: it is not searching) */
	bool busy;                     /**< it has a job */
} PlanWorker;

#define plan_slot(board, mask) ((unsigned int) (board_get_hash_code(board) >> 24) & (mask))

/** Add an item to a hash table of the plan (growing it if needed). */
static bool plan_index_add(BookPlan *plan, int **index, unsigned int *mask, const int n, const bool is_job)
{
	unsigned int j;
	int i;

	if (*index == NULL || (unsigned int) n * 2 > *mask) { // rebuild a larger table
		const unsigned int size = *index ? (*mask + 1) * 2 : 256;
		int *t = (int*) calloc(size, sizeof *t);
		if (t == NULL) return false;
		free(*index);
		*index = t; *mask = size - 1;
		i = 0;
	} else {
		i = n - 1;
	}
	for (; i < n; ++i) {
		const Board *board = is_job ? &plan->job[i].board : &plan->node[i].board;
		for (j = plan_slot(board, *mask); (*index)[j]; j = (j + 1) & *mask) ;
		(*index)[j] = i + 1;
	}
	return true;
}

static PlanNode* plan_node_find(const BookPlan *plan, const Board *unique)
{
	unsigned int j;

	if (plan->node_index == NULL) return NULL;
	for (j = plan_slot(unique, plan->node_mask); plan->node_index[j]; j = (j + 1) & plan->node_mask) {
		PlanNode *node = plan->node + plan->node_index[j] - 1;
		if (board_equal(&node->board, unique)) return node;
	}
	return NULL;
}

static PlanNode* plan_node_add(BookPlan *plan, const PlanNode *node)
{
	if (plan->n_node == plan->node_size) {
		const int size = plan->node_size * 2 + 64;
		PlanNode *n = (PlanNode*) realloc(plan->node, size * sizeof *n);
		if (n == NULL) { plan->failed = true; return NULL; }
		plan->node = n; plan->node_size = size;
	}
	plan->node[plan->n_node] = *node;
	if (!plan_index_add(plan, &plan->node_index, &plan->node_mask, plan->n_node + 1, false)) { plan->failed = true; return NULL; }
	return plan->node + plan->n_node++;
}

static PlanJob* plan_job_find(const BookPlan *plan, const Board *board, const unsigned long long links, const bool pass, const int level)
{
	unsigned int j;

	if (plan->job_index == NULL) return NULL;
	for (j = plan_slot(board, plan->job_mask); plan->job_index[j]; j = (j + 1) & plan->job_mask) {
		PlanJob *job = plan->job + plan->job_index[j] - 1;
		if (board_equal(&job->board, board) && job->links == links && job->pass == pass && job->level == level) return job;
	}
	return NULL;
}

static void plan_job_add(BookPlan *plan, const Board *board, const unsigned long long links, const bool pass, const int level)
{
	PlanJob *job;

	if (plan_job_find(plan, board, links, pass, level)) return;
	if (plan->n_job == plan->job_size) {
		const int size = plan->job_size * 2 + 64;
		job = (PlanJob*) realloc(plan->job, size * sizeof *job);
		if (job == NULL) { plan->failed = true; return; }
		plan->job = job; plan->job_size = size;
	}
	job = plan->job + plan->n_job;
	job->board = *board;
	job->links = links;
	job->leaf = BAD_LINK;
	job->level = (unsigned char) level;
	job->pass = pass;
	job->done = false;
	if (!plan_index_add(plan, &plan->job_index, &plan->job_mask, plan->n_job + 1, true)) { plan->failed = true; return; }
	++plan->n_job;
}

/** @return true if a board is in the book, or will be when the boards already planned are added. */
static bool plan_has_board(const BookPlan *plan, const Book *book, const Board *board)
{
	Board unique;

	board_unique(board, &unique);
	return plan_node_find(plan, &unique) != NULL
	    || position_array_probe(book->array + (board_get_hash_code(&unique) & (book->n - 1)), &unique) != NULL;
}

/**
 * @brief Moves of a board that position_link() would link.
 * @param pass Set to true if the pass would be linked.
 * @return the moves, as a bitboard.
 */
static unsigned long long plan_links(const BookPlan *plan, const Book *book, const Board *board, bool *pass)
{
	unsigned long long moves = board_get_moves(board), links = 0;
	Board next;
	int x;

	*pass = false;
	if (moves) {
		foreach_bit(x, moves) {
			board_next(board, x, &next);
			if (plan_has_board(plan, book, &next)) links |= x_to_bit(x);
		}
	} else if (can_move(board->opponent, board->player)) {
		next.player = board->opponent;
		next.opponent = board->player;
		*pass = plan_has_board(plan, book, &next);
	}
	return links;
}

/**
 * @brief Start a plan: the searches of the next calls to book_add_board() are done ahead, at the same time.
 *
 * Call book_plan_board() with the same boards, in the same order, as book_add_board() will get,
 * then book_plan_search(), then book_add_board() for each board, then book_plan_end().
 *
 * @param book Opening book.
 * @return false if it cannot be allocated (book_add_board() searches as usual).
 */
bool book_plan_begin(Book *book)
{
	BookPlan *plan = (BookPlan*) calloc(1, sizeof *plan);

	if (plan == NULL) return false;
	plan->book = book;
	lock_init(plan);
	book_plan = plan;
	return true;
}

/**
 * @brief Plan the search that book_add_board() will do for a board.
 *
 * @param book Opening book (not changed).
 * @param board Board that book_add_board() will get.
 */
void book_plan_board(Book *book, const Board *board)
{
	BookPlan *plan = book_plan;
	Board unique;
	PlanNode *node, new_node;
	unsigned long long links;
	int n_moves, n_link;
	bool pass;

	if (plan == NULL || plan->failed || board_count_empties(board) < book->options.n_empties - 1) return;
	board_unique(board, &unique);
	node = plan_node_find(plan, &unique);
	if (node == NULL) {
		const Position *p = position_array_probe(book->array + (board_get_hash_code(&unique) & (book->n - 1)), &unique);
		const Link *l;

		new_node.board = unique;
		if (p == NULL) { // a new position: linked and searched as it is given, then added
			links = plan_links(plan, book, board, &pass);
			n_link = bit_count(links) + pass;
			n_moves = get_mobility(board->player, board->opponent);
			if (n_link < n_moves || (n_link == 0 && n_moves == 0)) plan_job_add(plan, board, links, pass, book->options.level);
			new_node.links = plan_links(plan, book, &unique, &new_node.pass);
			new_node.leaf = PLAN_LEAF_UNKNOWN;
			new_node.level = (unsigned char) book->options.level;
			new_node.no_score = false;
			plan_node_add(plan, &new_node);
			return;
		}
		new_node.links = 0;
		new_node.pass = false;
		foreach_link(l, p) {
			if (l->move == PASS) new_node.pass = true;
			else if (l->move <= H8) new_node.links |= x_to_bit(l->move);
		}
		new_node.leaf = p->leaf.move;
		new_node.level = p->level;
		new_node.no_score = (p->score.value == -SCORE_INF);
		node = plan_node_add(plan, &new_node);
		if (node == NULL) return;
	}

	// a position of the book: linked, then searched if its leaf is (now) a link
	links = plan_links(plan, book, &unique, &pass);
	if (node->leaf <= H8 ? (links & ~node->links & x_to_bit(node->leaf)) != 0 : (node->leaf == PASS && pass && !node->pass)) node->leaf = NOMOVE;
	// the leaf that a planned search will find is not known yet: if the position gets a new link,
	// it may be that leaf. Search it with the new links as well, in case it is (else this search is not used)
	if (node->leaf == PLAN_LEAF_UNKNOWN && ((links & ~node->links) != 0 || (pass && !node->pass))) node->leaf = NOMOVE;
	node->links |= links;
	node->pass = node->pass || pass;
	if (node->leaf == NOMOVE) {
		n_link = bit_count(node->links) + node->pass;
		n_moves = get_mobility(unique.player, unique.opponent);
		if (n_link < n_moves || (n_link == 0 && n_moves == 0 && node->no_score)) {
			plan_job_add(plan, &unique, node->links, node->pass, node->level);
			node->leaf = PLAN_LEAF_UNKNOWN;
			node->no_score = false;
		}
	}
}

/**
 * Give the threads of the workers that have no job to the searches that are still running.
 *
 * Called with the lock of the plan, when there is no search left to start. A search that can get
 * at least twice its threads is stopped: its worker runs it again with more threads and the same
 * hash tables, which give back what was already searched. A level 24 search that takes 25 s with
 * one thread no longer keeps 31 threads idle until it ends.
 */
static void plan_share_threads(BookPlan *plan)
{
#ifndef BOOK_TEST_ONE_THREAD
	int i, n_busy = 0, n;

	for (i = 0; i < plan->n_worker; ++i) if (plan->worker[i].busy) ++n_busy;
	if (n_busy == 0) return;
	n = MIN(plan->n_threads / n_busy, MAX_THREADS - 1);
	for (i = 0; i < plan->n_worker; ++i) {
		PlanWorker *w = plan->worker + i;
		if (w->busy && n >= 2 * w->want) w->want = n;
		// (also asked again when a first request came before the search started)
		if (w->busy && w->run_tasks > 0 && w->run_tasks < w->want) search_stop_all(w->search, STOP_ON_DEMAND);
	}
#else
	(void) plan; // test builds: every search has one thread, to compare with the searches done one after the other
#endif
}

/** Thread doing the planned searches. */
static void* plan_worker_run(void *v)
{
	PlanWorker *w = (PlanWorker*) v;
	BookPlan *plan = w->plan;
	long long next = real_clock() + 1000;

	for (;;) {
		PlanJob *job;
		Position p;
		Link link;
		unsigned long long links;
		int i, x, n, n_done;
		bool done;

		lock(plan);
		i = plan->next < plan->n_job ? plan->next++ : -1;
		w->busy = (i >= 0);
		if (plan->next >= plan->n_job) plan_share_threads(plan); // no search left to start
		unlock(plan);
		if (i < 0) break;

		job = plan->job + i;
		search_cleanup(w->search); // empty hash tables: the result does not depend on the searches done before
		w->search->options.keep_date = false;
		for (;;) {
			position_init(&p);
			p.board = job->board;
			p.level = job->level;
			link.score = -SCORE_INF; // the search does not use the scores of the links
			links = job->links;
			foreach_bit(x, links) {
				link.move = (unsigned char) x;
				position_add_link(&p, &link);
			}
			if (job->pass) {
				link.move = PASS;
				position_add_link(&p, &link);
			}

			for (;;) { // the threads of this search
				lock(plan);
				n = w->want;
				if (n == w->n_tasks) w->run_tasks = n; // (a stop is only asked while run_tasks is set)
				unlock(plan);
				if (n == w->n_tasks) break;
				search_set_task_number(w->search, n);
				w->n_tasks = n;
			}
			done = (position_search_with(&p, w->search) & 2) != 0;
			lock(plan);
			w->run_tasks = 0;
			if (done && w->search->stop == STOP_ON_DEMAND) ++plan->n_continued;
			unlock(plan);
			if (!done || w->search->stop != STOP_ON_DEMAND) break; // the search ended by itself
			// stopped to get more threads: search again, with what the hash tables kept
			position_free(&p);
			w->search->options.keep_date = true;
		}
		w->search->options.keep_date = false;
		if (done) {
			job->leaf = p.leaf;
			job->done = true;
		}
		position_free(&p);

		lock(plan);
		n_done = ++plan->n_done;
		unlock(plan);
		if (w->progress && book_progress_due(&next)) bprint("Searching positions...%d/%d\r", n_done, plan->n_job);
	}
	return NULL;
}

/**
 * @brief Size of the hash tables of a planned search that starts with several threads.
 *
 * The size of a one-thread search (book_store_hash_bits), doubled each time the threads are: the
 * searches done at the same time never take more memory together than as many one-thread searches
 * as threads. It is never more than the size that hash-table-size gives to a search with these threads.
 *
 * @param book Opening book.
 * @param n_tasks Threads of the search.
 * @return size (in number of bits).
 */
static int book_plan_hash_bits(const Book *book, const int n_tasks)
{
	int bits = book_store_hash_bits(book, 1), n;

	if (n_tasks > 1) {
		for (n = 2; n <= n_tasks; n *= 2) ++bits;
		bits = MIN(bits, book_store_hash_bits(book, n_tasks));
	}
	return bits;
}

/**
 * @brief Do the planned searches at the same time.
 *
 * The threads (n-tasks) are shared between the searches:
 * - as many searches as threads, or more: one thread each;
 * - fewer searches: each one starts with n-tasks / searches threads;
 * - when no search is left to start, the searches that are still running get the threads of the
 *   ones that ended (plan_share_threads).
 * A one-thread search only depends on its position: it starts with empty hash tables. A search with
 * several threads can give another leaf from a run to the next, as any search with several threads.
 * The searches of the pool are created while there are searches left to start: a few short searches
 * do not pay for the memory of as many searches as threads.
 * A single search is not done here: it is done as usual when its position is added (all the threads,
 * the hash tables of the main search), exactly as with book-store-tasks = 1.
 *
 * @param book Opening book (not changed).
 */
void book_plan_search(Book *book)
{
	BookPlan *plan = book_plan;
	PlanWorker w[MAX_THREADS];
	Search *search;
	int i, n, n_tasks, bits;
	bool self = false; // the last worker has no thread: this thread did its searches

	if (plan == NULL || plan->failed || plan->n_job == 0) return;
#ifndef BOOK_TEST_ONE_THREAD
	if (plan->n_job == 1) { plan->usual = true; return; }
#endif
	plan->n_threads = book_store_thread_count();
	n = MIN(plan->n_threads, plan->n_job);
	n_tasks = MIN(plan->n_threads / n, MAX_THREADS - 1);
#ifdef BOOK_TEST_ONE_THREAD
	n_tasks = 1;
#endif
	bits = book_plan_hash_bits(book, n_tasks);

	bprint("Searching positions...\r");
	plan->worker = w;
	plan->n_worker = 0;
	for (i = 0; i < n; ++i) {
		bool left;
		if (search_pool_get(store_pool, i + 1, 1, bits) <= i) { // not enough memory
			if (i == 0) { // no search at all: the positions are searched as usual, when they are added
				warn("not enough memory to search the positions at the same time\n");
				plan->worker = NULL;
				return;
			}
			break; // the workers already started do all the searches
		}
		search = store_pool->search[i];
		w[i].plan = plan;
		w[i].search = search;
		w[i].progress = (i == 0);
		w[i].n_tasks = search_count_tasks(search);
		w[i].run_tasks = 0;
		w[i].busy = true; // until it finds no job
		search->options.verbosity = 0;
		lock(plan);
		w[i].want = n_tasks;
		plan->n_worker = i + 1;
		left = (plan->next < plan->n_job);
		unlock(plan);
		if (!thread_create(&w[i].thread, plan_worker_run, w + i)) { // no thread (memory exhausted):
			plan_worker_run(w + i); // this thread does the searches of this worker, and no other worker is started
			self = true;
			break;
		}
		if (!left) break; // the searches are short: no more worker is needed
	}
	n = plan->n_worker;
	for (i = 0; i < (self ? n - 1 : n); ++i) thread_join(w[i].thread);
	for (i = 0; i < n; ++i) { // back to the state of the pool
		if (w[i].n_tasks != 1) search_set_task_number(w[i].search, 1);
		w[i].search->options.keep_date = false;
	}
	plan->worker = NULL;
	plan->n_worker = 0;
#ifdef BOOK_TEST_STORE
	fprintf(stderr, "<plan search: %d searches, %d workers, %d threads each at first, %d continued with more threads>\n", plan->n_job, n, n_tasks, plan->n_continued);
#endif
	bprint("Searching positions...%d done\n", plan->n_job);
}

/**
 * @brief Search a position as position_search_with() does, with the result of the planned search.
 *
 * @param position Position to search.
 * @param book Opening book.
 * @return 1 if the leaf became a link, | 2 if a search was done.
 */
static int position_search_planned(Position *position, Book *book)
{
	BookPlan *plan = book_plan;
	const int n_moves = get_mobility(position->board.player, position->board.opponent);
	int r = 0;

	if (position->leaf.move != NOMOVE && position_add_link(position, &position->leaf)) {
		r = 1;
	}

	if (position->n_link < n_moves || (position->n_link == 0 && n_moves == 0 && position->score.value == -SCORE_INF)) {
		unsigned long long links = 0;
		bool pass = false;
		const Link *l;
		const PlanJob *job;

		foreach_link(l, position) {
			if (l->move == PASS) pass = true;
			else if (l->move <= H8) links |= x_to_bit(l->move);
		}
		job = (plan->book == book) ? plan_job_find(plan, &position->board, links, pass, position->level) : NULL;
		if (job && job->done) {
			position->leaf = job->leaf;
			if (position->leaf.score > position->score.value) {
				position->score.value = position->leaf.score;
			}
			++plan->n_used;
			r |= 2;
		} else if (job && plan->usual) { // the single search of the plan
			++plan->n_used;
			r |= position_search_with(position, book->search);
		} else { // not planned: search now
			++plan->n_missed;
#ifdef BOOK_TEST_STORE
			fprintf(stderr, "<not planned: %016llx %016llx links %016llx pass %d level %d n_link %d n_moves %d leaf %d empties %d>\n", position->board.player, position->board.opponent, links, pass, position->level, position->n_link, n_moves, position->leaf.move, board_count_empties(&position->board));
#endif
			r |= position_search_with(position, book->search);
		}
	}
	return r;
}

/**
 * @brief End a plan: book_add_board() searches as usual again.
 *
 * @param book Opening book.
 */
void book_plan_end(Book *book)
{
	BookPlan *plan = book_plan;

	if (plan == NULL) return;
#ifdef BOOK_TEST_STORE
	fprintf(stderr, "<book plan: %d searches planned, %d used, %d not planned>\n", plan->n_job, plan->n_used, plan->n_missed);
#endif
	if (plan->n_missed) {
		info("<book plan: %d searches planned, %d used, %d not planned>\n", plan->n_job, plan->n_used, plan->n_missed);
	}
	book_plan = NULL;
	lock_free(plan);
	free(plan->job); free(plan->node); free(plan->job_index); free(plan->node_index);
	free(plan);
	(void) book;
}

/**
 * @brief Add a position.
 *
 * @param book opening book.
 * @param board position to add.
 */
void book_add_board(Book *book, const Board *board)
{
	Position position;
	Position *probe;

	if (board_count_empties(board) >= book->options.n_empties - 1) {
		probe = book_probe(book, board);
		if (probe) {
			position_link(probe, book);
			if (probe->leaf.move == NOMOVE) position_search(probe, book);
			if (BOOK_DEBUG) {printf("update: "); position_print(probe, board, stdout);}
		} else {
			position_init(&position);
			position.board = *board;
			position.level = book->options.level;
			position_link(&position, book);
			position_search(&position, book);
			if (BOOK_DEBUG) {printf("new: "); position_print(&position, board, stdout);}
			position_unique(&position);
			if (book_add(book, &position) <= 0) position_free(&position);
		}
	}
}

/**
 * @brief Add positions from a game.
 *
 * @param book opening book.
 * @param game game to add.
 */
void book_add_game(Book *book, const Game *game)
{
	char file[FILENAME_MAX + 1];
	const long long n_stats = book->stats.n_nodes + book->stats.n_links;

	file_add_ext(options.book_file, ".gam", file);

	if (!book_game_boards(book, game, false)) return; // skip non standard game

	if (book->stats.n_nodes + book->stats.n_links > n_stats && book_get_age(book) > 3600) book_save_progress(book, file);
}

/**
 * @brief Add the positions of a game, or plan their searches (see book_plan_begin).
 *
 * @param book opening book.
 * @param game game to add.
 * @param plan Only plan the searches.
 * @return false if the game is skipped (it does not start from the standard position).
 */
static bool book_game_boards(Book *book, const Game *game, const bool plan)
{
	Board board;
	Move stack[99];
	int i, n_moves;

	board_init(&board);
	if (!board_equal(&board, &game->initial_board)) return false; // skip non standard game
	for (i = n_moves = 0; i < 60 - book->options.n_empties && game->move[i] != NOMOVE; ++i) {
		if (!can_move(board.player, board.opponent)) {
			stack[n_moves++] = MOVE_PASS;
			board_pass(&board);
		}
		if (!board_is_occupied(&board, game->move[i]) && board_get_move_flip(&board, game->move[i], &stack[n_moves])) {
			board_update(&board, stack + n_moves);
			++n_moves;
		} else {
			if (!plan) warn("illegal move in game");
			break; // stop, illegal moves
		}
	}

	if (!plan) search_cleanup(book->search);
	while (--n_moves >= 0) {
		if (plan) book_plan_board(book, &board);
		else book_add_board(book, &board);
		board_restore(&board, stack + n_moves);
	}
	return true;
}

/**
 * @brief Add positions from a game database.
 *
 * @param book opening book.
 * @param base games to add.
 */
void book_add_base(Book *book, const Base *base)
{
	int i, j;
	char file[FILENAME_MAX + 1];
	long long t0, t;
	const int n_tasks = book_store_task_count();

	file_add_ext(options.book_file, ".gam", file);

	book_clean(book);
	bprint("Adding %d games to book...\n", base->n_games);
	t0 = real_clock();
	for (i = 0; i < base->n_games; ++i) {
		if (n_tasks > 1 && i % n_tasks == 0) { // book-store-tasks: the searches of the next n_tasks games are done ahead, at the same time
			book_plan_end(book);
			if (book_plan_begin(book)) {
				for (j = i; j < base->n_games && j < i + n_tasks; ++j) book_game_boards(book, base->game + j, true);
				book_plan_search(book);
			}
		}
		book_add_game(book, base->game + i);
		t = real_clock();
		if (t - t0 > 1000) {
		    bprint("Adding games...%d/%d done: %lld positions, %lld links\r", i + 1, base->n_games, book->stats.n_nodes, book->stats.n_links);
			t0 = t;
		}
		if (book->search->options.verbosity) putchar('\n');
		
	}
	book_plan_end(book);
	bprint("Adding games...%d/%d done: %lld positions, %lld links\n", i, base->n_games, book->stats.n_nodes, book->stats.n_links);
	bprint("%d games added to book\n", i);

	book_save_progress(book, file);
}

typedef struct BookCheckGame {
	unsigned long long missing;
	unsigned long long good;
	unsigned long long bad;
} BookCheckGame;

/**
 * @brief Check positions from a game.
 *
 * @param book opening book.
 * @param hash Board + Move hash table.
 * @param game game to check.
 * @param stat Count statistics.
 */
void book_check_game(Book *book, MoveHash *hash, const Game *game, BookCheckGame *stat)
{
	Board board;
	Move stack[99], *iter;
	MoveList movelist;
	int i, n_moves;
	int bestscore;

	board_init(&board);
	if (!board_equal(&board, &game->initial_board)) return; // skip non standard game
	for (i = n_moves = 0; i <= 60 - book->options.n_empties && game->move[i] != NOMOVE; ++i) {
		if (!can_move(board.player, board.opponent)) {
			stack[n_moves++] = MOVE_PASS;
			board_pass(&board);
		}
		if (!board_is_occupied(&board, game->move[i]) && board_get_move_flip(&board, game->move[i], &stack[n_moves])) {
			board_update(&board, stack + n_moves);
			++n_moves;
		} else {
			warn("illegal move in game");
			break; // stop, illegal moves
		}
	}

	while (--n_moves >= 0) {
		board_restore(&board, stack + n_moves);
		if (movehash_append(hash, &board, stack[n_moves].x)) {
			if (book_get_moves(book, &board, &movelist)) {
				bestscore = movelist_first(&movelist)->score;
				foreach_move(iter, movelist) {
					if (iter->x == stack[n_moves].x) {
						if (iter->score < bestscore) ++stat->bad;
						else ++stat->good;
						break;
					}
				}
			} else ++stat->missing;
		}
	}
}

/**
 * @brief Check positions from a game database.
 *
 * @param book opening book.
 * @param base games to add.
 */
void book_check_base(Book *book, const Base *base)
{
	int i;
	BookCheckGame stat = {0, 0, 0};
	MoveHash hash;

	bprint("Checking %d games to book...\n", base->n_games);
	movehash_init(&hash, options.hash_table_size);
	for (i = 0; i < base->n_games; ++i) {
		book_check_game(book, &hash, base->game + i, &stat);
	}
	movehash_delete(&hash);
    bprint("Positions : %llu missing, %llu good, %llu bad (%.2f%% bad)\n", stat.missing, stat.good, stat.bad, (100.0 * stat.bad)/(stat.bad + stat.good));
}


/**
 * @brief Extract book lines to a game base 
 *
 * Recursively add move to a PV until no move are available, 
 * where we dump the PV to a game data base.
 *
 * @param book Opening book.
 * @param board Starting position.
 * @param pv Previous moves leading to the starting  position.
 * @param base game database.
 */
static void extract_skeleton(Book *book, Board *board, Line *pv, Base *base)
{
	MoveList movelist;
	Move *move;
	Board init;
	Game game;
	int bestscore;

	if (book_get_moves(book, board, &movelist)) {
		bestscore = movelist_best(&movelist)->score;
		
		foreach_move(move, movelist) {
			if (move->score == bestscore) {
				board_update(board, move); line_push(pv, move->x);
					extract_skeleton(book, board, pv, base);
				board_restore(board, move); line_pop(pv);
			}
		}
	} else if (pv->n_moves) {
		board_init(&init);
		line_to_game(&init, pv, &game);
		base_append(base, &game);
		if (base->n_games % 1000 == 0) {
			bprint("extracting %d games\r", base->n_games);
		}
	}
}

/**
 * @brief Extract book draws to a game base 
 *
 * This function supposes that f5d6c4 & f5f6e6f4 are draws and the only draws, excluding the transpositions
 * f5d6c3d3c4 & f5f6e6c6 & c4..., d3..., e6...
 *
 * @param book Opening book.
 * @param base game database.
 */
void book_extract_skeleton(Book *book, Base *base)
{
	Line pv;
	Board board;

	line_init(&pv, BLACK);
	line_push(&pv, F5); line_push(&pv, D6); line_push(&pv, C4);
	board_init(&board);
	board_next(&board, F5, &board); board_next(&board, D6, &board); board_next(&board, C4, &board);
	extract_skeleton(book, &board, &pv, base);
	
	line_init(&pv, BLACK);
	line_push(&pv, F5); line_push(&pv, F6); line_push(&pv, E6); line_push(&pv, F4);
	board_init(&board);
	board_next(&board, F5, &board); board_next(&board, F6, &board);
	board_next(&board, E6, &board); board_next(&board, F4, &board);	
	extract_skeleton(book, &board, &pv, base);
	bprint("%d games extracted   \n", base->n_games);
}


/**
 * @brief print a set of position.
 *
 * @param book Opening book.
 * @param n_empties Game stage.
 * @param n_positions Number of positions to extract.
 */
void book_extract_positions(Book *book, const int n_empties, const int n_positions)
{
	PositionArray *a;
	Position *p;
	MoveList movelist;
	Move *best, *second_best;
	int i = 0;
	char s[80];

	bprint("Extracting %d positions at %d ...\n", n_positions, n_empties); 
	foreach_position(p, a, book) {
		if (i == n_positions) break;
		if (board_count_empties(&p->board) == n_empties) {
			position_get_moves(p, &p->board, &movelist);
			best = movelist_first(&movelist);
			if (best) {
				second_best = move_next(best);
				if (second_best && best->score > second_best->score) {
					++i;
					board_to_string(&p->board, n_empties & 1, s);
					bprint("%s %% bm ", s);
					move_print(best->x, n_empties & 1, stdout);
					bprint(":%+2d; ba ", best->score);
					move_print(second_best->x, n_empties & 1, stdout);
					bprint(":%+2d;\n", second_best->score);
				}
			}
		}
	}
}

/**
 * @brief print book statistics.
 *
 * @param book Opening book.
 */
void book_stats(Book *book)
{
	PositionArray *a;
	Position *p;
	int i;
	unsigned long long n_hash[256];
	unsigned long long n_pos[61], n_leaf[61], n_link[61], n_terminal[61];
	unsigned long long n_score[129];

	printf("\n\nBook statistics:\n");

	printf("\nHash distribution:\n");
	for (i = 0; i < 256; ++i) n_hash[i] = 0;
	for (a = book->array; a < book->array + book->n; ++a) {
		if (a->n < 256) ++n_hash[a->n];
		else ++n_hash[255];
	}
	printf("index    positions\n");
	for (i = 0; i < 255; ++i) if (n_hash[i]) printf("%5d %12llu\n", i, n_hash[i]);
	if (n_hash[i]) printf(">%4d %12llu\n", i - 1, n_hash[i]);

	printf("\nStage distribution:\n");
	printf("stage    positions        links       leaves      terminal nodes\n");
	for (i = 0; i < 61; ++i) n_pos[i] = n_leaf[i] = n_link[i] = n_terminal[i] = 0;
	foreach_position(p, a, book) {
		i = board_count_empties(&p->board);
		++n_pos[i];
		if (p->leaf.move != NOMOVE) ++n_leaf[i];
		if (p->n_link == 0) ++n_terminal[i];
		n_link[i] += p->n_link;
	}
	for (i = 0; i < 61; ++i) if (n_pos[i]) printf("%5d %12llu %12llu %12llu %12llu\n", i, n_pos[i], n_link[i], n_leaf[i], n_terminal[i]);
		
	printf("\nBest Score Distribution:\n");
	printf("Score    positions\n");
	for (i = 0; i < 129; ++i) n_score[i] = 0;
	foreach_position(p, a, book) {
		++n_score[64 + p->score.value];
	}
	for (i = 0; i < 129; ++i) if (n_score[i]) printf("%+5d %12llu\n", i - 64, n_score[i]);
	printf("\n\n");
	fflush(stdout);
}

/**
 * @brief feed hash table from the opening book.
 * 
 * @param book Opening book.
 * @param board Position to start from.
 * @param search HashTables container.
 */
void book_feed_hash(const Book *book, Board *board, Search *search)
{
	board_feed_hash(board, book, search, true);
}
