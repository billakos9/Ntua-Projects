# Simple terminal UI for the movie recommender.

from pathlib import Path
from typing import List
from pyswip import Prolog


# friendly-name → Prolog score predicate
METRICS = {
    "overall" : "overall_similarity_score",
    "genre"   : "genre_similarity_score",
    "plot"    : "plot_similarity_score",
    "actor"   : "actor_similarity_score",
    "decade"  : "decade_similarity_score",
    "budget"  : "budget_similarity_score",
}

# utilities

def escape_atom(text: str) -> str:
    """Return text as a single-quoted Prolog atom with proper escaping."""
    return "'" + text.replace("\\", "\\\\").replace("'", "\\'") + "'"


# CLI FUNC
def prompt_int(prompt: str, lo: int, hi: int) -> int:
    while True:
        try:
            value = int(input(prompt).strip())
            if lo <= value <= hi:
                return value
        except ValueError:
            pass
        print(f"Please enter a number from {lo} to {hi}.")


def print_results(movies, headers, rows):
    sep = " | "
    print(sep.join(headers))
    print("-" * (sum(len(h) for h in headers) + 3 * (len(headers)-1)))
    for i, row in enumerate(rows, start=1):
        print(sep.join([str(i), *map(str, row)]))


# MAIN  ────────────────────────────────────────────────────────────────────
def main(prolog) -> None:
    print("🎬  Movie Recommender")

    while True:
        print("\n────────────────────────────────────────────────────────────")
        title = input("Movie title (or just Enter to quit): ").strip()
        if not title or title.lower() == "quit":
            break

        # ------------------------------------------------------------------
        # 1) Select strategy
        # ------------------------------------------------------------------
        print("\nSelect recommendation algorithm:")
        print("  1. Exact similarity level (recommend_score_N/2)")
        print("  2. Metric + minimum score filter")
        algo = prompt_int("Choice: ", 1, 2)

        # ------------------------------------------------------------------
        # 2a) Algorithm 1  – wrapper predicates (your original function)
        # ------------------------------------------------------------------
        if algo == 1:
            sim_level = prompt_int("Similarity level (1-5): ", 1, 5)
            max_res   = prompt_int("How many results (1-10)? ", 1, 10)

            movies = get_recommendations_algorithm(
                prolog,
                movie_title=title,
                similarity_level=sim_level,
                max_results=max_res,
            )

            if not movies:
                print("No matches found.")
                continue

            print("\nResults")
            print("───────")
            print(" # | Movie | Level")
            print("---|-------|------")
            for i, mov in enumerate(movies, 1):
                print(f"{i:2} | {mov} | {sim_level}")

        # ------------------------------------------------------------------
        # 2b) Algorithm 2  – metric + threshold (existing code)
        # ------------------------------------------------------------------
        else:
            print("\nSelect similarity metric:")
            for i, name in enumerate(METRICS, 1):
                print(f"  {i}. {name}")
            idx     = prompt_int("Choice: ", 1, len(METRICS))
            metric  = list(METRICS)[idx - 1]

            score   = prompt_int("Minimum similarity score (1-5): ", 1, 5)
            max_res = prompt_int("How many results (1-10)? ", 1, 10)

            results = query_recommendations(
                prolog, title, metric, score, max_res
            )

            if not results:
                print("No matches found.")
                continue

            print("\nResults")
            print("───────")

    print("\nBye! 👋")


if __name__ == "__main__":
    try:
        # Initialize Prolog engine
        prolog = Prolog()
        # Load your knowledge base file
        kb_file = "/content/drive/MyDrive/ECE/AI-Lab/movie_kb.pl"
        print(f"Loading knowledge base from {kb_file}...")

        try:
            # This consults (loads) the Prolog file
            prolog.consult(kb_file)
            print(f"✅ Successfully loaded knowledge base")

            # Check if basic predicates exist
            movie_count = len(list(prolog.query("movie_id(_, _)")))
            print(f"Found {movie_count} movies in knowledge base")

            print(list(prolog.query("genre('avatar', G).", maxresult=5)))
            # Should print something like [{'G': 'sci_fi'}, {'G': 'action'}]
            print(list(prolog.query("recommend_score_4('avatar', Movie).", maxresult=10)))



            # Start the main program
            main(prolog)

        except Exception as e:
            print(f"❌ Error loading knowledge base: {e}")
            print("Make sure you've created the knowledge base file first.")

    except KeyboardInterrupt:
        print("\nInterrupted — bye! 👋")