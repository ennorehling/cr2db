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

# Merge Strategien:

F�r alle Parteien: UPDATE der faction mit der neueren.
- was ist mit GEGENSTAENDE, OPTIONEN, ALLIANZ?
F�r alle Regionen:
- wenn ich GRENZE, PREISE, RESOURCE nicht sehe, dann den Block aus der DB einflechten.

# ToDo:
- "live" CR import w�hrend des parsing
    + GEGENSTAENDE, OPTIONEN, alles mit keyc == 0: nicht auf den stack
    - mit keyc < 0: nur das aktuelle array-element auf den stack.
    - ALLIANZ geht immer noch auf den Stack und nicht runter?
        - mehrere Array-Elemente in Folge erh�hen p->sp

- CR import innerhalb einer Transaktion, ROLLBACK wenn Fehelr beim parsing?

## CR import status:
- sample.cr wird gelesen bis zum ersten BATTLE
- Strategie f�r BATTLE ist noch unklar
