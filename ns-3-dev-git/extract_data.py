import numpy as np
import matplotlib.pyplot as plt

START_DR = 0
START_TX = 0

states = ["BUILD", "LOCATIONS", "MAIN"]

locations = [] #format is id, x, y, z

uplink_sends = []
uplink_recvs = []
dnlink_sends = []
dnlink_recvs = []

adr_ns_success = []
adr_ns_failure = []

uplink_transmissions = []
uplink_failed_transmissions = []
dnlink_transmissions = []
dnlink_failed_transmissions = []

# match sends and recvs based on times, identify missed packets
# assign the data rate and tx power of each send-recv pair based on time of the recv

#TODO:
# write a function that calculates the convergence time
# write a function that calculates the pdr of a device across time, seperated out into different data rates.
# ensure that the NS-side ADR algorithm doesn't add a new line to list if dr and tx power haven't actually changed. Actually this doesn't really matter
# choose a colour for each dr, display a plot of final drs and locations
# major: this won't factor in when a device changes its own DR itself. Change the ns-3 sim to also output the dr every time an uplink packet is received.

#in ns-3: log_info when the network server or ed WANTS to send a packet, but can't. Factor these into the above
def main():

	state = "BUILD"

	#add 1 extra for GW
	uplink_sends.append([])
	uplink_recvs.append([])
	dnlink_sends.append([])
	dnlink_recvs.append([])
	adr_ns_success.append([])
	adr_ns_failure.append([])
	
	uplink_transmissions.append([])
	uplink_failed_transmissions.append([])
	dnlink_transmissions.append([])
	dnlink_failed_transmissions.append([])

	f = open("err.txt", "r")

	content = f.readlines()

	for line in content:
		
		if state == "BUILD":
			if line == "LOCATIONS START\n":
				state = "LOCATIONS"  #we don't do anything else with lines prior to LOCATIONS START	
		elif state == "LOCATIONS":
			if line == "LOCATIONS END\n":
				state = "MAIN"
			else:
				#extract the location data and node id. 
				words = line.split()
				locations.append((int(words[0]), float(words[1]), float(words[2]), float(words[3])))
				uplink_sends.append([])
				uplink_recvs.append([])
				dnlink_sends.append([])
				dnlink_recvs.append([])
				adr_ns_success.append([])
				adr_ns_failure.append([])

				uplink_transmissions.append([])
				uplink_failed_transmissions.append([])
				dnlink_transmissions.append([])
				dnlink_failed_transmissions.append([])
		elif state == "MAIN":
			words = line.split()

			if len(words) >= 8 and words[4] == "LoRaWANEndDevice" and words[5] == "application" and words[6] == "on" and words[7] == "node":
				#this is an up send
				time = words[0][:-1]
				node_id = int(words[8])
				uplink_sends[node_id].append(time)
				#transmitted_bytes = words[10]
			elif len(words) >= 8 and words[4] == "the" and words[5] == "Network" and words[6] == "Server" and words[7] == "received":
				#this is an up receive
				time = words[0][:-1]
				node_id = int(words[11])
				uplink_recvs[node_id].append(time)
			elif len(words) >= 6 and words[2] == "Sent" and words[3] == "DS" and words[4] == "Packet" and words[5] == "to":	
				#this is a down send
				time = words[0][:-1]
				node_id = int(words[8])
				rw = words[13] #RW1 or RW2
				dnlink_sends[node_id].append([time, rw])
			elif len(words) >= 8 and words[4] == "end" and words[5] == "device" and words[6] == "(Receiver)" and words[7] == "on":
				#this is a down receive
				time = words[0][:-1]
				node_id = int(words[9])
				dnlink_recvs[node_id].append(time) #TODO: add in RW received in
		#	#elif ADR ED side
		#	#elif ADR NS side
			elif len(words) >= 6 and words[1] == "ADR" and words[2] == "algorithm" and words[3] == "(NS" and words[4] == "side)" and words[5] == "for":
				#ADR NS side: success
				time = words[0][:-1]
				node_id = int(words[7])
				print(words)
				dr = words[18]
				tx_pow = words[21]
				#new dr, new tx
				adr_ns_success[node_id].append([time, dr, tx_pow])
			elif len(words) >= 6 and words[1] == "ADR" and words[2] == "algorithm" and words[3] == "(NS" and words[4] == "side)" and words[5] == "failed":
				#ADR NS side: failure
				time = words[0][:-1]
				node_id = int(words[8])
				adr_ns_failure[node_id].append(time)
		# 	#elif gw or device can't send?
		#	#everything else is ignored 		
	
	print("Uplink sends")
	for i, x in enumerate(uplink_sends):
		print(i)
		for y in x:
			print(y)
		print("")
		
	print("Uplink recvs")
	for i, x in enumerate(uplink_recvs):
		print(i)
		for y in x:
			print(y)
		print("")

	print("Dnlink sends")
	for i, x in enumerate(dnlink_sends):
		print(i)
		for y in x:
			print(y)
		print("")

	print("Dnlink recvs")
	for i, x in enumerate(dnlink_recvs):
		print(i)
		for y in x:
			print(y)
		print("")

	print("ADR successes")
	for i, x in enumerate(adr_ns_success):
		print(i)
		for y in x:
			print(y)
		print("")	



	#if the NS received a packet from an ED within 3 seconds of it sending one, we can assume that is a send-recv relationship
	for ind, device_sends in enumerate(uplink_sends):
		#device_sends is a list
		for send_time in device_sends:
			found = False
			#for this uplink send, get the dr and tx power used
			dr = 0
			tx = 0
			for e, row in enumerate(adr_ns_success[ind]):

				if e == len(adr_ns_success[ind]) - 1: #if at the end
					dr = row[1]
					tx = row[2]
					break  
				elif e == 0 and float(send_time) < float(row[0]): #if at the start
					dr = START_DR
					tx = START_TX
					break
				else:
					if float(send_time) > float(row[0]) and float(send_time) <= float(adr_ns_success[ind][e][0]): #else
						dr = row[1]
						tx = row[2]
						break

			for recv_time in uplink_recvs[ind]:
				time_dif = float(recv_time) - float(send_time) 
				if time_dif <= 3.0 and time_dif >= 0.0:
					uplink_transmissions[ind].append([float(send_time), dr, tx])
					found = True
					break
			if not found:
				uplink_failed_transmissions[ind].append([float(send_time), dr, tx])

	#same for downlink
	for ind, ns_sends in enumerate(dnlink_sends):
		#ns_sends is a list of lists in the format [time, rw]
		for send_time in ns_sends:
			found = False
			#for this downlink send, get the dr and tx power used (rw can be used to get dr)
			dr = 0
			tx = 0
			if send_time[1] == "RW2":
				dr = 0
				tx = 0
			else:
				for e, row in enumerate(adr_ns_success[ind]):
					if e == len(adr_ns_success[ind]) - 1: #if at the end
						dr = row[1]
						tx = row[2]
						break  
					elif e == 0 and float(send_time[0]) <= float(row[0]): #if at the start
						dr = START_DR
						tx = START_TX
						break
					else:
						if float(send_time[0]) > float(row[0]) and float(send_time[0]) <= float(adr_ns_success[ind][e][0]): #else
							dr = row[1]
							tx = row[2]
							break

			for recv_time in dnlink_recvs[ind]:
				time_dif = float(recv_time) - float(send_time[0]) 
				if time_dif <= 3.0 and time_dif >= 0.0:
					dnlink_transmissions[ind].append([float(send_time[0]), dr, tx])
					found = True
					break
			if not found:
				dnlink_failed_transmissions[ind].append([float(send_time[0]), dr, tx])		

	print("Uplink successful transmissions")
	for i, x in enumerate(uplink_transmissions):
		print(i)
		for y in x:
			print(y)
		print("")

	print("Uplink failed transmissions")
	for i, x in enumerate(uplink_failed_transmissions):
		print(i)
		for y in x:
			print(y)
		print("")

	print("Downlink successful transmissions")
	for i, x in enumerate(dnlink_transmissions):
		print(i)
		for y in x:
			print(y)
		print("")

	print("Downlink failed transmissions")
	for i, x in enumerate(dnlink_failed_transmissions):
		print(i)
		for y in x:
			print(y)
		print("")

	x = []
	y = []

	dr_map = [
	[[],[]],
	[[],[]],
	[[],[]],
	[[],[]],
	[[],[]]
	]

	dr5_txpow_map = [
	[[],[]],
	[[],[]],
	[[],[]],
	[[],[]],
	[[],[]],
	[[],[]],
	[[],[]],
	[[],[]]
	]
	for ind, adr_per_node in enumerate(adr_ns_success):
		if ind != 0: #don't do for gateway
			#get last member of list
			if len(adr_per_node) > 0: #if at least one ADR dl was sent.
				last_adr = adr_per_node[len(adr_per_node)-1]
				last_dr  = last_adr[1]
				last_dr  = ord(last_dr)
			else:
				last_dr = START_DR

			if last_dr != 5:
				dr_map[last_dr][0].append(locations[ind-1][1]) #x coor
				dr_map[last_dr][1].append(locations[ind-1][2]) #y coor
			else:
				if len(adr_per_node) > 0: #if at least one ADR dl was sent.
					last_adr = adr_per_node[len(adr_per_node)-1]
					last_txPow  = last_adr[2]
					last_txPow  = ord(last_txPow)
				else:
					last_txPow = START_TX

				dr5_txpow_map[last_txPow][0].append(locations[ind-1][1]) #x coor
				dr5_txpow_map[last_txPow][1].append(locations[ind-1][2]) #y coor


	#for line in locations:
	#	x.append(line[1])
	#	y.append(line[2])

	#x.append(0) #the gateway
	#y.append(0)

	for x in range(0, 5):
		dr_color = 'C' + str(x)
		dr_str = 'DR' + str(x)
		plt.scatter(dr_map[x][0], dr_map[x][1], color=dr_color, label=dr_str, marker="o")	
	
	for x in range(0, 7):
		y = x+5

		if y == 10:
			dr_color = 'b'
		elif y == 11:
			dr_color = 'g'
		elif y == 12:
			dr_color = 'm'
		else:	
			dr_color = 'C' + str(y)
		dr_str = 'DR5, TX' + str(x)

		plt.scatter(dr5_txpow_map[x][0], dr5_txpow_map[x][1], color=dr_color, label=dr_str, marker="s")

	plt.scatter([0], [0], color='black', marker="p") #gateway

	plt.title('locations of the nodes')
	plt.xlabel('x')
	plt.ylabel('y')
	plt.legend(loc='right')
	plt.show()

if __name__ == "__main__":
	main()
