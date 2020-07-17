import os
import shutil
from os import path

def main():

    for filename in os.listdir("."):
        print(filename)
        originalFilename = filename
        filename = filename.lower()

        filename = filename.replace("am_", "")

        os.rename(originalFilename, filename)

		
if __name__ == "__main__":
    main()