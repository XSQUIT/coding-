import time
import math

e = math.e
fi = (1+5**(1/2))/2
a = 5
b = 2

thetaprevprev = 0
thetaprev = 0
theta = 1

while True:
    thetaprevprev = thetaprev
    thetaprev = theta
    theta = thetaprev + thetaprevprev
    
    r = a * e**(b*theta)
    
    x = r * math.cos(theta)
    y = r * math.sin(theta)
    
    x = int(x)
    y = int(y)
    
    time.sleep(1)
    
    print(x,",", y)