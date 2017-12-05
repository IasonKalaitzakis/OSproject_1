
#include "tinyos.h"
#include "kernel_cc.h"
#include "kernel_streams.h"



int socket_write(void* socket,const char *buf, unsigned int size);
int socket_read(void* socket, char *buf, unsigned int size);
void* socket_open(uint minor);
int socket_close(void* socket);

typedef struct unbound_socket {

	rlnode unbound_node;								//MAY NOT WORK AT ALL CATASTROPHIC FAILURE TOTAL DESTRUCTION WARNING ERROR TOXIC WASTE
}unbound;

typedef struct peer_socket {

	struct peer_socket* peerPtr;
	PipeCB* pipe_send;
	PipeCB* pipe_receive; 

}peer;

typedef struct listener_socket {

	CondVar reqs;
	rlnode reqs_queue;
}listener;

typedef struct socket_control_block {

	int refcount;
	FCB* fcb;
	uint port;
	SOCKET_TYPE type;

	union {
    	unbound Unbound;
    	peer Peer;
    	listener Listener;
  	};

} SocketCB;

static file_ops socket_ops = {

	.Open = socket_open,
  	.Read = socket_read,
  	.Write = socket_write,
  	.Close = socket_close
};



typedef struct connection_rq{

	SocketCB* client_socket;
	rlnode connection_node;  				
	CondVar conn_cv;
	int accepted;

} Connection_RQ;


static SocketCB* port_map[MAX_PORT] = {0};

void initialize_FCB_socket(FCB* fcb, SocketCB* socketcb){

	//fcb->refcount = 1;  			/**< @brief Reference counter. */
  	fcb->streamobj = socketcb;			/**< @brief The stream object (e.g., a device) */

	fcb->streamfunc = &socket_ops;
	
  	rlnode_init(& fcb->freelist_node, fcb);

} 

Fid_t sys_Socket(port_t port)
{	
	FCB* fcb[1];
	Fid_t fid[1];
	SocketCB* socketcb = (SocketCB*)xmalloc(sizeof(SocketCB));

	if(FCB_reserve(1, fid, fcb)==0){
		return -1;
	}

	socketcb->refcount = 1;
	socketcb->fcb = fcb[0];
	socketcb->port = (int)port;
	socketcb->type = UNBOUND;

	
	rlnode_init(&socketcb->Unbound.unbound_node, socketcb);
	
	initialize_FCB_socket(socketcb->fcb,socketcb);


	return fid[0];

}


void* socket_open(uint minor){return NULL;}


int sys_Listen(Fid_t sock)
{

	FCB* fcb = get_fcb(sock);
	SocketCB* socketcb;
	if(fcb != NULL){
		socketcb = fcb->streamobj;
	} else {return -1;}
	if (fcb->streamfunc != &socket_ops){return -1;}
	if(socketcb->type == LISTENER){return -1;}
	if (socketcb == NULL){return -1;}
	if(socketcb->port == NOPORT){return -1;}

	socketcb->type = LISTENER;

	if (port_map[socketcb->port] == 0){
	  	port_map[socketcb->port] = socketcb;
	}
	else {return -1;}

	socketcb->Listener.reqs = COND_INIT;
  	rlnode_init(&socketcb->Listener.reqs_queue, NULL);

	//kernel_wait(&socketcb->reqs, SCHED_PIPE);                                                             

	return 0;


}


Fid_t sys_Accept(Fid_t lsock)
{	
	FCB* lfcb = get_fcb(lsock);
	SocketCB* listener_socket;
	if(lfcb != NULL){
		listener_socket = lfcb->streamobj;
	} else {return NOFILE;}
	if (lfcb->streamfunc != &socket_ops){return NOFILE;}
	if (listener_socket == NULL){return NOFILE;}

	FCB* fcb_conn = get_fcb(sys_Socket(listener_socket->port));
	SocketCB* conn_socket;
	if(fcb_conn != NULL){
		conn_socket = fcb_conn->streamobj;
	} else {return NOFILE;}

	if(is_rlist_empty(&listener_socket->Listener.reqs_queue)){
		kernel_wait(&listener_socket->Listener.reqs, SCHED_PIPE);  
	}

	rlnode* node = rlist_pop_front(&listener_socket->Listener.reqs_queue);
	node->connrq->accepted = 1;
	conn_socket->type = PEER;

	SocketCB* peer_socket = node->connrq->client_socket;
	conn_socket->Peer.peerPtr = peer_socket;

	//PipeCB

	//conn_socket->Peer.pipe_send
	//conn_socket->Peer.pipe_receive


	kernel_signal(&node->connrq->conn_cv);

	

}


int sys_Connect(Fid_t sock, port_t port, timeout_t timeout)
{
	
	FCB* fcb = get_fcb(sock);
	SocketCB* socketcb;
	if(fcb != NULL){
		socketcb = fcb->streamobj;
	} else {return -1;}
	if (fcb->streamfunc != &socket_ops){return -1;}
	if (socketcb == NULL){return -1;}
	if(socketcb->port == NOPORT || socketcb->port > MAX_PORT || socketcb->port <0){return -1;}

	//socketcb->type = UNBOUND;

	SocketCB* listener = port_map[port];
	if (listener == NULL){return -1;}

	Connection_RQ* connrq = (Connection_RQ*)xmalloc(sizeof(Connection_RQ));

	connrq->client_socket = socketcb;
	rlnode_init(& connrq->connection_node, connrq);		
	connrq->conn_cv = COND_INIT;
	connrq->accepted = 0;

	rlist_push_back(&listener->Listener.reqs_queue, &connrq->connection_node);

	kernel_signal(&listener->Listener.reqs);

	int result;

	while(connrq->accepted==0){
		result = kernel_timedwait(&connrq->conn_cv, SCHED_PIPE, timeout);
		if(result == 1 && connrq->accepted==0){
			return -1;
		}
	}

	socketcb->type = PEER;

	return 0;

}


int sys_ShutDown(Fid_t sock, shutdown_mode how)
{
	return -1;
}

int socket_write(void* socket,const char *buf, unsigned int size){

	return -1;
}

int socket_read(void* socket, char *buf, unsigned int size){

	return -1;
}

int socket_close(void* socket){
	return -1;
}



