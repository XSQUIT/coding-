holes = ['0', '6', '8', '9']
good = []

for year in range(3001):
    if not any(char in holes for char in str(year)):
        good.append(year)
print(good)