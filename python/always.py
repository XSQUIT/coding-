import random

while True:
    
    number = random.randint(1, 6)
    #print(number)

    guess = int(input())

    if guess == number:
        print("Yay!!")
        break
    else:
        print("Too bad. Try again!")