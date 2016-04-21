#include <arpa/inet.h>
#include <netinet/in.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct Node Node;

struct Node {
	in_addr_t lo, hi;
	Node *next;
};

static Node *head;

static in_addr_t netmask(int prefix) {
	/* Shifting by 32 is undefined behavior, so 0 prefix is a special case. */
	return prefix == 0 ? 0 : ~(in_addr_t)0 << (32 - prefix);
}

static void printaddr(in_addr_t addr) {
	struct in_addr in;

	in.s_addr = htonl(addr);
	printf("%s", inet_ntoa(in));
}

static void cidrrange(in_addr_t lo, in_addr_t hi) {
	int prefix;
	in_addr_t brdcst, mask;

	do {
		for(prefix = 0;; prefix++) {
			mask = netmask(prefix);
			brdcst = lo | ~mask;
			if((lo & mask) == lo && brdcst <= hi)
				break;
		}
		printaddr(lo);
		printf("/%d\n", prefix);
		if(brdcst == ~(in_addr_t)0)
			break; /* Prevent overflow on the next line. */
		lo = brdcst + 1;
	} while(lo <= hi);
}

static void enumrange(in_addr_t lo, in_addr_t hi) {
	while(lo <= hi) {
		printaddr(lo++);
		putchar('\n');
	}
}

static void wholerange(in_addr_t lo, in_addr_t hi) {
	if(lo != hi) {
		printaddr(lo);
		putchar('-');
	}
	printaddr(hi);
	putchar('\n');
}

static in_addr_t atoh(char *s) {
	struct in_addr in;

	if(inet_aton(s, &in) == 0)
		errx(1, "inet_aton");
	return ntohl(in.s_addr);
}

static void parsecidr(in_addr_t *addr, in_addr_t *mask, char *str) {
	char *p;
	int prefix;

	prefix = 32;
	p = strchr(str, '/');
	if(p != NULL) {
		*p = '\0';
		prefix = atoi(p + 1);
		if(prefix < 0)
			prefix = 0;
		if(prefix > 32)
			prefix = 32;
	}
	*addr = atoh(str);
	*mask = netmask(prefix);
}

static int nodecmp(Node *a, Node *b) {
	if(a->lo < b->lo)
		return -1;
	if(a->lo > b->lo)
		return 1;
	if(a->hi < b->hi)
		return -1;
	if(a->hi > b->hi)
		return 1;
	return 0;
}

static void insert(Node **head, Node *node) {
	Node *curr;

	while((curr = *head) != NULL && nodecmp(node, curr) >= 0)
		head = &curr->next;
	*head = node;
	node->next = curr;
}

static void input(FILE *fp) {
	/* longest line is IP + '-' + IP + '\n' + '\0' */
	/* INET_ADDRSTRLEN = IP + '\0' */
	char buf[INET_ADDRSTRLEN * 2 + 1], *p;
	Node *node;
	in_addr_t addr, mask;

	while(fgets(buf, sizeof(buf), fp) != NULL) {
		p = strchr(buf, '\n');
		if(p == NULL)
			errx(1, "input line too long");
		node = malloc(sizeof(*node));
		p = strchr(buf, '-');
		if(p != NULL)
			*p = '\0';
		parsecidr(&addr, &mask, buf);
		node->lo = addr & mask;
		if(p != NULL)
			parsecidr(&addr, &mask, p + 1);
		node->hi = addr | ~mask;
		if(node->hi < node->lo)
			errx(1, "range ends are reversed");
		insert(&head, node);
	}
}

static void (*printrange)(in_addr_t lo, in_addr_t hi) = cidrrange;

static void output(void) {
	Node *node;
	in_addr_t lo, hi;

	node = head;
	while(node != NULL) {
		lo = node->lo;
		hi = node->hi;

		while(node != NULL && node->hi <= hi)
			node = node->next;
		while(node != NULL && node->lo <= hi + 1) {
			hi = node->hi;
			node = node->next;
		}
		printrange(lo, hi);
		if(node != NULL)
			node = node->next;
	}
}

int main(int argc, char *argv[]) {
	int c;
	FILE *fp;

	while((c = getopt(argc, argv, "cer")) != -1)
		switch(c) {
		case 'c':
			printrange = cidrrange;
			break;
		case 'e':
			printrange = enumrange;
			break;
		case 'r':
			printrange = wholerange;
			break;
		default:
			fputs("usage: cidr [-cer] [file ...]\n", stderr);
			exit(1);
		}
	argc -= optind;
	argv += optind;

	if(argc == 0)
		input(stdin);
	else while(*argv != NULL) {
		fp = fopen(*argv, "r");
		if(fp == NULL)
			err(1, "%s", *argv);
		input(fp);
		fclose(fp);
		argv++;
	}
	output();
	return 0;
}
