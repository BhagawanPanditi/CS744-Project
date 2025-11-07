#!/bin/bash
set -e

# update and core tools
sudo apt-get update
sudo apt-get upgrade -y
sudo apt-get install -y ca-certificates curl gnupg wget build-essential

# install docker
sudo install -m 0755 -d /etc/apt/keyrings
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /etc/apt/keyrings/docker.gpg
echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] \
https://download.docker.com/linux/ubuntu $(. /etc/os-release && echo "$VERSION_CODENAME") stable" \
| sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
sudo apt-get update
sudo apt-get install -y docker-ce docker-ce-cli containerd.io docker-compose-plugin

# install MySQL client
sudo apt-get install -y mysql-client-core-8.0

# install mysql C++ connector
sudo apt-get install -y libmysqlcppconn-dev

# download httplib header if not already there
mkdir -p server/include/lib
if [ ! -f server/include/lib/httplib.h ]; then
    wget https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h -O server/include/lib/httplib.h
fi

mkdir -p mysql-docker/dbdata
sudo docker compose -f mysql-docker/docker-compose.yml up -d
# Connect manually to MySQL (optional)
# mysql -h 127.0.0.1 -uroot -p

echo "✅ Setup complete."
