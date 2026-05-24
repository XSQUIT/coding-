#set the base values
t0 = 0
v0 = 0
h0 = 10

GRAVITATIONAL_FORCE = 9.81
TIME_STEP = 0.01
MASS = 0.25

#calculations
def calculate_time(current_time):
    current_time = current_time + TIME_STEP
    return current_time
def calculate_speed(current_height, current_speed):
    current_speed = current_speed + GRAVITATIONAL_FORCE * TIME_STEP
    current_height = current_height - current_speed * TIME_STEP
    return current_height, current_speed
def main(iterate):
    current_time = t0
    current_speed = v0
    current_height = h0
    print(current_time, current_height)
    for x in range(iterate):
        current_time = calculate_time(current_time)
        print(current_time, current_height)
        current_height, current_speed = calculate_speed(current_height, current_speed)
        
#runs
iterate = int(input("Enter the amount of times you want to calculate: "))
main(iterate)

