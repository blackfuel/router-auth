blackfuel / router-auth
=======================

An Arduino Pro Mini 3.3V is used to temporarily write a passphrase into the router's RAM, for the purpose of mounting an encrypted disk at startup, without needing to permanently store the passphrase inside the router itself.  Once the router has mounted the encrypted disk, it simply deletes its copy of the passphrase file for security purposes.  If your router and external USB storage device are ever taken from you, the passphrase is kept safe on the Arduino microcontroller that can be conveniently connected and disconnected from the router, or tethered on a long cable up to 40 meters away.  Perhaps my Arduino is hidden in the wall, or accesses another Arduino over a wireless link to fetch the passphrase? 
