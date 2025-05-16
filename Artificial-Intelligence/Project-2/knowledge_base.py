import pandas as pd
import ast
import re
from pyswip import Prolog
import json
import numpy as np

path = '/content/drive/MyDrive/ECE/AI-Lab/'

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


def parse_genres(genres_field):
    """
    Return a list of genre strings from one CSV cell.
    Supports:
      * JSON / Python-list strings  "[{'name':'Action'}, ...]"
      * Pipe-separated text         "Action|Adventure"
      * Already-parsed list         ['Action', 'Adventure']
      * NaNs / UNK → []
    """
    if not isinstance(genres_field, str) or genres_field in ("UNK", ""):
        return []

    # 1) Try JSON / Python-list via ast
    try:
        parsed = ast.literal_eval(genres_field)
        if isinstance(parsed, list):
            # allow [{'id':..,'name':..}, 'Action', ...]
            cleaned = []
            for item in parsed:
                if isinstance(item, dict) and "name" in item:
                    cleaned.append(item["name"])
                elif isinstance(item, str):
                    cleaned.append(item)
            if cleaned:
                return cleaned
    except Exception:
        pass

    # 2) Pipe-separated fallback
    if "|" in genres_field:
        return [part.strip() for part in genres_field.split("|") if part.strip()]

    # 3) Comma-separated last resort
    if "," in genres_field:
        return [part.strip() for part in genres_field.split(",") if part.strip()]

    return []


# Read and preprocess the movie metadata
# Check and prepare
def preprocess_data(file_path):
    """Read and preprocess the movie metadata CSV file"""
    # Read the CSV file
    data = pd.read_csv(file_path)

    # Fill numeric columns with a numeric placeholder (like -1)
    for col in data.select_dtypes(include=['number']).columns:
      data[col] = data[col].fillna(-1)

    for col in data.select_dtypes(include=['object']).columns:
      data[col] = data[col].fillna("UNK")

    # Remove duplicate movies based on original_title
    data.drop_duplicates(subset=['original_title'], keep='first', inplace=True)

    # Remove rows with problematic data
    data = data[data['id'].notna() & (data['id'] != "UNK")]

    return data



def build_knowledge_base(data, max_movies=5000):

  #create World
  #Ορίζουμε τον κόσμο μας
  prolog = Prolog()

  #Για κάθε row του πίνακα φτιάχνουμε τα κατηγορήματα που θέλουμε να αποθηκέυσουμε
  #αρχικά σε μια λίστα με το όνομα literals
  literals = []
  movie_score = {}
  processed_count = 0

  # Process each movie row
  for index, row in data.iterrows():
        if processed_count >= max_movies:
            break

        try:
            # Skip movies with empty titles
            if row['movie_title'] == "UNK" or not row['movie_title']:
                continue

            movie_title = clean_text_for_prolog(row['movie_title'])

            # 1. Movie ID (unique identifier)
            movie_id = str(row['id']).replace('.0', '')
            literals.append(f"movie_id('{movie_title}','{movie_id}')")

            # 2. Genre information
            genres = parse_genres(row['genres'])
            for g in genres:
                  # Accept either a dict {'name': 'Action'} or the raw string 'Action'
                  if isinstance(g, dict):
                      name = g.get('name', '')
                  else:
                      name = g                                # plain string

                  genre_name = clean_text_for_prolog(name)
                  if genre_name != 'unknown':
                        literals.append(f"genre('{movie_title}','{genre_name}')")


            # 3. Director
            if 'director_name' in row and row['director_name'] != "UNK":
                  director = clean_text_for_prolog(row['director_name'])
                  if director != 'unknown':
                        literals.append(f"director('{movie_title}','{director}')")


            # 4. Cast / Actors  (uses actor_1_name, actor_2_name, actor_3_name)
            actor_cols = ['actor_1_name', 'actor_2_name', 'actor_3_name']

            actor_count = 0
            for idx, col in enumerate(actor_cols, start=1):        # idx = 1..3
                if col in row and row[col] != "UNK" and row[col]:  # non-empty
                    actor = clean_text_for_prolog(row[col])
                    if actor != 'unknown':
                          literals.append(f"actor('{movie_title}','{actor}')")
                          literals.append(f"actor_position('{movie_title}','{actor}',{idx})")
                          actor_count += 1

            # 5. Plot keywords  (column name = plot_keywords)
            if 'plot_keywords' in row and row['plot_keywords'] != "UNK":
                  kws = parse_genres(row['plot_keywords'])
                  for kw in kws:
                        name = kw['name'] if isinstance(kw, dict) else kw
                        keyword_name = clean_text_for_prolog(name)
                        if keyword_name != 'unknown':
                              literals.append(f"keyword('{movie_title}','{keyword_name}')")


            # 6. Language   (column name = language)
            if 'language' in row and row['language'] != "UNK":
                  lang = clean_text_for_prolog(row['language'])
                  literals.append(f"language('{movie_title}','{lang}')")



            # 8. Production companies
            if 'production_companies' in row and row['production_companies'] != "UNK":
                companies = parse_genres(row['production_companies'])
                for company in companies:
                    if isinstance(company, dict) and 'name' in company:
                        company_name = clean_text_for_prolog(company['name'])
                        if company_name != 'unknown':
                            literals.append(f"production_company('{movie_title}','{company_name}')")

            # 9. Production countries
            if 'production_countries' in row and row['production_countries'] != "UNK":
                countries = parse_genres(row['production_countries'])
                for country in countries:
                    if isinstance(country, dict) and 'name' in country:
                        country_name = clean_text_for_prolog(country['name'])
                        if country_name != 'unknown':
                            literals.append(f"production_country('{movie_title}','{country_name}')")

            # 10. Release decade
            if 'release_date' in row and row['release_date'] != "UNK":
                try:
                    year = int(row['release_date'].split('-')[0])
                    decade = (year // 10) * 10
                    literals.append(f"release_year('{movie_title}',{year})")
                    literals.append(f"decade('{movie_title}',{decade})")
                except:
                    pass  # Skip if date parsing fails

            # 11. Runtime  (column name = duration)
            if 'duration' in row and row['duration'] != "UNK":
                  try:
                        runtime = float(row['duration'])
                        if runtime > 0 and not np.isnan(runtime):
                              literals.append(f"runtime('{movie_title}',{int(runtime)})")
                  except Exception:
                        pass


            # 12. Budget and Gross revenue
            if 'budget' in row and row['budget'] != "UNK":
                try:
                      budget = float(row['budget'])
                      if budget > 0 and not np.isnan(budget):
                                literals.append(f"budget('{movie_title}',{int(budget)})")
                except Exception:
                      pass

            if 'gross' in row and row['gross'] != "UNK":
                try:
                      gross = float(row['gross'])
                      if gross > 0 and not np.isnan(gross):
                                literals.append(f"revenue('{movie_title}',{int(gross)})")
                except Exception:
                      pass


            # 13. Vote average and count (popularity metrics)
            if 'vote_average' in row and row['vote_average'] != "UNK":
                try:
                    vote_avg = float(row['vote_average'])
                    if not np.isnan(vote_avg):
                        literals.append(f"vote_average('{movie_title}',{vote_avg})")
                except:
                    pass

            if 'num_voted_users' in row and row['num_voted_users'] != "UNK":
                try:
                    vote_count = int(row['num_voted_users'])
                    literals.append(f"vote_count('{movie_title}',{vote_count})")
                except:
                    pass

            processed_count += 1

        except Exception as e:
            print(f"Error processing row {index}: {e}")
            continue

  #Η Prolog θέλει τα κατηγορήματά της με την σειρά
  literals.sort()
  for literal in literals:
      try:
            prolog.assertz(literal)
      except Exception as e:
            print(f"Error adding fact to Prolog: {literal} - {e}")

  print(f"Added {len(literals)} facts about {processed_count} movies to the knowledge base")
  return prolog