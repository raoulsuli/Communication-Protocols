#SULIMOVICI RAOUL-RENATTO 321CD

Parsare routing table-> am folosit biblioteca c de i/o stream si am salvat intr-un vector global de tip route_table_entry * intrarile din fisier. Am folosit inet_addr sa convertesc.

Cautare optima-> Am sortat datele cu o functie qsort implementata de mine, dupa care am cautat binar intrare care corespunde cerintelor(ip & mask == prefix). Dupa ce am gasit intrarea am 
ales dintre intrarile adiacente pe cea cu masca cea mai mare.

Pentru pachete-> Am testat ether_type sa vad daca este pachet ip(icmp) sau arp.
	ARP-> pentru request (arp_op == 1) aflu mac-ul cerut, schimb destinatie/sursa pentru ip si mac si trimit inapoi la sursa un arp reply cu mac-ul ptrivit.
	      pentru reply (arp_op == 2) daca ip-ul dat exista deja in arp_Table, transmit toate pachetele din coada catre destinatie.(cele care au ruta pana la destinatie)
	
	IP/ICMP-> daca este icmp echo request schimb campurile destinatie/sursa si trimit un pachet echo reply inapoi la sursa, dupa ce verific checksum, updatez ttl  si schimb protocl.
		  daca nu gasesc o ruta pentru pachet, trimit un pachet icmp destination unreachable inapoi la sursa, setand campurile ca mai sus preponderent.
		  daca are ttl<1 este un timeout, trimit pachet timeout inapoi la sursa calculand checksum.
	
	Daca nu este niciun caz de mai sus fac procesul de forwarding: testez daca checksum-ul este corect, decrementez ttl, setez checksum, calculez ruta cea mai buna. Daca pentru ruta 
cea mai buna nu am intrare arp, trimit un arp request pentru a afla mac-ul destinatiei. Daca am intrare trimit pachetul la next_hop direct.