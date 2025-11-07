sudo apt-get update
sudo apt-get upgrade -y
sudo apt-get install ca-certificates curl gnupg -y
sudo install -m 0755 -d /etc/apt/keyrings
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /etc/apt/keyrings/docker.gpg
echo   "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] \
https://download.docker.com/linux/ubuntu $(. /etc/os-release && echo "$VERSION_CODENAME") stable"   | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
sudo apt-get update
sudo apt-get install docker-ce-cli docker-ce docker-compose-plugin -y
cd ~/Downloads
sudo apt-get install ./docker-desktop-amd64.deb

sudo apt install mysql-client-core-8.0 -y
cd "/home/bhagawan-panditi/Documents/CS 744/Project/mysql-docker"
mkdir dbdata
docker compose up -d
mysql -h 127.0.0.1 -uroot -p

sudo apt-get install libmysqlcppconn-dev -y

mkdir -p server/include/lib
wget https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h -O server/include/lib/httplib.h

sudo apt-get install libmysqlcppconn-dev -y
mysql -h 127.0.0.1 -u root -p