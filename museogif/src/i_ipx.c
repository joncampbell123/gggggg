
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include <time.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>

#include "i_ipx.h"

#ifdef __GLIBC__
#define MYSOCKLEN_T socklen_t
#include <netipx/ipx.h>
#else
#define MYSOCKLEN_T int
#include <linux/ipx.h>
#endif /* __GLIBC__ */

#define IPXSETUP_COMPAT    /* be compatible with ipxsetup.exe */

#define MY_PORT 0x869b     /* default port */
#define MAX_INTERFACES 6
#define PROC_IPX_IF "/proc/net/ipx_interface"



/*
 * Novell IPX NETWORKING
 */

/* setupdata_t is used as doomdata_t during setup (from ipxnet.h/ipxsrc.zip) */
typedef struct
{
  short gameid __attribute__((packed)); /* so multiple games can setup at once */
  short drone __attribute__((packed));
  short nodesfound __attribute__((packed));
  short nodeswanted __attribute__((packed));
} setupdata_t;

typedef unsigned char ipx_node[6];
typedef unsigned int ipx_net;
typedef unsigned short ipx_port;

typedef struct {
  struct sockaddr_ipx sa;  /* address of local interface */
  int sock;
} ipxif;   /* ipx interface */

static ipxif ipx_networks[MAX_INTERFACES];
static int num_ipxifs;   /* # of IPX interfaces found */

typedef struct {
  int interface;  /* index in ipx_networks */
  struct sockaddr_ipx peer_addr;
} ipx_peer;  /* network opponents */

static ipx_peer ipx_peers[MAXNETNODES];
static setupdata_t nodesetup[MAXNETNODES];

static int all_players_known=FALSE; /* if TRUE, net startup has finished, game in play */
static int DOOMPORT = MY_PORT;      /* port (IPX socket) to use */

static int my_localtime;
static int recv_maxfd;
static fd_set recv_fd_set;

typedef struct {
  int time;          /* sequence no., -1 at setup time */
  doomdata_t swx;
  char buffer[20];   /* to check for big packets */
} recvbuf_t, sendbuf_t;

recvbuf_t rb;                   /* my receive buffer */
struct sockaddr_ipx rb_address; /* my receive-address buffer */
#define remotetime (rb.time)

#define BROADCAST_NODE "\377\377\377\377\377\377"
static struct sockaddr_ipx broad_addr;  /* contains broadcast address */

/* prototypes */

static int get_ipx_interfaces(void);
static void look_for_nodes(int numnetnodes);
static void init_ipx(int interface);
static void ipx_send(int dest,void *data,int datalen);
static int ipx_select(void);
static void IPXPacketSend_notime (void);
#ifdef IPX_DEBUG
static void ipx_print_sockaddr(FILE *stream,struct sockaddr_ipx *sa);
static void ipx_print_network(FILE *stream,ipx_net *net);
static void ipx_print_node(FILE *stream,ipx_node *node);
static void ipx_print_port(FILE *stream,ipx_port *port);
#endif /* #ifdef IPX_DEBUG */

static inline int ipx_same_node(ipx_node *a,ipx_node *b)
{
  return(!memcmp(a,b,sizeof(ipx_node)));
}

static inline int ipx_same_addr(struct sockaddr_ipx *a,struct sockaddr_ipx *b)
{
  if (a->sipx_network != b->sipx_network || a->sipx_port != b->sipx_port) return(FALSE);
  return(ipx_same_node(&a->sipx_node,&b->sipx_node));
}


/*
 * IPXPacketSend
 * sends a packet and updates my_localtime
 */

void IPXPacketSend(void)  /* gets called from outside via pointer */
{
  my_localtime++;
  IPXPacketSend_notime();
}


/*
 * IPXPacketSend_notime
 * sends a packet w/o updating my_localtime
 */

static void IPXPacketSend_notime (void)
{
  sendbuf_t  sb;

  memcpy(&sb.swx,netbuffer,doomcom->datalength);
  sb.time = my_localtime;
#ifdef IPXSETUP_COMPAT    /* make packet 4 bytes longer, filled with zeros */
  /* FIXME: access might be unaligned! */
  *(unsigned long *)(((char *)(&sb.swx))+doomcom->datalength)=0;  /* dos doom hack */
#endif
  
#if 0   /* no byte swap for now, only for LITTLE_ENDIAN */
  sw.checksum = htonl(netbuffer->checksum);
  sw.player = netbuffer->player;
  sw.retransmitfrom = netbuffer->retransmitfrom;
  sw.starttic = netbuffer->starttic;
  sw.numtics = netbuffer->numtics;
  for (c=0 ; c< netbuffer->numtics ; c++)
    {
      sw.cmds[c].forwardmove = netbuffer->cmds[c].forwardmove;
      sw.cmds[c].sidemove = netbuffer->cmds[c].sidemove;
      sw.cmds[c].angleturn = htons(netbuffer->cmds[c].angleturn);
      sw.cmds[c].consistancy = htons(netbuffer->cmds[c].consistancy);
      sw.cmds[c].chatchar = netbuffer->cmds[c].chatchar;
      sw.cmds[c].buttons = netbuffer->cmds[c].buttons;
      sw.cmds[c].lookfly = netbuffer->cmds[c].lookfly;
      sw.cmds[c].arti = netbuffer->cmds[c].arti;
    }
#endif
  
#ifdef IPXSETUP_COMPAT
  ipx_send(doomcom->remotenode,&sb,doomcom->datalength+sizeof(int) + 4);
#else
  ipx_send(doomcom->remotenode,&sb,doomcom->datalength+sizeof(int)); /* sizeof time */
#endif
}


/*
 * IPXPacketGet
 * doomcom.remotenode = -1, packet from unknown/new peer
 * returns interface # (index in ipx_networks[]), where packet was reveived, -1 for error
 */

int IPXPacketGet (void)  /* gets called from outside via pointer */
{
#define sw (rb.swx)
  int         i;
  int         c;
  MYSOCKLEN_T fromlen;
  int         interface;
  
 restart:
  
  interface=ipx_select();  /* get interface to recvfrom */
  if (interface == -1) {
    doomcom->remotenode = -1;  /* indicates "no packet" on higher level (not setup time) */
    return(-1);  /* indicates "no packet" */
  }
  
#ifdef IPX_DEBUG
  /*
    printf("IPXPacketGet(): receiving from i.f. %d (",interface);
    ipx_print_sockaddr(stdout,&ipx_networks[interface].sa);
    printf(")\n");
  */
#endif
  
  fromlen = sizeof(rb_address);
  c = recvfrom(ipx_networks[interface].sock,&rb,sizeof(rb),0,
	       (struct sockaddr *)&rb_address,&fromlen);
  if (c == -1 ) {
    I_Error ("IPXPacketGet: recvfrom(): %s",strerror(errno));
  }
  
  /* check, whether packet is from myself and drop it if yes */
  if (ipx_same_addr(&rb_address,&ipx_networks[interface].sa)) {
    goto restart;
  }
  
#ifdef IPX_DEBUG
  printf("IPXPacketGet(): received from (");
  ipx_print_sockaddr(stdout,&rb_address);
  printf("): %d bytes\n",c);
  
  if (c > sizeof(sw) + sizeof(int)) {
    printf("**BIG SIZE PACKET RECEIVED: size: %d, max: %d\n",c,sizeof(sw)+sizeof(int));
  }
#endif
  
  /* search for peer address in my table, start w/index 1, index 0 is myself (dummy) */
  for (i=1; i<doomcom->numnodes; i++) {
    if (rb_address.sipx_network == ipx_peers[i].peer_addr.sipx_network &&
	ipx_same_node(&rb_address.sipx_node,&ipx_peers[i].peer_addr.sipx_node))
      break;
  }
  
  if (i == doomcom->numnodes) { /* not found in table... */
    doomcom->remotenode = -1;              /* packet from unknown peer */
  }
  else {
    doomcom->remotenode = i;               /* good packet from a game player */
  }
  
#ifdef IPXSETUP_COMPAT
  doomcom->datalength = c - sizeof(int) - 4; /* disregard lenght of time counter and dummies */
#else
  doomcom->datalength = c - sizeof(int);     /* disregard lenght of time counter */
#endif
  memcpy(netbuffer,&sw,c);                   /* copy received data to doom network buffer */
  return(interface);
  
#if 0        /* no swap for now, works on LITTLE_ENDIAN only */
  /* byte swap */
  netbuffer->checksum = ntohl(sw.checksum);
  netbuffer->player = sw.player;
  netbuffer->retransmitfrom = sw.retransmitfrom;
  netbuffer->starttic = sw.starttic;
  netbuffer->numtics = sw.numtics;
  
  for (c=0 ; c< netbuffer->numtics ; c++)
    {
      netbuffer->cmds[c].forwardmove = sw.cmds[c].forwardmove;
      netbuffer->cmds[c].sidemove = sw.cmds[c].sidemove;
      netbuffer->cmds[c].angleturn = ntohs(sw.cmds[c].angleturn);
      netbuffer->cmds[c].consistancy = ntohs(sw.cmds[c].consistancy);
      netbuffer->cmds[c].chatchar = sw.cmds[c].chatchar;
      netbuffer->cmds[c].buttons = sw.cmds[c].buttons;
      netbuffer->cmds[c].lookfly = sw.cmds[c].lookfly;
      netbuffer->cmds[c].arti = sw.cmds[c].arti;
    }
#endif
}
#undef sw


/*
 * I_InitIPX
 */

void I_InitIPX (void)    /* called from outside */
{
  int i,p;
  int num_netnodes;  /* from command line */
  
  p = M_CheckParm ("-port");
  if (p && p<myargc-1)
    {
      DOOMPORT = atoi (myargv[p+1]);
      printf ("using alternate port %i\n",DOOMPORT);
    }
  
  /* get # of players from command line */
  /* e.g. "-ipx 3" ---> 3 players */
  i = M_CheckParm("-ipx");
  if (!i || i>=myargc-1) {
    I_Error("-ipx switch needs an argument");
  }
  num_netnodes=atoi(myargv[i+1]);
  if (num_netnodes <= 1 || num_netnodes > MAXNETNODES) {
    I_Error("*how many* net players?");
  }
  
  num_ipxifs=get_ipx_interfaces();
/*
  if (!num_ipxifs) {
    I_Error("IPX not installed/configured");
  }
*/

  fprintf(stderr, "I_InitIPX num_ipxifs: %d\n", num_ipxifs);
 
  for (i=0; i<num_ipxifs; i++) {
    init_ipx(i);  /* init interface: socket() + bind() */
  }
  
  look_for_nodes(num_netnodes);  /* find and wait for the other nodes */
  
  my_localtime = 0;
  
  doomcom->id = DOOMCOM_ID;
  doomcom->numplayers = doomcom->numnodes;
}


/*
 * look_for_nodes
 * find other network players
 * large parts of this routine are from ipxsetup.c (from ipxsrc.zip)
 */

static void look_for_nodes(int numnetnodes)
{
  int          i;
  int          interface;
  time_t       curtime;
  time_t       oldsec;
  setupdata_t  *setup,*dest;
  int          total,console;
  
  /*
   * wait until we get [numnetnodes] packets, then start playing...
   * the playernumbers are assigned by netid
   */
  printf("Attempting to find all players for %i player net play.\n", numnetnodes);
  printf("Looking for a node");
  
  oldsec = -1;
  setup = (setupdata_t *)&doomcom->data;
  my_localtime = -1;          /* in setup, not game */
  
  nodesetup[0].nodesfound = 1;
  nodesetup[0].nodeswanted = numnetnodes;
  doomcom->numnodes = 1;
  memcpy(&ipx_peers[0].peer_addr,&ipx_networks[0].sa,sizeof(struct sockaddr_ipx));
  
  do {
    /*
     * listen to the network
     */
    while ((interface=IPXPacketGet()) != -1)
      {
	
#ifdef IPX_DEBUG
	printf("packet received before: ID = %04X\n",setup->gameid);
	for (i=0; i<doomcom->numnodes; i++) {
	  printf("i=%d, found=%d, wanted=%d\n",i,
		 nodesetup[i].nodesfound,
		 nodesetup[i].nodeswanted);
	}
#endif
	
	if (doomcom->remotenode == -1)  /* sender known? */
	  dest = &nodesetup[doomcom->numnodes];   /* no */
	else
	  dest = &nodesetup[doomcom->remotenode]; /* yes */
	
	if (remotetime != -1)
	  {   /* an early game packet, not a setup packet */
	    if (doomcom->remotenode == -1)
	      I_Error("Got an unknown game packet during setup");
	    /* if peer already started, it must have found all nodes */
	    dest->nodesfound = dest->nodeswanted;
	    printf("**early game packet received\n");
	    continue;
	  }
	
	/* update setup info */
	memcpy(dest,setup,sizeof(*dest));
	
#ifdef IPX_DEBUG
	printf("packet received after:\n");
	for (i=0; i<doomcom->numnodes; i++) {
	  printf("i=%d, found=%d, wanted=%d\n",i,
		 nodesetup[i].nodesfound,
		 nodesetup[i].nodeswanted);
	}
#endif
	
	if (doomcom->remotenode != -1)
	  continue;           /* already know that node address */
	
	/*
	 * this is a new node
	 * fill in ipx_peers[] entry
	 */
	memcpy(&ipx_peers[doomcom->numnodes].peer_addr,&rb_address,
	       sizeof(struct sockaddr_ipx));
	ipx_peers[doomcom->numnodes].interface=interface;
	
	/*
	 * if this node has a lower address, take all startup info
	 */
	/*
	  if (memcmp(&remoteadr,&nodeadr[0],sizeof(&remoteadr)) < 0)
	  {
	  }
	*/
	
	doomcom->numnodes++;
	
	printf("\nFound a node!\n");
	
	if (doomcom->numnodes < numnetnodes)
	  printf("Looking for a node");
      }
    
    /*
     * we are done if all nodes have found all other nodes
     */
    
    for (i=0; i<doomcom->numnodes; i++)
      if (nodesetup[i].nodesfound != nodesetup[i].nodeswanted)
	break;
    
    if (i == nodesetup[0].nodeswanted)
      break;         /* got them all */
    
    /*
     * send out a broadcast packet every second
     */
    curtime=time(NULL);
    if (curtime == oldsec)
      continue;
    oldsec = curtime;
    
    printf (".");
    doomcom->datalength = sizeof(*setup);
    
    nodesetup[0].nodesfound = doomcom->numnodes;
    memcpy (&doomcom->data, &nodesetup[0], sizeof(*setup));
    doomcom->remotenode=MAXNETNODES;  /* indicate broadcast address */
    IPXPacketSend_notime();     /* send to all */
  } while (1);
  
  /*
   * count players
   */
  total = 0;
  console = 0;
  
  for (i=0 ; i<numnetnodes ; i++)
    {
      if (nodesetup[i].drone)
	continue;
      total++;
      if (total > MAXPLAYERS)
	I_Error("More than %i players specified!",MAXPLAYERS);
      
#ifdef IPX_DEBUG
      printf("player: %d - ",i);
      ipx_print_node(stdout,&ipx_peers[i].peer_addr.sipx_node);
      printf(";  interface - ");
      ipx_print_node(stdout,&ipx_networks[ipx_peers[i].interface].sa.sipx_node);
      printf ("\n");
#endif
      
      if (memcmp (&ipx_peers[i].peer_addr.sipx_node,
		  &ipx_networks[ipx_peers[1].interface].sa.sipx_node,
		  sizeof(ipx_node)) < 0) {
	console++;
      }
    }
  
  if (!total)
    I_Error("No players specified for game!");
  
  doomcom->consoleplayer = console;
  doomcom->numplayers = total;
  
  /* build fdset for subsequent select calls */
  FD_ZERO(&recv_fd_set);
  recv_maxfd=0;
  for (i=1; i<doomcom->numnodes; i++) {
    FD_SET(ipx_networks[ipx_peers[i].interface].sock,&recv_fd_set);
    if (ipx_networks[ipx_peers[i].interface].sock > recv_maxfd)
      recv_maxfd=ipx_networks[ipx_peers[i].interface].sock;
  }
  recv_maxfd++;
  all_players_known=TRUE;
  
  printf("Console is player %i of %i\n", console+1, total);
}


/*
 * determines from which socket (ipx_network) to receive the next packet
 * returns -1, if no packet waiting...
 */

static int ipx_select(void)
{
  fd_set back_fds;
  int maxfd;
  fd_set fds;
  int ret,i;
  struct timeval timeout;
  
  /* build fd_set */
  
  if (all_players_known) { /* game time -- use prebuilt fdset */
    fds=recv_fd_set;
    maxfd=recv_maxfd;
  }
  else {  /* setup time -- build fdset */
    FD_ZERO(&fds);
    maxfd=0;
    for (i=0; i<num_ipxifs; i++) {
      FD_SET(ipx_networks[i].sock,&fds);
      if (ipx_networks[i].sock > maxfd)
	maxfd=ipx_networks[i].sock;
    }
    maxfd++; /* index --> number */
  }
  back_fds=fds;
  
  while(1) {
    timeout.tv_sec=timeout.tv_usec=0; /* no timeout, no block, no time to waste */
    ret=select(maxfd,&fds,NULL,NULL,&timeout);
    if (ret == -1) {  /* error occurred */
      if (errno == EINTR) {
	fds=back_fds;
	continue;
      }
      I_Error("select(): %s",strerror(errno));
    }
    if (!ret) return(-1);  /* no packet waiting */
    if (all_players_known) {
      for (i=1; i<doomcom->numnodes; i++) {
	if (FD_ISSET(ipx_networks[ipx_peers[i].interface].sock,&fds)) {
	  return(ipx_peers[i].interface);
	}
      }
    }
    else {
      for (i=0; i<num_ipxifs; i++) {
	if (FD_ISSET(ipx_networks[i].sock,&fds)) {
	  return(i);
	}
      }
    }
    /* shouldn't arrive here... */
    I_Error("fallen out of function ipx_select() ... booouummm!");
  }
}


/*
 * generic send routine
 * dest is index into ipx_peers[] or MAXNETNODES for broadcast
 */

static void ipx_send(int dest,void *data,int datalen)
{
  int i,status;
  
  if (dest != MAXNETNODES) {
    /* send to a single peer */
    status=sendto(ipx_networks[ipx_peers[dest].interface].sock,
		  data,datalen,0,(struct sockaddr *)&ipx_peers[dest].peer_addr,
		  sizeof(struct sockaddr_ipx));
    if (status == -1) {
      fprintf(stderr,"sendto() failed: %s\n",strerror(errno));
    }
    return;
  }
  
  /* do broadcasts to all connected IPX networks */
  broad_addr.sipx_family=AF_IPX;
  broad_addr.sipx_type=0;
  broad_addr.sipx_port=htons(DOOMPORT);
  memcpy(&broad_addr.sipx_node,BROADCAST_NODE,sizeof(ipx_node));
  for (i=0; i<num_ipxifs; i++) {
    broad_addr.sipx_network=ipx_networks[i].sa.sipx_network; /* set network address */
    status=sendto(ipx_networks[i].sock,data,datalen,0,
		  (struct sockaddr *)&broad_addr,sizeof(struct sockaddr_ipx));
#ifdef IPX_DEBUG
    if (status == -1) {
      int oerrno=errno;
      
      fprintf(stderr,"cannot broadcast to <");
      ipx_print_network(stderr,(ipx_net *)&broad_addr.sipx_network);
      fprintf(stderr,">: %s -- ignoring\n",strerror(oerrno));
    }
#endif
  }
}


/*
 * table of possible IPX frametypes
 * used by get_ipx_interfaces()
 */

static unsigned char ipx_frametypes[] = {
  IPX_FRAME_NONE,
  IPX_FRAME_SNAP,
  IPX_FRAME_8022,
  IPX_FRAME_ETHERII,
  IPX_FRAME_8023,
  IPX_FRAME_TR_8022
};
#define NUM_IPX_FRAMES sizeof(ipx_frametypes)

/*
 * tries to detect all IPX interfaces, /proc support not needed
 * won't detect internal IPX net, but we don't need that anyway...
 */

static int get_ipx_interfaces(void)
{
  int sock,res,i,j,k=0;
  char buff[1024];
  struct ifreq ifr,*pifr,ifr_bak;
  struct ifconf ifc;
  struct sockaddr_ipx *sipx;
  
  memset(ipx_networks,0,sizeof(ipx_networks));  /* initialize network list */
  memset(&ifr,0,sizeof(struct ifreq));
  memset(&ifc,0,sizeof(struct ifconf));
  memset(buff,0,1024);
  
  ifc.ifc_len=sizeof(buff);
  ifc.ifc_buf=buff;
  
  sock = socket(AF_IPX, SOCK_DGRAM, PF_IPX);
  if (sock == -1) {
    I_Error("socket(): failed\n");
  }
  
  res=ioctl(sock,SIOCGIFCONF,&ifc);
  if (res<0) {
    I_Error("ioctl(...,SIOCGIFCONF,...): %s\n",strerror(errno));
  }
  
  pifr=ifc.ifc_req;
  
  for (i=ifc.ifc_len / sizeof(struct ifreq); --i >=0; pifr++) {
#ifdef IPX_DEBUG
    printf("found interface %s\n",pifr->ifr_name);
#endif
    sipx=(struct sockaddr_ipx *)&pifr->ifr_addr;
    memcpy(&ifr_bak,pifr,sizeof(ifr_bak));
    for (j=0; j<(int)NUM_IPX_FRAMES; j++) { /* try all frame types */
      memcpy(pifr,&ifr_bak,sizeof(ifr_bak));
      sipx->sipx_type=ipx_frametypes[j];
      res=ioctl(sock,SIOCGIFADDR,pifr);
      if (!res) {
	ipx_networks[k].sa.sipx_network=sipx->sipx_network;
	memcpy(&ipx_networks[k++].sa.sipx_node,
	       sipx->sipx_node,sizeof(sipx->sipx_node));
      }
#ifdef IPX_DEBUG
      if (res<0) {
	printf(" -> ioctl(): %s\n",strerror(errno));
      }
      else {
	printf("  -> net address: ");
	ipx_print_network(stdout,(ipx_net *)&sipx->sipx_network);
	printf("\n");
      }
#endif
    }
  }
  
  close(sock);
  return(k);
}


#if 0

/*
 * get all IPX interfaces, needs /proc support, *not finished*
 */

static ipxif *get_ipx_interfaces(void)
{
  int fd;
  off_t size;
  char *line;
  
  fd=open(PROC_IPX_IF,O_RDONLY);
  if (fd == -1) {
    return(NULL);  /* cannot open -> maybe not present */
  }
  wrkbuf=malloc(8192); /* should be nuff */
  if (!wrkbuf) return(NULL);
  size=read(fd,wrkbuf,8192);
  if (!size) return(NULL);
  while (line = getline(&curpos,&size)) ; /* ... &c. &c. &c. ... */
}

#endif

/*
 * initialize an IPX interface: open socket, bind to doom port, enable broadcasts
 */

static void init_ipx(int interface)
{
  int         option=1;  /* for setsockopt() */
  int         status;
  MYSOCKLEN_T saipxlen=sizeof(struct sockaddr_ipx);
  
  ipx_networks[interface].sock=socket(AF_IPX,SOCK_DGRAM,PF_IPX);
  if (ipx_networks[interface].sock == -1) {
    I_Error("cannot open IPX socket: %s",strerror(errno));
  }
  ipx_networks[interface].sa.sipx_family=AF_IPX;
  ipx_networks[interface].sa.sipx_port=htons(DOOMPORT);
  ipx_networks[interface].sa.sipx_type=0;
  if (bind(ipx_networks[interface].sock,
	   (struct sockaddr *)&ipx_networks[interface].sa,
	   sizeof(struct sockaddr_ipx))) {
    I_Error("cannot bind to doom port: %s",strerror(errno));
  }
  status=getsockname(ipx_networks[interface].sock,
		     (struct sockaddr *)&ipx_networks[interface].sa,
		     &saipxlen);
  if (status == -1) {
    I_Error("getsockname() failed: %s",strerror(errno));
  }
#ifdef IPX_DEBUG
  printf("Using IPX address: ");
  ipx_print_sockaddr(stdout,&ipx_networks[interface].sa);
  printf("\n");
#endif
  /* enable broadcasts */
  if (setsockopt(ipx_networks[interface].sock,SOL_SOCKET,SO_BROADCAST,&option,sizeof(option))) {
    I_Error("cannot enable broadcasts: %s",strerror(errno));
  }
}

#ifdef IPX_DEBUG

static void ipx_print_sockaddr(FILE *stream,struct sockaddr_ipx *sa)
{
  ipx_print_network(stream,(ipx_net *)&sa->sipx_network);
  fprintf(stream,":");
  ipx_print_node(stream,&sa->sipx_node);
  fprintf(stream,":");
  ipx_print_port(stream,&sa->sipx_port);
}

static void ipx_print_network(FILE *stream,ipx_net *net)
{
  ipx_net h=ntohl(*net);
  fprintf(stream,"%02X%02X%02X%02X",
	  (h>>24)&0xff,(h>>16)&0xff,(h>>8)&0xff,h&0xff);
}

static void ipx_print_node(FILE *stream,ipx_node *node)
{
  int i;
  for (i=0; i<sizeof(ipx_node); i++) fprintf(stream,"%02X",*((unsigned char *)node+i));
}

static void ipx_print_port(FILE *stream,ipx_port *port)
{
  fprintf(stream,"%04X",ntohs(*port));
}

#endif /* #ifdef IPX_DEBUG */

