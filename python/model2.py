T0 =0
X0 =0
V0 =10
DT =1
A =1

def calc_time(time):
    time = time + DT
    return time
def calc_height(height, speed):
    speed = speed+(DT*A)
    if speed >= 30:
        speed = 30
    height = height + speed*DT
    return height, speed

def main(iterate):
    time = T0
    height = X0
    speed = V0
    print(time, height, speed)
    for i in range(iterate):
        time = calc_time(time)
        height, speed = calc_height(height, speed)
        print(time, height, speed)

iterate = int(input("how many iterations: "))
main(iterate)
    
