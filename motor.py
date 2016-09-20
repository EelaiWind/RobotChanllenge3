#!/usr/bin/python
import RPi.GPIO as GPIO
import time
import math
import sys
import termios
import fcntl
from curses import wrapper
from point import Point
from point import Points
import socket

"""
"color of multi-core" and "raspberry pi" pin mapping table:
orange:11
yellow:12
green: 13
blue:  15
purple:16
"""

xCwPin=11
xCcwPin=13
yCwPin=12   
yCcwPin=16
laserPin=19

GPIO.cleanup()

GPIO.setmode(GPIO.BOARD)
GPIO.setup(xCwPin, GPIO.OUT)
GPIO.setup(xCcwPin, GPIO.OUT)
GPIO.setup(yCwPin, GPIO.OUT)
GPIO.setup(yCcwPin, GPIO.OUT)
GPIO.setup(laserPin, GPIO.OUT)

#global_variable-----------------------

#record the parameter of the picture in order to do the methmatical counting
hr_degree=0 #horizontal right
hl_degree=0 #horizontal left  
hl_num=0 #how many pictures is at the left of the laser spot
hr_num=0 #how many pictures is at the right of the laser spot
hl_tan=0 #the tangent value of hl
hr_tan=0 #the tangent value of hr

vh_degree=0 #vertical height
vl_degree=0 #vertical low
vh_num=0 #how many pictures above the laser spot
vl_num=0 #how many pictures under the laser spot
vh_tan=0 #the tangent value of vh
vl_tan=0 #the tangent value of vl

#record the total degrees which has moved so far
_x_degrees=0.0
_y_degrees=0.0
_x_rotate_degrees=0.0
_y_rotate_degrees=0.0

#record the last move
_x_last_degrees=0.0
_y_last_degrees=0.0

#record for point
mem=[[0,0] for i in range(10)]
points=[]
version2=Points()

#--------------------------------------

#constant
SHOW_ROTATE_DEGREE=False
SHOW_ROTATE=False
DEBUG=True
VERSION=3 # version1:5 point locate mode    version2:32 point locate mode    version3:256 points locate mode   version4:testing mode
SHOOT_BY_TCP=True
 
TEMP=True ##testing
    
def rotate_degree(x_degree, y_degree):
    """pass in the arguments (x,y) which is the degrees intend to roatate
    Args:
        x_degree: rotate how many degrees in x-axis
        y_degree: rotate how many degrees in y-axis
    """
    rotate(0,x_degree)
    rotate(1,y_degree)

    
    
    global _x_degrees
    global _y_degrees
    global _x_last_degrees
    global _y_last_degrees
    global _x_rotate_degrees
    global _y_rotate_degrees
    
    
    _x_degrees=_x_degrees+x_degree
    _y_degrees=_y_degrees+y_degree
    _x_last_degrees=x_degree
    _y_last_degrees=y_degree
    _x_rotate_degrees=_x_rotate_degrees+round((x_degree)/0.36)*0.36
    _y_rotate_degrees=_y_rotate_degrees+round((y_degree)/0.36)*0.36
    
        
def rotate(x_or_y,degree):
    """active motor to Rotate x degree
    Args:
        x_or_y:which axis to move
        degree:rotate how many degrees. clockwise if positive else counter clockwise
    """

    #axis=0 represents x-axis
    #axis=1 represents y-axis
    
    if x_or_y=='X' or x_or_y=='x':
        axis=0
    elif x_or_y=='Y' or x_or_y=='y':
        axis=1
    elif x_or_y==0:
        axis=0
    elif x_or_y==1:
        axis=1
    else:
        print("Illeagel argument in rotate_degree")
        return

    #decide which pins to use accroding to the axis
    #info is for debug used it can be eliminated
    if axis==0:
        info="x-axis"
        stepsPin=xCwPin;
        cwOrCcwPin=xCcwPin
    elif axis==1:
        info="y-axis"
        stepsPin=yCwPin;
        cwOrCcwPin=yCcwPin

    if degree>0:
        info=info+" rotate cw"
        GPIO.output(cwOrCcwPin, True) #cw
    elif degree<0:
        info=info+" rotate ccw"
        GPIO.output(cwOrCcwPin, False) #ccw
    elif degree==0:
        return

    tmp=abs(degree)/0.36
    steps=round(tmp)

    info=info+" for "+str(degree)+" degrees "+str(steps)+" steps"

    i=0
    while i<steps:
        GPIO.output(stepsPin, True)
        time.sleep(0.001)
        GPIO.output(stepsPin, False)
        time.sleep(0.05)
        i=i+1
    #GPIO.output(cwOrCcwPin, True)

    if SHOW_ROTATE:
        print(info)




skip_shoot_by_tcp=False
def initial_h_mode(stdscr):
    """adjust the degree by hand
    8:up
    2:down
    4:left
    6:right
    +:increase the unit
    -:decrease the unit
    """
    j=0
    unit=0.36 #default
    initial_points=[]
    stdscr.clear()
    while True:
        
        #print("------------Hand control mode------------")
        #print("unit:", unit)
        #print("current position: "+"( "+str(_x_degrees)+" , "+str(_y_degrees)+" )")
        stdscr.addstr(0, 0, "------------initialize hand control mode------------")
        stdscr.addstr(1, 0, "unit: {}".format(unit))
        stdscr.addstr(2, 0, "current position: ({},{})".format(_x_degrees,_y_degrees))
        if j==0:
            stdscr.addstr(3, 0,"please shoot upper left")
        elif j==1:
            stdscr.addstr(3, 0,"please shoot upper right")
        elif j==2:
            stdscr.addstr(3, 0,"please shoot lower right")
        elif j==3:
            stdscr.addstr(3, 0,"please shoot lower left")

        ch=stdscr.getch()
        stdscr.clear()
        
        if ch==259:#up
            rotate_degree(0,unit)
        elif ch==258:#down
            rotate_degree(0,-unit)
        elif ch==261:#left
            rotate_degree(unit,0)
        elif ch==260:#right
            rotate_degree(-unit,0)
        elif ch==43:#+
            if unit>=72:
                continue
            unit=unit+0.36
        elif ch==45:#-
            if unit<=0.36:
                continue
            unit=unit-0.36
        elif ch==113 or ch==813:
            skip_shoot_by_tcp=True
            break
        elif ch==10: # enter
            initial_points.append(_x_degrees)
            initial_points.append(_y_degrees)
            j=j+1
            print(initial_points)
            if j==4:
                sendAngles(initial_points)
                return
        elif ch==8: #backspace
            if j==0:
                continue
            else:
                j=j-1
                initial_points.pop()
                initial_points.pop()
        stdscr.refresh()



def h_mode(stdscr):
    """adjust the degree by hand
    8:up
    2:down
    4:left
    6:right
    +:increase the unit
    -:decrease the unit
    """
    
    unit=0.36 #default
    stdscr.clear()
    while True:
        
        #print("------------Hand control mode------------")
        #print("unit:", unit)
        #print("current position: "+"( "+str(_x_degrees)+" , "+str(_y_degrees)+" )")
        stdscr.addstr(0, 0, "------------Hand control mode------------")
        stdscr.addstr(1, 0, "unit: {}".format(unit))
        stdscr.addstr(2, 0, "current position: ({},{})".format(_x_degrees,_y_degrees))

        ch=stdscr.getch()
        stdscr.clear()
        
        if ch==259:#up
            rotate_degree(0,unit)
        elif ch==258:#down
            rotate_degree(0,-unit)
        elif ch==261:#left
            rotate_degree(unit,0)
        elif ch==260:#right
            rotate_degree(-unit,0)
        elif ch==43:#+
            if unit>=72:
                continue
            unit=unit+0.36
        elif ch==45:#-
            if unit<=0.36:
                continue
            unit=unit-0.36
        elif ch==113 or ch==813:
            break
        stdscr.refresh()

def laser_flash(sec=1,times=1,status=0):
    """
    flash the laser
    sec:   the duration of laser on(unit:seconds)
    times: how many times to flash
    status: if status is 1 then laser will stay on after the flash, vice versa.
    """ 
    if (isinstance(sec, float) or isinstance(sec, int) )and isinstance(times, int):
        GPIO.output(laserPin, True)
        time.sleep(sec)
        GPIO.output(laserPin, False)

        if status==1:
           time.sleep(sec)
           GPIO.output(laserPin, True)




def shoot_by_tcp():
    global sock, client, _x_degrees, _y_degrees
    exit_=False
    point=[None,None]
    while True:
        data = client.recv(1024)
        data=data.decode('ascii')
        print(data) 
        tar=data.split(',')
        if data == '' or data=='q':
            break
        i=0
        print(tar)
        while i<len(tar)-1:
            try:
                if tar[i]=="":
                    i=i+1
                    continue
                if point[0] is None:
                    point[0]=float(tar[i])
                if point[1] is None:
                    point[1]=float(tar[i+1])
                    print("Point: ({},{})".format(point[0],point[1])) #change to shoot_coordinate_v2(x,y)
                    rotate_degree(point[0]-_x_rotate_degrees ,point[1]-_y_rotate_degrees)
                    laser_flash(sec=1.5, status=0)  # stauts=1 when debug
                    point[0]=None
                    point[1]=None
                    i=i+2
                    
                    """while True: #debug
                        moveToNext=input("press 'n' to continue: ").lower().split()
                        if moveToNext[0]=='n':
                            GPIO.output(laserPin, False)
                            break
                        elif moveToNext[0]=='q':
                            exit_=True
                            break
                        elif moveToNext[0]=='h':
                            wrapper(h_mode)
                        elif moveToNext[0]=='c' and len(moveToNext)==3:
                            rotate_degree(float(moveToNext[1])-_x_degrees ,float(moveToNext[2])-_y_degrees)
                        elif moveToNext[0]=='o':
                            rotate_degree(-_x_degrees, -_y_degrees)
                        elif moveToNext[0]=="so":
                            print("set ( "+str(_x_degrees)+" , "+str(_y_degrees)+" )"+" to origin")
                            _x_degrees=0
                            _y_degrees=0"""
            except ValueError:
                print("tar: ({},{})\t i:{}".format(tar[i],tar[i+1],i))
                        
            if exit_==True:
                break
                
    sock.close()

def sendAngles(initial_points):
    """
    private function to send angles
    """
    global client
    data=""
    j=0
    for i in initial_points:
        data=data+str(i)
        j=j+1
        if j!=8:
            data=data+','
    data=data.encode('ascii') #utf 8
    client.send(data)
    GPIO.output(laserPin, False)

def initialize():
    host = ''
    port = 8000
    point=[None,None]
    
    
    #port=int(input("please input the port number: "))
    global sock, client
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) #try
    sock.bind((host, port))
    sock.listen(10)

    print ('Wait for slave ...')
    (client, addr) = sock.accept()
       
if __name__=='__main__':
    """
        on: turn on the laser
        off turn off the laser
        q:quit the progrm
        o:get back to the origin
        r:return to the last position
        s:show the current position
        so:set the current position to origin
        h:move the step motor by "8,2,4,6" as up down left right
        c:use the coordinate to shoot ex:c 8 8(range is 16 X 16)
        sm [index]:set the point to the index of memory
        m [index]: go the point of the memory of index
        a [degree] [proportion of left] [proportion of right]: adjust (not available yet)
        
    """
    initialize()
    global sock
    exit=False
    GPIO.output(laserPin, True)
    while exit==False:
        user_input=input("please enter a pair of angles: ").lower().split()
        if user_input[0]=="on" or user_input[0]=="ON": 
            GPIO.output(laserPin, True)
        elif user_input[0]=="off" or user_input[0]=="OFF":
            GPIO.output(laserPin, False)
        elif user_input[0]=="q" or user_input[0]=="Q":
            GPIO.output(laserPin, False)
            sock.close()
            break;
        elif user_input[0]=="o" or user_input[0]=="O":
            rotate_degree(-_x_degrees, -_y_degrees)
        elif user_input[0]=="r" or user_input[0]=="R":
            rotate_degree(-_x_last_degrees, -_y_last_degrees)
        elif user_input[0]=="s" or user_input[0]=="S":
            print("current position: "+"( "+str(_x_degrees)+" , "+str(_y_degrees)+" )")
        elif user_input[0]=="so":
            print("set ( "+str(_x_degrees)+" , "+str(_y_degrees)+" )"+" to origin")
            _x_degrees=0
            _y_degrees=0
        elif user_input[0]=="h" or user_input[0]=="H":
            wrapper(h_mode)
        elif user_input[0]=="sm" and len(user_input)==2:
            mem[int(user_input[1])][0]=_x_degrees
            mem[int(user_input[1])][1]=_y_degrees
            print("set ({},{}) to mem[{}]".format(_x_degrees, _y_degrees, int(user_input[1])))
        elif user_input[0]=="m" and len(user_input)==2:
            index=int(user_input[1])
            if not mem[index][0]==0 and not mem[index][1]==0:
                rotate_degree(mem[index][0]-_x_degrees,mem[index][1]-_y_degrees)
                print("move to mem[{}]=({},{})".format(index, mem[index][0], mem[index][1]))
            else:
                print("the memory of the index is empty")
        elif user_input[0]=="i":
            wrapper(initial_h_mode);
            if skip_shoot_by_tcp==False:
                shoot_by_tcp()
        else :
            if len(user_input)!=2:
                continue
            #if isinstance(user_input[0],float) and isinstance(user_input[1],float):
            rotate_degree(float(user_input[0])-_x_degrees, float(user_input[1])-_y_degrees) ##testing
            print("rotate({} , {})".format(round(float(user_input[0])/0.36)*0.36, round(float(user_input[1])/0.36)*0.36)) ##testing
            
