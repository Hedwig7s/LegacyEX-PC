import os
import sys
from deps import tree_hash

def hash_directory(directory):
    files = {}

    for root, _, filenames in os.walk(directory):
        for filename in filenames:
            full_path = os.path.join(root, filename)
            relative_path = os.path.relpath(full_path, directory).replace("\\", "/")

            with open(full_path, "rb") as f:
                files[relative_path] = f.read()

    return tree_hash(files)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("Usage: python hash.py <directory>")

    print(hash_directory(os.path.abspath(sys.argv[1])))
