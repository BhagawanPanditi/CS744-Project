#!/bin/bash
set -e

### CPU pinning plan
SERVER_CPU=2
CLIENT_CPUS=3

### Experiment Settings
PORT=9000
CACHE=1000
THREADS=8
WORKLOAD="get_popular"
DURATION=300        # 5 minutes
LOAD_LEVELS=(256 512)

mkdir -p results plots

CSV_FILE="results/summary.csv"
echo "load,throughput,latency,cpu0,cpu1,cpu2,cpu3,ram,disk,net_sent,net_recv" > $CSV_FILE

###########################################
# Build server and client
###########################################
echo "[INFO] Building server..."
(cd server && make clean && make all)

echo "[INFO] Building client..."
(cd client && make clean && make all)

###########################################
# Start Server
###########################################
echo "[INFO] Starting kv_server on CPU ${SERVER_CPU}"
taskset -c $SERVER_CPU ./server/kv_server $PORT $CACHE $THREADS > server.txt &
SERVER_PID=$!
sleep 3
echo "[INFO] Server started (PID $SERVER_PID)"

###########################################
# Function: Measure system resources
###########################################
measure_resources() {
python3 <<'EOF'
import psutil
import time, json

# measure average over 5 seconds (1 sec sampling)
samples = 5
cpu = [[] for _ in range(psutil.cpu_count())]
ram = []
disk = []
net_before = psutil.net_io_counters()

for _ in range(samples):
    per = psutil.cpu_percent(interval=1, percpu=True)
    for i,p in enumerate(per):
        cpu[i].append(p)
    ram.append(psutil.virtual_memory().percent)
    disk.append(psutil.disk_usage('/').percent)

net_after = psutil.net_io_counters()
sent = (net_after.bytes_sent - net_before.bytes_sent) / samples
recv = (net_after.bytes_recv - net_before.bytes_recv) / samples

OUT = {
    "cpu": [sum(core)/samples for core in cpu],
    "ram": sum(ram)/samples,
    "disk": sum(disk)/samples,
    "sent": sent,
    "recv": recv
}

print(json.dumps(OUT))
EOF
}

###########################################
# RUN EXPERIMENTS
###########################################
for LOAD in "${LOAD_LEVELS[@]}"; do
    echo "=========================================="
    echo "[INFO] Running workload: ${WORKLOAD}, Load: ${LOAD}"
    echo "=========================================="

    OUT="results/run_${WORKLOAD}_${LOAD}.txt"

    # Start client
    taskset -c $CLIENT_CPUS ./client/kv_client \
        $WORKLOAD $LOAD $DURATION > "$OUT" &

    CLIENT_PID=$!

    # Wait 10 seconds before measuring resources (warm-up)
    sleep 10

    # Measure system resources while workload is running
    RES=$(measure_resources)

    wait $CLIENT_PID

    echo "[INFO] Saved output → $OUT"

    ###########################
    # Extract throughput & latency
    ###########################
    TH=$(grep "Throughput:" "$OUT" | awk '{print $2}')
    LAT=$(grep "Avg Latency:" "$OUT" | awk '{print $3}')

    # Parse JSON from Python
    CPU0=$(echo $RES | python3 -c "import json,sys;print(json.load(sys.stdin)['cpu'][0])")
    CPU1=$(echo $RES | python3 -c "import json,sys;print(json.load(sys.stdin)['cpu'][1])")
    CPU2=$(echo $RES | python3 -c "import json,sys;print(json.load(sys.stdin)['cpu'][2])")
    CPU3=$(echo $RES | python3 -c "import json,sys;print(json.load(sys.stdin)['cpu'][3])")

    RAM=$(echo $RES | python3 -c "import json,sys;print(json.load(sys.stdin)['ram'])")
    DISK=$(echo $RES | python3 -c "import json,sys;print(json.load(sys.stdin)['disk'])")
    SENT=$(echo $RES | python3 -c "import json,sys;print(json.load(sys.stdin)['sent'])")
    RECV=$(echo $RES | python3 -c "import json,sys;print(json.load(sys.stdin)['recv'])")

    ###########################
    # Write CSV line
    ###########################
    echo "$LOAD,$TH,$LAT,$CPU0,$CPU1,$CPU2,$CPU3,$RAM,$DISK,$SENT,$RECV" >> $CSV_FILE

done

echo "[INFO] All experiments complete."
echo "[INFO] CSV written to $CSV_FILE"

###########################################
# Plotting (unchanged except CSV input)
###########################################
python3 <<EOF
import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("results/summary.csv")

plt.figure()
plt.plot(df.load, df.throughput, marker='o')
plt.xlabel("Load Level")
plt.ylabel("Throughput (req/s)")
plt.grid()
plt.savefig("plots/throughput_${WORKLOAD}.png")

plt.figure()
plt.plot(df.load, df.latency, marker='o')
plt.xlabel("Load Level")
plt.ylabel("Latency (ms)")
plt.grid()
plt.savefig("plots/latency_${WORKLOAD}.png")
EOF

echo "[DONE]"
