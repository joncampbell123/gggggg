
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/ioctl.h>

#include "i_udp.h"

#ifdef __GLIBC__
#define MYSOCKLEN_T socklen_t
#else
#define MYSOCKLEN_T int
#endif /* __GLIBC__ */



/*
 * UDP NETWORKING
 */

boolean server = 0;

static int DOOMPORT = (IPPORT_USERRESERVED +0x1d );

/* static int sendsocket; */
/* static int insocket; */
static int hsocket;

static struct sockaddr_in sendaddress[MAXNETNODES];  /* table of peers */


/*
 * UDPsocket
 */
int UDPsocket (void)
{
  int	s;
  
  /* allocate a socket */
  s = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (s<0)
    I_Error ("can't create socket: %s",strerror(errno));
  
  return s;
}

/*
 * BindToLocalPort
 */
void BindToLocalPort( int s,int	port )
{
  int			v;
  struct sockaddr_in	address;
  
  memset (&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = port;
  
  v = bind (s, (void *)&address, sizeof(address));
  if (v == -1)
    I_Error ("BindToPort: bind: %s", strerror(errno));
}


/*
 * UDPPacketSend
 */
void UDPPacketSend (void)
{
  int		c;
  doomdata_t	sw;
  
  /* byte swap */
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
  
  /*   printf ("sending %i\n",gametic);   */
  
  /*
   * c = sendto (sendsocket, (void *)&sw, doomcom->datalength
   *	      ,0,(void *)&sendaddress[doomcom->remotenode]
   *	      ,sizeof(sendaddress[doomcom->remotenode]));
   */
  
  c = sendto (hsocket, (void *)&sw, doomcom->datalength,
	      0, (void *)&sendaddress[doomcom->remotenode],
	      sizeof(sendaddress[doomcom->remotenode]));
  
  /*	if (c == -1) */
  /*		I_Error ("SendPacket error: %s",strerror(errno)); */
}


/*
 * UDPPacketGet
 */
void UDPPacketGet (void)
{
  int			i;
  int			c;
  struct sockaddr_in	fromaddress;
  MYSOCKLEN_T           fromlen;
  doomdata_t		sw;
  
  fromlen = sizeof(fromaddress);

  /*
   * c = recvfrom (insocket, (void *)&sw, sizeof(sw), 0,
   *  		(struct sockaddr *)&fromaddress, &fromlen);
   */

  c = recvfrom (hsocket, (void *)&sw, sizeof(sw), 0,
		(struct sockaddr *)&fromaddress, &fromlen);
  
  if (c == -1 )
    {
      /* if (errno != EWOULDBLOCK) */
      if (errno != EWOULDBLOCK && errno != ECONNREFUSED)
	I_Error ("GetPacket: %s",strerror(errno));
      doomcom->remotenode = -1;		/* no packet */
     /* fromaddress.sin_addr.s_addr = inet_addr("0.0.0.0"); */   
     return;
    }
  
  {
    static int first=1;
    if (first)
      printf("len=%d:p=[0x%x 0x%x] \n", c, *(int*)&sw, *((int*)&sw+1));
    first = 0;
  }
  
  /* find remote node number */
  for (i=0 ; i<doomcom->numnodes ; i++)
    /* if ( fromaddress.sin_addr.s_addr == sendaddress[i].sin_addr.s_addr ) */
    if ( fromaddress.sin_addr.s_addr == sendaddress[i].sin_addr.s_addr
	 && fromaddress.sin_port == sendaddress[i].sin_port)
      break;
  
  if (i == doomcom->numnodes)
    {
      /* packet is not from one of the players (new game broadcast) */
      doomcom->remotenode = -1;		/* no packet */
      return;
    }
  
  doomcom->remotenode = i;	  /* good packet from a game player */
  doomcom->datalength = c;
  
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
}


int GetLocalAddress (void)
{
  char		        hostname[1024];
  struct hostent*	hostentry;	/* host information entry */
  int			v;
  
  /* get local address */
  v = gethostname (hostname, sizeof(hostname));
  if (v == -1)
    I_Error ("GetLocalAddress : gethostname: errno %d",errno);
  
  hostentry = gethostbyname (hostname);
  if (!hostentry)
    I_Error ("GetLocalAddress : gethostbyname: couldn't get local host");
  
  return *(int *)hostentry->h_addr_list[0];
}


/*
 * I_InitUDP
 */
void I_InitUDP (void)
{
  boolean		trueval = true;
  int			i;
  int			p;
  struct hostent*	hostentry;	/* host information entry */
  
    p = M_CheckParm ("-port");
    if (p && p<myargc-1)
      {
	DOOMPORT = atoi (myargv[p+1]);
	printf ("using alternate port %i\n",DOOMPORT);
      }
    
    /*
     * parse network game options,
     *  -net <consoleplayer> <host> <host> ...
     */
    i = M_CheckParm ("-net");
    if (!i)
      {
	/* single player game, should not happen here.... */
	netgame = false;
	doomcom->id = DOOMCOM_ID;
	doomcom->numplayers = doomcom->numnodes = 1;
	doomcom->deathmatch = false;
	doomcom->consoleplayer = 0;
	return;
      }
   
 /*  if (!strcmp(myargv[i+1], "server"))
     {
       doomcom->numnodes = atoi(myargv[i+2])-1;
       server = 1;
       doomcom->consoleplayer = 0;

       if (doomcom->numnodes >= MAXNETNODES)
       {
               char buffer[80];
               sprintf(buffer, "Bad number of players, maximum %i", MAXNETNODES);
               I_Error(buffer);
       }
 */
 
    /* parse player number and host list */
    doomcom->consoleplayer = myargv[i+1][0]-'1';
    
    doomcom->numnodes = 0;	/* this node for sure; was 1 */
    
    i++;
    while (++i < myargc && myargv[i][0] != '-')
      {
	sendaddress[doomcom->numnodes].sin_family = AF_INET;
	/* sendaddress[doomcom->numnodes].sin_port = htons(DOOMPORT); */
	sendaddress[doomcom->numnodes].sin_port = 
	  htons(DOOMPORT + doomcom->numnodes);
	if (myargv[i][0] == '.')
	  {
	    sendaddress[doomcom->numnodes].sin_addr.s_addr
	      = inet_addr (myargv[i]+1);
	  }
	else
	  {
	    hostentry = gethostbyname (myargv[i]);
	    if (!hostentry)
	      I_Error ("gethostbyname: couldn't find %s", myargv[i]);
	    sendaddress[doomcom->numnodes].sin_addr.s_addr
	      = *(int *)hostentry->h_addr_list[0];
	  }
	doomcom->numnodes++;
      }
    
    doomcom->id = DOOMCOM_ID;
    doomcom->numplayers = doomcom->numnodes;
    
    /* build message to receive */
    
    /*
     * insocket = UDPsocket ();
     * BindToLocalPort (insocket,htons(DOOMPORT));
     * ioctl (insocket, FIONBIO, &trueval);
     *
     * sendsocket = UDPsocket ();
     */

    hsocket = UDPsocket();
    BindToLocalPort (hsocket, htons(DOOMPORT + doomcom->consoleplayer));
    ioctl (hsocket, FIONBIO, &trueval);
}


