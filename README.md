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

# "Neuen" CR importieren:

Datenbank �ffnen
CR laden in ein cJSON
F�r alle Parteien: UPDATE der faction.
F�r alle Regionen:
- Einheiten, Schiffe und Burgen aus dem JSON l�sen
- wenn ich drin stehe, UPDATE der region.
- wenn ich GRENZE, PREISE, RESOURCE nicht sehe, dann den Block aus der DB einflechten.
