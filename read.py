import serial
import serial.tools.list_ports
import requests

# e.g. "https://mysite.org/send.php"
target = {your_target_page}

def list_com():
	# list all usable serial port
	com = serial.tools.list_ports.comports()
	print()
	print("All Your com port(s) list here:")
	print("*******************************")
	for list_com in com:
		print(list_com)
	print("*******************************\n")

def choose_dev():
	# loop till get correct system number
	while(1):
		sys = input("Please choose your system: 1. Linux/Mac 2. Windows\n")
		if( sys == '1' or sys == '2'):
			break
	# display usage info
	if( sys == '1'):
		com = input("Linux/Mac:\nPlease input your com port (e.g., /dev/ttyUSB0)\n")
		baud = input("baud rate? (int)\n")
	elif( sys == '2'):
		com = input("Windows:\nPlease input your com port (e.g., COM3)\n")
		baud = input("baud rate? (int)\n")
	# setting connection
	ser = connect(com, baud)
	return ser, sys

def connect(com,baud):
	try:
		ser = serial.Serial(port = com, baudrate = int(baud), bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE,timeout=2)
		# 打印設備名稱
		#print("Serial device name: " + ser.name)

	# 設定失敗就重新選擇
	except:
		print("information incorrect")
		# 重選裝置
		ser = choose_dev()
	return ser

def sendMessage(data, sys):

	# 去掉換行字元
	if (sys == '1'):
		data = data[:-1]
	elif (sys == '2'):
		data = data[:-2]
	# 判斷是否有逗號，以決定要不要註冊
	data = data.split(',')
	if( len(data) == 2):
		url = target + '?UID=' + data[0] + '&user=' + data[1]
	elif( len(data) == 1):
		url = target + '?UID=' + data[0]
	print(url)
	requests.get(url=url)



if __name__ == '__main__':
	# 列出可用 COM port
	list_com()
	# 選擇裝置
	ser, sys = choose_dev()

	try:
		ser.isOpen()
		print("Serial port is open")
	except:
		print("failure")
		exit()

	if(ser.isOpen()):
		try:
			while( True ) :
				data = ser.readlines()
				print(data)
				try:
					data = (data[0]).decode('utf-8')
					if(len(data) > 0):
						sendMessage(data, sys)	# 丟給後台傳資料
				except:
					pass

		except:
			print("error")
