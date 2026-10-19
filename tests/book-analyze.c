/*
 * Tests of the book analyses for a user interface (src/book_analyze.c).
 *
 * umigame: random graphs of positions (layers, transpositions, several links to the same position, links that
 * skip layers as passes do): the umigame numbers with the shared positions counted once are compared with a
 * search of every choice of moves, and with the numbers that count each line when no position is shared.
 *
 * Build: tests\build-book-analyze.cmd (Visual Studio 2022 x64 Developer Command Prompt), run: tests\book-analyze.exe umigame
 */
#define BOOK_ANALYZE_TEST
#define main edax_original_main
#include "../src/all.c"
#undef main

static unsigned long long rng_state = 0x9e3779b97f4a7c15ULL;

static unsigned int rnd(const unsigned int n)
{
	rng_state ^= rng_state << 13; rng_state ^= rng_state >> 7; rng_state ^= rng_state << 17;
	return (unsigned int) (rng_state % n);
}

typedef struct TestGraph {
	int n;
	int layer[4096];
	int color[4096];
	int n_child[4096];
	int child[4096][8];
} TestGraph;

/** @brief A random graph: node 0 is the root, the links go to a later layer of the other side to move. */
static void test_graph(TestGraph *g, const int n_layer, const int width, const int max_child)
{
	int first[64], count[64], l, i, k;

	g->n = 0;
	for (l = 0; l < n_layer; ++l) {
		count[l] = l == 0 ? 1 : 1 + (int) rnd((unsigned) width);
		first[l] = g->n;
		for (i = 0; i < count[l]; ++i) {
			g->layer[g->n] = l;
			g->color[g->n] = l & 1;
			g->n_child[g->n] = 0;
			++g->n;
		}
	}
	for (i = 0; i < g->n; ++i) {
		const int li = g->layer[i];
		const int nc = (li == n_layer - 1) ? 0 : (int) rnd((unsigned) max_child + 1);
		for (k = 0; k < nc; ++k) {
			int lj = li + 1;
			if (li + 3 < n_layer && rnd(6) == 0) lj = li + 3; // (a pass and its answer: the same side to move as one layer later)
			g->child[i][g->n_child[i]++] = first[lj] + (int) rnd((unsigned) count[lj]);
		}
		if (i == 0 && g->n_child[0] == 0 && n_layer > 1) g->child[0][g->n_child[0]++] = first[1];
	}
}

/** @brief Put a graph in the walk state of the analyses (as ba_umi_build does). */
static void test_load(BaWork *w, const TestGraph *g)
{
	int i, k, r = 0, l;
	unsigned int e = 0;

	ba_umi_free(w);
	w->n_node = w->size_node = (unsigned int) g->n;
	w->node = (BaUNode*) calloc((size_t) g->n, sizeof *w->node);
	w->edge = (unsigned int*) malloc(4096 * 8 * sizeof *w->edge);
	w->order = (unsigned int*) malloc((size_t) g->n * sizeof *w->order);
	w->rank = (unsigned int*) malloc((size_t) g->n * sizeof *w->rank);
	for (i = 0; i < g->n; ++i) {
		w->node[i].color = (unsigned char) g->color[i];
		w->node[i].alias = (unsigned int) i;
		w->node[i].edge = e;
		for (k = 0; k < g->n_child[i]; ++k) {
			w->edge[e++] = (unsigned int) g->child[i][k];
			++w->node[g->child[i][k]].indeg;
		}
		w->node[i].n_edge = (unsigned int) g->n_child[i];
		w->node[i].end = g->n_child[i] == 0;
	}
	w->n_edge = w->size_edge = e;
	for (l = 63; l >= 0; --l) for (i = 0; i < g->n; ++i) if (g->layer[i] == l) { w->order[r] = (unsigned int) i; w->rank[i] = (unsigned int) r; ++r; }
}

/* every choice of one move per position of the counting side */
static int bf_best, bf_calls;

static void bf_search(const TestGraph *g, const int counter, char *active, const int i, const int ends)
{
	int k;
	if (++bf_calls > 20000000) return;
	if (ends >= bf_best) return;
	if (i == g->n) { bf_best = ends; return; }
	if (!active[i]) { bf_search(g, counter, active, i + 1, ends); return; }
	if (g->n_child[i] == 0) { bf_search(g, counter, active, i + 1, ends + 1); return; }
	if (g->color[i] == counter) {
		for (k = 0; k < g->n_child[i]; ++k) {
			const int c = g->child[i][k], was = active[c];
			int j, seen = 0;
			for (j = 0; j < k; ++j) if (g->child[i][j] == c) seen = 1;
			if (seen) continue;
			active[c] = 1;
			bf_search(g, counter, active, i + 1, ends);
			active[c] = (char) was;
		}
	} else {
		char saved[8];
		for (k = 0; k < g->n_child[i]; ++k) saved[k] = active[g->child[i][k]];
		for (k = 0; k < g->n_child[i]; ++k) active[g->child[i][k]] = 1;
		bf_search(g, counter, active, i + 1, ends);
		for (k = g->n_child[i] - 1; k >= 0; --k) active[g->child[i][k]] = saved[k];
	}
}

static int brute_force(const TestGraph *g, const int counter, bool *complete)
{
	static char active[4096];
	memset(active, 0, (size_t) g->n);
	active[0] = 1;
	bf_best = INT_MAX; bf_calls = 0;
	bf_search(g, counter, active, 0, 0); // (nodes are numbered layer by layer: a node comes before its next positions)
	*complete = bf_calls <= 20000000;
	return bf_best;
}

static int umigame(void)
{
	static TestGraph g;
	BaJob job;
	BaParams prm;
	BaWork w;
	int t, c, n_bad = 0, n_shared = 0, n_checked = 0, n_skipped = 0, n_inexact = 0;
	long long t0 = real_clock();

	memset(&job, 0, sizeof job);
	ba_params_default(&prm);
	memset(&w, 0, sizeof w);
	w.job = &job;
	w.prm = &prm;
	for (t = 0; t < 20000; ++t) {
		const int n_layer = 2 + (int) rnd(9), width = 1 + (int) rnd(6), max_child = 1 + (int) rnd(4);
		test_graph(&g, n_layer, width, max_child);
		test_load(&w, &g);
		for (c = 0; c < 2; ++c) {
			bool complete;
			const long long got = ba_umi_count(&w, c);
			const bool exact = !w.inexact;
			const unsigned long long tree = ba_umi_tree(&w, c);
			const int want = brute_force(&g, c, &complete);
			bool shared = false;
			unsigned int i;
			ba_umi_clear(&w);
			for (i = 0; i < w.n_node; ++i) if (w.node[i].indeg >= 2) shared = true;
			if (!complete) { ++n_skipped; continue; }
			++n_checked;
			if (shared) ++n_shared;
			if (!exact) ++n_inexact;
			if (got != want || (!shared && (unsigned long long) got != tree)) {
				if (++n_bad <= 10) printf("FAIL umigame graph %d side %d: %lld, search %d, lines %llu (%d positions)\n", t, c, got, want, tree, g.n);
			}
		}
	}
	ba_umi_free(&w);
	printf("umigame: %d numbers checked (%d with shared positions, %d inexact, %d too long to search), %d wrong, %.1f s\n",
		n_checked, n_shared, n_inexact, n_skipped, n_bad, (real_clock() - t0) / 1000.0);
	return n_bad ? 2 : 0;
}

/** @brief Larger graphs: time and number of terms (no search of every choice). */
static int umigame_large(void)
{
	static TestGraph g;
	BaJob job;
	BaParams prm;
	BaWork w;
	int t, n_inexact = 0;
	long long t0 = real_clock();

	memset(&job, 0, sizeof job);
	ba_params_default(&prm);
	memset(&w, 0, sizeof w);
	w.job = &job;
	w.prm = &prm;
	for (t = 0; t < 200; ++t) {
		test_graph(&g, 40, 60, 3);
		test_load(&w, &g);
		ba_umi_count(&w, BLACK); if (w.inexact) ++n_inexact; ba_umi_clear(&w);
		ba_umi_count(&w, WHITE); if (w.inexact) ++n_inexact; ba_umi_clear(&w);
	}
	ba_umi_free(&w);
	printf("umigame large: 200 graphs of about %d positions, %d inexact numbers, %.1f s\n", g.n, n_inexact, (real_clock() - t0) / 1000.0);
	return 0;
}

int main(int argc, char **argv)
{
	setbuf(stdout, NULL);
	if (argc < 2) return 1;
	if (strcmp(argv[1], "umigame") == 0) return umigame();
	if (strcmp(argv[1], "umigame-large") == 0) return umigame_large();
	return 1;
}
