# Convert CR files to a database format

# /etc/apache2/sites-enabled/fcgitest.conf
<VirtualHost *:8080>
        DocumentRoot /var/www/fcgi
        ServerName pecan
        <Directory /var/www/fcgi/>
                SetHandler fcgid-script
                Options +ExecCGI
        </Directory>
        <Location />
                Options +ExecCGI
        </Location>
</VirtualHost>
