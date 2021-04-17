nodes = ["100", "500", "1000", "2500"]
dr = "2"

for i_ind in range(0, 4):
    for j in range(1,6):
        with open("adr-" + nodes[i_ind] + "-" + dr + "-" + str(j) + ".txt") as f:
            line = f.readline()
            total_time = 0.0
            total_nodes = 0
            while line != "":
                w = line.split('\t')
                l = len(w)

                if(l == 3): #only factor in nodes that started later
                    if float(w[2][:-1]) != 0:
                        total_time += float(w[2][:-1]) #only factor in time since those nodes started transmitting
                    else:
                        total_time += float(w[2][:-1])
                    total_nodes += 1
                    #print(line)
                line = f.readline()
            if(total_nodes == 0):
                print("adr-" + nodes[i_ind] + "-" + dr + "-" + str(j) + ", ")
            else:
                total_time /= total_nodes
                print("adr-" + nodes[i_ind] + "-" + dr + "-" + str(j) + ", " + str(total_time))
