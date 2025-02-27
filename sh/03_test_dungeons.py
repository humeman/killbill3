import glob
import subprocess
files = glob.glob("saved_dungeons/*.rlg327")

with open("saved_dungeons/path_examples.txt", "r") as f:
    a = f.readlines()

for file in files:
    res = subprocess.Popen(["./assignment1_03", "--load", "--path", file], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = res.communicate()
    out = out.decode()
    err = err.decode()
    if res.returncode != 0:
        print(err)
        exit(1)
        
    # find the test output from a.txt
    for i, line in enumerate(a):
        if line.strip() == file:
            break
        
    # test dungeon
    dungeon_lines = [x.strip("\n").strip("\r") for x in a[i + 2:i + 22]]
    no_tunnel_lines = [x.strip("\n").strip("\r") for x in a[i+23:i+43]]
    tunnel_lines = [x.strip("\n").strip("\r") for x in a[i+44:i+64]]
    
    # stdout now
    lines = out.split("\n")
    dungeon_lines_real = [x.strip("\n").strip("\r") for x in lines[2:22]]
    no_tunnel_lines_real = [x.strip("\n").strip("\r") for x in lines[24:44]]
    distance_lines_real = [x.strip("\n").strip("\r") for x in lines[46:66]]
    
    if dungeon_lines_real != dungeon_lines:
        print(f"======== EXCEPTION in {file}: dungeon ========")
        print("ACTUAL:")
        print("\n".join(dungeon_lines_real))
        print("EXPECTED:")
        print("\n".join(dungeon_lines))
        exit(1)
        
    if no_tunnel_lines_real != no_tunnel_lines:
        print(f"======== EXCEPTION in {file}: no-tunnel ========")
        print("ACTUAL:")
        print("\n".join(no_tunnel_lines_real))
        print("EXPECTED:")
        print("\n".join(no_tunnel_lines))
        exit(1)
        
    if distance_lines_real != tunnel_lines:
        print(f"======== EXCEPTION in {file}: tunnel ========")
        print("ACTUAL:")
        print("\n".join(distance_lines_real))
        print("EXPECTED:")
        print("\n".join(tunnel_lines))
        exit(1)
        
    print(f"{file} passed")