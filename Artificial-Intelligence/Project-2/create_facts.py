# Combine everything into a single function to create the knowledge base
def create_movie_kb(file_path, max_movies=5000, output_file=None):
    """Create the full movie knowledge base and optionally save to file"""
    # Preprocess the data
    data = preprocess_data(file_path)

    # Build the knowledge base
    prolog = build_knowledge_base(data, max_movies)

    # Save to file if requested
    if output_file:
        with open(output_file, 'w') as f:
            # List of all predicates we've created
            predicates = ["movie_id", "genre", "director", "actor", "actor_position",
                         "keyword", "language", "production_company", "production_country",
                         "release_year", "decade", "runtime", "budget", "revenue",
                         "vote_average", "vote_count"]

            for predicate in predicates:
              # Check if the predicate exists before querying
              if list(prolog.query(f"current_predicate({predicate}/2)")):
                f.write(f"% {predicate.upper()} FACTS\n")
                # Query for all facts of this predicate
                for solution in prolog.query(f"{predicate}(X,Y)"):
                    f.write(f"{predicate}('{solution['X']}','{solution['Y']}').\n")
                f.write("\n")
              else:
                f.write(f"% No {predicate.upper()} facts found\n\n")

        print(f"Knowledge base saved to {output_file}")

    return prolog

# Run & Create
prolog = create_movie_kb(path +"movies_metadata.csv", output_file= path +"movies.pl")
del prolog  # Explicitly delete to free resources