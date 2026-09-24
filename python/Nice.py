import time
import os

# Clear the screen first
os.system('cls' if os.name == 'nt' else 'clear')

# The structural layers of the light beam hitting the prism
prism_frames = [
    "         /\\        ",
    "        /  \\       ",
    "───────/────\\──────",
    "      /  /\  \\ \033[91m██████\033[0m",
    "     /  /  \  \\ \033[93m█████\033[0m",
    "    /  /────\  \\ \033[92m████\033[0m",
    "   /  /      \  \\ \033[94m███\033[0m",
    "  /──/────────\──\\ \033[95m██\033[0m"
]

try:
    while True:
        for line in prism_frames:
            print(f"   {line}")
            time.sleep(0.06)
        
        # Smooth Scroll: Feed single lines slowly instead of dumping 32 at once
        for _ in range(20):
            print("")
            time.sleep(0.06) # Ticked fast enough to look like a smooth mechanical roll
            
except KeyboardInterrupt:
    print("\n[ Station Shut Down ] Goodnight.")

