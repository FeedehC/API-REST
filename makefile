CFLAGS = -Wall -pedantic -Werror -Wextra -Wconversion -std=gnu11 -O2
CC = gcc

all: folders contador usuarios

requirements:
	sudo apt install libulfius-dev libjansson-dev libyder-dev liborcania-dev nginx whois apache2-utils -y

install:
	echo -e "\033[0;32mCompilando ejecutables\033[0m"
	make clean
	make
	sudo mkdir -p /var/log/usuarios
	sudo chmod +x ./run.sh
	sudo chmod +x ./config_hostnames.sh
	echo -e "\033[0;32mInstalando servicios contador y usuarios\033[0m"
	sudo cp ./bin/contador /bin/contador
	sudo cp ./bin/usuarios /bin/usuarios
	sudo cp ./src/contador.service /etc/systemd/system/
	sudo cp ./src/usuarios.service /etc/systemd/system/
	sudo cp ./usuarios_logrotate.conf /etc/logrotate.d/
#sudo logrotate -f -l /etc/logrotate.d/usuarios_logrotate.conf
	sudo systemctl daemon-reload
	sudo systemctl enable contador.service
	sudo systemctl start contador.service
	sudo systemctl enable usuarios.service
	sudo systemctl start usuarios.service
	echo -e "\033[0;32mConfigurando nginx web server\033[0m"
#make config_nginx

uninstall:	
	echo -e "\033[0;32mDesinstalando servicios contador y usuarios\033[0m"
	sudo systemctl stop contador.service
	sudo systemctl disable contador.service
	sudo systemctl stop usuarios.service
	sudo systemctl disable usuarios.service
	sudo systemctl daemon-reload
	echo -e "\033[0;32mEliminando archivos y carpetas excepto /var/log/usuarios\033[0m"
	sudo rm -rf /bin/contador
	sudo rm -f /etc/systemd/system/contador.service
	sudo rm -rf /etc/nginx/sites-available/config_contador
	sudo rm -rf /var/www/contador/
	sudo rm -rf /bin/usuarios
	sudo rm -f /etc/systemd/system/usuarios.service
	sudo rm -rf /etc/nginx/sites-available/config_usuarios
	sudo rm -rf /var/www/usuarios/
	sudo rm -f /etc/logrotate.d/usuarios_logrotate.conf
	make clean

delete_logs:
	echo -e "\033[0;32mEliminando /var/log/usuarios\033[0m"
	sudo rm -rf /var/log/usuarios

config_nginx:
	sudo rm -f /etc/nginx/sites-enabled/config_contador
	sudo rm -f /etc/nginx/sites-enabled/config_usuarios
	sudo cp ./config_contador /etc/nginx/sites-available/
	sudo cp ./config_usuarios /etc/nginx/sites-available/
	sudo mkdir -p /var/www/contador
	sudo mkdir -p /var/www/usuarios
	sudo cp ./src/index_contador.html /var/www/contador/index.html
	sudo cp ./src/index_usuarios.html /var/www/usuarios/index.html
#Se hace un enlace simbolico para que en la carpeta enabled simule estar el contenido de la carpeta available sin copiarlo
	sudo ln -s /etc/nginx/sites-available/config_contador /etc/nginx/sites-enabled/
	sudo ln -s /etc/nginx/sites-available/config_usuarios /etc/nginx/sites-enabled/
	sudo systemctl restart nginx

#run only once
config_http:
	sudo htpasswd -c /etc/apache2/.htpasswd fred
#sudo htpasswd /etc/apache2/.htpasswd fred
	cat /etc/apache2/.htpasswd

#run only once
config_host:
	sudo ./config_hostnames.sh
	sudo logrotate -f -l /etc/logrotate.d/usuarios_logrotate.conf


folders:
	mkdir -p ./bin ./log

contador: ./src/contador.c
	$(CC) $(CFLAGS) ./src/contador.c -o ./bin/contador -lulfius -ljansson

usuarios: ./src/usuarios.c
	$(CC) $(CFLAGS) ./src/usuarios.c -o ./bin/usuarios -lulfius -ljansson -lyder

clean:
	rm -rf ./bin *.o contador