#include "skel.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <queue.h>

struct route_table_entry {
	uint32_t prefix;
	uint32_t next_hop;
	uint32_t mask;
	int interface;
} __attribute__((packed));

struct arp_entry {
	uint8_t ip[4];
	uint8_t mac[6];
};
 

struct route_table_entry *rtable;
int rtable_size;

struct arp_entry *arp_table;
int arp_table_len;

int read_rtable(struct route_table_entry *rtable) {
	FILE *fp = fopen("rtable.txt", "r");
	char line[100];

	int i = 0;

	for(i = 0; fgets(line,sizeof(line), fp); i++) {
		char prefix[50], next_hop[50], mask[50], interface[50];
		sscanf(line, "%s %s %s %s", prefix, next_hop, mask, interface);
		rtable[i].prefix = inet_addr(prefix);
		rtable[i].next_hop = inet_addr(next_hop);
		rtable[i].mask = inet_addr(mask);
		rtable[i].interface = atoi(interface);
	}
	fclose(fp);
	return i;
}

void swap (struct route_table_entry *x, struct route_table_entry *y) {

	struct route_table_entry aux;
	aux.prefix = x->prefix;
	aux.next_hop = x->next_hop;
	aux.mask = x->mask;
	aux.interface = x->interface;

	x->prefix = y->prefix;
	x->next_hop = y->next_hop;
	x->mask = y->mask;
	x->interface = y->interface;

	y->prefix = aux.prefix;
	y->next_hop = aux.next_hop;
	y->mask = aux.mask;
	y->interface = aux.interface;
}

int partition(int start, int end) {
	
	struct route_table_entry *pivot = &rtable[end];
	int pIndex = start;

	for (int i = start; i < end; i++) {
		if (rtable[i].prefix < pivot->prefix) {
			swap (&rtable[i], &rtable[pIndex]);
			pIndex++;
		} else if (rtable[i].prefix == pivot->prefix) {
			if (rtable[i].mask > pivot->mask) {
				swap(&rtable[i], &rtable[pIndex]);
				pIndex++;
			}
		}
	}
	swap(&rtable[end],&rtable[pIndex]);
	return pIndex;
}

void quicksort(int start, int end) {

	if (start < end) {
		int pIndex = partition(start,end);
		quicksort(start, pIndex - 1);
		quicksort(pIndex + 1, end);
	}
}

int binarySearch(int l, int r, uint32_t dest_ip) 
{ 
    if (r >= l) { 

        int mid = l + (r - l) / 2; 

        if ((dest_ip & rtable[mid].mask) == rtable[mid].prefix) {
        	return mid; 
        }
  
        if ((dest_ip & rtable[mid].mask) < rtable[mid].prefix) {
            return binarySearch(l, mid - 1, dest_ip); 
        }
  
        return binarySearch(mid + 1, r, dest_ip); 
    } 
  
    return -1; 
} 

int get_best_route(uint32_t dest_ip) {
	
	int index = binarySearch(0, rtable_size - 1, dest_ip);

	if(index == -1) {
		return -1;
	}

	while (1) {
		if ((rtable[index - 1].prefix == (dest_ip & rtable[index - 1].mask)) && (rtable[index - 1].mask > rtable[index].mask) && (rtable[index - 1].prefix == rtable[index].prefix	)) {
			index--;
		} else {
			break;
		}
	}
	return index;
}

int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF,0);
	packet m;
	int rc;

	init();

	rtable = malloc(sizeof(struct route_table_entry) * 100000);
	
	rtable_size = read_rtable(rtable);

	queue q = queue_create();

	quicksort(0, rtable_size - 1);

	arp_table = malloc(sizeof(struct arp_entry) * 100000);

	arp_table_len = 0;

	while (1) {
		rc = get_packet(&m);
		DIE(rc < 0, "get_message");

		struct ether_header *eth_hdr = (struct ether_header *)m.payload;
		
		char * ip = get_interface_ip(m.interface);

		if(eth_hdr->ether_type == htons(0x0800)) {

			struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));

				struct icmphdr *icmp_hdr = (struct icmphdr* ) (m.payload + sizeof(struct ether_header) + sizeof(struct iphdr));

			if (ip_hdr->protocol == 1) {				


					if (icmp_hdr->type == 8) {//echo 
							//modifici reply/request

						if(ip_hdr->daddr == inet_addr(ip)) {
							icmp_hdr->type = 0;
							ip_hdr->ttl = 64;	

							u_char *aux_eth = malloc(6 *sizeof(u_char));

							memcpy(aux_eth, eth_hdr->ether_dhost, 6);
							memcpy(eth_hdr->ether_dhost, eth_hdr->ether_shost, 6);
							memcpy(eth_hdr->ether_shost, aux_eth, 6);

							uint32_t ip_aux2;
							ip_aux2 = ip_hdr->daddr;
							ip_hdr->daddr = ip_hdr->saddr;
							ip_hdr->saddr = ip_aux2;

							ip_hdr->check = 0;
							ip_hdr->check = ip_checksum(ip_hdr, sizeof(struct iphdr));
							icmp_hdr->checksum = 0;
							icmp_hdr->checksum = ip_checksum(icmp_hdr, sizeof(struct icmphdr));
							send_packet(m.interface, &m);
							continue;
						}
					} 
				}
					if (get_best_route(ip_hdr->daddr) == -1) {

						m.len = sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct icmphdr); 

						struct icmphdr *new_icmp = (struct icmphdr*) (m.payload + sizeof(struct iphdr) + sizeof(struct ether_header));

						u_char *aux_eth = malloc(6 * sizeof(u_char));

						memcpy(aux_eth, eth_hdr->ether_dhost, 6);
						memcpy(eth_hdr->ether_dhost, eth_hdr->ether_shost, 6);
						memcpy(eth_hdr->ether_shost, aux_eth, 6);

						ip_hdr->tot_len = htons(sizeof(struct icmphdr)) + htons(sizeof(struct iphdr));
						ip_hdr->ttl = 64;
						ip_hdr->protocol = 1;
						ip_hdr->check = 0;
						ip_hdr->check = ip_checksum(ip_hdr, sizeof(struct iphdr));
						
						uint32_t aux_ip = ip_hdr->daddr;
						ip_hdr->daddr = ip_hdr->saddr;
						ip_hdr->saddr = aux_ip;

						new_icmp->type = 3;
						new_icmp->code = 0;
						new_icmp->checksum = 0;
						new_icmp->checksum = ip_checksum(new_icmp, sizeof(struct icmphdr));
						send_packet(m.interface,&m);
						continue;

					}
					if (ip_hdr->ttl <= 1) {

						m.len = sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct icmphdr);
						
						struct icmphdr *new_icmp = (struct icmphdr*) (m.payload + sizeof(struct iphdr) + sizeof(struct ether_header));

						ip_hdr->ttl = 64;	
						ip_hdr->protocol = 1;
						ip_hdr->tot_len = htons(sizeof(struct icmphdr)) + htons(sizeof(struct iphdr));

						u_char *aux_eth = malloc(6 * sizeof(u_char));

						memcpy(aux_eth, eth_hdr->ether_dhost, 6);
						memcpy(eth_hdr->ether_dhost, eth_hdr->ether_shost, 6);
						memcpy(eth_hdr->ether_shost, aux_eth, 6);

						uint32_t ip_aux2;
						ip_aux2 = ip_hdr->daddr;
						ip_hdr->daddr = ip_hdr->saddr;
						ip_hdr->saddr = ip_aux2;

						ip_hdr->check = 0;

						ip_hdr->check = ip_checksum(ip_hdr, sizeof(struct iphdr));
						

							
						new_icmp->type = 11;
						
						new_icmp->checksum = 0;
						new_icmp->checksum = ip_checksum(new_icmp, sizeof(struct icmphdr));
						send_packet(m.interface, &m);
						continue;			

					}
				
					//dirijezi
					uint16_t check12 = ip_hdr->check;
					ip_hdr->check = 0;
					ip_hdr->check = ip_checksum(ip_hdr, sizeof(struct iphdr));

					if(ip_hdr->check == check12) {
						ip_hdr->ttl--;

						ip_hdr->check = 0;
						ip_hdr->check = ip_checksum(ip_hdr, sizeof(struct iphdr));
						
						int index = get_best_route(ip_hdr->daddr);

						int found = 0;
						int index1 = -1;
						for (int i = 0; i < arp_table_len; i++) {
							if (memcmp(arp_table[i].ip, &rtable[index].next_hop, 4) == 0) {
								found = 1;
								index1 = i;
								break;
							}
						}
	
						if (found == 0) {
							memcpy(eth_hdr->ether_shost, eth_hdr->ether_dhost, sizeof(eth_hdr->ether_dhost));	
							for (int i = 0; i < 5; i++) {
								eth_hdr->ether_dhost[i] = arp_table[index1].mac[i];
							}
							send_packet(rtable[index].interface, &m);
							continue;
						}

						packet *newPacket = (packet *)malloc(sizeof(packet));
						memcpy(newPacket, &m, sizeof(packet));
						
						queue_enq(q, &m);
						newPacket->len = sizeof(struct ether_arp) + sizeof(struct ether_header);
						
						struct ether_header *request_hdr = (struct ether_header *) (newPacket->payload);
						
						request_hdr->ether_type = htons(0x0806);

						struct ether_arp *request_arp = (struct ether_arp*) (newPacket->payload + sizeof(struct ether_header));
						
						request_arp->arp_pln = 4;
						request_arp->arp_hln = 6;
						request_arp->arp_op = htons(1);
						request_arp->arp_hrd = htons(1);
						request_arp->arp_pro = htons(0x0800);
						memcpy(request_arp->arp_tpa, &rtable[index].next_hop,4);
						get_interface_mac(rtable[index].interface, request_arp->arp_sha);

						uint32_t ip1 = inet_addr(get_interface_ip(rtable[index].interface));
						
						memcpy(request_arp->arp_spa, &ip1, 4);

						for (int i = 0; i < 6; i++) {
							request_arp->arp_tha[i] = (char)(255);
							request_hdr->ether_dhost[i] = (char)(255);
						}

						get_interface_mac(rtable[index].interface, request_hdr->ether_shost);
						send_packet(m.interface, newPacket);
					}
		
		} else if (eth_hdr->ether_type == htons(0x0806)) {

			struct ether_arp *arp_hdr = (struct ether_arp*)(m.payload + sizeof(struct ether_header));

			if (arp_hdr->arp_op == htons(1)) { //Daca primesc request
				uint32_t aux_copy;
				memcpy(&aux_copy, arp_hdr->arp_tpa, sizeof(arp_hdr->arp_tpa));

				if (aux_copy == inet_addr(ip)) {

					memcpy(eth_hdr->ether_dhost, eth_hdr->ether_shost, sizeof(eth_hdr->ether_shost));
					get_interface_mac(m.interface, eth_hdr->ether_shost);

					arp_hdr->arp_op = htons(2); // trimit un reply cu mac-ul corect

					uint8_t aux_ip[4];
					uint8_t aux_mac[6];

					for (int i = 0; i < 4; i++) {
						aux_ip[i] = arp_hdr->arp_spa[i];
						aux_mac[i] = arp_hdr->arp_sha[i];
					}
					aux_mac[4] = arp_hdr->arp_sha[4];
					aux_mac[5] = arp_hdr->arp_sha[5];

					for (int i = 0; i < 4; i++) {
						arp_hdr->arp_spa[i] = arp_hdr->arp_tpa[i];
					}
					get_interface_mac(m.interface, arp_hdr->arp_sha);
					for (int i = 0; i < 4; i++) {
						arp_hdr->arp_tpa[i] = aux_ip[i];
						arp_hdr->arp_tha[i] = aux_mac[i];
					}
					arp_hdr->arp_tha[4] = aux_mac[4];
					arp_hdr->arp_tha[5] = aux_mac[5];
					send_packet(m.interface, &m);
				} 

			} else if (arp_hdr->arp_op == htons(2)) { //reply

				int found = 0;
				for (int i = 0; i < arp_table_len; i++) {
					if (memcmp(arp_table[i].ip, arp_hdr->arp_spa, 4)) {
						found = 1;
						break;
					}
				}
				if (found == 0) {

					for (int i = 0; i < 4; i++) {
						arp_table[arp_table_len].ip[i] = arp_hdr->arp_spa[i];
						arp_table[arp_table_len].mac[i] = arp_hdr->arp_sha[i];
					}

					arp_table[arp_table_len].mac[4] = arp_hdr->arp_sha[4];
					arp_table[arp_table_len].mac[5] = arp_hdr->arp_sha[5];
					arp_table_len++;
				} 

				queue q_aux = queue_create();
				while (queue_empty(q) != 1) {
					
					packet *pkt = queue_deq(q);
					struct ether_header *eth_hdr = (struct ether_header*)pkt->payload;
					struct iphdr *ip_hdr = (struct iphdr*)(pkt->payload + sizeof(struct ether_header));

					int index = get_best_route(ip_hdr->daddr);
					for (int i = 0; i < arp_table_len; i++) {
						if (memcmp(arp_table[i].ip, &rtable[index].next_hop, 4) == 0) {
							memcpy(eth_hdr->ether_dhost, arp_table[i].mac, 6);
							send_packet(pkt->interface, pkt);
						} 
					}
					queue_enq(q_aux, pkt);
				}
				while(queue_empty(q_aux) != 1) {
					queue_enq(q, queue_deq(q_aux));
				}	
			}
		}
	}
}
