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

Für alle Parteien: UPDATE der faction mit der neueren.
- was ist mit GEGENSTAENDE, OPTIONEN, ALLIANZ?
Für alle Regionen:
- wenn ich GRENZE, PREISE, RESOURCE nicht sehe, dann den Block aus der DB einflechten.

# ToDo:
- "live" CR import während des parsing
    + GEGENSTAENDE, OPTIONEN, alles mit keyc == 0: nicht auf den stack
    - mit keyc < 0: nur das aktuelle array-element auf den stack.
    - ALLIANZ geht immer noch auf den Stack und nicht runter?
        - mehrere Array-Elemente in Folge erhöhen p->sp

- CR import innerhalb einer Transaktion, ROLLBACK wenn Fehelr beim parsing?

## CR import status:
- sample.cr wird gelesen bis zum ersten BATTLE
- Strategie für BATTLE ist noch unklar
