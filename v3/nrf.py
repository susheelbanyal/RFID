#!/usr/bin/env python

import RPi.GPIO as GPIO
from lib_nrf24 import NRF24
import time
import spidev
from omxplayer.player import OMXPlayer
from pathlib import Path
from time import sleep
import signal

player=''
playerRunning= False

GPIO.setmode(GPIO.BCM)

#pipes = [[0xe7, 0xe7, 0xe7, 0xe7, 0xe7], [0xc2, 0xc2, 0xc2, 0xc2, 0xc2]]
#pipes = [[0xE7, 0xE7, 0xE7, 0xE7, 0xE7], [0xF0, 0xF0, 0xF0, 0xF0, 0xE1]]
pipes = [[0xE8, 0xE8, 0xF0, 0xF0, 0xE1], [0xF0, 0xF0, 0xF0, 0xF0, 0xE1]]

radio = NRF24(GPIO, spidev.SpiDev())
radio.begin(0, 17)
radio.setPayloadSize(32)
radio.setChannel(0x76)

radio.setDataRate(NRF24.BR_1MBPS)
radio.setPALevel(NRF24.PA_MIN)

radio.setAutoAck(True)
radio.enableDynamicPayloads()
radio.enableAckPayload()

radio.openReadingPipe(1, pipes[1])
radio.printDetails()
radio.stopListening()
radio.startListening()

def playVideo(id):
	global player
	global playerRunning
	VIDEO_PATH = Path("/home/pi/NRF24/NRF24l01/videos/" + id + ".mp4")
	if VIDEO_PATH.is_file():
		player = OMXPlayer(VIDEO_PATH, args=['--loop'])
		playerRunning = True
	else:
		print("File not found");
		
def stopVideo():
	global playerRunning
	global player
	#if 'player' in globals():
	print('playerRunning')
	print(playerRunning)
	print('player')
	print(player)
	if playerRunning:
		playerRunning = False
		player.quit()
	
lastid = "";
runProgram = True;

try:
    while runProgram:
        print(radio.available(0))
        while not radio.available(0):
            time.sleep(1 / 100)
        receivedMessage = []
        radio.read(receivedMessage, radio.getDynamicPayloadSize())
        print("Received: {}".format(receivedMessage))

        #print("Translating the receivedMessage into unicode characters")
        string = ""

        for n in receivedMessage:
            # Decode into standard unicode set
            if (n >= 32 and n <= 126):
                string += chr(n)
        print("Out received message decodes to: {}".format(string))
        
        radio.stopListening()
        radio.startListening()

        data = string.split('-')
        
        print(data)
        # if(string == "stop"):
        #     stopVideo()
        #     lastid = ""
        # else:
        #     if(string != lastid):
        #         if(lastid != ""):
        #             stopVideo()
        #         lastid = string
        #         playVideo(string)
        
        time.sleep(1)

except KeyboardInterrupt:
    GPIO.cleanup()

# Capture SIGINT for cleanup when the script is aborte
def end_read(signal,frame):
    global runProgram
    global player
    runProgram = False
    #print "Ctrl+C captured, ending read."
    player.quit()
    GPIO.cleanup()

# Hook the SIGINT
signal.signal(signal.SIGINT, end_read)
