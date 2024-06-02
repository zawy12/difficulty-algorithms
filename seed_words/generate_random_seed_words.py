# This generates 12 random seed words in adjective-noun pairs and from BIP39. Requires 3 files in same location as this program.
import os 
import random

# find current file path
current_path = os.path.dirname(os.path.realpath(__file__))
os.chdir(current_path)

# load the 3 lists of 2048 words
nouns = open("nouns.txt", "r").read().split("\n") 
adjectives = open("adjectives.txt", "r").read().split("\n") 
BIP39 = open("BIP39.txt", "r").read().split("\n") 

# get 12 random numbers 0 to 2047
seeds = [random.randint(0,2047) for _ in range(12)]

# print the adjective-noun pairs matching those random numbers
for i in range(0,12):
    if i % 2==0:
        print(adjectives[seeds[i]], end = " ")
    else:
        print(nouns[seeds[i]], end = " ")
print()

# print the BIP39 words for those random numbers
for i in range(0,12):
    print (BIP39[seeds[i]], end = " ")
