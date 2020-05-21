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

Datenbank öffnen
CR laden in ein cJSON
Für alle Parteien: UPDATE der faction.
Für alle Regionen:
- Einheiten, Schiffe und Burgen aus dem JSON lösen
- wenn ich drin stehe, UPDATE der region.
- wenn ich GRENZE, PREISE, RESOURCE nicht sehe, dann den Block aus der DB einflechten.
