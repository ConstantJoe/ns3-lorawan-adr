nodes = ["100", "500", "1000", "2500"]
dr = "0"

for i_ind in range(0, 4):
    for j in range(1,6):
        with open("adr-" + nodes[i_ind] + "-" + dr + "-" + str(j) + ".txt") as f:
            line = f.readline()
            last_time = "0\n"
            while line != "":
                w = line.split(' ')
                l = len(w)

                if(l == 5 and w[2] == "dr"):
                    last_time =  w[4]
                line = f.readline()
            print("adr-" + nodes[i_ind] + "-" + dr + "-" + str(j) + ", " + str(last_time[:-1]))
