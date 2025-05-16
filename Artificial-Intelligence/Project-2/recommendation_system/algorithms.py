from pyswip import Prolog

# Functions for the recommendation system
Path = '/content/drive/MyDrive/ECE/AI-Lab/'
DEFAULT_MAX_RESULTS = 10

# Function to clean text for Prolog compatibility
def clean_text_for_prolog(text):
    """Clean text to make it compatible with Prolog assertions"""
    # Check for string not UNK
    if isinstance(text, str) and text != "UNK":
        # Remove special characters that could cause issues in Prolog
        text = re.sub(r'[^a-zA-Z0-9\s]', '', text)
        # Replace spaces with underscores for Prolog atoms
        text = text.replace(' ', '_')
        # Convert to lowercase for consistency
        text = text.lower()
        return text
    return 'unknown'

def assert_predicate_exists(prolog, predicate: str, arity: int = 3) -> None:
    """Raise RuntimeError if the given Prolog predicate/arity isn’t loaded."""
    check = list(prolog.query(f"current_predicate({predicate}/{arity})"))
    if not check:
        raise RuntimeError(f"Prolog predicate {predicate}/{arity} not found")


def available_similarity_levels(prolog) -> list[int]:
    """
    Probe Prolog for which recommend_score_N/2 predicates are loaded.
    Returns a list of integers N for which recommend_score_N/2 exists.
    """
    levels = []
    for lvl in range(1, 6):
        pred = f"recommend_score_{lvl}"
        # current_predicate/1 wants the functor/arity:
        if list(prolog.query(f"current_predicate({pred}/2)")):
            levels.append(lvl)
    return levels

# Using similarity overall score: includes every prolog rule

def get_recommendations_algorithm(prolog, movie_title:str, similarity_level: int, max_results=DEFAULT_MAX_RESULTS) -> list[str]:
    """Get movie recommendations at a specific similarity level"""
    # 1) validate inputs
    if not (1 <= similarity_level <= 5):
        raise ValueError(f"similarity_level must be 1–5 (got {similarity_level})")
    if max_results < 1:
        raise ValueError("max_results must be ≥ 1")

    clean_title = clean_text_for_prolog(movie_title)
    predicate = f"recommend_score_{similarity_level}"
    # 2) sanity‐check that the wrapper predicate is actually in the KB
    try:
      assert_predicate_exists(prolog, predicate, arity=2)
    except RuntimeError:
      available = available_similarity_levels(prolog)
      print(
            f"⚠️  No recommend_score_{similarity_level}/2 predicate loaded.\n"
            f"    Available similarity levels: {available}"
        )
      return []


    query = f"{predicate}({escape_atom(clean_title)}, Movie)"
    # Create result set
    results = []


    try:
        for solution in prolog.query(query):
            results.append(solution["Movie"])
            if len(results) >= max_results:
                break
    except Exception as e:
        print(f"Error querying Prolog: {e}")
        return []

    return results


# Metric + Threshold
def query_recommendations(
    prolog: Prolog,
    movie: str,
    metric: str,
    min_score: int,
    max_results: int,
) -> list[tuple[str,int]]:

    # 1) validate inputs
    if metric not in METRICS:
        raise KeyError(f"Unknown metric '{metric}'. Valid choices are: {list(METRICS)}")
    if not (1 <= min_score <= 5):
        raise ValueError(f"min_score must be 1–5 (got {min_score})")
    if max_results < 1:
        raise ValueError("max_results must be ≥ 1")

    movie_atom = escape_atom(movie)
    pred = METRICS[metric]

    # 2) sanity‐check predicate (arity 3: movie, other_movie, score)
    assert_predicate_exists(prolog, pred, arity=3)
    q = f"{pred}({movie_atom}, M2, Score), Score >= {min_score}"
    results: List[tuple[str, int]] = []

    try:
        for sol in prolog.query(q):
            results.append((sol["M2"], sol["Score"]))
            if len(results) >= max_results:
                break
    except Exception as e:
        print(f"⚠️  Prolog error: {e}")
    return results
