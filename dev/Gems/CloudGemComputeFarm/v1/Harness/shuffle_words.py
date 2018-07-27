import random
import zipfile

# Load in the dictionary of words
with zipfile.ZipFile("words.zip", "r") as zp:
    with zp.open("words.txt", "r") as fp:
        lines = fp.readlines()

# Shuffle the order
random.shuffle(lines)

# Output the shuffled dictionary
with zipfile.ZipFile("shuffled_words.zip", "w", zipfile.ZIP_DEFLATED) as zp:
    with zp.open("shuffled_words.txt", "w") as fp:
        fp.write(b"".join(lines))
