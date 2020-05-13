
import sys

nlevels = int(sys.argv[1])

file = open("../../Modules/SedimentOxygen/Levels.h", "w")

for it in range(1, nlevels+1):
	file.write("LevelEq(%d, %d, %d)" % (it-1, it, it+1))
	
file.close()