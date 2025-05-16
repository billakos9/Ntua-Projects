# prolog.assertz('(directed_by(X,Y) :- findall(M,director(M,X),Y))')

path = '/content/drive/MyDrive/ECE/AI-Lab/'


# Safer assertion with error handling
def safe_assertz(prolog, fact, file=None):
    """Assert a fact or rule to Prolog and optionally save it to a file"""
    try:
        prolog.query(f"assertz({fact})")
        if file:
            file.write(f"{fact}\n")
        return True
    except Exception as e:
        print(f"Failed to assert: {fact[:50]}... - Error: {e}")
        return False

# Prolog rules for movie similarity
def add_similarity_rules(prolog, file=None):
    """Add rules for finding similar movies based on different criteria"""

    safe_assertz(prolog,
          "safe_div(_N, 0, _R) :- !, fail.", file)
    safe_assertz(prolog,
          "safe_div(N, D, R) :- R is N / D.", file)


    # 1. Common genre (exact match)
    safe_assertz(prolog, "common_genre(Movie1, Movie2) :- dif(Movie1, Movie2), genre(Movie1, G), genre(Movie2, G).",file)

    # 2. Multiple common genres (stronger similarity)
    safe_assertz(prolog,
        "common_genres(Movie1, Movie2, Count) :- "
        "dif(Movie1, Movie2), "
        "findall(G, (genre(Movie1, G), genre(Movie2, G)), Genres), "
        "length(Genres, Count).", file)

    # Define levels of genre similarity
    safe_assertz(prolog, "very_similar_genre(Movie1, Movie2) :- common_genres(Movie1, Movie2, Count), Count >= 3.", file)
    safe_assertz(prolog, "somewhat_similar_genre(Movie1, Movie2) :- common_genres(Movie1, Movie2, Count), Count >= 1, Count < 3.", file)

    # Genre similarity score (1-5)
    safe_assertz(prolog,
        "genre_similarity_score(Movie1, Movie2, Score) :-"+
        "dif(Movie1, Movie2),"+
        "findall(G, (genre(Movie1, G), genre(Movie2, G)), CommonGenres),"+
        "length(CommonGenres, CommonCount),"+
        "(CommonCount >= 4 -> Score = 5;"+
         "CommonCount = 3 -> Score = 4;"+
         "CommonCount = 2 -> Score = 3;"+
         "CommonCount = 1 -> Score = 2;"+
         "Score = 1).", file)

    # 4. Same director
    safe_assertz(prolog, "same_director(Movie1, Movie2) :- dif(Movie1, Movie2), director(Movie1, D), director(Movie2, D).", file)

    # 5. Plot keyword similarity
    safe_assertz(prolog,
        "common_keywords(Movie1, Movie2, Count) :- "+
        "dif(Movie1, Movie2), "+
        "findall(K, (keyword(Movie1, K), keyword(Movie2, K)), Keywords), "+
        "length(Keywords, Count).", file)

    # Define levels of plot similarity based on keywords
    safe_assertz(prolog, "very_similar_plot(Movie1, Movie2) :- common_keywords(Movie1, Movie2, Count), Count >= 3.", file)
    safe_assertz(prolog, "somewhat_similar_plot(Movie1, Movie2) :- common_keywords(Movie1, Movie2, Count), Count >= 1, Count < 3.", file)

    # Plot similarity score (1-5)
    safe_assertz(prolog,
        "plot_similarity_score(Movie1, Movie2, Score) :- " +
        "dif(Movie1, Movie2), " +
        "findall(K, (keyword(Movie1, K), keyword(Movie2, K)), CommonKeywords), " +
        "length(CommonKeywords, CommonCount), " +
        "(CommonCount >= 4 -> Score = 5; " +
        "CommonCount = 3 -> Score = 4; " +
        "CommonCount = 2 -> Score = 3; " +
        "CommonCount = 1 -> Score = 2; " +
        "Score = 1).", file)


    # 7. Common actors
    safe_assertz(prolog,
        "common_actors(Movie1, Movie2, Count) :- " +
        "dif(Movie1, Movie2), " +
        "findall(A, (actor(Movie1, A), actor(Movie2, A)), Actors), " +
        "length(Actors, Count).", file)

    # Define levels of cast similarity
    safe_assertz(prolog, "all_same_actors(Movie1, Movie2) :- common_actors(Movie1, Movie2, 3).", file)
    safe_assertz(prolog, "most_same_actors(Movie1, Movie2) :- common_actors(Movie1, Movie2, 2).", file)
    safe_assertz(prolog, "some_same_actors(Movie1, Movie2) :- common_actors(Movie1, Movie2, 1).", file)

    # Actor similarity score (1-5)
    safe_assertz(prolog,
        "actor_similarity_score(Movie1, Movie2, Score) :- " +
        "dif(Movie1, Movie2), " +
        "findall(A, (actor(Movie1, A), actor(Movie2, A)), CommonActors), " +
        "length(CommonActors, CommonCount), " +
        "(CommonCount >= 3 -> Score = 5; " +
        "CommonCount = 2 -> Score = 4; " +
        "CommonCount = 1 -> Score = 3; " +
        "Score = 1).", file)

    # 10. Same language
    safe_assertz(prolog, "same_language(Movie1, Movie2) :- dif(Movie1, Movie2), language(Movie1, L), language(Movie2, L).", file)

    # 11. Color/Black&White
    safe_assertz(prolog, "same_color_type(Movie1, Movie2) :- dif(Movie1, Movie2), is_color(Movie1, C), is_color(Movie2, C).", file)

    # 12. Same production company
    safe_assertz(prolog, "same_production_company(Movie1, Movie2) :- dif(Movie1, Movie2), production_company(Movie1, C), production_company(Movie2, C).", file)

    # 13. Same production country
    safe_assertz(prolog, "same_country(Movie1, Movie2) :- dif(Movie1, Movie2), production_country(Movie1, C), production_country(Movie2, C).", file)

    # 14. Same decade
    safe_assertz(prolog, "same_decade(Movie1, Movie2) :- dif(Movie1, Movie2), decade(Movie1, D), decade(Movie2, D).", file)

    # Extra rules for similarity
    # Composite similarity rules for recommendation (with different levels)
    safe_assertz(prolog,
        "highly_similar(Movie1, Movie2) :- " +
        "dif(Movie1, Movie2), " +
        "(very_similar_genre(Movie1, Movie2); same_director(Movie1, Movie2)), " +
        "(very_similar_plot(Movie1, Movie2); some_same_actors(Movie1, Movie2)), " +
        "same_language(Movie1, Movie2).", file)

    safe_assertz(prolog,
        "moderately_similar(Movie1, Movie2) :- " +
        "dif(Movie1, Movie2), " +
        "(somewhat_similar_genre(Movie1, Movie2); same_decade(Movie1, Movie2)), " +
        "(somewhat_similar_plot(Movie1, Movie2); same_production_company(Movie1, Movie2))."
    , file)

    safe_assertz(prolog,
        "somewhat_similar(Movie1, Movie2) :- " +
        "dif(Movie1, Movie2), " +
        "(common_genre(Movie1, Movie2); same_country(Movie1, Movie2); same_decade(Movie1, Movie2))."
    , file)


    # *. Similar budget
    safe_assertz(prolog,
        "similar_budget_scale(Movie1, Movie2) :- " +
        "budget(Movie1, B1), budget(Movie2, B2), " +
        "B1 > 0, B2 > 0, " +
        "Ratio is max(B1/B2, B2/B1), Ratio < 3.", file)

    # Budget similarity score (1-5)
    safe_assertz(prolog,
        "budget_similarity_score(Movie1, Movie2, Score) :- " +
        "dif(Movie1, Movie2), " +
        "budget(Movie1, B1), " +
        "budget(Movie2, B2), " +
        "B1 > 0, B2 > 0, " +
        "Ratio is max(B1/B2, B2/B1), " +
        "(Ratio < 1.25 -> Score = 5; " +
        "Ratio < 1.5 -> Score = 4; " +
        "Ratio < 2.0 -> Score = 3; " +
        "Ratio < 3.0 -> Score = 2; " +
        "Score = 1).", file)

    # *. Similar vote average
    safe_assertz(prolog,
        "similarly_popular(Movie1, Movie2) :- " +
        "vote_count(Movie1, C1), vote_count(Movie2, C2), " +
        "vote_average(Movie1, A1), vote_average(Movie2, A2), " +
        "abs(A1 - A2) < 1.0, " +
        "C1 > 100, C2 > 100.", file)

    # Sequel or prequel
    # Helper to check if a string contains a digit
    safe_assertz(prolog,
        "contains_digit(Movie) :- " +
        "atom_chars(Movie, Chars), " +
        "member(D, Chars), " +
        "char_type(D, digit).", file)

    # Helper to detect sequel patterns (common base name with different numbers)
    safe_assertz(prolog,
        "sequel_pattern(Movie1, Movie2) :- " +
        "dif(Movie1, Movie2), " +
        "atom_chars(Movie1, Chars1), " +
        "atom_chars(Movie2, Chars2), " +
        "append(CommonPrefix, [Digit1|Rest1], Chars1), " +
        "append(CommonPrefix, [Digit2|Rest2], Chars2), " +
        "char_type(Digit1, digit), " +
        "char_type(Digit2, digit), " +
        "dif(Digit1, Digit2), " +
        "length(CommonPrefix, PrefixLen), " +
        "PrefixLen > 3.", file)  # Common prefix should be substantial

    # Direct sequel pattern (base_name and base_name_2)
    safe_assertz(prolog,
        "direct_sequel(Movie1, Movie2) :- " +
        "dif(Movie1, Movie2), " +
        "atom_concat(Base, '', Movie1), " +  # Get base title
        "atom_concat(Base, '_', WithUnderscore), " +  # Add underscore
        "atom_concat(WithUnderscore, Number, Movie2), " +  # Check format
        "atom_codes(Number, Codes), " +  # Convert to codes
        "maplist(char_type, Codes, Types), " +  # Get character types
        "member(digit, Types).", file)  # At least one digit

    # Combined sequel detection rule
    safe_assertz(prolog,
        "sequel_or_prequel(Movie1, Movie2) :- " +
        "sequel_pattern(Movie1, Movie2).", file)

    # Alternative sequel detection
    safe_assertz(prolog,
        "sequel_or_prequel(Movie1, Movie2) :- " +
        "direct_sequel(Movie1, Movie2).", file)

    # Overall similarity score (1-5) - weighted combination
    safe_assertz(prolog,
        "overall_similarity_score(Movie1, Movie2, Score) :- " +
        "dif(Movie1, Movie2), " +
        "genre_similarity_score(Movie1, Movie2, GenreScore), " +
        "plot_similarity_score(Movie1, Movie2, PlotScore), " +
        "actor_similarity_score(Movie1, Movie2, ActorScore), " +
        "movie_id(Movie2,_), " +
        "WeightedScore is (GenreScore * 3 + PlotScore * 2 + ActorScore * 2) / 6, " +
        "Score is max(1, min(5, round(WeightedScore))).", file)

    # Enhanced recommendation rules using scores
    safe_assertz(prolog,
        "recommend_score_5(Movie1, Movie2) :- " +
        "overall_similarity_score(Movie1, Movie2, 5).", file)

    safe_assertz(prolog,
        "recommend_score_4(Movie1, Movie2) :-"+
        "overall_similarity_score(Movie1, Movie2, 4).", file)

    safe_assertz(prolog,
        "recommend_score_3(Movie1, Movie2) :-"+
        "overall_similarity_score(Movie1, Movie2, 3).", file)

    safe_assertz(prolog,
        "recommend_score_2(Movie1, Movie2) :-"+
        "overall_similarity_score(Movie1, Movie2, 2).", file)

    safe_assertz(prolog,
        "recommend_score_1(Movie1, Movie2) :-"+
        "overall_similarity_score(Movie1, Movie2, 1).", file)


    print("Added similarity rules to the knowledge base")
    return prolog

# Main function to create the full knowledge base with rules
def create_full_movie_kb(file_path, max_movies=5000, output_file=None):
    """Create a complete movie knowledge base with facts and similarity rules"""
    # Preprocess the data
    data = preprocess_data(file_path)

    # Build the knowledge base
    prolog = build_knowledge_base(data, max_movies)

    # Save to file if requested
    if output_file:
        print(f"Saving complete knowledge base to {output_file}...")
        with open(output_file, 'w') as f:
            # Write facts first
            predicates = ["movie_id", "genre", "director", "actor", "actor_position",
                          "keyword", "language", "production_company", "production_country",
                          "release_year", "decade", "runtime", "budget", "revenue",
                          "vote_average", "vote_count"]

            for predicate in predicates:
                if list(prolog.query(f"current_predicate({predicate}/2)")):
                    f.write(f"% {predicate.upper()} FACTS\n")
                    for solution in prolog.query(f"{predicate}(X,Y)"):
                        f.write(f"{predicate}('{solution['X']}','{solution['Y']}').\n")
                    f.write("\n")
                else:
                    f.write(f"% No {predicate.upper()} facts found\n\n")

            # Then write rules
            f.write("\n% SIMILARITY RULES\n")
            print("Adding similarity rules...")
            add_similarity_rules(prolog, f)
            f.write("% End of similarity rules\n")

        print(f"Complete knowledge base saved to {output_file}")
    else:
        print("WARNING: No output file specified - rules will not be permanently saved.")

    return prolog

# Example usage:
prolog = create_full_movie_kb(path + "movies_metadata.csv", max_movies=5000, output_file=path + "movie_kb.pl")
del prolog  # Explicitly delete to free resources